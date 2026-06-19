#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "server_http.h"
#include "batch_cli.h"
#include "json_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>

#ifdef _WIN32

int run_http_server(const char *port)
{
    fprintf(stderr, "HTTP server no disponible en esta compilacion (Windows).\n");
    return 1;
}

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    size_t left = len;
    while (left > 0)
    {
        ssize_t n = write(fd, p, left);
        if (n <= 0)
            return -1;
        p += n;
        left -= n;
    }
    return 0;
}

static int read_until_header_end(int fd, char *buf, size_t buflen, size_t *out_len)
{
    size_t total = 0;
    while (total + 1 < buflen)
    {
        ssize_t n = read(fd, buf + total, 1);
        if (n <= 0)
            return -1;
        total += n;
        buf[total] = '\0';
        if (total >= 4 && strstr(buf, "\r\n\r\n") != NULL)
            break;
    }
    *out_len = total;
    return 0;
}

int run_http_server(const char *port)
{
    int portnum = port ? atoi(port) : 8080;
    int listen_fd = -1;
    struct sockaddr_in addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)portnum);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 8) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    fprintf(stderr, "ProgVoraz HTTP server escuchando en puerto %d\n", portnum);

    for (;;)
    {
        int client = accept(listen_fd, NULL, NULL);
        if (client < 0)
        {
            perror("accept");
            break;
        }

        /* Leer headers */
        char header_buf[8192];
        size_t hdr_len = 0;
        if (read_until_header_end(client, header_buf, sizeof(header_buf), &hdr_len) < 0)
        {
            close(client);
            continue;
        }

        /* Parse request line */
        char method[8] = {0};
        char path[512] = {0};
        sscanf(header_buf, "%7s %511s", method, path);

        /* Obtener Content-Length si existe */
        size_t content_length = 0;
        char *cl = strcasestr(header_buf, "Content-Length:");
        if (cl)
        {
            cl += strlen("Content-Length:");
            while (*cl == ' ')
                cl++;
            content_length = (size_t)strtoul(cl, NULL, 10);
        }

        /* Puntero al comienzo del cuerpo (si ya vino parte en el buffer) */
        char *body_start = strstr(header_buf, "\r\n\r\n");
        size_t body_in_buf = 0;
        if (body_start)
        {
            body_start += 4;
            body_in_buf = hdr_len - (size_t)(body_start - header_buf);
        }

        /* Leer cuerpo completo */
        char *body = NULL;
        if (content_length > 0)
        {
            body = malloc(content_length + 1);
            if (!body)
            {
                close(client);
                continue;
            }
            if (body_in_buf > 0)
                memcpy(body, body_start, body_in_buf);
            size_t need = content_length - body_in_buf;
            size_t off = body_in_buf;
            while (need > 0)
            {
                ssize_t r = read(client, body + off, need);
                if (r <= 0)
                    break;
                need -= (size_t)r;
                off += (size_t)r;
            }
            body[content_length] = '\0';
        }

        /* Rutas implementadas */
        if (strcmp(method, "GET") == 0 && strcmp(path, "/health") == 0)
        {
            const char *resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nOK";
            write_all(client, resp, strlen(resp));
            close(client);
            if (body)
                free(body);
            continue;
        }

        if (strcmp(method, "GET") == 0 && strcmp(path, "/export_stock_json") == 0)
        {
            /* Exportar stock a un buffer temporal y devolverlo */
            const char *tmpout = "/tmp/progvoraz_export_stock.json";
            if (!export_stock_json(tmpout))
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_all(client, resp, strlen(resp));
                close(client);
                if (body)
                    free(body);
                continue;
            }
            FILE *f = fopen(tmpout, "rb");
            if (!f)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_all(client, resp, strlen(resp));
                close(client);
                if (body)
                    free(body);
                continue;
            }
            fseek(f, 0, SEEK_END);
            long flen = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = malloc((size_t)flen + 1);
            fread(buf, 1, (size_t)flen, f);
            buf[flen] = '\0';
            fclose(f);

            char hdr[256];
            int hdrlen = snprintf(hdr, sizeof(hdr), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %ld\r\n\r\n", flen);
            write_all(client, hdr, (size_t)hdrlen);
            write_all(client, buf, (size_t)flen);
            free(buf);
            close(client);
            if (body)
                free(body);
            continue;
        }

        if (strcmp(method, "POST") == 0 && strcmp(path, "/batch") == 0)
        {
            /* Guardar body en archivo temporal, ejecutar batch_process_file y devolver CSV resultante */
            char in_template[] = "/tmp/progvoraz_in_XXXXXX";
            char out_template[] = "/tmp/progvoraz_out_XXXXXX";
            int in_fd = mkstemp(in_template);
            int out_fd = mkstemp(out_template);
            if (in_fd < 0 || out_fd < 0)
            {
                if (in_fd >= 0)
                    close(in_fd);
                if (out_fd >= 0)
                    close(out_fd);
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_all(client, resp, strlen(resp));
                close(client);
                if (body)
                    free(body);
                continue;
            }

            if (content_length > 0 && body)
            {
                ssize_t w = write(in_fd, body, content_length);
                (void)w;
            }
            close(in_fd);
            close(out_fd);

            if (!batch_process_file(in_template, out_template, NULL))
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_all(client, resp, strlen(resp));
                remove(in_template);
                remove(out_template);
                close(client);
                if (body)
                    free(body);
                continue;
            }

            FILE *fout = fopen(out_template, "rb");
            if (!fout)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_all(client, resp, strlen(resp));
                remove(in_template);
                remove(out_template);
                close(client);
                if (body)
                    free(body);
                continue;
            }
            fseek(fout, 0, SEEK_END);
            long flen = ftell(fout);
            fseek(fout, 0, SEEK_SET);
            char *buf = malloc((size_t)flen + 1);
            fread(buf, 1, (size_t)flen, fout);
            buf[flen] = '\0';
            fclose(fout);

            char hdr[256];
            int hdrlen = snprintf(hdr, sizeof(hdr), "HTTP/1.1 200 OK\r\nContent-Type: text/csv\r\nContent-Length: %ld\r\n\r\n", flen);
            write_all(client, hdr, (size_t)hdrlen);
            write_all(client, buf, (size_t)flen);
            free(buf);

            remove(in_template);
            remove(out_template);
            close(client);
            if (body)
                free(body);
            continue;
        }

        /* Ruta no encontrada */
        {
            const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            write_all(client, resp, strlen(resp));
            close(client);
            if (body)
                free(body);
            continue;
        }
    }

    close(listen_fd);
    return 0;
}

#endif

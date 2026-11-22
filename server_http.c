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

/* funcion run_http_server: contiene la logica principal de esta operacion. */
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
#include <limits.h>
#include <ctype.h>
#include <poll.h>
#include <time.h>
#include <stdint.h>

/* funcion write_all: contiene la logica principal de esta operacion. */
static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    size_t left = len;
    /* while: repite el bloque mientras se cumpla left > 0. */
    while (left > 0)
    {
        ssize_t n = write(fd, p, left);
        /* if: comprueba n <= 0 antes de ejecutar esta rama. */
        if (n <= 0)
            return -1;
        p += n;
        left -= n;
    }
    return 0;
}

/* A client can disconnect before the response is written. On Unix, a raw
 * write() to a closed socket may terminate the entire server with SIGPIPE.
 * Keep file writes on write(), but send responses with MSG_NOSIGNAL. */
static int write_socket_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    while (len > 0)
    {
#ifdef MSG_NOSIGNAL
        ssize_t n = send(fd, p, len, MSG_NOSIGNAL);
#else
        ssize_t n = send(fd, p, len, 0);
#endif
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

/* funcion read_until_header_end: contiene la logica principal de esta operacion. */
/*
 * One slow or stalled client must not monopolize the sequential HTTP server.
 * Use a wall-clock deadline, not only SO_RCVTIMEO (a trickling client could
 * reset an inactivity timeout indefinitely).
 */
static int read_with_deadline(int fd, char *destination, size_t length,
                              size_t *received, int deadline_ms)
{
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    size_t total = 0;
    while (total < length)
    {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
            return -1;
        int64_t elapsed = (int64_t)(now.tv_sec - start.tv_sec) * 1000 +
                          ((int64_t)now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed >= deadline_ms)
            return -1;
        struct pollfd client = { .fd = fd, .events = POLLIN, .revents = 0 };
        int ready = poll(&client, 1, (int)(deadline_ms - elapsed));
        if (ready < 0 && errno == EINTR)
            continue;
        if (ready <= 0 || (client.revents & (POLLERR | POLLNVAL)))
            return -1;
        ssize_t n = read(fd, destination + total, length - total);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        total += (size_t)n;
    }
    if (received) *received = total;
    return 0;
}

/* Bound both header size and total processing time, checking CRLFCRLF
 * incrementally instead of repeatedly scanning the whole header. */
static int read_until_header_end(int fd, char *buf, size_t buflen, size_t *out_len)
{
    struct timespec start;
    size_t total = 0;
    if (!buf || buflen < 5 || !out_len ||
        clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return -1;
    while (total + 1 < buflen)
    {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
            return -1;
        int64_t elapsed = (int64_t)(now.tv_sec - start.tv_sec) * 1000 +
                          ((int64_t)now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed >= 5000)
            return -1;
        struct pollfd client = { .fd = fd, .events = POLLIN, .revents = 0 };
        int ready = poll(&client, 1, (int)(5000 - elapsed));
        if (ready < 0 && errno == EINTR)
            continue;
        if (ready <= 0 || (client.revents & (POLLERR | POLLNVAL)))
            return -1;
        ssize_t n = read(fd, buf + total, 1);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        total += (size_t)n;
        buf[total] = '\0';
        if (total >= 4 && memcmp(buf + total - 4, "\r\n\r\n", 4) == 0)
        {
            *out_len = total;
            return 0;
        }
    }
    return -1;
}

/*
 * Parse only full HTTP field names, never substring matches in the request
 * line or X-Content-Length. Reject duplicate Content-Length, unsupported
 * Transfer-Encoding, and malformed decimal values to prevent ambiguity.
 * Returns 0: absent, 1: present, 2: too large, -1: invalid.
 */
static int parse_http_content_length(const char *headers, size_t *out)
{
    const char *cursor;
    int seen = 0;
    if (headers == NULL || out == NULL)
        return -1;
    cursor = strstr(headers, "\r\n");
    if (cursor == NULL)
        return -1;
    cursor += 2; /* Skip the request line. */
    for (;;)
    {
        const char *end = strstr(cursor, "\r\n");
        const char *colon;
        size_t name_len;
        if (end == NULL)
            return -1;
        if (cursor == end)
            break;
        colon = memchr(cursor, ':', (size_t)(end - cursor));
        if (colon == NULL)
            return -1;
        name_len = (size_t)(colon - cursor);
        if (name_len == 17 &&
            strncasecmp(cursor, "Transfer-Encoding", 17) == 0)
            return -1; /* Chunked bodies are not implemented. */
        if (name_len == 14 &&
            strncasecmp(cursor, "Content-Length", 14) == 0)
        {
            const char *number = colon + 1;
            char *parsed_end = NULL;
            unsigned long long parsed;
            if (seen)
                return -1;
            while (number < end && (*number == ' ' || *number == '\t'))
                ++number;
            if (number == end || !isdigit((unsigned char)*number))
                return -1;
            errno = 0;
            parsed = strtoull(number, &parsed_end, 10);
            if (errno != 0 || parsed_end == number)
                return -1;
            while (parsed_end < end && (*parsed_end == ' ' || *parsed_end == '\t'))
                ++parsed_end;
            if (parsed_end != end)
                return -1;
            if (parsed > 8ULL * 1024ULL * 1024ULL)
                return 2;
            *out = (size_t)parsed;
            seen = 1;
        }
        cursor = end + 2;
    }
    return seen;
}

/* funcion run_http_server: contiene la logica principal de esta operacion. */
int run_http_server(const char *port)
{
    long parsed = 8080;
    if (port)
    {
        char *end = NULL;
        errno = 0;
        parsed = strtol(port, &end, 10);
        if (errno || end == port || *end || parsed < 1 || parsed > 65535)
        {
            fprintf(stderr, "Invalid HTTP port: %s\n", port);
            return 1;
        }
    }
    int portnum = (int)parsed;
    int listen_fd = -1;
    struct sockaddr_in addr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    /* if: comprueba listen_fd < 0 antes de ejecutar esta rama. */
    if (listen_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    /* Local endpoint by default. Remote exposure requires explicit opt-in. */
    const char *bind_all = getenv("PROGVORAZ_HTTP_BIND_ALL");
    addr.sin_addr.s_addr = htonl(bind_all && strcmp(bind_all, "1") == 0
                                ? INADDR_ANY : INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)portnum);

    /* if: comprueba bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 antes de ejecutar esta rama. */
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    /* if: comprueba listen(listen_fd, 8) < 0 antes de ejecutar esta rama. */
    if (listen(listen_fd, 8) < 0)
    {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    fprintf(stderr, "ProgVoraz HTTP server escuchando en puerto %d\n", portnum);

    /* Optional finite lifetime makes Valgrind leak checks possible after
       handling actual HTTP requests, with a normal exit (no SIGKILL). */
    unsigned long max_requests = 0, served = 0;
    const char *limit = getenv("PROGVORAZ_HTTP_MAX_REQUESTS");
    if (limit && *limit)
    {
        char *end = NULL;
        errno = 0;
        max_requests = strtoul(limit, &end, 10);
        if (errno || !end || *end || end == limit || max_requests == 0)
        {
            close(listen_fd);
            return 1;
        }
    }

    /* for: itera segun ;; para recorrer el bloque. */
    for (;;)
    {
        if (max_requests && served >= max_requests)
            break;
        int client = accept(listen_fd, NULL, NULL);
        /* if: comprueba client < 0 antes de ejecutar esta rama. */
        if (client < 0)
        {
            perror("accept");
            break;
        }
        ++served;
#ifdef SO_NOSIGPIPE
        /* macOS/BSD equivalent of Linux MSG_NOSIGNAL. */
        int no_sigpipe = 1;
        (void)setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe));
#endif

        /* Leer headers */
        char header_buf[8192];
        size_t hdr_len = 0;
        /* if: comprueba read_until_header_end(client, header_buf, sizeof(header_buf), &hdr_le... antes de ejecutar esta rama. */
        if (read_until_header_end(client, header_buf, sizeof(header_buf), &hdr_len) < 0)
        {
            close(client);
            continue;
        }

        /* Parse request line */
        char method[8] = {0};
        char path[512] = {0};
        sscanf(header_buf, "%7s %511s", method, path);

        /* Validate header names and numeric lengths before reading the body. */
        size_t content_length = 0;
        int length_state = parse_http_content_length(header_buf, &content_length);
        if (length_state < 0)
        {
            const char *resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            write_socket_all(client, resp, strlen(resp));
            close(client);
            continue;
        }
        if (length_state == 2)
        {
            const char *resp = "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            write_socket_all(client, resp, strlen(resp));
            close(client);
            continue;
        }
        if (length_state == 0 && strcmp(method, "POST") == 0 &&
            strcmp(path, "/batch") == 0)
        {
            const char *resp = "HTTP/1.1 411 Length Required\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            write_socket_all(client, resp, strlen(resp));
            close(client);
            continue;
        }

        /* Puntero al comienzo del cuerpo (si ya vino parte en el buffer) */
        char *body_start = strstr(header_buf, "\r\n\r\n");
        size_t body_in_buf = 0;
        /* if: comprueba body_start antes de ejecutar esta rama. */
        if (body_start)
        {
            body_start += 4;
            body_in_buf = hdr_len - (size_t)(body_start - header_buf);
        }

        /* Leer cuerpo completo */
        char *body = NULL;
        /* if: comprueba content_length > 0 antes de ejecutar esta rama. */
        if (content_length > 0)
        {
            body = malloc(content_length + 1);
            /* if: comprueba !body antes de ejecutar esta rama. */
            if (!body)
            {
                close(client);
                continue;
            }
            /* Defense in depth: never copy beyond advertised body length,
             * even if header parsing is later optimized to buffer the body. */
            size_t in_buffer = body_in_buf < content_length
                             ? body_in_buf : content_length;
            if (in_buffer)
                memcpy(body, body_start, in_buffer);
            size_t need = content_length - in_buffer;
            if (need > 0 &&
                read_with_deadline(client, body + in_buffer, need, NULL, 10000) < 0)
            {
                free(body);
                close(client);
                continue;
            }
            body[content_length] = '\0';
        }

        /* Rutas implementadas */
        /* if: comprueba strcmp(method, "GET") == 0 && strcmp(path, "/health") == 0 antes de ejecutar esta rama. */
        if (strcmp(method, "GET") == 0 && strcmp(path, "/health") == 0)
        {
            const char *resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nOK";
            write_socket_all(client, resp, strlen(resp));
            close(client);
            /* if: comprueba body antes de ejecutar esta rama. */
            if (body)
                free(body);
            continue;
        }

        /* if: comprueba strcmp(method, "GET") == 0 && strcmp(path, "/export_stock_json") == 0 antes de ejecutar esta rama. */
        if (strcmp(method, "GET") == 0 && strcmp(path, "/export_stock_json") == 0)
        {
            /* A unique, mode-0600 temporary path avoids collisions and
               must be unlinked on every exit, not left on disk forever. */
            char tmpout[] = "/tmp/progvoraz_export_stock_XXXXXX";
            int temp_fd = mkstemp(tmpout);
            if (temp_fd < 0)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                close(client);
                free(body);
                continue;
            }
            close(temp_fd);
            if (!export_stock_json(tmpout))
            {
                remove(tmpout);
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                close(client);
                /* if: comprueba body antes de ejecutar esta rama. */
                if (body)
                    free(body);
                continue;
            }
            FILE *f = fopen(tmpout, "rb");
            remove(tmpout); /* already open: unlink even on later read failures */
            /* if: comprueba !f antes de ejecutar esta rama. */
            if (!f)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                close(client);
                /* if: comprueba body antes de ejecutar esta rama. */
                if (body)
                    free(body);
                continue;
            }
            fseek(f, 0, SEEK_END);
            long flen = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *buf = NULL;
            if (flen < 0 || (buf = malloc((size_t)flen + 1)) == NULL || fread(buf, 1, (size_t)flen, f) != (size_t)flen)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                free(buf);
                fclose(f);
                write_socket_all(client, resp, strlen(resp));
                close(client);
                if (body)
                    free(body);
                continue;
            }
            buf[flen] = '\0';
            fclose(f);

            char hdr[256];
            int hdrlen = snprintf(hdr, sizeof(hdr), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %ld\r\n\r\n", flen);
            write_socket_all(client, hdr, (size_t)hdrlen);
            write_socket_all(client, buf, (size_t)flen);
            free(buf);
            close(client);
            /* if: comprueba body antes de ejecutar esta rama. */
            if (body)
                free(body);
            continue;
        }

        /* if: comprueba strcmp(method, "POST") == 0 && strcmp(path, "/batch") == 0 antes de ejecutar esta rama. */
        if (strcmp(method, "POST") == 0 && strcmp(path, "/batch") == 0)
        {
            /* Guardar body en archivo temporal, ejecutar batch_process_file y devolver CSV resultante */
            char in_template[] = "/tmp/progvoraz_in_XXXXXX";
            char out_template[] = "/tmp/progvoraz_out_XXXXXX";
            int in_fd = mkstemp(in_template);
            int out_fd = mkstemp(out_template);
            /* if: comprueba in_fd < 0 || out_fd < 0 antes de ejecutar esta rama. */
            if (in_fd < 0 || out_fd < 0)
            {
                /* if: comprueba in_fd >= 0 antes de ejecutar esta rama. */
                if (in_fd >= 0)
                {
                    close(in_fd);
                    remove(in_template);
                }
                if (out_fd >= 0)
                {
                    close(out_fd);
                    remove(out_template);
                }
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                close(client);
                /* if: comprueba body antes de ejecutar esta rama. */
                if (body)
                    free(body);
                continue;
            }

            /* if: comprueba content_length > 0 && body antes de ejecutar esta rama. */
            int write_ok = content_length == 0 || (body && write_all(in_fd, body, content_length) == 0);
            close(in_fd);
            close(out_fd);
            if (!write_ok)
            {
                remove(in_template);
                remove(out_template);
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                close(client);
                free(body);
                continue;
            }

            /* if: comprueba !batch_process_file(in_template, out_template, NULL) antes de ejecutar esta rama. */
            if (!batch_process_file(in_template, out_template, NULL))
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                remove(in_template);
                remove(out_template);
                close(client);
                /* if: comprueba body antes de ejecutar esta rama. */
                if (body)
                    free(body);
                continue;
            }

            FILE *fout = fopen(out_template, "rb");
            /* if: comprueba !fout antes de ejecutar esta rama. */
            if (!fout)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                write_socket_all(client, resp, strlen(resp));
                remove(in_template);
                remove(out_template);
                close(client);
                /* if: comprueba body antes de ejecutar esta rama. */
                if (body)
                    free(body);
                continue;
            }
            fseek(fout, 0, SEEK_END);
            long flen = ftell(fout);
            fseek(fout, 0, SEEK_SET);
            char *buf = NULL;
            if (flen < 0 || (buf = malloc((size_t)flen + 1)) == NULL || fread(buf, 1, (size_t)flen, fout) != (size_t)flen)
            {
                const char *resp = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                free(buf);
                fclose(fout);
                remove(in_template);
                remove(out_template);
                write_socket_all(client, resp, strlen(resp));
                close(client);
                if (body)
                    free(body);
                continue;
            }
            buf[flen] = '\0';
            fclose(fout);

            char hdr[256];
            int hdrlen = snprintf(hdr, sizeof(hdr), "HTTP/1.1 200 OK\r\nContent-Type: text/csv\r\nContent-Length: %ld\r\n\r\n", flen);
            write_socket_all(client, hdr, (size_t)hdrlen);
            write_socket_all(client, buf, (size_t)flen);
            free(buf);

            remove(in_template);
            remove(out_template);
            close(client);
            /* if: comprueba body antes de ejecutar esta rama. */
            if (body)
                free(body);
            continue;
        }

        /* Ruta no encontrada */
        {
            const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            write_socket_all(client, resp, strlen(resp));
            close(client);
            /* if: comprueba body antes de ejecutar esta rama. */
            if (body)
                free(body);
            continue;
        }
    }

    close(listen_fd);
    return 0;
}

#endif

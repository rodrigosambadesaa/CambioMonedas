#include "batch_cli.h"
#include "bigint.h"
#include "moneda_gestion.h"
#include "algoritmo_cambio.h"
#include "csv_io.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static char *trim(char *s)
{
    if (s == NULL)
        return s;
    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
        s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        *end-- = '\0';
    return s;
}

int batch_process_file(const char *rutaEntrada, const char *rutaSalida, const char *rutaLog)
{
    FILE *in = NULL;
    FILE *out = NULL;
    int ok = 1;

    if (rutaLog)
        logger_init(rutaLog);

    in = fopen(rutaEntrada, "r");
    if (!in)
    {
        logger_error("No se pudo abrir entrada %s", rutaEntrada);
        if (rutaLog)
            logger_close();
        return 0;
    }

    out = rutaSalida ? fopen(rutaSalida, "w") : stdout;
    if (!out)
    {
        logger_error("No se pudo abrir salida %s", rutaSalida);
        fclose(in);
        if (rutaLog)
            logger_close();
        return 0;
    }

    /* Cabecera CSV de salida */
    const char *hdr[] = {"modo", "moneda", "monto", "resultado", "nota"};
    csv_write_row(out, hdr, 5);

    char linea[4096];
    size_t lineno = 0;
    while (fgets(linea, sizeof(linea), in))
    {
        lineno++;
        char *p = linea;
        /* separar por comas en 4 campos max */
        char *cols[4] = {NULL, NULL, NULL, NULL};
        for (int i = 0; i < 4; i++)
        {
            char *c = strchr(p, ',');
            if (c)
            {
                *c = '\0';
                cols[i] = trim(p);
                p = c + 1;
            }
            else
            {
                cols[i] = trim(p);
                break;
            }
        }

        char modo = cols[0] && cols[0][0] ? cols[0][0] : 'a';
        char *moneda = cols[1] ? cols[1] : "";
        char *monto_txt = cols[2] ? cols[2] : "0";
        size_t min_m = 0, max_m = (size_t)-1;
        if (cols[3] && cols[3][0])
        {
            char *dash = strchr(cols[3], '-');
            if (dash)
            {
                *dash = '\0';
                min_m = (size_t)strtoul(cols[3], NULL, 10);
                max_m = (size_t)strtoul(dash + 1, NULL, 10);
            }
            else
            {
                max_m = (size_t)strtoul(cols[3], NULL, 10);
            }
        }

        /* Saltar cabecera si aparece. */
        if (lineno == 1)
        {
            char tmp0[16];
            strncpy(tmp0, cols[0] ? cols[0] : "", sizeof(tmp0) - 1);
            tmp0[sizeof(tmp0) - 1] = '\0';
            for (char *t = tmp0; *t; ++t)
                *t = (char)tolower((unsigned char)*t);
            if (strcmp(tmp0, "mode") == 0 || strcmp(tmp0, "modo") == 0 || strcmp(tmp0, "modo") == 0)
                continue;
        }

        /* Normal flow: cargar denominaciones (y stock si modo b/c). */
        /* Normalizar moneda a minusculas. */
        for (char *q = moneda; q && *q; ++q)
            *q = (char)tolower((unsigned char)*q);
        for (char *q = moneda; q && *q; ++q)
            if (*q == ' ')
                *q = '_';
        BigIntArray denom = {0};
        BigIntArray stock = {0};
        BigInt monto = {0};
        BigIntArray sol = {0};
        char resultado_buf[256] = "";

        if (!bigint_init(&monto, monto_txt))
        {
            const char *row[] = {"", "", monto_txt, "ERROR", "monto invalido"};
            csv_write_row(out, row, 5);
            logger_error("Linea %zu: monto invalido: %s", lineno, monto_txt);
            bigint_free(&monto);
            continue;
        }

        if (!cargar_denominaciones_moneda(moneda, &denom))
        {
            const char *row[] = {"", "", monto_txt, "ERROR", "moneda no encontrada"};
            csv_write_row(out, row, 5);
            logger_error("Linea %zu: moneda no encontrada: %s", lineno, moneda);
            bigint_free(&monto);
            continue;
        }

        int found = 0;
        if (modo == 'b' || modo == 'c')
        {
            if (!cargar_stock_moneda(moneda, &stock))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", "stock no encontrado"};
                csv_write_row(out, row, 5);
                logger_error("Linea %zu: stock no encontrado: %s", lineno, moneda);
                bigint_free(&monto);
                bigint_array_free(&denom);
                continue;
            }
        }

        if (modo == 'a')
        {
            if (max_m == (size_t)-1)
                found = calcular_cambio_optimo(&monto, &denom, &sol);
            else
                found = calcular_cambio_optimo_con_rango(&monto, &denom, min_m, max_m, &sol);
        }
        else
        {
            if (max_m == (size_t)-1)
                found = calcular_cambio_optimo_stock(&monto, &denom, &stock, &sol);
            else
                found = calcular_cambio_optimo_stock_con_rango(&monto, &denom, &stock, min_m, max_m, &sol);
        }

        if (found)
        {
            /* serializar solucion simple: contar monedas totales y lista */
            size_t total = 0;
            for (size_t i = 0; i < sol.len; i++)
                total += (size_t)strtoul(sol.items[i].digits, NULL, 10);
            snprintf(resultado_buf, sizeof(resultado_buf), "OK total=%zu", total);
            char note[512] = "";
            for (size_t i = 0; i < sol.len && i < 8; i++)
            {
                char tmp[64];
                snprintf(tmp, sizeof(tmp), "%s:%s;", denom.items[i].digits, sol.items[i].digits);
                strncat(note, tmp, sizeof(note) - strlen(note) - 1);
            }
            const char *row[] = {"", moneda, monto_txt, resultado_buf, note};
            csv_write_row(out, row, 5);
            logger_info("Linea %zu: OK %s %s", lineno, moneda, monto_txt);
        }
        else
        {
            const char *row[] = {"", moneda, monto_txt, "NO", "sin solucion"};
            csv_write_row(out, row, 5);
            logger_info("Linea %zu: no solution %s %s", lineno, moneda, monto_txt);
        }

        bigint_free(&monto);
        bigint_array_free(&denom);
        bigint_array_free(&stock);
        bigint_array_free(&sol);
    }

    if (in)
        fclose(in);
    if (out && out != stdout)
        fclose(out);
    if (rutaLog)
        logger_close();
    return ok;
}

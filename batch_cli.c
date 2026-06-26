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

/* funcion trim: contiene la logica principal de esta operacion. */
static char *trim(char *s)
{
    /* if: comprueba s == NULL antes de ejecutar esta rama. */
    if (s == NULL)
        return s;
    /* while: repite el bloque mientras se cumpla *s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'). */
    while (*s && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
        s++;
    char *end = s + strlen(s) - 1;
    /* while: repite el bloque mientras se cumpla end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '.... */
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        *end-- = '\0';
    return s;
}

/* funcion batch_process_file: contiene la logica principal de esta operacion. */
int batch_process_file(const char *rutaEntrada, const char *rutaSalida, const char *rutaLog)
{
    FILE *in = NULL;
    FILE *out = NULL;
    int ok = 1;
    /* if: comprueba rutaEntrada == NULL antes de ejecutar esta rama. */
    if (rutaEntrada == NULL)
        return 0;

    /* if: comprueba rutaLog antes de ejecutar esta rama. */
    if (rutaLog)
        logger_init(rutaLog);

    in = fopen(rutaEntrada, "r");
    /* if: comprueba !in antes de ejecutar esta rama. */
    if (!in)
    {
        logger_error("No se pudo abrir entrada %s", rutaEntrada);
        /* if: comprueba rutaLog antes de ejecutar esta rama. */
        if (rutaLog)
            logger_close();
        return 0;
    }

    out = rutaSalida ? fopen(rutaSalida, "w") : stdout;
    /* if: comprueba !out antes de ejecutar esta rama. */
    if (!out)
    {
        logger_error("No se pudo abrir salida %s", rutaSalida);
        fclose(in);
        /* if: comprueba rutaLog antes de ejecutar esta rama. */
        if (rutaLog)
            logger_close();
        return 0;
    }

    /* Reuse core processing implemented below via helper. */
    {
        /* Cabecera CSV de salida */
        const char *hdr[] = {"modo", "moneda", "monto", "resultado", "nota"};
        csv_write_row(out, hdr, 5);

        char linea[4096];
        size_t lineno = 0;
        /* while: repite el bloque mientras se cumpla fgets(linea, sizeof(linea), in). */
        while (fgets(linea, sizeof(linea), in))
        {
            lineno++;
            char *p = linea;
            /* separar por comas en 4 campos max */
            char *cols[4] = {NULL, NULL, NULL, NULL};
            /* for: itera segun int i = 0; i < 4; i++ para recorrer el bloque. */
            for (int i = 0; i < 4; i++)
            {
                char *c = strchr(p, ',');
                /* if: comprueba c antes de ejecutar esta rama. */
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
            /* if: comprueba cols[3] && cols[3][0] antes de ejecutar esta rama. */
            if (cols[3] && cols[3][0])
            {
                char *dash = strchr(cols[3], '-');
                /* if: comprueba dash antes de ejecutar esta rama. */
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
            /* if: comprueba lineno == 1 antes de ejecutar esta rama. */
            if (lineno == 1)
            {
                char tmp0[16];
                strncpy(tmp0, cols[0] ? cols[0] : "", sizeof(tmp0) - 1);
                tmp0[sizeof(tmp0) - 1] = '\0';
                /* for: itera segun char *t = tmp0; *t; ++t para recorrer el bloque. */
                for (char *t = tmp0; *t; ++t)
                    *t = (char)tolower((unsigned char)*t);
                /* if: comprueba strcmp(tmp0, "mode") == 0 || strcmp(tmp0, "modo") == 0 || strcmp(tmp0... antes de ejecutar esta rama. */
                if (strcmp(tmp0, "mode") == 0 || strcmp(tmp0, "modo") == 0 || strcmp(tmp0, "modo") == 0)
                    continue;
            }

            /* Normal flow: cargar denominaciones (y stock si modo b/c). */
            /* Normalizar moneda a minusculas. */
            /* for: itera segun char *q = moneda; q && *q; ++q para recorrer el bloque. */
            for (char *q = moneda; q && *q; ++q)
                *q = (char)tolower((unsigned char)*q);
            /* for: itera segun char *q = moneda; q && *q; ++q para recorrer el bloque. */
            for (char *q = moneda; q && *q; ++q)
                /* if: comprueba *q == ' ' antes de ejecutar esta rama. */
                if (*q == ' ')
                    *q = '_';
            BigIntArray denom = {0};
            BigIntArray stock = {0};
            BigInt monto = {0};
            BigIntArray sol = {0};
            char resultado_buf[256] = "";

            /* if: comprueba !bigint_init(&monto, monto_txt) antes de ejecutar esta rama. */
            if (!bigint_init(&monto, monto_txt))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", "monto invalido"};
                csv_write_row(out, row, 5);
                logger_error("Linea %zu: monto invalido: %s", lineno, monto_txt);
                bigint_free(&monto);
                continue;
            }

            /* if: comprueba !cargar_denominaciones_moneda(moneda, &denom) antes de ejecutar esta rama. */
            if (!cargar_denominaciones_moneda(moneda, &denom))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", "moneda no encontrada"};
                csv_write_row(out, row, 5);
                logger_error("Linea %zu: moneda no encontrada: %s", lineno, moneda);
                bigint_free(&monto);
                continue;
            }

            int found = 0;
            /* if: comprueba modo == 'b' || modo == 'c' antes de ejecutar esta rama. */
            if (modo == 'b' || modo == 'c')
            {
                /* if: comprueba !cargar_stock_moneda(moneda, &stock) antes de ejecutar esta rama. */
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

            /* if: comprueba modo == 'a' antes de ejecutar esta rama. */
            if (modo == 'a')
            {
                /* if: comprueba max_m == (size_t)-1 antes de ejecutar esta rama. */
                if (max_m == (size_t)-1)
                    found = calcular_cambio_optimo(&monto, &denom, &sol);
                else
                    found = calcular_cambio_optimo_con_rango(&monto, &denom, min_m, max_m, &sol);
            }
            else
            {
                /* if: comprueba max_m == (size_t)-1 antes de ejecutar esta rama. */
                if (max_m == (size_t)-1)
                    found = calcular_cambio_optimo_stock(&monto, &denom, &stock, &sol);
                else
                    found = calcular_cambio_optimo_stock_con_rango(&monto, &denom, &stock, min_m, max_m, &sol);
            }

            /* if: comprueba found antes de ejecutar esta rama. */
            if (found)
            {
                /* serializar solucion simple: contar monedas totales y lista */
                size_t total = 0;
                /* for: itera segun size_t i = 0; i < sol.len; i++ para recorrer el bloque. */
                for (size_t i = 0; i < sol.len; i++)
                    total += (size_t)strtoul(sol.items[i].digits, NULL, 10);
                snprintf(resultado_buf, sizeof(resultado_buf), "OK total=%zu", total);
                char note[512] = "";
                /* for: itera segun size_t i = 0; i < sol.len && i < 8; i++ para recorrer el bloque. */
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
    }

    /* if: comprueba in antes de ejecutar esta rama. */
    if (in)
        fclose(in);
    /* if: comprueba out && out != stdout antes de ejecutar esta rama. */
    if (out && out != stdout)
        fclose(out);
    /* if: comprueba rutaLog antes de ejecutar esta rama. */
    if (rutaLog)
        logger_close();
    return ok;
}

/* funcion batch_process_stream: contiene la logica principal de esta operacion. */
int batch_process_stream(const char *rutaLog)
{
    /* if: comprueba rutaLog antes de ejecutar esta rama. */
    if (rutaLog)
        logger_init(rutaLog);

    /* Cabecera CSV de salida */
    const char *hdr[] = {"modo", "moneda", "monto", "resultado", "nota"};
    csv_write_row(stdout, hdr, 5);

    char linea[4096];
    size_t lineno = 0;
    /* while: repite el bloque mientras se cumpla fgets(linea, sizeof(linea), stdin). */
    while (fgets(linea, sizeof(linea), stdin))
    {
        lineno++;
        char *p = linea;
        char *cols[4] = {NULL, NULL, NULL, NULL};
        /* for: itera segun int i = 0; i < 4; i++ para recorrer el bloque. */
        for (int i = 0; i < 4; i++)
        {
            char *c = strchr(p, ',');
            /* if: comprueba c antes de ejecutar esta rama. */
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
        /* if: comprueba cols[3] && cols[3][0] antes de ejecutar esta rama. */
        if (cols[3] && cols[3][0])
        {
            char *dash = strchr(cols[3], '-');
            /* if: comprueba dash antes de ejecutar esta rama. */
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

        /* if: comprueba lineno == 1 antes de ejecutar esta rama. */
        if (lineno == 1)
        {
            char tmp0[16];
            strncpy(tmp0, cols[0] ? cols[0] : "", sizeof(tmp0) - 1);
            tmp0[sizeof(tmp0) - 1] = '\0';
            /* for: itera segun char *t = tmp0; *t; ++t para recorrer el bloque. */
            for (char *t = tmp0; *t; ++t)
                *t = (char)tolower((unsigned char)*t);
            /* if: comprueba strcmp(tmp0, "mode") == 0 || strcmp(tmp0, "modo") == 0 antes de ejecutar esta rama. */
            if (strcmp(tmp0, "mode") == 0 || strcmp(tmp0, "modo") == 0)
                continue;
        }

        /* for: itera segun char *q = moneda; q && *q; ++q para recorrer el bloque. */
        for (char *q = moneda; q && *q; ++q)
            *q = (char)tolower((unsigned char)*q);
        /* for: itera segun char *q = moneda; q && *q; ++q para recorrer el bloque. */
        for (char *q = moneda; q && *q; ++q)
            /* if: comprueba *q == ' ' antes de ejecutar esta rama. */
            if (*q == ' ')
                *q = '_';

        BigIntArray denom = {0};
        BigIntArray stock = {0};
        BigInt monto = {0};
        BigIntArray sol = {0};

        /* if: comprueba !bigint_init(&monto, monto_txt) antes de ejecutar esta rama. */
        if (!bigint_init(&monto, monto_txt))
        {
            const char *row[] = {"", "", monto_txt, "ERROR", "monto invalido"};
            csv_write_row(stdout, row, 5);
            logger_error("Linea %zu: monto invalido: %s", lineno, monto_txt);
            bigint_free(&monto);
            continue;
        }

        /* if: comprueba !cargar_denominaciones_moneda(moneda, &denom) antes de ejecutar esta rama. */
        if (!cargar_denominaciones_moneda(moneda, &denom))
        {
            const char *row[] = {"", "", monto_txt, "ERROR", "moneda no encontrada"};
            csv_write_row(stdout, row, 5);
            logger_error("Linea %zu: moneda no encontrada: %s", lineno, moneda);
            bigint_free(&monto);
            continue;
        }

        int found = 0;
        /* if: comprueba modo == 'b' || modo == 'c' antes de ejecutar esta rama. */
        if (modo == 'b' || modo == 'c')
        {
            /* if: comprueba !cargar_stock_moneda(moneda, &stock) antes de ejecutar esta rama. */
            if (!cargar_stock_moneda(moneda, &stock))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", "stock no encontrado"};
                csv_write_row(stdout, row, 5);
                logger_error("Linea %zu: stock no encontrado: %s", lineno, moneda);
                bigint_free(&monto);
                bigint_array_free(&denom);
                continue;
            }
        }

        /* if: comprueba modo == 'a' antes de ejecutar esta rama. */
        if (modo == 'a')
        {
            /* if: comprueba max_m == (size_t)-1 antes de ejecutar esta rama. */
            if (max_m == (size_t)-1)
                found = calcular_cambio_optimo(&monto, &denom, &sol);
            else
                found = calcular_cambio_optimo_con_rango(&monto, &denom, min_m, max_m, &sol);
        }
        else
        {
            /* if: comprueba max_m == (size_t)-1 antes de ejecutar esta rama. */
            if (max_m == (size_t)-1)
                found = calcular_cambio_optimo_stock(&monto, &denom, &stock, &sol);
            else
                found = calcular_cambio_optimo_stock_con_rango(&monto, &denom, &stock, min_m, max_m, &sol);
        }

        /* if: comprueba found antes de ejecutar esta rama. */
        if (found)
        {
            size_t total = 0;
            /* for: itera segun size_t i = 0; i < sol.len; i++ para recorrer el bloque. */
            for (size_t i = 0; i < sol.len; i++)
                total += (size_t)strtoul(sol.items[i].digits, NULL, 10);
            char resultado_buf[256];
            snprintf(resultado_buf, sizeof(resultado_buf), "OK total=%zu", total);
            char note[512] = "";
            /* for: itera segun size_t i = 0; i < sol.len && i < 8; i++ para recorrer el bloque. */
            for (size_t i = 0; i < sol.len && i < 8; i++)
            {
                char tmp[64];
                snprintf(tmp, sizeof(tmp), "%s:%s;", denom.items[i].digits, sol.items[i].digits);
                strncat(note, tmp, sizeof(note) - strlen(note) - 1);
            }
            const char *row[] = {"", moneda, monto_txt, resultado_buf, note};
            csv_write_row(stdout, row, 5);
            logger_info("Linea %zu: OK %s %s", lineno, moneda, monto_txt);
        }
        else
        {
            const char *row[] = {"", moneda, monto_txt, "NO", "sin solucion"};
            csv_write_row(stdout, row, 5);
            logger_info("Linea %zu: no solution %s %s", lineno, moneda, monto_txt);
        }

        bigint_free(&monto);
        bigint_array_free(&denom);
        bigint_array_free(&stock);
        bigint_array_free(&sol);
    }

    /* if: comprueba rutaLog antes de ejecutar esta rama. */
    if (rutaLog)
        logger_close();
    return 1;
}

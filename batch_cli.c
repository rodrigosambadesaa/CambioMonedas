#include "batch_cli.h"
#include "bigint.h"
#include "moneda_gestion.h"
#include "algoritmo_cambio.h"
#include "csv_io.h"
#include "logger.h"
#include "progvoraz_locale.h"
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
    size_t len = strlen(s);
    if (len == 0)
        return s;
    char *end = s + len - 1;
    /* while: repite el bloque mientras se cumpla end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'). */
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        *end-- = '\0';
    return s;
}

static void write_csv_header(FILE *out)
{
    const char *hdr_es[] = {"modo", "moneda", "monto", "resultado", "nota"};
    const char *hdr_en[] = {"mode", "currency", "amount", "result", "note"};
    csv_write_row(out, progvoraz_lang_is_english() ? hdr_en : hdr_es, 5);
}

static const char *batch_note_invalid_amount(void)
{
    return progvoraz_tr("monto invalido", "invalid amount");
}

static const char *batch_note_currency_not_found(void)
{
    return progvoraz_tr("moneda no encontrada", "currency not found");
}

static const char *batch_note_stock_not_found(void)
{
    return progvoraz_tr("stock no encontrado", "stock not found");
}

static const char *batch_note_no_solution(void)
{
    return progvoraz_tr("sin solucion", "no solution");
}

static char *batch_read_line(FILE *fp)
{
    size_t capacidad = 4096;
    size_t len = 0;
    char *buffer = (char *)malloc(capacidad);
    int ch;

    if (buffer == NULL || fp == NULL)
    {
        free(buffer);
        return NULL;
    }

    while ((ch = fgetc(fp)) != EOF)
    {
        if (ch == '\r')
            continue;
        if (ch == '\n')
            break;

        if (len + 1 >= capacidad)
        {
            size_t nueva_capacidad = capacidad * 2;
            char *nuevo = (char *)realloc(buffer, nueva_capacidad);
            if (nuevo == NULL)
            {
                free(buffer);
                return NULL;
            }
            buffer = nuevo;
            capacidad = nueva_capacidad;
        }

        buffer[len++] = (char)ch;
    }

    if (ch == EOF && len == 0)
    {
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';
    return buffer;
}

static void batch_build_result_summary(const BigIntArray *sol, char *resultado_buf, size_t resultado_buf_len)
{
    BigInt total = {0};

    if (resultado_buf == NULL || resultado_buf_len == 0)
        return;

    resultado_buf[0] = '\0';
    if (!bigint_init(&total, "0"))
        return;

    for (size_t i = 0; i < sol->len; i++)
    {
        BigInt nuevo_total = {0};
        if (!bigint_add(&total, &sol->items[i], &nuevo_total))
            break;
        bigint_free(&total);
        total = nuevo_total;
    }

    snprintf(resultado_buf, resultado_buf_len, "OK total=%s", total.digits != NULL ? total.digits : "0");
    bigint_free(&total);
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
        write_csv_header(out);

        size_t lineno = 0;
        char *linea = NULL;
        while ((linea = batch_read_line(in)) != NULL)
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
                if (strcmp(tmp0, "mode") == 0 || strcmp(tmp0, "modo") == 0)
                {
                    free(linea);
                    linea = NULL;
                    continue;
                }
            }

            /* Normal flow: cargar denominaciones (y stock si modo b/c). */
            moneda = (char *)progvoraz_map_currency_key(moneda);
            BigIntArray denom = {0};
            BigIntArray stock = {0};
            BigInt monto = {0};
            BigIntArray sol = {0};
            char resultado_buf[256] = "";

            /* if: comprueba !bigint_init(&monto, monto_txt) antes de ejecutar esta rama. */
            if (!bigint_init(&monto, monto_txt))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", batch_note_invalid_amount()};
                csv_write_row(out, row, 5);
                logger_error("Line %zu: invalid amount: %s", lineno, monto_txt);
                bigint_free(&monto);
                free(linea);
                continue;
            }

            /* if: comprueba !cargar_denominaciones_moneda(moneda, &denom) antes de ejecutar esta rama. */
            if (!cargar_denominaciones_moneda(moneda, &denom))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", batch_note_currency_not_found()};
                csv_write_row(out, row, 5);
                logger_error("Line %zu: currency not found: %s", lineno, moneda);
                bigint_free(&monto);
                free(linea);
                continue;
            }

            int found = 0;
            /* if: comprueba modo == 'b' || modo == 'c' antes de ejecutar esta rama. */
            if (modo == 'b' || modo == 'c')
            {
                /* if: comprueba !cargar_stock_moneda(moneda, &stock) antes de ejecutar esta rama. */
                if (!cargar_stock_moneda(moneda, &stock))
                {
                    const char *row[] = {"", "", monto_txt, "ERROR", batch_note_stock_not_found()};
                    csv_write_row(out, row, 5);
                    logger_error("Line %zu: stock not found: %s", lineno, moneda);
                    bigint_free(&monto);
                    bigint_array_free(&denom);
                    free(linea);
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
                batch_build_result_summary(&sol, resultado_buf, sizeof(resultado_buf));
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
                const char *row[] = {"", moneda, monto_txt, "NO", batch_note_no_solution()};
                csv_write_row(out, row, 5);
                logger_info("Linea %zu: no solution %s %s", lineno, moneda, monto_txt);
            }

            bigint_free(&monto);
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            bigint_array_free(&sol);
            free(linea);
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
    write_csv_header(stdout);

    size_t lineno = 0;
    char *linea = NULL;
    while ((linea = batch_read_line(stdin)) != NULL)
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
            {
                free(linea);
                linea = NULL;
                continue;
            }
        }

        moneda = (char *)progvoraz_map_currency_key(moneda);

        BigIntArray denom = {0};
        BigIntArray stock = {0};
        BigInt monto = {0};
        BigIntArray sol = {0};

        /* if: comprueba !bigint_init(&monto, monto_txt) antes de ejecutar esta rama. */
        if (!bigint_init(&monto, monto_txt))
        {
            const char *row[] = {"", "", monto_txt, "ERROR", batch_note_invalid_amount()};
            csv_write_row(stdout, row, 5);
            logger_error("Line %zu: invalid amount: %s", lineno, monto_txt);
            bigint_free(&monto);
            free(linea);
            continue;
        }

        /* if: comprueba !cargar_denominaciones_moneda(moneda, &denom) antes de ejecutar esta rama. */
        if (!cargar_denominaciones_moneda(moneda, &denom))
        {
            const char *row[] = {"", "", monto_txt, "ERROR", batch_note_currency_not_found()};
            csv_write_row(stdout, row, 5);
            logger_error("Line %zu: currency not found: %s", lineno, moneda);
            bigint_free(&monto);
            free(linea);
            continue;
        }

        int found = 0;
        /* if: comprueba modo == 'b' || modo == 'c' antes de ejecutar esta rama. */
        if (modo == 'b' || modo == 'c')
        {
            /* if: comprueba !cargar_stock_moneda(moneda, &stock) antes de ejecutar esta rama. */
            if (!cargar_stock_moneda(moneda, &stock))
            {
                const char *row[] = {"", "", monto_txt, "ERROR", batch_note_stock_not_found()};
                csv_write_row(stdout, row, 5);
                logger_error("Line %zu: stock not found: %s", lineno, moneda);
                bigint_free(&monto);
                bigint_array_free(&denom);
                free(linea);
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
            char resultado_buf[256];
            batch_build_result_summary(&sol, resultado_buf, sizeof(resultado_buf));
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
            const char *row[] = {"", moneda, monto_txt, "NO", batch_note_no_solution()};
            csv_write_row(stdout, row, 5);
            logger_info("Linea %zu: no solution %s %s", lineno, moneda, monto_txt);
        }

        bigint_free(&monto);
        bigint_array_free(&denom);
        bigint_array_free(&stock);
        bigint_array_free(&sol);
        free(linea);
    }

    /* if: comprueba rutaLog antes de ejecutar esta rama. */
    if (rutaLog)
        logger_close();
    return 1;
}

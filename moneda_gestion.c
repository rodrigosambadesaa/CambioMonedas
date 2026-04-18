#if !defined(_WIN32)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include "moneda_gestion.h"

#define MAX_TOKEN 256
#define MAX_DENOMINACIONES 64
#define MAX_LINEA_STOCK 256
#define MAX_LINEAS_STOCK 512

/* es_texto_bigint_valido: documenta el comportamiento principal y validaciones de entrada. */
static int es_texto_bigint_valido(const char *texto)
{
    size_t i;

    if (texto == NULL || *texto == '\0')
        return 0;

    for (i = 0; texto[i] != '\0'; i++)
    {
        if (texto[i] < '0' || texto[i] > '9')
            return 0;
    }

    return 1;
}

/* *duplicar_cadena: documenta el comportamiento principal y validaciones de entrada. */
static char *duplicar_cadena(const char *texto)
{
    size_t len;
    char *copia;

    if (texto == NULL)
        return NULL;

    len = strlen(texto) + 1;
    copia = (char *)malloc(len);
    if (copia == NULL)
        return NULL;

    memcpy(copia, texto, len);
    return copia;
}

/* recortar_fin_linea: documenta el comportamiento principal y validaciones de entrada. */
static void recortar_fin_linea(char *texto)
{
    if (texto == NULL)
        return;

    texto[strcspn(texto, "\r\n")] = '\0';
}

/* cargar_datos_moneda: documenta el comportamiento principal y validaciones de entrada. */
static int cargar_datos_moneda(const char *archivo, const char *nombreMoneda, int convertirMenosUnoACero, BigIntArray *resultado)
{
    FILE *fp;
    char token[MAX_TOKEN];
    char valores[MAX_DENOMINACIONES][MAX_TOKEN];
    size_t cantidad = 0;
    int enSeccion = 0;

    if (archivo == NULL || nombreMoneda == NULL || *nombreMoneda == '\0' || resultado == NULL)
        return 0;

    fp = fopen(archivo, "r");
    if (fp == NULL)
        return 0;

    /* while: documenta el comportamiento principal y validaciones de entrada. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (!enSeccion)
        {
            if (strcmp(token, nombreMoneda) == 0)
                enSeccion = 1;
            continue;
        }

        if (convertirMenosUnoACero && strcmp(token, "-1") == 0)
            strcpy(token, "0");

        if (!es_texto_bigint_valido(token))
            break;

        if (cantidad >= MAX_DENOMINACIONES)
            break;

        strcpy(valores[cantidad], token);
        cantidad++;
    }

    fclose(fp);

    if (!enSeccion || cantidad == 0)
        return 0;

    if (!bigint_array_create(resultado, cantidad))
        return 0;

    for (size_t i = 0; i < cantidad; i++)
    {
        BigInt tmp = {0};
        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (!bigint_init(&tmp, valores[i]))
        {
            bigint_array_free(resultado);
            return 0;
        }
        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (!bigint_array_set(resultado, i, &tmp))
        {
            bigint_free(&tmp);
            bigint_array_free(resultado);
            return 0;
        }
        bigint_free(&tmp);
    }

    return 1;
}

/* cargar_denominaciones_moneda: documenta el comportamiento principal y validaciones de entrada. */
int cargar_denominaciones_moneda(const char *nombreMoneda, BigIntArray *resultado)
{
    return cargar_datos_moneda("monedas.txt", nombreMoneda, 0, resultado);
}

/* cargar_stock_moneda: documenta el comportamiento principal y validaciones de entrada. */
int cargar_stock_moneda(const char *nombreMoneda, BigIntArray *resultado)
{
    return cargar_datos_moneda("stock.txt", nombreMoneda, 1, resultado);
}

/* actualizar_stock_moneda: documenta el comportamiento principal y validaciones de entrada. */
int actualizar_stock_moneda(const char *nombreMoneda, const BigIntArray *stock)
{
    FILE *archivo;
    char *lineas[MAX_LINEAS_STOCK];
    char buffer[MAX_LINEA_STOCK];
    char comparable[MAX_LINEA_STOCK];
    size_t i;
    size_t totalLineas = 0;
    int ok = 1;
    int actualizado = 0;

    if (nombreMoneda == NULL || *nombreMoneda == '\0' ||
        stock == NULL || stock->items == NULL || stock->len == 0)
        return 0;

    archivo = fopen("stock.txt", "r+");
    if (archivo == NULL)
        return 0;

    for (i = 0; i < MAX_LINEAS_STOCK; i++)
        lineas[i] = NULL;

    /* while: documenta el comportamiento principal y validaciones de entrada. */
    while (fgets(buffer, sizeof(buffer), archivo) != NULL)
    {
        size_t len = strlen(buffer);

        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (totalLineas >= MAX_LINEAS_STOCK)
        {
            ok = 0;
            break;
        }

        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (len > 0 && buffer[len - 1] != '\n' && !feof(archivo))
        {
            ok = 0;
            break;
        }

        lineas[totalLineas] = duplicar_cadena(buffer);
        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (lineas[totalLineas] == NULL)
        {
            ok = 0;
            break;
        }
        totalLineas++;
    }

    /* if: documenta el comportamiento principal y validaciones de entrada. */
    if (ok)
    {
        for (size_t linea = 0; linea < totalLineas && !actualizado; linea++)
        {
            if (lineas[linea] == NULL)
            {
                ok = 0;
                break;
            }

            strncpy(comparable, lineas[linea], sizeof(comparable) - 1);
            comparable[sizeof(comparable) - 1] = '\0';
            recortar_fin_linea(comparable);

            if (strcmp(comparable, nombreMoneda) != 0)
                continue;

            /* if: documenta el comportamiento principal y validaciones de entrada. */
            if (stock->len > totalLineas - (linea + 1))
            {
                ok = 0;
                break;
            }

            for (i = 0; i < stock->len; i++)
            {
                size_t objetivo = linea + 1 + i;
                char nuevaLinea[MAX_LINEA_STOCK];

                if (bigint_is_zero(&stock->items[i]))
                    snprintf(nuevaLinea, sizeof(nuevaLinea), "-1\n");
                else
                    snprintf(nuevaLinea, sizeof(nuevaLinea), "%s\n", stock->items[i].digits);

                free(lineas[objetivo]);
                lineas[objetivo] = duplicar_cadena(nuevaLinea);
                /* if: documenta el comportamiento principal y validaciones de entrada. */
                if (lineas[objetivo] == NULL)
                {
                    ok = 0;
                    break;
                }
            }

            if (ok)
                actualizado = 1;
        }
    }

    /* if: documenta el comportamiento principal y validaciones de entrada. */
    if (!ok || !actualizado)
    {
        for (i = 0; i < totalLineas; i++)
            free(lineas[i]);
        fclose(archivo);
        return 0;
    }

    rewind(archivo);
    for (i = 0; i < totalLineas; i++)
    {
        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (fputs(lineas[i], archivo) == EOF)
        {
            ok = 0;
            break;
        }
    }

    if (ok && fflush(archivo) != 0)
        ok = 0;

    /* if: documenta el comportamiento principal y validaciones de entrada. */
    if (ok)
    {
        long fin = ftell(archivo);
        /* if: documenta el comportamiento principal y validaciones de entrada. */
        if (fin < 0)
        {
            ok = 0;
        }
        else
        {
#ifdef _WIN32
            if (_chsize_s(_fileno(archivo), (__int64)fin) != 0)
                ok = 0;
#else
            if (ftruncate(fileno(archivo), (off_t)fin) != 0)
                ok = 0;
#endif
        }
    }

    for (i = 0; i < totalLineas; i++)
        free(lineas[i]);
    fclose(archivo);

    return ok ? 1 : 0;
}

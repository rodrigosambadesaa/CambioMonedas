#if !defined(_WIN32)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

/* es_texto_bigint_valido: Funcion auxiliar que verifica si un string es un numero grande valido. */
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

/* duplicar_cadena: Asigna memoria y copia el contenido de una cadena en ella. */
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

/* recortar_fin_linea: Elimina saltos de linea CR o LF al final del string. */
static void recortar_fin_linea(char *texto)
{
    if (texto == NULL)
        return;

    texto[strcspn(texto, "\r\n")] = '\0';
}

/* cargar_datos_moneda: Nucleo lector para monedas y stock. */
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

    /* Bucle principal: va extrayendo palabras (tokens) limitadas a 255 caracteres separados por espacios o saltos de linea. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        /* Si aun no hemos encontrado la declaracion de la moneda que buscamos... */
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
        /* Intenta inicializar tmp con los digitos guardados en valores[i]. Si falla la reserva dentro de init... */
        if (!bigint_init(&tmp, valores[i]))
        {
            bigint_array_free(resultado);
            return 0;
        }
        /* Asigna la estructura tmp recien creada al indice 'i' del array destino 'resultado'. Si falla... */
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

/* cargar_denominaciones_moneda: Wrapper que delega en cargar_datos_moneda apuntando a "monedas.txt". */
int cargar_denominaciones_moneda(const char *nombreMoneda, BigIntArray *resultado)
{
    return cargar_datos_moneda("monedas.txt", nombreMoneda, 0, resultado);
}

/* cargar_stock_moneda: Wrapper que delega en cargar_datos_moneda apuntando a "stock.txt". */
int cargar_stock_moneda(const char *nombreMoneda, BigIntArray *resultado)
{
    return cargar_datos_moneda("stock.txt", nombreMoneda, 1, resultado);
}

/* actualizar_stock_moneda: Funcion que reescribe stock.txt mediante temporal para evitar corrupcion parcial. */
int actualizar_stock_moneda(const char *nombreMoneda, const BigIntArray *stock)
{
    FILE *archivoLectura;
    FILE *archivoTemporal = NULL;
    char *lineas[MAX_LINEAS_STOCK];
    char buffer[MAX_LINEA_STOCK];
    char comparable[MAX_LINEA_STOCK];
    const char *rutaStock = "stock.txt";
    const char *rutaTemporal = "stock.txt.tmp";
    size_t i;
    size_t totalLineas = 0;
    int ok = 1;
    int actualizado = 0;

    if (nombreMoneda == NULL || *nombreMoneda == '\0' ||
        stock == NULL || stock->items == NULL || stock->len == 0)
        return 0;

    archivoLectura = fopen(rutaStock, "r");
    if (archivoLectura == NULL)
        return 0;

    for (i = 0; i < MAX_LINEAS_STOCK; i++)
        lineas[i] = NULL;

    /* Bucle principal de lectura: carga todo el archivo stock.txt en la matriz dinamica 'lineas'. */
    while (fgets(buffer, sizeof(buffer), archivoLectura) != NULL)
    {
        size_t len = strlen(buffer);

        /* Limita la lectura a MAX_LINEAS_STOCK para evitar buffer overflow en el arreglo 'lineas'. */
        if (totalLineas >= MAX_LINEAS_STOCK)
        {
            ok = 0;
            break;
        }

        /* Si la linea es demasiado larga, fgets no habra incluido un salto de linea al final (salvo EOF). */
        if (len > 0 && buffer[len - 1] != '\n' && !feof(archivoLectura))
        {
            ok = 0;
            break;
        }

        lineas[totalLineas] = duplicar_cadena(buffer);
        /* Verifica que malloc no haya fallado durante 'duplicar_cadena'. */
        if (lineas[totalLineas] == NULL)
        {
            ok = 0;
            break;
        }
        totalLineas++;
    }

    /* Fase de modificación: busca y altera las lineas del stock de la moneda si no hubo errores previos. */
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

            /* Verificamos que queden suficientes lineas debajo de la etiqueta para reemplazar todo el stock sin pasarnos. */
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
                /* Si no habia memoria para crear esta nueva linea... */
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

    /* Fase de aborto o limpieza temprana. */
    if (!ok || !actualizado)
    {
        for (i = 0; i < totalLineas; i++)
            free(lineas[i]);
        fclose(archivoLectura);
        return 0;
    }

    fclose(archivoLectura);
    archivoLectura = NULL;

    archivoTemporal = fopen(rutaTemporal, "w");
    if (archivoTemporal == NULL)
        ok = 0;

    for (i = 0; ok && i < totalLineas; i++)
    {
        /* Escribe secuencialmente la linea en el archivo temporal. */
        if (fputs(lineas[i], archivoTemporal) == EOF)
        {
            ok = 0;
        }
    }

    if (ok && fflush(archivoTemporal) != 0)
        ok = 0;

    if (archivoTemporal != NULL && fclose(archivoTemporal) != 0)
        ok = 0;
    archivoTemporal = NULL;

    if (ok)
    {
#ifdef _WIN32
        if (remove(rutaStock) != 0)
            ok = 0;
#endif
        if (ok && rename(rutaTemporal, rutaStock) != 0)
        {
            FILE *archivoDestino = fopen(rutaStock, "w");
            if (archivoDestino == NULL)
            {
                ok = 0;
            }
            else
            {
                for (i = 0; i < totalLineas; i++)
                {
                    if (fputs(lineas[i], archivoDestino) == EOF)
                    {
                        ok = 0;
                        break;
                    }
                }
                if (ok && fflush(archivoDestino) != 0)
                    ok = 0;
                if (fclose(archivoDestino) != 0)
                    ok = 0;
            }
        }
    }

    if (!ok)
        remove(rutaTemporal);

    for (i = 0; i < totalLineas; i++)
        free(lineas[i]);

    if (archivoLectura != NULL)
        fclose(archivoLectura);
    if (archivoTemporal != NULL)
        fclose(archivoTemporal);

    return (ok && actualizado) ? 1 : 0;
}

int validar_consistencia_moneda(const char *nombreMoneda)
{
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    int ok = 0;

    if (nombreMoneda == NULL || *nombreMoneda == '\0')
        return 0;

    if (!cargar_denominaciones_moneda(nombreMoneda, &denom))
        goto fin;

    if (!cargar_stock_moneda(nombreMoneda, &stock))
        goto fin;

    if (denom.len != stock.len || denom.len == 0)
        goto fin;

    for (size_t i = 1; i < denom.len; i++)
    {
        if (bigint_compare(&denom.items[i - 1], &denom.items[i]) <= 0)
            goto fin;
    }

    ok = 1;

fin:
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    return ok;
}

static const char *ruta_o_default(const char *ruta, const char *defecto)
{
    if (ruta == NULL || *ruta == '\0')
        return defecto;
    return ruta;
}

static int copiar_archivo_binario(const char *origen, const char *destino)
{
    FILE *in = NULL;
    FILE *out = NULL;
    unsigned char buffer[4096];
    size_t leidos;
    int ok = 1;

    if (origen == NULL || destino == NULL)
        return 0;

    in = fopen(origen, "rb");
    if (in == NULL)
        return 0;

    out = fopen(destino, "wb");
    if (out == NULL)
    {
        fclose(in);
        return 0;
    }

    while ((leidos = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        if (fwrite(buffer, 1, leidos, out) != leidos)
        {
            ok = 0;
            break;
        }
    }

    if (ferror(in))
        ok = 0;

    if (fclose(out) != 0)
        ok = 0;
    fclose(in);

    if (!ok)
        remove(destino);

    return ok;
}

int crear_snapshot_stock(const char *rutaSnapshot)
{
    const char *ruta = ruta_o_default(rutaSnapshot, "stock_snapshot.txt");
    return copiar_archivo_binario("stock.txt", ruta);
}

int restaurar_snapshot_stock(const char *rutaSnapshot)
{
    const char *ruta = ruta_o_default(rutaSnapshot, "stock_snapshot.txt");
    return copiar_archivo_binario(ruta, "stock.txt");
}

static int cargar_claves_monedas_archivo(const char *archivo,
                                         char claves[][MAX_TOKEN],
                                         size_t max_claves,
                                         size_t *cantidad)
{
    FILE *fp;
    char token[MAX_TOKEN];

    if (archivo == NULL || claves == NULL || cantidad == NULL)
        return 0;

    *cantidad = 0;
    fp = fopen(archivo, "r");
    if (fp == NULL)
        return 0;

    while (fscanf(fp, "%255s", token) == 1)
    {
        size_t i;
        int repetida = 0;

        if (strcmp(token, "-1") == 0 || es_texto_bigint_valido(token))
            continue;

        for (i = 0; i < *cantidad; i++)
        {
            if (strcmp(claves[i], token) == 0)
            {
                repetida = 1;
                break;
            }
        }

        if (repetida)
            continue;

        if (*cantidad >= max_claves)
        {
            fclose(fp);
            return 0;
        }

        snprintf(claves[*cantidad], MAX_TOKEN, "%s", token);
        (*cantidad)++;
    }

    fclose(fp);
    return 1;
}

int exportar_reporte_global(const char *rutaReporte)
{
    const char *ruta = ruta_o_default(rutaReporte, "reporte_global.txt");
    FILE *fp = NULL;
    char monedas[128][MAX_TOKEN];
    size_t cantidad_monedas = 0;
    size_t i;
    BigInt total_piezas_global = {0};
    BigInt total_valor_global = {0};
    int ok = 0;

    if (!cargar_claves_monedas_archivo("monedas.txt", monedas, 128, &cantidad_monedas))
        return 0;

    fp = fopen(ruta, "w");
    if (fp == NULL)
        return 0;

    if (!bigint_init(&total_piezas_global, "0") || !bigint_init(&total_valor_global, "0"))
        goto cleanup;

    {
        time_t ahora = time(NULL);
        struct tm *tm_info = localtime(&ahora);
        if (tm_info != NULL)
        {
            fprintf(fp,
                    "Reporte global de inventario\n"
                    "Generado: %04d-%02d-%02d %02d:%02d:%02d\n\n",
                    tm_info->tm_year + 1900,
                    tm_info->tm_mon + 1,
                    tm_info->tm_mday,
                    tm_info->tm_hour,
                    tm_info->tm_min,
                    tm_info->tm_sec);
        }
        else
        {
            fprintf(fp, "Reporte global de inventario\n\n");
        }
    }

    for (i = 0; i < cantidad_monedas; i++)
    {
        BigIntArray denom = {0};
        BigIntArray stock = {0};
        BigInt total_piezas = {0};
        BigInt total_valor = {0};
        size_t j;

        if (!cargar_denominaciones_moneda(monedas[i], &denom) ||
            !cargar_stock_moneda(monedas[i], &stock) ||
            denom.len == 0 || stock.len != denom.len)
        {
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            continue;
        }

        if (!bigint_init(&total_piezas, "0") || !bigint_init(&total_valor, "0"))
        {
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            bigint_free(&total_piezas);
            bigint_free(&total_valor);
            goto cleanup;
        }

        for (j = 0; j < denom.len; j++)
        {
            BigInt nuevo_total_piezas = {0};
            BigInt parcial_valor = {0};
            BigInt nuevo_total_valor = {0};

            if (!bigint_add(&total_piezas, &stock.items[j], &nuevo_total_piezas) ||
                !bigint_multiply(&denom.items[j], &stock.items[j], &parcial_valor) ||
                !bigint_add(&total_valor, &parcial_valor, &nuevo_total_valor))
            {
                bigint_free(&nuevo_total_piezas);
                bigint_free(&parcial_valor);
                bigint_free(&nuevo_total_valor);
                bigint_array_free(&denom);
                bigint_array_free(&stock);
                bigint_free(&total_piezas);
                bigint_free(&total_valor);
                goto cleanup;
            }

            bigint_free(&total_piezas);
            total_piezas = nuevo_total_piezas;
            bigint_free(&total_valor);
            total_valor = nuevo_total_valor;
            bigint_free(&parcial_valor);
        }

        fprintf(fp,
                "Moneda: %s\n"
                "  Denominaciones: %zu\n"
                "  Piezas en stock: %s\n"
                "  Valor total: %s c\n\n",
                monedas[i],
                denom.len,
                total_piezas.digits,
                total_valor.digits);

        {
            BigInt nuevo_global_piezas = {0};
            BigInt nuevo_global_valor = {0};

            if (!bigint_add(&total_piezas_global, &total_piezas, &nuevo_global_piezas) ||
                !bigint_add(&total_valor_global, &total_valor, &nuevo_global_valor))
            {
                bigint_free(&nuevo_global_piezas);
                bigint_free(&nuevo_global_valor);
                bigint_array_free(&denom);
                bigint_array_free(&stock);
                bigint_free(&total_piezas);
                bigint_free(&total_valor);
                goto cleanup;
            }

            bigint_free(&total_piezas_global);
            total_piezas_global = nuevo_global_piezas;
            bigint_free(&total_valor_global);
            total_valor_global = nuevo_global_valor;
        }

        bigint_array_free(&denom);
        bigint_array_free(&stock);
        bigint_free(&total_piezas);
        bigint_free(&total_valor);
    }

    fprintf(fp,
            "Totales globales\n"
            "  Monedas procesadas: %zu\n"
            "  Piezas globales: %s\n"
            "  Valor global: %s c\n",
            cantidad_monedas,
            total_piezas_global.digits,
            total_valor_global.digits);

    ok = 1;

cleanup:
    bigint_free(&total_piezas_global);
    bigint_free(&total_valor_global);
    if (fp != NULL)
    {
        fclose(fp);
        if (!ok)
            remove(ruta);
    }
    return ok;
}

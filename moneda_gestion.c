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
/* funcion es_texto_bigint_valido: contiene la logica principal de esta operacion. */
static int es_texto_bigint_valido(const char *texto)
{
    size_t i;

    /* if: comprueba texto == NULL || *texto == '\0' antes de ejecutar esta rama. */
    if (texto == NULL || *texto == '\0')
        return 0;

    /* for: itera segun i = 0; texto[i] != '\0'; i++ para recorrer el bloque. */
    for (i = 0; texto[i] != '\0'; i++)
    {
        /* if: comprueba texto[i] < '0' || texto[i] > '9' antes de ejecutar esta rama. */
        if (texto[i] < '0' || texto[i] > '9')
            return 0;
    }

    return 1;
}

/* duplicar_cadena: Asigna memoria y copia el contenido de una cadena en ella. */
/* funcion duplicar_cadena: contiene la logica principal de esta operacion. */
static char *duplicar_cadena(const char *texto)
{
    size_t len;
    char *copia;

    /* if: comprueba texto == NULL antes de ejecutar esta rama. */
    if (texto == NULL)
        return NULL;

    len = strlen(texto) + 1;
    copia = (char *)malloc(len);
    /* if: comprueba copia == NULL antes de ejecutar esta rama. */
    if (copia == NULL)
        return NULL;

    memcpy(copia, texto, len);
    return copia;
}

/* recortar_fin_linea: Elimina saltos de linea CR o LF al final del string. */
/* funcion recortar_fin_linea: contiene la logica principal de esta operacion. */
static void recortar_fin_linea(char *texto)
{
    /* if: comprueba texto == NULL antes de ejecutar esta rama. */
    if (texto == NULL)
        return;

    texto[strcspn(texto, "\r\n")] = '\0';
}

/* cargar_datos_moneda: Nucleo lector para monedas y stock. */
/* funcion cargar_datos_moneda: contiene la logica principal de esta operacion. */
static int cargar_datos_moneda(const char *archivo, const char *nombreMoneda, int convertirMenosUnoACero, BigIntArray *resultado)
{
    FILE *fp;
    char token[MAX_TOKEN];
    char valores[MAX_DENOMINACIONES][MAX_TOKEN];
    size_t cantidad = 0;
    int enSeccion = 0;

    /* if: comprueba archivo == NULL || nombreMoneda == NULL || *nombreMoneda == '\0' || r... antes de ejecutar esta rama. */
    if (archivo == NULL || nombreMoneda == NULL || *nombreMoneda == '\0' || resultado == NULL)
        return 0;

    fp = fopen(archivo, "r");
    /* if: comprueba fp == NULL antes de ejecutar esta rama. */
    if (fp == NULL)
        return 0;

    /* Bucle principal: va extrayendo palabras (tokens) limitadas a 255 caracteres separados por espacios o saltos de linea. */
    /* while: repite el bloque mientras se cumpla fscanf(fp, "%255s", token) == 1. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        /* Si aun no hemos encontrado la declaracion de la moneda que buscamos... */
        /* if: comprueba !enSeccion antes de ejecutar esta rama. */
        if (!enSeccion)
        {
            /* if: comprueba strcmp(token, nombreMoneda) == 0 antes de ejecutar esta rama. */
            if (strcmp(token, nombreMoneda) == 0)
                enSeccion = 1;
            continue;
        }

        /* if: comprueba convertirMenosUnoACero && strcmp(token, "-1") == 0 antes de ejecutar esta rama. */
        if (convertirMenosUnoACero && strcmp(token, "-1") == 0)
            strcpy(token, "0");

        /* if: comprueba !es_texto_bigint_valido(token) antes de ejecutar esta rama. */
        if (!es_texto_bigint_valido(token))
            break;

        /* if: comprueba cantidad >= MAX_DENOMINACIONES antes de ejecutar esta rama. */
        if (cantidad >= MAX_DENOMINACIONES)
            break;

        strcpy(valores[cantidad], token);
        cantidad++;
    }

    fclose(fp);

    /* if: comprueba !enSeccion || cantidad == 0 antes de ejecutar esta rama. */
    if (!enSeccion || cantidad == 0)
        return 0;

    /* if: comprueba !bigint_array_create(resultado, cantidad) antes de ejecutar esta rama. */
    if (!bigint_array_create(resultado, cantidad))
        return 0;

    /* for: itera segun size_t i = 0; i < cantidad; i++ para recorrer el bloque. */
    for (size_t i = 0; i < cantidad; i++)
    {
        BigInt tmp = {0};
        /* Intenta inicializar tmp con los digitos guardados en valores[i]. Si falla la reserva dentro de init... */
        /* if: comprueba !bigint_init(&tmp, valores[i]) antes de ejecutar esta rama. */
        if (!bigint_init(&tmp, valores[i]))
        {
            bigint_array_free(resultado);
            return 0;
        }
        /* Asigna la estructura tmp recien creada al indice 'i' del array destino 'resultado'. Si falla... */
        /* if: comprueba !bigint_array_set(resultado, i, &tmp) antes de ejecutar esta rama. */
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
/* funcion cargar_denominaciones_moneda: contiene la logica principal de esta operacion. */
int cargar_denominaciones_moneda(const char *nombreMoneda, BigIntArray *resultado)
{
    return cargar_datos_moneda("monedas.txt", nombreMoneda, 0, resultado);
}

/* cargar_stock_moneda: Wrapper que delega en cargar_datos_moneda apuntando a "stock.txt". */
/* funcion cargar_stock_moneda: contiene la logica principal de esta operacion. */
int cargar_stock_moneda(const char *nombreMoneda, BigIntArray *resultado)
{
    return cargar_datos_moneda("stock.txt", nombreMoneda, 1, resultado);
}

/* actualizar_stock_moneda: Funcion que reescribe stock.txt mediante temporal para evitar corrupcion parcial.
Explicación del algoritmo: Se lee todo el archivo stock.txt en memoria, se busca la moneda a actualizar, se reemplazan sus lineas de stock por las nuevas, y luego se escribe todo en un archivo temporal. Finalmente, se renombra el temporal al original. */
/* funcion actualizar_stock_moneda: contiene la logica principal de esta operacion. */
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

    /* if: comprueba nombreMoneda == NULL || *nombreMoneda == '\0' || antes de ejecutar esta rama. */
    if (nombreMoneda == NULL || *nombreMoneda == '\0' ||
        stock == NULL || stock->items == NULL || stock->len == 0)
        return 0;

    archivoLectura = fopen(rutaStock, "r");
    /* if: comprueba archivoLectura == NULL antes de ejecutar esta rama. */
    if (archivoLectura == NULL)
        return 0;

    /* for: itera segun i = 0; i < MAX_LINEAS_STOCK; i++ para recorrer el bloque. */
    for (i = 0; i < MAX_LINEAS_STOCK; i++)
        lineas[i] = NULL;

    /* Bucle principal de lectura: carga todo el archivo stock.txt en la matriz dinamica 'lineas'. */
    /* while: repite el bloque mientras se cumpla fgets(buffer, sizeof(buffer), archivoLectura) != NULL. */
    while (fgets(buffer, sizeof(buffer), archivoLectura) != NULL)
    {
        size_t len = strlen(buffer);

        /* Limita la lectura a MAX_LINEAS_STOCK para evitar buffer overflow en el arreglo 'lineas'. */
        /* if: comprueba totalLineas >= MAX_LINEAS_STOCK antes de ejecutar esta rama. */
        if (totalLineas >= MAX_LINEAS_STOCK)
        {
            ok = 0;
            break;
        }

        /* Si la linea es demasiado larga, fgets no habra incluido un salto de linea al final (salvo EOF). */
        /* if: comprueba len > 0 && buffer[len - 1] != '\n' && !feof(archivoLectura) antes de ejecutar esta rama. */
        if (len > 0 && buffer[len - 1] != '\n' && !feof(archivoLectura))
        {
            ok = 0;
            break;
        }

        lineas[totalLineas] = duplicar_cadena(buffer);
        /* Verifica que malloc no haya fallado durante 'duplicar_cadena'. */
        /* if: comprueba lineas[totalLineas] == NULL antes de ejecutar esta rama. */
        if (lineas[totalLineas] == NULL)
        {
            ok = 0;
            break;
        }
        totalLineas++;
    }

    /* Fase de modificación: busca y altera las lineas del stock de la moneda si no hubo errores previos. */
    /* if: comprueba ok antes de ejecutar esta rama. */
    if (ok)
    {
        /* for: itera segun size_t linea = 0; linea < totalLineas && !actualizado; linea++ para recorrer el bloque. */
        for (size_t linea = 0; linea < totalLineas && !actualizado; linea++)
        {
            // Compara la linea actual con el nombre de la moneda que queremos actualizar. Si no coincide, continua al siguiente ciclo.
            /* if: comprueba lineas[linea] == NULL antes de ejecutar esta rama. */
            if (lineas[linea] == NULL)
            {
                ok = 0;
                break;
            }

            strncpy(comparable, lineas[linea], sizeof(comparable) - 1);
            comparable[sizeof(comparable) - 1] = '\0';
            recortar_fin_linea(comparable);

            // Si la linea no coincide con el nombre de la moneda que queremos actualizar, continua al siguiente ciclo.
            /* if: comprueba strcmp(comparable, nombreMoneda) != 0 antes de ejecutar esta rama. */
            if (strcmp(comparable, nombreMoneda) != 0)
                continue;

            /* Verificamos que queden suficientes lineas debajo de la etiqueta para reemplazar todo el stock sin pasarnos. */
            /* if: comprueba stock->len > totalLineas - (linea + 1) antes de ejecutar esta rama. */
            if (stock->len > totalLineas - (linea + 1))
            {
                ok = 0;
                break;
            }

            // Reemplaza las lineas de stock con los nuevos valores del arreglo 'stock'.
            /* for: itera segun i = 0; i < stock->len; i++ para recorrer el bloque. */
            for (i = 0; i < stock->len; i++)
            {
                size_t objetivo = linea + 1 + i;
                char nuevaLinea[MAX_LINEA_STOCK];

                /* if: comprueba bigint_is_zero(&stock->items[i]) antes de ejecutar esta rama. */
                if (bigint_is_zero(&stock->items[i]))
                    snprintf(nuevaLinea, sizeof(nuevaLinea), "-1\n");
                else
                    snprintf(nuevaLinea, sizeof(nuevaLinea), "%s\n", stock->items[i].digits);

                free(lineas[objetivo]);
                lineas[objetivo] = duplicar_cadena(nuevaLinea);
                /* Si no habia memoria para crear esta nueva linea... */
                /* if: comprueba lineas[objetivo] == NULL antes de ejecutar esta rama. */
                if (lineas[objetivo] == NULL)
                {
                    ok = 0;
                    break;
                }
            }

            /* if: comprueba ok antes de ejecutar esta rama. */
            if (ok)
                actualizado = 1;
        }
    }

    /* Fase de aborto o limpieza temprana. */
    /* if: comprueba !ok || !actualizado antes de ejecutar esta rama. */
    if (!ok || !actualizado)
    {
        /* for: itera segun i = 0; i < totalLineas; i++ para recorrer el bloque. */
        for (i = 0; i < totalLineas; i++)
            free(lineas[i]);
        fclose(archivoLectura);
        return 0;
    }

    fclose(archivoLectura);
    archivoLectura = NULL;

    archivoTemporal = fopen(rutaTemporal, "w");
    /* if: comprueba archivoTemporal == NULL antes de ejecutar esta rama. */
    if (archivoTemporal == NULL)
        ok = 0;

    /* for: itera segun i = 0; ok && i < totalLineas; i++ para recorrer el bloque. */
    for (i = 0; ok && i < totalLineas; i++)
    {
        /* Escribe secuencialmente la linea en el archivo temporal. */
        /* if: comprueba fputs(lineas[i], archivoTemporal) == EOF antes de ejecutar esta rama. */
        if (fputs(lineas[i], archivoTemporal) == EOF)
        {
            ok = 0;
        }
    }

    /* Asegura que todos los datos se escribieron correctamente en el archivo temporal.
       Explicacion logica: fflush devuelve 0 si todo fue bien, y un valor distinto de cero si hubo error; la variable ok se niega para marcar fallo. Por eso se niega el resultado para marcar error.
       Por eso se niega el resultado para marcar error.
    */
    /* if: comprueba ok && fflush(archivoTemporal) != 0 antes de ejecutar esta rama. */
    if (ok && fflush(archivoTemporal) != 0)
        ok = 0;

    /* Cierra el archivo temporal y marca error si hubo fallo. Explicacion logica: fclose devuelve 0 si todo fue bien, y un valor distinto de cero si hubo error; la variable ok se niega para marcar fallo. Por eso se niega el resultado para marcar error. */
    /* if: comprueba archivoTemporal != NULL && fclose(archivoTemporal) != 0 antes de ejecutar esta rama. */
    if (archivoTemporal != NULL && fclose(archivoTemporal) != 0)
        ok = 0;
    archivoTemporal = NULL;

    /* if: comprueba ok antes de ejecutar esta rama. */
    if (ok)
    {
#ifdef _WIN32
        /* Este bloque de codigo es para Windows: elimina el archivo original antes de renombrar el temporal, ya que Windows no permite renombrar sobre un archivo existente. */
        /* if: comprueba remove(rutaStock) != 0 antes de ejecutar esta rama. */
        if (remove(rutaStock) != 0)
            ok = 0;
#endif
        /* Este bloque de codigo es para sistemas POSIX: renombra el archivo temporal al nombre del stock, reemplazando el original. */
        /* if: comprueba ok && rename(rutaTemporal, rutaStock) != 0 antes de ejecutar esta rama. */
        if (ok && rename(rutaTemporal, rutaStock) != 0)
        {
            FILE *archivoDestino = fopen(rutaStock, "w");
            /* if: comprueba archivoDestino == NULL antes de ejecutar esta rama. */
            if (archivoDestino == NULL)
            {
                ok = 0;
            }
            else
            /* Explicación del for: Reescribe todas las lineas en el archivo destino. Si alguna falla, marca error y sale del bucle. */
            {
                /* for: itera segun i = 0; i < totalLineas; i++ para recorrer el bloque. */
                for (i = 0; i < totalLineas; i++)
                {
                    // Este if verifica si fputs devolvio EOF, lo que indica un error de escritura. Si ocurre, se marca ok como 0 y se rompe el bucle.
                    /* if: comprueba fputs(lineas[i], archivoDestino) == EOF antes de ejecutar esta rama. */
                    if (fputs(lineas[i], archivoDestino) == EOF)
                    {
                        ok = 0;
                        break;
                    }
                }
                // Este if verifica si fflush devolvio un valor distinto de 0, lo que indica un error al vaciar el buffer. Si ocurre, se marca ok como 0.
                /* if: comprueba ok && fflush(archivoDestino) != 0 antes de ejecutar esta rama. */
                if (ok && fflush(archivoDestino) != 0)
                    ok = 0;
                // Este if verifica si fclose devolvio un valor distinto de 0, lo que indica un error al cerrar el archivo. Si ocurre, se marca ok como 0.
                /* if: comprueba fclose(archivoDestino) != 0 antes de ejecutar esta rama. */
                if (fclose(archivoDestino) != 0)
                    ok = 0;
            }
        }
    }

    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
        remove(rutaTemporal);

    /* for: itera segun i = 0; i < totalLineas; i++ para recorrer el bloque. */
    for (i = 0; i < totalLineas; i++)
        free(lineas[i]);

    /* if: comprueba archivoLectura != NULL antes de ejecutar esta rama. */
    if (archivoLectura != NULL)
        fclose(archivoLectura);
    /* if: comprueba archivoTemporal != NULL antes de ejecutar esta rama. */
    if (archivoTemporal != NULL)
        fclose(archivoTemporal);

    return (ok && actualizado) ? 1 : 0;
}

/* funcion validar_consistencia_moneda: contiene la logica principal de esta operacion. */
int validar_consistencia_moneda(const char *nombreMoneda)
{
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    int ok = 0;

    /* if: comprueba nombreMoneda == NULL || *nombreMoneda == '\0' antes de ejecutar esta rama. */
    if (nombreMoneda == NULL || *nombreMoneda == '\0')
        return 0;

    /* if: comprueba !cargar_denominaciones_moneda(nombreMoneda, &denom) antes de ejecutar esta rama. */
    if (!cargar_denominaciones_moneda(nombreMoneda, &denom))
        goto fin;

    /* if: comprueba !cargar_stock_moneda(nombreMoneda, &stock) antes de ejecutar esta rama. */
    if (!cargar_stock_moneda(nombreMoneda, &stock))
        goto fin;

    /* if: comprueba denom.len != stock.len || denom.len == 0 antes de ejecutar esta rama. */
    if (denom.len != stock.len || denom.len == 0)
        goto fin;

    /* for: itera segun size_t i = 1; i < denom.len; i++ para recorrer el bloque. */
    for (size_t i = 1; i < denom.len; i++)
    {
        /* if: comprueba bigint_compare(&denom.items[i - 1], &denom.items[i]) <= 0 antes de ejecutar esta rama. */
        if (bigint_compare(&denom.items[i - 1], &denom.items[i]) <= 0)
            goto fin;
    }

    ok = 1;

fin:
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    return ok;
}

/* funcion ruta_o_default: contiene la logica principal de esta operacion. */
static const char *ruta_o_default(const char *ruta, const char *defecto)
{
    /* if: comprueba ruta == NULL || *ruta == '\0' antes de ejecutar esta rama. */
    if (ruta == NULL || *ruta == '\0')
        return defecto;
    return ruta;
}

/* funcion copiar_archivo_binario: contiene la logica principal de esta operacion. */
static int copiar_archivo_binario(const char *origen, const char *destino)
{
    FILE *in = NULL;
    FILE *out = NULL;
    unsigned char buffer[4096];
    size_t leidos;
    int ok = 1;

    /* if: comprueba origen == NULL || destino == NULL antes de ejecutar esta rama. */
    if (origen == NULL || destino == NULL)
        return 0;

    in = fopen(origen, "rb");
    /* if: comprueba in == NULL antes de ejecutar esta rama. */
    if (in == NULL)
        return 0;

    out = fopen(destino, "wb");
    /* if: comprueba out == NULL antes de ejecutar esta rama. */
    if (out == NULL)
    {
        fclose(in);
        return 0;
    }

    /* while: repite el bloque mientras se cumpla (leidos = fread(buffer, 1, sizeof(buffer), in)) > 0. */
    while ((leidos = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        /* if: comprueba fwrite(buffer, 1, leidos, out) != leidos antes de ejecutar esta rama. */
        if (fwrite(buffer, 1, leidos, out) != leidos)
        {
            ok = 0;
            break;
        }
    }

    /* if: comprueba ferror(in) antes de ejecutar esta rama. */
    if (ferror(in))
        ok = 0;

    /* if: comprueba fclose(out) != 0 antes de ejecutar esta rama. */
    if (fclose(out) != 0)
        ok = 0;
    fclose(in);

    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
        remove(destino);

    return ok;
}

/* funcion crear_snapshot_stock: contiene la logica principal de esta operacion. */
int crear_snapshot_stock(const char *rutaSnapshot)
{
    const char *ruta = ruta_o_default(rutaSnapshot, "stock_snapshot.txt");
    return copiar_archivo_binario("stock.txt", ruta);
}

/* funcion restaurar_snapshot_stock: contiene la logica principal de esta operacion. */
int restaurar_snapshot_stock(const char *rutaSnapshot)
{
    const char *ruta = ruta_o_default(rutaSnapshot, "stock_snapshot.txt");
    return copiar_archivo_binario(ruta, "stock.txt");
}

/* funcion cargar_claves_monedas_archivo: contiene la logica principal de esta operacion. */
static int cargar_claves_monedas_archivo(const char *archivo,
                                         char claves[][MAX_TOKEN],
                                         size_t max_claves,
                                         size_t *cantidad)
{
    FILE *fp;
    char token[MAX_TOKEN];

    /* if: comprueba archivo == NULL || claves == NULL || cantidad == NULL antes de ejecutar esta rama. */
    if (archivo == NULL || claves == NULL || cantidad == NULL)
        return 0;

    *cantidad = 0;
    fp = fopen(archivo, "r");
    /* if: comprueba fp == NULL antes de ejecutar esta rama. */
    if (fp == NULL)
        return 0;

    /* while: repite el bloque mientras se cumpla fscanf(fp, "%255s", token) == 1. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        size_t i;
        int repetida = 0;

        /* if: comprueba strcmp(token, "-1") == 0 || es_texto_bigint_valido(token) antes de ejecutar esta rama. */
        if (strcmp(token, "-1") == 0 || es_texto_bigint_valido(token))
            continue;

        /* for: itera segun i = 0; i < *cantidad; i++ para recorrer el bloque. */
        for (i = 0; i < *cantidad; i++)
        {
            /* if: comprueba strcmp(claves[i], token) == 0 antes de ejecutar esta rama. */
            if (strcmp(claves[i], token) == 0)
            {
                repetida = 1;
                break;
            }
        }

        /* if: comprueba repetida antes de ejecutar esta rama. */
        if (repetida)
            continue;

        /* if: comprueba *cantidad >= max_claves antes de ejecutar esta rama. */
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

/* funcion exportar_reporte_global: contiene la logica principal de esta operacion. */
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

    /* if: comprueba !cargar_claves_monedas_archivo("monedas.txt", monedas, 128, &cantidad... antes de ejecutar esta rama. */
    if (!cargar_claves_monedas_archivo("monedas.txt", monedas, 128, &cantidad_monedas))
        return 0;

    fp = fopen(ruta, "w");
    /* if: comprueba fp == NULL antes de ejecutar esta rama. */
    if (fp == NULL)
        return 0;

    /* if: comprueba !bigint_init(&total_piezas_global, "0") || !bigint_init(&total_valor_... antes de ejecutar esta rama. */
    if (!bigint_init(&total_piezas_global, "0") || !bigint_init(&total_valor_global, "0"))
        goto cleanup;

    {
        time_t ahora = time(NULL);
        struct tm *tm_info = localtime(&ahora);
        /* if: comprueba tm_info != NULL antes de ejecutar esta rama. */
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

    /* for: itera segun i = 0; i < cantidad_monedas; i++ para recorrer el bloque. */
    for (i = 0; i < cantidad_monedas; i++)
    {
        BigIntArray denom = {0};
        BigIntArray stock = {0};
        BigInt total_piezas = {0};
        BigInt total_valor = {0};
        size_t j;

        /* if: comprueba !cargar_denominaciones_moneda(monedas[i], &denom) || antes de ejecutar esta rama. */
        if (!cargar_denominaciones_moneda(monedas[i], &denom) ||
            !cargar_stock_moneda(monedas[i], &stock) ||
            denom.len == 0 || stock.len != denom.len)
        {
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            continue;
        }

        /* if: comprueba !bigint_init(&total_piezas, "0") || !bigint_init(&total_valor, "0") antes de ejecutar esta rama. */
        if (!bigint_init(&total_piezas, "0") || !bigint_init(&total_valor, "0"))
        {
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            bigint_free(&total_piezas);
            bigint_free(&total_valor);
            goto cleanup;
        }

        /* for: itera segun j = 0; j < denom.len; j++ para recorrer el bloque. */
        for (j = 0; j < denom.len; j++)
        {
            BigInt nuevo_total_piezas = {0};
            BigInt parcial_valor = {0};
            BigInt nuevo_total_valor = {0};

            /* if: comprueba !bigint_add(&total_piezas, &stock.items[j], &nuevo_total_piezas) || antes de ejecutar esta rama. */
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

            /* if: comprueba !bigint_add(&total_piezas_global, &total_piezas, &nuevo_global_piezas... antes de ejecutar esta rama. */
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
    /* if: comprueba fp != NULL antes de ejecutar esta rama. */
    if (fp != NULL)
    {
        fclose(fp);
        /* if: comprueba !ok antes de ejecutar esta rama. */
        if (!ok)
            remove(ruta);
    }
    return ok;
}

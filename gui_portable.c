#ifndef _WIN32

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "bigint.h"
#include "moneda_gestion.h"
#include "algoritmo_cambio.h"
#include "exchange_api.h"

#include <time.h>

/* registrar_historial: Guarda transacciones en historial.txt */
/* funcion registrar_historial: contiene la logica principal de esta operacion. */
static void registrar_historial(const char *mensaje)
{
    FILE *fp = fopen("historial.txt", "a");
    /* if: comprueba fp antes de ejecutar esta rama. */
    if (fp)
    {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        /* if: comprueba tm_info antes de ejecutar esta rama. */
        if (tm_info)
        {
            fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                    tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, mensaje);
        }
        fclose(fp);
    }
}

/* funcion registrar_historialf: contiene la logica principal de esta operacion. */
static void registrar_historialf(const char *fmt, ...)
{
    char mensaje[512];
    va_list args;

    /* if: comprueba fmt == NULL antes de ejecutar esta rama. */
    if (fmt == NULL)
        return;

    va_start(args, fmt);
    vsnprintf(mensaje, sizeof(mensaje), fmt, args);
    va_end(args);
    registrar_historial(mensaje);
}

/* funcion mostrar_historial_transacciones: contiene la logica principal de esta operacion. */
static void mostrar_historial_transacciones(void)
{
    FILE *fp = fopen("historial.txt", "r");
    char linea[256];

    printf("\n--- HISTORIAL DE TRANSACCIONES ---\n");
    /* if: comprueba !fp antes de ejecutar esta rama. */
    if (!fp)
    {
        printf("No hay transacciones registradas.\n");
    }
    else
    {
        /* while: repite el bloque mientras se cumpla fgets(linea, sizeof(linea), fp). */
        while (fgets(linea, sizeof(linea), fp))
            printf("%s", linea);
        fclose(fp);
    }
    printf("----------------------------------\n\n");
}

/* funcion pausar_pantalla: contiene la logica principal de esta operacion. */
static void pausar_pantalla(void)
{
    char buffer[8];

    printf("Presione Enter para continuar...");
    /* if: comprueba fgets(buffer, (int)sizeof(buffer), stdin) == NULL antes de ejecutar esta rama. */
    if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL)
        clearerr(stdin);
}

/* funcion ejecutar_operacion_global: contiene la logica principal de esta operacion. */
static void ejecutar_operacion_global(const char *accion)
{
    /* if: comprueba accion == NULL antes de ejecutar esta rama. */
    if (accion == NULL)
        return;

    /* if: comprueba strcmp(accion, "snapshot") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "snapshot") == 0)
    {
        /* if: comprueba crear_snapshot_stock("stock_snapshot.txt") antes de ejecutar esta rama. */
        if (crear_snapshot_stock("stock_snapshot.txt"))
        {
            printf("Snapshot creado: stock_snapshot.txt\n");
            registrar_historial("Snapshot de stock creado (portable)");
        }
        else
        {
            printf("No se pudo crear snapshot de stock.\n");
        }
        return;
    }

    /* if: comprueba strcmp(accion, "restaurar") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "restaurar") == 0)
    {
        /* if: comprueba restaurar_snapshot_stock("stock_snapshot.txt") antes de ejecutar esta rama. */
        if (restaurar_snapshot_stock("stock_snapshot.txt"))
        {
            printf("Stock restaurado desde stock_snapshot.txt\n");
            registrar_historial("Stock restaurado desde snapshot (portable)");
        }
        else
        {
            printf("No se pudo restaurar stock desde snapshot.\n");
        }
        return;
    }

    /* if: comprueba strcmp(accion, "reporte") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "reporte") == 0)
    {
        /* if: comprueba exportar_reporte_global("reporte_global.txt") antes de ejecutar esta rama. */
        if (exportar_reporte_global("reporte_global.txt"))
        {
            printf("Reporte global generado: reporte_global.txt\n");
            registrar_historial("Reporte global exportado (portable)");
        }
        else
        {
            printf("No se pudo generar reporte global.\n");
        }
    }
}

#define MAX_MONEDAS 64
#define MAX_NOMBRE 64

typedef enum
{
    MODO_LIMITADO = 0,
    MODO_ILIMITADO = 1
} ModoGUI;

/* leer_linea: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion leer_linea: contiene la logica principal de esta operacion. */
static int leer_linea(char *buffer, size_t tam)
{
    /* if: comprueba buffer == NULL || tam == 0 antes de ejecutar esta rama. */
    if (buffer == NULL || tam == 0)
        return 0;

    /* if: comprueba fgets(buffer, (int)tam, stdin) == NULL antes de ejecutar esta rama. */
    if (fgets(buffer, (int)tam, stdin) == NULL)
        return 0;

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

/* funcion parse_size_t_positivo_gui: contiene la logica principal de esta operacion. */
static int parse_size_t_positivo_gui(const char *texto, size_t *valor_out)
{
    char *fin;
    unsigned long long valor;

    /* if: comprueba texto == NULL || texto[0] == '\0' || valor_out == NULL antes de ejecutar esta rama. */
    if (texto == NULL || texto[0] == '\0' || valor_out == NULL)
        return 0;

    valor = strtoull(texto, &fin, 10);
    /* if: comprueba *fin != '\0' || valor == 0 || valor > (unsigned long long)((size_t)-1) antes de ejecutar esta rama. */
    if (*fin != '\0' || valor == 0 || valor > (unsigned long long)((size_t)-1))
        return 0;

    *valor_out = (size_t)valor;
    return 1;
}

/* funcion parse_restriccion_gui: contiene la logica principal de esta operacion. */
static int parse_restriccion_gui(const char *entrada, size_t *min_monedas, size_t *max_monedas)
{
    char limpio[128];
    size_t i = 0;
    size_t j = 0;

    /* if: comprueba entrada == NULL || min_monedas == NULL || max_monedas == NULL antes de ejecutar esta rama. */
    if (entrada == NULL || min_monedas == NULL || max_monedas == NULL)
        return 0;

    /* while: repite el bloque mientras se cumpla entrada[i] != '\0' && j + 1 < sizeof(limpio). */
    while (entrada[i] != '\0' && j + 1 < sizeof(limpio))
    {
        /* if: comprueba !isspace((unsigned char)entrada[i]) antes de ejecutar esta rama. */
        if (!isspace((unsigned char)entrada[i]))
            limpio[j++] = entrada[i];
        i++;
    }
    limpio[j] = '\0';

    /* if: comprueba limpio[0] == '\0' antes de ejecutar esta rama. */
    if (limpio[0] == '\0')
        return 0;

    /* if: comprueba limpio[0] == '=' antes de ejecutar esta rama. */
    if (limpio[0] == '=')
    {
        size_t exacto;
        /* if: comprueba !parse_size_t_positivo_gui(limpio + 1, &exacto) antes de ejecutar esta rama. */
        if (!parse_size_t_positivo_gui(limpio + 1, &exacto))
            return 0;
        *min_monedas = exacto;
        *max_monedas = exacto;
        return 1;
    }

    {
        char *guion = strchr(limpio, '-');
        /* if: comprueba guion != NULL antes de ejecutar esta rama. */
        if (guion != NULL)
        {
            char izq[64];
            char der[64];
            size_t min_v;
            size_t max_v;
            size_t len_izq = (size_t)(guion - limpio);

            /* if: comprueba len_izq == 0 || len_izq >= sizeof(izq) antes de ejecutar esta rama. */
            if (len_izq == 0 || len_izq >= sizeof(izq))
                return 0;

            memcpy(izq, limpio, len_izq);
            izq[len_izq] = '\0';
            strncpy(der, guion + 1, sizeof(der) - 1);
            der[sizeof(der) - 1] = '\0';

            /* if: comprueba !parse_size_t_positivo_gui(izq, &min_v) || !parse_size_t_positivo_gui... antes de ejecutar esta rama. */
            if (!parse_size_t_positivo_gui(izq, &min_v) || !parse_size_t_positivo_gui(der, &max_v))
                return 0;
            /* if: comprueba min_v > max_v antes de ejecutar esta rama. */
            if (min_v > max_v)
                return 0;

            *min_monedas = min_v;
            *max_monedas = max_v;
            return 1;
        }
    }

    {
        size_t max_v;
        /* if: comprueba !parse_size_t_positivo_gui(limpio, &max_v) antes de ejecutar esta rama. */
        if (!parse_size_t_positivo_gui(limpio, &max_v))
            return 0;
        *min_monedas = 0;
        *max_monedas = max_v;
        return 1;
    }
}

/* a_minusculas: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion a_minusculas: contiene la logica principal de esta operacion. */
static void a_minusculas(char *texto)
{
    size_t i;

    /* if: comprueba texto == NULL antes de ejecutar esta rama. */
    if (texto == NULL)
        return;

    /* for: itera segun i = 0; texto[i] != '\0'; i++ para recorrer el bloque. */
    for (i = 0; texto[i] != '\0'; i++)
        texto[i] = (char)tolower((unsigned char)texto[i]);
}

/* es_numero: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion es_numero: contiene la logica principal de esta operacion. */
static int es_numero(const char *s)
{
    size_t i;

    /* if: comprueba s == NULL || *s == '\0' antes de ejecutar esta rama. */
    if (s == NULL || *s == '\0')
        return 0;

    /* for: itera segun i = 0; s[i] != '\0'; i++ para recorrer el bloque. */
    for (i = 0; s[i] != '\0'; i++)
    {
        /* if: comprueba !isdigit((unsigned char)s[i]) antes de ejecutar esta rama. */
        if (!isdigit((unsigned char)s[i]))
            return 0;
    }

    return 1;
}

/* cargar_nombres_moneda: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion cargar_nombres_moneda: contiene la logica principal de esta operacion. */
static int cargar_nombres_moneda(char monedas[MAX_MONEDAS][MAX_NOMBRE], int *cantidad)
{
    FILE *fp = fopen("monedas.txt", "r");
    char token[256];

    /* if: comprueba fp == NULL || cantidad == NULL antes de ejecutar esta rama. */
    if (fp == NULL || cantidad == NULL)
        return 0;

    *cantidad = 0;

    /* while: repite el bloque mientras se cumpla fscanf(fp, "%255s", token) == 1. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        int existe = 0;
        /* if: comprueba es_numero(token) antes de ejecutar esta rama. */
        if (es_numero(token))
            continue;

        /* for: itera segun int i = 0; i < *cantidad; i++ para recorrer el bloque. */
        for (int i = 0; i < *cantidad; i++)
        {

            /* if: comprueba strcmp(monedas[i], token) == 0 antes de ejecutar esta rama. */
            if (strcmp(monedas[i], token) == 0)
            {
                existe = 1;
                break;
            }
        }

        /* if: comprueba !existe && *cantidad < MAX_MONEDAS antes de ejecutar esta rama. */
        if (!existe && *cantidad < MAX_MONEDAS)
        {
            size_t n = strlen(token);
            /* if: comprueba n >= MAX_NOMBRE antes de ejecutar esta rama. */
            if (n >= MAX_NOMBRE)
                n = MAX_NOMBRE - 1;
            memcpy(monedas[*cantidad], token, n);
            monedas[*cantidad][n] = '\0';
            (*cantidad)++;
        }
    }

    fclose(fp);
    return *cantidad > 0;
}

/* imprimir_panel: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion imprimir_panel: contiene la logica principal de esta operacion. */
static void imprimir_panel(const char *moneda, const BigIntArray *denom, const BigIntArray *stock)
{
    printf("\n==============================================\n");
    printf("Panel Administrador (portable) - moneda: %s\n", moneda);
    printf("==============================================\n");
    /* for: itera segun size_t i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < denom->len; i++)
        printf("[%zu] Denom %s c -> stock %s\n", i + 1, denom->items[i].digits, stock->items[i].digits);
    printf("==============================================\n");
}

/* funcion mostrar_resumen_moneda: contiene la logica principal de esta operacion. */
static void mostrar_resumen_moneda(const char *moneda, const BigIntArray *denom, const BigIntArray *stock, int usarStock)
{
    const BigInt *minDenom;
    const BigInt *maxDenom;
    size_t i;

    /* if: comprueba denom == NULL || denom->items == NULL || denom->len == 0 antes de ejecutar esta rama. */
    if (denom == NULL || denom->items == NULL || denom->len == 0)
    {
        printf("No hay denominaciones cargadas para mostrar resumen.\n");
        return;
    }

    minDenom = &denom->items[0];
    maxDenom = &denom->items[0];
    /* for: itera segun i = 1; i < denom->len; i++ para recorrer el bloque. */
    for (i = 1; i < denom->len; i++)
    {
        /* if: comprueba bigint_compare(&denom->items[i], minDenom) < 0 antes de ejecutar esta rama. */
        if (bigint_compare(&denom->items[i], minDenom) < 0)
            minDenom = &denom->items[i];
        /* if: comprueba bigint_compare(&denom->items[i], maxDenom) > 0 antes de ejecutar esta rama. */
        if (bigint_compare(&denom->items[i], maxDenom) > 0)
            maxDenom = &denom->items[i];
    }

    printf("\n--- RESUMEN DE MONEDA ---\n");
    printf("Moneda: %s\n", moneda != NULL ? moneda : "(sin nombre)");
    printf("Denominaciones cargadas: %zu\n", denom->len);
    printf("Denominacion minima: %s c\n", minDenom->digits);
    printf("Denominacion maxima: %s c\n", maxDenom->digits);

    /* if: comprueba !usarStock antes de ejecutar esta rama. */
    if (!usarStock)
    {
        printf("Modo stock ilimitado: no se calcula inventario fisico.\n");
        printf("-------------------------\n\n");
        return;
    }

    /* if: comprueba stock == NULL || stock->items == NULL || stock->len != denom->len antes de ejecutar esta rama. */
    if (stock == NULL || stock->items == NULL || stock->len != denom->len)
    {
        printf("No hay stock valido para calcular resumen de inventario.\n");
        printf("-------------------------\n\n");
        return;
    }

    {
        BigInt totalPiezas = {0};
        BigInt totalValor = {0};
        int ok = 1;

        /* if: comprueba !bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0") antes de ejecutar esta rama. */
        if (!bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0"))
            ok = 0;

        /* for: itera segun i = 0; ok && i < denom->len; i++ para recorrer el bloque. */
        for (i = 0; ok && i < denom->len; i++)
        {
            BigInt nuevoTotalPiezas = {0};
            BigInt parcialValor = {0};
            BigInt nuevoTotalValor = {0};

            /* if: comprueba !bigint_add(&totalPiezas, &stock->items[i], &nuevoTotalPiezas) || antes de ejecutar esta rama. */
            if (!bigint_add(&totalPiezas, &stock->items[i], &nuevoTotalPiezas) ||
                !bigint_multiply(&denom->items[i], &stock->items[i], &parcialValor) ||
                !bigint_add(&totalValor, &parcialValor, &nuevoTotalValor))
            {
                bigint_free(&nuevoTotalPiezas);
                bigint_free(&parcialValor);
                bigint_free(&nuevoTotalValor);
                ok = 0;
                break;
            }

            bigint_free(&totalPiezas);
            bigint_free(&totalValor);
            totalPiezas = nuevoTotalPiezas;
            totalValor = nuevoTotalValor;
            bigint_free(&parcialValor);
        }

        /* if: comprueba ok antes de ejecutar esta rama. */
        if (ok)
        {
            printf("Total de piezas en stock: %s\n", totalPiezas.digits);
            printf("Valor total del stock: %s c\n", totalValor.digits);
            registrar_historialf("Resumen consultado (portable) | Moneda=%s | Piezas=%s | Valor=%s c",
                                 moneda != NULL ? moneda : "(sin nombre)",
                                 totalPiezas.digits,
                                 totalValor.digits);
        }
        else
        {
            printf("No se pudo calcular el resumen de inventario.\n");
        }

        bigint_free(&totalPiezas);
        bigint_free(&totalValor);
    }

    printf("-------------------------\n\n");
}

/* pedir_indice: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion pedir_indice: contiene la logica principal de esta operacion. */
static int pedir_indice(size_t max, size_t *indice)
{
    char buffer[64];
    char *fin;
    long v;

    /* if: comprueba indice == NULL || max == 0 antes de ejecutar esta rama. */
    if (indice == NULL || max == 0)
        return 0;

    printf("Indice (1-%zu): ", max);
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    v = strtol(buffer, &fin, 10);
    /* if: comprueba *fin != '\0' || v < 1 || (size_t)v > max antes de ejecutar esta rama. */
    if (*fin != '\0' || v < 1 || (size_t)v > max)
        return 0;

    *indice = (size_t)(v - 1);
    return 1;
}

/* pedir_cantidad: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion pedir_cantidad: contiene la logica principal de esta operacion. */
static int pedir_cantidad(BigInt *delta)
{
    char buffer[2048];
    BigInt tmp = {0};

    /* if: comprueba delta == NULL antes de ejecutar esta rama. */
    if (delta == NULL)
        return 0;

    printf("Cantidad (entero no negativo): ");
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    /* if: comprueba !bigint_init(&tmp, buffer) antes de ejecutar esta rama. */
    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(delta);
    *delta = tmp;
    return 1;
}

/* pedir_monto: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion pedir_monto: contiene la logica principal de esta operacion. */
static int pedir_monto(BigInt *monto)
{
    char buffer[2048];
    BigInt tmp = {0};

    /* if: comprueba monto == NULL antes de ejecutar esta rama. */
    if (monto == NULL)
        return 0;

    printf("Monto (entero no negativo en centimos): ");
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    /* if: comprueba !bigint_init(&tmp, buffer) antes de ejecutar esta rama. */
    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(monto);
    *monto = tmp;
    return 1;
}

/* copiar_arreglo_bigint: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion copiar_arreglo_bigint: contiene la logica principal de esta operacion. */
static int copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)
{
    /* if: comprueba origen == NULL || destino == NULL antes de ejecutar esta rama. */
    if (origen == NULL || destino == NULL)
        return 0;

    /* if: comprueba !bigint_array_create(destino, origen->len) antes de ejecutar esta rama. */
    if (!bigint_array_create(destino, origen->len))
        return 0;

    /* for: itera segun size_t i = 0; i < origen->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < origen->len; i++)
    {

        /* if: comprueba !bigint_array_set(destino, i, &origen->items[i]) antes de ejecutar esta rama. */
        if (!bigint_array_set(destino, i, &origen->items[i]))
        {
            bigint_array_free(destino);
            return 0;
        }
    }

    return 1;
}

/* pedir_cantidades_por_denominacion: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion pedir_cantidades_por_denominacion: contiene la logica principal de esta operacion. */
static int pedir_cantidades_por_denominacion(const BigIntArray *denom, const char *titulo, BigIntArray *cantidades)
{
    /* if: comprueba denom == NULL || cantidades == NULL antes de ejecutar esta rama. */
    if (denom == NULL || cantidades == NULL)
        return 0;

    /* if: comprueba !bigint_array_create(cantidades, denom->len) antes de ejecutar esta rama. */
    if (!bigint_array_create(cantidades, denom->len))
        return 0;

    printf("%s\n", titulo);
    /* for: itera segun size_t i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < denom->len; i++)
    {

        /* while: repite el bloque mientras se cumpla 1. */
        while (1)
        {
            char buffer[2048];
            BigInt tmp = {0};

            printf("Cantidad para %s c: ", denom->items[i].digits);

            /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
            if (!leer_linea(buffer, sizeof(buffer)))
            {
                bigint_array_free(cantidades);
                return 0;
            }

            /* if: comprueba !bigint_init(&tmp, buffer) antes de ejecutar esta rama. */
            if (!bigint_init(&tmp, buffer))
            {
                printf("Cantidad invalida. Usa entero no negativo.\n");
                continue;
            }

            /* if: comprueba !bigint_array_set(cantidades, i, &tmp) antes de ejecutar esta rama. */
            if (!bigint_array_set(cantidades, i, &tmp))
            {
                bigint_free(&tmp);
                bigint_array_free(cantidades);
                return 0;
            }

            bigint_free(&tmp);
            break;
        }
    }

    return 1;
}

/* calcular_total_valor: Ejecuta logica del algoritmo o calculos matematicos y de stock. */
/* funcion calcular_total_valor: contiene la logica principal de esta operacion. */
static int calcular_total_valor(const BigIntArray *denom, const BigIntArray *cantidades, BigInt *total)
{
    BigInt acumulado = {0};

    /* if: comprueba denom == NULL || cantidades == NULL || total == NULL antes de ejecutar esta rama. */
    if (denom == NULL || cantidades == NULL || total == NULL)
        return 0;
    /* if: comprueba denom->len != cantidades->len antes de ejecutar esta rama. */
    if (denom->len != cantidades->len)
        return 0;

    /* if: comprueba !bigint_init(&acumulado, "0") antes de ejecutar esta rama. */
    if (!bigint_init(&acumulado, "0"))
        return 0;

    /* for: itera segun size_t i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < denom->len; i++)
    {
        BigInt parcial = {0};
        BigInt nuevo = {0};

        /* if: comprueba bigint_is_zero(&cantidades->items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&cantidades->items[i]))
            continue;

        /* if: comprueba !bigint_multiply(&denom->items[i], &cantidades->items[i], &parcial) || antes de ejecutar esta rama. */
        if (!bigint_multiply(&denom->items[i], &cantidades->items[i], &parcial) ||
            !bigint_add(&acumulado, &parcial, &nuevo))
        {
            bigint_free(&parcial);
            bigint_free(&acumulado);
            return 0;
        }

        bigint_free(&parcial);
        bigint_free(&acumulado);
        acumulado = nuevo;
    }

    bigint_free(total);
    *total = acumulado;
    return 1;
}

/* funcion aplicar_cambio_especifico: contiene la logica principal de esta operacion. */
static int aplicar_cambio_especifico(const char *moneda,
                                     const BigIntArray *denom,
                                     BigIntArray *stock,
                                     const BigIntArray *entregadas,
                                     const BigIntArray *devolucion)
{
    BigInt totalEntregado = {0};
    BigInt totalDevolucion = {0};
    BigIntArray stockNuevo = {0};

    /* if: comprueba moneda == NULL || denom == NULL || stock == NULL || entregadas == NUL... antes de ejecutar esta rama. */
    if (moneda == NULL || denom == NULL || stock == NULL || entregadas == NULL || devolucion == NULL)
        return 0;
    /* if: comprueba denom->len != stock->len || denom->len != entregadas->len || denom->l... antes de ejecutar esta rama. */
    if (denom->len != stock->len || denom->len != entregadas->len || denom->len != devolucion->len)
        return 0;

    /* if: comprueba !calcular_total_valor(denom, entregadas, &totalEntregado) || antes de ejecutar esta rama. */
    if (!calcular_total_valor(denom, entregadas, &totalEntregado) ||
        !calcular_total_valor(denom, devolucion, &totalDevolucion))
    {
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    /* if: comprueba bigint_compare(&totalEntregado, &totalDevolucion) != 0 antes de ejecutar esta rama. */
    if (bigint_compare(&totalEntregado, &totalDevolucion) != 0)
    {
        printf("Total entregado (%s c) y total solicitado (%s c) deben ser iguales.\n",
               totalEntregado.digits, totalDevolucion.digits);
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    /* if: comprueba !copiar_arreglo_bigint(stock, &stockNuevo) antes de ejecutar esta rama. */
    if (!copiar_arreglo_bigint(stock, &stockNuevo))
    {
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    /* for: itera segun size_t i = 0; i < stockNuevo.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stockNuevo.len; i++)
    {
        BigInt trasEntrada = {0};
        BigInt trasSalida = {0};

        /* if: comprueba !bigint_add(&stockNuevo.items[i], &entregadas->items[i], &trasEntrada... antes de ejecutar esta rama. */
        if (!bigint_add(&stockNuevo.items[i], &entregadas->items[i], &trasEntrada) ||
            bigint_compare(&trasEntrada, &devolucion->items[i]) < 0 ||
            !bigint_subtract(&trasEntrada, &devolucion->items[i], &trasSalida) ||
            !bigint_array_set(&stockNuevo, i, &trasSalida))
        {
            bigint_free(&trasEntrada);
            bigint_free(&trasSalida);
            bigint_array_free(&stockNuevo);
            bigint_free(&totalEntregado);
            bigint_free(&totalDevolucion);
            printf("No se pudo aplicar cambio especifico (stock insuficiente o error de calculo).\n");
            return 0;
        }

        bigint_free(&trasEntrada);
        bigint_free(&trasSalida);
    }

    /* if: comprueba !actualizar_stock_moneda(moneda, &stockNuevo) antes de ejecutar esta rama. */
    if (!actualizar_stock_moneda(moneda, &stockNuevo))
    {
        bigint_array_free(&stockNuevo);
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        printf("No se pudo persistir cambio especifico en stock.txt.\n");
        return 0;
    }

    bigint_array_free(stock);
    *stock = stockNuevo;

    printf("Cambio especifico aplicado. Total entregado/devuelto: %s c\n", totalEntregado.digits);
    registrar_historial("Cambio especifico aplicado con stock (portable).");

    bigint_free(&totalEntregado);
    bigint_free(&totalDevolucion);
    return 1;
}

/* funcion validar_cambio_especifico_ilimitado: contiene la logica principal de esta operacion. */
static int validar_cambio_especifico_ilimitado(const BigIntArray *denom,
                                               const BigIntArray *entregadas,
                                               const BigIntArray *devolucion)
{
    BigInt totalEntregado = {0};
    BigInt totalDevolucion = {0};
    int ok;

    /* if: comprueba denom == NULL || entregadas == NULL || devolucion == NULL antes de ejecutar esta rama. */
    if (denom == NULL || entregadas == NULL || devolucion == NULL)
        return 0;
    /* if: comprueba denom->len != entregadas->len || denom->len != devolucion->len antes de ejecutar esta rama. */
    if (denom->len != entregadas->len || denom->len != devolucion->len)
        return 0;

    /* if: comprueba !calcular_total_valor(denom, entregadas, &totalEntregado) || antes de ejecutar esta rama. */
    if (!calcular_total_valor(denom, entregadas, &totalEntregado) ||
        !calcular_total_valor(denom, devolucion, &totalDevolucion))
    {
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    ok = bigint_compare(&totalEntregado, &totalDevolucion) == 0;
    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
        printf("Total entregado (%s c) y total solicitado (%s c) deben ser iguales.\n",
               totalEntregado.digits, totalDevolucion.digits);

    bigint_free(&totalEntregado);
    bigint_free(&totalDevolucion);
    return ok;
}

/* imprimir_resultado_cambio: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion imprimir_resultado_cambio: contiene la logica principal de esta operacion. */
static void imprimir_resultado_cambio(const BigInt *monto, const BigIntArray *denom, const BigIntArray *solucion)
{
    int hayItems = 0;

    printf("\nResultado devolucion para %s c:\n", monto->digits);
    /* for: itera segun size_t i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < denom->len; i++)
    {
        /* if: comprueba bigint_is_zero(&solucion->items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&solucion->items[i]))
            continue;

        printf("  %s c -> %s\n", denom->items[i].digits, solucion->items[i].digits);
        hayItems = 1;
    }

    /* if: comprueba !hayItems antes de ejecutar esta rama. */
    if (!hayItems)
        printf("  No se requieren monedas para devolver 0.\n");
}

/* calcular_y_aplicar_cambio: Ejecuta logica del algoritmo o calculos matematicos y de stock. */
/* funcion calcular_y_aplicar_cambio: contiene la logica principal de esta operacion. */
static int calcular_y_aplicar_cambio(ModoGUI modo, const char *moneda, const BigIntArray *denom, BigIntArray *stock, int es_caja, int con_limite)
{
    BigInt monto = {0};
    BigIntArray solucion = {0};
    BigIntArray stockNuevo = {0};

    size_t min_monedas = 0;
    size_t max_monedas = 0;
    /* if: comprueba es_caja antes de ejecutar esta rama. */
    if (es_caja)
    {
        char buf1[256], buf2[256];
        BigInt precio = {0}, pago = {0};
        printf("Precio a cobrar: ");
        /* if: comprueba !leer_linea(buf1, sizeof(buf1)) || !bigint_init(&precio, buf1) antes de ejecutar esta rama. */
        if (!leer_linea(buf1, sizeof(buf1)) || !bigint_init(&precio, buf1))
        {
            printf("Precio invalido.\n");
            bigint_free(&precio);
            bigint_free(&monto);
            return 0;
        }
        printf("Cantidad pagada: ");
        /* if: comprueba !leer_linea(buf2, sizeof(buf2)) || !bigint_init(&pago, buf2) antes de ejecutar esta rama. */
        if (!leer_linea(buf2, sizeof(buf2)) || !bigint_init(&pago, buf2))
        {
            printf("Pago invalido.\n");
            bigint_free(&precio);
            bigint_free(&pago);
            bigint_free(&monto);
            return 0;
        }
        /* if: comprueba bigint_compare(&pago, &precio) < 0 antes de ejecutar esta rama. */
        if (bigint_compare(&pago, &precio) < 0)
        {
            printf("El pago es menor que el precio.\n");
            bigint_free(&precio);
            bigint_free(&pago);
            bigint_free(&monto);
            return 0;
        }
        bigint_subtract(&pago, &precio, &monto);
        bigint_free(&precio);
        bigint_free(&pago);
        printf("Cambio a devolver: %s c\n", monto.digits);
    }
    else
    {
        int okMonto = pedir_monto(&monto);
        /* if: comprueba okMonto <= 0 antes de ejecutar esta rama. */
        if (okMonto <= 0)
        {
            printf("Monto invalido.\n");
            bigint_free(&monto);
            return 0;
        }
        /* if: comprueba con_limite antes de ejecutar esta rama. */
        if (con_limite)
        {
            char bufL[256];
            printf("Restriccion de monedas (N, =N, N-M): ");
            /* if: comprueba !leer_linea(bufL, sizeof(bufL)) antes de ejecutar esta rama. */
            if (!leer_linea(bufL, sizeof(bufL)))
            {
                bigint_free(&monto);
                return 0;
            }
            /* if: comprueba !parse_restriccion_gui(bufL, &min_monedas, &max_monedas) antes de ejecutar esta rama. */
            if (!parse_restriccion_gui(bufL, &min_monedas, &max_monedas))
            {
                printf("Restriccion invalida.\n");
                bigint_free(&monto);
                return 0;
            }
        }
    }

    int ok = 0;
    /* if: comprueba modo == MODO_LIMITADO antes de ejecutar esta rama. */
    if (modo == MODO_LIMITADO)
    {
        /* if: comprueba con_limite antes de ejecutar esta rama. */
        if (con_limite)
            ok = calcular_cambio_optimo_stock_con_rango(&monto, denom, stock, min_monedas, max_monedas, &solucion);
        else
            ok = calcular_cambio_optimo_stock(&monto, denom, stock, &solucion);
    }
    else
    {
        /* if: comprueba con_limite antes de ejecutar esta rama. */
        if (con_limite)
            ok = calcular_cambio_optimo_con_rango(&monto, denom, min_monedas, max_monedas, &solucion);
        else
            ok = calcular_cambio_optimo(&monto, denom, &solucion);
    }

    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
    {
        BigInt monto_cubierto = {0};
        BigInt faltante = {0};
        BigIntArray sugerencia = {0};
        BigIntArray stockSugerido = {0};
        char respuesta[32];
        char cmd[32];
        int aceptar = 0;
        int ok_cercano;

        /* if: comprueba con_limite antes de ejecutar esta rama. */
        if (con_limite)
            ok_cercano = (modo == MODO_LIMITADO)
                             ? calcular_cambio_cercano_stock_con_rango(&monto, denom, stock, min_monedas, max_monedas, &monto_cubierto, &sugerencia)
                             : calcular_cambio_cercano_con_rango(&monto, denom, min_monedas, max_monedas, &monto_cubierto, &sugerencia);
        else
            ok_cercano = (modo == MODO_LIMITADO)
                             ? calcular_cambio_cercano_stock_con_rango(&monto, denom, stock, 0, (size_t)-1, &monto_cubierto, &sugerencia)
                             : calcular_cambio_cercano_con_rango(&monto, denom, 0, (size_t)-1, &monto_cubierto, &sugerencia);

        /* if: comprueba ok_cercano && bigint_subtract(&monto, &monto_cubierto, &faltante) antes de ejecutar esta rama. */
        if (ok_cercano && bigint_subtract(&monto, &monto_cubierto, &faltante))
        {
            printf("No existe devolucion exacta con los parametros actuales.\n");
            printf("Sugerencia cercana: devolver %s c (faltan %s c).\n", monto_cubierto.digits, faltante.digits);

            /* while: repite el bloque mientras se cumpla 1. */
            while (1)
            {
                printf("Aceptar sugerencia? (si/no): ");
                /* if: comprueba !leer_linea(respuesta, sizeof(respuesta)) antes de ejecutar esta rama. */
                if (!leer_linea(respuesta, sizeof(respuesta)))
                {
                    bigint_free(&faltante);
                    bigint_free(&monto_cubierto);
                    bigint_array_free(&sugerencia);
                    bigint_free(&monto);
                    bigint_array_free(&solucion);
                    return 0;
                }

                strncpy(cmd, respuesta, sizeof(cmd) - 1);
                cmd[sizeof(cmd) - 1] = '\0';
                a_minusculas(cmd);

                /* if: comprueba strcmp(cmd, "si") == 0 || strcmp(cmd, "s") == 0 || strcmp(cmd, "yes")... antes de ejecutar esta rama. */
                if (strcmp(cmd, "si") == 0 || strcmp(cmd, "s") == 0 || strcmp(cmd, "yes") == 0 || strcmp(cmd, "y") == 0)
                {
                    aceptar = 1;
                    break;
                }

                /* if: comprueba strcmp(cmd, "no") == 0 || strcmp(cmd, "n") == 0 antes de ejecutar esta rama. */
                if (strcmp(cmd, "no") == 0 || strcmp(cmd, "n") == 0)
                    break;

                printf("Respuesta invalida. Escribe si o no.\n");
            }

            /* if: comprueba !aceptar antes de ejecutar esta rama. */
            if (!aceptar)
            {
                printf("Operacion cancelada: no se aplico cambio.\n");
                bigint_free(&faltante);
                bigint_free(&monto_cubierto);
                bigint_array_free(&sugerencia);
                bigint_free(&monto);
                bigint_array_free(&solucion);
                return 0;
            }

            /* if: comprueba modo == MODO_LIMITADO antes de ejecutar esta rama. */
            if (modo == MODO_LIMITADO)
            {
                /* if: comprueba !copiar_arreglo_bigint(stock, &stockSugerido) antes de ejecutar esta rama. */
                if (!copiar_arreglo_bigint(stock, &stockSugerido))
                {
                    printf("No se pudo preparar actualizacion de stock para sugerencia.\n");
                    bigint_free(&faltante);
                    bigint_free(&monto_cubierto);
                    bigint_array_free(&sugerencia);
                    bigint_free(&monto);
                    bigint_array_free(&solucion);
                    return 0;
                }

                /* for: itera segun size_t i = 0; i < stockSugerido.len; i++ para recorrer el bloque. */
                for (size_t i = 0; i < stockSugerido.len; i++)
                {
                    BigInt nuevoStock = {0};

                    /* if: comprueba bigint_is_zero(&sugerencia.items[i]) antes de ejecutar esta rama. */
                    if (bigint_is_zero(&sugerencia.items[i]))
                        continue;

                    /* if: comprueba !bigint_subtract(&stockSugerido.items[i], &sugerencia.items[i], &nuev... antes de ejecutar esta rama. */
                    if (!bigint_subtract(&stockSugerido.items[i], &sugerencia.items[i], &nuevoStock) ||
                        !bigint_array_set(&stockSugerido, i, &nuevoStock))
                    {
                        bigint_free(&nuevoStock);
                        bigint_array_free(&stockSugerido);
                        printf("No se pudo aplicar sugerencia al stock.\n");
                        bigint_free(&faltante);
                        bigint_free(&monto_cubierto);
                        bigint_array_free(&sugerencia);
                        bigint_free(&monto);
                        bigint_array_free(&solucion);
                        return 0;
                    }

                    bigint_free(&nuevoStock);
                }

                /* if: comprueba !actualizar_stock_moneda(moneda, &stockSugerido) antes de ejecutar esta rama. */
                if (!actualizar_stock_moneda(moneda, &stockSugerido))
                {
                    bigint_array_free(&stockSugerido);
                    printf("No se pudo persistir la sugerencia en stock.txt.\n");
                    bigint_free(&faltante);
                    bigint_free(&monto_cubierto);
                    bigint_array_free(&sugerencia);
                    bigint_free(&monto);
                    bigint_array_free(&solucion);
                    return 0;
                }

                bigint_array_free(stock);
                *stock = stockSugerido;
                imprimir_resultado_cambio(&monto_cubierto, denom, &sugerencia);
                printf("Sugerencia cercana aplicada y stock persistido.\n");
                registrar_historial("Cambio cercano aceptado con stock (portable).");
                bigint_free(&faltante);
                bigint_free(&monto_cubierto);
                bigint_array_free(&sugerencia);
                bigint_free(&monto);
                bigint_array_free(&solucion);
                return 1;
            }

            imprimir_resultado_cambio(&monto_cubierto, denom, &sugerencia);
            printf("Sugerencia cercana aceptada en modo ilimitado.\n");
            registrar_historial("Cambio cercano aceptado (portable, ilimitado).");
            bigint_free(&faltante);
            bigint_free(&monto_cubierto);
            bigint_array_free(&sugerencia);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            return 1;
        }
        else
        {
            printf("No existe devolucion exacta con los parametros actuales.\n");
            bigint_free(&faltante);
            bigint_free(&monto_cubierto);
            bigint_array_free(&sugerencia);
        }

        bigint_free(&monto);
        bigint_array_free(&solucion);
        return 0;
    }

    imprimir_resultado_cambio(&monto, denom, &solucion);

    /* if: comprueba modo == MODO_ILIMITADO antes de ejecutar esta rama. */
    if (modo == MODO_ILIMITADO)
    {
        bigint_free(&monto);
        bigint_array_free(&solucion);
        registrar_historial("Cambio portable aplicado (ilimitado).");
        return 1;
    }

    /* if: comprueba !copiar_arreglo_bigint(stock, &stockNuevo) antes de ejecutar esta rama. */
    if (!copiar_arreglo_bigint(stock, &stockNuevo))
    {
        printf("No se pudo preparar actualizacion de stock.\n");
        bigint_free(&monto);
        bigint_array_free(&solucion);
        return 0;
    }

    /* for: itera segun size_t i = 0; i < stockNuevo.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stockNuevo.len; i++)
    {
        BigInt nuevoStock = {0};

        /* if: comprueba bigint_is_zero(&solucion.items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&solucion.items[i]))
            continue;

        /* if: comprueba !bigint_subtract(&stockNuevo.items[i], &solucion.items[i], &nuevoStock) antes de ejecutar esta rama. */
        if (!bigint_subtract(&stockNuevo.items[i], &solucion.items[i], &nuevoStock))
        {
            bigint_array_free(&stockNuevo);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            printf("No se pudo aplicar la devolucion al stock.\n");
            return 0;
        }

        /* if: comprueba !bigint_array_set(&stockNuevo, i, &nuevoStock) antes de ejecutar esta rama. */
        if (!bigint_array_set(&stockNuevo, i, &nuevoStock))
        {
            bigint_free(&nuevoStock);
            bigint_array_free(&stockNuevo);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            printf("No se pudo actualizar stock en memoria.\n");
            return 0;
        }

        bigint_free(&nuevoStock);
    }

    /* if: comprueba !actualizar_stock_moneda(moneda, &stockNuevo) antes de ejecutar esta rama. */
    if (!actualizar_stock_moneda(moneda, &stockNuevo))
    {
        bigint_array_free(&stockNuevo);
        bigint_free(&monto);
        bigint_array_free(&solucion);
        printf("No se pudo persistir la devolucion en stock.txt.\n");
        return 0;
    }

    bigint_array_free(stock);
    *stock = stockNuevo;

    bigint_free(&monto);
    bigint_array_free(&solucion);
    registrar_historial("Cambio portable aplicado con stock.");
    printf("Devolucion aplicada y stock persistido.\n");
    return 1;
}

/* funcion normalizar_clave_moneda: contiene la logica principal de esta operacion. */
static void normalizar_clave_moneda(const char *origen, char *destino, size_t tam_destino)
{
    size_t i = 0;
    size_t j = 0;

    /* if: comprueba destino == NULL || tam_destino == 0 antes de ejecutar esta rama. */
    if (destino == NULL || tam_destino == 0)
        return;

    destino[0] = '\0';
    /* if: comprueba origen == NULL antes de ejecutar esta rama. */
    if (origen == NULL)
        return;

    /* while: repite el bloque mientras se cumpla origen[i] != '\0' && j + 1 < tam_destino. */
    while (origen[i] != '\0' && j + 1 < tam_destino)
    {
        unsigned char c = (unsigned char)origen[i];

        /* if: comprueba c == ' ' antes de ejecutar esta rama. */
        if (c == ' ')
            destino[j++] = '_';
        else
            destino[j++] = (char)tolower(c);
        i++;
    }

    destino[j] = '\0';
}

/* funcion resolver_moneda_por_entrada: contiene la logica principal de esta operacion. */
static int resolver_moneda_por_entrada(char monedas[MAX_MONEDAS][MAX_NOMBRE],
                                       int nMonedas,
                                       const char *entrada,
                                       char salida[MAX_NOMBRE])
{
    char clave[MAX_NOMBRE];
    char *fin;
    long indice;

    /* if: comprueba monedas == NULL || entrada == NULL || salida == NULL || nMonedas <= 0 antes de ejecutar esta rama. */
    if (monedas == NULL || entrada == NULL || salida == NULL || nMonedas <= 0)
        return 0;

    indice = strtol(entrada, &fin, 10);
    /* if: comprueba *fin == '\0' && indice >= 1 && indice <= nMonedas antes de ejecutar esta rama. */
    if (*fin == '\0' && indice >= 1 && indice <= nMonedas)
    {
        strncpy(salida, monedas[indice - 1], MAX_NOMBRE - 1);
        salida[MAX_NOMBRE - 1] = '\0';
        return 1;
    }

    normalizar_clave_moneda(entrada, clave, sizeof(clave));
    /* for: itera segun int i = 0; i < nMonedas; i++ para recorrer el bloque. */
    for (int i = 0; i < nMonedas; i++)
    {
        /* if: comprueba strcmp(monedas[i], clave) == 0 antes de ejecutar esta rama. */
        if (strcmp(monedas[i], clave) == 0)
        {
            strncpy(salida, monedas[i], MAX_NOMBRE - 1);
            salida[MAX_NOMBRE - 1] = '\0';
            return 1;
        }
    }

    return 0;
}

/* funcion ejecutar_conversion_gui_portable: contiene la logica principal de esta operacion. */
static void ejecutar_conversion_gui_portable(char monedas[MAX_MONEDAS][MAX_NOMBRE], int nMonedas)
{
    char entrada[128];
    char origen[MAX_NOMBRE];
    char destino[MAX_NOMBRE];
    char usarStockTxt[32];
    BigInt montoOrigen = {0};
    BigInt montoDestino = {0};
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};
    double tasa = 0.0;
    int usarStock = 0;
    int ok;

    printf("\n--- Conversion entre monedas ---\n");
    /* for: itera segun int i = 0; i < nMonedas; i++ para recorrer el bloque. */
    for (int i = 0; i < nMonedas; i++)
        printf("  [%d] %s\n", i + 1, monedas[i]);

    printf("Moneda origen (indice o nombre, o salir): ");
    /* if: comprueba !leer_linea(entrada, sizeof(entrada)) antes de ejecutar esta rama. */
    if (!leer_linea(entrada, sizeof(entrada)))
        goto cleanup;
    a_minusculas(entrada);
    /* if: comprueba strcmp(entrada, "salir") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "salir") == 0)
        goto cleanup;
    /* if: comprueba !resolver_moneda_por_entrada(monedas, nMonedas, entrada, origen) antes de ejecutar esta rama. */
    if (!resolver_moneda_por_entrada(monedas, nMonedas, entrada, origen))
    {
        printf("Moneda origen invalida.\n");
        goto cleanup;
    }

    /* if: comprueba pedir_monto(&montoOrigen) <= 0 antes de ejecutar esta rama. */
    if (pedir_monto(&montoOrigen) <= 0)
    {
        printf("Monto invalido.\n");
        goto cleanup;
    }

    printf("Moneda destino (indice o nombre, o salir): ");
    /* if: comprueba !leer_linea(entrada, sizeof(entrada)) antes de ejecutar esta rama. */
    if (!leer_linea(entrada, sizeof(entrada)))
        goto cleanup;
    a_minusculas(entrada);
    /* if: comprueba strcmp(entrada, "salir") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "salir") == 0)
        goto cleanup;
    /* if: comprueba !resolver_moneda_por_entrada(monedas, nMonedas, entrada, destino) antes de ejecutar esta rama. */
    if (!resolver_moneda_por_entrada(monedas, nMonedas, entrada, destino))
    {
        printf("Moneda destino invalida.\n");
        goto cleanup;
    }

    printf("Usar stock de la moneda destino? (s/n): ");
    /* if: comprueba !leer_linea(usarStockTxt, sizeof(usarStockTxt)) antes de ejecutar esta rama. */
    if (!leer_linea(usarStockTxt, sizeof(usarStockTxt)))
        goto cleanup;
    a_minusculas(usarStockTxt);
    /* if: comprueba usarStockTxt[0] == 's' || usarStockTxt[0] == 'y' antes de ejecutar esta rama. */
    if (usarStockTxt[0] == 's' || usarStockTxt[0] == 'y')
        usarStock = 1;

    /* if: comprueba !fetch_exchange_rate(origen, destino, &tasa) antes de ejecutar esta rama. */
    if (!fetch_exchange_rate(origen, destino, &tasa))
    {
        printf("No se pudo obtener la tasa de cambio para %s -> %s.\n", origen, destino);
        goto cleanup;
    }

    {
        double src_cents = strtod(montoOrigen.digits, NULL);
        double dst_cents = src_cents * tasa;
        long long redondeado = (long long)(dst_cents + 0.5);
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "%lld", redondeado);
        /* if: comprueba !bigint_init(&montoDestino, buffer) antes de ejecutar esta rama. */
        if (!bigint_init(&montoDestino, buffer))
        {
            printf("No se pudo calcular el monto convertido.\n");
            goto cleanup;
        }
    }

    /* if: comprueba !validar_consistencia_moneda(destino) || !cargar_denominaciones_moned... antes de ejecutar esta rama. */
    if (!validar_consistencia_moneda(destino) || !cargar_denominaciones_moneda(destino, &denom))
    {
        printf("No se pudieron cargar denominaciones de la moneda destino.\n");
        goto cleanup;
    }

    /* if: comprueba usarStock && !cargar_stock_moneda(destino, &stock) antes de ejecutar esta rama. */
    if (usarStock && !cargar_stock_moneda(destino, &stock))
    {
        printf("No se pudo cargar el stock de la moneda destino.\n");
        goto cleanup;
    }

    ok = usarStock
             ? calcular_cambio_optimo_stock(&montoDestino, &denom, &stock, &solucion)
             : calcular_cambio_optimo(&montoDestino, &denom, &solucion);

    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
    {
        printf("No se pudo descomponer %s c en la moneda destino.\n", montoDestino.digits);
        goto cleanup;
    }

    printf("Conversion aplicada: %s %s-centimos -> %s %s-centimos | tasa=%.6f\n",
           montoOrigen.digits, origen, montoDestino.digits, destino, tasa);
    imprimir_resultado_cambio(&montoDestino, &denom, &solucion);
    registrar_historialf("Conversion portable | %s->%s | Origen=%s c | Destino=%s c | Tasa=%.6f",
                         origen, destino, montoOrigen.digits, montoDestino.digits, tasa);

cleanup:
    bigint_free(&montoOrigen);
    bigint_free(&montoDestino);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    bigint_array_free(&solucion);
}

/* pedir_modo: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion pedir_modo: contiene la logica principal de esta operacion. */
static int pedir_modo(ModoGUI *modo)
{
    char entrada[32];

    printf("Modo (limitado/ilimitado/convertir/historial/snapshot/restaurar/reporte o salir): ");
    /* if: comprueba !leer_linea(entrada, sizeof(entrada)) antes de ejecutar esta rama. */
    if (!leer_linea(entrada, sizeof(entrada)))
        return 0;

    a_minusculas(entrada);
    /* if: comprueba strcmp(entrada, "salir") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "salir") == 0)
        return 0;
    /* if: comprueba strcmp(entrada, "historial") == 0 || strcmp(entrada, "h") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "historial") == 0 || strcmp(entrada, "h") == 0)
        return 2;
    /* if: comprueba strcmp(entrada, "snapshot") == 0 || strcmp(entrada, "s") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "snapshot") == 0 || strcmp(entrada, "s") == 0)
        return 3;
    /* if: comprueba strcmp(entrada, "restaurar") == 0 || strcmp(entrada, "u") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "restaurar") == 0 || strcmp(entrada, "u") == 0)
        return 4;
    /* if: comprueba strcmp(entrada, "reporte") == 0 || strcmp(entrada, "g") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "reporte") == 0 || strcmp(entrada, "g") == 0)
        return 5;
    /* if: comprueba strcmp(entrada, "convertir") == 0 || strcmp(entrada, "x") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "convertir") == 0 || strcmp(entrada, "x") == 0)
        return 6;

    /* if: comprueba strcmp(entrada, "limitado") == 0 || strcmp(entrada, "b") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "limitado") == 0 || strcmp(entrada, "b") == 0)
    {
        *modo = MODO_LIMITADO;
        return 1;
    }

    /* if: comprueba strcmp(entrada, "ilimitado") == 0 || strcmp(entrada, "a") == 0 antes de ejecutar esta rama. */
    if (strcmp(entrada, "ilimitado") == 0 || strcmp(entrada, "a") == 0)
    {
        *modo = MODO_ILIMITADO;
        return 1;
    }

    printf("Modo invalido.\n");
    return -1;
}

/* main: Punto de entrada principal. Configura el entorno y lanza la ejecucion. */
/* funcion main: contiene la logica principal de esta operacion. */
int main(void)
{
#if defined(__APPLE__)
    const char *so = "macOS";
#elif defined(__linux__)
    const char *so = "Linux";
#elif defined(__FreeBSD__)
    const char *so = "FreeBSD";
#elif defined(__OpenBSD__)
    const char *so = "OpenBSD";
#elif defined(__NetBSD__)
    const char *so = "NetBSD";
#elif defined(__sun)
    const char *so = "Solaris";
#else
    const char *so = "Unix-like";
#endif
    char monedas[MAX_MONEDAS][MAX_NOMBRE];
    int nMonedas = 0;
    ModoGUI modoActual = MODO_LIMITADO;

    printf("ProgVoraz GUI portable iniciado en %s\n", so);
    printf("Nota: en este SO se usa panel administrador y calculo de devolucion en terminal (sin WinAPI).\n");

    /* if: comprueba !cargar_nombres_moneda(monedas, &nMonedas) antes de ejecutar esta rama. */
    if (!cargar_nombres_moneda(monedas, &nMonedas))
    {
        printf("No se pudieron cargar monedas desde monedas.txt\n");
        return 1;
    }

    /* while: repite el bloque mientras se cumpla 1. */
    while (1)
    {
        char entrada[128];
        char cmd[128];
        BigIntArray denom = {0};
        BigIntArray stock = {0};
        int idxMoneda = -1;

        int estadoModo;

        estadoModo = pedir_modo(&modoActual);
        /* if: comprueba estadoModo == 0 antes de ejecutar esta rama. */
        if (estadoModo == 0)
            break;
        /* if: comprueba estadoModo == 2 antes de ejecutar esta rama. */
        if (estadoModo == 2)
        {
            mostrar_historial_transacciones();
            pausar_pantalla();
            continue;
        }
        /* if: comprueba estadoModo == 3 antes de ejecutar esta rama. */
        if (estadoModo == 3)
        {
            ejecutar_operacion_global("snapshot");
            pausar_pantalla();
            continue;
        }
        /* if: comprueba estadoModo == 4 antes de ejecutar esta rama. */
        if (estadoModo == 4)
        {
            ejecutar_operacion_global("restaurar");
            pausar_pantalla();
            continue;
        }
        /* if: comprueba estadoModo == 5 antes de ejecutar esta rama. */
        if (estadoModo == 5)
        {
            ejecutar_operacion_global("reporte");
            pausar_pantalla();
            continue;
        }
        /* if: comprueba estadoModo == 6 antes de ejecutar esta rama. */
        if (estadoModo == 6)
        {
            ejecutar_conversion_gui_portable(monedas, nMonedas);
            pausar_pantalla();
            continue;
        }
        /* if: comprueba estadoModo < 0 antes de ejecutar esta rama. */
        if (estadoModo < 0)
            continue;

        printf("\nMonedas disponibles (modo %s):\n", modoActual == MODO_LIMITADO ? "stock limitado" : "stock ilimitado");
        /* for: itera segun int i = 0; i < nMonedas; i++ para recorrer el bloque. */
        for (int i = 0; i < nMonedas; i++)
            printf("  [%d] %s\n", i + 1, monedas[i]);

        printf("Selecciona moneda por indice o nombre (o salir): ");
        /* if: comprueba !leer_linea(entrada, sizeof(entrada)) antes de ejecutar esta rama. */
        if (!leer_linea(entrada, sizeof(entrada)))
            break;

        strncpy(cmd, entrada, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';
        a_minusculas(cmd);
        /* if: comprueba strcmp(cmd, "salir") == 0 antes de ejecutar esta rama. */
        if (strcmp(cmd, "salir") == 0)
            break;

        {
            char *fin;
            long v = strtol(entrada, &fin, 10);
            /* if: comprueba *fin == '\0' && v >= 1 && v <= nMonedas antes de ejecutar esta rama. */
            if (*fin == '\0' && v >= 1 && v <= nMonedas)
                idxMoneda = (int)(v - 1);
        }

        /* if: comprueba idxMoneda < 0 antes de ejecutar esta rama. */
        if (idxMoneda < 0)
        {
            /* for: itera segun int i = 0; i < nMonedas; i++ para recorrer el bloque. */
            for (int i = 0; i < nMonedas; i++)
            {

                /* if: comprueba strcmp(monedas[i], entrada) == 0 antes de ejecutar esta rama. */
                if (strcmp(monedas[i], entrada) == 0)
                {
                    idxMoneda = i;
                    break;
                }
            }
        }

        /* if: comprueba idxMoneda < 0 antes de ejecutar esta rama. */
        if (idxMoneda < 0)
        {
            printf("Moneda invalida.\n");
            continue;
        }

        /* if: comprueba !validar_consistencia_moneda(monedas[idxMoneda]) || antes de ejecutar esta rama. */
        if (!validar_consistencia_moneda(monedas[idxMoneda]) ||
            !cargar_denominaciones_moneda(monedas[idxMoneda], &denom) ||
            !cargar_stock_moneda(monedas[idxMoneda], &stock))
        {
            printf("No se pudo cargar denominaciones/stock para esa moneda (archivo inconsistente).\n");
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            continue;
        }

        /* while: repite el bloque mientras se cumpla 1. */
        while (1)
        {
            char accion[32];
            size_t idxDen = 0;
            BigInt delta = {0};
            BigInt nuevo = {0};
            int esSuma;
            int requiereAdmin = (modoActual == MODO_LIMITADO);

            imprimir_panel(monedas[idxMoneda], &denom, &stock);
            /* if: comprueba requiereAdmin antes de ejecutar esta rama. */
            if (requiereAdmin)
                printf("Accion (calcular/caja/limite/especifico/historial/resumen/snapshot/restaurar/reporte/anadir/quitar/modo/volver/salir): ");
            else
                printf("Accion (calcular/caja/limite/especifico/historial/resumen/snapshot/restaurar/reporte/modo/volver/salir): ");

            /* if: comprueba !leer_linea(accion, sizeof(accion)) antes de ejecutar esta rama. */
            if (!leer_linea(accion, sizeof(accion)))
            {
                bigint_free(&delta);
                bigint_array_free(&denom);
                bigint_array_free(&stock);
                return 0;
            }

            a_minusculas(accion);

            /* if: comprueba strcmp(accion, "salir") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "salir") == 0)
            {
                bigint_free(&delta);
                bigint_array_free(&denom);
                bigint_array_free(&stock);
                return 0;
            }

            /* if: comprueba strcmp(accion, "volver") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "volver") == 0)
            {
                bigint_free(&delta);
                break;
            }

            /* if: comprueba strcmp(accion, "modo") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "modo") == 0)
            {
                bigint_free(&delta);
                break;
            }

            /* if: comprueba strcmp(accion, "historial") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "historial") == 0)
            {
                mostrar_historial_transacciones();
                pausar_pantalla();
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba strcmp(accion, "resumen") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "resumen") == 0)
            {
                mostrar_resumen_moneda(monedas[idxMoneda], &denom, requiereAdmin ? &stock : NULL, requiereAdmin);
                pausar_pantalla();
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba strcmp(accion, "snapshot") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "snapshot") == 0)
            {
                ejecutar_operacion_global("snapshot");
                pausar_pantalla();
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba strcmp(accion, "restaurar") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "restaurar") == 0)
            {
                ejecutar_operacion_global("restaurar");
                /* if: comprueba modoActual == MODO_LIMITADO antes de ejecutar esta rama. */
                if (modoActual == MODO_LIMITADO)
                {
                    bigint_array_free(&stock);
                    cargar_stock_moneda(monedas[idxMoneda], &stock);
                }
                pausar_pantalla();
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba strcmp(accion, "reporte") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "reporte") == 0)
            {
                ejecutar_operacion_global("reporte");
                pausar_pantalla();
                bigint_free(&delta);
                continue;
            }
            /* if: comprueba strcmp(accion, "calcular") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "calcular") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock, 0, 0);
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba strcmp(accion, "caja") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "caja") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock, 1, 0);
                bigint_free(&delta);
                continue;
            }
            /* if: comprueba strcmp(accion, "limite") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "limite") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock, 0, 1);
                bigint_free(&delta);
                continue;
            }
            /* if: comprueba strcmp(accion, "especifico") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "especifico") == 0)
            {
                BigIntArray entregadas = {0};
                BigIntArray devolucion = {0};

                /* if: comprueba !pedir_cantidades_por_denominacion(&denom, "Monedas/Billetes entregad... antes de ejecutar esta rama. */
                if (!pedir_cantidades_por_denominacion(&denom, "Monedas/Billetes entregados por el usuario:", &entregadas) ||
                    !pedir_cantidades_por_denominacion(&denom, "Cambio especifico solicitado (cantidades a devolver):", &devolucion))
                {
                    printf("No se pudieron leer cantidades para cambio especifico.\n");
                    bigint_array_free(&entregadas);
                    bigint_array_free(&devolucion);
                    bigint_free(&delta);
                    continue;
                }

                /* if: comprueba (requiereAdmin && aplicar_cambio_especifico(monedas[idxMoneda], &deno... antes de ejecutar esta rama. */
                if ((requiereAdmin && aplicar_cambio_especifico(monedas[idxMoneda], &denom, &stock, &entregadas, &devolucion)) ||
                    (!requiereAdmin && validar_cambio_especifico_ilimitado(&denom, &entregadas, &devolucion)))
                {
                    /* if: comprueba !requiereAdmin antes de ejecutar esta rama. */
                    if (!requiereAdmin)
                    {
                        printf("Cambio especifico aplicado en modo ilimitado.\n");
                        registrar_historial("Cambio especifico aplicado (ilimitado, portable).");
                    }

                    printf("Cambio especifico entregado:\n");
                    /* for: itera segun size_t i = 0; i < denom.len; i++ para recorrer el bloque. */
                    for (size_t i = 0; i < denom.len; i++)
                    {
                        /* if: comprueba bigint_is_zero(&devolucion.items[i]) antes de ejecutar esta rama. */
                        if (bigint_is_zero(&devolucion.items[i]))
                            continue;
                        printf("  %s c -> %s\n", denom.items[i].digits, devolucion.items[i].digits);
                    }
                }

                bigint_array_free(&entregadas);
                bigint_array_free(&devolucion);
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba !requiereAdmin antes de ejecutar esta rama. */
            if (!requiereAdmin)
            {
                printf("Accion invalida para modo stock ilimitado.\n");
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba strcmp(accion, "anadir") == 0 antes de ejecutar esta rama. */
            if (strcmp(accion, "anadir") == 0)
                esSuma = 1;
            /* if: comprueba strcmp(accion, "quitar") == 0 antes de continuar con esta alternativa. */
            else if (strcmp(accion, "quitar") == 0)
                esSuma = 0;
            else
            {
                printf("Accion invalida.\n");
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba pedir_indice(denom.len, &idxDen) <= 0 antes de ejecutar esta rama. */
            if (pedir_indice(denom.len, &idxDen) <= 0)
            {
                printf("Indice invalido.\n");
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba pedir_cantidad(&delta) <= 0 antes de ejecutar esta rama. */
            if (pedir_cantidad(&delta) <= 0)
            {
                printf("Cantidad invalida.\n");
                bigint_free(&delta);
                continue;
            }

            /* if: comprueba esSuma antes de ejecutar esta rama. */
            if (esSuma)
            {

                /* if: comprueba !bigint_add(&stock.items[idxDen], &delta, &nuevo) antes de ejecutar esta rama. */
                if (!bigint_add(&stock.items[idxDen], &delta, &nuevo))
                {
                    printf("No se pudo sumar stock.\n");
                    bigint_free(&delta);
                    bigint_free(&nuevo);
                    continue;
                }
            }
            else
            {

                /* if: comprueba bigint_compare(&stock.items[idxDen], &delta) < 0 || !bigint_subtract(... antes de ejecutar esta rama. */
                if (bigint_compare(&stock.items[idxDen], &delta) < 0 || !bigint_subtract(&stock.items[idxDen], &delta, &nuevo))
                {
                    printf("No se pudo quitar stock (insuficiente o error).\n");
                    bigint_free(&delta);
                    bigint_free(&nuevo);
                    continue;
                }
            }

            /* if: comprueba !bigint_array_set(&stock, idxDen, &nuevo) antes de ejecutar esta rama. */
            if (!bigint_array_set(&stock, idxDen, &nuevo))
            {
                printf("No se pudo aplicar cambio en memoria.\n");
                bigint_free(&delta);
                bigint_free(&nuevo);
                continue;
            }

            /* if: comprueba !actualizar_stock_moneda(monedas[idxMoneda], &stock) antes de ejecutar esta rama. */
            if (!actualizar_stock_moneda(monedas[idxMoneda], &stock))
                printf("No se pudo persistir stock en archivo.\n");
            else
            {
                registrar_historialf("Admin %s (portable) | Moneda=%s | Denom=%s c | Cantidad=%s",
                                     esSuma ? "ANADIR" : "QUITAR",
                                     monedas[idxMoneda],
                                     denom.items[idxDen].digits,
                                     delta.digits);
                printf("Stock actualizado correctamente.\n");
            }

            bigint_free(&delta);
            bigint_free(&nuevo);
        }

        bigint_array_free(&denom);
        bigint_array_free(&stock);
    }

    return 0;
}

#else
/* main: Punto de entrada principal. Configura el entorno y lanza la ejecucion. */
/* funcion main: contiene la logica principal de esta operacion. */
int main(void)
{
    return 0;
}
#endif

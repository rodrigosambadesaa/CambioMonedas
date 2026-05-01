#ifndef _WIN32

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "bigint.h"
#include "moneda_gestion.h"
#include "algoritmo_cambio.h"

#include <time.h>

/* registrar_historial: Guarda transacciones en historial.txt */
static void registrar_historial(const char *mensaje)
{
    FILE *fp = fopen("historial.txt", "a");
    if (fp)
    {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        if (tm_info)
        {
            fprintf(fp, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                    tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, mensaje);
        }
        fclose(fp);
    }
}

static void registrar_historialf(const char *fmt, ...)
{
    char mensaje[512];
    va_list args;

    if (fmt == NULL)
        return;

    va_start(args, fmt);
    vsnprintf(mensaje, sizeof(mensaje), fmt, args);
    va_end(args);
    registrar_historial(mensaje);
}

static void mostrar_historial_transacciones(void)
{
    FILE *fp = fopen("historial.txt", "r");
    char linea[256];

    printf("\n--- HISTORIAL DE TRANSACCIONES ---\n");
    if (!fp)
    {
        printf("No hay transacciones registradas.\n");
    }
    else
    {
        while (fgets(linea, sizeof(linea), fp))
            printf("%s", linea);
        fclose(fp);
    }
    printf("----------------------------------\n\n");
}

static void pausar_pantalla(void)
{
    char buffer[8];

    printf("Presione Enter para continuar...");
    if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL)
        clearerr(stdin);
}

#define MAX_MONEDAS 64
#define MAX_NOMBRE 64

typedef enum
{
    MODO_LIMITADO = 0,
    MODO_ILIMITADO = 1
} ModoGUI;

/* leer_linea: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int leer_linea(char *buffer, size_t tam)
{
    if (buffer == NULL || tam == 0)
        return 0;

    if (fgets(buffer, (int)tam, stdin) == NULL)
        return 0;

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

/* a_minusculas: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static void a_minusculas(char *texto)
{
    size_t i;

    if (texto == NULL)
        return;

    for (i = 0; texto[i] != '\0'; i++)
        texto[i] = (char)tolower((unsigned char)texto[i]);
}

/* es_numero: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int es_numero(const char *s)
{
    size_t i;

    if (s == NULL || *s == '\0')
        return 0;

    for (i = 0; s[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)s[i]))
            return 0;
    }

    return 1;
}

/* cargar_nombres_moneda: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int cargar_nombres_moneda(char monedas[MAX_MONEDAS][MAX_NOMBRE], int *cantidad)
{
    FILE *fp = fopen("monedas.txt", "r");
    char token[256];

    if (fp == NULL || cantidad == NULL)
        return 0;

    *cantidad = 0;

    while (fscanf(fp, "%255s", token) == 1)
    {
        int existe = 0;
        if (es_numero(token))
            continue;

        for (int i = 0; i < *cantidad; i++)
        {

            if (strcmp(monedas[i], token) == 0)
            {
                existe = 1;
                break;
            }
        }

        if (!existe && *cantidad < MAX_MONEDAS)
        {
            size_t n = strlen(token);
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
static void imprimir_panel(const char *moneda, const BigIntArray *denom, const BigIntArray *stock)
{
    printf("\n==============================================\n");
    printf("Panel Administrador (portable) - moneda: %s\n", moneda);
    printf("==============================================\n");
    for (size_t i = 0; i < denom->len; i++)
        printf("[%zu] Denom %s c -> stock %s\n", i + 1, denom->items[i].digits, stock->items[i].digits);
    printf("==============================================\n");
}

/* pedir_indice: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int pedir_indice(size_t max, size_t *indice)
{
    char buffer[64];
    char *fin;
    long v;

    if (indice == NULL || max == 0)
        return 0;

    printf("Indice (1-%zu): ", max);
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    v = strtol(buffer, &fin, 10);
    if (*fin != '\0' || v < 1 || (size_t)v > max)
        return 0;

    *indice = (size_t)(v - 1);
    return 1;
}

/* pedir_cantidad: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int pedir_cantidad(BigInt *delta)
{
    char buffer[2048];
    BigInt tmp = {0};

    if (delta == NULL)
        return 0;

    printf("Cantidad (entero no negativo): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(delta);
    *delta = tmp;
    return 1;
}

/* pedir_monto: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int pedir_monto(BigInt *monto)
{
    char buffer[2048];
    BigInt tmp = {0};

    if (monto == NULL)
        return 0;

    printf("Monto (entero no negativo en centimos): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(monto);
    *monto = tmp;
    return 1;
}

/* copiar_arreglo_bigint: Funcion de precision arbitraria para operar con numeros grandes. */
static int copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)
{
    if (origen == NULL || destino == NULL)
        return 0;

    if (!bigint_array_create(destino, origen->len))
        return 0;

    for (size_t i = 0; i < origen->len; i++)
    {

        if (!bigint_array_set(destino, i, &origen->items[i]))
        {
            bigint_array_free(destino);
            return 0;
        }
    }

    return 1;
}

/* pedir_cantidades_por_denominacion: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int pedir_cantidades_por_denominacion(const BigIntArray *denom, const char *titulo, BigIntArray *cantidades)
{
    if (denom == NULL || cantidades == NULL)
        return 0;

    if (!bigint_array_create(cantidades, denom->len))
        return 0;

    printf("%s\n", titulo);
    for (size_t i = 0; i < denom->len; i++)
    {

        while (1)
        {
            char buffer[2048];
            BigInt tmp = {0};

            printf("Cantidad para %s c: ", denom->items[i].digits);

            if (!leer_linea(buffer, sizeof(buffer)))
            {
                bigint_array_free(cantidades);
                return 0;
            }

            if (!bigint_init(&tmp, buffer))
            {
                printf("Cantidad invalida. Usa entero no negativo.\n");
                continue;
            }

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
static int calcular_total_valor(const BigIntArray *denom, const BigIntArray *cantidades, BigInt *total)
{
    BigInt acumulado = {0};

    if (denom == NULL || cantidades == NULL || total == NULL)
        return 0;
    if (denom->len != cantidades->len)
        return 0;

    if (!bigint_init(&acumulado, "0"))
        return 0;

    for (size_t i = 0; i < denom->len; i++)
    {
        BigInt parcial = {0};
        BigInt nuevo = {0};

        if (bigint_is_zero(&cantidades->items[i]))
            continue;

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

static int aplicar_cambio_especifico(const char *moneda,
                                     const BigIntArray *denom,
                                     BigIntArray *stock,
                                     const BigIntArray *entregadas,
                                     const BigIntArray *devolucion)
{
    BigInt totalEntregado = {0};
    BigInt totalDevolucion = {0};
    BigIntArray stockNuevo = {0};

    if (moneda == NULL || denom == NULL || stock == NULL || entregadas == NULL || devolucion == NULL)
        return 0;
    if (denom->len != stock->len || denom->len != entregadas->len || denom->len != devolucion->len)
        return 0;

    if (!calcular_total_valor(denom, entregadas, &totalEntregado) ||
        !calcular_total_valor(denom, devolucion, &totalDevolucion))
    {
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    if (bigint_compare(&totalEntregado, &totalDevolucion) != 0)
    {
        printf("Total entregado (%s c) y total solicitado (%s c) deben ser iguales.\n",
               totalEntregado.digits, totalDevolucion.digits);
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    if (!copiar_arreglo_bigint(stock, &stockNuevo))
    {
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    for (size_t i = 0; i < stockNuevo.len; i++)
    {
        BigInt trasEntrada = {0};
        BigInt trasSalida = {0};

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

static int validar_cambio_especifico_ilimitado(const BigIntArray *denom,
                                               const BigIntArray *entregadas,
                                               const BigIntArray *devolucion)
{
    BigInt totalEntregado = {0};
    BigInt totalDevolucion = {0};
    int ok;

    if (denom == NULL || entregadas == NULL || devolucion == NULL)
        return 0;
    if (denom->len != entregadas->len || denom->len != devolucion->len)
        return 0;

    if (!calcular_total_valor(denom, entregadas, &totalEntregado) ||
        !calcular_total_valor(denom, devolucion, &totalDevolucion))
    {
        bigint_free(&totalEntregado);
        bigint_free(&totalDevolucion);
        return 0;
    }

    ok = bigint_compare(&totalEntregado, &totalDevolucion) == 0;
    if (!ok)
        printf("Total entregado (%s c) y total solicitado (%s c) deben ser iguales.\n",
               totalEntregado.digits, totalDevolucion.digits);

    bigint_free(&totalEntregado);
    bigint_free(&totalDevolucion);
    return ok;
}

/* imprimir_resultado_cambio: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static void imprimir_resultado_cambio(const BigInt *monto, const BigIntArray *denom, const BigIntArray *solucion)
{
    int hayItems = 0;

    printf("\nResultado devolucion para %s c:\n", monto->digits);
    for (size_t i = 0; i < denom->len; i++)
    {
        if (bigint_is_zero(&solucion->items[i]))
            continue;

        printf("  %s c -> %s\n", denom->items[i].digits, solucion->items[i].digits);
        hayItems = 1;
    }

    if (!hayItems)
        printf("  No se requieren monedas para devolver 0.\n");
}

/* calcular_y_aplicar_cambio: Ejecuta logica del algoritmo o calculos matematicos y de stock. */
static int calcular_y_aplicar_cambio(ModoGUI modo, const char *moneda, const BigIntArray *denom, BigIntArray *stock, int es_caja, int con_limite)
{
    BigInt monto = {0};
    BigIntArray solucion = {0};
    BigIntArray stockNuevo = {0};

    size_t limite_monedas = 0;
    if (es_caja)
    {
        char buf1[256], buf2[256];
        BigInt precio = {0}, pago = {0};
        printf("Precio a cobrar: ");
        if (!leer_linea(buf1, sizeof(buf1)) || !bigint_init(&precio, buf1))
        {
            printf("Precio invalido.\n");
            bigint_free(&precio);
            bigint_free(&monto);
            return 0;
        }
        printf("Cantidad pagada: ");
        if (!leer_linea(buf2, sizeof(buf2)) || !bigint_init(&pago, buf2))
        {
            printf("Pago invalido.\n");
            bigint_free(&precio);
            bigint_free(&pago);
            bigint_free(&monto);
            return 0;
        }
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
        if (okMonto <= 0)
        {
            printf("Monto invalido.\n");
            bigint_free(&monto);
            return 0;
        }
        if (con_limite)
        {
            char bufL[256];
            printf("Limite maximo de monedas: ");
            if (!leer_linea(bufL, sizeof(bufL)))
            {
                bigint_free(&monto);
                return 0;
            }
            long v = strtol(bufL, NULL, 10);
            if (v < 1)
            {
                printf("Limite invalido.\n");
                bigint_free(&monto);
                return 0;
            }
            limite_monedas = (size_t)v;
        }
    }

    int ok = 0;
    if (modo == MODO_LIMITADO)
    {
        if (con_limite)
            ok = calcular_cambio_optimo_stock_con_limite(&monto, denom, stock, limite_monedas, &solucion);
        else
            ok = calcular_cambio_optimo_stock(&monto, denom, stock, &solucion);
    }
    else
    {
        if (con_limite)
            ok = calcular_cambio_optimo_con_limite(&monto, denom, limite_monedas, &solucion);
        else
            ok = calcular_cambio_optimo(&monto, denom, &solucion);
    }

    if (!ok)
    {
        printf("No existe devolucion exacta con los parametros actuales.\n");
        bigint_free(&monto);
        bigint_array_free(&solucion);
        return 0;
    }

    imprimir_resultado_cambio(&monto, denom, &solucion);

    if (modo == MODO_ILIMITADO)
    {
        bigint_free(&monto);
        bigint_array_free(&solucion);
        registrar_historial("Cambio portable aplicado (ilimitado).");
        return 1;
    }

    if (!copiar_arreglo_bigint(stock, &stockNuevo))
    {
        printf("No se pudo preparar actualizacion de stock.\n");
        bigint_free(&monto);
        bigint_array_free(&solucion);
        return 0;
    }

    for (size_t i = 0; i < stockNuevo.len; i++)
    {
        BigInt nuevoStock = {0};

        if (bigint_is_zero(&solucion.items[i]))
            continue;

        if (!bigint_subtract(&stockNuevo.items[i], &solucion.items[i], &nuevoStock))
        {
            bigint_array_free(&stockNuevo);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            printf("No se pudo aplicar la devolucion al stock.\n");
            return 0;
        }

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

/* pedir_modo: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int pedir_modo(ModoGUI *modo)
{
    char entrada[32];

    printf("Modo (limitado/ilimitado/historial o salir): ");
    if (!leer_linea(entrada, sizeof(entrada)))
        return 0;

    a_minusculas(entrada);
    if (strcmp(entrada, "salir") == 0)
        return 0;
    if (strcmp(entrada, "historial") == 0 || strcmp(entrada, "h") == 0)
        return 2;

    if (strcmp(entrada, "limitado") == 0 || strcmp(entrada, "b") == 0)
    {
        *modo = MODO_LIMITADO;
        return 1;
    }

    if (strcmp(entrada, "ilimitado") == 0 || strcmp(entrada, "a") == 0)
    {
        *modo = MODO_ILIMITADO;
        return 1;
    }

    printf("Modo invalido.\n");
    return -1;
}

/* main: Punto de entrada principal. Configura el entorno y lanza la ejecucion. */
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

    if (!cargar_nombres_moneda(monedas, &nMonedas))
    {
        printf("No se pudieron cargar monedas desde monedas.txt\n");
        return 1;
    }

    while (1)
    {
        char entrada[128];
        char cmd[128];
        BigIntArray denom = {0};
        BigIntArray stock = {0};
        int idxMoneda = -1;

        int estadoModo;

        estadoModo = pedir_modo(&modoActual);
        if (estadoModo == 0)
            break;
        if (estadoModo == 2)
        {
            mostrar_historial_transacciones();
            pausar_pantalla();
            continue;
        }
        if (estadoModo < 0)
            continue;

        printf("\nMonedas disponibles (modo %s):\n", modoActual == MODO_LIMITADO ? "stock limitado" : "stock ilimitado");
        for (int i = 0; i < nMonedas; i++)
            printf("  [%d] %s\n", i + 1, monedas[i]);

        printf("Selecciona moneda por indice o nombre (o salir): ");
        if (!leer_linea(entrada, sizeof(entrada)))
            break;

        strncpy(cmd, entrada, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';
        a_minusculas(cmd);
        if (strcmp(cmd, "salir") == 0)
            break;

        {
            char *fin;
            long v = strtol(entrada, &fin, 10);
            if (*fin == '\0' && v >= 1 && v <= nMonedas)
                idxMoneda = (int)(v - 1);
        }

        if (idxMoneda < 0)
        {
            for (int i = 0; i < nMonedas; i++)
            {

                if (strcmp(monedas[i], entrada) == 0)
                {
                    idxMoneda = i;
                    break;
                }
            }
        }

        if (idxMoneda < 0)
        {
            printf("Moneda invalida.\n");
            continue;
        }

        if (!cargar_denominaciones_moneda(monedas[idxMoneda], &denom) || !cargar_stock_moneda(monedas[idxMoneda], &stock) || denom.len != stock.len)
        {
            printf("No se pudo cargar denominaciones/stock para esa moneda.\n");
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            continue;
        }

        while (1)
        {
            char accion[32];
            size_t idxDen = 0;
            BigInt delta = {0};
            BigInt nuevo = {0};
            int esSuma;
            int requiereAdmin = (modoActual == MODO_LIMITADO);

            imprimir_panel(monedas[idxMoneda], &denom, &stock);
            if (requiereAdmin)
                printf("Accion (calcular/caja/limite/especifico/historial/anadir/quitar/modo/volver/salir): ");
            else
                printf("Accion (calcular/caja/limite/especifico/historial/modo/volver/salir): ");

            if (!leer_linea(accion, sizeof(accion)))
            {
                bigint_free(&delta);
                bigint_array_free(&denom);
                bigint_array_free(&stock);
                return 0;
            }

            a_minusculas(accion);

            if (strcmp(accion, "salir") == 0)
            {
                bigint_free(&delta);
                bigint_array_free(&denom);
                bigint_array_free(&stock);
                return 0;
            }

            if (strcmp(accion, "volver") == 0)
            {
                bigint_free(&delta);
                break;
            }

            if (strcmp(accion, "modo") == 0)
            {
                bigint_free(&delta);
                break;
            }

            if (strcmp(accion, "historial") == 0)
            {
                mostrar_historial_transacciones();
                pausar_pantalla();
                bigint_free(&delta);
                continue;
            }
            if (strcmp(accion, "calcular") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock, 0, 0);
                bigint_free(&delta);
                continue;
            }

            if (strcmp(accion, "caja") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock, 1, 0);
                bigint_free(&delta);
                continue;
            }
            if (strcmp(accion, "limite") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock, 0, 1);
                bigint_free(&delta);
                continue;
            }
            if (strcmp(accion, "especifico") == 0)
            {
                BigIntArray entregadas = {0};
                BigIntArray devolucion = {0};

                if (!pedir_cantidades_por_denominacion(&denom, "Monedas/Billetes entregados por el usuario:", &entregadas) ||
                    !pedir_cantidades_por_denominacion(&denom, "Cambio especifico solicitado (cantidades a devolver):", &devolucion))
                {
                    printf("No se pudieron leer cantidades para cambio especifico.\n");
                    bigint_array_free(&entregadas);
                    bigint_array_free(&devolucion);
                    bigint_free(&delta);
                    continue;
                }

                if ((requiereAdmin && aplicar_cambio_especifico(monedas[idxMoneda], &denom, &stock, &entregadas, &devolucion)) ||
                    (!requiereAdmin && validar_cambio_especifico_ilimitado(&denom, &entregadas, &devolucion)))
                {
                    if (!requiereAdmin)
                    {
                        printf("Cambio especifico aplicado en modo ilimitado.\n");
                        registrar_historial("Cambio especifico aplicado (ilimitado, portable).");
                    }

                    printf("Cambio especifico entregado:\n");
                    for (size_t i = 0; i < denom.len; i++)
                    {
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

            if (!requiereAdmin)
            {
                printf("Accion invalida para modo stock ilimitado.\n");
                bigint_free(&delta);
                continue;
            }

            if (strcmp(accion, "anadir") == 0)
                esSuma = 1;
            else if (strcmp(accion, "quitar") == 0)
                esSuma = 0;
            else
            {
                printf("Accion invalida.\n");
                bigint_free(&delta);
                continue;
            }

            if (pedir_indice(denom.len, &idxDen) <= 0)
            {
                printf("Indice invalido.\n");
                bigint_free(&delta);
                continue;
            }

            if (pedir_cantidad(&delta) <= 0)
            {
                printf("Cantidad invalida.\n");
                bigint_free(&delta);
                continue;
            }

            if (esSuma)
            {

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

                if (bigint_compare(&stock.items[idxDen], &delta) < 0 || !bigint_subtract(&stock.items[idxDen], &delta, &nuevo))
                {
                    printf("No se pudo quitar stock (insuficiente o error).\n");
                    bigint_free(&delta);
                    bigint_free(&nuevo);
                    continue;
                }
            }

            if (!bigint_array_set(&stock, idxDen, &nuevo))
            {
                printf("No se pudo aplicar cambio en memoria.\n");
                bigint_free(&delta);
                bigint_free(&nuevo);
                continue;
            }

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
int main(void)
{
    return 0;
}
#endif

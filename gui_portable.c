#ifndef _WIN32

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bigint.h"
#include "moneda_gestion.h"

#define MAX_MONEDAS 64
#define MAX_NOMBRE 64

typedef enum
{
    MODO_LIMITADO = 0,
    MODO_ILIMITADO = 1
} ModoGUI;

static int leer_linea(char *buffer, size_t tam)
{
    if (fgets(buffer, (int)tam, stdin) == NULL)
        return 0;

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

static void a_minusculas(char *texto)
{
    size_t i;

    if (texto == NULL)
        return;

    for (i = 0; texto[i] != '\0'; i++)
        texto[i] = (char)tolower((unsigned char)texto[i]);
}

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

static void imprimir_panel(const char *moneda, const BigIntArray *denom, const BigIntArray *stock)
{
    printf("\n==============================================\n");
    printf("Panel Administrador (portable) - moneda: %s\n", moneda);
    printf("==============================================\n");
    for (size_t i = 0; i < denom->len; i++)
        printf("[%zu] Denom %s c -> stock %s\n", i + 1, denom->items[i].digits, stock->items[i].digits);
    printf("==============================================\n");
}

static int pedir_indice(size_t max, size_t *indice)
{
    char buffer[64];
    char *fin;
    long v;

    printf("Indice (1-%zu): ", max);
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    v = strtol(buffer, &fin, 10);
    if (*fin != '\0' || v < 1 || (size_t)v > max)
        return 0;

    *indice = (size_t)(v - 1);
    return 1;
}

static int pedir_cantidad(BigInt *delta)
{
    char buffer[2048];
    BigInt tmp = {0};

    printf("Cantidad (entero no negativo): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(delta);
    *delta = tmp;
    return 1;
}

static int pedir_monto(BigInt *monto)
{
    char buffer[2048];
    BigInt tmp = {0};

    printf("Monto (entero no negativo en centimos): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(monto);
    *monto = tmp;
    return 1;
}

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

static int calcular_cambio_stock(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, BigIntArray *solucion)
{
    BigInt restante = {0};

    if (monto == NULL || denominaciones == NULL || stock == NULL || solucion == NULL)
        return 0;
    if (denominaciones->len != stock->len)
        return 0;

    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;

    if (!bigint_set(&restante, monto) && !bigint_init(&restante, monto->digits))
    {
        bigint_array_free(solucion);
        return 0;
    }

    for (size_t i = 0; i < denominaciones->len; i++)
    {
        BigInt max_usables = {0};
        BigInt r = {0};
        BigInt tomar = {0};

        if (bigint_is_zero(&denominaciones->items[i]))
            continue;

        if (!bigint_divmod(&restante, &denominaciones->items[i], &max_usables, &r))
            goto fallo;

        if (bigint_compare(&max_usables, &stock->items[i]) > 0)
        {
            if (!bigint_set(&tomar, &stock->items[i]) && !bigint_init(&tomar, stock->items[i].digits))
                goto fallo;
        }
        else
        {
            if (!bigint_set(&tomar, &max_usables) && !bigint_init(&tomar, max_usables.digits))
                goto fallo;
        }

        if (!bigint_array_set(solucion, i, &tomar))
            goto fallo;

        if (!bigint_is_zero(&tomar))
        {
            BigInt uso = {0};
            BigInt nuevo_restante = {0};

            if (!bigint_multiply(&denominaciones->items[i], &tomar, &uso))
            {
                bigint_free(&uso);
                goto fallo;
            }

            if (!bigint_subtract(&restante, &uso, &nuevo_restante))
            {
                bigint_free(&uso);
                bigint_free(&nuevo_restante);
                goto fallo;
            }

            bigint_free(&restante);
            restante = nuevo_restante;
            bigint_free(&uso);
        }

        bigint_free(&max_usables);
        bigint_free(&r);
        bigint_free(&tomar);

        if (bigint_is_zero(&restante))
            break;

        continue;

    fallo:
        bigint_free(&max_usables);
        bigint_free(&r);
        bigint_free(&tomar);
        bigint_free(&restante);
        bigint_array_free(solucion);
        return 0;
    }

    if (!bigint_is_zero(&restante))
    {
        bigint_free(&restante);
        bigint_array_free(solucion);
        return 0;
    }

    bigint_free(&restante);
    return 1;
}

static int calcular_cambio_ilimitado(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)
{
    BigInt restante = {0};

    if (monto == NULL || denominaciones == NULL || solucion == NULL)
        return 0;

    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;

    if (!bigint_set(&restante, monto) && !bigint_init(&restante, monto->digits))
    {
        bigint_array_free(solucion);
        return 0;
    }

    for (size_t i = 0; i < denominaciones->len; i++)
    {
        BigInt q = {0};
        BigInt r = {0};

        if (bigint_is_zero(&denominaciones->items[i]))
            continue;

        if (!bigint_divmod(&restante, &denominaciones->items[i], &q, &r))
        {
            bigint_free(&q);
            bigint_free(&r);
            bigint_free(&restante);
            bigint_array_free(solucion);
            return 0;
        }

        if (!bigint_array_set(solucion, i, &q))
        {
            bigint_free(&q);
            bigint_free(&r);
            bigint_free(&restante);
            bigint_array_free(solucion);
            return 0;
        }

        bigint_free(&restante);
        restante = r;
        bigint_free(&q);

        if (bigint_is_zero(&restante))
            break;
    }

    if (!bigint_is_zero(&restante))
    {
        bigint_free(&restante);
        bigint_array_free(solucion);
        return 0;
    }

    bigint_free(&restante);
    return 1;
}

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

static int calcular_y_aplicar_cambio(ModoGUI modo, const char *moneda, const BigIntArray *denom, BigIntArray *stock)
{
    BigInt monto = {0};
    BigIntArray solucion = {0};
    BigIntArray stockNuevo = {0};

    int okMonto = pedir_monto(&monto);
    if (okMonto <= 0)
    {
        printf("Monto invalido.\n");
        bigint_free(&monto);
        return 0;
    }

    if (modo == MODO_LIMITADO)
    {
        if (!calcular_cambio_stock(&monto, denom, stock, &solucion))
        {
            printf("No existe devolucion exacta con el stock actual.\n");
            bigint_free(&monto);
            bigint_array_free(&solucion);
            return 0;
        }
    }
    else
    {
        if (!calcular_cambio_ilimitado(&monto, denom, &solucion))
        {
            printf("No existe devolucion exacta con denominaciones actuales.\n");
            bigint_free(&monto);
            bigint_array_free(&solucion);
            return 0;
        }
    }

    imprimir_resultado_cambio(&monto, denom, &solucion);

    if (modo == MODO_ILIMITADO)
    {
        bigint_free(&monto);
        bigint_array_free(&solucion);
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
    printf("Devolucion aplicada y stock persistido.\n");
    return 1;
}

static int pedir_modo(ModoGUI *modo)
{
    char entrada[32];

    printf("Modo (limitado/ilimitado o salir): ");
    if (!leer_linea(entrada, sizeof(entrada)))
        return 0;

    a_minusculas(entrada);
    if (strcmp(entrada, "salir") == 0)
        return 0;
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
                printf("Accion (calcular/especifico/anadir/quitar/modo/volver/salir): ");
            else
                printf("Accion (calcular/especifico/modo/volver/salir): ");
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

            if (strcmp(accion, "calcular") == 0)
            {
                calcular_y_aplicar_cambio(modoActual, monedas[idxMoneda], &denom, &stock);
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
                        printf("Cambio especifico aplicado en modo ilimitado.\n");

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
                printf("Stock actualizado correctamente.\n");

            bigint_free(&delta);
            bigint_free(&nuevo);
        }

        bigint_array_free(&denom);
        bigint_array_free(&stock);
    }

    return 0;
}

#else
int main(void)
{
    return 0;
}
#endif

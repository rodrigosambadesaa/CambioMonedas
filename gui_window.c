#ifdef _WIN32
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <time.h>
#include "bigint.h"
#include "moneda_gestion.h"
#include "algoritmo_cambio.h"

#define MAX_MONEDAS 32
#define MAX_NOMBRE 32

#define ID_COMBO_MONEDA 1001
#define ID_BTN_CARGAR 1002
#define ID_LIST_STOCK 1003
#define ID_COMBO_DENOM 1004
#define ID_EDIT_CANTIDAD 1005
#define ID_BTN_AGREGAR 1006
#define ID_BTN_QUITAR 1007
#define ID_BTN_RECARGAR 1008
#define ID_EDIT_MONTO 1009
#define ID_BTN_CALCULAR 1010
#define ID_LIST_RESULTADO 1011
#define ID_RADIO_LIMITADO 1012
#define ID_RADIO_ILIMITADO 1013
#define ID_EDIT_LOTE 1014
#define ID_BTN_AGREGAR_LOTE 1015
#define ID_BTN_QUITAR_LOTE 1016
#define ID_EDIT_ENTRADA_CAMBIO 1017
#define ID_EDIT_DEVOLUCION_CAMBIO 1018
#define ID_BTN_CAMBIO_ESPECIFICO 1019
#define ID_CHECK_CAJA 1020
#define ID_EDIT_PRECIO 1021
#define ID_EDIT_PAGO 1022
#define ID_EDIT_LIMITE 1023
#define ID_BTN_HISTORIAL 1024
#define ID_BTN_RESUMEN 1025

static HWND g_comboMoneda;
static HWND g_listStock;
static HWND g_comboDenom;
static HWND g_editCantidad;
static HWND g_btnAgregar;
static HWND g_btnQuitar;
static HWND g_btnCargar;
static HWND g_btnRecargar;
static HWND g_editMonto;
static HWND g_btnCalcular;
static HWND g_listResultado;
static HWND g_radioLimitado;
static HWND g_radioIlimitado;
static HWND g_editLote;
static HWND g_btnAgregarLote;
static HWND g_btnQuitarLote;
static HWND g_editEntradaCambio;
static HWND g_editDevolucionCambio;
static HWND g_btnCambioEspecifico;
static HWND g_checkCaja;
static HWND g_editPrecio;
static HWND g_editPago;
static HWND g_editLimite;
static HWND g_btnHistorial;
static HWND g_btnResumen;

static char g_monedas[MAX_MONEDAS][MAX_NOMBRE];
static int g_monedasCount = 0;
static int g_monedaActiva = -1;
static int g_modo_stock_limitado = 1;

static BigIntArray g_denom = {0};
static BigIntArray g_stock = {0};

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

/* es_numero: Revisa si una cadena entrante es estrictamente numerica (ej: '500'). */
static int es_numero(const char *s)
{
    size_t i;

    if (s == NULL || s[0] == '\0')
        return 0;

    for (i = 0; s[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)s[i]))
            return 0;
    }

    return 1;
}

/* liberar_datos_moneda: Macro local para purgar cache de DB en UI y reiniciar a neutro. */
static void liberar_datos_moneda(void)
{
    bigint_array_free(&g_denom);
    bigint_array_free(&g_stock);
    g_monedaActiva = -1;
}

/* cargar_nombres_moneda: Parser simplificado que lee "monedas.txt" buscando los PAISES/NOMBRES y no sus denominaciones en si. */
static int cargar_nombres_moneda(void)
{
    FILE *fp = fopen("monedas.txt", "r");
    char token[256];

    g_monedasCount = 0;
    if (fp == NULL)
        return 0;

    /* Extrae iterativamente palabras (ignorando blancos y tabs autom.) */
    while (fscanf(fp, "%255s", token) == 1)
    {
        /* Si NO es numero, lo asumimos como el titular o nombre identificador de la divisa (ej "euro"). */
        if (!es_numero(token))
        {
            int yaExiste = 0;
            for (int i = 0; i < g_monedasCount; i++)
            {
                /* Previene leer divisas duplicadas que alguien escribio mal en el TXT. */
                if (strcmp(g_monedas[i], token) == 0)
                {
                    yaExiste = 1;
                    break;
                }
            }

            /* Si superó la prueba de duplicidad y aún caben en el selector Combo de Windows... */
            if (!yaExiste && g_monedasCount < MAX_MONEDAS)
            {
                size_t n = strlen(token);
                if (n >= MAX_NOMBRE)
                    n = MAX_NOMBRE - 1;
                memcpy(g_monedas[g_monedasCount], token, n);
                g_monedas[g_monedasCount][n] = '\0';
                g_monedasCount++;
            }
        }
    }

    fclose(fp);
    return g_monedasCount > 0;
}

/* refrescar_combo_monedas: Vacia y repuebla el Dropdown de Windows API listando las monedas halladas. */
static void refrescar_combo_monedas(void)
{
    SendMessageA(g_comboMoneda, CB_RESETCONTENT, 0, 0);

    for (int i = 0; i < g_monedasCount; i++)
        SendMessageA(g_comboMoneda, CB_ADDSTRING, 0, (LPARAM)g_monedas[i]);

    if (g_monedasCount > 0)
        SendMessageA(g_comboMoneda, CB_SETCURSEL, 0, 0);
}

/* mostrar_error: Dispara un Error MsgBox nativo de OS. */
static void mostrar_error(const char *titulo, const char *mensaje)
{
    MessageBoxA(NULL, mensaje, titulo, MB_ICONERROR | MB_OK);
}

/* mostrar_info: Dispara una Dialog Info MsgBox nativa. */
static void mostrar_info(const char *titulo, const char *mensaje)
{
    MessageBoxA(NULL, mensaje, titulo, MB_ICONINFORMATION | MB_OK);
}

/* limpiar_resultado_cambio: Purga el TextBox o ListBox donde se muestran los billetes calculados por el backtracking. */
static void limpiar_resultado_cambio(void)
{
    SendMessageA(g_listResultado, LB_RESETCONTENT, 0, 0);
    SendMessageA(g_listResultado, LB_SETHORIZONTALEXTENT, 0, 0);
}

/* configurar_modo_stock: Enablea/Disablea secciones enteras de la App dependiendo de si clickeamos "Limitado" o "Ilimitado". */
static void configurar_modo_stock(int limitado)
{
    g_modo_stock_limitado = limitado ? 1 : 0;

    SendMessageA(g_radioLimitado, BM_SETCHECK, g_modo_stock_limitado ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageA(g_radioIlimitado, BM_SETCHECK, g_modo_stock_limitado ? BST_UNCHECKED : BST_CHECKED, 0);

    EnableWindow(g_comboDenom, g_modo_stock_limitado);
    EnableWindow(g_editCantidad, g_modo_stock_limitado);
    EnableWindow(g_btnAgregar, g_modo_stock_limitado);
    EnableWindow(g_btnQuitar, g_modo_stock_limitado);
    EnableWindow(g_editLote, g_modo_stock_limitado);
    EnableWindow(g_btnAgregarLote, g_modo_stock_limitado);
    EnableWindow(g_btnQuitarLote, g_modo_stock_limitado);
    EnableWindow(g_editEntradaCambio, TRUE);
    EnableWindow(g_editDevolucionCambio, TRUE);
    EnableWindow(g_btnCambioEspecifico, TRUE);
    EnableWindow(g_checkCaja, TRUE);
    if (SendMessageA(g_checkCaja, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        EnableWindow(g_editPrecio, TRUE);
        EnableWindow(g_editPago, TRUE);
        EnableWindow(g_editMonto, FALSE);
    }
    else
    {
        EnableWindow(g_editPrecio, FALSE);
        EnableWindow(g_editPago, FALSE);
        EnableWindow(g_editMonto, TRUE);
    }
    EnableWindow(g_editLimite, TRUE);
}

/* precargar_ceros_en_edit: Rellena con caracteres '0\n0\n0' las multilines de inputs "por lote" para guiar al usuario. */
static void precargar_ceros_en_edit(HWND edit)
{
    char texto[4096];
    size_t pos = 0;

    texto[0] = '\0';
    if (edit == NULL)
        return;

    /* Valida y si la divisa no esta cargada... */
    if (g_denom.items == NULL || g_denom.len == 0)
    {
        SetWindowTextA(edit, "");
        return;
    }

    for (size_t i = 0; i < g_denom.len; i++)
    {
        int escritos;
        size_t restante = sizeof(texto) - pos;

        if (restante <= 1)
            break;

        escritos = snprintf(texto + pos, restante, "%s%s", (i == 0) ? "" : "\r\n", "0");
        if (escritos < 0)
            break;

        /* Detecta BufferTruncation. */
        if ((size_t)escritos >= restante)
            /* Detecta BufferTruncation. */
            if ((size_t)escritos >= restante)
            {
                pos = sizeof(texto) - 1;
                break;
            }

        pos += (size_t)escritos;
    }

    texto[pos] = '\0';
    SetWindowTextA(edit, texto);
}

/* precargar_cantidades_lote: Macro-funcion que aplica el relleno de ceros a todos los inputs multiline en pantalla. */
static void precargar_cantidades_lote(void)
{
    precargar_ceros_en_edit(g_editLote);
    precargar_ceros_en_edit(g_editEntradaCambio);
    precargar_ceros_en_edit(g_editDevolucionCambio);
}

/* leer_cantidades_desde_edit: Convierte un campo de texto multilinea ("0\n150\n30\n...") en un Array de BigInts manejable. */
static int leer_cantidades_desde_edit(HWND edit, BigIntArray *deltas)
{
    int len;
    char *buffer;
    char *ctx;
    char *token;
    size_t idx = 0;

    /* Valida punteros y estado de base de datos antes de siquiera asomarse a leer UI. */
    if (edit == NULL || deltas == NULL || g_denom.items == NULL || g_denom.len == 0)
        return 0;

    len = GetWindowTextLengthA(edit);
    if (len <= 0)
        return 0;

    buffer = (char *)malloc((size_t)len + 1);
    if (buffer == NULL)
        return 0;

    GetWindowTextA(edit, buffer, len + 1);

    /* Construye el vector de precision arbitraria en memoria usando la dimension de Denominaciones conocidas. */
    if (!bigint_array_create(deltas, g_denom.len))
    {
        free(buffer);
        return 0;
    }

    token = strtok_s(buffer, " \t\r\n", &ctx);
    /* Itera cada numero hallado en el Multiline. */
    while (token != NULL)
    {
        BigInt delta = {0};

        /* Validador de desbordamiento de Array (ej. el usuario metio MAS lineas de texto que las permitidas por la Divisa). */
        if (idx >= g_denom.len)
        {
            bigint_array_free(deltas);
            free(buffer);
            return 0;
        }

        /* Si el token hallado es basura ("hola", "NAN", letras...) `bigint_init` fallara y atrapara el error. */
        if (!bigint_init(&delta, token))
        {
            bigint_array_free(deltas);
            free(buffer);
            return 0;
        }

        /* Inserta el objeto numerico leido en la posicion correspondiente del Array Resultante. */
        if (!bigint_array_set(deltas, idx, &delta))
        {
            bigint_free(&delta);
            bigint_array_free(deltas);
            free(buffer);
            return 0;
        }

        bigint_free(&delta);
        idx++;
        token = strtok_s(NULL, " \t\r\n", &ctx);
    }

    free(buffer);

    /* Validador estricto. Si metió MENOS numeros que los billetes que existen en el pais... */
    if (idx != g_denom.len)
    {
        bigint_array_free(deltas);
        return 0;
    }

    return 1;
}

/* leer_cantidades_lote: Wrapper simplificado que en ruta predeterminada asume el input box "g_editLote" (Administrador). */
static int leer_cantidades_lote(BigIntArray *deltas)
{
    return leer_cantidades_desde_edit(g_editLote, deltas);
}

/* ajustar_scroll_horizontal_lista: Funcion para evitar que strings largos en la GUI queden cortados y ocultos. */
static void ajustar_scroll_horizontal_lista(HWND lista, int max_chars)
{
    int extent = max_chars * 8;
    if (lista == NULL)
        return;
    if (extent < 0)
        extent = 0;
    SendMessageA(lista, LB_SETHORIZONTALEXTENT, (WPARAM)extent, 0);
}

/* copiar_arreglo_bigint: Clona un objeto BigIntArray completamente (Deep Copy) util para revertir transacciones erroneas. */
static int copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)
{
    if (origen == NULL || destino == NULL)
        return 0;

    if (!bigint_array_create(destino, origen->len))
        return 0;

    for (size_t i = 0; i < origen->len; i++)
    {
        /* Valida que no haya falta de RAM en alguna asignacion de nodo string del BigInt interior. */
        if (!bigint_array_set(destino, i, &origen->items[i]))
        {
            bigint_array_free(destino);
            return 0;
        }
    }

    return 1;
}

/* refrescar_lista_stock: Toma el estado C puro actual y lo PINTA en la GUI Windows (ListBox). */
static void refrescar_lista_stock(void)
{
    char linea[512];
    int max_chars = 0;

    SendMessageA(g_listStock, LB_RESETCONTENT, 0, 0);
    SendMessageA(g_comboDenom, CB_RESETCONTENT, 0, 0);

    if (g_denom.items == NULL)
        return;

    for (size_t i = 0; i < g_denom.len; i++)
    {
        if (g_modo_stock_limitado && g_stock.items != NULL && i < g_stock.len)
            snprintf(linea, sizeof(linea), "Denom %s c -> stock %s", g_denom.items[i].digits, g_stock.items[i].digits);
        else
            snprintf(linea, sizeof(linea), "Denom %s c -> stock ilimitado", g_denom.items[i].digits);

        if ((int)strlen(linea) > max_chars)
            max_chars = (int)strlen(linea);

        SendMessageA(g_listStock, LB_ADDSTRING, 0, (LPARAM)linea);
        SendMessageA(g_comboDenom, CB_ADDSTRING, 0, (LPARAM)g_denom.items[i].digits);
    }

    ajustar_scroll_horizontal_lista(g_listStock, max_chars + 4);

    if (g_denom.len > 0)
        SendMessageA(g_comboDenom, CB_SETCURSEL, 0, 0);

    precargar_cantidades_lote();

    limpiar_resultado_cambio();
}

/* leer_monto_cambio: Trata de jalar la string cruda del TexBox monto principal a una variable BigInt (Modo Clasico). */
static int leer_monto_cambio(BigInt *monto)
{
    char buffer[256];
    int len = GetWindowTextA(g_editMonto, buffer, (int)sizeof(buffer));

    if (monto == NULL || g_editMonto == NULL)
        return 0;

    if (len <= 0)
        return 0;

    return bigint_init(monto, buffer);
}

/* calcular_y_mostrar_cambio: El CORE del GUI. Une la GUI visual con los motores logicos abstractos del BackEnd 'algoritmo_cambio.c'. */
static int calcular_y_mostrar_cambio(void)
{
    BigInt monto = {0};
    BigIntArray solucion = {0};
    BigIntArray stock_nuevo = {0};
    char linea[512];
    int hay_items = 0;
    int max_chars = 0;

    /* Obliga al usuario a tener algun pais (Ej. 'dolar') activo antes de presionar BOTON CALCULAR o crasheariamos. */
    if (g_monedaActiva < 0)
    {
        mostrar_error("Cambio", "Primero carga una moneda.");
        return 0;
    }

    int es_caja = (SendMessageA(g_checkCaja, BM_GETCHECK, 0, 0) == BST_CHECKED);
    size_t limite_monedas = 0;
    int tiene_limite = 0;
    char bufferLim[256];

    if (GetWindowTextA(g_editLimite, bufferLim, sizeof(bufferLim)) > 0)
    {
        long v = strtol(bufferLim, NULL, 10);
        if (v > 0)
        {
            limite_monedas = (size_t)v;
            tiene_limite = 1;
        }
    }

    if (es_caja)
    {
        char bufferPrecio[256], bufferPago[256];
        BigInt precio = {0}, pago = {0};
        GetWindowTextA(g_editPrecio, bufferPrecio, sizeof(bufferPrecio));
        GetWindowTextA(g_editPago, bufferPago, sizeof(bufferPago));
        if (!bigint_init(&precio, bufferPrecio) || !bigint_init(&pago, bufferPago))
        {
            mostrar_error("Cambio", "Precio o Pago invalido.");
            bigint_free(&precio);
            bigint_free(&pago);
            return 0;
        }
        if (bigint_compare(&pago, &precio) < 0)
        {
            mostrar_error("Cambio", "El pago es menor que el precio.");
            bigint_free(&precio);
            bigint_free(&pago);
            return 0;
        }
        bigint_subtract(&pago, &precio, &monto);
        bigint_free(&precio);
        bigint_free(&pago);
    }
    else
    {
        if (!leer_monto_cambio(&monto))
        {
            mostrar_error("Cambio", "Monto invalido. Usa un entero no negativo en centimos.");
            return 0;
        }
    }

    limpiar_resultado_cambio();
    snprintf(linea, sizeof(linea), "Cambio solicitado: %s c", monto.digits);
    max_chars = (int)strlen(linea);
    SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);

    int ok = 0;
    if (g_modo_stock_limitado)
    {
        if (tiene_limite)
            ok = calcular_cambio_optimo_stock_con_limite(&monto, &g_denom, &g_stock, limite_monedas, &solucion);
        else
            ok = calcular_cambio_optimo_stock(&monto, &g_denom, &g_stock, &solucion);
    }
    else
    {
        if (tiene_limite)
            ok = calcular_cambio_optimo_con_limite(&monto, &g_denom, limite_monedas, &solucion);
        else
            ok = calcular_cambio_optimo(&monto, &g_denom, &solucion);
    }

    if (!ok)
    {
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM) "No existe devolucion exacta con los parametros actuales.");
        ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);
        bigint_free(&monto);
        bigint_array_free(&solucion);
        return 0;
    }

    for (size_t i = 0; i < g_denom.len; i++)
    {
        if (bigint_is_zero(&solucion.items[i]))
            continue;

        snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, solucion.items[i].digits);
        if ((int)strlen(linea) > max_chars)
            max_chars = (int)strlen(linea);
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
        hay_items = 1;
    }

    /* Caso Edge matematico: Nos pidieron cambiar '0' dineros, y el DP retorno true que la formula es DevolverCero. */
    if (!hay_items)
    {
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM) "No se requieren monedas para devolver 0.");
        if ((int)strlen("No se requieren monedas para devolver 0.") > max_chars)
            max_chars = (int)strlen("No se requieren monedas para devolver 0.");
    }

    ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);

    /* Si es el modo irreal que no toca el archivo persistente .txt */
    if (!g_modo_stock_limitado)
    {
        SetWindowTextA(g_editMonto, "");
        bigint_free(&monto);
        bigint_array_free(&solucion);
        registrar_historial("Cambio aplicado (ilimitado).");
        return 1;
    }

    /* SI EL STOCK ES LIMITADO y si hay que ir a tocar Discos... Clona la DB como buffer para operaciones atómicas. */
    if (!copiar_arreglo_bigint(&g_stock, &stock_nuevo))
    {
        bigint_free(&monto);
        bigint_array_free(&solucion);
        mostrar_error("Cambio", "No se pudo preparar actualizacion de stock.");
        return 0;
    }

    for (size_t i = 0; i < stock_nuevo.len; i++)
    {
        BigInt nuevo_stock = {0};

        if (bigint_is_zero(&solucion.items[i]))
            continue;

        if (!bigint_subtract(&stock_nuevo.items[i], &solucion.items[i], &nuevo_stock))
        {
            bigint_array_free(&stock_nuevo);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            mostrar_error("Cambio", "No se pudo aplicar la devolucion al stock.");
            return 0;
        }

        if (!bigint_array_set(&stock_nuevo, i, &nuevo_stock))
        {
            bigint_free(&nuevo_stock);
            bigint_array_free(&stock_nuevo);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            mostrar_error("Cambio", "No se pudo actualizar stock en memoria.");
            return 0;
        }

        bigint_free(&nuevo_stock);
    }

    if (!actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_nuevo))
    {
        bigint_array_free(&stock_nuevo);
        bigint_free(&monto);
        bigint_array_free(&solucion);
        mostrar_error("Cambio", "No se pudo persistir la devolucion en stock.txt.");
        return 0;
    }

    bigint_array_free(&g_stock);
    g_stock = stock_nuevo;
    SetWindowTextA(g_editMonto, "");

    bigint_free(&monto);
    bigint_array_free(&solucion);
    registrar_historial("Cambio aplicado con stock.");
    return 1;
}

/* cargar_moneda_seleccionada: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int cargar_moneda_seleccionada(void)
{
    int idx = (int)SendMessageA(g_comboMoneda, CB_GETCURSEL, 0, 0);

    if (idx < 0 || idx >= g_monedasCount)
        return 0;

    liberar_datos_moneda();

    if (!cargar_denominaciones_moneda(g_monedas[idx], &g_denom))
        return 0;

    if (g_modo_stock_limitado)
    {

        if (!cargar_stock_moneda(g_monedas[idx], &g_stock))
        {
            bigint_array_free(&g_denom);
            return 0;
        }

        if (g_denom.len != g_stock.len)
        {
            liberar_datos_moneda();
            return 0;
        }
    }

    g_monedaActiva = idx;
    refrescar_lista_stock();
    return 1;
}

/* leer_cantidad_delta: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int leer_cantidad_delta(BigInt *delta)
{
    char buffer[256];
    int len = GetWindowTextA(g_editCantidad, buffer, (int)sizeof(buffer));

    if (len <= 0)
        return 0;

    return bigint_init(delta, buffer);
}

/* aplicar_cambio_stock: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int aplicar_cambio_stock(int esSuma)
{

    if (!g_modo_stock_limitado)
    {
        mostrar_error("Administrador", "El panel administrador solo aplica en modo stock limitado.");
        return 0;
    }

    int idxDenom;
    BigInt delta = {0};
    BigInt nuevo = {0};

    if (g_monedaActiva < 0)
    {
        mostrar_error("Administrador", "Primero carga una moneda.");
        return 0;
    }

    idxDenom = (int)SendMessageA(g_comboDenom, CB_GETCURSEL, 0, 0);

    if (idxDenom < 0 || (size_t)idxDenom >= g_stock.len)
    {
        mostrar_error("Administrador", "Selecciona una denominacion valida.");
        return 0;
    }

    if (!leer_cantidad_delta(&delta))
    {
        mostrar_error("Administrador", "Cantidad invalida. Usa un entero no negativo.");
        return 0;
    }

    if (esSuma)
    {

        if (!bigint_add(&g_stock.items[idxDenom], &delta, &nuevo))
        {
            bigint_free(&delta);
            mostrar_error("Administrador", "No se pudo sumar la cantidad.");
            return 0;
        }
    }
    else
    {

        if (bigint_compare(&g_stock.items[idxDenom], &delta) < 0)
        {
            bigint_free(&delta);
            mostrar_error("Administrador", "No puedes quitar mas stock del disponible.");
            return 0;
        }

        if (!bigint_subtract(&g_stock.items[idxDenom], &delta, &nuevo))
        {
            bigint_free(&delta);
            mostrar_error("Administrador", "No se pudo restar la cantidad.");
            return 0;
        }
    }

    if (!bigint_array_set(&g_stock, (size_t)idxDenom, &nuevo))
    {
        bigint_free(&delta);
        bigint_free(&nuevo);
        mostrar_error("Administrador", "No se pudo actualizar el stock en memoria.");
        return 0;
    }

    if (!actualizar_stock_moneda(g_monedas[g_monedaActiva], &g_stock))
    {
        bigint_free(&delta);
        bigint_free(&nuevo);
        mostrar_error("Administrador", "No se pudo persistir el stock en archivo.");
        return 0;
    }

    bigint_free(&delta);
    bigint_free(&nuevo);

    SetWindowTextA(g_editCantidad, "");
    refrescar_lista_stock();
    registrar_historialf("Admin %s (windows) | Moneda=%s | Denom=%s c | Cantidad=%s",
                         esSuma ? "ANADIR" : "QUITAR",
                         g_monedas[g_monedaActiva],
                         g_denom.items[idxDenom].digits,
                         delta.digits);
    mostrar_info("Administrador", esSuma ? "Stock actualizado (suma)." : "Stock actualizado (resta).");
    return 1;
}

/* aplicar_cambio_stock_lote: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int aplicar_cambio_stock_lote(int esSuma)
{
    BigIntArray deltas = {0};
    BigIntArray stock_nuevo = {0};

    if (!g_modo_stock_limitado)
    {
        mostrar_error("Administrador", "El panel administrador solo aplica en modo stock limitado.");
        return 0;
    }

    if (g_monedaActiva < 0 || g_stock.items == NULL || g_denom.len != g_stock.len)
    {
        mostrar_error("Administrador", "Primero carga una moneda valida.");
        return 0;
    }

    if (!leer_cantidades_lote(&deltas))
    {
        mostrar_error("Administrador", "Lote invalido. Introduce exactamente una cantidad por denominacion (linea por linea).");
        return 0;
    }

    if (!copiar_arreglo_bigint(&g_stock, &stock_nuevo))
    {
        bigint_array_free(&deltas);
        mostrar_error("Administrador", "No se pudo preparar actualizacion de stock.");
        return 0;
    }

    for (size_t i = 0; i < stock_nuevo.len; i++)
    {
        BigInt nuevo = {0};

        if (bigint_is_zero(&deltas.items[i]))
            continue;

        if (esSuma)
        {

            if (!bigint_add(&stock_nuevo.items[i], &deltas.items[i], &nuevo))
            {
                bigint_array_free(&deltas);
                bigint_array_free(&stock_nuevo);
                mostrar_error("Administrador", "No se pudo sumar una o mas cantidades del lote.");
                return 0;
            }
        }
        else
        {
            if (bigint_compare(&stock_nuevo.items[i], &deltas.items[i]) < 0 ||
                !bigint_subtract(&stock_nuevo.items[i], &deltas.items[i], &nuevo))
            {
                bigint_array_free(&deltas);
                bigint_array_free(&stock_nuevo);
                mostrar_error("Administrador", "No se pudo quitar lote: falta stock en alguna denominacion.");
                return 0;
            }
        }

        if (!bigint_array_set(&stock_nuevo, i, &nuevo))
        {
            bigint_free(&nuevo);
            bigint_array_free(&deltas);
            bigint_array_free(&stock_nuevo);
            mostrar_error("Administrador", "No se pudo actualizar stock en memoria.");
            return 0;
        }

        bigint_free(&nuevo);
    }

    if (!actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_nuevo))
    {
        bigint_array_free(&deltas);
        bigint_array_free(&stock_nuevo);
        mostrar_error("Administrador", "No se pudo persistir el lote en stock.txt.");
        return 0;
    }

    bigint_array_free(&g_stock);
    g_stock = stock_nuevo;
    bigint_array_free(&deltas);

    refrescar_lista_stock();
    registrar_historialf("Admin lote %s (windows) | Moneda=%s",
                         esSuma ? "ANADIR" : "QUITAR",
                         g_monedas[g_monedaActiva]);
    mostrar_info("Administrador", esSuma ? "Lote aplicado correctamente (suma)." : "Lote aplicado correctamente (resta).");
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

static int mostrar_resumen_moneda_gui(void)
{
    char mensaje[2048];
    size_t usado = 0;
    const BigInt *minDenom;
    const BigInt *maxDenom;
    size_t i;

    if (g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0)
    {
        mostrar_error("Resumen", "Primero carga una moneda valida.");
        return 0;
    }

    minDenom = &g_denom.items[0];
    maxDenom = &g_denom.items[0];
    for (i = 1; i < g_denom.len; i++)
    {
        if (bigint_compare(&g_denom.items[i], minDenom) < 0)
            minDenom = &g_denom.items[i];
        if (bigint_compare(&g_denom.items[i], maxDenom) > 0)
            maxDenom = &g_denom.items[i];
    }

    usado += (size_t)snprintf(mensaje + usado, sizeof(mensaje) - usado,
                              "Moneda: %s\r\nDenominaciones: %zu\r\nMin: %s c\r\nMax: %s c\r\n",
                              g_monedas[g_monedaActiva],
                              g_denom.len,
                              minDenom->digits,
                              maxDenom->digits);

    if (!g_modo_stock_limitado)
    {
        (void)snprintf(mensaje + usado, sizeof(mensaje) - usado,
                       "Modo stock ilimitado: no se calcula inventario fisico.");
        mostrar_info("Resumen", mensaje);
        registrar_historialf("Resumen consultado (windows) | Moneda=%s | Modo=Ilimitado",
                             g_monedas[g_monedaActiva]);
        return 1;
    }

    if (g_stock.items == NULL || g_stock.len != g_denom.len)
    {
        mostrar_error("Resumen", "No hay stock valido para calcular resumen.");
        return 0;
    }

    {
        BigInt totalPiezas = {0};
        BigInt totalValor = {0};
        int ok = 1;

        if (!bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0"))
            ok = 0;

        for (i = 0; ok && i < g_denom.len; i++)
        {
            BigInt nuevoTotalPiezas = {0};
            BigInt parcialValor = {0};
            BigInt nuevoTotalValor = {0};

            if (!bigint_add(&totalPiezas, &g_stock.items[i], &nuevoTotalPiezas) ||
                !bigint_multiply(&g_denom.items[i], &g_stock.items[i], &parcialValor) ||
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

        if (!ok)
        {
            bigint_free(&totalPiezas);
            bigint_free(&totalValor);
            mostrar_error("Resumen", "No se pudo calcular el resumen de inventario.");
            return 0;
        }

        (void)snprintf(mensaje + strlen(mensaje), sizeof(mensaje) - strlen(mensaje),
                       "Piezas en stock: %s\r\nValor total stock: %s c",
                       totalPiezas.digits,
                       totalValor.digits);

        registrar_historialf("Resumen consultado (windows) | Moneda=%s | Piezas=%s | Valor=%s c",
                             g_monedas[g_monedaActiva],
                             totalPiezas.digits,
                             totalValor.digits);

        bigint_free(&totalPiezas);
        bigint_free(&totalValor);
    }

    mostrar_info("Resumen", mensaje);
    return 1;
}

/* aplicar_cambio_especifico: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
static int aplicar_cambio_especifico(void)
{
    BigIntArray entradas = {0};
    BigIntArray devolucion = {0};
    BigIntArray stock_nuevo = {0};
    BigInt totalEntrada = {0};
    BigInt totalDevolucion = {0};
    char linea[512];
    int max_chars = 0;

    if (g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0)
    {
        mostrar_error("Cambio especifico", "Primero carga una moneda valida.");
        return 0;
    }

    if (!leer_cantidades_desde_edit(g_editEntradaCambio, &entradas) ||
        !leer_cantidades_desde_edit(g_editDevolucionCambio, &devolucion))
    {
        mostrar_error("Cambio especifico", "Entrada invalida. Usa una cantidad por linea y por denominacion en ambos cuadros.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        return 0;
    }

    if (!calcular_total_valor(&g_denom, &entradas, &totalEntrada) ||
        !calcular_total_valor(&g_denom, &devolucion, &totalDevolucion))
    {
        mostrar_error("Cambio especifico", "No se pudieron calcular los totales.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    if (bigint_compare(&totalEntrada, &totalDevolucion) != 0)
    {
        mostrar_error("Cambio especifico", "El total entregado debe ser igual al total solicitado como devolucion.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    if (!g_modo_stock_limitado)
    {
        limpiar_resultado_cambio();
        snprintf(linea, sizeof(linea), "Cambio especifico aplicado (ilimitado): %s c", totalEntrada.digits);
        max_chars = (int)strlen(linea);
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
        for (size_t i = 0; i < g_denom.len; i++)
        {
            if (bigint_is_zero(&devolucion.items[i]))
                continue;

            snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, devolucion.items[i].digits);
            if ((int)strlen(linea) > max_chars)
                max_chars = (int)strlen(linea);
            SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
        }
        ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);

        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        mostrar_info("Cambio especifico", "Operacion aplicada en modo ilimitado.");
        registrar_historial("Cambio especifico aplicado (ilimitado).");
        return 1;
    }

    if (g_stock.items == NULL || g_denom.len != g_stock.len)
    {
        mostrar_error("Cambio especifico", "No hay stock cargado para modo limitado.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    if (!copiar_arreglo_bigint(&g_stock, &stock_nuevo))
    {
        mostrar_error("Cambio especifico", "No se pudo preparar la actualizacion de stock.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    for (size_t i = 0; i < stock_nuevo.len; i++)
    {
        BigInt trasEntrada = {0};
        BigInt trasSalida = {0};

        if (!bigint_add(&stock_nuevo.items[i], &entradas.items[i], &trasEntrada) ||
            bigint_compare(&trasEntrada, &devolucion.items[i]) < 0 ||
            !bigint_subtract(&trasEntrada, &devolucion.items[i], &trasSalida) ||
            !bigint_array_set(&stock_nuevo, i, &trasSalida))
        {
            bigint_free(&trasEntrada);
            bigint_free(&trasSalida);
            bigint_array_free(&entradas);
            bigint_array_free(&devolucion);
            bigint_array_free(&stock_nuevo);
            bigint_free(&totalEntrada);
            bigint_free(&totalDevolucion);
            mostrar_error("Cambio especifico", "No se puede aplicar la devolucion especifica con el stock disponible.");
            return 0;
        }

        bigint_free(&trasEntrada);
        bigint_free(&trasSalida);
    }

    if (!actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_nuevo))
    {
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_array_free(&stock_nuevo);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        mostrar_error("Cambio especifico", "No se pudo persistir el cambio especifico en stock.txt.");
        return 0;
    }

    bigint_array_free(&g_stock);
    g_stock = stock_nuevo;

    refrescar_lista_stock();

    limpiar_resultado_cambio();
    snprintf(linea, sizeof(linea), "Cambio especifico aplicado: %s c", totalEntrada.digits);
    max_chars = (int)strlen(linea);
    SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
    for (size_t i = 0; i < g_denom.len; i++)
    {
        if (bigint_is_zero(&devolucion.items[i]))
            continue;

        snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, devolucion.items[i].digits);
        if ((int)strlen(linea) > max_chars)
            max_chars = (int)strlen(linea);
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
    }
    ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);

    bigint_array_free(&entradas);
    bigint_array_free(&devolucion);
    bigint_free(&totalEntrada);
    bigint_free(&totalDevolucion);
    mostrar_info("Cambio especifico", "Operacion aplicada y stock actualizado.");
    registrar_historial("Cambio especifico aplicado con stock.");
    return 1;
}

/* crear_controles: Gestiona la inicializacion y el ciclo de mensajes de la interfaz de usuario. */
static void crear_controles(HWND hwnd)
{
    CreateWindowA("STATIC", "Panel Principal", WS_CHILD | WS_VISIBLE | SS_LEFT,
                  20, 20, 220, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "Moneda:", WS_CHILD | WS_VISIBLE,
                  20, 50, 60, 20, hwnd, NULL, NULL, NULL);

    g_comboMoneda = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                  80, 48, 180, 400, hwnd, (HMENU)ID_COMBO_MONEDA, NULL, NULL);

    g_btnCargar = CreateWindowA("BUTTON", "Cargar", WS_CHILD | WS_VISIBLE,
                                270, 47, 90, 24, hwnd, (HMENU)ID_BTN_CARGAR, NULL, NULL);

    g_btnRecargar = CreateWindowA("BUTTON", "Recargar", WS_CHILD | WS_VISIBLE,
                                  370, 47, 90, 24, hwnd, (HMENU)ID_BTN_RECARGAR, NULL, NULL);

    g_radioLimitado = CreateWindowA("BUTTON", "Stock limitado", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                    20, 76, 130, 20, hwnd, (HMENU)ID_RADIO_LIMITADO, NULL, NULL);

    g_radioIlimitado = CreateWindowA("BUTTON", "Stock ilimitado", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                     160, 76, 130, 20, hwnd, (HMENU)ID_RADIO_ILIMITADO, NULL, NULL);

    configurar_modo_stock(1);

    g_listStock = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | LBS_NOTIFY,
                                20, 102, 440, 140, hwnd, (HMENU)ID_LIST_STOCK, NULL, NULL);

    CreateWindowA("STATIC", "Panel Administrador", WS_CHILD | WS_VISIBLE | SS_LEFT,
                  20, 250, 220, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "Denominacion:", WS_CHILD | WS_VISIBLE,
                  20, 285, 90, 20, hwnd, NULL, NULL, NULL);

    g_comboDenom = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                 115, 282, 150, 400, hwnd, (HMENU)ID_COMBO_DENOM, NULL, NULL);

    CreateWindowA("STATIC", "Cantidad:", WS_CHILD | WS_VISIBLE,
                  280, 285, 60, 20, hwnd, NULL, NULL, NULL);

    g_editCantidad = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   345, 282, 115, 24, hwnd, (HMENU)ID_EDIT_CANTIDAD, NULL, NULL);

    g_btnAgregar = CreateWindowA("BUTTON", "Anadir", WS_CHILD | WS_VISIBLE,
                                 190, 320, 120, 30, hwnd, (HMENU)ID_BTN_AGREGAR, NULL, NULL);

    g_btnQuitar = CreateWindowA("BUTTON", "Quitar", WS_CHILD | WS_VISIBLE,
                                320, 320, 120, 30, hwnd, (HMENU)ID_BTN_QUITAR, NULL, NULL);

    CreateWindowA("STATIC", "Lote (una cantidad por linea en orden de denominaciones):", WS_CHILD | WS_VISIBLE,
                  20, 360, 330, 20, hwnd, NULL, NULL, NULL);

    g_editLote = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
                               20, 382, 290, 76, hwnd, (HMENU)ID_EDIT_LOTE, NULL, NULL);

    g_btnAgregarLote = CreateWindowA("BUTTON", "Anadir lote", WS_CHILD | WS_VISIBLE,
                                     320, 392, 120, 28, hwnd, (HMENU)ID_BTN_AGREGAR_LOTE, NULL, NULL);

    g_btnQuitarLote = CreateWindowA("BUTTON", "Quitar lote", WS_CHILD | WS_VISIBLE,
                                    320, 428, 120, 28, hwnd, (HMENU)ID_BTN_QUITAR_LOTE, NULL, NULL);

    CreateWindowA("STATIC", "Cambio especifico (una cantidad por linea, en orden de denominaciones):", WS_CHILD | WS_VISIBLE,
                  20, 468, 430, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "Entregado", WS_CHILD | WS_VISIBLE,
                  20, 492, 90, 20, hwnd, NULL, NULL, NULL);

    g_editEntradaCambio = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
                                        20, 514, 190, 72, hwnd, (HMENU)ID_EDIT_ENTRADA_CAMBIO, NULL, NULL);

    CreateWindowA("STATIC", "Devolucion", WS_CHILD | WS_VISIBLE,
                  220, 492, 90, 20, hwnd, NULL, NULL, NULL);

    g_editDevolucionCambio = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
                                           220, 514, 190, 72, hwnd, (HMENU)ID_EDIT_DEVOLUCION_CAMBIO, NULL, NULL);

    g_btnCambioEspecifico = CreateWindowA("BUTTON", "Aplicar cambio especifico", WS_CHILD | WS_VISIBLE,
                                          420, 538, 170, 30, hwnd, (HMENU)ID_BTN_CAMBIO_ESPECIFICO, NULL, NULL);

    CreateWindowA("STATIC", "Panel Devolucion", WS_CHILD | WS_VISIBLE | SS_LEFT,
                  20, 580, 220, 20, hwnd, NULL, NULL, NULL);

    g_checkCaja = CreateWindowA("BUTTON", "Caja Reg.", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                20, 600, 100, 20, hwnd, (HMENU)ID_CHECK_CAJA, NULL, NULL);

    CreateWindowA("STATIC", "Precio:", WS_CHILD | WS_VISIBLE,
                  125, 602, 50, 20, hwnd, NULL, NULL, NULL);
    g_editPrecio = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_DISABLED,
                                 180, 600, 80, 24, hwnd, (HMENU)ID_EDIT_PRECIO, NULL, NULL);

    CreateWindowA("STATIC", "Pago:", WS_CHILD | WS_VISIBLE,
                  270, 602, 40, 20, hwnd, NULL, NULL, NULL);
    g_editPago = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_DISABLED,
                               315, 600, 80, 24, hwnd, (HMENU)ID_EDIT_PAGO, NULL, NULL);

    CreateWindowA("STATIC", "Monto:", WS_CHILD | WS_VISIBLE,
                  20, 632, 50, 20, hwnd, NULL, NULL, NULL);
    g_editMonto = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                70, 630, 80, 24, hwnd, (HMENU)ID_EDIT_MONTO, NULL, NULL);

    CreateWindowA("STATIC", "Limite:", WS_CHILD | WS_VISIBLE,
                  160, 632, 50, 20, hwnd, NULL, NULL, NULL);
    g_editLimite = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                 215, 630, 60, 24, hwnd, (HMENU)ID_EDIT_LIMITE, NULL, NULL);

    g_btnCalcular = CreateWindowA("BUTTON", "Calcular", WS_CHILD | WS_VISIBLE,
                                  285, 628, 80, 28, hwnd, (HMENU)ID_BTN_CALCULAR, NULL, NULL);

    g_btnHistorial = CreateWindowA("BUTTON", "Historial", WS_CHILD | WS_VISIBLE,
                                   375, 628, 80, 28, hwnd, (HMENU)ID_BTN_HISTORIAL, NULL, NULL);

    g_btnResumen = CreateWindowA("BUTTON", "Resumen", WS_CHILD | WS_VISIBLE,
                                 465, 628, 80, 28, hwnd, (HMENU)ID_BTN_RESUMEN, NULL, NULL);

    g_listResultado = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | LBS_NOTIFY,
                                    20, 665, 570, 100, hwnd, (HMENU)ID_LIST_RESULTADO, NULL, NULL);
}

/* wnd_proc: Gestiona la inicializacion y el ciclo de mensajes de la interfaz de usuario. */
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
    case WM_CREATE:
        crear_controles(hwnd);

        if (!cargar_nombres_moneda())
        {
            mostrar_error("Inicio", "No se pudieron leer monedas desde monedas.txt");
        }
        refrescar_combo_monedas();
        break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        if (id == ID_BTN_CARGAR)
        {
            if (!cargar_moneda_seleccionada())
                mostrar_error("Carga", "No se pudo cargar moneda/stock.");
            return 0;
        }

        if (id == ID_BTN_RECARGAR)
        {
            int idx_prev = (int)SendMessageA(g_comboMoneda, CB_GETCURSEL, 0, 0);

            if (!cargar_nombres_moneda())
            {
                mostrar_error("Recarga", "No se pudieron recargar monedas.");
                return 0;
            }

            liberar_datos_moneda();
            refrescar_combo_monedas();

            if (idx_prev >= 0 && idx_prev < g_monedasCount)
                SendMessageA(g_comboMoneda, CB_SETCURSEL, (WPARAM)idx_prev, 0);

            if (!cargar_moneda_seleccionada())
            {
                mostrar_error("Recarga", "No se pudo recargar moneda/stock.");
                refrescar_lista_stock();
            }
            return 0;
        }

        if (id == ID_BTN_AGREGAR)
        {
            aplicar_cambio_stock(1);
            return 0;
        }

        if (id == ID_BTN_QUITAR)
        {
            aplicar_cambio_stock(0);
            return 0;
        }

        if (id == ID_BTN_AGREGAR_LOTE)
        {
            aplicar_cambio_stock_lote(1);
            return 0;
        }

        if (id == ID_BTN_QUITAR_LOTE)
        {
            aplicar_cambio_stock_lote(0);
            return 0;
        }

        if (id == ID_BTN_HISTORIAL)
        {
            system("notepad historial.txt");
            return 0;
        }

        if (id == ID_BTN_RESUMEN)
        {
            mostrar_resumen_moneda_gui();
            return 0;
        }

        if (id == ID_CHECK_CAJA)
        {
            if (SendMessageA(g_checkCaja, BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
                EnableWindow(g_editPrecio, TRUE);
                EnableWindow(g_editPago, TRUE);
                EnableWindow(g_editMonto, FALSE);
            }
            else
            {
                EnableWindow(g_editPrecio, FALSE);
                EnableWindow(g_editPago, FALSE);
                EnableWindow(g_editMonto, TRUE);
            }
            return 0;
        }

        if (id == ID_BTN_CALCULAR)
        {
            calcular_y_mostrar_cambio();
            return 0;
        }

        if (id == ID_BTN_CAMBIO_ESPECIFICO)
        {
            aplicar_cambio_especifico();
            return 0;
        }

        if (id == ID_RADIO_LIMITADO || id == ID_RADIO_ILIMITADO)
        {
            int nuevo_limitado = (id == ID_RADIO_LIMITADO) ? 1 : 0;

            if (nuevo_limitado != g_modo_stock_limitado)
            {
                int idx = (int)SendMessageA(g_comboMoneda, CB_GETCURSEL, 0, 0);
                configurar_modo_stock(nuevo_limitado);
                limpiar_resultado_cambio();
                if (idx >= 0 && idx < g_monedasCount)
                    cargar_moneda_seleccionada();
                else
                    refrescar_lista_stock();
            }
            return 0;
        }
        break;
    }

    case WM_DESTROY:
        liberar_datos_moneda();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* WinMain: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};
    HWND hwnd;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;

    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ProgVorazGUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (!RegisterClassA(&wc))
        return 1;

    hwnd = CreateWindowA("ProgVorazGUI", "ProgVoraz - Interfaz Grafica",
                         WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                         CW_USEDEFAULT, CW_USEDEFAULT, 640, 840,
                         NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
#else
/* main: Punto de entrada principal. Configura el entorno y lanza la ejecucion. */
int main(void)
{
    return 0;
}
#endif

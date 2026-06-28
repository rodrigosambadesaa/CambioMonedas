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
#include "exchange_api.h"
#include "json_io.h"
#include "progvoraz_locale.h"

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
#define ID_BTN_SNAPSHOT 1026
#define ID_BTN_RESTAURAR 1027
#define ID_BTN_REPORTE 1028
#define ID_BTN_EXPORT_JSON 1029
#define ID_EDIT_CONVERT_ORIGEN 1030
#define ID_EDIT_CONVERT_MONTO 1031
#define ID_CHECK_CONVERT_STOCK 1032
#define ID_BTN_CONVERTIR 1033

#define TR(es, en) progvoraz_tr((es), (en))

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
static HWND g_btnSnapshot;
static HWND g_btnRestaurar;
static HWND g_btnReporte;
static HWND g_btnExportJson;
static HWND g_editConvertOrigen;
static HWND g_editConvertMonto;
static HWND g_checkConvertStock;
static HWND g_btnConvertir;

static char g_monedas[MAX_MONEDAS][MAX_NOMBRE];
static int g_monedasCount = 0;
static int g_monedaActiva = -1;
static int g_modo_stock_limitado = 1;

static BigIntArray g_denom = {0};
static BigIntArray g_stock = {0};

static int cargar_moneda_seleccionada(void);
static int ejecutar_conversion_gui(void);

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

/* es_numero: Revisa si una cadena entrante es estrictamente numerica (ej: '500'). */
/* funcion es_numero: contiene la logica principal de esta operacion. */
static int es_numero(const char *s)
{
    size_t i;

    /* if: comprueba s == NULL || s[0] == '\0' antes de ejecutar esta rama. */
    if (s == NULL || s[0] == '\0')
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

/* liberar_datos_moneda: Macro local para purgar cache de DB en UI y reiniciar a neutro. */
/* funcion liberar_datos_moneda: contiene la logica principal de esta operacion. */
static void liberar_datos_moneda(void)
{
    bigint_array_free(&g_denom);
    bigint_array_free(&g_stock);
    g_monedaActiva = -1;
}

/* cargar_nombres_moneda: Parser simplificado que lee "monedas.txt" buscando los PAISES/NOMBRES y no sus denominaciones en si. */
/* funcion cargar_nombres_moneda: contiene la logica principal de esta operacion. */
static int cargar_nombres_moneda(void)
{
    FILE *fp = fopen("monedas.txt", "r");
    char token[256];

    g_monedasCount = 0;
    /* if: comprueba fp == NULL antes de ejecutar esta rama. */
    if (fp == NULL)
        return 0;

    /* Extrae iterativamente palabras (ignorando blancos y tabs autom.) */
    /* while: repite el bloque mientras se cumpla fscanf(fp, "%255s", token) == 1. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        /* Si NO es numero, lo asumimos como el titular o nombre identificador de la divisa (ej "euro"). */
        /* if: comprueba !es_numero(token) antes de ejecutar esta rama. */
        if (!es_numero(token))
        {
            int yaExiste = 0;
            /* for: itera segun int i = 0; i < g_monedasCount; i++ para recorrer el bloque. */
            for (int i = 0; i < g_monedasCount; i++)
            {
                /* Previene leer divisas duplicadas que alguien escribio mal en el TXT. */
                /* if: comprueba strcmp(g_monedas[i], token) == 0 antes de ejecutar esta rama. */
                if (strcmp(g_monedas[i], token) == 0)
                {
                    yaExiste = 1;
                    break;
                }
            }

            /* Si superó la prueba de duplicidad y aún caben en el selector Combo de Windows... */
            /* if: comprueba !yaExiste && g_monedasCount < MAX_MONEDAS antes de ejecutar esta rama. */
            if (!yaExiste && g_monedasCount < MAX_MONEDAS)
            {
                size_t n = strlen(token);
                /* if: comprueba n >= MAX_NOMBRE antes de ejecutar esta rama. */
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
/* funcion refrescar_combo_monedas: contiene la logica principal de esta operacion. */
static void refrescar_combo_monedas(void)
{
    SendMessageA(g_comboMoneda, CB_RESETCONTENT, 0, 0);

    /* for: itera segun int i = 0; i < g_monedasCount; i++ para recorrer el bloque. */
    for (int i = 0; i < g_monedasCount; i++)
        SendMessageA(g_comboMoneda, CB_ADDSTRING, 0, (LPARAM)g_monedas[i]);

    /* if: comprueba g_monedasCount > 0 antes de ejecutar esta rama. */
    if (g_monedasCount > 0)
        SendMessageA(g_comboMoneda, CB_SETCURSEL, 0, 0);
}

/* mostrar_error: Dispara un Error MsgBox nativo de OS. */
/* funcion mostrar_error: contiene la logica principal de esta operacion. */
static void mostrar_error(const char *titulo, const char *mensaje)
{
    MessageBoxA(NULL, mensaje, titulo, MB_ICONERROR | MB_OK);
}

/* mostrar_info: Dispara una Dialog Info MsgBox nativa. */
/* funcion mostrar_info: contiene la logica principal de esta operacion. */
static void mostrar_info(const char *titulo, const char *mensaje)
{
    MessageBoxA(NULL, mensaje, titulo, MB_ICONINFORMATION | MB_OK);
}

/* funcion ejecutar_operacion_global_gui: contiene la logica principal de esta operacion. */
static void ejecutar_operacion_global_gui(const char *accion)
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
            registrar_historial("Snapshot de stock creado (windows)");
            mostrar_info(TR("Snapshot", "Snapshot"), TR("Snapshot creado: stock_snapshot.txt", "Snapshot created: stock_snapshot.txt"));
        }
        else
        {
            mostrar_error(TR("Snapshot", "Snapshot"), TR("No se pudo crear snapshot de stock.", "Could not create the stock snapshot."));
        }
        return;
    }

    /* if: comprueba strcmp(accion, "restaurar") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "restaurar") == 0)
    {
        /* if: comprueba restaurar_snapshot_stock("stock_snapshot.txt") antes de ejecutar esta rama. */
        if (restaurar_snapshot_stock("stock_snapshot.txt"))
        {
            registrar_historial("Stock restaurado desde snapshot (windows)");
            /* if: comprueba g_monedaActiva >= 0 antes de ejecutar esta rama. */
            if (g_monedaActiva >= 0)
                cargar_moneda_seleccionada();
            mostrar_info(TR("Restaurar", "Restore"), TR("Stock restaurado desde stock_snapshot.txt", "Stock restored from stock_snapshot.txt"));
        }
        else
        {
            mostrar_error(TR("Restaurar", "Restore"), TR("No se pudo restaurar stock desde snapshot.", "Could not restore stock from snapshot."));
        }
        return;
    }

    /* if: comprueba strcmp(accion, "reporte") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "reporte") == 0)
    {
        /* if: comprueba exportar_reporte_global("reporte_global.txt") antes de ejecutar esta rama. */
        if (exportar_reporte_global("reporte_global.txt"))
        {
            registrar_historial("Reporte global exportado (windows)");
            mostrar_info(TR("Reporte", "Report"), TR("Reporte generado: reporte_global.txt", "Report generated: reporte_global.txt"));
        }
        else
        {
            mostrar_error(TR("Reporte", "Report"), TR("No se pudo generar reporte global.", "Could not generate the global report."));
        }
        return;
    }

    /* if: comprueba strcmp(accion, "json") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "json") == 0)
    {
        /* if: comprueba export_stock_json("stock_out.json") antes de ejecutar esta rama. */
        if (export_stock_json("stock_out.json"))
        {
            registrar_historial("Stock exportado a JSON (windows)");
            mostrar_info("JSON", TR("Stock exportado: stock_out.json", "Stock exported: stock_out.json"));
        }
        else
        {
            mostrar_error("JSON", TR("No se pudo exportar stock_out.json.", "Could not export stock_out.json."));
        }
    }
}

/* limpiar_resultado_cambio: Purga el TextBox o ListBox donde se muestran los billetes calculados por el backtracking. */
/* funcion limpiar_resultado_cambio: contiene la logica principal de esta operacion. */
static void limpiar_resultado_cambio(void)
{
    SendMessageA(g_listResultado, LB_RESETCONTENT, 0, 0);
    SendMessageA(g_listResultado, LB_SETHORIZONTALEXTENT, 0, 0);
}

/* configurar_modo_stock: Enablea/Disablea secciones enteras de la App dependiendo de si clickeamos "Limitado" o "Ilimitado". */
/* funcion configurar_modo_stock: contiene la logica principal de esta operacion. */
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
    /* if: comprueba SendMessageA(g_checkCaja, BM_GETCHECK, 0, 0) == BST_CHECKED antes de ejecutar esta rama. */
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
/* funcion precargar_ceros_en_edit: contiene la logica principal de esta operacion. */
static void precargar_ceros_en_edit(HWND edit)
{
    char texto[4096];
    size_t pos = 0;

    texto[0] = '\0';
    /* if: comprueba edit == NULL antes de ejecutar esta rama. */
    if (edit == NULL)
        return;

    /* Valida y si la divisa no esta cargada... */
    /* if: comprueba g_denom.items == NULL || g_denom.len == 0 antes de ejecutar esta rama. */
    if (g_denom.items == NULL || g_denom.len == 0)
    {
        SetWindowTextA(edit, "");
        return;
    }

    /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < g_denom.len; i++)
    {
        int escritos;
        size_t restante = sizeof(texto) - pos;

        /* if: comprueba restante <= 1 antes de ejecutar esta rama. */
        if (restante <= 1)
            break;

        escritos = snprintf(texto + pos, restante, "%s%s", (i == 0) ? "" : "\r\n", "0");
        /* if: comprueba escritos < 0 antes de ejecutar esta rama. */
        if (escritos < 0)
            break;

        /* Detecta BufferTruncation. */
        /* if: comprueba (size_t)escritos >= restante antes de ejecutar esta rama. */
        if ((size_t)escritos >= restante)
            /* Detecta BufferTruncation. */
            /* if: comprueba (size_t)escritos >= restante antes de ejecutar esta rama. */
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
/* funcion precargar_cantidades_lote: contiene la logica principal de esta operacion. */
static void precargar_cantidades_lote(void)
{
    precargar_ceros_en_edit(g_editLote);
    precargar_ceros_en_edit(g_editEntradaCambio);
    precargar_ceros_en_edit(g_editDevolucionCambio);
}

/* leer_cantidades_desde_edit: Convierte un campo de texto multilinea ("0\n150\n30\n...") en un Array de BigInts manejable. */
/* funcion leer_cantidades_desde_edit: contiene la logica principal de esta operacion. */
static int leer_cantidades_desde_edit(HWND edit, BigIntArray *deltas)
{
    int len;
    char *buffer;
    char *ctx;
    char *token;
    size_t idx = 0;

    /* Valida punteros y estado de base de datos antes de siquiera asomarse a leer UI. */
    /* if: comprueba edit == NULL || deltas == NULL || g_denom.items == NULL || g_denom.le... antes de ejecutar esta rama. */
    if (edit == NULL || deltas == NULL || g_denom.items == NULL || g_denom.len == 0)
        return 0;

    len = GetWindowTextLengthA(edit);
    /* if: comprueba len <= 0 antes de ejecutar esta rama. */
    if (len <= 0)
        return 0;

    buffer = (char *)malloc((size_t)len + 1);
    /* if: comprueba buffer == NULL antes de ejecutar esta rama. */
    if (buffer == NULL)
        return 0;

    GetWindowTextA(edit, buffer, len + 1);

    /* Construye el vector de precision arbitraria en memoria usando la dimension de Denominaciones conocidas. */
    /* if: comprueba !bigint_array_create(deltas, g_denom.len) antes de ejecutar esta rama. */
    if (!bigint_array_create(deltas, g_denom.len))
    {
        free(buffer);
        return 0;
    }

    token = strtok_s(buffer, " \t\r\n", &ctx);
    /* Itera cada numero hallado en el Multiline. */
    /* while: repite el bloque mientras se cumpla token != NULL. */
    while (token != NULL)
    {
        BigInt delta = {0};

        /* Validador de desbordamiento de Array (ej. el usuario metio MAS lineas de texto que las permitidas por la Divisa). */
        /* if: comprueba idx >= g_denom.len antes de ejecutar esta rama. */
        if (idx >= g_denom.len)
        {
            bigint_array_free(deltas);
            free(buffer);
            return 0;
        }

        /* Si el token hallado es basura ("hola", "NAN", letras...) `bigint_init` fallara y atrapara el error. */
        /* if: comprueba !bigint_init(&delta, token) antes de ejecutar esta rama. */
        if (!bigint_init(&delta, token))
        {
            bigint_array_free(deltas);
            free(buffer);
            return 0;
        }

        /* Inserta el objeto numerico leido en la posicion correspondiente del Array Resultante. */
        /* if: comprueba !bigint_array_set(deltas, idx, &delta) antes de ejecutar esta rama. */
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
    /* if: comprueba idx != g_denom.len antes de ejecutar esta rama. */
    if (idx != g_denom.len)
    {
        bigint_array_free(deltas);
        return 0;
    }

    return 1;
}

/* leer_cantidades_lote: Wrapper simplificado que en ruta predeterminada asume el input box "g_editLote" (Administrador). */
/* funcion leer_cantidades_lote: contiene la logica principal de esta operacion. */
static int leer_cantidades_lote(BigIntArray *deltas)
{
    return leer_cantidades_desde_edit(g_editLote, deltas);
}

/* ajustar_scroll_horizontal_lista: Funcion para evitar que strings largos en la GUI queden cortados y ocultos. */
/* funcion ajustar_scroll_horizontal_lista: contiene la logica principal de esta operacion. */
static void ajustar_scroll_horizontal_lista(HWND lista, int max_chars)
{
    int extent = max_chars * 8;
    /* if: comprueba lista == NULL antes de ejecutar esta rama. */
    if (lista == NULL)
        return;
    /* if: comprueba extent < 0 antes de ejecutar esta rama. */
    if (extent < 0)
        extent = 0;
    SendMessageA(lista, LB_SETHORIZONTALEXTENT, (WPARAM)extent, 0);
}

/* copiar_arreglo_bigint: Clona un objeto BigIntArray completamente (Deep Copy) util para revertir transacciones erroneas. */
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
        /* Valida que no haya falta de RAM en alguna asignacion de nodo string del BigInt interior. */
        /* if: comprueba !bigint_array_set(destino, i, &origen->items[i]) antes de ejecutar esta rama. */
        if (!bigint_array_set(destino, i, &origen->items[i]))
        {
            bigint_array_free(destino);
            return 0;
        }
    }

    return 1;
}

/* refrescar_lista_stock: Toma el estado C puro actual y lo PINTA en la GUI Windows (ListBox). */
/* funcion refrescar_lista_stock: contiene la logica principal de esta operacion. */
static void refrescar_lista_stock(void)
{
    char linea[512];
    int max_chars = 0;

    SendMessageA(g_listStock, LB_RESETCONTENT, 0, 0);
    SendMessageA(g_comboDenom, CB_RESETCONTENT, 0, 0);

    /* if: comprueba g_denom.items == NULL antes de ejecutar esta rama. */
    if (g_denom.items == NULL)
        return;

    /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < g_denom.len; i++)
    {
        /* if: comprueba g_modo_stock_limitado && g_stock.items != NULL && i < g_stock.len antes de ejecutar esta rama. */
        if (g_modo_stock_limitado && g_stock.items != NULL && i < g_stock.len)
            snprintf(linea, sizeof(linea), "Denom %s c -> stock %s", g_denom.items[i].digits, g_stock.items[i].digits);
        else
            snprintf(linea, sizeof(linea), "Denom %s c -> stock ilimitado", g_denom.items[i].digits);

        /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
        if ((int)strlen(linea) > max_chars)
            max_chars = (int)strlen(linea);

        SendMessageA(g_listStock, LB_ADDSTRING, 0, (LPARAM)linea);
        SendMessageA(g_comboDenom, CB_ADDSTRING, 0, (LPARAM)g_denom.items[i].digits);
    }

    ajustar_scroll_horizontal_lista(g_listStock, max_chars + 4);

    /* if: comprueba g_denom.len > 0 antes de ejecutar esta rama. */
    if (g_denom.len > 0)
        SendMessageA(g_comboDenom, CB_SETCURSEL, 0, 0);

    precargar_cantidades_lote();

    limpiar_resultado_cambio();
}

/* leer_monto_cambio: Trata de jalar la string cruda del TexBox monto principal a una variable BigInt (Modo Clasico). */
/* funcion leer_monto_cambio: contiene la logica principal de esta operacion. */
static int leer_monto_cambio(BigInt *monto)
{
    char buffer[256];
    int len = GetWindowTextA(g_editMonto, buffer, (int)sizeof(buffer));

    /* if: comprueba monto == NULL || g_editMonto == NULL antes de ejecutar esta rama. */
    if (monto == NULL || g_editMonto == NULL)
        return 0;

    /* if: comprueba len <= 0 antes de ejecutar esta rama. */
    if (len <= 0)
        return 0;

    return bigint_init(monto, buffer);
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

/* calcular_y_mostrar_cambio: El CORE del GUI. Une la GUI visual con los motores logicos abstractos del BackEnd 'algoritmo_cambio.c'. */
/* funcion calcular_y_mostrar_cambio: contiene la logica principal de esta operacion. */
static int calcular_y_mostrar_cambio(void)
{
    BigInt monto = {0};
    BigIntArray solucion = {0};
    BigIntArray stock_nuevo = {0};
    char linea[512];
    int hay_items = 0;
    int max_chars = 0;

    /* Obliga al usuario a tener algun pais (Ej. 'dolar') activo antes de presionar BOTON CALCULAR o crasheariamos. */
    /* if: comprueba g_monedaActiva < 0 antes de ejecutar esta rama. */
    if (g_monedaActiva < 0)
    {
        mostrar_error("Cambio", "Primero carga una moneda.");
        return 0;
    }

    int es_caja = (SendMessageA(g_checkCaja, BM_GETCHECK, 0, 0) == BST_CHECKED);
    size_t min_monedas = 0;
    size_t max_monedas = 0;
    int tiene_limite = 0;
    char bufferLim[256];

    /* if: comprueba GetWindowTextA(g_editLimite, bufferLim, sizeof(bufferLim)) > 0 antes de ejecutar esta rama. */
    if (GetWindowTextA(g_editLimite, bufferLim, sizeof(bufferLim)) > 0)
    {
        /* if: comprueba parse_restriccion_gui(bufferLim, &min_monedas, &max_monedas) antes de ejecutar esta rama. */
        if (parse_restriccion_gui(bufferLim, &min_monedas, &max_monedas))
        {
            tiene_limite = 1;
        }
        else
        {
            mostrar_error("Cambio", "Restriccion invalida. Usa N, =N o N-M.");
            return 0;
        }
    }

    /* if: comprueba es_caja antes de ejecutar esta rama. */
    if (es_caja)
    {
        char bufferPrecio[256], bufferPago[256];
        BigInt precio = {0}, pago = {0};
        GetWindowTextA(g_editPrecio, bufferPrecio, sizeof(bufferPrecio));
        GetWindowTextA(g_editPago, bufferPago, sizeof(bufferPago));
        /* if: comprueba !bigint_init(&precio, bufferPrecio) || !bigint_init(&pago, bufferPago) antes de ejecutar esta rama. */
        if (!bigint_init(&precio, bufferPrecio) || !bigint_init(&pago, bufferPago))
        {
            mostrar_error("Cambio", "Precio o Pago invalido.");
            bigint_free(&precio);
            bigint_free(&pago);
            return 0;
        }
        /* if: comprueba bigint_compare(&pago, &precio) < 0 antes de ejecutar esta rama. */
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
        /* if: comprueba !leer_monto_cambio(&monto) antes de ejecutar esta rama. */
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
    /* if: comprueba g_modo_stock_limitado antes de ejecutar esta rama. */
    if (g_modo_stock_limitado)
    {
        /* if: comprueba tiene_limite antes de ejecutar esta rama. */
        if (tiene_limite)
            ok = calcular_cambio_optimo_stock_con_rango(&monto, &g_denom, &g_stock, min_monedas, max_monedas, &solucion);
        else
            ok = calcular_cambio_optimo_stock(&monto, &g_denom, &g_stock, &solucion);
    }
    else
    {
        /* if: comprueba tiene_limite antes de ejecutar esta rama. */
        if (tiene_limite)
            ok = calcular_cambio_optimo_con_rango(&monto, &g_denom, min_monedas, max_monedas, &solucion);
        else
            ok = calcular_cambio_optimo(&monto, &g_denom, &solucion);
    }

    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
    {
        BigInt monto_cubierto = {0};
        BigInt faltante = {0};
        BigIntArray sugerencia = {0};
        BigIntArray stock_sugerido = {0};
        int respuesta;
        int ok_cercano;

        /* if: comprueba tiene_limite antes de ejecutar esta rama. */
        if (tiene_limite)
            ok_cercano = g_modo_stock_limitado
                             ? calcular_cambio_cercano_stock_con_rango(&monto, &g_denom, &g_stock, min_monedas, max_monedas, &monto_cubierto, &sugerencia)
                             : calcular_cambio_cercano_con_rango(&monto, &g_denom, min_monedas, max_monedas, &monto_cubierto, &sugerencia);
        else
            ok_cercano = g_modo_stock_limitado
                             ? calcular_cambio_cercano_stock_con_rango(&monto, &g_denom, &g_stock, 0, (size_t)-1, &monto_cubierto, &sugerencia)
                             : calcular_cambio_cercano_con_rango(&monto, &g_denom, 0, (size_t)-1, &monto_cubierto, &sugerencia);

        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM) "No existe devolucion exacta con los parametros actuales.");

        /* if: comprueba ok_cercano && bigint_subtract(&monto, &monto_cubierto, &faltante) antes de ejecutar esta rama. */
        if (ok_cercano && bigint_subtract(&monto, &monto_cubierto, &faltante))
        {
            snprintf(linea, sizeof(linea), "Sugerencia cercana: %s c (faltan %s c)", monto_cubierto.digits, faltante.digits);
            /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
            if ((int)strlen(linea) > max_chars)
                max_chars = (int)strlen(linea);
            SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);

            respuesta = MessageBoxA(NULL,
                                    "No existe devolucion exacta.\nDeseas aceptar la sugerencia cercana?",
                                    "Sugerencia cercana",
                                    MB_ICONQUESTION | MB_YESNO);

            /* if: comprueba respuesta == IDYES antes de ejecutar esta rama. */
            if (respuesta == IDYES)
            {
                /* if: comprueba g_modo_stock_limitado antes de ejecutar esta rama. */
                if (g_modo_stock_limitado)
                {
                    /* if: comprueba !copiar_arreglo_bigint(&g_stock, &stock_sugerido) antes de ejecutar esta rama. */
                    if (!copiar_arreglo_bigint(&g_stock, &stock_sugerido))
                    {
                        mostrar_error("Cambio", "No se pudo preparar stock para la sugerencia.");
                    }
                    else
                    {
                        int ok_stock = 1;
                        /* for: itera segun size_t i = 0; i < stock_sugerido.len; i++ para recorrer el bloque. */
                        for (size_t i = 0; i < stock_sugerido.len; i++)
                        {
                            BigInt nuevo_stock = {0};

                            /* if: comprueba bigint_is_zero(&sugerencia.items[i]) antes de ejecutar esta rama. */
                            if (bigint_is_zero(&sugerencia.items[i]))
                                continue;

                            /* if: comprueba !bigint_subtract(&stock_sugerido.items[i], &sugerencia.items[i], &nue... antes de ejecutar esta rama. */
                            if (!bigint_subtract(&stock_sugerido.items[i], &sugerencia.items[i], &nuevo_stock) ||
                                !bigint_array_set(&stock_sugerido, i, &nuevo_stock))
                            {
                                bigint_free(&nuevo_stock);
                                ok_stock = 0;
                                break;
                            }

                            bigint_free(&nuevo_stock);
                        }

                        /* if: comprueba !ok_stock antes de ejecutar esta rama. */
                        if (!ok_stock)
                        {
                            bigint_array_free(&stock_sugerido);
                            mostrar_error("Cambio", "No se pudo aplicar la sugerencia al stock.");
                        }
                        /* if: comprueba !actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_sugerido) antes de continuar con esta alternativa. */
                        else if (!actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_sugerido))
                        {
                            bigint_array_free(&stock_sugerido);
                            mostrar_error("Cambio", "No se pudo persistir sugerencia en stock.txt.");
                        }
                        else
                        {
                            bigint_array_free(&g_stock);
                            g_stock = stock_sugerido;
                            refrescar_lista_stock();
                            registrar_historial("Cambio cercano aceptado con stock.");
                        }
                    }
                }
                else
                {
                    registrar_historial("Cambio cercano aceptado (ilimitado).");
                }
            }
            else
            {
                SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM) "Operacion cancelada: no se aplico cambio.");
            }

            /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
            for (size_t i = 0; i < g_denom.len; i++)
            {
                /* if: comprueba bigint_is_zero(&sugerencia.items[i]) antes de ejecutar esta rama. */
                if (bigint_is_zero(&sugerencia.items[i]))
                    continue;

                snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, sugerencia.items[i].digits);
                /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
                if ((int)strlen(linea) > max_chars)
                    max_chars = (int)strlen(linea);
                SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
            }

            /* if: comprueba g_modo_stock_limitado && respuesta != IDYES antes de ejecutar esta rama. */
            if (g_modo_stock_limitado && respuesta != IDYES)
                SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM) "Nota: la sugerencia cercana no se aplico al stock.");
        }

        bigint_free(&faltante);
        bigint_free(&monto_cubierto);
        bigint_array_free(&sugerencia);
        ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);
        bigint_free(&monto);
        bigint_array_free(&solucion);
        return 0;
    }

    /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < g_denom.len; i++)
    {
        /* if: comprueba bigint_is_zero(&solucion.items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&solucion.items[i]))
            continue;

        snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, solucion.items[i].digits);
        /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
        if ((int)strlen(linea) > max_chars)
            max_chars = (int)strlen(linea);
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
        hay_items = 1;
    }

    /* Caso Edge matematico: Nos pidieron cambiar '0' dineros, y el DP retorno true que la formula es DevolverCero. */
    /* if: comprueba !hay_items antes de ejecutar esta rama. */
    if (!hay_items)
    {
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM) "No se requieren monedas para devolver 0.");
        /* if: comprueba (int)strlen("No se requieren monedas para devolver 0.") > max_chars antes de ejecutar esta rama. */
        if ((int)strlen("No se requieren monedas para devolver 0.") > max_chars)
            max_chars = (int)strlen("No se requieren monedas para devolver 0.");
    }

    ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);

    /* Si es el modo irreal que no toca el archivo persistente .txt */
    /* if: comprueba !g_modo_stock_limitado antes de ejecutar esta rama. */
    if (!g_modo_stock_limitado)
    {
        SetWindowTextA(g_editMonto, "");
        bigint_free(&monto);
        bigint_array_free(&solucion);
        registrar_historial("Cambio aplicado (ilimitado).");
        return 1;
    }

    /* SI EL STOCK ES LIMITADO y si hay que ir a tocar Discos... Clona la DB como buffer para operaciones atómicas. */
    /* if: comprueba !copiar_arreglo_bigint(&g_stock, &stock_nuevo) antes de ejecutar esta rama. */
    if (!copiar_arreglo_bigint(&g_stock, &stock_nuevo))
    {
        bigint_free(&monto);
        bigint_array_free(&solucion);
        mostrar_error("Cambio", "No se pudo preparar actualizacion de stock.");
        return 0;
    }

    /* for: itera segun size_t i = 0; i < stock_nuevo.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stock_nuevo.len; i++)
    {
        BigInt nuevo_stock = {0};

        /* if: comprueba bigint_is_zero(&solucion.items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&solucion.items[i]))
            continue;

        /* if: comprueba !bigint_subtract(&stock_nuevo.items[i], &solucion.items[i], &nuevo_st... antes de ejecutar esta rama. */
        if (!bigint_subtract(&stock_nuevo.items[i], &solucion.items[i], &nuevo_stock))
        {
            bigint_array_free(&stock_nuevo);
            bigint_free(&monto);
            bigint_array_free(&solucion);
            mostrar_error("Cambio", "No se pudo aplicar la devolucion al stock.");
            return 0;
        }

        /* if: comprueba !bigint_array_set(&stock_nuevo, i, &nuevo_stock) antes de ejecutar esta rama. */
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

    /* if: comprueba !actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_nuevo) antes de ejecutar esta rama. */
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
/* funcion cargar_moneda_seleccionada: contiene la logica principal de esta operacion. */
static int cargar_moneda_seleccionada(void)
{
    int idx = (int)SendMessageA(g_comboMoneda, CB_GETCURSEL, 0, 0);

    /* if: comprueba idx < 0 || idx >= g_monedasCount antes de ejecutar esta rama. */
    if (idx < 0 || idx >= g_monedasCount)
        return 0;

    liberar_datos_moneda();

    /* if: comprueba !cargar_denominaciones_moneda(g_monedas[idx], &g_denom) antes de ejecutar esta rama. */
    if (!cargar_denominaciones_moneda(g_monedas[idx], &g_denom))
        return 0;

    /* if: comprueba g_modo_stock_limitado antes de ejecutar esta rama. */
    if (g_modo_stock_limitado)
    {

        /* if: comprueba !cargar_stock_moneda(g_monedas[idx], &g_stock) antes de ejecutar esta rama. */
        if (!cargar_stock_moneda(g_monedas[idx], &g_stock))
        {
            bigint_array_free(&g_denom);
            return 0;
        }

        /* if: comprueba g_denom.len != g_stock.len antes de ejecutar esta rama. */
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
/* funcion leer_cantidad_delta: contiene la logica principal de esta operacion. */
static int leer_cantidad_delta(BigInt *delta)
{
    char buffer[256];
    int len = GetWindowTextA(g_editCantidad, buffer, (int)sizeof(buffer));

    /* if: comprueba len <= 0 antes de ejecutar esta rama. */
    if (len <= 0)
        return 0;

    return bigint_init(delta, buffer);
}

/* aplicar_cambio_stock: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion aplicar_cambio_stock: contiene la logica principal de esta operacion. */
static int aplicar_cambio_stock(int esSuma)
{

    /* if: comprueba !g_modo_stock_limitado antes de ejecutar esta rama. */
    if (!g_modo_stock_limitado)
    {
        mostrar_error("Administrador", "El panel administrador solo aplica en modo stock limitado.");
        return 0;
    }

    int idxDenom;
    BigInt delta = {0};
    BigInt nuevo = {0};

    /* if: comprueba g_monedaActiva < 0 antes de ejecutar esta rama. */
    if (g_monedaActiva < 0)
    {
        mostrar_error("Administrador", "Primero carga una moneda.");
        return 0;
    }

    idxDenom = (int)SendMessageA(g_comboDenom, CB_GETCURSEL, 0, 0);

    /* if: comprueba idxDenom < 0 || (size_t)idxDenom >= g_stock.len antes de ejecutar esta rama. */
    if (idxDenom < 0 || (size_t)idxDenom >= g_stock.len)
    {
        mostrar_error("Administrador", "Selecciona una denominacion valida.");
        return 0;
    }

    /* if: comprueba !leer_cantidad_delta(&delta) antes de ejecutar esta rama. */
    if (!leer_cantidad_delta(&delta))
    {
        mostrar_error("Administrador", "Cantidad invalida. Usa un entero no negativo.");
        return 0;
    }

    /* if: comprueba esSuma antes de ejecutar esta rama. */
    if (esSuma)
    {

        /* if: comprueba !bigint_add(&g_stock.items[idxDenom], &delta, &nuevo) antes de ejecutar esta rama. */
        if (!bigint_add(&g_stock.items[idxDenom], &delta, &nuevo))
        {
            bigint_free(&delta);
            mostrar_error("Administrador", "No se pudo sumar la cantidad.");
            return 0;
        }
    }
    else
    {

        /* if: comprueba bigint_compare(&g_stock.items[idxDenom], &delta) < 0 antes de ejecutar esta rama. */
        if (bigint_compare(&g_stock.items[idxDenom], &delta) < 0)
        {
            bigint_free(&delta);
            mostrar_error("Administrador", "No puedes quitar mas stock del disponible.");
            return 0;
        }

        /* if: comprueba !bigint_subtract(&g_stock.items[idxDenom], &delta, &nuevo) antes de ejecutar esta rama. */
        if (!bigint_subtract(&g_stock.items[idxDenom], &delta, &nuevo))
        {
            bigint_free(&delta);
            mostrar_error("Administrador", "No se pudo restar la cantidad.");
            return 0;
        }
    }

    /* if: comprueba !bigint_array_set(&g_stock, (size_t)idxDenom, &nuevo) antes de ejecutar esta rama. */
    if (!bigint_array_set(&g_stock, (size_t)idxDenom, &nuevo))
    {
        bigint_free(&delta);
        bigint_free(&nuevo);
        mostrar_error("Administrador", "No se pudo actualizar el stock en memoria.");
        return 0;
    }

    /* if: comprueba !actualizar_stock_moneda(g_monedas[g_monedaActiva], &g_stock) antes de ejecutar esta rama. */
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
/* funcion aplicar_cambio_stock_lote: contiene la logica principal de esta operacion. */
static int aplicar_cambio_stock_lote(int esSuma)
{
    BigIntArray deltas = {0};
    BigIntArray stock_nuevo = {0};

    /* if: comprueba !g_modo_stock_limitado antes de ejecutar esta rama. */
    if (!g_modo_stock_limitado)
    {
        mostrar_error("Administrador", "El panel administrador solo aplica en modo stock limitado.");
        return 0;
    }

    /* if: comprueba g_monedaActiva < 0 || g_stock.items == NULL || g_denom.len != g_stock... antes de ejecutar esta rama. */
    if (g_monedaActiva < 0 || g_stock.items == NULL || g_denom.len != g_stock.len)
    {
        mostrar_error("Administrador", "Primero carga una moneda valida.");
        return 0;
    }

    /* if: comprueba !leer_cantidades_lote(&deltas) antes de ejecutar esta rama. */
    if (!leer_cantidades_lote(&deltas))
    {
        mostrar_error("Administrador", "Lote invalido. Introduce exactamente una cantidad por denominacion (linea por linea).");
        return 0;
    }

    /* if: comprueba !copiar_arreglo_bigint(&g_stock, &stock_nuevo) antes de ejecutar esta rama. */
    if (!copiar_arreglo_bigint(&g_stock, &stock_nuevo))
    {
        bigint_array_free(&deltas);
        mostrar_error("Administrador", "No se pudo preparar actualizacion de stock.");
        return 0;
    }

    /* for: itera segun size_t i = 0; i < stock_nuevo.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stock_nuevo.len; i++)
    {
        BigInt nuevo = {0};

        /* if: comprueba bigint_is_zero(&deltas.items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&deltas.items[i]))
            continue;

        /* if: comprueba esSuma antes de ejecutar esta rama. */
        if (esSuma)
        {

            /* if: comprueba !bigint_add(&stock_nuevo.items[i], &deltas.items[i], &nuevo) antes de ejecutar esta rama. */
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
            /* if: comprueba bigint_compare(&stock_nuevo.items[i], &deltas.items[i]) < 0 || antes de ejecutar esta rama. */
            if (bigint_compare(&stock_nuevo.items[i], &deltas.items[i]) < 0 ||
                !bigint_subtract(&stock_nuevo.items[i], &deltas.items[i], &nuevo))
            {
                bigint_array_free(&deltas);
                bigint_array_free(&stock_nuevo);
                mostrar_error("Administrador", "No se pudo quitar lote: falta stock en alguna denominacion.");
                return 0;
            }
        }

        /* if: comprueba !bigint_array_set(&stock_nuevo, i, &nuevo) antes de ejecutar esta rama. */
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

    /* if: comprueba !actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_nuevo) antes de ejecutar esta rama. */
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

/* funcion mostrar_resumen_moneda_gui: contiene la logica principal de esta operacion. */
static int mostrar_resumen_moneda_gui(void)
{
    char mensaje[2048];
    size_t usado = 0;
    const BigInt *minDenom;
    const BigInt *maxDenom;
    size_t i;

    /* if: comprueba g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0 antes de ejecutar esta rama. */
    if (g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0)
    {
        mostrar_error("Resumen", "Primero carga una moneda valida.");
        return 0;
    }

    minDenom = &g_denom.items[0];
    maxDenom = &g_denom.items[0];
    /* for: itera segun i = 1; i < g_denom.len; i++ para recorrer el bloque. */
    for (i = 1; i < g_denom.len; i++)
    {
        /* if: comprueba bigint_compare(&g_denom.items[i], minDenom) < 0 antes de ejecutar esta rama. */
        if (bigint_compare(&g_denom.items[i], minDenom) < 0)
            minDenom = &g_denom.items[i];
        /* if: comprueba bigint_compare(&g_denom.items[i], maxDenom) > 0 antes de ejecutar esta rama. */
        if (bigint_compare(&g_denom.items[i], maxDenom) > 0)
            maxDenom = &g_denom.items[i];
    }

    usado += (size_t)snprintf(mensaje + usado, sizeof(mensaje) - usado,
                              "Moneda: %s\r\nDenominaciones: %zu\r\nMin: %s c\r\nMax: %s c\r\n",
                              g_monedas[g_monedaActiva],
                              g_denom.len,
                              minDenom->digits,
                              maxDenom->digits);

    /* if: comprueba !g_modo_stock_limitado antes de ejecutar esta rama. */
    if (!g_modo_stock_limitado)
    {
        (void)snprintf(mensaje + usado, sizeof(mensaje) - usado,
                       "Modo stock ilimitado: no se calcula inventario fisico.");
        mostrar_info("Resumen", mensaje);
        registrar_historialf("Resumen consultado (windows) | Moneda=%s | Modo=Ilimitado",
                             g_monedas[g_monedaActiva]);
        return 1;
    }

    /* if: comprueba g_stock.items == NULL || g_stock.len != g_denom.len antes de ejecutar esta rama. */
    if (g_stock.items == NULL || g_stock.len != g_denom.len)
    {
        mostrar_error("Resumen", "No hay stock valido para calcular resumen.");
        return 0;
    }

    {
        BigInt totalPiezas = {0};
        BigInt totalValor = {0};
        int ok = 1;

        /* if: comprueba !bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0") antes de ejecutar esta rama. */
        if (!bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0"))
            ok = 0;

        /* for: itera segun i = 0; ok && i < g_denom.len; i++ para recorrer el bloque. */
        for (i = 0; ok && i < g_denom.len; i++)
        {
            BigInt nuevoTotalPiezas = {0};
            BigInt parcialValor = {0};
            BigInt nuevoTotalValor = {0};

            /* if: comprueba !bigint_add(&totalPiezas, &g_stock.items[i], &nuevoTotalPiezas) || antes de ejecutar esta rama. */
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

        /* if: comprueba !ok antes de ejecutar esta rama. */
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

/* funcion ejecutar_conversion_gui: contiene la logica principal de esta operacion. */
static int ejecutar_conversion_gui(void)
{
    char origen[128];
    char montoTexto[128];
    char linea[512];
    BigInt montoOrigen = {0};
    BigInt montoDestino = {0};
    BigIntArray stockConversion = {0};
    BigIntArray solucion = {0};
    double tasa = 0.0;
    int usarStock;
    int ok;
    int max_chars = 0;

    /* if: comprueba g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0 antes de ejecutar esta rama. */
    if (g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0)
    {
        mostrar_error("Conversion", "Primero carga la moneda destino en la interfaz.");
        return 0;
    }

    /* if: comprueba GetWindowTextA(g_editConvertOrigen, origen, (int)sizeof(origen)) <= 0 antes de ejecutar esta rama. */
    if (GetWindowTextA(g_editConvertOrigen, origen, (int)sizeof(origen)) <= 0)
    {
        mostrar_error("Conversion", "Indica la moneda origen.");
        return 0;
    }

    /* if: comprueba GetWindowTextA(g_editConvertMonto, montoTexto, (int)sizeof(montoTexto)) <= 0 antes de ejecutar esta rama. */
    if (GetWindowTextA(g_editConvertMonto, montoTexto, (int)sizeof(montoTexto)) <= 0 ||
        !bigint_init(&montoOrigen, montoTexto))
    {
        mostrar_error("Conversion", "Indica un monto valido en centimos.");
        bigint_free(&montoOrigen);
        return 0;
    }

    usarStock = (SendMessageA(g_checkConvertStock, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

    /* if: comprueba !fetch_exchange_rate(origen, g_monedas[g_monedaActiva], &tasa) antes de ejecutar esta rama. */
    if (!fetch_exchange_rate(origen, g_monedas[g_monedaActiva], &tasa))
    {
        mostrar_error("Conversion", "No se pudo obtener la tasa de cambio solicitada.");
        bigint_free(&montoOrigen);
        return 0;
    }

    {
        double src_cents = strtod(montoOrigen.digits, NULL);
        double dst_cents = src_cents * tasa;
        long long rounded = (long long)(dst_cents + 0.5);
        char tmp[64];

        snprintf(tmp, sizeof(tmp), "%lld", rounded);
        /* if: comprueba !bigint_init(&montoDestino, tmp) antes de ejecutar esta rama. */
        if (!bigint_init(&montoDestino, tmp))
        {
            mostrar_error("Conversion", "No se pudo preparar el monto convertido.");
            bigint_free(&montoOrigen);
            return 0;
        }
    }

    /* if: comprueba usarStock && !cargar_stock_moneda(g_monedas[g_monedaActiva], &stockConversion) antes de ejecutar esta rama. */
    if (usarStock && !cargar_stock_moneda(g_monedas[g_monedaActiva], &stockConversion))
    {
        mostrar_error("Conversion", "No se pudo cargar el stock real de la moneda destino.");
        bigint_free(&montoOrigen);
        bigint_free(&montoDestino);
        return 0;
    }

    ok = usarStock
             ? calcular_cambio_optimo_stock(&montoDestino, &g_denom, &stockConversion, &solucion)
             : calcular_cambio_optimo(&montoDestino, &g_denom, &solucion);

    /* if: comprueba !ok antes de ejecutar esta rama. */
    if (!ok)
    {
        mostrar_error("Conversion", "No se pudo descomponer el monto convertido en la moneda destino.");
        bigint_free(&montoOrigen);
        bigint_free(&montoDestino);
        bigint_array_free(&stockConversion);
        bigint_array_free(&solucion);
        return 0;
    }

    limpiar_resultado_cambio();
    snprintf(linea, sizeof(linea),
             "Conversion aplicada: %s -> %s | origen=%s c | destino=%s c | tasa=%.6f",
             origen,
             g_monedas[g_monedaActiva],
             montoOrigen.digits,
             montoDestino.digits,
             tasa);
    max_chars = (int)strlen(linea);
    SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);

    /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < g_denom.len; i++)
    {
        /* if: comprueba bigint_is_zero(&solucion.items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&solucion.items[i]))
            continue;

        snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, solucion.items[i].digits);
        /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
        if ((int)strlen(linea) > max_chars)
            max_chars = (int)strlen(linea);
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
    }
    ajustar_scroll_horizontal_lista(g_listResultado, max_chars + 4);

    registrar_historialf("Conversion windows | %s->%s | Origen=%s c | Destino=%s c | Tasa=%f",
                         origen,
                         g_monedas[g_monedaActiva],
                         montoOrigen.digits,
                         montoDestino.digits,
                         tasa);
    SetWindowTextA(g_editConvertMonto, "");
    mostrar_info("Conversion", "Conversion completada y resultado mostrado.");

    bigint_free(&montoOrigen);
    bigint_free(&montoDestino);
    bigint_array_free(&stockConversion);
    bigint_array_free(&solucion);
    return 1;
}

/* aplicar_cambio_especifico: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion aplicar_cambio_especifico: contiene la logica principal de esta operacion. */
static int aplicar_cambio_especifico(void)
{
    BigIntArray entradas = {0};
    BigIntArray devolucion = {0};
    BigIntArray stock_nuevo = {0};
    BigInt totalEntrada = {0};
    BigInt totalDevolucion = {0};
    char linea[512];
    int max_chars = 0;

    /* if: comprueba g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0 antes de ejecutar esta rama. */
    if (g_monedaActiva < 0 || g_denom.items == NULL || g_denom.len == 0)
    {
        mostrar_error("Cambio especifico", "Primero carga una moneda valida.");
        return 0;
    }

    /* if: comprueba !leer_cantidades_desde_edit(g_editEntradaCambio, &entradas) || antes de ejecutar esta rama. */
    if (!leer_cantidades_desde_edit(g_editEntradaCambio, &entradas) ||
        !leer_cantidades_desde_edit(g_editDevolucionCambio, &devolucion))
    {
        mostrar_error("Cambio especifico", "Entrada invalida. Usa una cantidad por linea y por denominacion en ambos cuadros.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        return 0;
    }

    /* if: comprueba !calcular_total_valor(&g_denom, &entradas, &totalEntrada) || antes de ejecutar esta rama. */
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

    /* if: comprueba bigint_compare(&totalEntrada, &totalDevolucion) != 0 antes de ejecutar esta rama. */
    if (bigint_compare(&totalEntrada, &totalDevolucion) != 0)
    {
        mostrar_error("Cambio especifico", "El total entregado debe ser igual al total solicitado como devolucion.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    /* if: comprueba !g_modo_stock_limitado antes de ejecutar esta rama. */
    if (!g_modo_stock_limitado)
    {
        limpiar_resultado_cambio();
        snprintf(linea, sizeof(linea), "Cambio especifico aplicado (ilimitado): %s c", totalEntrada.digits);
        max_chars = (int)strlen(linea);
        SendMessageA(g_listResultado, LB_ADDSTRING, 0, (LPARAM)linea);
        /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
        for (size_t i = 0; i < g_denom.len; i++)
        {
            /* if: comprueba bigint_is_zero(&devolucion.items[i]) antes de ejecutar esta rama. */
            if (bigint_is_zero(&devolucion.items[i]))
                continue;

            snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, devolucion.items[i].digits);
            /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
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

    /* if: comprueba g_stock.items == NULL || g_denom.len != g_stock.len antes de ejecutar esta rama. */
    if (g_stock.items == NULL || g_denom.len != g_stock.len)
    {
        mostrar_error("Cambio especifico", "No hay stock cargado para modo limitado.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    /* if: comprueba !copiar_arreglo_bigint(&g_stock, &stock_nuevo) antes de ejecutar esta rama. */
    if (!copiar_arreglo_bigint(&g_stock, &stock_nuevo))
    {
        mostrar_error("Cambio especifico", "No se pudo preparar la actualizacion de stock.");
        bigint_array_free(&entradas);
        bigint_array_free(&devolucion);
        bigint_free(&totalEntrada);
        bigint_free(&totalDevolucion);
        return 0;
    }

    /* for: itera segun size_t i = 0; i < stock_nuevo.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stock_nuevo.len; i++)
    {
        BigInt trasEntrada = {0};
        BigInt trasSalida = {0};

        /* if: comprueba !bigint_add(&stock_nuevo.items[i], &entradas.items[i], &trasEntrada) || antes de ejecutar esta rama. */
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

    /* if: comprueba !actualizar_stock_moneda(g_monedas[g_monedaActiva], &stock_nuevo) antes de ejecutar esta rama. */
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
    /* for: itera segun size_t i = 0; i < g_denom.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < g_denom.len; i++)
    {
        /* if: comprueba bigint_is_zero(&devolucion.items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&devolucion.items[i]))
            continue;

        snprintf(linea, sizeof(linea), "%s c -> %s", g_denom.items[i].digits, devolucion.items[i].digits);
        /* if: comprueba (int)strlen(linea) > max_chars antes de ejecutar esta rama. */
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
/* funcion crear_controles: contiene la logica principal de esta operacion. */
static void crear_controles(HWND hwnd)
{
    CreateWindowA("STATIC", TR("Panel Principal", "Main Panel"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                  20, 20, 220, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", TR("Moneda:", "Currency:"), WS_CHILD | WS_VISIBLE,
                  20, 50, 60, 20, hwnd, NULL, NULL, NULL);

    g_comboMoneda = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                  80, 48, 180, 400, hwnd, (HMENU)ID_COMBO_MONEDA, NULL, NULL);

    g_btnCargar = CreateWindowA("BUTTON", TR("Cargar", "Load"), WS_CHILD | WS_VISIBLE,
                                270, 47, 90, 24, hwnd, (HMENU)ID_BTN_CARGAR, NULL, NULL);

    g_btnRecargar = CreateWindowA("BUTTON", TR("Recargar", "Reload"), WS_CHILD | WS_VISIBLE,
                                  370, 47, 90, 24, hwnd, (HMENU)ID_BTN_RECARGAR, NULL, NULL);

    g_radioLimitado = CreateWindowA("BUTTON", TR("Stock limitado", "Limited stock"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                    20, 76, 130, 20, hwnd, (HMENU)ID_RADIO_LIMITADO, NULL, NULL);

    g_radioIlimitado = CreateWindowA("BUTTON", TR("Stock ilimitado", "Unlimited stock"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                     160, 76, 130, 20, hwnd, (HMENU)ID_RADIO_ILIMITADO, NULL, NULL);

    configurar_modo_stock(1);

    g_listStock = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | LBS_NOTIFY,
                                20, 102, 440, 140, hwnd, (HMENU)ID_LIST_STOCK, NULL, NULL);

    CreateWindowA("STATIC", TR("Panel Administrador", "Administrator Panel"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                  20, 250, 220, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", TR("Denominacion:", "Denomination:"), WS_CHILD | WS_VISIBLE,
                  20, 285, 90, 20, hwnd, NULL, NULL, NULL);

    g_comboDenom = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                 115, 282, 150, 400, hwnd, (HMENU)ID_COMBO_DENOM, NULL, NULL);

    CreateWindowA("STATIC", TR("Cantidad:", "Quantity:"), WS_CHILD | WS_VISIBLE,
                  280, 285, 60, 20, hwnd, NULL, NULL, NULL);

    g_editCantidad = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   345, 282, 115, 24, hwnd, (HMENU)ID_EDIT_CANTIDAD, NULL, NULL);

    g_btnAgregar = CreateWindowA("BUTTON", TR("Anadir", "Add"), WS_CHILD | WS_VISIBLE,
                                 190, 320, 120, 30, hwnd, (HMENU)ID_BTN_AGREGAR, NULL, NULL);

    g_btnQuitar = CreateWindowA("BUTTON", TR("Quitar", "Remove"), WS_CHILD | WS_VISIBLE,
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

    CreateWindowA("STATIC", TR("Panel Devolucion", "Change Panel"), WS_CHILD | WS_VISIBLE | SS_LEFT,
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

    g_btnCalcular = CreateWindowA("BUTTON", TR("Calcular", "Calculate"), WS_CHILD | WS_VISIBLE,
                                  285, 628, 80, 28, hwnd, (HMENU)ID_BTN_CALCULAR, NULL, NULL);

    g_btnHistorial = CreateWindowA("BUTTON", TR("Historial", "History"), WS_CHILD | WS_VISIBLE,
                                   375, 628, 80, 28, hwnd, (HMENU)ID_BTN_HISTORIAL, NULL, NULL);

    g_btnResumen = CreateWindowA("BUTTON", TR("Resumen", "Summary"), WS_CHILD | WS_VISIBLE,
                                 465, 628, 80, 28, hwnd, (HMENU)ID_BTN_RESUMEN, NULL, NULL);

    g_btnSnapshot = CreateWindowA("BUTTON", "Snapshot", WS_CHILD | WS_VISIBLE,
                                  20, 770, 90, 26, hwnd, (HMENU)ID_BTN_SNAPSHOT, NULL, NULL);

    g_btnRestaurar = CreateWindowA("BUTTON", TR("Restaurar", "Restore"), WS_CHILD | WS_VISIBLE,
                                   120, 770, 90, 26, hwnd, (HMENU)ID_BTN_RESTAURAR, NULL, NULL);

    g_btnReporte = CreateWindowA("BUTTON", TR("Reporte", "Report"), WS_CHILD | WS_VISIBLE,
                                 220, 770, 90, 26, hwnd, (HMENU)ID_BTN_REPORTE, NULL, NULL);

    g_btnExportJson = CreateWindowA("BUTTON", "JSON", WS_CHILD | WS_VISIBLE,
                                    320, 770, 90, 26, hwnd, (HMENU)ID_BTN_EXPORT_JSON, NULL, NULL);

    CreateWindowA("STATIC", TR("Conversion a moneda cargada", "Conversion to loaded currency"), WS_CHILD | WS_VISIBLE | SS_LEFT,
                  20, 806, 180, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "Origen:", WS_CHILD | WS_VISIBLE,
                  20, 832, 50, 20, hwnd, NULL, NULL, NULL);

    g_editConvertOrigen = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                        72, 830, 120, 24, hwnd, (HMENU)ID_EDIT_CONVERT_ORIGEN, NULL, NULL);

    CreateWindowA("STATIC", "Monto c:", WS_CHILD | WS_VISIBLE,
                  205, 832, 55, 20, hwnd, NULL, NULL, NULL);

    g_editConvertMonto = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                       262, 830, 110, 24, hwnd, (HMENU)ID_EDIT_CONVERT_MONTO, NULL, NULL);

    g_checkConvertStock = CreateWindowA("BUTTON", "Usar stock destino", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                        385, 831, 130, 22, hwnd, (HMENU)ID_CHECK_CONVERT_STOCK, NULL, NULL);

    g_btnConvertir = CreateWindowA("BUTTON", TR("Convertir", "Convert"), WS_CHILD | WS_VISIBLE,
                                   525, 827, 90, 30, hwnd, (HMENU)ID_BTN_CONVERTIR, NULL, NULL);

    g_listResultado = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | LBS_NOTIFY,
                                    20, 665, 570, 100, hwnd, (HMENU)ID_LIST_RESULTADO, NULL, NULL);
}

/* wnd_proc: Gestiona la inicializacion y el ciclo de mensajes de la interfaz de usuario. */
/* funcion wnd_proc: contiene la logica principal de esta operacion. */
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    /* switch: selecciona la ruta segun msg. */
    switch (msg)
    {
    case WM_CREATE:
        crear_controles(hwnd);

        /* if: comprueba !cargar_nombres_moneda() antes de ejecutar esta rama. */
        if (!cargar_nombres_moneda())
        {
            mostrar_error("Inicio", "No se pudieron leer monedas desde monedas.txt");
        }
        refrescar_combo_monedas();
        break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);

        /* if: comprueba id == ID_BTN_CARGAR antes de ejecutar esta rama. */
        if (id == ID_BTN_CARGAR)
        {
            /* if: comprueba !cargar_moneda_seleccionada() antes de ejecutar esta rama. */
            if (!cargar_moneda_seleccionada())
                mostrar_error("Carga", "No se pudo cargar moneda/stock.");
            return 0;
        }

        /* if: comprueba id == ID_BTN_RECARGAR antes de ejecutar esta rama. */
        if (id == ID_BTN_RECARGAR)
        {
            int idx_prev = (int)SendMessageA(g_comboMoneda, CB_GETCURSEL, 0, 0);

            /* if: comprueba !cargar_nombres_moneda() antes de ejecutar esta rama. */
            if (!cargar_nombres_moneda())
            {
                mostrar_error("Recarga", "No se pudieron recargar monedas.");
                return 0;
            }

            liberar_datos_moneda();
            refrescar_combo_monedas();

            /* if: comprueba idx_prev >= 0 && idx_prev < g_monedasCount antes de ejecutar esta rama. */
            if (idx_prev >= 0 && idx_prev < g_monedasCount)
                SendMessageA(g_comboMoneda, CB_SETCURSEL, (WPARAM)idx_prev, 0);

            /* if: comprueba !cargar_moneda_seleccionada() antes de ejecutar esta rama. */
            if (!cargar_moneda_seleccionada())
            {
                mostrar_error("Recarga", "No se pudo recargar moneda/stock.");
                refrescar_lista_stock();
            }
            return 0;
        }

        /* if: comprueba id == ID_BTN_AGREGAR antes de ejecutar esta rama. */
        if (id == ID_BTN_AGREGAR)
        {
            aplicar_cambio_stock(1);
            return 0;
        }

        /* if: comprueba id == ID_BTN_QUITAR antes de ejecutar esta rama. */
        if (id == ID_BTN_QUITAR)
        {
            aplicar_cambio_stock(0);
            return 0;
        }

        /* if: comprueba id == ID_BTN_AGREGAR_LOTE antes de ejecutar esta rama. */
        if (id == ID_BTN_AGREGAR_LOTE)
        {
            aplicar_cambio_stock_lote(1);
            return 0;
        }

        /* if: comprueba id == ID_BTN_QUITAR_LOTE antes de ejecutar esta rama. */
        if (id == ID_BTN_QUITAR_LOTE)
        {
            aplicar_cambio_stock_lote(0);
            return 0;
        }

        /* if: comprueba id == ID_BTN_HISTORIAL antes de ejecutar esta rama. */
        if (id == ID_BTN_HISTORIAL)
        {
            system("notepad historial.txt");
            return 0;
        }

        /* if: comprueba id == ID_BTN_RESUMEN antes de ejecutar esta rama. */
        if (id == ID_BTN_RESUMEN)
        {
            mostrar_resumen_moneda_gui();
            return 0;
        }

        /* if: comprueba id == ID_BTN_SNAPSHOT antes de ejecutar esta rama. */
        if (id == ID_BTN_SNAPSHOT)
        {
            ejecutar_operacion_global_gui("snapshot");
            return 0;
        }

        /* if: comprueba id == ID_BTN_RESTAURAR antes de ejecutar esta rama. */
        if (id == ID_BTN_RESTAURAR)
        {
            ejecutar_operacion_global_gui("restaurar");
            return 0;
        }

        /* if: comprueba id == ID_BTN_REPORTE antes de ejecutar esta rama. */
        if (id == ID_BTN_REPORTE)
        {
            ejecutar_operacion_global_gui("reporte");
            return 0;
        }

        /* if: comprueba id == ID_BTN_EXPORT_JSON antes de ejecutar esta rama. */
        if (id == ID_BTN_EXPORT_JSON)
        {
            ejecutar_operacion_global_gui("json");
            return 0;
        }

        /* if: comprueba id == ID_BTN_CONVERTIR antes de ejecutar esta rama. */
        if (id == ID_BTN_CONVERTIR)
        {
            ejecutar_conversion_gui();
            return 0;
        }

        /* if: comprueba id == ID_CHECK_CAJA antes de ejecutar esta rama. */
        if (id == ID_CHECK_CAJA)
        {
            /* if: comprueba SendMessageA(g_checkCaja, BM_GETCHECK, 0, 0) == BST_CHECKED antes de ejecutar esta rama. */
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

        /* if: comprueba id == ID_BTN_CALCULAR antes de ejecutar esta rama. */
        if (id == ID_BTN_CALCULAR)
        {
            calcular_y_mostrar_cambio();
            return 0;
        }

        /* if: comprueba id == ID_BTN_CAMBIO_ESPECIFICO antes de ejecutar esta rama. */
        if (id == ID_BTN_CAMBIO_ESPECIFICO)
        {
            aplicar_cambio_especifico();
            return 0;
        }

        /* if: comprueba id == ID_RADIO_LIMITADO || id == ID_RADIO_ILIMITADO antes de ejecutar esta rama. */
        if (id == ID_RADIO_LIMITADO || id == ID_RADIO_ILIMITADO)
        {
            int nuevo_limitado = (id == ID_RADIO_LIMITADO) ? 1 : 0;

            /* if: comprueba nuevo_limitado != g_modo_stock_limitado antes de ejecutar esta rama. */
            if (nuevo_limitado != g_modo_stock_limitado)
            {
                int idx = (int)SendMessageA(g_comboMoneda, CB_GETCURSEL, 0, 0);
                configurar_modo_stock(nuevo_limitado);
                limpiar_resultado_cambio();
                /* if: comprueba idx >= 0 && idx < g_monedasCount antes de ejecutar esta rama. */
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
/* funcion WinMain: contiene la logica principal de esta operacion. */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {0};
    HWND hwnd;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;
    progvoraz_locale_init_from_env();

    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ProgVorazGUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    /* if: comprueba !RegisterClassA(&wc) antes de ejecutar esta rama. */
    if (!RegisterClassA(&wc))
        return 1;

    hwnd = CreateWindowA("ProgVorazGUI", TR("ProgVoraz - Interfaz Grafica", "ProgVoraz - Graphical Interface"),
                         WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                         CW_USEDEFAULT, CW_USEDEFAULT, 660, 940,
                         NULL, NULL, hInstance, NULL);

    /* if: comprueba hwnd == NULL antes de ejecutar esta rama. */
    if (hwnd == NULL)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    /* while: repite el bloque mientras se cumpla GetMessageA(&msg, NULL, 0, 0) > 0. */
    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
#else
/* main: Punto de entrada principal. Configura el entorno y lanza la ejecucion. */
/* funcion main: contiene la logica principal de esta operacion. */
int main(void)
{
    return 0;
}
#endif

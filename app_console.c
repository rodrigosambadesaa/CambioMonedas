/*
 * app_console.c
 *
 * PROPOSITO GENERAL
 * -----------------
 * Aplicacion de cambio de monedas con soporte para enteros de tamano arbitrario.
 * Toda la aritmetica de montos, denominaciones, stock y cantidades se realiza
 * mediante BigInt (enteros no negativos sin limite practico de digitos).
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "app_console.h"
#include "bigint.h"
#include "moneda_gestion.h"
#include "algoritmo_cambio.h"
#include "exchange_api.h"

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

/* forward prototypes: used by public wrapper below */
static void dibujar_linea(void);
static void imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock);
/* Implementation moved here so wrapper can call it without forward-declaration issues. */
static void imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock)
{
    const char *json_env = getenv("PROGVORAZ_JSON");
    int json_mode = json_env && json_env[0] != '\0';

    if (json_mode)
    {
        /* Salida JSON compacta para integración */
        printf("{\"resultado\": [");
        for (size_t i = 0; i < monedas->len; i++)
        {
            if (i > 0)
                printf(",");
            printf("{\"denominacion\": \"%s\", \"cantidad\": \"%s\"",
                   monedas->items[i].digits, solucion->items[i].digits);
            if (usarStock && stock)
                printf(", \"stock\": \"%s\"", stock->items[i].digits);
            printf("}");
        }
        printf("]}\n");
        return;
    }

    dibujar_linea();
    printf("Resultado del cambio\n");
    dibujar_linea();

    for (size_t i = 0; i < monedas->len; i++)
    {
        /* Si el modo de ejecucion estaba en 'usarStock' (Modo B o Caja)... */
        if (usarStock)
            printf("Moneda %s c  -> cantidad %s | stock %s\n", monedas->items[i].digits, solucion->items[i].digits, stock->items[i].digits);
        else
            printf("Moneda %s c  -> cantidad %s\n", monedas->items[i].digits, solucion->items[i].digits);
    }

    dibujar_linea();
}
/* Public wrapper: expone la impresion de resultados a otros modulos. */
void app_console_imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock)
{
    imprimir_resultado(monedas, solucion, stock, usarStock);
}

static void registrar_historialf(const char *fmt, ...)
{
    char mensaje[1024];
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
    char linea[512];

    printf("\n--- HISTORIAL DE TRANSACCIONES ---\n");
    if (fp == NULL)
    {
        printf("No hay transacciones registradas.\n");
        printf("----------------------------------\n\n");
        return;
    }

    while (fgets(linea, sizeof(linea), fp) != NULL)
        printf("%s", linea);

    fclose(fp);
    printf("----------------------------------\n\n");
}

static void pausar_pantalla(void)
{
    char buffer[8];

    printf("Presione Enter para continuar...");
    if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL)
        clearerr(stdin);
}

#define MAX_MONEDA_NOMBRE 20
#define MAX_MONEDAS_DISPONIBLES 512

void limpiar_arreglo(BigIntArray *arr);
static void mostrar_resumen_moneda(const char *monedaClave, const BigIntArray *monedas, const BigIntArray *stock, int usarStock);

/* limpiar_arreglo: libera recursos asociados a un BigIntArray (wrapper sobre bigint_array_free). */
void limpiar_arreglo(BigIntArray *arr)
{
    if (arr == NULL)
        return;
    bigint_array_free(arr);
}

/* limpiar_pantalla: Envia secuencias ANSI de escape para borrar la terminal. */
static void limpiar_pantalla(void)
{
    printf("\033[2J\033[H");
}

/* dibujar_linea: Imprime una barra separadora estetica. */
static void dibujar_linea(void)
{
    printf("+--------------------------------------------------+\n");
}

/* dibujar_titulo: Escribe el encabezado principal de la app. */
static void dibujar_titulo(void)
{
    limpiar_pantalla();
    dibujar_linea();
    printf("|          SISTEMA DE CAMBIO DE MONEDAS            |\n");
    dibujar_linea();
}

/* leer_linea: Lee una linea desde teclado limitando la cantidad de caracteres y limpiando saltos. */
static int leer_linea(char *buffer, size_t tam)
{
    if (fgets(buffer, (int)tam, stdin) == NULL)
        return 0;

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

/* a_minusculas: Modifica in-place un string para que todo sea lowercase. */
static void a_minusculas(char *texto)
{
    size_t i;

    if (texto == NULL)
        return;

    for (i = 0; texto[i] != '\0'; i++)
        texto[i] = (char)tolower((unsigned char)texto[i]);
}

/*
 * Normaliza texto de clave para comparacion con archivos:
 * - pasa a minusculas
 * - elimina acentos comunes (UTF-8 y Latin-1/CP1252)
 */
static void normalizar_clave(const char *origen, char *destino, size_t tamDestino)
{
    size_t i = 0;
    size_t j = 0;

    if (destino == NULL || tamDestino == 0)
        return;

    destino[0] = '\0';
    if (origen == NULL)
        return;

    /* Bucle principal: recorre cada byte del string ingresado. */
    while (origen[i] != '\0' && j + 1 < tamDestino)
    {
        unsigned char c = (unsigned char)origen[i];

        /* Detecta el primer byte del prefijo UTF-8 para vocales acentuadas comunes (0xC3). */
        if (c == 0xC3 && origen[i + 1] != '\0')
        {
            unsigned char s = (unsigned char)origen[i + 1];
            char reemplazo = '\0';

            if (s == 0xA1 || s == 0x81)
                reemplazo = 'a';
            else if (s == 0xA9 || s == 0x89)
                reemplazo = 'e';
            else if (s == 0xAD || s == 0x8D)
                reemplazo = 'i';
            else if (s == 0xB3 || s == 0x93)
                reemplazo = 'o';
            else if (s == 0xBA || s == 0x9A || s == 0xBC || s == 0x9C)
                reemplazo = 'u';
            else if (s == 0xB1 || s == 0x91)
                reemplazo = 'n';

            /* Si se determino una traduccion... */
            if (reemplazo != '\0')
            {
                destino[j++] = reemplazo;
                i += 2;
                continue;
            }
        }

        // Tabla de normalizacion rapida por bytes sueltos (para CP1252 o restos).
        if (c == 0xE1 || c == 0xC1 || c == 0xA0)
            destino[j++] = 'a';
        else if (c == 0xE9 || c == 0xC9 || c == 0x82 || c == 0x90)
            destino[j++] = 'e';
        else if (c == 0xED || c == 0xCD || c == 0xA1 || c == 0xD6)
            destino[j++] = 'i';
        else if (c == 0xF3 || c == 0xD3 || c == 0xA2 || c == 0xE0)
            destino[j++] = 'o';
        else if (c == 0xFA || c == 0xDA || c == 0xFC || c == 0xDC || c == 0xA3 || c == 0x81 || c == 0x9A)
            destino[j++] = 'u';
        else if (c == 0xF1 || c == 0xD1 || c == 0xA4 || c == 0xA5)
            destino[j++] = 'n';
        else
            destino[j++] = (char)tolower(c);

        i++;
    }

    destino[j] = '\0';
}

/* es_token_denominacion: Valida si la palabra leida es un numero natural o un flag de -1. */
static int es_token_denominacion(const char *token)
{
    size_t i;

    if (token == NULL || token[0] == '\0')
        return 0;

    if (strcmp(token, "-1") == 0)
        return 1;

    for (i = 0; token[i] != '\0'; i++)
    {
        if (token[i] < '0' || token[i] > '9')
            return 0;
    }

    return 1;
}

/* clave_ya_existe: Chequea en un arreglo de strings si ya hemos cargado este nombre de moneda. */
static int clave_ya_existe(char claves[][MAX_MONEDA_NOMBRE + 1], size_t cantidad, const char *clave)
{
    size_t i;

    for (i = 0; i < cantidad; i++)
    {
        if (strcmp(claves[i], clave) == 0)
            return 1;
    }

    return 0;
}

/* copiar_clave: Wrapper seguro para copiar strings fijos de tamaño especifico en la RAM. */
static void copiar_clave(char destino[MAX_MONEDA_NOMBRE + 1], const char *origen)
{
    size_t i = 0;

    /* Protege contra accesos invalidos verificando que no copiemos la nada. */
    if (origen == NULL)
    {
        destino[0] = '\0';
        return;
    }

    /* Bucle de clonacion: copia hasta alcanzar el MAXimo permitido para que no haya buffer overflow. */
    while (i < MAX_MONEDA_NOMBRE && origen[i] != '\0')
    {
        destino[i] = origen[i];
        i++;
    }

    destino[i] = '\0';
}

/* cargar_claves_monedas: Extrae la lista de las monedas (ej: Euro, Dolar) del archivo txt parseando. */
static int cargar_claves_monedas(const char *archivo, char claves[][MAX_MONEDA_NOMBRE + 1], size_t maxClaves, size_t *cantidad)
{
    FILE *fp;
    char token[256];

    if (archivo == NULL || claves == NULL || cantidad == NULL)
        return 0;

    *cantidad = 0;
    fp = fopen(archivo, "r");
    if (fp == NULL)
        return 0;

    /* Extrae secuencialmente todas las palabras (tokens) separadas por whitespace en el archivo. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        char clave[MAX_MONEDA_NOMBRE + 1];

        if (es_token_denominacion(token))
            continue;

        normalizar_clave(token, clave, sizeof(clave));
        if (clave[0] == '\0')
            continue;
        if (clave_ya_existe(claves, *cantidad, clave))
            continue;

        /* Chequea que el arreglo del menu de la consola no exceda su tamano maximo hardcodeado. */
        if (*cantidad >= maxClaves)
        {
            fclose(fp);
            return 0;
        }

        copiar_clave(claves[*cantidad], clave);
        (*cantidad)++;
    }

    fclose(fp);
    return 1;
}

/* imprimir_lista_monedas: Lista por consola las monedas disponibles en formato CSV wrap-around. */
static void imprimir_lista_monedas(const char claves[][MAX_MONEDA_NOMBRE + 1], size_t cantidad)
{
    size_t i;

    /* Valida si existen monedas para mostrar. */
    if (cantidad == 0)
    {
        printf("Monedas disponibles: (sin datos)\n");
        return;
    }

    printf("Monedas disponibles:\n");
    for (i = 0; i < cantidad; i++)
    {
        printf("%s", claves[i]);
        if (i + 1 < cantidad)
            printf(", ");
        if ((i + 1) % 8 == 0 || i + 1 == cantidad)
            printf("\n");
    }
}

/* mostrar_monedas_disponibles: Extrae, filtra e imprime las divisas dependiendo del modo seleccionado. */
static void mostrar_monedas_disponibles(int opcion)
{
    char desdeMonedas[MAX_MONEDAS_DISPONIBLES][MAX_MONEDA_NOMBRE + 1];
    char desdeStock[MAX_MONEDAS_DISPONIBLES][MAX_MONEDA_NOMBRE + 1];
    char visibles[MAX_MONEDAS_DISPONIBLES][MAX_MONEDA_NOMBRE + 1];
    size_t nMonedas = 0;
    size_t nStock = 0;
    size_t nVisibles = 0;
    size_t i;

    /* Si el usuario eligio la opcion 'a' (Modo infinito)... */
    if (opcion == 'a')
    {
        /* Intenta leer unicamente las definiciones de monedas sin importar si tienen stock. */
        if (!cargar_claves_monedas("monedas.txt", desdeMonedas, MAX_MONEDAS_DISPONIBLES, &nMonedas))
        {
            printf("No se pudieron leer monedas disponibles desde monedas.txt.\n");
            return;
        }

        imprimir_lista_monedas((const char (*)[MAX_MONEDA_NOMBRE + 1]) desdeMonedas, nMonedas);
        return;
    }

    // Modo 'b' o 'c' (Limitado o Admin): requiere cruzar datos de ambas DB.
    if (!cargar_claves_monedas("monedas.txt", desdeMonedas, MAX_MONEDAS_DISPONIBLES, &nMonedas) ||
        !cargar_claves_monedas("stock.txt", desdeStock, MAX_MONEDAS_DISPONIBLES, &nStock))
    {
        printf("No se pudieron leer monedas/stock disponibles.\n");
        return;
    }

    for (i = 0; i < nMonedas; i++)
    {
        /* Verifica que la moneda de "monedas.txt" TAMBIEN exista declarada en "stock.txt". */
        if (clave_ya_existe(desdeStock, nStock, desdeMonedas[i]))
        {
            if (nVisibles >= MAX_MONEDAS_DISPONIBLES)
                break;

            copiar_clave(visibles[nVisibles], desdeMonedas[i]);
            nVisibles++;
        }
    }

    imprimir_lista_monedas((const char (*)[MAX_MONEDA_NOMBRE + 1]) visibles, nVisibles);
}

static void mostrar_resumen_desde_menu_principal(void)
{
    char moneda[MAX_MONEDA_NOMBRE + 1];
    char monedaClave[MAX_MONEDA_NOMBRE + 1];
    char comando[MAX_MONEDA_NOMBRE + 1];
    BigIntArray monedas = {0};
    BigIntArray stock = {0};
    int tieneStock;

    mostrar_monedas_disponibles('a');
    printf("Nombre de la moneda para resumen (volver o salir): ");

    if (!leer_linea(moneda, sizeof(moneda)))
    {
        printf("Entrada finalizada.\n");
        return;
    }

    if (moneda[0] == '\0')
    {
        printf("Nombre de moneda no valido.\n");
        return;
    }

    normalizar_clave(moneda, comando, sizeof(comando));
    if (strcmp(comando, "volver") == 0)
        return;
    if (strcmp(comando, "salir") == 0)
    {
        printf("Use 'salir' en el menu principal para cerrar la aplicacion.\n");
        return;
    }

    normalizar_clave(moneda, monedaClave, sizeof(monedaClave));

    if (!cargar_denominaciones_moneda(monedaClave, &monedas))
    {
        printf("No se encontro la moneda en monedas.txt.\n");
        return;
    }

    tieneStock = cargar_stock_moneda(monedaClave, &stock) && stock.len == monedas.len;
    mostrar_resumen_moneda(monedaClave, &monedas, tieneStock ? &stock : NULL, tieneStock);

    if (!tieneStock)
    {
        printf("Resumen mostrado sin stock (no disponible o inconsistente).\n");
        registrar_historialf("Resumen consultado | Moneda=%s | Modo=Ilimitado", monedaClave);
    }

    limpiar_arreglo(&stock);
    limpiar_arreglo(&monedas);
}

static void gestionar_operacion_global(const char *accion)
{
    if (accion == NULL)
        return;

    if (strcmp(accion, "snapshot") == 0)
    {
        if (crear_snapshot_stock("stock_snapshot.txt"))
        {
            printf("Snapshot de stock creado en stock_snapshot.txt\n");
            registrar_historial("Snapshot de stock creado");
        }
        else
        {
            printf("No se pudo crear snapshot de stock.\n");
        }
        return;
    }

    if (strcmp(accion, "restaurar") == 0)
    {
        if (restaurar_snapshot_stock("stock_snapshot.txt"))
        {
            printf("Stock restaurado desde stock_snapshot.txt\n");
            registrar_historial("Stock restaurado desde snapshot");
        }
        else
        {
            printf("No se pudo restaurar stock (verifique stock_snapshot.txt).\n");
        }
        return;
    }

    if (strcmp(accion, "reporte") == 0)
    {
        if (exportar_reporte_global("reporte_global.txt"))
        {
            printf("Reporte global generado en reporte_global.txt\n");
            registrar_historial("Reporte global exportado");
        }
        else
        {
            printf("No se pudo generar reporte global.\n");
        }
    }
}

/*
 * Muestra menu inicial y devuelve opcion normalizada en minuscula.
 * Retorna:
 * - 'a', 'b' o 'c' si la entrada es valida
 * - -3 si usuario pide ver historial
 * - -4 si usuario pide ver resumen de moneda
 * - -5 si usuario pide crear snapshot de stock
 * - -6 si usuario pide restaurar snapshot de stock
 * - -7 si usuario pide generar reporte global
 * - 0 si la entrada es invalida/vacia
 * - -1 si hubo EOF/error de lectura
 * - -2 si usuario pide salir
 */
static int pedir_opcion(void)
{
    char buffer[16];
    char comando[16];

    dibujar_titulo();
    printf("| Elija modo de trabajo:                            |\n");
    printf("|   a) Monedas infinitas                            |\n");
    printf("|   b) Monedas limitadas (usa stock)                |\n");
    printf("|   c) Administrador de stock                       |\n");
    printf("|   h) Historial de transacciones                   |\n");
    printf("|   r) Resumen de moneda                            |\n");
    printf("|   s) Snapshot stock                               |\n");
    printf("|   u) Restaurar snapshot                           |\n");
    printf("|   g) Generar reporte global                       |\n");
    printf("|   x) Conversion entre monedas                     |\n");
    dibujar_linea();
    printf("Opcion (a/b/c/h/r/s/u/g/x o salir): ");

    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);
    if (strcmp(comando, "salir") == 0)
        return -2;
    if (strcmp(comando, "historial") == 0 || strcmp(comando, "h") == 0)
        return -3;
    if (strcmp(comando, "resumen") == 0 || strcmp(comando, "r") == 0)
        return -4;
    if (strcmp(comando, "snapshot") == 0 || strcmp(comando, "s") == 0)
        return -5;
    if (strcmp(comando, "restaurar") == 0 || strcmp(comando, "u") == 0)
        return -6;
    if (strcmp(comando, "reporte") == 0 || strcmp(comando, "g") == 0)
        return -7;
    if (strcmp(comando, "convertir") == 0 || strcmp(comando, "x") == 0)
        return (int)'x';

    return (int)tolower((unsigned char)buffer[0]);
}

/*
 * Solicita monto y lo parsea como BigInt.
 * Retorna:
 * - 1 si lectura valida
 * - 0 si entrada invalida
 * - -1 si EOF/error
 * - 2 si usuario pide volver a seleccion de moneda ("0" o "volver")
 * - 3 si usuario pide salir
 */
static int pedir_cantidad(BigInt *cantidad)
{
    char buffer[2048];
    char comando[2048];
    BigInt tmp = {0};

    printf("Cantidad en centimos (0, volver o salir): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    if (strcmp(comando, "salir") == 0)
        return 3;

    if (strcmp(comando, "volver") == 0 || strcmp(comando, "0") == 0)
        return 2;

    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(cantidad);
    *cantidad = tmp;
    return 1;
}

/*
 * Solicita una cantidad para operaciones de administrador.
 * Retorna:
 * - 1 si lectura valida
 * - 0 si entrada invalida
 * - -1 si EOF/error
 * - 2 si usuario pide volver
 * - 3 si usuario pide cambiar de modo
 * - 4 si usuario pide salir
 */
static int pedir_cantidad_admin(BigInt *cantidad)
{
    char buffer[2048];
    char comando[2048];
    BigInt tmp = {0};

    printf("Cantidad (volver, modo o salir): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    if (strcmp(comando, "salir") == 0)
        return 4;
    if (strcmp(comando, "modo") == 0)
        return 3;
    if (strcmp(comando, "volver") == 0)
        return 2;

    if (!bigint_init(&tmp, buffer))
        return 0;

    bigint_free(cantidad);
    *cantidad = tmp;
    return 1;
}

/* pedir_indice_denominacion: Pide al admin que divisa especifica dentro del array de monedas quiere modificar. */
static int pedir_indice_denominacion(size_t maximo, size_t *indice)
{
    char buffer[64];
    char comando[64];
    char *fin;
    long v;

    printf("Indice de denominacion (1-%zu, o volver/modo/salir): ", maximo);
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    if (strcmp(comando, "salir") == 0)
        return 4;
    if (strcmp(comando, "modo") == 0)
        return 3;
    if (strcmp(comando, "volver") == 0)
        return 2;

    v = strtol(buffer, &fin, 10);
    if (*fin != '\0' || v < 1 || (size_t)v > maximo)
        return 0;

    *indice = (size_t)(v - 1);
    return 1;
}

/* copiar_arreglo_bigint: Clona el vector dinamico subyacente que aloja BigInts. */
static int copiar_arreglo_bigint(const BigIntArray *origen, BigIntArray *destino)
{
    if (origen == NULL || destino == NULL)
        return 0;

    if (!bigint_array_create(destino, origen->len))
        return 0;

    for (size_t i = 0; i < origen->len; i++)
    {
        /* Trata de inyectar y clonar el elemento 'i' desde el origen al indice equivalente 'i' del destino. */
        if (!bigint_array_set(destino, i, &origen->items[i]))
        {
            bigint_array_free(destino);
            return 0;
        }
    }

    return 1;
}

/* limpiar_arreglo: Wrapper para destructores de BigInt. */
void limpiar_arreglo(BigIntArray *arr);
static void mostrar_resumen_moneda(const char *monedaClave, const BigIntArray *monedas, const BigIntArray *stock, int usarStock);

/* imprimir_resultado: Muestra la solucion del calculo de forma entendible por consola. */

static void imprimir_sugerencia_cercana(const BigInt *solicitado,
                                        const BigInt *cubierto,
                                        const BigIntArray *monedas,
                                        const BigIntArray *solucion,
                                        int con_stock)
{
    BigInt faltante = {0};

    if (solicitado == NULL || cubierto == NULL || monedas == NULL || solucion == NULL)
        return;

    if (!bigint_subtract(solicitado, cubierto, &faltante))
    {
        printf("No existe cambio exacto. Se encontro un cambio cercano parcial.\n");
        imprimir_resultado(monedas, solucion, NULL, 0);
        return;
    }

    printf("No existe cambio exacto.\n");
    printf("Sugerencia cercana: cubrir %s c (faltan %s c).\n", cubierto->digits, faltante.digits);
    if (con_stock)
        printf("Esta sugerencia no se aplica automaticamente al stock actual.\n");

    imprimir_resultado(monedas, solucion, NULL, 0);
    bigint_free(&faltante);
}

static int preguntar_aceptar_sugerencia(const BigInt *solicitado,
                                        const BigInt *cubierto,
                                        int con_stock)
{
    BigInt faltante = {0};
    char buffer[32];
    char comando[32];

    if (solicitado == NULL || cubierto == NULL)
        return 0;

    if (!bigint_subtract(solicitado, cubierto, &faltante))
        return 0;

    while (1)
    {
        if (con_stock)
            printf("No hay cambio exacto. Sugerencia: devolver %s c (faltan %s c). Aceptar? (si/no): ", cubierto->digits, faltante.digits);
        else
            printf("No hay cambio exacto. Sugerencia: cubrir %s c (faltan %s c). Aceptar? (si/no): ", cubierto->digits, faltante.digits);

        if (!leer_linea(buffer, sizeof(buffer)))
        {
            bigint_free(&faltante);
            return -1;
        }

        strncpy(comando, buffer, sizeof(comando) - 1);
        comando[sizeof(comando) - 1] = '\0';
        a_minusculas(comando);

        if (strcmp(comando, "si") == 0 || strcmp(comando, "s") == 0 || strcmp(comando, "yes") == 0 || strcmp(comando, "y") == 0)
        {
            bigint_free(&faltante);
            return 1;
        }

        if (strcmp(comando, "no") == 0 || strcmp(comando, "n") == 0)
        {
            bigint_free(&faltante);
            return 0;
        }

        printf("Respuesta invalida. Escriba si o no.\n");
    }
}

static int aplicar_devolucion_stock(const char *monedaClave,
                                    BigIntArray *stock,
                                    const BigIntArray *solucion)
{
    BigIntArray stockNuevo = {0};
    int okStockNuevo;

    if (monedaClave == NULL || stock == NULL || solucion == NULL)
        return 0;

    okStockNuevo = copiar_arreglo_bigint(stock, &stockNuevo);
    if (!okStockNuevo)
        return 0;

    for (size_t i = 0; i < stockNuevo.len; i++)
    {
        BigInt restante = {0};

        if (bigint_is_zero(&solucion->items[i]))
            continue;

        if (!bigint_subtract(&stockNuevo.items[i], &solucion->items[i], &restante) ||
            !bigint_array_set(&stockNuevo, i, &restante))
        {
            bigint_free(&restante);
            okStockNuevo = 0;
            break;
        }

        bigint_free(&restante);
    }

    if (!okStockNuevo)
    {
        limpiar_arreglo(&stockNuevo);
        return 0;
    }

    if (!actualizar_stock_moneda(monedaClave, &stockNuevo))
    {
        limpiar_arreglo(&stockNuevo);
        return 0;
    }

    limpiar_arreglo(stock);
    *stock = stockNuevo;
    return 1;
}

/* imprimir_stock_administrador: Muestra todos los stocks actuales al entrar al modo C. */
static void imprimir_stock_administrador(const BigIntArray *monedas, const BigIntArray *stock)
{
    dibujar_linea();
    printf("Panel Administrador (Consola)\n");
    dibujar_linea();
    for (size_t i = 0; i < monedas->len; i++)
        printf("[%zu] Denom %s c -> stock %s\n", i + 1, monedas->items[i].digits, stock->items[i].digits);
    dibujar_linea();
}

static void mostrar_resumen_moneda(const char *monedaClave, const BigIntArray *monedas, const BigIntArray *stock, int usarStock)
{
    const BigInt *minDenom;
    const BigInt *maxDenom;
    size_t i;

    if (monedas == NULL || monedas->items == NULL || monedas->len == 0)
    {
        printf("No hay denominaciones cargadas para mostrar resumen.\n");
        return;
    }

    minDenom = &monedas->items[0];
    maxDenom = &monedas->items[0];

    for (i = 1; i < monedas->len; i++)
    {
        if (bigint_compare(&monedas->items[i], minDenom) < 0)
            minDenom = &monedas->items[i];
        if (bigint_compare(&monedas->items[i], maxDenom) > 0)
            maxDenom = &monedas->items[i];
    }

    dibujar_linea();
    printf("Resumen de moneda: %s\n", monedaClave != NULL ? monedaClave : "(sin nombre)");
    printf("Denominaciones cargadas: %zu\n", monedas->len);
    printf("Denominacion minima: %s c\n", minDenom->digits);
    printf("Denominacion maxima: %s c\n", maxDenom->digits);

    if (!usarStock)
    {
        printf("Modo stock ilimitado: no se calcula inventario fisico.\n");
        dibujar_linea();
        return;
    }

    if (stock == NULL || stock->items == NULL || stock->len != monedas->len)
    {
        printf("No hay stock valido para calcular resumen de inventario.\n");
        dibujar_linea();
        return;
    }

    {
        BigInt totalPiezas = {0};
        BigInt totalValor = {0};
        int ok = 1;

        if (!bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0"))
            ok = 0;

        for (i = 0; ok && i < monedas->len; i++)
        {
            BigInt nuevoTotalPiezas = {0};
            BigInt parcialValor = {0};
            BigInt nuevoTotalValor = {0};

            if (!bigint_add(&totalPiezas, &stock->items[i], &nuevoTotalPiezas) ||
                !bigint_multiply(&monedas->items[i], &stock->items[i], &parcialValor) ||
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

        if (ok)
        {
            printf("Total de piezas en stock: %s\n", totalPiezas.digits);
            printf("Valor total del stock: %s c\n", totalValor.digits);
            registrar_historialf("Resumen consultado | Moneda=%s | Piezas=%s | Valor=%s c",
                                 monedaClave != NULL ? monedaClave : "(sin nombre)",
                                 totalPiezas.digits,
                                 totalValor.digits);
        }
        else
        {
            printf("No se pudo calcular el resumen de inventario por error interno.\n");
        }

        bigint_free(&totalPiezas);
        bigint_free(&totalValor);
    }

    dibujar_linea();
}

/* aplicar_cambio_administrador: Suma o Resta stock manual a una divisa indicada en 'idx'. */
static int aplicar_cambio_administrador(BigIntArray *stock, size_t idx, const BigInt *delta, int esSuma)
{
    BigInt nuevo = {0};

    /* Bifurca si el usuario pidio agregar monedas (+) o quitar monedas (-). */
    if (esSuma)
    {
        if (!bigint_add(&stock->items[idx], delta, &nuevo))
            return 0;
    }
    else
    {
        if (bigint_compare(&stock->items[idx], delta) < 0)
            return 0;
        if (!bigint_subtract(&stock->items[idx], delta, &nuevo))
            return 0;
    }

    /* Asigna exitosamente al arreglo stock el puntero resultante (se le transfiere su propiedad interna string). */
    if (!bigint_array_set(stock, idx, &nuevo))
    {
        bigint_free(&nuevo);
        return 0;
    }

    bigint_free(&nuevo);
    return 1;
}

/* pedir_subopcion_cambio: Interfaz para sub-opciones de usuario segun el modo activo. */
static int pedir_subopcion_cambio(int permitir_limite)
{
    char buffer[32];
    char comando[32];

    if (permitir_limite)
        printf("Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo o salir): ");
    else
        printf("Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4, volver, modo o salir): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    if (strcmp(comando, "salir") == 0)
        return 4;
    if (strcmp(comando, "modo") == 0)
        return 3;
    if (strcmp(comando, "volver") == 0)
        return 2;
    if (strcmp(comando, "tradicional") == 0 || strcmp(comando, "t") == 0 || strcmp(comando, "1") == 0)
        return 1;
    if (strcmp(comando, "especifico") == 0 || strcmp(comando, "e") == 0 || strcmp(comando, "2") == 0)
        return 5;
    if (strcmp(comando, "historial") == 0 || strcmp(comando, "h") == 0 || strcmp(comando, "3") == 0)
        return 6;
    if (strcmp(comando, "resumen") == 0 || strcmp(comando, "r") == 0 || strcmp(comando, "4") == 0)
        return 7;
    if (permitir_limite && (strcmp(comando, "limite") == 0 || strcmp(comando, "l") == 0 || strcmp(comando, "5") == 0))
        return 8;

    return 0;
}

static int parse_size_t_positivo(const char *texto, size_t *valor_out)
{
    char *fin;
    unsigned long long valor;

    if (texto == NULL || texto[0] == '\0' || valor_out == NULL)
        return 0;

    valor = strtoull(texto, &fin, 10);
    if (*fin != '\0' || valor == 0 || valor > (unsigned long long)((size_t)-1))
        return 0;

    *valor_out = (size_t)valor;
    return 1;
}

static int parse_restriccion_monedas_texto(const char *entrada, size_t *min_monedas, size_t *max_monedas)
{
    char limpio[128];
    size_t i = 0;
    size_t j = 0;

    if (entrada == NULL || min_monedas == NULL || max_monedas == NULL)
        return 0;

    while (entrada[i] != '\0' && j + 1 < sizeof(limpio))
    {
        if (!isspace((unsigned char)entrada[i]))
            limpio[j++] = entrada[i];
        i++;
    }
    limpio[j] = '\0';

    if (limpio[0] == '\0')
        return 0;

    if (limpio[0] == '=')
    {
        size_t exacto;
        if (!parse_size_t_positivo(limpio + 1, &exacto))
            return 0;
        *min_monedas = exacto;
        *max_monedas = exacto;
        return 1;
    }

    {
        char *guion = strchr(limpio, '-');
        if (guion != NULL)
        {
            size_t min_v;
            size_t max_v;
            char izq[64];
            char der[64];
            size_t len_izq = (size_t)(guion - limpio);

            if (len_izq == 0 || len_izq >= sizeof(izq))
                return 0;

            memcpy(izq, limpio, len_izq);
            izq[len_izq] = '\0';
            strncpy(der, guion + 1, sizeof(der) - 1);
            der[sizeof(der) - 1] = '\0';

            if (!parse_size_t_positivo(izq, &min_v) || !parse_size_t_positivo(der, &max_v))
                return 0;
            if (min_v > max_v)
                return 0;

            *min_monedas = min_v;
            *max_monedas = max_v;
            return 1;
        }
    }

    {
        size_t max_v;
        if (!parse_size_t_positivo(limpio, &max_v))
            return 0;
        *min_monedas = 0;
        *max_monedas = max_v;
        return 1;
    }
}

static int pedir_restriccion_monedas(size_t *min_monedas, size_t *max_monedas)
{
    char buffer[128];
    char comando[128];

    printf("Restriccion de monedas (N, =N, N-M, volver, modo o salir): ");
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    if (strcmp(comando, "salir") == 0)
        return 4;
    if (strcmp(comando, "modo") == 0)
        return 3;
    if (strcmp(comando, "volver") == 0)
        return 2;

    if (!parse_restriccion_monedas_texto(buffer, min_monedas, max_monedas))
        return 0;

    return 1;
}

static int calcular_total_valor(const BigIntArray *monedas, const BigIntArray *cantidades, BigInt *total);

static int validar_cambio_especifico_ilimitado(const BigIntArray *monedas,
                                               const BigIntArray *entregadas,
                                               const BigIntArray *devolucion,
                                               BigInt *totalEntregado,
                                               BigInt *totalDevolucion)
{
    if (monedas == NULL || entregadas == NULL || devolucion == NULL ||
        totalEntregado == NULL || totalDevolucion == NULL)
        return 0;

    if (monedas->len != entregadas->len || monedas->len != devolucion->len)
        return 0;

    if (!calcular_total_valor(monedas, entregadas, totalEntregado) ||
        !calcular_total_valor(monedas, devolucion, totalDevolucion))
        return 0;

    return bigint_compare(totalEntregado, totalDevolucion) == 0;
}

/* pedir_cantidades_por_denominacion: Bucle input para introducir cuanto billete de CADA clase estamos operando. */
static int pedir_cantidades_por_denominacion(const BigIntArray *monedas, const char *titulo, BigIntArray *cantidades)
{
    size_t i;

    if (monedas == NULL || monedas->items == NULL || monedas->len == 0 || titulo == NULL || cantidades == NULL)
        return 0;

    if (!bigint_array_create(cantidades, monedas->len))
        return 0;

    printf("%s\n", titulo);
    for (i = 0; i < monedas->len; i++)
    {
        /* Bucle infinito de trampa: Se repite solo si el usuario da entradas corrompidas; hasta que conteste bien o escriba exit. */
        while (1)
        {
            char buffer[2048];
            char comando[2048];
            BigInt cantidad = {0};

            printf("Cantidad para %s c (volver/modo/salir): ", monedas->items[i].digits);
            /* Atrapa la lectura bloqueando con I/O fgets. */
            if (!leer_linea(buffer, sizeof(buffer)))
            {
                limpiar_arreglo(cantidades);
                return -1;
            }

            /* Si no ingreso un carajo. */
            if (buffer[0] == '\0')
            {
                printf("Entrada vacia. Intente de nuevo.\n");
                continue;
            }

            strncpy(comando, buffer, sizeof(comando) - 1);
            comando[sizeof(comando) - 1] = '\0';
            a_minusculas(comando);

            /* Salida de emergencia 1. */
            if (strcmp(comando, "salir") == 0)
            {
                limpiar_arreglo(cantidades);
                return 4;
            }
            /* Salida emergencia 2. */
            if (strcmp(comando, "modo") == 0)
            {
                limpiar_arreglo(cantidades);
                return 3;
            }
            /* Salida de emergencia 3. */
            if (strcmp(comando, "volver") == 0)
            {
                limpiar_arreglo(cantidades);
                return 2;
            }

            /* Trata de inyectar el valor del texto dentro de la maquinaria BigInt. */
            if (!bigint_init(&cantidad, buffer))
            {
                printf("Cantidad invalida. Debe ser entero no negativo.\n");
                continue;
            }

            /* Incorpora con deep copy el objeto bigint que si sirvio dentro del espacio correcto `i`. */
            if (!bigint_array_set(cantidades, i, &cantidad))
            {
                bigint_free(&cantidad);
                limpiar_arreglo(cantidades);
                return 0;
            }

            bigint_free(&cantidad);
            break;
        }
    }

    return 1;
}

/* calcular_total_valor: Multiplica el vector unitario (valores faciales) con el escalar unitario (unidades). Total acumulado. */
static int calcular_total_valor(const BigIntArray *monedas, const BigIntArray *cantidades, BigInt *total)
{
    BigInt acumulado = {0};

    if (monedas == NULL || cantidades == NULL || total == NULL)
        return 0;
    if (monedas->len != cantidades->len)
        return 0;

    if (!bigint_init(&acumulado, "0"))
        return 0;

    for (size_t i = 0; i < monedas->len; i++)
    {
        BigInt parcial = {0};
        BigInt nuevoTotal = {0};

        if (bigint_is_zero(&cantidades->items[i]))
            continue;

        /* Ejecuta calculo del termino (Valor*Cant) en objeto 'parcial'. */
        if (!bigint_multiply(&monedas->items[i], &cantidades->items[i], &parcial))
        {
            bigint_free(&acumulado);
            return 0;
        }

        /* Suma el parcial (Ej: $500) a lo que teniamos en total 'acumulado'. */
        if (!bigint_add(&acumulado, &parcial, &nuevoTotal))
        {
            bigint_free(&parcial);
            bigint_free(&acumulado);
            return 0;
        }

        bigint_free(&parcial);
        bigint_free(&acumulado);
        acumulado = nuevoTotal;
    }

    bigint_free(total);
    *total = acumulado;
    return 1;
}

static int aplicar_cambio_especifico_stock(const BigIntArray *monedas,
                                           const BigIntArray *stockActual,
                                           const BigIntArray *entregadas,
                                           const BigIntArray *devolucion,
                                           BigIntArray *stockNuevo,
                                           BigInt *totalEntregado,
                                           BigInt *totalDevolucion)
{
    if (monedas == NULL || stockActual == NULL || entregadas == NULL || devolucion == NULL ||
        stockNuevo == NULL || totalEntregado == NULL || totalDevolucion == NULL)
        return 0;

    if (monedas->len != stockActual->len || monedas->len != entregadas->len || monedas->len != devolucion->len)
        return 0;

    if (!calcular_total_valor(monedas, entregadas, totalEntregado) ||
        !calcular_total_valor(monedas, devolucion, totalDevolucion))
        return 0;

    if (bigint_compare(totalEntregado, totalDevolucion) != 0)
        return 0;

    if (!copiar_arreglo_bigint(stockActual, stockNuevo))
        return 0;

    for (size_t i = 0; i < stockNuevo->len; i++)
    {
        BigInt trasEntrada = {0};
        BigInt trasSalida = {0};

        /* Agrega billetes: StockCajon_i = StockCajon_i + BilletesUsuarioEntrega_i */
        if (!bigint_add(&stockNuevo->items[i], &entregadas->items[i], &trasEntrada))
        {
            limpiar_arreglo(stockNuevo);
            return 0;
        }

        /* Remueve billetes: chequear si existe la cantidad fisica en el cajon primero. Stock = StockCajon_i - BilletesNosPiden_i. */
        if (bigint_compare(&trasEntrada, &devolucion->items[i]) < 0 ||
            !bigint_subtract(&trasEntrada, &devolucion->items[i], &trasSalida))
        {
            bigint_free(&trasEntrada);
            limpiar_arreglo(stockNuevo);
            return 0;
        }

        /* Escribe el resorte final tras entrada y salida al cajon `stockNuevo` en su slot. */
        if (!bigint_array_set(stockNuevo, i, &trasSalida))
        {
            bigint_free(&trasEntrada);
            bigint_free(&trasSalida);
            limpiar_arreglo(stockNuevo);
            return 0;
        }

        bigint_free(&trasEntrada);
        bigint_free(&trasSalida);
    }

    return 1;
}

/* flujo_conversion_interactiva: Flujo interactivo que pide moneda origen, monto,
 * moneda destino, obtiene la tasa desde la API y calcula el cambio en la moneda
 * destino (usando stock o modo ilimitado segun seleccione el usuario). */
static int flujo_conversion_interactiva(void)
{
    char origen[MAX_MONEDA_NOMBRE + 1];
    char destino[MAX_MONEDA_NOMBRE + 1];
    char claveOrigen[MAX_MONEDA_NOMBRE + 1];
    char claveDestino[MAX_MONEDA_NOMBRE + 1];
    char linea[64];
    BigInt cantidad = {0};
    BigInt cantidadDestino = {0};
    BigIntArray monedas = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};
    int estado;
    int usarStock = 0;
    double tasa = 0.0;

    printf("--- Conversion entre monedas (API) ---\n");
    printf("Moneda origen: ");
    if (!leer_linea(origen, sizeof(origen)))
        return 0;
    if (origen[0] == '\0')
    {
        printf("Moneda origen vacia.\n");
        return 0;
    }
    normalizar_clave(origen, claveOrigen, sizeof(claveOrigen));

    estado = pedir_cantidad(&cantidad);
    if (estado == -1)
    {
        printf("Entrada finalizada.\n");
        bigint_free(&cantidad);
        return 0;
    }
    if (estado == 2 || estado == 3)
    {
        bigint_free(&cantidad);
        return 0;
    }
    if (estado == 0)
    {
        printf("Cantidad invalida.\n");
        bigint_free(&cantidad);
        return 0;
    }

    printf("Moneda destino: ");
    if (!leer_linea(destino, sizeof(destino)))
    {
        bigint_free(&cantidad);
        return 0;
    }
    if (destino[0] == '\0')
    {
        printf("Moneda destino vacia.\n");
        bigint_free(&cantidad);
        return 0;
    }
    normalizar_clave(destino, claveDestino, sizeof(claveDestino));

    printf("Usar stock de destino? (s/n): ");
    if (!leer_linea(linea, sizeof(linea)))
    {
        bigint_free(&cantidad);
        return 0;
    }
    a_minusculas(linea);
    if (linea[0] == 's' || linea[0] == 'y')
        usarStock = 1;

    if (!fetch_exchange_rate(claveOrigen, claveDestino, &tasa))
    {
        printf("No se pudo obtener la tasa de cambio para %s -> %s\n", claveOrigen, claveDestino);
        bigint_free(&cantidad);
        return 0;
    }

    /* Convertir cantidad (en centimos) multiplicando por la tasa. */
    {
        double src_cents = strtod(cantidad.digits, NULL);
        double dst_cents = src_cents * tasa;
        long long rounded = (long long)(dst_cents + 0.5);
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%lld", rounded);
        if (!bigint_init(&cantidadDestino, tmp))
        {
            printf("Error interno al convertir monto destino.\n");
            bigint_free(&cantidad);
            return 0;
        }
    }

    /* Cargar denominaciones destino. */
    if (!validar_consistencia_moneda(claveDestino))
    {
        printf("Moneda destino inconsistente o no encontrada.\n");
        goto cleanup;
    }

    if (!cargar_denominaciones_moneda(claveDestino, &monedas))
    {
        printf("No se encontraron denominaciones para la moneda destino.\n");
        goto cleanup;
    }

    if (usarStock)
    {
        if (!cargar_stock_moneda(claveDestino, &stock))
        {
            printf("No se encontro stock para la moneda destino.\n");
            goto cleanup;
        }
    }

    if (usarStock)
        estado = calcular_cambio_optimo_stock(&cantidadDestino, &monedas, &stock, &solucion);
    else
        estado = calcular_cambio_optimo(&cantidadDestino, &monedas, &solucion);

    if (estado)
    {
        printf("Conversion aplicada: %s %s-centimos -> %s %s-centimos | tasa=%.6f\n",
               cantidad.digits, claveOrigen, cantidadDestino.digits, claveDestino, tasa);
        imprimir_resultado(&monedas, &solucion, usarStock ? &stock : NULL, usarStock ? 1 : 0);
        registrar_historialf("Conversion %s->%s | Origen=%s c | Destino=%s c | Tasa=%f",
                             claveOrigen,
                             claveDestino,
                             cantidad.digits,
                             cantidadDestino.digits,
                             tasa);
    }
    else
    {
        printf("No se pudo calcular el cambio en la moneda destino.\n");
    }

cleanup:
    limpiar_arreglo(&monedas);
    limpiar_arreglo(&stock);
    limpiar_arreglo(&solucion);
    bigint_free(&cantidad);
    bigint_free(&cantidadDestino);
    return 1;
}

/* app_console_run: Ejecuta la aplicación de consola completa. */
int app_console_run(void)
{
    char moneda[MAX_MONEDA_NOMBRE + 1];
    char monedaClave[MAX_MONEDA_NOMBRE + 1];
    char monedaCmd[MAX_MONEDA_NOMBRE + 1];
    int opcion = 0;
    int ejecutando = 1;
    BigIntArray monedas = {0};
    BigIntArray stock = {0};

    /* Bucle AppLoop principal. Mantiene la app viva hasta un Ctrl+C o salida intencionada. */
    while (ejecutando)
    {
        /* Si no hay una opcion seleccionada, estamos en la pantalla inicial de Menu. */
        if (opcion != 'a' && opcion != 'b' && opcion != 'c')
        {
            /* Bucle de sub-menu inicial de captura de modalidad. */
            while (1)
            {
                opcion = pedir_opcion();
                if (opcion == 'a' || opcion == 'b' || opcion == 'c' || opcion == 'x')
                    break;

                if (opcion == -3)
                {
                    mostrar_historial_transacciones();
                    pausar_pantalla();
                    continue;
                }

                if (opcion == -4)
                {
                    mostrar_resumen_desde_menu_principal();
                    pausar_pantalla();
                    continue;
                }

                if (opcion == -5)
                {
                    gestionar_operacion_global("snapshot");
                    pausar_pantalla();
                    continue;
                }

                if (opcion == -6)
                {
                    gestionar_operacion_global("restaurar");
                    pausar_pantalla();
                    continue;
                }

                if (opcion == -7)
                {
                    gestionar_operacion_global("reporte");
                    pausar_pantalla();
                    continue;
                }

                /* Si devolvió el magik number -2 de salida manual. */
                if (opcion == -2)
                {
                    ejecutando = 0;
                    break;
                }

                /* Si hubo desconexion de standard input (EOF). */
                if (opcion == -1)
                {
                    printf("Entrada finalizada.\n");
                    ejecutando = 0;
                    break;
                }

                printf("Opcion no valida. Intente de nuevo.\n");
            }
        }

        if (!ejecutando)
            break;

        /* Manejo de opcion especial: conversion entre monedas. */
        if (opcion == 'x')
        {
            /* Ejecuta flujo interactivo de conversion y vuelve al menu principal. */
            flujo_conversion_interactiva();
            pausar_pantalla();
            opcion = 0;
            continue;
        }

        /* Bucle de Sesion de Trabajo (Donde se opera con monedas repetidamente). */
        while (1)
        {
            limpiar_arreglo(&stock);
            limpiar_arreglo(&monedas);

            mostrar_monedas_disponibles(opcion);
            printf("Nombre de la moneda ('modo', 'volver' o 'salir'): ");

            /* Espera I/O bloqueante. */
            if (!leer_linea(moneda, sizeof(moneda)))
            {
                printf("Entrada finalizada.\n");
                ejecutando = 0;
                break;
            }

            /* Si no escribe nada... */
            if (moneda[0] == '\0')
            {
                printf("Nombre de moneda no valido. Intente de nuevo.\n");
                continue;
            }

            normalizar_clave(moneda, monedaCmd, sizeof(monedaCmd));
            /* Evalua si el comando fue orden de fin total. */
            if (strcmp(monedaCmd, "salir") == 0)
            {
                ejecutando = 0;
                break;
            }

            /* Evalua si el usuario pidio cambiar de modo (Ej. Pasar del Ilimitado al Limitado). */
            if (strcmp(monedaCmd, "modo") == 0 || strcmp(monedaCmd, "volver") == 0)
            {
                opcion = 0;
                break;
            }

            normalizar_clave(moneda, monedaClave, sizeof(monedaClave));

            if (!validar_consistencia_moneda(monedaClave))
            {
                printf("Moneda no valida o inconsistente (denominaciones/stock). Intente de nuevo.\n");
                continue;
            }

            if (!cargar_denominaciones_moneda(monedaClave, &monedas))
            {
                printf("No se encontro la moneda en monedas.txt. Intente de nuevo.\n");
                continue;
            }

            if (opcion == 'b' || opcion == 'c')
            {
                if (!cargar_stock_moneda(monedaClave, &stock))
                {
                    limpiar_arreglo(&monedas);
                    printf("No se encontro stock para la moneda seleccionada. Intente de nuevo.\n");
                    continue;
                }
            }

            /* Bucle de Sub-Operacion Activa (Ya con las DB cargadas en memoria y modo listo). */
            while (1)
            {
                /* Caso MODO ADMINISTRADOR (Opcion C). */
                if (opcion == 'c')
                {
                    char accion[32];
                    int esSuma;
                    size_t idxDenom;
                    int estadoIndice;
                    BigInt delta = {0};
                    int estadoDelta;

                    imprimir_stock_administrador(&monedas, &stock);
                    printf("Accion admin (anadir/quitar/historial/resumen, volver, modo, salir): ");

                    /* Bloqueo en consola. */
                    if (!leer_linea(accion, sizeof(accion)))
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }

                    a_minusculas(accion);
                    /* Chequeo de keywords para la accion general. */
                    if (strcmp(accion, "salir") == 0)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Chequeo keywords sub_ui. */
                    if (strcmp(accion, "modo") == 0)
                    {
                        opcion = 0;
                        break;
                    }
                    if (strcmp(accion, "volver") == 0)
                        break;

                    if (strcmp(accion, "historial") == 0)
                    {
                        mostrar_historial_transacciones();
                        continue;
                    }

                    if (strcmp(accion, "resumen") == 0)
                    {
                        mostrar_resumen_moneda(monedaClave, &monedas, &stock, 1);
                        continue;
                    }

                    /* Set de banderillas de orden. */
                    if (strcmp(accion, "anadir") == 0)
                        esSuma = 1;
                    else if (strcmp(accion, "quitar") == 0)
                        esSuma = 0;
                    else
                    {
                        printf("Accion invalida.\n");
                        continue;
                    }

                    estadoIndice = pedir_indice_denominacion(monedas.len, &idxDenom);
                    /* Switch de validacion del output magik de la UI: */
                    if (estadoIndice == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* Si pidio salir... */
                    if (estadoIndice == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Si pidio salir a UI 1. */
                    if (estadoIndice == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    if (estadoIndice == 2)
                        continue;
                    /* Si lo que introdujo ni siquiera fue un numero parseable... */
                    if (estadoIndice == 0)
                    {
                        printf("Indice invalido.\n");
                        continue;
                    }

                    estadoDelta = pedir_cantidad_admin(&delta);
                    /* Mismas evaluaciones de salida temprana por keyword... */
                    if (estadoDelta == -1)
                    {
                        printf("Entrada finalizada.\n");
                        bigint_free(&delta);
                        ejecutando = 0;
                        break;
                    }
                    /* Validacion salida. */
                    if (estadoDelta == 4)
                    {
                        bigint_free(&delta);
                        ejecutando = 0;
                        break;
                    }
                    /* Validacion menu 1. */
                    if (estadoDelta == 3)
                    {
                        bigint_free(&delta);
                        opcion = 0;
                        break;
                    }
                    /* Validacion menu divisas. */
                    if (estadoDelta == 2)
                    {
                        bigint_free(&delta);
                        continue;
                    }
                    /* Validacion por basura NaN. */
                    if (estadoDelta == 0)
                    {
                        bigint_free(&delta);
                        printf("Cantidad invalida.\n");
                        continue;
                    }

                    /* Efectua in-memory la alteracion al array del stock activo. */
                    if (!aplicar_cambio_administrador(&stock, idxDenom, &delta, esSuma))
                    {
                        bigint_free(&delta);
                        printf("No se pudo aplicar el cambio de stock (revisa disponibilidad).\n");
                        continue;
                    }

                    /* Consolida finalmente el array recien modificado en RAM hacia el Disco Duro. */
                    if (!actualizar_stock_moneda(monedaClave, &stock))
                        printf("No se pudo actualizar el archivo de stock.\n");
                    else
                    {
                        registrar_historialf("Admin %s | Moneda=%s | Denom=%s c | Cantidad=%s",
                                             esSuma ? "ANADIR" : "QUITAR",
                                             monedaClave,
                                             monedas.items[idxDenom].digits,
                                             delta.digits);
                        printf("Stock actualizado correctamente.\n");
                    }

                    bigint_free(&delta);
                    continue;
                }

                BigInt cantidad = {0};
                int estadoCantidad;
                int estadoRestriccion;
                int resultado;
                BigIntArray solucion = {0};
                int subopcionStock = 1;
                int usar_restriccion = 0;
                size_t min_monedas = 0;
                size_t max_monedas = 0;
                size_t min_cercano = 0;
                size_t max_cercano = (size_t)-1;

                /* Caso MODO USUARIO NORMAL (Opciones A y B). */
                if (opcion == 'a' || opcion == 'b')
                {
                    subopcionStock = pedir_subopcion_cambio(opcion == 'b');

                    /* Validacion estricta magiks menu... */
                    if (subopcionStock == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* Valida magiks.. */
                    if (subopcionStock == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Valida.. */
                    if (subopcionStock == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    if (subopcionStock == 2)
                        break;
                    /* Valida fallo. */
                    if (subopcionStock == 0)
                    {
                        printf("Subopcion invalida.\n");
                        continue;
                    }

                    if (subopcionStock == 6)
                    {
                        mostrar_historial_transacciones();
                        continue;
                    }

                    if (subopcionStock == 7)
                    {
                        mostrar_resumen_moneda(monedaClave, &monedas, opcion == 'b' ? &stock : NULL, opcion == 'b');
                        continue;
                    }
                }

                /* Flujo ESPECIAL: Modo Usuario Infinito + Variante Intercambio Multi-billete. */
                if (opcion == 'a' && subopcionStock == 5)
                {
                    BigIntArray entregadas = {0};
                    BigIntArray devolucion = {0};
                    BigInt totalEntregado = {0};
                    BigInt totalDevolucion = {0};
                    int estadoEntrada;
                    int estadoDevolucion;

                    estadoEntrada = pedir_cantidades_por_denominacion(&monedas, "Monedas/Billetes entregados por el usuario:", &entregadas);
                    /* Chequeos de magiks... */
                    if (estadoEntrada == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* Exit... */
                    if (estadoEntrada == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Menu app. */
                    if (estadoEntrada == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    if (estadoEntrada == 2)
                        continue;
                    /* Fallo genérico. */
                    if (estadoEntrada == 0)
                    {
                        printf("No se pudieron leer cantidades entregadas.\n");
                        continue;
                    }

                    estadoDevolucion = pedir_cantidades_por_denominacion(&monedas, "Cambio especifico solicitado (cantidades a devolver):", &devolucion);
                    /* Chequeo magiks... */
                    if (estadoDevolucion == -1)
                    {
                        printf("Entrada finalizada.\n");
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* Exit. */
                    if (estadoDevolucion == 4)
                    {
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* App. */
                    if (estadoDevolucion == 3)
                    {
                        limpiar_arreglo(&entregadas);
                        opcion = 0;
                        break;
                    }
                    /* Volver. */
                    if (estadoDevolucion == 2)
                    {
                        limpiar_arreglo(&entregadas);
                        continue;
                    }
                    /* Error. */
                    if (estadoDevolucion == 0)
                    {
                        printf("No se pudieron leer cantidades de cambio solicitado.\n");
                        limpiar_arreglo(&entregadas);
                        continue;
                    }

                    /* Validador de consistencia monetaria pura (Total Depositado == Total Exigido). */
                    if (!validar_cambio_especifico_ilimitado(&monedas, &entregadas, &devolucion, &totalEntregado, &totalDevolucion))
                    {
                        printf("No se pudo aplicar cambio especifico en modo ilimitado. Verifica que el total entregado sea igual al total solicitado.\n");
                        limpiar_arreglo(&entregadas);
                        limpiar_arreglo(&devolucion);
                        bigint_free(&totalEntregado);
                        bigint_free(&totalDevolucion);
                        continue;
                    }

                    printf("Cambio especifico aplicado en modo ilimitado. Total entregado: %s c | Total devuelto: %s c\n",
                           totalEntregado.digits, totalDevolucion.digits);
                    imprimir_resultado(&monedas, &devolucion, NULL, 0);
                    registrar_historialf("Cambio especifico ilimitado | Moneda=%s | Total=%s c",
                                         monedaClave,
                                         totalEntregado.digits);

                    limpiar_arreglo(&entregadas);
                    limpiar_arreglo(&devolucion);
                    bigint_free(&totalEntregado);
                    bigint_free(&totalDevolucion);
                    continue;
                }

                /* Flujo ESPECIAL: Modo Usuario Limitado + Variante Intercambio Multi-billete. */
                if (opcion == 'b' && subopcionStock == 5)
                {
                    BigIntArray entregadas = {0};
                    BigIntArray devolucion = {0};
                    BigIntArray stockNuevo = {0};
                    BigInt totalEntregado = {0};
                    BigInt totalDevolucion = {0};
                    int estadoEntrada;
                    int estadoDevolucion;

                    estadoEntrada = pedir_cantidades_por_denominacion(&monedas, "Monedas/Billetes entregados por el usuario:", &entregadas);
                    /* Magiks... */
                    if (estadoEntrada == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    if (estadoEntrada == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    if (estadoEntrada == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    if (estadoEntrada == 2)
                        continue;
                    /* .. */
                    if (estadoEntrada == 0)
                    {
                        printf("No se pudieron leer cantidades entregadas.\n");
                        continue;
                    }

                    estadoDevolucion = pedir_cantidades_por_denominacion(&monedas, "Cambio especifico solicitado (cantidades a devolver):", &devolucion);
                    /* Magiks y purgados condicionales por cada break... */
                    if (estadoDevolucion == -1)
                    {
                        printf("Entrada finalizada.\n");
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    if (estadoDevolucion == 4)
                    {
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    if (estadoDevolucion == 3)
                    {
                        limpiar_arreglo(&entregadas);
                        opcion = 0;
                        break;
                    }
                    /* .. */
                    if (estadoDevolucion == 2)
                    {
                        limpiar_arreglo(&entregadas);
                        continue;
                    }
                    /* .. */
                    if (estadoDevolucion == 0)
                    {
                        printf("No se pudieron leer cantidades de cambio solicitado.\n");
                        limpiar_arreglo(&entregadas);
                        continue;
                    }

                    /* Ejecuta rutina compleja de validacion atomica (Balance total == Balance Devuelto AND StockFisico >= Billetes Devueltos). */
                    if (!aplicar_cambio_especifico_stock(&monedas, &stock, &entregadas, &devolucion, &stockNuevo, &totalEntregado, &totalDevolucion))
                    {
                        printf("No se pudo aplicar cambio especifico. Verifica que el total entregado sea igual al total solicitado y que el stock alcance.\n");
                        limpiar_arreglo(&entregadas);
                        limpiar_arreglo(&devolucion);
                        limpiar_arreglo(&stockNuevo);
                        bigint_free(&totalEntregado);
                        bigint_free(&totalDevolucion);
                        continue;
                    }

                    /* Intenta commitear el cambio al archivo de texto fisico en disco duro. */
                    if (!actualizar_stock_moneda(monedaClave, &stockNuevo))
                    {
                        printf("No se pudo actualizar el archivo de stock.\n");
                        limpiar_arreglo(&entregadas);
                        limpiar_arreglo(&devolucion);
                        limpiar_arreglo(&stockNuevo);
                        bigint_free(&totalEntregado);
                        bigint_free(&totalDevolucion);
                        continue;
                    }

                    limpiar_arreglo(&stock);
                    stock = stockNuevo;

                    printf("Cambio especifico aplicado. Total entregado: %s c | Total devuelto: %s c\n",
                           totalEntregado.digits, totalDevolucion.digits);
                    imprimir_resultado(&monedas, &devolucion, &stock, 1);
                    registrar_historialf("Cambio especifico con stock | Moneda=%s | Total=%s c",
                                         monedaClave,
                                         totalEntregado.digits);

                    limpiar_arreglo(&entregadas);
                    limpiar_arreglo(&devolucion);
                    bigint_free(&totalEntregado);
                    bigint_free(&totalDevolucion);
                    continue;
                }

                estadoCantidad = pedir_cantidad(&cantidad);
                /* Validaciones magiks de siempre... */
                if (estadoCantidad == -1)
                {
                    printf("Entrada finalizada.\n");
                    bigint_free(&cantidad);
                    ejecutando = 0;
                    break;
                }

                /* Magiks.. */
                if (estadoCantidad == 2)
                {
                    bigint_free(&cantidad);
                    break;
                }

                /* Magiks.. */
                if (estadoCantidad == 3)
                {
                    bigint_free(&cantidad);
                    ejecutando = 0;
                    break;
                }

                /* Magiks.. */
                if (estadoCantidad == 0)
                {
                    printf("Entrada invalida. Introduzca un entero no negativo.\n");
                    bigint_free(&cantidad);
                    continue;
                }

                usar_restriccion = (subopcionStock == 8);
                if (usar_restriccion)
                {
                    estadoRestriccion = pedir_restriccion_monedas(&min_monedas, &max_monedas);

                    if (estadoRestriccion == -1)
                    {
                        printf("Entrada finalizada.\n");
                        bigint_free(&cantidad);
                        ejecutando = 0;
                        break;
                    }
                    if (estadoRestriccion == 4)
                    {
                        bigint_free(&cantidad);
                        ejecutando = 0;
                        break;
                    }
                    if (estadoRestriccion == 3)
                    {
                        bigint_free(&cantidad);
                        opcion = 0;
                        break;
                    }
                    if (estadoRestriccion == 2)
                    {
                        bigint_free(&cantidad);
                        continue;
                    }
                    if (estadoRestriccion == 0)
                    {
                        printf("Restriccion invalida. Use N, =N o N-M.\n");
                        bigint_free(&cantidad);
                        continue;
                    }

                    min_cercano = min_monedas;
                    max_cercano = max_monedas;
                }

                /* Si estamos en modo de monedas ilimitado (El Algoritmo Clásico Original). */
                if (opcion == 'a')
                {
                    if (usar_restriccion)
                        resultado = calcular_cambio_optimo_con_rango(&cantidad, &monedas, min_monedas, max_monedas, &solucion);
                    else
                        resultado = calcular_cambio_optimo(&cantidad, &monedas, &solucion);

                    if (resultado)
                    {
                        imprimir_resultado(&monedas, &solucion, NULL, 0);
                        if (usar_restriccion)
                            registrar_historialf("Cambio con restriccion ilimitado | Moneda=%s | Monto=%s c | Restriccion=%zu-%zu",
                                                 monedaClave,
                                                 cantidad.digits,
                                                 min_monedas,
                                                 max_monedas);
                        else
                            registrar_historialf("Cambio tradicional ilimitado | Moneda=%s | Monto=%s c",
                                                 monedaClave,
                                                 cantidad.digits);
                    }
                    else
                    {
                        BigInt montoCubierto = {0};
                        BigIntArray sugerencia = {0};
                        int decision;
                        int okCercano = calcular_cambio_cercano_con_rango(&cantidad,
                                                                          &monedas,
                                                                          min_cercano,
                                                                          max_cercano,
                                                                          &montoCubierto,
                                                                          &sugerencia);

                        if (okCercano)
                        {
                            decision = preguntar_aceptar_sugerencia(&cantidad, &montoCubierto, 0);

                            if (decision == -1)
                            {
                                printf("Entrada finalizada.\n");
                                bigint_free(&montoCubierto);
                                limpiar_arreglo(&sugerencia);
                                bigint_free(&cantidad);
                                ejecutando = 0;
                                break;
                            }

                            if (decision == 1)
                            {
                                imprimir_sugerencia_cercana(&cantidad, &montoCubierto, &monedas, &sugerencia, 0);
                                registrar_historialf("Cambio cercano aceptado ilimitado | Moneda=%s | Solicitado=%s c | Cubierto=%s c",
                                                     monedaClave,
                                                     cantidad.digits,
                                                     montoCubierto.digits);
                            }
                            else
                            {
                                printf("Operacion cancelada: no se aplico cambio.\n");
                            }

                            bigint_free(&montoCubierto);
                            limpiar_arreglo(&sugerencia);
                        }
                        else
                        {
                            if (usar_restriccion)
                                printf("No existe cambio exacto para esa cantidad con la restriccion indicada.\n");
                            else
                                printf("No existe cambio exacto para esa cantidad.\n");
                        }
                    }
                }
                else
                {
                    if (usar_restriccion)
                        resultado = calcular_cambio_optimo_stock_con_rango(&cantidad, &monedas, &stock, min_monedas, max_monedas, &solucion);
                    else
                        resultado = calcular_cambio_optimo_stock(&cantidad, &monedas, &stock, &solucion);

                    /* Si encontro una respuesta viable. */
                    if (resultado)
                    {
                        if (!aplicar_devolucion_stock(monedaClave, &stock, &solucion))
                        {
                            printf("No se pudo aplicar la devolucion al stock.\n");
                            limpiar_arreglo(&solucion);
                            bigint_free(&cantidad);
                            continue;
                        }

                        imprimir_resultado(&monedas, &solucion, &stock, 1);
                        if (usar_restriccion)
                            registrar_historialf("Cambio con restriccion con stock | Moneda=%s | Monto=%s c | Restriccion=%zu-%zu",
                                                 monedaClave,
                                                 cantidad.digits,
                                                 min_monedas,
                                                 max_monedas);
                        else
                            registrar_historialf("Cambio tradicional con stock | Moneda=%s | Monto=%s c",
                                                 monedaClave,
                                                 cantidad.digits);
                    }
                    else
                    {
                        BigInt montoCubierto = {0};
                        BigIntArray sugerencia = {0};
                        int decision;
                        int okCercano = calcular_cambio_cercano_stock_con_rango(&cantidad,
                                                                                &monedas,
                                                                                &stock,
                                                                                min_cercano,
                                                                                max_cercano,
                                                                                &montoCubierto,
                                                                                &sugerencia);

                        if (okCercano)
                        {
                            decision = preguntar_aceptar_sugerencia(&cantidad, &montoCubierto, 1);

                            if (decision == -1)
                            {
                                printf("Entrada finalizada.\n");
                                bigint_free(&montoCubierto);
                                limpiar_arreglo(&sugerencia);
                                bigint_free(&cantidad);
                                ejecutando = 0;
                                break;
                            }

                            if (decision == 1)
                            {
                                if (!aplicar_devolucion_stock(monedaClave, &stock, &sugerencia))
                                {
                                    printf("No se pudo aplicar la sugerencia cercana al stock.\n");
                                }
                                else
                                {
                                    imprimir_sugerencia_cercana(&cantidad, &montoCubierto, &monedas, &sugerencia, 1);
                                    imprimir_resultado(&monedas, &sugerencia, &stock, 1);
                                    registrar_historialf("Cambio cercano aceptado con stock | Moneda=%s | Solicitado=%s c | Cubierto=%s c",
                                                         monedaClave,
                                                         cantidad.digits,
                                                         montoCubierto.digits);
                                }
                            }
                            else
                            {
                                printf("Operacion cancelada: no se aplico cambio.\n");
                            }

                            bigint_free(&montoCubierto);
                            limpiar_arreglo(&sugerencia);
                        }
                        else
                        {
                            if (usar_restriccion)
                                printf("No existe cambio exacto para esa cantidad con el stock actual y la restriccion indicada.\n");
                            else
                                printf("No existe cambio exacto para esa cantidad con el stock actual.\n");
                        }
                    }
                }

                limpiar_arreglo(&solucion);
                bigint_free(&cantidad);
            }

            if (!ejecutando)
                break;

            /* Vuelve a seleccionar moneda dentro del mismo modo. */
            break;
        }

        if (!ejecutando)
            break;

        /* Si opcion se reinicio a 0, el usuario pidio cambiar de modo. */
        if (opcion == 0)
            continue;
    }

    limpiar_arreglo(&stock);
    limpiar_arreglo(&monedas);
    printf("Gracias por utilizar este programa.\n");
    return EXIT_SUCCESS;
}

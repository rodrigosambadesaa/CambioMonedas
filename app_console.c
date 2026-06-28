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
#include "gui_launcher.h"
#include "json_io.h"
#include "progvoraz_locale.h"

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

/* forward prototypes: used by public wrapper below */
static void dibujar_linea(void);
static void imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock);
static void a_minusculas(char *texto);
#define TR(es, en) progvoraz_tr((es), (en))
/* Implementation moved here so wrapper can call it without forward-declaration issues. */
/* funcion imprimir_resultado: contiene la logica principal de esta operacion. */
static void imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock)
{
    const char *json_env = getenv("PROGVORAZ_JSON");
    int json_mode = json_env && json_env[0] != '\0';

    /* if: comprueba json_mode antes de ejecutar esta rama. */
    if (json_mode)
    {
        /* Salida JSON compacta para integración */
        printf("{\"resultado\": [");
        /* for: itera segun size_t i = 0; i < monedas->len; i++ para recorrer el bloque. */
        for (size_t i = 0; i < monedas->len; i++)
        {
            /* if: comprueba i > 0 antes de ejecutar esta rama. */
            if (i > 0)
                printf(",");
            printf("{\"denominacion\": \"%s\", \"cantidad\": \"%s\"",
                   monedas->items[i].digits, solucion->items[i].digits);
            /* if: comprueba usarStock && stock antes de ejecutar esta rama. */
            if (usarStock && stock)
                printf(", \"stock\": \"%s\"", stock->items[i].digits);
            printf("}");
        }
        printf("]}\n");
        return;
    }

    dibujar_linea();
    printf("%s\n", TR("Resultado del cambio", "Change result"));
    dibujar_linea();

    /* for: itera segun size_t i = 0; i < monedas->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < monedas->len; i++)
    {
        /* Si el modo de ejecucion estaba en 'usarStock' (Modo B o Caja)... */
        /* if: comprueba usarStock antes de ejecutar esta rama. */
        if (usarStock)
            printf("%s %s c  -> %s %s | stock %s\n", TR("Moneda", "Coin"), monedas->items[i].digits, TR("cantidad", "amount"), solucion->items[i].digits, stock->items[i].digits);
        else
            printf("%s %s c  -> %s %s\n", TR("Moneda", "Coin"), monedas->items[i].digits, TR("cantidad", "amount"), solucion->items[i].digits);
    }

    dibujar_linea();
}
/* Public wrapper: expone la impresion de resultados a otros modulos. */
/* funcion app_console_imprimir_resultado: contiene la logica principal de esta operacion. */
void app_console_imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock)
{
    imprimir_resultado(monedas, solucion, stock, usarStock);
}

/* funcion registrar_historialf: contiene la logica principal de esta operacion. */
static void registrar_historialf(const char *fmt, ...)
{
    char mensaje[1024];
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
    char linea[512];

    printf("\n--- %s ---\n", TR("HISTORIAL DE TRANSACCIONES", "TRANSACTION HISTORY"));
    /* if: comprueba fp == NULL antes de ejecutar esta rama. */
    if (fp == NULL)
    {
        printf("%s\n", TR("No hay transacciones registradas.", "There are no recorded transactions."));
        printf("----------------------------------\n\n");
        return;
    }

    /* while: repite el bloque mientras se cumpla fgets(linea, sizeof(linea), fp) != NULL. */
    while (fgets(linea, sizeof(linea), fp) != NULL)
        printf("%s", linea);

    fclose(fp);
    printf("----------------------------------\n\n");
}

/* funcion pausar_pantalla: contiene la logica principal de esta operacion. */
static void pausar_pantalla(void)
{
    char buffer[8];

    printf("%s", TR("Presione Enter para continuar...", "Press Enter to continue..."));
    /* if: comprueba fgets(buffer, (int)sizeof(buffer), stdin) == NULL antes de ejecutar esta rama. */
    if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL)
        clearerr(stdin);
}

#define MAX_MONEDA_NOMBRE 20
#define MAX_MONEDAS_DISPONIBLES 512

void limpiar_arreglo(BigIntArray *arr);
static void mostrar_resumen_moneda(const char *monedaClave, const BigIntArray *monedas, const BigIntArray *stock, int usarStock);

/* limpiar_arreglo: libera recursos asociados a un BigIntArray (wrapper sobre bigint_array_free). */
/* funcion limpiar_arreglo: contiene la logica principal de esta operacion. */
void limpiar_arreglo(BigIntArray *arr)
{
    /* if: comprueba arr == NULL antes de ejecutar esta rama. */
    if (arr == NULL)
        return;
    bigint_array_free(arr);
}

/* limpiar_pantalla: Envia secuencias ANSI de escape para borrar la terminal. */
/* funcion limpiar_pantalla: contiene la logica principal de esta operacion. */
static void limpiar_pantalla(void)
{
    printf("\033[2J\033[H");
}

/* dibujar_linea: Imprime una barra separadora estetica. */
/* funcion dibujar_linea: contiene la logica principal de esta operacion. */
static void dibujar_linea(void)
{
    printf("+--------------------------------------------------+\n");
}

/* dibujar_titulo: Escribe el encabezado principal de la app. */
/* funcion dibujar_titulo: contiene la logica principal de esta operacion. */
static void dibujar_titulo(void)
{
    limpiar_pantalla();
    dibujar_linea();
    printf("|          SISTEMA DE CAMBIO DE MONEDAS            |\n");
    dibujar_linea();
}

/* leer_linea_dinamica: Lee una linea completa de stdin sin truncarla. */
static char *leer_linea_dinamica(void)
{
    size_t capacidad = 4096;
    size_t len = 0;
    char *buffer = (char *)malloc(capacidad);
    int ch;

    if (buffer == NULL)
        return NULL;

    while ((ch = fgetc(stdin)) != EOF)
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

/* leer_linea: Lee una linea desde teclado limitando la cantidad de caracteres y limpiando saltos. */
/* funcion leer_linea: contiene la logica principal de esta operacion. */
static int leer_linea(char *buffer, size_t tam)
{
    /* while: repite el bloque mientras se cumpla 1. */
    while (1)
    {
        char comando[64];

        /* if: comprueba fgets(buffer, (int)tam, stdin) == NULL antes de ejecutar esta rama. */
        if (fgets(buffer, (int)tam, stdin) == NULL)
            return 0;

        buffer[strcspn(buffer, "\r\n")] = '\0';

        strncpy(comando, buffer, sizeof(comando) - 1);
        comando[sizeof(comando) - 1] = '\0';
        a_minusculas(comando);

        /* if: comprueba strcmp(comando, "gui") == 0 antes de ejecutar esta rama. */
        if (strcmp(comando, "gui") == 0)
        {
            /* if: comprueba progvoraz_launch_gui_nonblocking() antes de ejecutar esta rama. */
            if (progvoraz_launch_gui_nonblocking())
                printf("%s\n", TR("Interfaz GUI lanzada en segundo plano.", "GUI launched in the background."));
            else
                printf("%s\n", TR("No se encontro ejecutable GUI (progvoraz_gui).", "GUI executable not found (progvoraz_gui)."));
            continue;
        }

        return 1;
    }
}

/* a_minusculas: Modifica in-place un string para que todo sea lowercase. */
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

/*
 * Normaliza texto de clave para comparacion con archivos:
 * - pasa a minusculas
 * - elimina acentos comunes (UTF-8 y Latin-1/CP1252)
 */
/* funcion normalizar_clave: contiene la logica principal de esta operacion. */
static void normalizar_clave(const char *origen, char *destino, size_t tamDestino)
{
    size_t i = 0;
    size_t j = 0;

    /* if: comprueba destino == NULL || tamDestino == 0 antes de ejecutar esta rama. */
    if (destino == NULL || tamDestino == 0)
        return;

    destino[0] = '\0';
    /* if: comprueba origen == NULL antes de ejecutar esta rama. */
    if (origen == NULL)
        return;

    /* Bucle principal: recorre cada byte del string ingresado. */
    /* while: repite el bloque mientras se cumpla origen[i] != '\0' && j + 1 < tamDestino. */
    while (origen[i] != '\0' && j + 1 < tamDestino)
    {
        unsigned char c = (unsigned char)origen[i];

        /* Detecta el primer byte del prefijo UTF-8 para vocales acentuadas comunes (0xC3). */
        /* if: comprueba c == 0xC3 && origen[i + 1] != '\0' antes de ejecutar esta rama. */
        if (c == 0xC3 && origen[i + 1] != '\0')
        {
            unsigned char s = (unsigned char)origen[i + 1];
            char reemplazo = '\0';

            /* if: comprueba s == 0xA1 || s == 0x81 antes de ejecutar esta rama. */
            if (s == 0xA1 || s == 0x81)
                reemplazo = 'a';
            /* if: comprueba s == 0xA9 || s == 0x89 antes de continuar con esta alternativa. */
            else if (s == 0xA9 || s == 0x89)
                reemplazo = 'e';
            /* if: comprueba s == 0xAD || s == 0x8D antes de continuar con esta alternativa. */
            else if (s == 0xAD || s == 0x8D)
                reemplazo = 'i';
            /* if: comprueba s == 0xB3 || s == 0x93 antes de continuar con esta alternativa. */
            else if (s == 0xB3 || s == 0x93)
                reemplazo = 'o';
            /* if: comprueba s == 0xBA || s == 0x9A || s == 0xBC || s == 0x9C antes de continuar con esta alternativa. */
            else if (s == 0xBA || s == 0x9A || s == 0xBC || s == 0x9C)
                reemplazo = 'u';
            /* if: comprueba s == 0xB1 || s == 0x91 antes de continuar con esta alternativa. */
            else if (s == 0xB1 || s == 0x91)
                reemplazo = 'n';

            /* Si se determino una traduccion... */
            /* if: comprueba reemplazo != '\0' antes de ejecutar esta rama. */
            if (reemplazo != '\0')
            {
                destino[j++] = reemplazo;
                i += 2;
                continue;
            }
        }

        // Tabla de normalizacion rapida por bytes sueltos (para CP1252 o restos).
        /* if: comprueba c == 0xE1 || c == 0xC1 || c == 0xA0 antes de ejecutar esta rama. */
        if (c == 0xE1 || c == 0xC1 || c == 0xA0)
            destino[j++] = 'a';
        /* if: comprueba c == 0xE9 || c == 0xC9 || c == 0x82 || c == 0x90 antes de continuar con esta alternativa. */
        else if (c == 0xE9 || c == 0xC9 || c == 0x82 || c == 0x90)
            destino[j++] = 'e';
        /* if: comprueba c == 0xED || c == 0xCD || c == 0xA1 || c == 0xD6 antes de continuar con esta alternativa. */
        else if (c == 0xED || c == 0xCD || c == 0xA1 || c == 0xD6)
            destino[j++] = 'i';
        /* if: comprueba c == 0xF3 || c == 0xD3 || c == 0xA2 || c == 0xE0 antes de continuar con esta alternativa. */
        else if (c == 0xF3 || c == 0xD3 || c == 0xA2 || c == 0xE0)
            destino[j++] = 'o';
        /* if: comprueba c == 0xFA || c == 0xDA || c == 0xFC || c == 0xDC || c == 0xA3 || c ==... antes de continuar con esta alternativa. */
        else if (c == 0xFA || c == 0xDA || c == 0xFC || c == 0xDC || c == 0xA3 || c == 0x81 || c == 0x9A)
            destino[j++] = 'u';
        /* if: comprueba c == 0xF1 || c == 0xD1 || c == 0xA4 || c == 0xA5 antes de continuar con esta alternativa. */
        else if (c == 0xF1 || c == 0xD1 || c == 0xA4 || c == 0xA5)
            destino[j++] = 'n';
        else
            destino[j++] = (char)tolower(c);

        i++;
    }

    destino[j] = '\0';
}

/* es_token_denominacion: Valida si la palabra leida es un numero natural o un flag de -1. */
/* funcion es_token_denominacion: contiene la logica principal de esta operacion. */
static int es_token_denominacion(const char *token)
{
    size_t i;

    /* if: comprueba token == NULL || token[0] == '\0' antes de ejecutar esta rama. */
    if (token == NULL || token[0] == '\0')
        return 0;

    /* if: comprueba strcmp(token, "-1") == 0 antes de ejecutar esta rama. */
    if (strcmp(token, "-1") == 0)
        return 1;

    /* for: itera segun i = 0; token[i] != '\0'; i++ para recorrer el bloque. */
    for (i = 0; token[i] != '\0'; i++)
    {
        /* if: comprueba token[i] < '0' || token[i] > '9' antes de ejecutar esta rama. */
        if (token[i] < '0' || token[i] > '9')
            return 0;
    }

    return 1;
}

/* clave_ya_existe: Chequea en un arreglo de strings si ya hemos cargado este nombre de moneda. */
/* funcion clave_ya_existe: contiene la logica principal de esta operacion. */
static int clave_ya_existe(char claves[][MAX_MONEDA_NOMBRE + 1], size_t cantidad, const char *clave)
{
    size_t i;

    /* for: itera segun i = 0; i < cantidad; i++ para recorrer el bloque. */
    for (i = 0; i < cantidad; i++)
    {
        /* if: comprueba strcmp(claves[i], clave) == 0 antes de ejecutar esta rama. */
        if (strcmp(claves[i], clave) == 0)
            return 1;
    }

    return 0;
}

/* copiar_clave: Wrapper seguro para copiar strings fijos de tamaño especifico en la RAM. */
/* funcion copiar_clave: contiene la logica principal de esta operacion. */
static void copiar_clave(char destino[MAX_MONEDA_NOMBRE + 1], const char *origen)
{
    size_t i = 0;

    /* Protege contra accesos invalidos verificando que no copiemos la nada. */
    /* if: comprueba origen == NULL antes de ejecutar esta rama. */
    if (origen == NULL)
    {
        destino[0] = '\0';
        return;
    }

    /* Bucle de clonacion: copia hasta alcanzar el MAXimo permitido para que no haya buffer overflow. */
    /* while: repite el bloque mientras se cumpla i < MAX_MONEDA_NOMBRE && origen[i] != '\0'. */
    while (i < MAX_MONEDA_NOMBRE && origen[i] != '\0')
    {
        destino[i] = origen[i];
        i++;
    }

    destino[i] = '\0';
}

/* cargar_claves_monedas: Extrae la lista de las monedas (ej: Euro, Dolar) del archivo txt parseando. */
/* funcion cargar_claves_monedas: contiene la logica principal de esta operacion. */
static int cargar_claves_monedas(const char *archivo, char claves[][MAX_MONEDA_NOMBRE + 1], size_t maxClaves, size_t *cantidad)
{
    FILE *fp;
    char token[256];

    /* if: comprueba archivo == NULL || claves == NULL || cantidad == NULL antes de ejecutar esta rama. */
    if (archivo == NULL || claves == NULL || cantidad == NULL)
        return 0;

    *cantidad = 0;
    fp = fopen(archivo, "r");
    /* if: comprueba fp == NULL antes de ejecutar esta rama. */
    if (fp == NULL)
        return 0;

    /* Extrae secuencialmente todas las palabras (tokens) separadas por whitespace en el archivo. */
    /* while: repite el bloque mientras se cumpla fscanf(fp, "%255s", token) == 1. */
    while (fscanf(fp, "%255s", token) == 1)
    {
        char clave[MAX_MONEDA_NOMBRE + 1];

        /* if: comprueba es_token_denominacion(token) antes de ejecutar esta rama. */
        if (es_token_denominacion(token))
            continue;

        normalizar_clave(token, clave, sizeof(clave));
        /* if: comprueba clave[0] == '\0' antes de ejecutar esta rama. */
        if (clave[0] == '\0')
            continue;
        /* if: comprueba clave_ya_existe(claves, *cantidad, clave) antes de ejecutar esta rama. */
        if (clave_ya_existe(claves, *cantidad, clave))
            continue;

        /* Chequea que el arreglo del menu de la consola no exceda su tamano maximo hardcodeado. */
        /* if: comprueba *cantidad >= maxClaves antes de ejecutar esta rama. */
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
/* funcion imprimir_lista_monedas: contiene la logica principal de esta operacion. */
static void imprimir_lista_monedas(const char claves[][MAX_MONEDA_NOMBRE + 1], size_t cantidad)
{
    size_t i;

    /* Valida si existen monedas para mostrar. */
    /* if: comprueba cantidad == 0 antes de ejecutar esta rama. */
    if (cantidad == 0)
    {
        printf("%s\n", TR("Monedas disponibles: (sin datos)", "Available currencies: (no data)"));
        return;
    }

    printf("%s\n", TR("Monedas disponibles:", "Available currencies:"));
    /* for: itera segun i = 0; i < cantidad; i++ para recorrer el bloque. */
    for (i = 0; i < cantidad; i++)
    {
        printf("%s", claves[i]);
        /* if: comprueba i + 1 < cantidad antes de ejecutar esta rama. */
        if (i + 1 < cantidad)
            printf(", ");
        /* if: comprueba (i + 1) % 8 == 0 || i + 1 == cantidad antes de ejecutar esta rama. */
        if ((i + 1) % 8 == 0 || i + 1 == cantidad)
            printf("\n");
    }
}

/* mostrar_monedas_disponibles: Extrae, filtra e imprime las divisas dependiendo del modo seleccionado. */
/* funcion mostrar_monedas_disponibles: contiene la logica principal de esta operacion. */
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
    /* if: comprueba opcion == 'a' antes de ejecutar esta rama. */
    if (opcion == 'a')
    {
        /* Intenta leer unicamente las definiciones de monedas sin importar si tienen stock. */
        /* if: comprueba !cargar_claves_monedas("monedas.txt", desdeMonedas, MAX_MONEDAS_DISPO... antes de ejecutar esta rama. */
        if (!cargar_claves_monedas("monedas.txt", desdeMonedas, MAX_MONEDAS_DISPONIBLES, &nMonedas))
        {
            printf("No se pudieron leer monedas disponibles desde monedas.txt.\n");
            return;
        }

        imprimir_lista_monedas((const char (*)[MAX_MONEDA_NOMBRE + 1]) desdeMonedas, nMonedas);
        return;
    }

    // Modo 'b' o 'c' (Limitado o Admin): requiere cruzar datos de ambas DB.
    /* if: comprueba !cargar_claves_monedas("monedas.txt", desdeMonedas, MAX_MONEDAS_DISPO... antes de ejecutar esta rama. */
    if (!cargar_claves_monedas("monedas.txt", desdeMonedas, MAX_MONEDAS_DISPONIBLES, &nMonedas) ||
        !cargar_claves_monedas("stock.txt", desdeStock, MAX_MONEDAS_DISPONIBLES, &nStock))
    {
        printf("No se pudieron leer monedas/stock disponibles.\n");
        return;
    }

    /* for: itera segun i = 0; i < nMonedas; i++ para recorrer el bloque. */
    for (i = 0; i < nMonedas; i++)
    {
        /* Verifica que la moneda de "monedas.txt" TAMBIEN exista declarada en "stock.txt". */
        /* if: comprueba clave_ya_existe(desdeStock, nStock, desdeMonedas[i]) antes de ejecutar esta rama. */
        if (clave_ya_existe(desdeStock, nStock, desdeMonedas[i]))
        {
            /* if: comprueba nVisibles >= MAX_MONEDAS_DISPONIBLES antes de ejecutar esta rama. */
            if (nVisibles >= MAX_MONEDAS_DISPONIBLES)
                break;

            copiar_clave(visibles[nVisibles], desdeMonedas[i]);
            nVisibles++;
        }
    }

    imprimir_lista_monedas((const char (*)[MAX_MONEDA_NOMBRE + 1]) visibles, nVisibles);
}

/* funcion mostrar_resumen_desde_menu_principal: contiene la logica principal de esta operacion. */
static void mostrar_resumen_desde_menu_principal(void)
{
    char moneda[MAX_MONEDA_NOMBRE + 1];
    char monedaClave[MAX_MONEDA_NOMBRE + 1];
    char comando[MAX_MONEDA_NOMBRE + 1];
    BigIntArray monedas = {0};
    BigIntArray stock = {0};
    int tieneStock;

    mostrar_monedas_disponibles('a');
    printf("%s", TR("Nombre de la moneda para resumen (volver o salir): ", "Currency name for summary (back or exit): "));

    /* if: comprueba !leer_linea(moneda, sizeof(moneda)) antes de ejecutar esta rama. */
    if (!leer_linea(moneda, sizeof(moneda)))
    {
        printf("%s\n", TR("Entrada finalizada.", "Input ended."));
        return;
    }

    /* if: comprueba moneda[0] == '\0' antes de ejecutar esta rama. */
    if (moneda[0] == '\0')
    {
        printf("%s\n", TR("Invalid currency name.", "Invalid currency name."));
        return;
    }

    normalizar_clave(moneda, comando, sizeof(comando));
    /* if: comprueba strcmp(comando, "volver") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_back(comando))
        return;
    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
    {
        printf("%s\n", TR("Use 'salir' en el menu principal para cerrar la aplicacion.", "Use 'exit' in the main menu to close the application."));
        return;
    }

    {
        const char *mapped = progvoraz_map_currency_key(moneda);
        snprintf(monedaClave, sizeof(monedaClave), "%s", mapped != NULL ? mapped : "");
    }

    /* if: comprueba !cargar_denominaciones_moneda(monedaClave, &monedas) antes de ejecutar esta rama. */
    if (!cargar_denominaciones_moneda(monedaClave, &monedas))
    {
        printf("%s\n", TR("Currency not found in monedas.txt.", "Currency not found in monedas.txt."));
        return;
    }

    tieneStock = cargar_stock_moneda(monedaClave, &stock) && stock.len == monedas.len;
    mostrar_resumen_moneda(monedaClave, &monedas, tieneStock ? &stock : NULL, tieneStock);

    /* if: comprueba !tieneStock antes de ejecutar esta rama. */
    if (!tieneStock)
    {
        printf("%s\n", TR("Resumen mostrado sin stock (no disponible o inconsistente).", "Summary shown without stock (unavailable or inconsistent)."));
        registrar_historialf("Resumen consultado | Moneda=%s | Modo=Ilimitado", monedaClave);
    }

    limpiar_arreglo(&stock);
    limpiar_arreglo(&monedas);
}

/* funcion gestionar_operacion_global: contiene la logica principal de esta operacion. */
static void gestionar_operacion_global(const char *accion)
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
            printf("%s\n", TR("Snapshot de stock creado en stock_snapshot.txt", "Stock snapshot created in stock_snapshot.txt"));
            registrar_historial("Snapshot de stock creado");
        }
        else
        {
            printf("%s\n", TR("No se pudo crear snapshot de stock.", "Could not create stock snapshot."));
        }
        return;
    }

    /* if: comprueba strcmp(accion, "restaurar") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "restaurar") == 0)
    {
        /* if: comprueba restaurar_snapshot_stock("stock_snapshot.txt") antes de ejecutar esta rama. */
        if (restaurar_snapshot_stock("stock_snapshot.txt"))
        {
            printf("%s\n", TR("Stock restaurado desde stock_snapshot.txt", "Stock restored from stock_snapshot.txt"));
            registrar_historial("Stock restaurado desde snapshot");
        }
        else
        {
            printf("%s\n", TR("No se pudo restaurar stock (verifique stock_snapshot.txt).", "Could not restore stock (check stock_snapshot.txt)."));
        }
        return;
    }

    /* if: comprueba strcmp(accion, "reporte") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "reporte") == 0)
    {
        /* if: comprueba exportar_reporte_global("reporte_global.txt") antes de ejecutar esta rama. */
        if (exportar_reporte_global("reporte_global.txt"))
        {
            printf("%s\n", TR("Reporte global generado en reporte_global.txt", "Global report generated in reporte_global.txt"));
            registrar_historial("Reporte global exportado");
        }
        else
        {
            printf("%s\n", TR("No se pudo generar reporte global.", "Could not generate the global report."));
        }
        return;
    }

    /* if: comprueba strcmp(accion, "json") == 0 antes de ejecutar esta rama. */
    if (strcmp(accion, "json") == 0)
    {
        /* if: comprueba export_stock_json("stock_out.json") antes de ejecutar esta rama. */
        if (export_stock_json("stock_out.json"))
        {
            printf("%s\n", TR("Stock exportado en JSON a stock_out.json", "Stock exported to JSON at stock_out.json"));
            registrar_historial("Stock exportado a JSON");
        }
        else
        {
            printf("%s\n", TR("No se pudo exportar el stock a JSON.", "Could not export stock to JSON."));
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
 * - -8 si usuario pide exportar stock JSON
 * - 0 si la entrada es invalida/vacia
 * - -1 si hubo EOF/error de lectura
 * - -2 si usuario pide salir
 */
/* funcion pedir_opcion: contiene la logica principal de esta operacion. */
static int pedir_opcion(void)
{
    char buffer[16];
    char comando[16];

    dibujar_titulo();
    printf("| %s |\n", TR("Elija modo de trabajo:                            ", "Choose a working mode:                           "));
    printf("|   a) %s |\n", TR("Monedas infinitas                            ", "Unlimited coins                              "));
    printf("|   b) %s |\n", TR("Monedas limitadas (usa stock)                ", "Limited coins (uses stock)                   "));
    printf("|   c) %s |\n", TR("Administrador de stock                       ", "Stock administrator                          "));
    printf("|   h) %s |\n", TR("Historial de transacciones                   ", "Transaction history                          "));
    printf("|   r) %s |\n", TR("Resumen de moneda                            ", "Currency summary                             "));
    printf("|   s) %s |\n", TR("Snapshot stock                               ", "Stock snapshot                               "));
    printf("|   u) %s |\n", TR("Restaurar snapshot                           ", "Restore snapshot                             "));
    printf("|   g) %s |\n", TR("Generar reporte global                       ", "Generate global report                       "));
    printf("|   j) %s |\n", TR("Exportar stock JSON                          ", "Export stock JSON                            "));
    printf("|   x) %s |\n", TR("Conversion entre monedas                     ", "Currency conversion                          "));
    dibujar_linea();
    printf("%s", TR("Opcion (a/b/c/h/r/s/u/g/j/x, gui o salir): ", "Option (a/b/c/h/r/s/u/g/j/x, gui or exit): "));

    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);
    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
        return -2;
    /* if: comprueba strcmp(comando, "historial") == 0 || strcmp(comando, "h") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_history(comando) || strcmp(comando, "h") == 0)
        return -3;
    /* if: comprueba strcmp(comando, "resumen") == 0 || strcmp(comando, "r") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_summary(comando) || strcmp(comando, "r") == 0)
        return -4;
    /* if: comprueba strcmp(comando, "snapshot") == 0 || strcmp(comando, "s") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_snapshot(comando) || strcmp(comando, "s") == 0)
        return -5;
    /* if: comprueba strcmp(comando, "restaurar") == 0 || strcmp(comando, "u") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_restore(comando) || strcmp(comando, "u") == 0)
        return -6;
    /* if: comprueba strcmp(comando, "reporte") == 0 || strcmp(comando, "g") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_report(comando) || strcmp(comando, "g") == 0)
        return -7;
    /* if: comprueba strcmp(comando, "json") == 0 || strcmp(comando, "j") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_json(comando) || strcmp(comando, "j") == 0)
        return -8;
    /* if: comprueba strcmp(comando, "convertir") == 0 || strcmp(comando, "x") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_convert(comando) || strcmp(comando, "x") == 0)
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
/* funcion pedir_cantidad: contiene la logica principal de esta operacion. */
static int pedir_cantidad(BigInt *cantidad)
{
    char comando[64];
    char *buffer = NULL;
    BigInt tmp = {0};

    printf("%s", TR("Cantidad en centimos (0, volver, gui o salir): ",
                   "Amount in cents (0, back, gui or exit): "));
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    buffer = leer_linea_dinamica();
    if (buffer == NULL)
        return -1;

    /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
    if (buffer[0] == '\0')
    {
        free(buffer);
        return 0;
    }

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
    {
        free(buffer);
        return 3;
    }

    /* if: comprueba strcmp(comando, "volver") == 0 || strcmp(comando, "0") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_back(comando) || strcmp(comando, "0") == 0)
    {
        free(buffer);
        return 2;
    }

    /* if: comprueba !bigint_init(&tmp, buffer) antes de ejecutar esta rama. */
    if (!bigint_init(&tmp, buffer))
    {
        free(buffer);
        return 0;
    }

    bigint_free(cantidad);
    *cantidad = tmp;
    free(buffer);
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
/* funcion pedir_cantidad_admin: contiene la logica principal de esta operacion. */
static int pedir_cantidad_admin(BigInt *cantidad)
{
    char comando[64];
    char *buffer = NULL;
    BigInt tmp = {0};

    printf("%s", TR("Cantidad (volver, modo, gui o salir): ",
                   "Amount (back, mode, gui or exit): "));
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    buffer = leer_linea_dinamica();
    if (buffer == NULL)
        return -1;

    /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
    if (buffer[0] == '\0')
    {
        free(buffer);
        return 0;
    }

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
    {
        free(buffer);
        return 4;
    }
    /* if: comprueba strcmp(comando, "modo") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_mode(comando))
    {
        free(buffer);
        return 3;
    }
    /* if: comprueba strcmp(comando, "volver") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_back(comando))
    {
        free(buffer);
        return 2;
    }

    /* if: comprueba !bigint_init(&tmp, buffer) antes de ejecutar esta rama. */
    if (!bigint_init(&tmp, buffer))
    {
        free(buffer);
        return 0;
    }

    bigint_free(cantidad);
    *cantidad = tmp;
    free(buffer);
    return 1;
}

/* pedir_indice_denominacion: Pide al admin que divisa especifica dentro del array de monedas quiere modificar. */
/* funcion pedir_indice_denominacion: contiene la logica principal de esta operacion. */
static int pedir_indice_denominacion(size_t maximo, size_t *indice)
{
    char buffer[64];
    char comando[64];
    char *fin;
    long v;

    printf("%s%zu%s",
           TR("Indice de denominacion (1-", "Denomination index (1-"),
           maximo,
           TR(", o volver/modo/gui/salir): ", ", or back/mode/gui/exit): "));
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
        return 4;
    /* if: comprueba strcmp(comando, "modo") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_mode(comando))
        return 3;
    /* if: comprueba strcmp(comando, "volver") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_back(comando))
        return 2;

    v = strtol(buffer, &fin, 10);
    /* if: comprueba *fin != '\0' || v < 1 || (size_t)v > maximo antes de ejecutar esta rama. */
    if (*fin != '\0' || v < 1 || (size_t)v > maximo)
        return 0;

    *indice = (size_t)(v - 1);
    return 1;
}

/* copiar_arreglo_bigint: Clona el vector dinamico subyacente que aloja BigInts. */
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
        /* Trata de inyectar y clonar el elemento 'i' desde el origen al indice equivalente 'i' del destino. */
        /* if: comprueba !bigint_array_set(destino, i, &origen->items[i]) antes de ejecutar esta rama. */
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

/* funcion imprimir_sugerencia_cercana: contiene la logica principal de esta operacion. */
static void imprimir_sugerencia_cercana(const BigInt *solicitado,
                                        const BigInt *cubierto,
                                        const BigIntArray *monedas,
                                        const BigIntArray *solucion,
                                        int con_stock)
{
    BigInt faltante = {0};

    /* if: comprueba solicitado == NULL || cubierto == NULL || monedas == NULL || solucion... antes de ejecutar esta rama. */
    if (solicitado == NULL || cubierto == NULL || monedas == NULL || solucion == NULL)
        return;

    /* if: comprueba !bigint_subtract(solicitado, cubierto, &faltante) antes de ejecutar esta rama. */
    if (!bigint_subtract(solicitado, cubierto, &faltante))
    {
        printf("%s\n", TR("No existe cambio exacto. Se encontro un cambio cercano parcial.",
                          "No exact change exists. A partial close change was found."));
        imprimir_resultado(monedas, solucion, NULL, 0);
        return;
    }

    printf("%s\n", TR("No existe cambio exacto.", "No exact change exists."));
    printf("%s%s%s%s%s\n",
           TR("Sugerencia cercana: cubrir ", "Close suggestion: cover "),
           cubierto->digits,
           TR(" c (faltan ", " c (missing "),
           faltante.digits,
           TR(" c).", " c)."));
    /* if: comprueba con_stock antes de ejecutar esta rama. */
    if (con_stock)
        printf("%s\n", TR("Esta sugerencia no se aplica automaticamente al stock actual.",
                          "This suggestion is not applied automatically to the current stock."));

    imprimir_resultado(monedas, solucion, NULL, 0);
    bigint_free(&faltante);
}

/* funcion preguntar_aceptar_sugerencia: contiene la logica principal de esta operacion. */
static int preguntar_aceptar_sugerencia(const BigInt *solicitado,
                                        const BigInt *cubierto,
                                        int con_stock)
{
    BigInt faltante = {0};
    char buffer[32];
    char comando[32];

    /* if: comprueba solicitado == NULL || cubierto == NULL antes de ejecutar esta rama. */
    if (solicitado == NULL || cubierto == NULL)
        return 0;

    /* if: comprueba !bigint_subtract(solicitado, cubierto, &faltante) antes de ejecutar esta rama. */
    if (!bigint_subtract(solicitado, cubierto, &faltante))
        return 0;

    /* while: repite el bloque mientras se cumpla 1. */
    while (1)
    {
        /* if: comprueba con_stock antes de ejecutar esta rama. */
        if (con_stock)
            printf("%s%s%s%s%s",
                   TR("No hay cambio exacto. Sugerencia: devolver ", "No exact change. Suggestion: return "),
                   cubierto->digits,
                   TR(" c (faltan ", " c (missing "),
                   faltante.digits,
                   TR(" c). Aceptar? (si/no): ", " c). Accept? (yes/no): "));
        else
            printf("%s%s%s%s%s",
                   TR("No hay cambio exacto. Sugerencia: cubrir ", "No exact change. Suggestion: cover "),
                   cubierto->digits,
                   TR(" c (faltan ", " c (missing "),
                   faltante.digits,
                   TR(" c). Aceptar? (si/no): ", " c). Accept? (yes/no): "));

        /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
        if (!leer_linea(buffer, sizeof(buffer)))
        {
            bigint_free(&faltante);
            return -1;
        }

        strncpy(comando, buffer, sizeof(comando) - 1);
        comando[sizeof(comando) - 1] = '\0';
        a_minusculas(comando);

        /* if: comprueba strcmp(comando, "si") == 0 || strcmp(comando, "s") == 0 || strcmp(com... antes de ejecutar esta rama. */
        if (strcmp(comando, "si") == 0 || strcmp(comando, "s") == 0 || strcmp(comando, "yes") == 0 || strcmp(comando, "y") == 0)
        {
            bigint_free(&faltante);
            return 1;
        }

        /* if: comprueba strcmp(comando, "no") == 0 || strcmp(comando, "n") == 0 antes de ejecutar esta rama. */
        if (strcmp(comando, "no") == 0 || strcmp(comando, "n") == 0)
        {
            bigint_free(&faltante);
            return 0;
        }

        printf("%s\n", TR("Respuesta invalida. Escriba si o no.",
                          "Invalid response. Type yes or no."));
    }
}

/* funcion aplicar_devolucion_stock: contiene la logica principal de esta operacion. */
static int aplicar_devolucion_stock(const char *monedaClave,
                                    BigIntArray *stock,
                                    const BigIntArray *solucion)
{
    BigIntArray stockNuevo = {0};
    int okStockNuevo;

    /* if: comprueba monedaClave == NULL || stock == NULL || solucion == NULL antes de ejecutar esta rama. */
    if (monedaClave == NULL || stock == NULL || solucion == NULL)
        return 0;

    okStockNuevo = copiar_arreglo_bigint(stock, &stockNuevo);
    /* if: comprueba !okStockNuevo antes de ejecutar esta rama. */
    if (!okStockNuevo)
        return 0;

    /* for: itera segun size_t i = 0; i < stockNuevo.len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stockNuevo.len; i++)
    {
        BigInt restante = {0};

        /* if: comprueba bigint_is_zero(&solucion->items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&solucion->items[i]))
            continue;

        /* if: comprueba !bigint_subtract(&stockNuevo.items[i], &solucion->items[i], &restante... antes de ejecutar esta rama. */
        if (!bigint_subtract(&stockNuevo.items[i], &solucion->items[i], &restante) ||
            !bigint_array_set(&stockNuevo, i, &restante))
        {
            bigint_free(&restante);
            okStockNuevo = 0;
            break;
        }

        bigint_free(&restante);
    }

    /* if: comprueba !okStockNuevo antes de ejecutar esta rama. */
    if (!okStockNuevo)
    {
        limpiar_arreglo(&stockNuevo);
        return 0;
    }

    /* if: comprueba !actualizar_stock_moneda(monedaClave, &stockNuevo) antes de ejecutar esta rama. */
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
/* funcion imprimir_stock_administrador: contiene la logica principal de esta operacion. */
static void imprimir_stock_administrador(const BigIntArray *monedas, const BigIntArray *stock)
{
    dibujar_linea();
    printf("%s\n", TR("Panel Administrador (Consola)", "Administrator Panel (Console)"));
    dibujar_linea();
    /* for: itera segun size_t i = 0; i < monedas->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < monedas->len; i++)
        printf("[%zu] Denom %s c -> stock %s\n", i + 1, monedas->items[i].digits, stock->items[i].digits);
    dibujar_linea();
}

/* funcion mostrar_resumen_moneda: contiene la logica principal de esta operacion. */
static void mostrar_resumen_moneda(const char *monedaClave, const BigIntArray *monedas, const BigIntArray *stock, int usarStock)
{
    const BigInt *minDenom;
    const BigInt *maxDenom;
    size_t i;

    /* if: comprueba monedas == NULL || monedas->items == NULL || monedas->len == 0 antes de ejecutar esta rama. */
    if (monedas == NULL || monedas->items == NULL || monedas->len == 0)
    {
            printf("%s\n", TR("No hay denominaciones cargadas para mostrar resumen.",
                              "No denominations loaded to show the summary."));
        return;
    }

    minDenom = &monedas->items[0];
    maxDenom = &monedas->items[0];

    /* for: itera segun i = 1; i < monedas->len; i++ para recorrer el bloque. */
    for (i = 1; i < monedas->len; i++)
    {
        /* if: comprueba bigint_compare(&monedas->items[i], minDenom) < 0 antes de ejecutar esta rama. */
        if (bigint_compare(&monedas->items[i], minDenom) < 0)
            minDenom = &monedas->items[i];
        /* if: comprueba bigint_compare(&monedas->items[i], maxDenom) > 0 antes de ejecutar esta rama. */
        if (bigint_compare(&monedas->items[i], maxDenom) > 0)
            maxDenom = &monedas->items[i];
    }

    dibujar_linea();
    printf("%s %s\n", TR("Resumen de moneda:", "Currency summary:"), monedaClave != NULL ? monedaClave : TR("(sin nombre)", "(unnamed)"));
    printf("%s %zu\n", TR("Denominaciones cargadas:", "Loaded denominations:"), monedas->len);
    printf("%s %s c\n", TR("Denominacion minima:", "Minimum denomination:"), minDenom->digits);
    printf("%s %s c\n", TR("Denominacion maxima:", "Maximum denomination:"), maxDenom->digits);

    /* if: comprueba !usarStock antes de ejecutar esta rama. */
    if (!usarStock)
    {
        printf("%s\n", TR("Modo stock ilimitado: no se calcula inventario fisico.",
                          "Unlimited stock mode: physical inventory is not calculated."));
        dibujar_linea();
        return;
    }

    /* if: comprueba stock == NULL || stock->items == NULL || stock->len != monedas->len antes de ejecutar esta rama. */
    if (stock == NULL || stock->items == NULL || stock->len != monedas->len)
    {
            printf("%s\n", TR("No hay stock valido para calcular resumen de inventario.",
                              "There is no valid stock to compute the inventory summary."));
        dibujar_linea();
        return;
    }

    {
        BigInt totalPiezas = {0};
        BigInt totalValor = {0};
        int ok = 1;

        /* if: comprueba !bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0") antes de ejecutar esta rama. */
        if (!bigint_init(&totalPiezas, "0") || !bigint_init(&totalValor, "0"))
            ok = 0;

        /* for: itera segun i = 0; ok && i < monedas->len; i++ para recorrer el bloque. */
        for (i = 0; ok && i < monedas->len; i++)
        {
            BigInt nuevoTotalPiezas = {0};
            BigInt parcialValor = {0};
            BigInt nuevoTotalValor = {0};

            /* if: comprueba !bigint_add(&totalPiezas, &stock->items[i], &nuevoTotalPiezas) || antes de ejecutar esta rama. */
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

        /* if: comprueba ok antes de ejecutar esta rama. */
        if (ok)
        {
            printf("%s %s\n", TR("Total de piezas en stock:", "Total pieces in stock:"), totalPiezas.digits);
            printf("%s %s c\n", TR("Valor total del stock:", "Total stock value:"), totalValor.digits);
            registrar_historialf("Resumen consultado | Moneda=%s | Piezas=%s | Valor=%s c",
                                 monedaClave != NULL ? monedaClave : "(sin nombre)",
                                 totalPiezas.digits,
                                 totalValor.digits);
        }
        else
        {
            printf("%s\n", TR("No se pudo calcular el resumen de inventario por error interno.",
                              "Could not compute the inventory summary due to an internal error."));
        }

        bigint_free(&totalPiezas);
        bigint_free(&totalValor);
    }

    dibujar_linea();
}

/* aplicar_cambio_administrador: Suma o Resta stock manual a una divisa indicada en 'idx'. */
/* funcion aplicar_cambio_administrador: contiene la logica principal de esta operacion. */
static int aplicar_cambio_administrador(BigIntArray *stock, size_t idx, const BigInt *delta, int esSuma)
{
    BigInt nuevo = {0};

    /* Bifurca si el usuario pidio agregar monedas (+) o quitar monedas (-). */
    /* if: comprueba esSuma antes de ejecutar esta rama. */
    if (esSuma)
    {
        /* if: comprueba !bigint_add(&stock->items[idx], delta, &nuevo) antes de ejecutar esta rama. */
        if (!bigint_add(&stock->items[idx], delta, &nuevo))
            return 0;
    }
    else
    {
        /* if: comprueba bigint_compare(&stock->items[idx], delta) < 0 antes de ejecutar esta rama. */
        if (bigint_compare(&stock->items[idx], delta) < 0)
            return 0;
        /* if: comprueba !bigint_subtract(&stock->items[idx], delta, &nuevo) antes de ejecutar esta rama. */
        if (!bigint_subtract(&stock->items[idx], delta, &nuevo))
            return 0;
    }

    /* Asigna exitosamente al arreglo stock el puntero resultante (se le transfiere su propiedad interna string). */
    /* if: comprueba !bigint_array_set(stock, idx, &nuevo) antes de ejecutar esta rama. */
    if (!bigint_array_set(stock, idx, &nuevo))
    {
        bigint_free(&nuevo);
        return 0;
    }

    bigint_free(&nuevo);
    return 1;
}

/* pedir_subopcion_cambio: Interfaz para sub-opciones de usuario segun el modo activo. */
/* funcion pedir_subopcion_cambio: contiene la logica principal de esta operacion. */
static int pedir_subopcion_cambio(int permitir_limite)
{
    char buffer[32];
    char comando[32];

    /* if: comprueba permitir_limite antes de ejecutar esta rama. */
    if (permitir_limite)
        printf("%s", TR("Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4 / limite|5|l, volver, modo, gui o salir): ",
                        "Change option (traditional|1 / specific|2 / history|3 / summary|4 / limit|5|l, back, mode, gui or exit): "));
    else
        printf("%s", TR("Subopcion cambio (tradicional|1 / especifico|2 / historial|3 / resumen|4, volver, modo, gui o salir): ",
                        "Change option (traditional|1 / specific|2 / history|3 / summary|4, back, mode, gui or exit): "));
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
        return 4;
    /* if: comprueba strcmp(comando, "modo") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_mode(comando))
        return 3;
    /* if: comprueba strcmp(comando, "volver") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_back(comando))
        return 2;
    /* if: comprueba strcmp(comando, "tradicional") == 0 || strcmp(comando, "t") == 0 || s... antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_traditional(comando) || strcmp(comando, "t") == 0 || strcmp(comando, "1") == 0)
        return 1;
    /* if: comprueba strcmp(comando, "especifico") == 0 || strcmp(comando, "e") == 0 || st... antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_specific(comando) || strcmp(comando, "e") == 0 || strcmp(comando, "2") == 0)
        return 5;
    /* if: comprueba strcmp(comando, "historial") == 0 || strcmp(comando, "h") == 0 || str... antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_history(comando) || strcmp(comando, "h") == 0 || strcmp(comando, "3") == 0)
        return 6;
    /* if: comprueba strcmp(comando, "resumen") == 0 || strcmp(comando, "r") == 0 || strcm... antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_summary(comando) || strcmp(comando, "r") == 0 || strcmp(comando, "4") == 0)
        return 7;
    /* if: comprueba permitir_limite && (strcmp(comando, "limite") == 0 || strcmp(comando,... antes de ejecutar esta rama. */
    if (permitir_limite && (progvoraz_cmd_is_limit(comando) || strcmp(comando, "l") == 0 || strcmp(comando, "5") == 0))
        return 8;

    return 0;
}

/* funcion parse_size_t_positivo: contiene la logica principal de esta operacion. */
static int parse_size_t_positivo(const char *texto, size_t *valor_out)
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

/* funcion parse_restriccion_monedas_texto: contiene la logica principal de esta operacion. */
static int parse_restriccion_monedas_texto(const char *entrada, size_t *min_monedas, size_t *max_monedas)
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
        /* if: comprueba !parse_size_t_positivo(limpio + 1, &exacto) antes de ejecutar esta rama. */
        if (!parse_size_t_positivo(limpio + 1, &exacto))
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
            size_t min_v;
            size_t max_v;
            char izq[64];
            char der[64];
            size_t len_izq = (size_t)(guion - limpio);

            /* if: comprueba len_izq == 0 || len_izq >= sizeof(izq) antes de ejecutar esta rama. */
            if (len_izq == 0 || len_izq >= sizeof(izq))
                return 0;

            memcpy(izq, limpio, len_izq);
            izq[len_izq] = '\0';
            strncpy(der, guion + 1, sizeof(der) - 1);
            der[sizeof(der) - 1] = '\0';

            /* if: comprueba !parse_size_t_positivo(izq, &min_v) || !parse_size_t_positivo(der, &m... antes de ejecutar esta rama. */
            if (!parse_size_t_positivo(izq, &min_v) || !parse_size_t_positivo(der, &max_v))
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
        /* if: comprueba !parse_size_t_positivo(limpio, &max_v) antes de ejecutar esta rama. */
        if (!parse_size_t_positivo(limpio, &max_v))
            return 0;
        *min_monedas = 0;
        *max_monedas = max_v;
        return 1;
    }
}

/* funcion pedir_restriccion_monedas: contiene la logica principal de esta operacion. */
static int pedir_restriccion_monedas(size_t *min_monedas, size_t *max_monedas)
{
    char buffer[128];
    char comando[128];

    printf("%s", TR("Restriccion de monedas (N, =N, N-M, volver, modo, gui o salir): ",
                   "Coin restriction (N, =N, N-M, back, mode, gui or exit): "));
    /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
    if (!leer_linea(buffer, sizeof(buffer)))
        return -1;

    /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
    if (buffer[0] == '\0')
        return 0;

    strncpy(comando, buffer, sizeof(comando) - 1);
    comando[sizeof(comando) - 1] = '\0';
    a_minusculas(comando);

    /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_exit(comando))
        return 4;
    /* if: comprueba strcmp(comando, "modo") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_mode(comando))
        return 3;
    /* if: comprueba strcmp(comando, "volver") == 0 antes de ejecutar esta rama. */
    if (progvoraz_cmd_is_back(comando))
        return 2;

    /* if: comprueba !parse_restriccion_monedas_texto(buffer, min_monedas, max_monedas) antes de ejecutar esta rama. */
    if (!parse_restriccion_monedas_texto(buffer, min_monedas, max_monedas))
        return 0;

    return 1;
}

static int calcular_total_valor(const BigIntArray *monedas, const BigIntArray *cantidades, BigInt *total);

/* funcion validar_cambio_especifico_ilimitado: contiene la logica principal de esta operacion. */
static int validar_cambio_especifico_ilimitado(const BigIntArray *monedas,
                                               const BigIntArray *entregadas,
                                               const BigIntArray *devolucion,
                                               BigInt *totalEntregado,
                                               BigInt *totalDevolucion)
{
    /* if: comprueba monedas == NULL || entregadas == NULL || devolucion == NULL || antes de ejecutar esta rama. */
    if (monedas == NULL || entregadas == NULL || devolucion == NULL ||
        totalEntregado == NULL || totalDevolucion == NULL)
        return 0;

    /* if: comprueba monedas->len != entregadas->len || monedas->len != devolucion->len antes de ejecutar esta rama. */
    if (monedas->len != entregadas->len || monedas->len != devolucion->len)
        return 0;

    /* if: comprueba !calcular_total_valor(monedas, entregadas, totalEntregado) || antes de ejecutar esta rama. */
    if (!calcular_total_valor(monedas, entregadas, totalEntregado) ||
        !calcular_total_valor(monedas, devolucion, totalDevolucion))
        return 0;

    return bigint_compare(totalEntregado, totalDevolucion) == 0;
}

/* pedir_cantidades_por_denominacion: Bucle input para introducir cuanto billete de CADA clase estamos operando. */
/* funcion pedir_cantidades_por_denominacion: contiene la logica principal de esta operacion. */
static int pedir_cantidades_por_denominacion(const BigIntArray *monedas, const char *titulo, BigIntArray *cantidades)
{
    size_t i;

    /* if: comprueba monedas == NULL || monedas->items == NULL || monedas->len == 0 || tit... antes de ejecutar esta rama. */
    if (monedas == NULL || monedas->items == NULL || monedas->len == 0 || titulo == NULL || cantidades == NULL)
        return 0;

    /* if: comprueba !bigint_array_create(cantidades, monedas->len) antes de ejecutar esta rama. */
    if (!bigint_array_create(cantidades, monedas->len))
        return 0;

    printf("%s\n", titulo);
    /* for: itera segun i = 0; i < monedas->len; i++ para recorrer el bloque. */
    for (i = 0; i < monedas->len; i++)
    {
        /* Bucle infinito de trampa: Se repite solo si el usuario da entradas corrompidas; hasta que conteste bien o escriba exit. */
        /* while: repite el bloque mientras se cumpla 1. */
        while (1)
        {
            char comando[64];
            char *buffer = NULL;
            BigInt cantidad = {0};

            printf("%s%s%s\n", "", "", "");
            printf("%s%s%s",
                   TR("Cantidad para ", "Quantity for "),
                   monedas->items[i].digits,
                   TR(" c (volver/modo/gui/salir): ", " c (back/mode/gui/exit): "));
            /* Atrapa la lectura bloqueando con I/O fgets. */
            /* if: comprueba !leer_linea(buffer, sizeof(buffer)) antes de ejecutar esta rama. */
            buffer = leer_linea_dinamica();
            if (buffer == NULL)
            {
                limpiar_arreglo(cantidades);
                return -1;
            }

            /* Si no ingreso un carajo. */
            /* if: comprueba buffer[0] == '\0' antes de ejecutar esta rama. */
            if (buffer[0] == '\0')
            {
                printf("%s\n", TR("Entrada vacia. Intente de nuevo.", "Empty input. Try again."));
                free(buffer);
                continue;
            }

            strncpy(comando, buffer, sizeof(comando) - 1);
            comando[sizeof(comando) - 1] = '\0';
            a_minusculas(comando);

            /* Salida de emergencia 1. */
            /* if: comprueba strcmp(comando, "salir") == 0 antes de ejecutar esta rama. */
            if (progvoraz_cmd_is_exit(comando))
            {
                free(buffer);
                limpiar_arreglo(cantidades);
                return 4;
            }
            /* Salida emergencia 2. */
            /* if: comprueba strcmp(comando, "modo") == 0 antes de ejecutar esta rama. */
            if (progvoraz_cmd_is_mode(comando))
            {
                free(buffer);
                limpiar_arreglo(cantidades);
                return 3;
            }
            /* Salida de emergencia 3. */
            /* if: comprueba strcmp(comando, "volver") == 0 antes de ejecutar esta rama. */
            if (progvoraz_cmd_is_back(comando))
            {
                free(buffer);
                limpiar_arreglo(cantidades);
                return 2;
            }

            /* Trata de inyectar el valor del texto dentro de la maquinaria BigInt. */
            /* if: comprueba !bigint_init(&cantidad, buffer) antes de ejecutar esta rama. */
            if (!bigint_init(&cantidad, buffer))
            {
                printf("%s\n", TR("Cantidad invalida. Debe ser entero no negativo.",
                                  "Invalid quantity. It must be a non-negative integer."));
                free(buffer);
                continue;
            }

            /* Incorpora con deep copy el objeto bigint que si sirvio dentro del espacio correcto `i`. */
            /* if: comprueba !bigint_array_set(cantidades, i, &cantidad) antes de ejecutar esta rama. */
            if (!bigint_array_set(cantidades, i, &cantidad))
            {
                bigint_free(&cantidad);
                free(buffer);
                limpiar_arreglo(cantidades);
                return 0;
            }

            bigint_free(&cantidad);
            free(buffer);
            break;
        }
    }

    return 1;
}

/* calcular_total_valor: Multiplica el vector unitario (valores faciales) con el escalar unitario (unidades). Total acumulado. */
/* funcion calcular_total_valor: contiene la logica principal de esta operacion. */
static int calcular_total_valor(const BigIntArray *monedas, const BigIntArray *cantidades, BigInt *total)
{
    BigInt acumulado = {0};

    /* if: comprueba monedas == NULL || cantidades == NULL || total == NULL antes de ejecutar esta rama. */
    if (monedas == NULL || cantidades == NULL || total == NULL)
        return 0;
    /* if: comprueba monedas->len != cantidades->len antes de ejecutar esta rama. */
    if (monedas->len != cantidades->len)
        return 0;

    /* if: comprueba !bigint_init(&acumulado, "0") antes de ejecutar esta rama. */
    if (!bigint_init(&acumulado, "0"))
        return 0;

    /* for: itera segun size_t i = 0; i < monedas->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < monedas->len; i++)
    {
        BigInt parcial = {0};
        BigInt nuevoTotal = {0};

        /* if: comprueba bigint_is_zero(&cantidades->items[i]) antes de ejecutar esta rama. */
        if (bigint_is_zero(&cantidades->items[i]))
            continue;

        /* Ejecuta calculo del termino (Valor*Cant) en objeto 'parcial'. */
        /* if: comprueba !bigint_multiply(&monedas->items[i], &cantidades->items[i], &parcial) antes de ejecutar esta rama. */
        if (!bigint_multiply(&monedas->items[i], &cantidades->items[i], &parcial))
        {
            bigint_free(&acumulado);
            return 0;
        }

        /* Suma el parcial (Ej: $500) a lo que teniamos en total 'acumulado'. */
        /* if: comprueba !bigint_add(&acumulado, &parcial, &nuevoTotal) antes de ejecutar esta rama. */
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

/* funcion aplicar_cambio_especifico_stock: contiene la logica principal de esta operacion. */
static int aplicar_cambio_especifico_stock(const BigIntArray *monedas,
                                           const BigIntArray *stockActual,
                                           const BigIntArray *entregadas,
                                           const BigIntArray *devolucion,
                                           BigIntArray *stockNuevo,
                                           BigInt *totalEntregado,
                                           BigInt *totalDevolucion)
{
    /* if: comprueba monedas == NULL || stockActual == NULL || entregadas == NULL || devol... antes de ejecutar esta rama. */
    if (monedas == NULL || stockActual == NULL || entregadas == NULL || devolucion == NULL ||
        stockNuevo == NULL || totalEntregado == NULL || totalDevolucion == NULL)
        return 0;

    /* if: comprueba monedas->len != stockActual->len || monedas->len != entregadas->len |... antes de ejecutar esta rama. */
    if (monedas->len != stockActual->len || monedas->len != entregadas->len || monedas->len != devolucion->len)
        return 0;

    /* if: comprueba !calcular_total_valor(monedas, entregadas, totalEntregado) || antes de ejecutar esta rama. */
    if (!calcular_total_valor(monedas, entregadas, totalEntregado) ||
        !calcular_total_valor(monedas, devolucion, totalDevolucion))
        return 0;

    /* if: comprueba bigint_compare(totalEntregado, totalDevolucion) != 0 antes de ejecutar esta rama. */
    if (bigint_compare(totalEntregado, totalDevolucion) != 0)
        return 0;

    /* if: comprueba !copiar_arreglo_bigint(stockActual, stockNuevo) antes de ejecutar esta rama. */
    if (!copiar_arreglo_bigint(stockActual, stockNuevo))
        return 0;

    /* for: itera segun size_t i = 0; i < stockNuevo->len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < stockNuevo->len; i++)
    {
        BigInt trasEntrada = {0};
        BigInt trasSalida = {0};

        /* Agrega billetes: StockCajon_i = StockCajon_i + BilletesUsuarioEntrega_i */
        /* if: comprueba !bigint_add(&stockNuevo->items[i], &entregadas->items[i], &trasEntrada) antes de ejecutar esta rama. */
        if (!bigint_add(&stockNuevo->items[i], &entregadas->items[i], &trasEntrada))
        {
            limpiar_arreglo(stockNuevo);
            return 0;
        }

        /* Remueve billetes: chequear si existe la cantidad fisica en el cajon primero. Stock = StockCajon_i - BilletesNosPiden_i. */
        /* if: comprueba bigint_compare(&trasEntrada, &devolucion->items[i]) < 0 || antes de ejecutar esta rama. */
        if (bigint_compare(&trasEntrada, &devolucion->items[i]) < 0 ||
            !bigint_subtract(&trasEntrada, &devolucion->items[i], &trasSalida))
        {
            bigint_free(&trasEntrada);
            limpiar_arreglo(stockNuevo);
            return 0;
        }

        /* Escribe el resorte final tras entrada y salida al cajon `stockNuevo` en su slot. */
        /* if: comprueba !bigint_array_set(stockNuevo, i, &trasSalida) antes de ejecutar esta rama. */
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
/* funcion flujo_conversion_interactiva: contiene la logica principal de esta operacion. */
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

    printf("--- %s ---\n", TR("Conversion entre monedas (API)", "Currency conversion (API)"));
    printf("%s", TR("Moneda origen: ", "Source currency: "));
    // Este if es para leer la moneda origen desde la entrada estándar. Si no se puede leer, retorna 0 indicando fallo.
    /* if: comprueba !leer_linea(origen, sizeof(origen)) antes de ejecutar esta rama. */
    if (!leer_linea(origen, sizeof(origen)))
        return 0;
    // Este if verifica si la cadena de origen está vacía. Si es así, imprime un mensaje y retorna 0.
    /* if: comprueba origen[0] == '\0' antes de ejecutar esta rama. */
    if (origen[0] == '\0')
    {
        printf("%s\n", TR("Moneda origen vacia.", "Source currency is empty."));
        return 0;
    }
    normalizar_clave(origen, claveOrigen, sizeof(claveOrigen));

    estado = pedir_cantidad(&cantidad);
    /* Tabla de estados:
     * -1: Entrada finalizada
     *  0: Cantidad invalida
     *  2: Error interno
     *  3: Otro error
     */
    /* if: comprueba estado == -1 antes de ejecutar esta rama. */
    if (estado == -1)
    {
        printf("%s\n", TR("Entrada finalizada.", "Input ended."));
        bigint_free(&cantidad);
        return 0;
    }
    /* if: comprueba estado == 2 || estado == 3 antes de ejecutar esta rama. */
    if (estado == 2 || estado == 3)
    {
        bigint_free(&cantidad);
        return 0;
    }
    /* if: comprueba estado == 0 antes de ejecutar esta rama. */
    if (estado == 0)
    {
        printf("%s\n", TR("Cantidad invalida.", "Invalid amount."));
        bigint_free(&cantidad);
        return 0;
    }

    printf("%s", TR("Moneda destino: ", "Target currency: "));
    /* if: comprueba !leer_linea(destino, sizeof(destino)) antes de ejecutar esta rama. */
    if (!leer_linea(destino, sizeof(destino)))
    {
        bigint_free(&cantidad);
        return 0;
    }
    /* if: comprueba destino[0] == '\0' antes de ejecutar esta rama. */
    if (destino[0] == '\0')
    {
        printf("%s\n", TR("Moneda destino vacia.", "Target currency is empty."));
        bigint_free(&cantidad);
        return 0;
    }
    normalizar_clave(destino, claveDestino, sizeof(claveDestino));

    printf("%s", TR("Usar stock de destino? (s/n): ", "Use destination stock? (y/n): "));
    /* if: comprueba !leer_linea(linea, sizeof(linea)) antes de ejecutar esta rama. */
    if (!leer_linea(linea, sizeof(linea)))
    {
        bigint_free(&cantidad);
        return 0;
    }
    a_minusculas(linea);
    /* if: comprueba linea[0] == 's' || linea[0] == 'y' antes de ejecutar esta rama. */
    if (linea[0] == 's' || linea[0] == 'y')
        usarStock = 1;

    /* if: comprueba !fetch_exchange_rate(claveOrigen, claveDestino, &tasa) antes de ejecutar esta rama. */
    if (!fetch_exchange_rate(claveOrigen, claveDestino, &tasa))
    {
        printf("%s %s -> %s\n", TR("No se pudo obtener la tasa de cambio para", "Could not get the exchange rate for"), claveOrigen, claveDestino);
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
        /* if: comprueba !bigint_init(&cantidadDestino, tmp) antes de ejecutar esta rama. */
        if (!bigint_init(&cantidadDestino, tmp))
        {
            printf("%s\n", TR("Error interno al convertir monto destino.", "Internal error while converting the target amount."));
            bigint_free(&cantidad);
            return 0;
        }
    }

    /* Cargar denominaciones destino. */
    /* if: comprueba !validar_consistencia_moneda(claveDestino) antes de ejecutar esta rama. */
    if (!validar_consistencia_moneda(claveDestino))
    {
        printf("%s\n", TR("Moneda destino inconsistente o no encontrada.", "Target currency is inconsistent or was not found."));
        goto cleanup;
    }

    /* if: comprueba !cargar_denominaciones_moneda(claveDestino, &monedas) antes de ejecutar esta rama. */
    if (!cargar_denominaciones_moneda(claveDestino, &monedas))
    {
        printf("%s\n", TR("No se encontraron denominaciones para la moneda destino.", "No denominations were found for the target currency."));
        goto cleanup;
    }

    /* if: comprueba usarStock antes de ejecutar esta rama. */
    if (usarStock)
    {
        /* if: comprueba !cargar_stock_moneda(claveDestino, &stock) antes de ejecutar esta rama. */
        if (!cargar_stock_moneda(claveDestino, &stock))
        {
            printf("%s\n", TR("No se encontro stock para la moneda destino.", "No stock was found for the target currency."));
            goto cleanup;
        }
    }

    /* if: comprueba usarStock antes de ejecutar esta rama. */
    if (usarStock)
        estado = calcular_cambio_optimo_stock(&cantidadDestino, &monedas, &stock, &solucion);
    else
        estado = calcular_cambio_optimo(&cantidadDestino, &monedas, &solucion);

    /* if: comprueba estado antes de ejecutar esta rama. */
    if (estado)
    {
        printf("%s %s %s-centimos -> %s %s-centimos | %s=%.6f\n",
               TR("Conversion aplicada:", "Conversion applied:"),
               cantidad.digits, claveOrigen, cantidadDestino.digits, claveDestino,
               TR("tasa", "rate"), tasa);
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
        printf("%s\n", TR("No se pudo calcular el cambio en la moneda destino.",
                          "Could not calculate the change in the target currency."));
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
/* funcion app_console_run: contiene la logica principal de esta operacion. */
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
    /* while: repite el bloque mientras se cumpla ejecutando. */
    while (ejecutando)
    {
        /* Si no hay una opcion seleccionada, estamos en la pantalla inicial de Menu. */
        /* if: comprueba opcion != 'a' && opcion != 'b' && opcion != 'c' antes de ejecutar esta rama. */
        if (opcion != 'a' && opcion != 'b' && opcion != 'c')
        {
            /* Bucle de sub-menu inicial de captura de modalidad. */
            /* while: repite el bloque mientras se cumpla 1. */
            while (1)
            {
                opcion = pedir_opcion();
                /* if: comprueba opcion == 'a' || opcion == 'b' || opcion == 'c' || opcion == 'x' antes de ejecutar esta rama. */
                if (opcion == 'a' || opcion == 'b' || opcion == 'c' || opcion == 'x')
                    break;

                /* if: comprueba opcion == -3 antes de ejecutar esta rama. */
                if (opcion == -3)
                {
                    mostrar_historial_transacciones();
                    pausar_pantalla();
                    continue;
                }

                /* if: comprueba opcion == -4 antes de ejecutar esta rama. */
                if (opcion == -4)
                {
                    mostrar_resumen_desde_menu_principal();
                    pausar_pantalla();
                    continue;
                }

                /* if: comprueba opcion == -5 antes de ejecutar esta rama. */
                if (opcion == -5)
                {
                    gestionar_operacion_global("snapshot");
                    pausar_pantalla();
                    continue;
                }

                /* if: comprueba opcion == -6 antes de ejecutar esta rama. */
                if (opcion == -6)
                {
                    gestionar_operacion_global("restaurar");
                    pausar_pantalla();
                    continue;
                }

                /* if: comprueba opcion == -7 antes de ejecutar esta rama. */
                if (opcion == -7)
                {
                    gestionar_operacion_global("reporte");
                    pausar_pantalla();
                    continue;
                }

                /* if: comprueba opcion == -8 antes de ejecutar esta rama. */
                if (opcion == -8)
                {
                    gestionar_operacion_global("json");
                    pausar_pantalla();
                    continue;
                }

                /* Si devolvió el magik number -2 de salida manual. */
                /* if: comprueba opcion == -2 antes de ejecutar esta rama. */
                if (opcion == -2)
                {
                    ejecutando = 0;
                    break;
                }

                /* Si hubo desconexion de standard input (EOF). */
                /* if: comprueba opcion == -1 antes de ejecutar esta rama. */
                if (opcion == -1)
                {
                    printf("Entrada finalizada.\n");
                    ejecutando = 0;
                    break;
                }

                printf("%s\n", TR("Opcion no valida. Intente de nuevo.", "Invalid option. Try again."));
            }
        }

        /* if: comprueba !ejecutando antes de ejecutar esta rama. */
        if (!ejecutando)
            break;

        /* Manejo de opcion especial: conversion entre monedas. */
        /* if: comprueba opcion == 'x' antes de ejecutar esta rama. */
        if (opcion == 'x')
        {
            /* Ejecuta flujo interactivo de conversion y vuelve al menu principal. */
            flujo_conversion_interactiva();
            pausar_pantalla();
            opcion = 0;
            continue;
        }

        /* Bucle de Sesion de Trabajo (Donde se opera con monedas repetidamente). */
        /* while: repite el bloque mientras se cumpla 1. */
        while (1)
        {
            limpiar_arreglo(&stock);
            limpiar_arreglo(&monedas);

            mostrar_monedas_disponibles(opcion);
            printf("%s", TR("Nombre de la moneda ('modo', 'volver', 'gui' o 'salir'): ", "Currency name ('mode', 'back', 'gui' or 'exit'): "));

            /* Espera I/O bloqueante. */
            /* if: comprueba !leer_linea(moneda, sizeof(moneda)) antes de ejecutar esta rama. */
            if (!leer_linea(moneda, sizeof(moneda)))
            {
                printf("%s\n", TR("Entrada finalizada.", "Input ended."));
                ejecutando = 0;
                break;
            }

            /* Si no escribe nada... */
            /* if: comprueba moneda[0] == '\0' antes de ejecutar esta rama. */
            if (moneda[0] == '\0')
            {
                printf("%s\n", TR("Nombre de moneda no valido. Intente de nuevo.", "Invalid currency name. Try again."));
                continue;
            }

            normalizar_clave(moneda, monedaCmd, sizeof(monedaCmd));
            /* Evalua si el comando fue orden de fin total. */
            /* if: comprueba strcmp(monedaCmd, "salir") == 0 antes de ejecutar esta rama. */
            if (progvoraz_cmd_is_exit(monedaCmd))
            {
                ejecutando = 0;
                break;
            }

            /* Evalua si el usuario pidio cambiar de modo (Ej. Pasar del Ilimitado al Limitado). */
            /* if: comprueba strcmp(monedaCmd, "modo") == 0 || strcmp(monedaCmd, "volver") == 0 antes de ejecutar esta rama. */
            if (progvoraz_cmd_is_mode(monedaCmd) || progvoraz_cmd_is_back(monedaCmd))
            {
                opcion = 0;
                break;
            }

            {
                const char *mapped = progvoraz_map_currency_key(moneda);
                snprintf(monedaClave, sizeof(monedaClave), "%s", mapped != NULL ? mapped : "");
            }

            /* if: comprueba !validar_consistencia_moneda(monedaClave) antes de ejecutar esta rama. */
            if (!validar_consistencia_moneda(monedaClave))
            {
                printf("%s\n", TR("Moneda no valida o inconsistente (denominaciones/stock). Intente de nuevo.", "Invalid or inconsistent currency (denominations/stock). Try again."));
                continue;
            }

            /* if: comprueba !cargar_denominaciones_moneda(monedaClave, &monedas) antes de ejecutar esta rama. */
            if (!cargar_denominaciones_moneda(monedaClave, &monedas))
            {
                printf("%s\n", TR("No se encontro la moneda en monedas.txt. Intente de nuevo.", "Currency not found in monedas.txt. Try again."));
                continue;
            }

            /* if: comprueba opcion == 'b' || opcion == 'c' antes de ejecutar esta rama. */
            if (opcion == 'b' || opcion == 'c')
            {
                /* if: comprueba !cargar_stock_moneda(monedaClave, &stock) antes de ejecutar esta rama. */
                if (!cargar_stock_moneda(monedaClave, &stock))
                {
                    limpiar_arreglo(&monedas);
                    printf("%s\n", TR("No se encontro stock para la moneda seleccionada. Intente de nuevo.", "No stock found for the selected currency. Try again."));
                    continue;
                }
            }

            /* Bucle de Sub-Operacion Activa (Ya con las DB cargadas en memoria y modo listo). */
            /* while: repite el bloque mientras se cumpla 1. */
            while (1)
            {
                /* Caso MODO ADMINISTRADOR (Opcion C). */
                /* if: comprueba opcion == 'c' antes de ejecutar esta rama. */
                if (opcion == 'c')
                {
                    char accion[32];
                    int esSuma;
                    size_t idxDenom;
                    int estadoIndice;
                    BigInt delta = {0};
                    int estadoDelta;

                    imprimir_stock_administrador(&monedas, &stock);
                    printf("%s", TR("Accion admin (anadir/quitar/historial/resumen, volver, modo, gui, salir): ",
                                   "Admin action (add/remove/history/summary, back, mode, gui, exit): "));

                    /* Bloqueo en consola. */
                    /* if: comprueba !leer_linea(accion, sizeof(accion)) antes de ejecutar esta rama. */
                    if (!leer_linea(accion, sizeof(accion)))
                    {
                        printf("%s\n", TR("Entrada finalizada.", "Input ended."));
                        ejecutando = 0;
                        break;
                    }

                    a_minusculas(accion);
                    /* Chequeo de keywords para la accion general. */
                    /* if: comprueba strcmp(accion, "salir") == 0 antes de ejecutar esta rama. */
                    if (progvoraz_cmd_is_exit(accion))
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Chequeo keywords sub_ui. */
                    /* if: comprueba strcmp(accion, "modo") == 0 antes de ejecutar esta rama. */
                    if (progvoraz_cmd_is_mode(accion))
                    {
                        opcion = 0;
                        break;
                    }
                    /* if: comprueba strcmp(accion, "volver") == 0 antes de ejecutar esta rama. */
                    if (progvoraz_cmd_is_back(accion))
                        break;

                    /* if: comprueba strcmp(accion, "historial") == 0 antes de ejecutar esta rama. */
                    if (progvoraz_cmd_is_history(accion))
                    {
                        mostrar_historial_transacciones();
                        continue;
                    }

                    /* if: comprueba strcmp(accion, "resumen") == 0 antes de ejecutar esta rama. */
                    if (progvoraz_cmd_is_summary(accion))
                    {
                        mostrar_resumen_moneda(monedaClave, &monedas, &stock, 1);
                        continue;
                    }

                    /* Set de banderillas de orden. */
                    /* if: comprueba strcmp(accion, "anadir") == 0 antes de ejecutar esta rama. */
                    if (progvoraz_cmd_is_add(accion))
                        esSuma = 1;
                    /* if: comprueba strcmp(accion, "quitar") == 0 antes de continuar con esta alternativa. */
                    else if (progvoraz_cmd_is_remove(accion))
                        esSuma = 0;
                    else
                    {
                        printf("%s\n", TR("Accion invalida.", "Invalid action."));
                        continue;
                    }

                    estadoIndice = pedir_indice_denominacion(monedas.len, &idxDenom);
                    /* Switch de validacion del output magik de la UI: */
                    /* if: comprueba estadoIndice == -1 antes de ejecutar esta rama. */
                    if (estadoIndice == -1)
                    {
                        printf("%s\n", TR("Entrada finalizada.", "Input ended."));
                        ejecutando = 0;
                        break;
                    }
                    /* Si pidio salir... */
                    /* if: comprueba estadoIndice == 4 antes de ejecutar esta rama. */
                    if (estadoIndice == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Si pidio salir a UI 1. */
                    /* if: comprueba estadoIndice == 3 antes de ejecutar esta rama. */
                    if (estadoIndice == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    /* if: comprueba estadoIndice == 2 antes de ejecutar esta rama. */
                    if (estadoIndice == 2)
                        continue;
                    /* Si lo que introdujo ni siquiera fue un numero parseable... */
                    /* if: comprueba estadoIndice == 0 antes de ejecutar esta rama. */
                    if (estadoIndice == 0)
                    {
                        printf("%s\n", TR("Indice invalido.", "Invalid index."));
                        continue;
                    }

                    estadoDelta = pedir_cantidad_admin(&delta);
                    /* Mismas evaluaciones de salida temprana por keyword... */
                    /* if: comprueba estadoDelta == -1 antes de ejecutar esta rama. */
                    if (estadoDelta == -1)
                    {
                        printf("%s\n", TR("Entrada finalizada.", "Input ended."));
                        bigint_free(&delta);
                        ejecutando = 0;
                        break;
                    }
                    /* Validacion salida. */
                    /* if: comprueba estadoDelta == 4 antes de ejecutar esta rama. */
                    if (estadoDelta == 4)
                    {
                        bigint_free(&delta);
                        ejecutando = 0;
                        break;
                    }
                    /* Validacion menu 1. */
                    /* if: comprueba estadoDelta == 3 antes de ejecutar esta rama. */
                    if (estadoDelta == 3)
                    {
                        bigint_free(&delta);
                        opcion = 0;
                        break;
                    }
                    /* Validacion menu divisas. */
                    /* if: comprueba estadoDelta == 2 antes de ejecutar esta rama. */
                    if (estadoDelta == 2)
                    {
                        bigint_free(&delta);
                        continue;
                    }
                    /* Validacion por basura NaN. */
                    /* if: comprueba estadoDelta == 0 antes de ejecutar esta rama. */
                    if (estadoDelta == 0)
                    {
                        bigint_free(&delta);
                        printf("%s\n", TR("Cantidad invalida.", "Invalid quantity."));
                        continue;
                    }

                    /* Efectua in-memory la alteracion al array del stock activo. */
                    /* if: comprueba !aplicar_cambio_administrador(&stock, idxDenom, &delta, esSuma) antes de ejecutar esta rama. */
                    if (!aplicar_cambio_administrador(&stock, idxDenom, &delta, esSuma))
                    {
                        bigint_free(&delta);
                        printf("%s\n", TR("No se pudo aplicar el cambio de stock (revisa disponibilidad).",
                                          "Could not apply the stock change (check availability)."));
                        continue;
                    }

                    /* Consolida finalmente el array recien modificado en RAM hacia el Disco Duro. */
                    /* if: comprueba !actualizar_stock_moneda(monedaClave, &stock) antes de ejecutar esta rama. */
                    if (!actualizar_stock_moneda(monedaClave, &stock))
                        printf("%s\n", TR("No se pudo actualizar el archivo de stock.", "Could not update the stock file."));
                    else
                    {
                        registrar_historialf("Admin %s | Moneda=%s | Denom=%s c | Cantidad=%s",
                                             esSuma ? "ANADIR" : "QUITAR",
                                             monedaClave,
                                             monedas.items[idxDenom].digits,
                                             delta.digits);
                        printf("%s\n", TR("Stock actualizado correctamente.", "Stock updated successfully."));
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
                /* if: comprueba opcion == 'a' || opcion == 'b' antes de ejecutar esta rama. */
                if (opcion == 'a' || opcion == 'b')
                {
                    subopcionStock = pedir_subopcion_cambio(opcion == 'b');

                    /* Validacion estricta magiks menu... */
                    /* if: comprueba subopcionStock == -1 antes de ejecutar esta rama. */
                    if (subopcionStock == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* Valida magiks.. */
                    /* if: comprueba subopcionStock == 4 antes de ejecutar esta rama. */
                    if (subopcionStock == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Valida.. */
                    /* if: comprueba subopcionStock == 3 antes de ejecutar esta rama. */
                    if (subopcionStock == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    /* if: comprueba subopcionStock == 2 antes de ejecutar esta rama. */
                    if (subopcionStock == 2)
                        break;
                    /* Valida fallo. */
                    /* if: comprueba subopcionStock == 0 antes de ejecutar esta rama. */
                    if (subopcionStock == 0)
                    {
                        printf("%s\n", TR("Subopcion invalida.", "Invalid sub-option."));
                        continue;
                    }

                    /* if: comprueba subopcionStock == 6 antes de ejecutar esta rama. */
                    if (subopcionStock == 6)
                    {
                        mostrar_historial_transacciones();
                        continue;
                    }

                    /* if: comprueba subopcionStock == 7 antes de ejecutar esta rama. */
                    if (subopcionStock == 7)
                    {
                        mostrar_resumen_moneda(monedaClave, &monedas, opcion == 'b' ? &stock : NULL, opcion == 'b');
                        continue;
                    }
                }

                /* Flujo ESPECIAL: Modo Usuario Infinito + Variante Intercambio Multi-billete. */
                /* if: comprueba opcion == 'a' && subopcionStock == 5 antes de ejecutar esta rama. */
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
                    /* if: comprueba estadoEntrada == -1 antes de ejecutar esta rama. */
                    if (estadoEntrada == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* Exit... */
                    /* if: comprueba estadoEntrada == 4 antes de ejecutar esta rama. */
                    if (estadoEntrada == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* Menu app. */
                    /* if: comprueba estadoEntrada == 3 antes de ejecutar esta rama. */
                    if (estadoEntrada == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    /* if: comprueba estadoEntrada == 2 antes de ejecutar esta rama. */
                    if (estadoEntrada == 2)
                        continue;
                    /* Fallo genérico. */
                    /* if: comprueba estadoEntrada == 0 antes de ejecutar esta rama. */
                    if (estadoEntrada == 0)
                    {
                        printf("No se pudieron leer cantidades entregadas.\n");
                        continue;
                    }

                    estadoDevolucion = pedir_cantidades_por_denominacion(&monedas, "Cambio especifico solicitado (cantidades a devolver):", &devolucion);
                    /* Chequeo magiks... */
                    /* if: comprueba estadoDevolucion == -1 antes de ejecutar esta rama. */
                    if (estadoDevolucion == -1)
                    {
                        printf("Entrada finalizada.\n");
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* Exit. */
                    /* if: comprueba estadoDevolucion == 4 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 4)
                    {
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* App. */
                    /* if: comprueba estadoDevolucion == 3 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 3)
                    {
                        limpiar_arreglo(&entregadas);
                        opcion = 0;
                        break;
                    }
                    /* Volver. */
                    /* if: comprueba estadoDevolucion == 2 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 2)
                    {
                        limpiar_arreglo(&entregadas);
                        continue;
                    }
                    /* Error. */
                    /* if: comprueba estadoDevolucion == 0 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 0)
                    {
                        printf("%s\n", TR("No se pudieron leer cantidades de cambio solicitado.",
                                          "Could not read the requested change quantities."));
                        limpiar_arreglo(&entregadas);
                        continue;
                    }

                    /* Validador de consistencia monetaria pura (Total Depositado == Total Exigido). */
                    /* if: comprueba !validar_cambio_especifico_ilimitado(&monedas, &entregadas, &devoluci... antes de ejecutar esta rama. */
                    if (!validar_cambio_especifico_ilimitado(&monedas, &entregadas, &devolucion, &totalEntregado, &totalDevolucion))
                    {
                        printf("%s\n", TR("No se pudo aplicar cambio especifico en modo ilimitado. Verifica que el total entregado sea igual al total solicitado.",
                                          "Could not apply the specific change in unlimited mode. Verify that the delivered total matches the requested total."));
                        limpiar_arreglo(&entregadas);
                        limpiar_arreglo(&devolucion);
                        bigint_free(&totalEntregado);
                        bigint_free(&totalDevolucion);
                        continue;
                    }

                    printf("%s %s c | %s %s c\n",
                           TR("Cambio especifico aplicado en modo ilimitado. Total entregado:", "Specific change applied in unlimited mode. Delivered total:"),
                           totalEntregado.digits,
                           TR("Total devuelto:", "Returned total:"),
                           totalDevolucion.digits);
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
                /* if: comprueba opcion == 'b' && subopcionStock == 5 antes de ejecutar esta rama. */
                if (opcion == 'b' && subopcionStock == 5)
                {
                    BigIntArray entregadas = {0};
                    BigIntArray devolucion = {0};
                    BigIntArray stockNuevo = {0};
                    BigInt totalEntregado = {0};
                    BigInt totalDevolucion = {0};
                    int estadoEntrada;
                    int estadoDevolucion;

                    estadoEntrada = pedir_cantidades_por_denominacion(&monedas, TR("Monedas/Billetes entregados por el usuario:", "Coins/bills delivered by the user:"), &entregadas);
                    /* Magiks... */
                    /* if: comprueba estadoEntrada == -1 antes de ejecutar esta rama. */
                    if (estadoEntrada == -1)
                    {
                        printf("Entrada finalizada.\n");
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    /* if: comprueba estadoEntrada == 4 antes de ejecutar esta rama. */
                    if (estadoEntrada == 4)
                    {
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    /* if: comprueba estadoEntrada == 3 antes de ejecutar esta rama. */
                    if (estadoEntrada == 3)
                    {
                        opcion = 0;
                        break;
                    }
                    /* if: comprueba estadoEntrada == 2 antes de ejecutar esta rama. */
                    if (estadoEntrada == 2)
                        continue;
                    /* .. */
                    /* if: comprueba estadoEntrada == 0 antes de ejecutar esta rama. */
                    if (estadoEntrada == 0)
                    {
                        printf("%s\n", TR("No se pudieron leer cantidades entregadas.", "Could not read the delivered quantities."));
                        continue;
                    }

                    estadoDevolucion = pedir_cantidades_por_denominacion(&monedas, TR("Cambio especifico solicitado (cantidades a devolver):", "Specific requested change (quantities to return):"), &devolucion);
                    /* Magiks y purgados condicionales por cada break... */
                    /* if: comprueba estadoDevolucion == -1 antes de ejecutar esta rama. */
                    if (estadoDevolucion == -1)
                    {
                        printf("Entrada finalizada.\n");
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    /* if: comprueba estadoDevolucion == 4 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 4)
                    {
                        limpiar_arreglo(&entregadas);
                        ejecutando = 0;
                        break;
                    }
                    /* .. */
                    /* if: comprueba estadoDevolucion == 3 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 3)
                    {
                        limpiar_arreglo(&entregadas);
                        opcion = 0;
                        break;
                    }
                    /* .. */
                    /* if: comprueba estadoDevolucion == 2 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 2)
                    {
                        limpiar_arreglo(&entregadas);
                        continue;
                    }
                    /* .. */
                    /* if: comprueba estadoDevolucion == 0 antes de ejecutar esta rama. */
                    if (estadoDevolucion == 0)
                    {
                        printf("%s\n", TR("No se pudieron leer cantidades de cambio solicitado.",
                                          "Could not read the requested change quantities."));
                        limpiar_arreglo(&entregadas);
                        continue;
                    }

                    /* Ejecuta rutina compleja de validacion atomica (Balance total == Balance Devuelto AND StockFisico >= Billetes Devueltos). */
                    /* if: comprueba !aplicar_cambio_especifico_stock(&monedas, &stock, &entregadas, &devo... antes de ejecutar esta rama. */
                    if (!aplicar_cambio_especifico_stock(&monedas, &stock, &entregadas, &devolucion, &stockNuevo, &totalEntregado, &totalDevolucion))
                    {
                        printf("%s\n", TR("No se pudo aplicar cambio especifico. Verifica que el total entregado sea igual al total solicitado y que el stock alcance.",
                                          "Could not apply the specific change. Verify that the delivered total matches the requested total and that stock is sufficient."));
                        limpiar_arreglo(&entregadas);
                        limpiar_arreglo(&devolucion);
                        limpiar_arreglo(&stockNuevo);
                        bigint_free(&totalEntregado);
                        bigint_free(&totalDevolucion);
                        continue;
                    }

                    /* Intenta commitear el cambio al archivo de texto fisico en disco duro. */
                    /* if: comprueba !actualizar_stock_moneda(monedaClave, &stockNuevo) antes de ejecutar esta rama. */
                    if (!actualizar_stock_moneda(monedaClave, &stockNuevo))
                    {
                        printf("%s\n", TR("No se pudo actualizar el archivo de stock.", "Could not update the stock file."));
                        limpiar_arreglo(&entregadas);
                        limpiar_arreglo(&devolucion);
                        limpiar_arreglo(&stockNuevo);
                        bigint_free(&totalEntregado);
                        bigint_free(&totalDevolucion);
                        continue;
                    }

                    limpiar_arreglo(&stock);
                    stock = stockNuevo;

                    printf("%s %s c | %s %s c\n",
                           TR("Cambio especifico aplicado. Total entregado:", "Specific change applied. Delivered total:"),
                           totalEntregado.digits,
                           TR("Total devuelto:", "Returned total:"),
                           totalDevolucion.digits);
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
                /* if: comprueba estadoCantidad == -1 antes de ejecutar esta rama. */
                if (estadoCantidad == -1)
                {
                    printf("%s\n", TR("Entrada finalizada.", "Input ended."));
                    bigint_free(&cantidad);
                    ejecutando = 0;
                    break;
                }

                /* Magiks.. */
                /* if: comprueba estadoCantidad == 2 antes de ejecutar esta rama. */
                if (estadoCantidad == 2)
                {
                    bigint_free(&cantidad);
                    break;
                }

                /* Magiks.. */
                /* if: comprueba estadoCantidad == 3 antes de ejecutar esta rama. */
                if (estadoCantidad == 3)
                {
                    bigint_free(&cantidad);
                    ejecutando = 0;
                    break;
                }

                /* Magiks.. */
                /* if: comprueba estadoCantidad == 0 antes de ejecutar esta rama. */
                if (estadoCantidad == 0)
                {
                    printf("%s\n", TR("Entrada invalida. Introduzca un entero no negativo.",
                                      "Invalid input. Enter a non-negative integer."));
                    bigint_free(&cantidad);
                    continue;
                }

                usar_restriccion = (subopcionStock == 8);
                /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
                if (usar_restriccion)
                {
                    estadoRestriccion = pedir_restriccion_monedas(&min_monedas, &max_monedas);

                    /* if: comprueba estadoRestriccion == -1 antes de ejecutar esta rama. */
                    if (estadoRestriccion == -1)
                    {
                        printf("%s\n", TR("Entrada finalizada.", "Input ended."));
                        bigint_free(&cantidad);
                        ejecutando = 0;
                        break;
                    }
                    /* if: comprueba estadoRestriccion == 4 antes de ejecutar esta rama. */
                    if (estadoRestriccion == 4)
                    {
                        bigint_free(&cantidad);
                        ejecutando = 0;
                        break;
                    }
                    /* if: comprueba estadoRestriccion == 3 antes de ejecutar esta rama. */
                    if (estadoRestriccion == 3)
                    {
                        bigint_free(&cantidad);
                        opcion = 0;
                        break;
                    }
                    /* if: comprueba estadoRestriccion == 2 antes de ejecutar esta rama. */
                    if (estadoRestriccion == 2)
                    {
                        bigint_free(&cantidad);
                        continue;
                    }
                    /* if: comprueba estadoRestriccion == 0 antes de ejecutar esta rama. */
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
                /* if: comprueba opcion == 'a' antes de ejecutar esta rama. */
                if (opcion == 'a')
                {
                    /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
                    if (usar_restriccion)
                        resultado = calcular_cambio_optimo_con_rango(&cantidad, &monedas, min_monedas, max_monedas, &solucion);
                    else
                        resultado = calcular_cambio_optimo(&cantidad, &monedas, &solucion);

                    /* if: comprueba resultado antes de ejecutar esta rama. */
                    if (resultado)
                    {
                        imprimir_resultado(&monedas, &solucion, NULL, 0);
                        /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
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

                        /* if: comprueba okCercano antes de ejecutar esta rama. */
                        if (okCercano)
                        {
                            decision = preguntar_aceptar_sugerencia(&cantidad, &montoCubierto, 0);

                            /* if: comprueba decision == -1 antes de ejecutar esta rama. */
                            if (decision == -1)
                            {
                                printf("Entrada finalizada.\n");
                                bigint_free(&montoCubierto);
                                limpiar_arreglo(&sugerencia);
                                bigint_free(&cantidad);
                                ejecutando = 0;
                                break;
                            }

                            /* if: comprueba decision == 1 antes de ejecutar esta rama. */
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
                            /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
                            if (usar_restriccion)
                                printf("No existe cambio exacto para esa cantidad con la restriccion indicada.\n");
                            else
                                printf("No existe cambio exacto para esa cantidad.\n");
                        }
                    }
                }
                else
                {
                    /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
                    if (usar_restriccion)
                        resultado = calcular_cambio_optimo_stock_con_rango(&cantidad, &monedas, &stock, min_monedas, max_monedas, &solucion);
                    else
                        resultado = calcular_cambio_optimo_stock(&cantidad, &monedas, &stock, &solucion);

                    /* Si encontro una respuesta viable. */
                    /* if: comprueba resultado antes de ejecutar esta rama. */
                    if (resultado)
                    {
                        /* if: comprueba !aplicar_devolucion_stock(monedaClave, &stock, &solucion) antes de ejecutar esta rama. */
                        if (!aplicar_devolucion_stock(monedaClave, &stock, &solucion))
                        {
                            printf("No se pudo aplicar la devolucion al stock.\n");
                            limpiar_arreglo(&solucion);
                            bigint_free(&cantidad);
                            continue;
                        }

                        imprimir_resultado(&monedas, &solucion, &stock, 1);
                        /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
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

                        /* if: comprueba okCercano antes de ejecutar esta rama. */
                        if (okCercano)
                        {
                            decision = preguntar_aceptar_sugerencia(&cantidad, &montoCubierto, 1);

                            /* if: comprueba decision == -1 antes de ejecutar esta rama. */
                            if (decision == -1)
                            {
                                printf("Entrada finalizada.\n");
                                bigint_free(&montoCubierto);
                                limpiar_arreglo(&sugerencia);
                                bigint_free(&cantidad);
                                ejecutando = 0;
                                break;
                            }

                            /* if: comprueba decision == 1 antes de ejecutar esta rama. */
                            if (decision == 1)
                            {
                                /* if: comprueba !aplicar_devolucion_stock(monedaClave, &stock, &sugerencia) antes de ejecutar esta rama. */
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
                            /* if: comprueba usar_restriccion antes de ejecutar esta rama. */
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

            /* if: comprueba !ejecutando antes de ejecutar esta rama. */
            if (!ejecutando)
                break;

            /* Vuelve a seleccionar moneda dentro del mismo modo. */
            break;
        }

        /* if: comprueba !ejecutando antes de ejecutar esta rama. */
        if (!ejecutando)
            break;

        /* Si opcion se reinicio a 0, el usuario pidio cambiar de modo. */
        /* if: comprueba opcion == 0 antes de ejecutar esta rama. */
        if (opcion == 0)
            continue;
    }

    limpiar_arreglo(&stock);
    limpiar_arreglo(&monedas);
    printf("%s\n", TR("Gracias por utilizar este programa.", "Thank you for using this program."));
    return EXIT_SUCCESS;
}

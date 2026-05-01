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

/*
 * Muestra menu inicial y devuelve opcion normalizada en minuscula.
 * Retorna:
 * - 'a', 'b' o 'c' si la entrada es valida
 * - -3 si usuario pide ver historial
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
    dibujar_linea();
    printf("Opcion (a/b/c/h, historial o salir): ");

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
static void limpiar_arreglo(BigIntArray *arr)
{
    if (arr != NULL)
        bigint_array_free(arr);
}

/* imprimir_resultado: Muestra la solucion del calculo de forma entendible por consola. */
static void imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock)
{
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

/* pedir_subopcion_cambio: Interfaz para modo "caja registradora" avanzado del modo limitado. */
static int pedir_subopcion_cambio(void)
{
    char buffer[32];
    char comando[32];

    printf("Subopcion cambio (tradicional|1 / especifico|2 / historial|3, volver, modo o salir): ");
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

    return 0;
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
                if (opcion == 'a' || opcion == 'b' || opcion == 'c')
                    break;

                if (opcion == -3)
                {
                    mostrar_historial_transacciones();
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

            /* Intenta ir a disco para parsear la BD de dicha moneda. */
            if (!cargar_denominaciones_moneda(monedaClave, &monedas))
            {
                printf("No se encontro la moneda en monedas.txt. Intente de nuevo.\n");
                continue;
            }

            /* Si estamos en los modos que requiren base de datos de Stock... */
            if (opcion == 'b' || opcion == 'c')
            {
                /* Intentamos cargar la segunda base de datos en paralelo. */
                if (!cargar_stock_moneda(monedaClave, &stock))
                {
                    limpiar_arreglo(&monedas);
                    printf("No se encontro stock para la moneda seleccionada. Intente de nuevo.\n");
                    continue;
                }

                /* Validador de salud de Bases de Datos cruzada. */
                if (stock.len != monedas.len)
                {
                    limpiar_arreglo(&stock);
                    limpiar_arreglo(&monedas);
                    printf("Inconsistencia entre denominaciones y stock. Intente otra moneda.\n");
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
                    printf("Accion admin (anadir/quitar/historial, volver, modo, salir): ");

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
                int resultado;
                BigIntArray solucion = {0};
                int subopcionStock = 1;

                /* Caso MODO USUARIO NORMAL (Opciones A y B). */
                if (opcion == 'a' || opcion == 'b')
                {
                    subopcionStock = pedir_subopcion_cambio();

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

                /* Si estamos en modo de monedas ilimitado (El Algoritmo Clásico Original). */
                if (opcion == 'a')
                {
                    resultado = calcular_cambio_optimo(&cantidad, &monedas, &solucion);
                    if (resultado)
                    {
                        imprimir_resultado(&monedas, &solucion, NULL, 0);
                        registrar_historialf("Cambio tradicional ilimitado | Moneda=%s | Monto=%s c",
                                             monedaClave,
                                             cantidad.digits);
                    }
                    else
                        printf("No existe cambio exacto para esa cantidad.\n");
                }
                else
                {
                    resultado = calcular_cambio_optimo_stock(&cantidad, &monedas, &stock, &solucion);
                    /* Si encontro una respuesta viable. */
                    if (resultado)
                    {
                        BigIntArray stockNuevo = {0};
                        int okStockNuevo = copiar_arreglo_bigint(&stock, &stockNuevo);

                        if (!okStockNuevo)
                        {
                            printf("No se pudo preparar la actualizacion de stock.\n");
                            limpiar_arreglo(&solucion);
                            bigint_free(&cantidad);
                            continue;
                        }

                        for (size_t i = 0; i < stockNuevo.len; i++)
                        {
                            BigInt restante = {0};

                            if (bigint_is_zero(&solucion.items[i]))
                                continue;

                            if (!bigint_subtract(&stockNuevo.items[i], &solucion.items[i], &restante) ||
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
                            printf("No se pudo aplicar la devolucion al stock.\n");
                            limpiar_arreglo(&solucion);
                            bigint_free(&cantidad);
                            continue;
                        }

                        if (!actualizar_stock_moneda(monedaClave, &stockNuevo))
                        {
                            limpiar_arreglo(&stockNuevo);
                            printf("No se pudo actualizar el archivo de stock.\n");
                            limpiar_arreglo(&solucion);
                            bigint_free(&cantidad);
                            continue;
                        }

                        limpiar_arreglo(&stock);
                        stock = stockNuevo;
                        imprimir_resultado(&monedas, &solucion, &stock, 1);
                        registrar_historialf("Cambio tradicional con stock | Moneda=%s | Monto=%s c",
                                             monedaClave,
                                             cantidad.digits);
                    }
                    else
                    {
                        printf("No existe cambio exacto para esa cantidad con el stock actual.\n");
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

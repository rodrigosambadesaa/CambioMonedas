#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include "app_console.h"
#include "batch_cli.h"
#include "json_io.h"
#include "moneda_gestion.h"
#include "server_http.h"
#include "exchange_api.h"
#include "algoritmo_cambio.h"
#include "gui_launcher.h"

// Explicación del propósito de cada variable de este archivo, función por función:
// Función 1: map_currency_key
//     - input: cadena de entrada que representa un código ISO o nombre corto de moneda.
//     - tmp: buffer estático para almacenar la clave mapeada de la moneda.
// Función 2: print_help
//     - prog: nombre del programa para mostrar en la ayuda.
// Función 3: print_version
//     - f: puntero a archivo para leer la versión desde VERSIONES.md.
//     - linea: buffer para almacenar la línea leída del archivo de versión.
// Función 4: file_exists

/* map_currency_key: convierte un codigo ISO o nombre corto a la clave usada en monedas.txt
 * Devuelve puntero a cadena estatica o NULL si no hay mapeo. */
/* funcion map_currency_key: contiene la logica principal de esta operacion. */
static const char *map_currency_key(const char *input)
{
    static char tmp[64];
    /* if: comprueba !input || !*input antes de ejecutar esta rama. */
    if (!input || !*input)
        return NULL;

    /* Comparaciones comunes (case-insensitive) */
    /* if: comprueba strcasecmp(input, "EUR") == 0 || strcasecmp(input, "E") == 0 || strca... antes de ejecutar esta rama. */
    if (strcasecmp(input, "EUR") == 0 || strcasecmp(input, "E") == 0 || strcasecmp(input, "EURO") == 0)
        return "euro";
    /* if: comprueba strcasecmp(input, "USD") == 0 || strcasecmp(input, "US") == 0 || strc... antes de ejecutar esta rama. */
    if (strcasecmp(input, "USD") == 0 || strcasecmp(input, "US") == 0 || strcasecmp(input, "DOLAR") == 0 || strcasecmp(input, "DOLLAR") == 0)
        return "dolar";
    /* if: comprueba strcasecmp(input, "JPY") == 0 || strcasecmp(input, "YEN") == 0 antes de ejecutar esta rama. */
    if (strcasecmp(input, "JPY") == 0 || strcasecmp(input, "YEN") == 0)
        return "yen";
    /* if: comprueba strcasecmp(input, "GBP") == 0 || strcasecmp(input, "GBP") == 0 || str... antes de ejecutar esta rama. */
    if (strcasecmp(input, "GBP") == 0 || strcasecmp(input, "GBP") == 0 || strcasecmp(input, "LIBRA") == 0 || strcasecmp(input, "POUND") == 0)
        return "libra";
    /* if: comprueba strcasecmp(input, "CHF") == 0 || strcasecmp(input, "FRANC") == 0 || s... antes de ejecutar esta rama. */
    if (strcasecmp(input, "CHF") == 0 || strcasecmp(input, "FRANC") == 0 || strcasecmp(input, "FRANCO_SUIZO") == 0)
        return "franco_suizo";
    /* if: comprueba strcasecmp(input, "CAD") == 0 antes de ejecutar esta rama. */
    if (strcasecmp(input, "CAD") == 0)
        return "dolar_canadiense";
    /* if: comprueba strcasecmp(input, "AUD") == 0 antes de ejecutar esta rama. */
    if (strcasecmp(input, "AUD") == 0)
        return "dolar_australiano";
    /* if: comprueba strcasecmp(input, "CNY") == 0 || strcasecmp(input, "RMB") == 0 antes de ejecutar esta rama. */
    if (strcasecmp(input, "CNY") == 0 || strcasecmp(input, "RMB") == 0)
        return "yuan_chino";
    /* if: comprueba strcasecmp(input, "MXN") == 0 antes de ejecutar esta rama. */
    if (strcasecmp(input, "MXN") == 0)
        return "peso_mexicano";

    /* Fallback: convierte a minusculas y reemplaza espacios por _ */
    size_t i, j = 0;
    /* for: itera segun i = 0; input[i] != '\0' && j + 1 < sizeof(tmp); i++ para recorrer el bloque. */
    for (i = 0; input[i] != '\0' && j + 1 < sizeof(tmp); i++)
    {
        char c = input[i];
        /* if: comprueba c == ' ' antes de ejecutar esta rama. */
        if (c == ' ')
            tmp[j++] = '_';
        else
            tmp[j++] = (char)tolower((unsigned char)c);
    }
    tmp[j] = '\0';
    return tmp;
}

#ifndef _WIN32
extern int setenv(const char *name, const char *value, int overwrite);
#endif

/* funcion print_help: contiene la logica principal de esta operacion. */
static void print_help(const char *prog)
{
    printf("Uso: %s [opciones]\n", prog);
    printf("Opciones:\n");
    printf("  --input <ruta>    Procesar archivo CSV en modo batch.\n");
    printf("  --output <ruta>   Archivo de salida CSV (por defecto stdout).\n");
    printf("  --log <ruta>      Archivo de log para modo batch/stream.\n");
    printf("  --gui             Lanzar interfaz GUI (progvoraz_gui).\n");
    printf("  --docker          Modo no interactivo legacy (lee CSV desde stdin y escribe CSV a stdout).\n");
    printf("  --stream          Alias nativo de --docker para stdin/stdout CSV.\n");
    printf("  --json            Habilitar salida/flags JSON (variable de entorno).\n");
    printf("  --config <ruta>   Archivo de configuracion alternativo.\n");
    printf("  --export-stock-json <ruta>  Exportar stock actual a JSON.\n");
    printf("  --export-report <ruta>      Exportar reporte global de inventario.\n");
    printf("  --server          Iniciar servidor HTTP local.\n");
    printf("  --port <n>        Puerto para --server (por defecto 8080).\n");
    printf("  --convert <origen> <destino> <monto_centimos>\n");
    printf("                    Convierte y descompone un monto en la moneda destino.\n");
    printf("  --convert-stock   Usa stock real al calcular el cambio de --convert.\n");
    printf("  --version         Muestra la version y sale.\n");
    printf("  --help            Muestra esta ayuda.\n");
}

/* funcion print_version: contiene la logica principal de esta operacion. */
static void print_version(void)
{
    FILE *f = fopen("VERSIONES.md", "r");
    /* if: comprueba f antes de ejecutar esta rama. */
    if (f)
    {
        char linea[256];
        /* if: comprueba fgets(linea, sizeof(linea), f) antes de ejecutar esta rama. */
        if (fgets(linea, sizeof(linea), f))
        {
            /* Imprime la primera linea util como version */
            printf("%s", linea);
        }
        else
        {
            printf("ProgVoraz (version desconocida)\n");
        }
        fclose(f);
    }
    else
    {
        printf("ProgVoraz (version desconocida)\n");
    }
}

/* funcion main: contiene la logica principal de esta operacion. */
int main(int argc, char **argv)
{
    const char *input = NULL;
    const char *output = NULL;
    const char *logpath = NULL;
    const char *configpath = NULL;
    int gui_flag = 0;
    int docker_flag = 0;
    int json_flag = 0;

    const char *export_stock_json_path = NULL;
    const char *export_report_path = NULL;

    int server_flag = 0;
    const char *server_port = NULL;
    int convert_flag = 0;
    const char *convert_from = NULL;
    const char *convert_to = NULL;
    const char *convert_amount = NULL; /* in cents */
    int convert_use_stock = 0;

    /* for: itera segun int i = 1; i < argc; i++ para recorrer el bloque. */
    for (int i = 1; i < argc; i++)
    {
        /* if: comprueba strcmp(argv[i], "--input") == 0 && i + 1 < argc antes de ejecutar esta rama. */
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            input = argv[++i];
        /* if: comprueba strcmp(argv[i], "--output") == 0 && i + 1 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output = argv[++i];
        /* if: comprueba strcmp(argv[i], "--log") == 0 && i + 1 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc)
            logpath = argv[++i];
        /* if: comprueba strcmp(argv[i], "--config") == 0 && i + 1 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
            configpath = argv[++i];
        /* if: comprueba strcmp(argv[i], "--gui") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--gui") == 0 || strcmp(argv[i], "gui") == 0)
            gui_flag = 1;
        /* if: comprueba strcmp(argv[i], "--docker") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--docker") == 0 || strcmp(argv[i], "--stream") == 0)
            docker_flag = 1;
        /* if: comprueba strcmp(argv[i], "--json") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--json") == 0)
            json_flag = 1;
        /* if: comprueba strcmp(argv[i], "--export-stock-json") == 0 && i + 1 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--export-stock-json") == 0 && i + 1 < argc)
            export_stock_json_path = argv[++i];
        /* if: comprueba strcmp(argv[i], "--export-report") == 0 && i + 1 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--export-report") == 0 && i + 1 < argc)
            export_report_path = argv[++i];
        /* if: comprueba strcmp(argv[i], "--server") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--server") == 0)
            server_flag = 1;
        /* if: comprueba strcmp(argv[i], "--port") == 0 && i + 1 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            server_port = argv[++i];
        /* if: comprueba strcmp(argv[i], "--convert") == 0 && i + 3 < argc antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--convert") == 0 && i + 3 < argc)
        {
            convert_flag = 1;
            convert_from = argv[++i];
            convert_to = argv[++i];
            convert_amount = argv[++i];
        }
        /* if: comprueba strcmp(argv[i], "--convert-stock") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--convert-stock") == 0)
        {
            convert_use_stock = 1;
        }
        /* if: comprueba strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)
        {
            print_version();
            return 0;
        }
        /* if: comprueba strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 antes de continuar con esta alternativa. */
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
    }

    /* Exportar variables de entorno para que otros componentes las lean si es necesario. */
#ifdef _WIN32
    /* if: comprueba json_flag antes de ejecutar esta rama. */
    if (json_flag)
        _putenv_s("PROGVORAZ_JSON", "1");
    /* if: comprueba configpath antes de ejecutar esta rama. */
    if (configpath)
        _putenv_s("PROGVORAZ_CONFIG", configpath);
#else
    /* if: comprueba json_flag antes de ejecutar esta rama. */
    if (json_flag)
        setenv("PROGVORAZ_JSON", "1", 1);
    /* if: comprueba configpath antes de ejecutar esta rama. */
    if (configpath)
        setenv("PROGVORAZ_CONFIG", configpath, 1);
#endif

    /* Operaciones no interactivas adicionales: exportar stock JSON o reporte. */
    /* if: comprueba export_stock_json_path antes de ejecutar esta rama. */
    if (export_stock_json_path)
    {
        /* if: comprueba export_stock_json(export_stock_json_path) antes de ejecutar esta rama. */
        if (export_stock_json(export_stock_json_path))
        {
            printf("Stock exportado a %s\n", export_stock_json_path);
            return 0;
        }
        fprintf(stderr, "No se pudo exportar stock a %s\n", export_stock_json_path);
        return 2;
    }

    /* if: comprueba export_report_path antes de ejecutar esta rama. */
    if (export_report_path)
    {
        /* if: comprueba exportar_reporte_global(export_report_path) antes de ejecutar esta rama. */
        if (exportar_reporte_global(export_report_path))
        {
            printf("Reporte global exportado a %s\n", export_report_path);
            return 0;
        }
        fprintf(stderr, "No se pudo exportar reporte a %s\n", export_report_path);
        return 2;
    }

    /* Modo servidor HTTP (bloqueante) */
    /* if: comprueba server_flag antes de ejecutar esta rama. */
    if (server_flag)
    {
        int rc = run_http_server(server_port ? server_port : "8080");
        /* if: comprueba rc != 0 antes de ejecutar esta rama. */
        if (rc != 0)
        {
            fprintf(stderr, "No se pudo iniciar el servidor HTTP (rc=%d).\n", rc);
            return 2;
        }
        return 0;
    }

    /* Modo convert CLI no interactivo */
    /* if: comprueba convert_flag antes de ejecutar esta rama. */
    if (convert_flag)
    {
        double tasa = 0.0;
        // Este if es para evitar que se intente obtener la tasa si la moneda de origen y destino son iguales.
        /* if: comprueba !fetch_exchange_rate(convert_from, convert_to, &tasa) antes de ejecutar esta rama. */
        if (!fetch_exchange_rate(convert_from, convert_to, &tasa))
        {
            fprintf(stderr, "No se pudo obtener la tasa para %s->%s\n", convert_from, convert_to);
            return 2;
        }

        /* Calcular monto destino en centimos: destino_cents = round(src_cents * tasa) */
        double src_cents = atof(convert_amount);
        double dst_cents = src_cents * tasa;
        long long rounded = (long long)(dst_cents + 0.5);
        char dst_str[64];
        snprintf(dst_str, sizeof(dst_str), "%lld", rounded);

        BigInt montoDestino = {0};
        /* if: comprueba !bigint_init(&montoDestino, dst_str) antes de ejecutar esta rama. */
        if (!bigint_init(&montoDestino, dst_str))
        {
            fprintf(stderr, "Error interno al crear BigInt destino.\n");
            return 2;
        }

        BigIntArray monedas = {0};
        BigIntArray stock = {0};
        BigIntArray solucion = {0};
        const char *to_key = map_currency_key(convert_to);
        /* if: comprueba to_key == NULL || !validar_consistencia_moneda(to_key) || !cargar_den... antes de ejecutar esta rama. */
        if (to_key == NULL || !validar_consistencia_moneda(to_key) || !cargar_denominaciones_moneda(to_key, &monedas))
        {
            fprintf(stderr, "Moneda destino no encontrada o inconsistente: %s\n", convert_to);
            bigint_free(&montoDestino);
            return 2;
        }

        /* if: comprueba convert_use_stock antes de ejecutar esta rama. */
        if (convert_use_stock)
        {
            /* if: comprueba !cargar_stock_moneda(to_key, &stock) antes de ejecutar esta rama. */
            if (!cargar_stock_moneda(to_key, &stock))
            {
                fprintf(stderr, "No se encontro stock para moneda destino: %s\n", convert_to);
                bigint_array_free(&monedas);
                bigint_free(&montoDestino);
                return 2;
            }
        }

        int rc;
        /* if: comprueba convert_use_stock antes de ejecutar esta rama. */
        if (convert_use_stock)
            rc = calcular_cambio_optimo_stock(&montoDestino, &monedas, &stock, &solucion);
        else
            rc = calcular_cambio_optimo(&montoDestino, &monedas, &solucion);

        /* if: comprueba rc antes de ejecutar esta rama. */
        if (rc)
        {
            app_console_imprimir_resultado(&monedas, &solucion, convert_use_stock ? &stock : NULL, convert_use_stock ? 1 : 0);
            printf("Convert CLI %s->%s | Origen=%s c | Destino=%s c | Tasa=%f\n",
                   convert_from, convert_to, convert_amount, dst_str, tasa);
        }
        else
        {
            fprintf(stderr, "No se pudo calcular descomposicion en moneda destino.\n");
        }

        bigint_array_free(&monedas);
        bigint_array_free(&stock);
        bigint_array_free(&solucion);
        bigint_free(&montoDestino);
        return rc ? 0 : 2;
    }

    /* Modo GUI: lanzar ejecutable gráfico si disponible. */
    /* if: comprueba gui_flag antes de ejecutar esta rama. */
    if (gui_flag)
    {
        int launched = progvoraz_launch_gui_nonblocking();

        /* if: comprueba !launched antes de ejecutar esta rama. */
        if (!launched)
            fprintf(stderr, "No se encontro ejecutable GUI (progvoraz_gui).\n");

        return launched ? 0 : 2;
    }

    /* Modo Docker/streaming: leer stdin y escribir stdout. */
    /* if: comprueba docker_flag antes de ejecutar esta rama. */
    if (docker_flag)
    {
        /* if: comprueba !batch_process_stream(logpath) antes de ejecutar esta rama. */
        if (!batch_process_stream(logpath))
            return 2;
        return 0;
    }

    /* Modo batch por archivo (existente comportamiento). */
    /* if: comprueba input != NULL antes de ejecutar esta rama. */
    if (input != NULL)
    {
        /* if: comprueba !batch_process_file(input, output, logpath) antes de ejecutar esta rama. */
        if (!batch_process_file(input, output, logpath))
            return 2;
        return 0;
    }

    /* Por defecto, modo consola interactivo. */
    return app_console_run();
}

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
#include "progvoraz_locale.h"

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

#ifndef _WIN32
extern int setenv(const char *name, const char *value, int overwrite);
#endif

/* funcion print_help: contiene la logica principal de esta operacion. */
static void print_help(const char *prog)
{
    printf("%s: %s [options]\n", progvoraz_tr("Uso", "Usage"), prog);
    printf("%s:\n", progvoraz_tr("Opciones", "Options"));
    printf("  --lang <es|en>    %s\n", progvoraz_tr("Selecciona idioma de interfaz.", "Select interface language."));
    printf("  --input <ruta>    %s\n", progvoraz_tr("Procesar archivo CSV en modo batch.", "Process a CSV file in batch mode."));
    printf("  --output <ruta>   %s\n", progvoraz_tr("Archivo de salida CSV (por defecto stdout).", "CSV output file (stdout by default)."));
    printf("  --log <ruta>      %s\n", progvoraz_tr("Archivo de log para modo batch/stream.", "Log file for batch/stream mode."));
    printf("  --gui             %s\n", progvoraz_tr("Lanzar interfaz GUI (progvoraz_gui).", "Launch the GUI application (progvoraz_gui)."));
    printf("  --docker          %s\n", progvoraz_tr("Modo no interactivo legacy (lee CSV desde stdin y escribe CSV a stdout).", "Legacy non-interactive mode (reads CSV from stdin and writes CSV to stdout)."));
    printf("  --stream          %s\n", progvoraz_tr("Alias nativo de --docker para stdin/stdout CSV.", "Native alias for --docker using CSV stdin/stdout."));
    printf("  --json            %s\n", progvoraz_tr("Habilitar salida/flags JSON (variable de entorno).", "Enable JSON output/flags (environment variable)."));
    printf("  --config <ruta>   %s\n", progvoraz_tr("Archivo de configuracion alternativo.", "Alternate configuration file."));
    printf("  --export-stock-json <ruta>  %s\n", progvoraz_tr("Exportar stock actual a JSON.", "Export current stock to JSON."));
    printf("  --export-report <ruta>      %s\n", progvoraz_tr("Exportar reporte global de inventario.", "Export the global inventory report."));
    printf("  --server          %s\n", progvoraz_tr("Iniciar servidor HTTP local.", "Start the local HTTP server."));
    printf("  --port <n>        %s\n", progvoraz_tr("Puerto para --server (por defecto 8080).", "Port for --server (default 8080)."));
    printf("  --convert <origen> <destino> <monto_centimos>\n");
    printf("                    %s\n", progvoraz_tr("Convierte y descompone un monto en la moneda destino.", "Convert and decompose an amount into the target currency."));
    printf("  --convert-stock   %s\n", progvoraz_tr("Usa stock real al calcular el cambio de --convert.", "Use real stock when calculating --convert."));
    printf("  --version         %s\n", progvoraz_tr("Muestra la version y sale.", "Show the version and exit."));
    printf("  --help            %s\n", progvoraz_tr("Muestra esta ayuda.", "Show this help message."));
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
            printf("%s\n", progvoraz_tr("ProgVoraz (version desconocida)", "ProgVoraz (unknown version)"));
        }
        fclose(f);
    }
    else
    {
        printf("%s\n", progvoraz_tr("ProgVoraz (version desconocida)", "ProgVoraz (unknown version)"));
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
    const char *lang_code = NULL;
    int convert_flag = 0;
    const char *convert_from = NULL;
    const char *convert_to = NULL;
    const char *convert_amount = NULL; /* in cents */
    int convert_use_stock = 0;

    /* for: itera segun int i = 1; i < argc; i++ para recorrer el bloque. */
    progvoraz_locale_init_from_env();
    for (int i = 1; i < argc; i++)
    {
        /* if: comprueba strcmp(argv[i], "--lang") == 0 && i + 1 < argc antes de ejecutar esta rama. */
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc)
        {
            lang_code = argv[++i];
            progvoraz_locale_set_from_code(lang_code);
        }
        /* if: comprueba strcmp(argv[i], "--input") == 0 && i + 1 < argc antes de ejecutar esta rama. */
        else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
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
    /* if: comprueba lang_code != NULL antes de ejecutar esta rama. */
    if (lang_code != NULL)
        _putenv_s("PROGVORAZ_LANG", progvoraz_lang_code());
#else
    /* if: comprueba json_flag antes de ejecutar esta rama. */
    if (json_flag)
        setenv("PROGVORAZ_JSON", "1", 1);
    /* if: comprueba configpath antes de ejecutar esta rama. */
    if (configpath)
        setenv("PROGVORAZ_CONFIG", configpath, 1);
    /* if: comprueba lang_code != NULL antes de ejecutar esta rama. */
    if (lang_code != NULL)
        setenv("PROGVORAZ_LANG", progvoraz_lang_code(), 1);
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
        fprintf(stderr, "%s %s\n", progvoraz_tr("No se pudo exportar stock a", "Could not export stock to"), export_stock_json_path);
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
        fprintf(stderr, "%s %s\n", progvoraz_tr("No se pudo exportar reporte a", "Could not export report to"), export_report_path);
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
            fprintf(stderr, progvoraz_tr("No se pudo iniciar el servidor HTTP (rc=%d).\n", "Could not start the HTTP server (rc=%d).\n"), rc);
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
            fprintf(stderr, progvoraz_tr("No se pudo obtener la tasa para %s->%s\n", "Could not obtain the rate for %s->%s\n"), convert_from, convert_to);
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
            fprintf(stderr, "%s\n", progvoraz_tr("Error interno al crear BigInt destino.", "Internal error while creating destination BigInt."));
            return 2;
        }

        BigIntArray monedas = {0};
        BigIntArray stock = {0};
        BigIntArray solucion = {0};
        const char *to_key = progvoraz_map_currency_key(convert_to);
        /* if: comprueba to_key == NULL || !validar_consistencia_moneda(to_key) || !cargar_den... antes de ejecutar esta rama. */
        if (to_key == NULL || !validar_consistencia_moneda(to_key) || !cargar_denominaciones_moneda(to_key, &monedas))
        {
            fprintf(stderr, progvoraz_tr("Moneda destino no encontrada o inconsistente: %s\n", "Target currency not found or inconsistent: %s\n"), convert_to);
            bigint_free(&montoDestino);
            return 2;
        }

        /* if: comprueba convert_use_stock antes de ejecutar esta rama. */
        if (convert_use_stock)
        {
            /* if: comprueba !cargar_stock_moneda(to_key, &stock) antes de ejecutar esta rama. */
            if (!cargar_stock_moneda(to_key, &stock))
            {
                fprintf(stderr, progvoraz_tr("No se encontro stock para moneda destino: %s\n", "No stock found for target currency: %s\n"), convert_to);
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
            printf(progvoraz_tr("Convert CLI %s->%s | Origen=%s c | Destino=%s c | Tasa=%f\n",
                                "Convert CLI %s->%s | Source=%s c | Target=%s c | Rate=%f\n"),
                   convert_from, convert_to, convert_amount, dst_str, tasa);
        }
        else
        {
            fprintf(stderr, "%s\n", progvoraz_tr("No se pudo calcular descomposicion en moneda destino.", "Could not calculate the decomposition in the target currency."));
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
            fprintf(stderr, "%s\n", progvoraz_tr("No se encontro ejecutable GUI (progvoraz_gui).", "GUI executable not found (progvoraz_gui)."));

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

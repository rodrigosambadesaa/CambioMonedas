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

/* map_currency_key: convierte un codigo ISO o nombre corto a la clave usada en monedas.txt
 * Devuelve puntero a cadena estatica o NULL si no hay mapeo. */
static const char *map_currency_key(const char *input)
{
    static char tmp[64];
    if (!input || !*input)
        return NULL;

    /* Comparaciones comunes (case-insensitive) */
    if (strcasecmp(input, "EUR") == 0 || strcasecmp(input, "E") == 0 || strcasecmp(input, "EURO") == 0)
        return "euro";
    if (strcasecmp(input, "USD") == 0 || strcasecmp(input, "US") == 0 || strcasecmp(input, "DOLAR") == 0 || strcasecmp(input, "DOLLAR") == 0)
        return "dolar";
    if (strcasecmp(input, "JPY") == 0 || strcasecmp(input, "YEN") == 0)
        return "yen";
    if (strcasecmp(input, "GBP") == 0 || strcasecmp(input, "GBP") == 0 || strcasecmp(input, "LIBRA") == 0 || strcasecmp(input, "POUND") == 0)
        return "libra";
    if (strcasecmp(input, "CHF") == 0 || strcasecmp(input, "FRANC") == 0 || strcasecmp(input, "FRANCO_SUIZO") == 0)
        return "franco_suizo";
    if (strcasecmp(input, "CAD") == 0)
        return "dolar_canadiense";
    if (strcasecmp(input, "AUD") == 0)
        return "dolar_australiano";
    if (strcasecmp(input, "CNY") == 0 || strcasecmp(input, "RMB") == 0)
        return "yuan_chino";
    if (strcasecmp(input, "MXN") == 0)
        return "peso_mexicano";

    /* Fallback: convierte a minusculas y reemplaza espacios por _ */
    size_t i, j = 0;
    for (i = 0; input[i] != '\0' && j + 1 < sizeof(tmp); i++)
    {
        char c = input[i];
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

static void print_help(const char *prog)
{
    printf("Uso: %s [opciones]\n", prog);
    printf("Opciones:\n");
    printf("  --input <ruta>    Procesar archivo CSV en modo batch.\n");
    printf("  --output <ruta>   Archivo de salida CSV (por defecto stdout).\n");
    printf("  --log <ruta>      Archivo de log para modo batch/stream.\n");
    printf("  --gui             Lanzar interfaz GUI (progvoraz_gui).\n");
    printf("  --docker          Modo no interactivo (lee CSV desde stdin y escribe CSV a stdout).\n");
    printf("  --json            Habilitar salida/flags JSON (variable de entorno).\n");
    printf("  --config <ruta>   Archivo de configuracion alternativo.\n");
    printf("  --version         Muestra la version y sale.\n");
    printf("  --help            Muestra esta ayuda.\n");
}

static void print_version(void)
{
    FILE *f = fopen("VERSIONES.md", "r");
    if (f)
    {
        char linea[256];
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

static int file_exists(const char *path)
{
    if (!path)
        return 0;
    FILE *f = fopen(path, "r");
    if (f)
    {
        fclose(f);
        return 1;
    }
    return 0;
}

static int launch_gui_nonblocking(const char *exec)
{
#ifdef _WIN32
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", exec);
    return system(cmd) == 0;
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "setsid %s >/dev/null 2>&1 &", exec);
    return system(cmd) == 0;
#endif
}

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

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            input = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output = argv[++i];
        else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc)
            logpath = argv[++i];
        else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
            configpath = argv[++i];
        else if (strcmp(argv[i], "--gui") == 0)
            gui_flag = 1;
        else if (strcmp(argv[i], "--docker") == 0)
            docker_flag = 1;
        else if (strcmp(argv[i], "--json") == 0)
            json_flag = 1;
        else if (strcmp(argv[i], "--export-stock-json") == 0 && i + 1 < argc)
            export_stock_json_path = argv[++i];
        else if (strcmp(argv[i], "--export-report") == 0 && i + 1 < argc)
            export_report_path = argv[++i];
        else if (strcmp(argv[i], "--server") == 0)
            server_flag = 1;
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            server_port = argv[++i];
        else if (strcmp(argv[i], "--convert") == 0 && i + 3 < argc)
        {
            convert_flag = 1;
            convert_from = argv[++i];
            convert_to = argv[++i];
            convert_amount = argv[++i];
        }
        else if (strcmp(argv[i], "--convert-stock") == 0)
        {
            convert_use_stock = 1;
        }
        else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)
        {
            print_version();
            return 0;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
    }

    /* Exportar variables de entorno para que otros componentes las lean si es necesario. */
#ifdef _WIN32
    if (json_flag)
        _putenv_s("PROGVORAZ_JSON", "1");
    if (configpath)
        _putenv_s("PROGVORAZ_CONFIG", configpath);
#else
    if (json_flag)
        setenv("PROGVORAZ_JSON", "1", 1);
    if (configpath)
        setenv("PROGVORAZ_CONFIG", configpath, 1);
#endif

    /* Operaciones no interactivas adicionales: exportar stock JSON o reporte. */
    if (export_stock_json_path)
    {
        if (export_stock_json(export_stock_json_path))
        {
            printf("Stock exportado a %s\n", export_stock_json_path);
            return 0;
        }
        fprintf(stderr, "No se pudo exportar stock a %s\n", export_stock_json_path);
        return 2;
    }

    if (export_report_path)
    {
        if (exportar_reporte_global(export_report_path))
        {
            printf("Reporte global exportado a %s\n", export_report_path);
            return 0;
        }
        fprintf(stderr, "No se pudo exportar reporte a %s\n", export_report_path);
        return 2;
    }

    /* Modo servidor HTTP (bloqueante) */
    if (server_flag)
    {
        int rc = run_http_server(server_port ? server_port : "8080");
        if (rc != 0)
        {
            fprintf(stderr, "No se pudo iniciar el servidor HTTP (rc=%d).\n", rc);
            return 2;
        }
        return 0;
    }

    /* Modo convert CLI no interactivo */
    if (convert_flag)
    {
        double tasa = 0.0;
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
        if (!bigint_init(&montoDestino, dst_str))
        {
            fprintf(stderr, "Error interno al crear BigInt destino.\n");
            return 2;
        }

        BigIntArray monedas = {0};
        BigIntArray stock = {0};
        BigIntArray solucion = {0};
        const char *to_key = map_currency_key(convert_to);
        if (to_key == NULL || !validar_consistencia_moneda(to_key) || !cargar_denominaciones_moneda(to_key, &monedas))
        {
            fprintf(stderr, "Moneda destino no encontrada o inconsistente: %s\n", convert_to);
            bigint_free(&montoDestino);
            return 2;
        }

        if (convert_use_stock)
        {
            if (!cargar_stock_moneda(to_key, &stock))
            {
                fprintf(stderr, "No se encontro stock para moneda destino: %s\n", convert_to);
                bigint_array_free(&monedas);
                bigint_free(&montoDestino);
                return 2;
            }
        }

        int rc;
        if (convert_use_stock)
            rc = calcular_cambio_optimo_stock(&montoDestino, &monedas, &stock, &solucion);
        else
            rc = calcular_cambio_optimo(&montoDestino, &monedas, &solucion);

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
    if (gui_flag)
    {
#ifdef _WIN32
        const char *gui_execs[] = {"progvoraz_gui.exe", "./progvoraz_gui.exe"};
#else
        const char *gui_execs[] = {"./progvoraz_gui", "progvoraz_gui"};
#endif
        int launched = 0;
        for (size_t i = 0; i < sizeof(gui_execs) / sizeof(gui_execs[0]); i++)
        {
            if (file_exists(gui_execs[i]))
            {
                if (!launch_gui_nonblocking(gui_execs[i]))
                    fprintf(stderr, "Advertencia: fallo al lanzar GUI (no bloqueante): %s\n", gui_execs[i]);
                launched = 1;
                break;
            }
        }

        if (!launched)
            fprintf(stderr, "No se encontro ejecutable GUI (progvoraz_gui).\n");

        return launched ? 0 : 2;
    }

    /* Modo Docker/streaming: leer stdin y escribir stdout. */
    if (docker_flag)
    {
        if (!batch_process_stream(logpath))
            return 2;
        return 0;
    }

    /* Modo batch por archivo (existente comportamiento). */
    if (input != NULL)
    {
        if (!batch_process_file(input, output, logpath))
            return 2;
        return 0;
    }

    /* Por defecto, modo consola interactivo. */
    return app_console_run();
}

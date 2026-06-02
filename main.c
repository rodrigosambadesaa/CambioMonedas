#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app_console.h"
#include "batch_cli.h"
#include "json_io.h"
#include "moneda_gestion.h"
#include "server_http.h"

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

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *g_log = NULL;
static char g_path[1024];

/* funcion logger_init: contiene la logica principal de esta operacion. */
int logger_init(const char *path)
{
    /* if: comprueba path == NULL antes de ejecutar esta rama. */
    if (path == NULL)
        return 0;
    strncpy(g_path, path, sizeof(g_path) - 1);
    g_path[sizeof(g_path) - 1] = '\0';
    g_log = fopen(g_path, "a");
    return g_log != NULL;
}

/* funcion logger_close: contiene la logica principal de esta operacion. */
void logger_close(void)
{
    /* if: comprueba g_log antes de ejecutar esta rama. */
    if (g_log)
    {
        fclose(g_log);
        g_log = NULL;
    }
}

/* funcion logger_vlog: contiene la logica principal de esta operacion. */
static void logger_vlog(const char *prefix, const char *fmt, va_list ap)
{
    /* if: comprueba !g_log antes de ejecutar esta rama. */
    if (!g_log)
        return;
    fprintf(g_log, "%s: ", prefix);
    vfprintf(g_log, fmt, ap);
    fprintf(g_log, "\n");
    fflush(g_log);
}

/* funcion logger_info: contiene la logica principal de esta operacion. */
void logger_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger_vlog("INFO", fmt, ap);
    va_end(ap);
}

/* funcion logger_error: contiene la logica principal de esta operacion. */
void logger_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger_vlog("ERROR", fmt, ap);
    va_end(ap);
}

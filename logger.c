#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *g_log = NULL;
static char g_path[1024];

int logger_init(const char *path)
{
    if (path == NULL)
        return 0;
    strncpy(g_path, path, sizeof(g_path) - 1);
    g_path[sizeof(g_path) - 1] = '\0';
    g_log = fopen(g_path, "a");
    return g_log != NULL;
}

void logger_close(void)
{
    if (g_log)
    {
        fclose(g_log);
        g_log = NULL;
    }
}

static void logger_vlog(const char *prefix, const char *fmt, va_list ap)
{
    if (!g_log)
        return;
    fprintf(g_log, "%s: ", prefix);
    vfprintf(g_log, fmt, ap);
    fprintf(g_log, "\n");
    fflush(g_log);
}

void logger_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger_vlog("INFO", fmt, ap);
    va_end(ap);
}

void logger_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger_vlog("ERROR", fmt, ap);
    va_end(ap);
}

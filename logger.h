#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

int logger_init(const char *path);
void logger_close(void);
void logger_info(const char *fmt, ...);
void logger_error(const char *fmt, ...);

#endif

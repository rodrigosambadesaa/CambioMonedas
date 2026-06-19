#ifndef APP_CONSOLE_H
#define APP_CONSOLE_H

#include "bigint.h"

int app_console_run(void);

/* Public wrapper to print a computed solution using the console formatting. */
void app_console_imprimir_resultado(const BigIntArray *monedas, const BigIntArray *solucion, const BigIntArray *stock, int usarStock);

#endif

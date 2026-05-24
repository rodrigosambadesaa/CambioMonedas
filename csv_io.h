#ifndef CSV_IO_H
#define CSV_IO_H

#include <stdio.h>

/* Escribe una fila CSV simple (campos ya sin comillas). Devuelve 1 si ok. */
int csv_write_row(FILE *fp, const char **cols, size_t ncols);

#endif

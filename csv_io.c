#include "csv_io.h"
#include <stdio.h>
#include <string.h>

/* funcion csv_write_row: contiene la logica principal de esta operacion. */
int csv_write_row(FILE *fp, const char **cols, size_t ncols)
{
    /* if: comprueba fp == NULL || cols == NULL antes de ejecutar esta rama. */
    if (fp == NULL || cols == NULL)
        return 0;

    /* for: itera segun size_t i = 0; i < ncols; i++ para recorrer el bloque. */
    for (size_t i = 0; i < ncols; i++)
    {
        /* if: comprueba fputs(cols[i], fp) == EOF antes de ejecutar esta rama. */
        if (fputs(cols[i], fp) == EOF)
            return 0;
        /* if: comprueba i + 1 < ncols antes de ejecutar esta rama. */
        if (i + 1 < ncols)
        {
            /* if: comprueba fputc(',', fp) == EOF antes de ejecutar esta rama. */
            if (fputc(',', fp) == EOF)
                return 0;
        }
    }

    /* if: comprueba fputc('\n', fp) == EOF antes de ejecutar esta rama. */
    if (fputc('\n', fp) == EOF)
        return 0;

    return 1;
}

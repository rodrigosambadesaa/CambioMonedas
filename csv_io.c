#include "csv_io.h"
#include <stdio.h>
#include <string.h>

int csv_write_row(FILE *fp, const char **cols, size_t ncols)
{
    if (fp == NULL || cols == NULL)
        return 0;

    for (size_t i = 0; i < ncols; i++)
    {
        if (fputs(cols[i], fp) == EOF)
            return 0;
        if (i + 1 < ncols)
        {
            if (fputc(',', fp) == EOF)
                return 0;
        }
    }

    if (fputc('\n', fp) == EOF)
        return 0;

    return 1;
}

#include "moneda_gestion.h"
#include "bigint.h"
#include <stdio.h>
#include <string.h>

/* Test-only executable: replace a currency with a uniform, arbitrary-size
 * stock amount. Production code stays in moneda_gestion.c.
 */
int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) return 2;
    BigIntArray stock = {0};
    BigInt amount = {0};
    int ok = 0;
    if (!cargar_stock_moneda(argv[1], &stock) ||
        !bigint_init(&amount, argv[2])) goto cleanup;
    for (size_t i = 0; i < stock.len; ++i)
        if (!bigint_array_set(&stock, i, &amount)) goto cleanup;

    if (argc == 4 && strcmp(argv[3], "--short") == 0)
    {
        /* A malformed denomination count must always be rejected. */
        size_t original_length = stock.len;
        stock.len--;
        int accepted = actualizar_stock_moneda(argv[1], &stock);
        stock.len = original_length;
        ok = !accepted;
    }
    else
    {
        ok = actualizar_stock_moneda(argv[1], &stock);
    }
cleanup:
    bigint_free(&amount);
    bigint_array_free(&stock);
    return ok ? 0 : 1;
}

#include "algoritmo_cambio.h"
#include <stdio.h>
#include <string.h>

static int failures = 0, cases = 0;

static int make(BigIntArray *out, const char *const *strings, size_t count)
{
    if (!bigint_array_create(out, count)) return 0;
    for (size_t i = 0; i < count; ++i)
    {
        BigInt item = {0};
        int ok = bigint_init(&item, strings[i]) &&
                 bigint_array_set(out, i, &item);
        bigint_free(&item);
        if (!ok) return 0;
    }
    return 1;
}

static void test(const char *description, int mode, const char *amountText,
                 const char *const *denomText, const char *const *stockText,
                 size_t count, const char *expectedCovered,
                 const char *const *expectedCounts)
{
    BigInt amount = {0}, covered = {0};
    BigIntArray denoms = {0}, stock = {0}, result = {0};
    int rc = 0, ok = 1;
    if (!bigint_init(&amount, amountText) ||
        !make(&denoms, denomText, count) ||
        (stockText != NULL && !make(&stock, stockText, count)))
    {
        fprintf(stderr, "SETUP FAILURE: %s\n", description);
        ++failures;
        goto done;
    }
    if (mode == 0) rc = calcular_cambio_optimo_con_rango(&amount, &denoms, 1, 1, &result);
    if (mode == 1) rc = calcular_cambio_optimo_stock_con_rango(&amount, &denoms, &stock, 1, 1, &result);
    if (mode == 2) rc = calcular_cambio_cercano_con_rango(&amount, &denoms, 0, 1, &covered, &result);
    if (mode == 3) rc = calcular_cambio_cercano_stock_con_rango(&amount, &denoms, &stock, 0, 1, &covered, &result);
    ++cases;
    if (!rc || result.len != count)
        ok = 0;
    if (ok)
        for (size_t i = 0; i < count; ++i)
            if (!result.items[i].digits ||
                strcmp(result.items[i].digits, expectedCounts[i]) != 0)
                ok = 0;
    if (mode >= 2 && (!covered.digits ||
        strcmp(covered.digits, expectedCovered) != 0))
        ok = 0;
    if (!ok)
    {
        fprintf(stderr, "FAIL: %s\n", description);
        ++failures;
    }
done:
    bigint_free(&amount);
    bigint_free(&covered);
    bigint_array_free(&denoms);
    bigint_array_free(&stock);
    bigint_array_free(&result);
}

int main(void)
{
    const char *d1[] = {"1"};
    const char *s1[] = {"50001"};
    const char *d50000[] = {"50000"};
    const char *s50000[] = {"1"};
    const char *d50001[] = {"50001"};
    const char *s50001[] = {"1"};
    const char *dMixed[] = {"50000", "1", "40000"};
    const char *sMixed[] = {"1", "50001", "1"};
    const char *one[] = {"1"};
    const char *mixed[] = {"1", "0", "0"};
    test("one-coin exact >50000",0,"50001",d50001,NULL,1,"",one);
    test("one-coin exact stock >50000",1,"50001",d50001,s50001,1,"",one);
    test("closest 1 >50000",2,"50001",d1,NULL,1,"1",one);
    test("closest 1 stock >50000",3,"50001",d1,s1,1,"1",one);
    test("closest 50000 >50000",2,"50001",d50000,NULL,1,"50000",one);
    test("closest 50000 stock >50000",3,"50001",d50000,s50000,1,"50000",one);
    test("largest valid denomination chosen",2,"50001",dMixed,NULL,3,"50000",mixed);
    test("largest available in stock",3,"50001",dMixed,sMixed,3,"50000",mixed);
    fprintf(stderr,"BigInt one-coin regression: %d checked, %d failures\n",cases,failures);
    return failures ? 1 : 0;
}

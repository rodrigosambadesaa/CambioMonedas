#include "algoritmo_cambio.h"
#include <stdlib.h>

static int copiar_bigint(BigInt *dest, const BigInt *src)
{
    if (dest == NULL || src == NULL || src->digits == NULL)
        return 0;

    if (bigint_set(dest, src))
        return 1;

    return bigint_init(dest, src->digits);
}

static int construir_sufijos_stock(const BigIntArray *denom,
                                   const BigIntArray *stock,
                                   BigInt **sufijos_out)
{
    BigInt *sufijos;
    size_t i;

    if (denom == NULL || stock == NULL || sufijos_out == NULL || denom->len != stock->len)
        return 0;

    sufijos = (BigInt *)calloc(denom->len + 1, sizeof(BigInt));
    if (sufijos == NULL)
        return 0;

    if (!bigint_init(&sufijos[denom->len], "0"))
    {
        free(sufijos);
        return 0;
    }

    for (i = denom->len; i > 0; i--)
    {
        BigInt aporte = {0};
        BigInt acumulado = {0};

        if (!bigint_multiply(&denom->items[i - 1], &stock->items[i - 1], &aporte) ||
            !bigint_add(&aporte, &sufijos[i], &acumulado) ||
            !copiar_bigint(&sufijos[i - 1], &acumulado))
        {
            size_t j;
            bigint_free(&aporte);
            bigint_free(&acumulado);
            for (j = 0; j <= denom->len; j++)
                bigint_free(&sufijos[j]);
            free(sufijos);
            return 0;
        }

        bigint_free(&aporte);
        bigint_free(&acumulado);
    }

    *sufijos_out = sufijos;
    return 1;
}

static void liberar_sufijos_stock(BigInt *sufijos, size_t len)
{
    size_t i;

    if (sufijos == NULL)
        return;

    for (i = 0; i <= len; i++)
        bigint_free(&sufijos[i]);
    free(sufijos);
}

static int backtrack_ilimitado(const BigIntArray *denom,
                               const BigInt *one,
                               const BigInt *zero,
                               BigInt *restante,
                               BigIntArray *solucion,
                               size_t index)
{
    BigInt max_usable = {0};
    BigInt residuo = {0};
    BigInt cuenta = {0};
    BigInt uso = {0};
    BigInt actual_restante = {0};
    int ok = 0;

    if (bigint_is_zero(restante))
        return 1;
    if (index >= denom->len)
        return 0;

    if (bigint_is_zero(&denom->items[index]))
    {
        if (!bigint_array_set(solucion, index, zero))
            return 0;
        return backtrack_ilimitado(denom, one, zero, restante, solucion, index + 1);
    }

    if (!bigint_divmod(restante, &denom->items[index], &max_usable, &residuo) ||
        !copiar_bigint(&cuenta, &max_usable) ||
        !bigint_multiply(&denom->items[index], &cuenta, &uso) ||
        !bigint_subtract(restante, &uso, &actual_restante))
        goto cleanup;

    while (1)
    {
        if (!bigint_array_set(solucion, index, &cuenta))
            goto cleanup;

        if (backtrack_ilimitado(denom, one, zero, &actual_restante, solucion, index + 1))
        {
            ok = 1;
            goto cleanup;
        }

        if (bigint_compare(&cuenta, zero) == 0)
            break;

        {
            BigInt sig_cuenta = {0};
            BigInt sig_restante = {0};

            if (!bigint_subtract(&cuenta, one, &sig_cuenta) ||
                !bigint_add(&actual_restante, &denom->items[index], &sig_restante))
            {
                bigint_free(&sig_cuenta);
                bigint_free(&sig_restante);
                goto cleanup;
            }

            bigint_free(&cuenta);
            bigint_free(&actual_restante);
            cuenta = sig_cuenta;
            actual_restante = sig_restante;
        }
    }

    if (!bigint_array_set(solucion, index, zero))
        goto cleanup;

cleanup:
    bigint_free(&max_usable);
    bigint_free(&residuo);
    bigint_free(&cuenta);
    bigint_free(&uso);
    bigint_free(&actual_restante);
    return ok;
}

static int backtrack_stock(const BigIntArray *denom,
                           const BigIntArray *stock,
                           const BigIntArray *sufijos,
                           const BigInt *one,
                           const BigInt *zero,
                           BigInt *restante,
                           BigIntArray *solucion,
                           size_t index)
{
    BigInt max_usable = {0};
    BigInt residuo = {0};
    BigInt cuenta = {0};
    BigInt uso = {0};
    BigInt actual_restante = {0};
    int ok = 0;

    if (bigint_is_zero(restante))
        return 1;
    if (index >= denom->len)
        return 0;
    if (sufijos != NULL && index < sufijos->len && bigint_compare(restante, &sufijos->items[index]) > 0)
        return 0;

    if (bigint_is_zero(&denom->items[index]))
    {
        if (!bigint_array_set(solucion, index, zero))
            return 0;
        return backtrack_stock(denom, stock, sufijos, one, zero, restante, solucion, index + 1);
    }

    if (!bigint_divmod(restante, &denom->items[index], &max_usable, &residuo))
        goto cleanup;

    if (bigint_compare(&max_usable, &stock->items[index]) > 0)
    {
        if (!copiar_bigint(&cuenta, &stock->items[index]))
            goto cleanup;
    }
    else
    {
        if (!copiar_bigint(&cuenta, &max_usable))
            goto cleanup;
    }

    if (!bigint_multiply(&denom->items[index], &cuenta, &uso) ||
        !bigint_subtract(restante, &uso, &actual_restante))
        goto cleanup;

    while (1)
    {
        if (!bigint_array_set(solucion, index, &cuenta))
            goto cleanup;

        if (backtrack_stock(denom, stock, sufijos, one, zero, &actual_restante, solucion, index + 1))
        {
            ok = 1;
            goto cleanup;
        }

        if (bigint_compare(&cuenta, zero) == 0)
            break;

        {
            BigInt sig_cuenta = {0};
            BigInt sig_restante = {0};

            if (!bigint_subtract(&cuenta, one, &sig_cuenta) ||
                !bigint_add(&actual_restante, &denom->items[index], &sig_restante))
            {
                bigint_free(&sig_cuenta);
                bigint_free(&sig_restante);
                goto cleanup;
            }

            bigint_free(&cuenta);
            bigint_free(&actual_restante);
            cuenta = sig_cuenta;
            actual_restante = sig_restante;
        }
    }

    if (!bigint_array_set(solucion, index, zero))
        goto cleanup;

cleanup:
    bigint_free(&max_usable);
    bigint_free(&residuo);
    bigint_free(&cuenta);
    bigint_free(&uso);
    bigint_free(&actual_restante);
    return ok;
}

int calcular_cambio_optimo(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)
{
    BigInt restante = {0};
    BigInt one = {0};
    BigInt zero = {0};
    int res;

    if (monto == NULL || denominaciones == NULL || solucion == NULL)
        return 0;
    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;
    if (!copiar_bigint(&restante, monto) ||
        !bigint_init(&one, "1") ||
        !bigint_init(&zero, "0"))
    {
        bigint_free(&restante);
        bigint_free(&one);
        bigint_free(&zero);
        bigint_array_free(solucion);
        return 0;
    }

    res = backtrack_ilimitado(denominaciones, &one, &zero, &restante, solucion, 0);

    bigint_free(&restante);
    bigint_free(&one);
    bigint_free(&zero);
    if (!res)
        bigint_array_free(solucion);
    return res;
}

int calcular_cambio_optimo_stock(const BigInt *monto,
                                 const BigIntArray *denominaciones,
                                 const BigIntArray *stock,
                                 BigIntArray *solucion)
{
    BigInt restante = {0};
    BigInt one = {0};
    BigInt zero = {0};
    BigInt *sufijos_raw = NULL;
    BigIntArray sufijos = {0};
    int res;

    if (monto == NULL || denominaciones == NULL || stock == NULL || solucion == NULL)
        return 0;
    if (denominaciones->len != stock->len)
        return 0;
    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;

    if (!copiar_bigint(&restante, monto) ||
        !bigint_init(&one, "1") ||
        !bigint_init(&zero, "0") ||
        !construir_sufijos_stock(denominaciones, stock, &sufijos_raw))
    {
        bigint_free(&restante);
        bigint_free(&one);
        bigint_free(&zero);
        liberar_sufijos_stock(sufijos_raw, denominaciones != NULL ? denominaciones->len : 0);
        bigint_array_free(solucion);
        return 0;
    }

    sufijos.items = sufijos_raw;
    sufijos.len = denominaciones->len + 1;

    res = backtrack_stock(denominaciones, stock, &sufijos, &one, &zero, &restante, solucion, 0);

    bigint_free(&restante);
    bigint_free(&one);
    bigint_free(&zero);
    liberar_sufijos_stock(sufijos_raw, denominaciones->len);

    if (!res)
        bigint_array_free(solucion);
    return res;
}
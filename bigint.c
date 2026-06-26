#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "bigint.h"

/* *duplicar: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion duplicar: contiene la logica principal de esta operacion. */
static char *duplicar(const char *s)
{
    size_t len;
    char *copia;

    /* if: comprueba s == NULL antes de ejecutar esta rama. */
    if (s == NULL)
        return NULL;

    len = strlen(s) + 1;
    copia = (char *)malloc(len);
    /* if: comprueba copia == NULL antes de ejecutar esta rama. */
    if (copia == NULL)
        return NULL;

    memcpy(copia, s, len);
    return copia;
}

/* texto_valido_entero_no_negativo: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion texto_valido_entero_no_negativo: contiene la logica principal de esta operacion. */
static int texto_valido_entero_no_negativo(const char *text)
{
    size_t i;

    /* if: comprueba text == NULL || *text == '\0' antes de ejecutar esta rama. */
    if (text == NULL || *text == '\0')
        return 0;

    /* for: itera segun i = 0; text[i] != '\0'; i++ para recorrer el bloque. */
    for (i = 0; text[i] != '\0'; i++)
    {
        /* if: comprueba !isdigit((unsigned char)text[i]) antes de ejecutar esta rama. */
        if (!isdigit((unsigned char)text[i]))
            return 0;
    }

    return 1;
}

/* *normalizar: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion normalizar: contiene la logica principal de esta operacion. */
static char *normalizar(const char *text)
{
    size_t i = 0;

    /* if: comprueba text == NULL || *text == '\0' antes de ejecutar esta rama. */
    if (text == NULL || *text == '\0')
        return NULL;

    /* while: repite el bloque mientras se cumpla text[i] == '0' && text[i + 1] != '\0'. */
    while (text[i] == '0' && text[i + 1] != '\0')
        i++;

    return duplicar(text + i);
}

/* bigint_init: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_init: contiene la logica principal de esta operacion. */
int bigint_init(BigInt *n, const char *text)
{
    char *normalizado;

    /* if: comprueba n == NULL || !texto_valido_entero_no_negativo(text) antes de ejecutar esta rama. */
    if (n == NULL || !texto_valido_entero_no_negativo(text))
        return 0;

    normalizado = normalizar(text);
    /* if: comprueba normalizado == NULL antes de ejecutar esta rama. */
    if (normalizado == NULL)
        return 0;

    n->digits = normalizado;
    return 1;
}

/* bigint_set: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_set: contiene la logica principal de esta operacion. */
int bigint_set(BigInt *dest, const BigInt *src)
{
    char *copia;

    /* if: comprueba dest == NULL || src == NULL || src->digits == NULL antes de ejecutar esta rama. */
    if (dest == NULL || src == NULL || src->digits == NULL)
        return 0;

    copia = duplicar(src->digits);
    /* if: comprueba copia == NULL antes de ejecutar esta rama. */
    if (copia == NULL)
        return 0;

    free(dest->digits);
    dest->digits = copia;
    return 1;
}

/* bigint_free: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_free: contiene la logica principal de esta operacion. */
void bigint_free(BigInt *n)
{
    /* if: comprueba n == NULL antes de ejecutar esta rama. */
    if (n == NULL)
        return;

    free(n->digits);
    n->digits = NULL;
}

/* bigint_compare: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_compare: contiene la logica principal de esta operacion. */
int bigint_compare(const BigInt *a, const BigInt *b)
{
    size_t la;
    size_t lb;
    int cmp;

    /* if: comprueba a == NULL || b == NULL || a->digits == NULL || b->digits == NULL antes de ejecutar esta rama. */
    if (a == NULL || b == NULL || a->digits == NULL || b->digits == NULL)
        return 0;

    la = strlen(a->digits);
    lb = strlen(b->digits);

    /* if: comprueba la < lb antes de ejecutar esta rama. */
    if (la < lb)
        return -1;
    /* if: comprueba la > lb antes de ejecutar esta rama. */
    if (la > lb)
        return 1;

    cmp = strcmp(a->digits, b->digits);
    /* if: comprueba cmp < 0 antes de ejecutar esta rama. */
    if (cmp < 0)
        return -1;
    /* if: comprueba cmp > 0 antes de ejecutar esta rama. */
    if (cmp > 0)
        return 1;
    return 0;
}

/* bigint_is_zero: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_is_zero: contiene la logica principal de esta operacion. */
int bigint_is_zero(const BigInt *n)
{
    return n != NULL && n->digits != NULL && strcmp(n->digits, "0") == 0;
}

/* bigint_add: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_add: contiene la logica principal de esta operacion. */
int bigint_add(const BigInt *a, const BigInt *b, BigInt *out)
{
    size_t la;
    size_t lb;
    size_t maxlen;
    char *tmp;
    int carry = 0;

    /* if: comprueba a == NULL || b == NULL || out == NULL || a->digits == NULL || b->digi... antes de ejecutar esta rama. */
    if (a == NULL || b == NULL || out == NULL || a->digits == NULL || b->digits == NULL)
        return 0;

    la = strlen(a->digits);
    lb = strlen(b->digits);
    maxlen = la > lb ? la : lb;

    tmp = (char *)malloc(maxlen + 2);
    /* if: comprueba tmp == NULL antes de ejecutar esta rama. */
    if (tmp == NULL)
        return 0;

    tmp[maxlen + 1] = '\0';

    /* for: itera segun size_t k = 0; k < maxlen; k++ para recorrer el bloque. */
    for (size_t k = 0; k < maxlen; k++)
    {
        int da = (la > k) ? (a->digits[la - 1 - k] - '0') : 0;
        int db = (lb > k) ? (b->digits[lb - 1 - k] - '0') : 0;
        int s = da + db + carry;
        tmp[maxlen - k] = (char)('0' + (s % 10));
        carry = s / 10;
    }

    tmp[0] = (char)('0' + carry);

    {
        BigInt res = {0};
        char *norm = carry ? duplicar(tmp) : duplicar(tmp + 1);
        free(tmp);
        /* if: comprueba norm == NULL antes de ejecutar esta rama. */
        if (norm == NULL)
            return 0;
        res.digits = norm;
        bigint_free(out);
        *out = res;
    }

    return 1;
}

/* bigint_subtract: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_subtract: contiene la logica principal de esta operacion. */
int bigint_subtract(const BigInt *a, const BigInt *b, BigInt *out)
{
    size_t la;
    size_t lb;
    char *tmp;
    int borrow = 0;

    /* if: comprueba a == NULL || b == NULL || out == NULL || a->digits == NULL || b->digi... antes de ejecutar esta rama. */
    if (a == NULL || b == NULL || out == NULL || a->digits == NULL || b->digits == NULL)
        return 0;

    /* if: comprueba bigint_compare(a, b) < 0 antes de ejecutar esta rama. */
    if (bigint_compare(a, b) < 0)
        return 0;

    la = strlen(a->digits);
    lb = strlen(b->digits);

    tmp = (char *)malloc(la + 1);
    /* if: comprueba tmp == NULL antes de ejecutar esta rama. */
    if (tmp == NULL)
        return 0;

    tmp[la] = '\0';

    /* for: itera segun size_t k = 0; k < la; k++ para recorrer el bloque. */
    for (size_t k = 0; k < la; k++)
    {
        int da = a->digits[la - 1 - k] - '0';
        int db = (lb > k) ? (b->digits[lb - 1 - k] - '0') : 0;
        int d = da - db - borrow;

        /* if: comprueba d < 0 antes de ejecutar esta rama. */
        if (d < 0)
        {
            d += 10;
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }

        tmp[la - 1 - k] = (char)('0' + d);
    }

    {
        BigInt res = {0};
        char *norm = normalizar(tmp);
        free(tmp);
        /* if: comprueba norm == NULL antes de ejecutar esta rama. */
        if (norm == NULL)
            return 0;
        res.digits = norm;
        bigint_free(out);
        *out = res;
    }

    return 1;
}

/* bigint_multiply: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_multiply: contiene la logica principal de esta operacion. */
int bigint_multiply(const BigInt *a, const BigInt *b, BigInt *out)
{
    size_t la;
    size_t lb;
    int *acc;
    size_t n;
    char *tmp;

    /* if: comprueba a == NULL || b == NULL || out == NULL || a->digits == NULL || b->digi... antes de ejecutar esta rama. */
    if (a == NULL || b == NULL || out == NULL || a->digits == NULL || b->digits == NULL)
        return 0;

    /* if: comprueba bigint_is_zero(a) || bigint_is_zero(b) antes de ejecutar esta rama. */
    if (bigint_is_zero(a) || bigint_is_zero(b))
        return bigint_init(out, "0");

    la = strlen(a->digits);
    lb = strlen(b->digits);
    n = la + lb;

    acc = (int *)calloc(n, sizeof(int));
    /* if: comprueba acc == NULL antes de ejecutar esta rama. */
    if (acc == NULL)
        return 0;

    /* for: itera segun size_t i = 0; i < la; i++ para recorrer el bloque. */
    for (size_t i = 0; i < la; i++)
    {
        int da = a->digits[la - 1 - i] - '0';
        /* for: itera segun size_t j = 0; j < lb; j++ para recorrer el bloque. */
        for (size_t j = 0; j < lb; j++)
        {
            int db = b->digits[lb - 1 - j] - '0';
            acc[n - 1 - (i + j)] += da * db;
        }
    }

    /* for: itera segun size_t k = n - 1; k > 0; k-- para recorrer el bloque. */
    for (size_t k = n - 1; k > 0; k--)
    {
        acc[k - 1] += acc[k] / 10;
        acc[k] %= 10;
    }

    tmp = (char *)malloc(n + 1);

    /* if: comprueba tmp == NULL antes de ejecutar esta rama. */
    if (tmp == NULL)
    {
        free(acc);
        return 0;
    }

    /* for: itera segun size_t k = 0; k < n; k++ para recorrer el bloque. */
    for (size_t k = 0; k < n; k++)
        tmp[k] = (char)('0' + acc[k]);
    tmp[n] = '\0';

    free(acc);

    {
        BigInt res = {0};
        char *norm = normalizar(tmp);
        free(tmp);
        /* if: comprueba norm == NULL antes de ejecutar esta rama. */
        if (norm == NULL)
            return 0;
        res.digits = norm;
        bigint_free(out);
        *out = res;
    }

    return 1;
}

/* bigint_divmod: Punto de entrada principal. Configura el entorno y lanza la ejecucion. */
/* funcion bigint_divmod: contiene la logica principal de esta operacion. */
int bigint_divmod(const BigInt *a, const BigInt *b, BigInt *quotient, BigInt *remainder)
{
    size_t la;
    char *qdigits;
    BigInt rem = {0};
    BigInt d = {0};
    int ok = 0;

    /* if: comprueba a == NULL || b == NULL || quotient == NULL || remainder == NULL antes de ejecutar esta rama. */
    if (a == NULL || b == NULL || quotient == NULL || remainder == NULL)
        return 0;
    /* if: comprueba a->digits == NULL || b->digits == NULL antes de ejecutar esta rama. */
    if (a->digits == NULL || b->digits == NULL)
        return 0;
    /* if: comprueba bigint_is_zero(b) antes de ejecutar esta rama. */
    if (bigint_is_zero(b))
        return 0;

    /* if: comprueba !bigint_init(&rem, "0") antes de ejecutar esta rama. */
    if (!bigint_init(&rem, "0"))
        return 0;

    /* if: comprueba !bigint_set(&d, b) antes de ejecutar esta rama. */
    if (!bigint_set(&d, b))
    {
        bigint_free(&rem);
        return 0;
    }

    la = strlen(a->digits);
    qdigits = (char *)malloc(la + 1);
    /* if: comprueba qdigits == NULL antes de ejecutar esta rama. */
    if (qdigits == NULL)
        goto cleanup;

    /* for: itera segun size_t i = 0; i < la; i++ para recorrer el bloque. */
    for (size_t i = 0; i < la; i++)
    {
        BigInt rem10 = {0};
        BigInt digit = {0};
        BigInt nuevoRem = {0};
        int qd = 0;
        char digTxt[2];

        digTxt[0] = a->digits[i];
        digTxt[1] = '\0';

        /* if: comprueba !bigint_multiply(&rem, &(BigInt antes de ejecutar esta rama. */
        if (!bigint_multiply(&rem, &(BigInt){"10"}, &rem10))
            goto cleanup;

        /* if: comprueba !bigint_init(&digit, digTxt) antes de ejecutar esta rama. */
        if (!bigint_init(&digit, digTxt))
        {
            bigint_free(&rem10);
            goto cleanup;
        }

        /* if: comprueba !bigint_add(&rem10, &digit, &nuevoRem) antes de ejecutar esta rama. */
        if (!bigint_add(&rem10, &digit, &nuevoRem))
        {
            bigint_free(&rem10);
            bigint_free(&digit);
            goto cleanup;
        }
        bigint_free(&rem);
        rem = nuevoRem;
        bigint_free(&rem10);
        bigint_free(&digit);

        /* while: repite el bloque mientras se cumpla bigint_compare(&rem, &d) >= 0. */
        while (bigint_compare(&rem, &d) >= 0)
        {
            BigInt tmp = {0};
            /* if: comprueba !bigint_subtract(&rem, &d, &tmp) antes de ejecutar esta rama. */
            if (!bigint_subtract(&rem, &d, &tmp))
                goto cleanup;
            bigint_free(&rem);
            rem = tmp;
            qd++;
        }

        qdigits[i] = (char)('0' + qd);
    }

    qdigits[la] = '\0';

    {
        BigInt qtmp = {0};
        BigInt rtmp = {0};
        char *nq = normalizar(qdigits);
        /* if: comprueba nq == NULL antes de ejecutar esta rama. */
        if (nq == NULL)
            goto cleanup;
        qtmp.digits = nq;

        /* if: comprueba !bigint_set(&rtmp, &rem) && !bigint_init(&rtmp, rem.digits) antes de ejecutar esta rama. */
        if (!bigint_set(&rtmp, &rem) && !bigint_init(&rtmp, rem.digits))
        {
            bigint_free(&qtmp);
            goto cleanup;
        }
        bigint_free(quotient);
        bigint_free(remainder);
        *quotient = qtmp;
        *remainder = rtmp;
        ok = 1;
    }

cleanup:
    free(qdigits);
    bigint_free(&rem);
    bigint_free(&d);
    return ok;
}

/* bigint_array_create: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_array_create: contiene la logica principal de esta operacion. */
int bigint_array_create(BigIntArray *arr, size_t len)
{
    unsigned long len_vector;

    /* if: comprueba arr == NULL || len == 0 antes de ejecutar esta rama. */
    if (arr == NULL || len == 0)
        return 0;

    /* if: comprueba len > (size_t)ULONG_MAX antes de ejecutar esta rama. */
    if (len > (size_t)ULONG_MAX)
        return 0;

    bigint_array_free(arr);

    len_vector = (unsigned long)len;
    crear_bytes(&arr->storage, len_vector, sizeof(BigInt));
    /* if: comprueba arr->storage == NULL antes de ejecutar esta rama. */
    if (arr->storage == NULL)
        return 0;

    arr->items = (BigInt *)datos(arr->storage);
    /* if: comprueba arr->items == NULL antes de ejecutar esta rama. */
    if (arr->items == NULL)
    {
        liberar(&arr->storage);
        return 0;
    }

    arr->len = (size_t)tamano(&arr->storage);
    /* for: itera segun size_t i = 0; i < len; i++ para recorrer el bloque. */
    for (size_t i = 0; i < len; i++)
    {

        /* if: comprueba !bigint_init(&arr->items[i], "0") antes de ejecutar esta rama. */
        if (!bigint_init(&arr->items[i], "0"))
        {
            bigint_array_free(arr);
            return 0;
        }
    }

    return 1;
}

/* bigint_array_set: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_array_set: contiene la logica principal de esta operacion. */
int bigint_array_set(BigIntArray *arr, size_t idx, const BigInt *value)
{
    /* if: comprueba arr == NULL || value == NULL || idx >= arr->len antes de ejecutar esta rama. */
    if (arr == NULL || value == NULL || idx >= arr->len)
        return 0;

    return bigint_set(&arr->items[idx], value);
}

/* bigint_array_free: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_array_free: contiene la logica principal de esta operacion. */
void bigint_array_free(BigIntArray *arr)
{
    /* if: comprueba arr == NULL antes de ejecutar esta rama. */
    if (arr == NULL)
        return;

    /* if: comprueba arr->items != NULL antes de ejecutar esta rama. */
    if (arr->items != NULL)
    {
        /* for: itera segun size_t i = 0; i < arr->len; i++ para recorrer el bloque. */
        for (size_t i = 0; i < arr->len; i++)
            bigint_free(&arr->items[i]);
    }

    /* if: comprueba arr->storage != NULL antes de ejecutar esta rama. */
    if (arr->storage != NULL)
    {
        liberar(&arr->storage);
    }
    /* if: comprueba arr->items != NULL antes de continuar con esta alternativa. */
    else if (arr->items != NULL)
    {
        free(arr->items);
    }

    arr->items = NULL;
    arr->len = 0;
    arr->storage = NULL;
}

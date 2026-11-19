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
    // El objetivo de este if es evitar que se intente duplicar un puntero nulo, lo cual podría causar un comportamiento indefinido o un fallo en tiempo de ejecución.
    // Al retornar NULL en este caso, se indica que no se pudo realizar la duplicación debido a un argumento inválido.
    // Explicación lógica detallada del funcionamiento: Si el puntero 's' es nulo, significa que no hay una cadena válida para duplicar. Por lo tanto, la función retorna NULL para indicar que no se puede crear una copia de una cadena inexistente.
    if (s == NULL)
        return NULL;

    len = strlen(s) + 1;
    copia = (char *)malloc(len);
    /* if: comprueba copia == NULL antes de ejecutar esta rama. */
    // El objetivo de este if es verificar si la asignación de memoria para la copia de la cadena fue exitosa. Si malloc devuelve NULL, significa que no se pudo asignar memoria suficiente, lo cual podría ocurrir por falta de recursos del sistema.
    // Al retornar NULL en este caso, se indica que la duplicación de la cadena no pudo completarse debido a un error de asignación de memoria.
    // Explicación lógica detallada del funcionamiento: La función malloc intenta reservar un bloque de memoria del tamaño especificado (len). Si la reserva falla, malloc devuelve NULL. Por lo tanto, se verifica si 'copia' es NULL para manejar adecuadamente el error y evitar desreferenciar un puntero nulo más adelante en el código.
    if (copia == NULL)
        return NULL;

    // La siguiente instrucción copia el contenido de la cadena original 's' a la nueva ubicación de memoria 'copia', incluyendo el carácter nulo de terminación.
    memcpy(copia, s, len);
    return copia;
}

/* texto_valido_entero_no_negativo: Funcion auxiliar. Ejecuta su logica, valida parametros de entrada y retorna un estado. */
/* funcion texto_valido_entero_no_negativo: contiene la logica principal de esta operacion. */
static int texto_valido_entero_no_negativo(const char *text)
{
    size_t i;

    /* if: comprueba text == NULL || *text == '\0' antes de ejecutar esta rama. */
    // El objetivo de este if es verificar si el puntero 'text' es nulo o si apunta a una cadena vacía. En ambos casos, no se puede considerar que el texto represente un entero no negativo válido.
    // Al retornar 0 en este caso, se indica que el texto no es válido como entero no negativo.
    if (text == NULL || *text == '\0')
        return 0;

    /* for: itera segun i = 0; text[i] != '\0'; i++ para recorrer el bloque. */
    // El objetivo de este bucle for es recorrer cada carácter de la cadena 'text' hasta llegar al carácter nulo de terminación. Se utiliza para verificar que todos los caracteres sean dígitos numéricos.
    for (i = 0; text[i] != '\0'; i++)
    {
        /* if: comprueba !isdigit((unsigned char)text[i]) antes de ejecutar esta rama. */
        // El objetivo de este if es verificar si el carácter actual no es un dígito numérico. La función isdigit() devuelve un valor distinto de cero si el carácter es un dígito, y cero en caso contrario.
        // Se utiliza (unsigned char) para evitar problemas con caracteres con signo en sistemas donde char es signed por defecto. Si se encuentra un carácter que no es un dígito, se retorna 0, indicando que el texto no es un entero no negativo válido.
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
    // El objetivo de este if es verificar si el puntero 'text' es nulo o si apunta a una cadena vacía. En ambos casos, no se puede considerar que el texto represente un entero no negativo válido.
    // Al retornar NULL en este caso, se indica que la normalización no se puede realizar debido a un texto inválido.
    if (text == NULL || *text == '\0')
        return NULL;

    /* while: repite el bloque mientras se cumpla text[i] == '0' && text[i + 1] != '\0'. */
    // El objetivo de este bucle while es avanzar el índice 'i' mientras se encuentren ceros a la izquierda en la cadena 'text', pero asegurándose de no eliminar el último cero si toda la cadena es "0".
    // Explicación lógica detallada del funcionamiento: El bucle continúa incrementando 'i' mientras el carácter actual sea '0' y el siguiente carácter no sea el final de la cadena. Esto permite eliminar los ceros a la izquierda, dejando al menos un dígito en la cadena resultante.
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
    // El objetivo de este if es verificar si el puntero 'n' es nulo o si el texto no representa un entero no negativo válido. En ambos casos, no se puede inicializar el BigInt.
    // Al retornar 0 en este caso, se indica que la inicialización no se pudo realizar debido a parámetros inválidos.
    // Explicación lógica detallada del funcionamiento: Si 'n' es NULL, significa que no hay una estructura BigInt válida para inicializar. Si el texto no es un entero no negativo válido, no se puede representar correctamente en la estructura BigInt. Por lo tanto, se retorna 0 para indicar el fallo en la inicialización.
    if (n == NULL || !texto_valido_entero_no_negativo(text))
        return 0;

    normalizado = normalizar(text);
    /* if: comprueba normalizado == NULL antes de ejecutar esta rama. */
    // El objetivo de este if es verificar si la normalización del texto falló, lo que podría ocurrir si hubo un error de memoria al duplicar la cadena. En este caso, no se puede inicializar el BigInt correctamente.
    // Al retornar 0 en este caso, se indica que la inicialización no se pudo realizar debido a un error en la normalización del texto.
    // Explicación lógica detallada del funcionamiento: Si la función normalizar devuelve NULL, significa que no se pudo crear una copia válida del texto normalizado, posiblemente debido a un fallo de memoria. Por lo tanto, se retorna 0 para indicar que la inicialización del BigInt no se pudo completar correctamente.
    if (normalizado == NULL)
        return 0;

    // La siguiente instrucción asigna la cadena normalizada al campo 'digits' de la estructura BigInt, completando así la inicialización del número grande.
    // Explicación lógica detallada del funcionamiento: Se asigna la cadena normalizada a 'n->digits', lo que permite que la estructura BigInt represente correctamente el número grande sin ceros a la izquierda. La función retorna 1 para indicar que la inicialización fue exitosa.
    n->digits = normalizado;
    return 1;
}

/* bigint_set: Funcion de precision arbitraria para operar con numeros grandes. */
/* funcion bigint_set: contiene la logica principal de esta operacion. */
int bigint_set(BigInt *dest, const BigInt *src)
{
    char *copia;

    /* if: comprueba dest == NULL || src == NULL || src->digits == NULL antes de ejecutar esta rama. */
    // El objetivo de este if es verificar si los punteros 'dest' o 'src' son nulos, o si 'src->digits' es nulo. En cualquiera de estos casos, no se puede realizar la operación de copia.
    // Al retornar 0 en este caso, se indica que la operación de copia no se pudo realizar debido a parámetros inválidos.
    // Explicación lógica detallada del funcionamiento: Si 'dest' es NULL, significa que no hay una estructura BigInt válida para recibir la copia. Si 'src' es NULL o 'src->digits' es NULL, significa que no hay un número grande válido para copiar. Por lo tanto, se retorna 0 para indicar que la operación de copia no se pudo completar correctamente.
    if (dest == NULL || src == NULL || src->digits == NULL)
        return 0;

    copia = duplicar(src->digits);
    /* if: comprueba copia == NULL antes de ejecutar esta rama. */
    // El objetivo de este if es verificar si la duplicación de la cadena 'src->digits' falló, lo que podría ocurrir si hubo un error de memoria al duplicar la cadena. En este caso, no se puede realizar la operación de copia correctamente.
    // Al retornar 0 en este caso, se indica que la operación de copia no se pudo realizar debido a un error en la duplicación de la cadena.
    // Explicación lógica detallada del funcionamiento: Si la función duplicar devuelve NULL, significa que no se pudo crear una copia válida de la cadena 'src->digits', posiblemente debido a un fallo de memoria. Por lo tanto, se retorna 0 para indicar que la operación de copia no se pudo completar correctamente.
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
    // El objetivo de este if es verificar si el puntero 'n' es nulo. Si es nulo, no hay una estructura BigInt válida para liberar, y por lo tanto, no se debe intentar acceder a ella.
    // Al retornar en este caso, se evita un posible fallo en tiempo de ejecución al intentar liberar memoria de un puntero nulo.
    // Explicación lógica detallada del funcionamiento: Si 'n' es NULL, significa que no hay una estructura BigInt válida para liberar. Intentar acceder a un puntero nulo podría causar un comportamiento indefinido o un fallo en tiempo de ejecución. Por lo tanto, se retorna inmediatamente para evitar cualquier operación sobre un puntero nulo.
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

/* Conserva elementos existentes y la validez del alias arr->items tras realloc. */
int bigint_array_resize(BigIntArray *arr, size_t new_len)
{
    size_t previous, i;
    if (arr == NULL || arr->storage == NULL || new_len > (size_t)ULONG_MAX)
        return 0;
    previous = arr->len;
    if (new_len < previous)
        for (i = new_len; i < previous; ++i)
            bigint_free(&arr->items[i]);
    if (!redimensionar_bytes(arr->storage, (unsigned long)new_len))
        return 0;
    arr->items = (BigInt *)datos(arr->storage);
    arr->len = new_len;
    if (new_len > previous)
    {
        for (i = previous; i < new_len; ++i)
        {
            if (!bigint_init(&arr->items[i], "0"))
            {
                size_t j;
                for (j = previous; j < i; ++j)
                    bigint_free(&arr->items[j]);
                (void)redimensionar_bytes(arr->storage, (unsigned long)previous);
                arr->items = (BigInt *)datos(arr->storage);
                arr->len = previous;
                return 0;
            }
        }
    }
    return 1;
}

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

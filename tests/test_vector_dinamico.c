#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vector_dinamico.h"
#include "bigint.h"

int main(void)
{
    vectorP v = NULL;
    BigIntArray big = {0};
    BigInt value = {0};
    assert(tamano(&v) == 0);
    crear(&v, 0);
    assert(v != NULL && capacidad_vector(v) == 0);
    assert(redimensionar(v, 3));
    assert(recuperar(v, 2) == 0);
    asignar(v, 1, 42);
    assert(recuperar(v, 1) == 42);
    assert(reservar(v, 16));
    assert(capacidad_vector(v) >= 16);
    /* La fuente del append esta dentro del vector. */
    assert(agregar_bytes(v, (TELEMENTO *)datos(v) + 1));
    assert(recuperar(v, 3) == 42);
    assert(redimensionar(v, 1));
    assert(redimensionar(v, 4));
    assert(recuperar(v, 1) == 0 && recuperar(v, 3) == 0);
    assert(agregar(v, 9));
    assert(recuperar(v, 4) == 9);
    if ((uintmax_t)ULONG_MAX > SIZE_MAX / sizeof(TELEMENTO))
        assert(!reservar_bytes(v, ULONG_MAX));
    assert(tamano(&v) == 5);
    liberar(&v);
    liberar(&v);
    assert(v == NULL);

    assert(bigint_array_create(&big, 2));
    assert(bigint_init(&value, "998877665544332211"));
    assert(bigint_array_set(&big, 1, &value));
    assert(bigint_array_resize(&big, 9));
    assert(big.len == 9 && big.items != NULL);
    assert(strcmp(big.items[1].digits, "998877665544332211") == 0);
    assert(strcmp(big.items[8].digits, "0") == 0);
    assert(bigint_array_resize(&big, 1));
    assert(big.len == 1);
    assert(bigint_array_resize(&big, 0));
    assert(bigint_array_resize(&big, 3));
    assert(strcmp(big.items[2].digits, "0") == 0);
    bigint_free(&value);
    bigint_array_free(&big);
    puts("generic vector and BigInt resize: OK");
    return 0;
}

#include <stdio.h>
#include <string.h>
#include "bigint.h"
#include "algoritmo_cambio.h"

static int fallos = 0;

static void check_int(int condicion, const char *mensaje)
{
    if (!condicion)
    {
        printf("FAIL: %s\n", mensaje);
        fallos++;
    }
}

static void check_bigint_texto(const BigInt *valor, const char *esperado, const char *mensaje)
{
    if (valor == NULL || valor->digits == NULL || strcmp(valor->digits, esperado) != 0)
    {
        printf("FAIL: %s (esperado=%s, obtenido=%s)\n",
               mensaje,
               esperado,
               valor != NULL && valor->digits != NULL ? valor->digits : "(null)");
        fallos++;
    }
}

static int crear_arreglo(BigIntArray *arr, const char *const *valores, size_t len)
{
    size_t i;

    if (!bigint_array_create(arr, len))
        return 0;

    for (i = 0; i < len; i++)
    {
        BigInt tmp = {0};

        if (!bigint_init(&tmp, valores[i]) ||
            !bigint_array_set(arr, i, &tmp))
        {
            bigint_free(&tmp);
            bigint_array_free(arr);
            return 0;
        }

        bigint_free(&tmp);
    }

    return 1;
}

static void check_arreglo_texto(const BigIntArray *arr, const char *const *esperado, size_t len, const char *mensaje)
{
    size_t i;

    if (arr == NULL || arr->items == NULL || arr->len != len)
    {
        printf("FAIL: %s (longitud inesperada)\n", mensaje);
        fallos++;
        return;
    }

    for (i = 0; i < len; i++)
    {
        if (strcmp(arr->items[i].digits, esperado[i]) != 0)
        {
            printf("FAIL: %s en indice %zu (esperado=%s, obtenido=%s)\n",
                   mensaje,
                   i,
                   esperado[i],
                   arr->items[i].digits);
            fallos++;
        }
    }
}

static void test_bigint_basico(void)
{
    BigInt a = {0};
    BigInt b = {0};
    BigInt out = {0};
    BigInt q = {0};
    BigInt r = {0};

    check_int(bigint_init(&a, "00012345678901234567890"), "inicializa bigint con ceros a la izquierda");
    check_int(bigint_init(&b, "9876543210"), "inicializa segundo bigint");
    check_bigint_texto(&a, "12345678901234567890", "normaliza bigint");

    check_int(bigint_add(&a, &b, &out), "suma bigint");
    check_bigint_texto(&out, "12345678911111111100", "resultado suma");

    check_int(bigint_subtract(&a, &b, &out), "resta bigint");
    check_bigint_texto(&out, "12345678891358024680", "resultado resta");

    bigint_free(&a);
    bigint_free(&b);
    check_int(bigint_init(&a, "12345"), "inicializa multiplicando");
    check_int(bigint_init(&b, "6789"), "inicializa multiplicador");
    check_int(bigint_multiply(&a, &b, &out), "multiplica bigint");
    check_bigint_texto(&out, "83810205", "resultado multiplicacion");

    bigint_free(&a);
    bigint_free(&b);
    check_int(bigint_init(&a, "123456789"), "inicializa dividendo");
    check_int(bigint_init(&b, "97"), "inicializa divisor");
    check_int(bigint_divmod(&a, &b, &q, &r), "divide bigint");
    check_bigint_texto(&q, "1272750", "cociente division");
    check_bigint_texto(&r, "39", "resto division");

    bigint_free(&a);
    bigint_free(&b);
    bigint_free(&out);
    bigint_free(&q);
    bigint_free(&r);
}

static void test_cambio_no_canonico_ilimitado(void)
{
    const char *denom_txt[] = {"4", "3", "1"};
    const char *esperado[] = {"0", "2", "0"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "6"), "inicializa monto no canonico");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones no canonicas");
    check_int(calcular_cambio_optimo(&monto, &denom, &solucion), "calcula cambio no canonico ilimitado");
    check_arreglo_texto(&solucion, esperado, 3, "elige minimo numero de monedas en modo ilimitado");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&solucion);
}

static void test_cambio_no_canonico_con_stock(void)
{
    const char *denom_txt[] = {"4", "3", "1"};
    const char *stock_txt[] = {"1", "2", "10"};
    const char *esperado[] = {"0", "2", "0"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "6"), "inicializa monto con stock");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones con stock");
    check_int(crear_arreglo(&stock, stock_txt, 3), "crea stock");
    check_int(calcular_cambio_optimo_stock(&monto, &denom, &stock, &solucion), "calcula cambio no canonico con stock");
    check_arreglo_texto(&solucion, esperado, 3, "elige minimo numero de monedas respetando stock");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    bigint_array_free(&solucion);
}

static void test_sin_solucion_con_stock(void)
{
    const char *denom_txt[] = {"5", "3"};
    const char *stock_txt[] = {"0", "1"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "10"), "inicializa monto sin solucion");
    check_int(crear_arreglo(&denom, denom_txt, 2), "crea denominaciones sin solucion");
    check_int(crear_arreglo(&stock, stock_txt, 2), "crea stock insuficiente");
    check_int(!calcular_cambio_optimo_stock(&monto, &denom, &stock, &solucion), "detecta ausencia de solucion con stock");
    check_int(solucion.items == NULL && solucion.len == 0, "libera solucion fallida");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    bigint_array_free(&solucion);
}

int main(void)
{
    test_bigint_basico();
    test_cambio_no_canonico_ilimitado();
    test_cambio_no_canonico_con_stock();
    test_sin_solucion_con_stock();

    if (fallos != 0)
    {
        printf("%d prueba(s) fallaron.\n", fallos);
        return 1;
    }

    printf("Todas las pruebas pasaron.\n");
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "bigint.h"
#include "algoritmo_cambio.h"

static int fallos = 0;

/* funcion check_int: contiene la logica principal de esta operacion. */
static void check_int(int condicion, const char *mensaje)
{
    /* if: comprueba !condicion antes de ejecutar esta rama. */
    if (!condicion)
    {
        printf("FAIL: %s\n", mensaje);
        fallos++;
    }
}

/* funcion check_bigint_texto: contiene la logica principal de esta operacion. */
static void check_bigint_texto(const BigInt *valor, const char *esperado, const char *mensaje)
{
    /* if: comprueba valor == NULL || valor->digits == NULL || strcmp(valor->digits, esper... antes de ejecutar esta rama. */
    if (valor == NULL || valor->digits == NULL || strcmp(valor->digits, esperado) != 0)
    {
        printf("FAIL: %s (esperado=%s, obtenido=%s)\n",
               mensaje,
               esperado,
               valor != NULL && valor->digits != NULL ? valor->digits : "(null)");
        fallos++;
    }
}

/* funcion crear_arreglo: contiene la logica principal de esta operacion. */
static int crear_arreglo(BigIntArray *arr, const char *const *valores, size_t len)
{
    size_t i;

    /* if: comprueba !bigint_array_create(arr, len) antes de ejecutar esta rama. */
    if (!bigint_array_create(arr, len))
        return 0;

    /* for: itera segun i = 0; i < len; i++ para recorrer el bloque. */
    for (i = 0; i < len; i++)
    {
        BigInt tmp = {0};

        /* if: comprueba !bigint_init(&tmp, valores[i]) || antes de ejecutar esta rama. */
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

/* funcion check_arreglo_texto: contiene la logica principal de esta operacion. */
static void check_arreglo_texto(const BigIntArray *arr, const char *const *esperado, size_t len, const char *mensaje)
{
    size_t i;

    /* if: comprueba arr == NULL || arr->items == NULL || arr->len != len antes de ejecutar esta rama. */
    if (arr == NULL || arr->items == NULL || arr->len != len)
    {
        printf("FAIL: %s (longitud inesperada)\n", mensaje);
        fallos++;
        return;
    }

    /* for: itera segun i = 0; i < len; i++ para recorrer el bloque. */
    for (i = 0; i < len; i++)
    {
        /* if: comprueba strcmp(arr->items[i].digits, esperado[i]) != 0 antes de ejecutar esta rama. */
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

/* funcion test_bigint_basico: contiene la logica principal de esta operacion. */
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

/* funcion test_cambio_no_canonico_ilimitado: contiene la logica principal de esta operacion. */
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

/* funcion test_cambio_no_canonico_con_stock: contiene la logica principal de esta operacion. */
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

/* funcion test_sin_solucion_con_stock: contiene la logica principal de esta operacion. */
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

/* funcion test_cambio_cercano_con_rango_ilimitado: contiene la logica principal de esta operacion. */
static void test_cambio_cercano_con_rango_ilimitado(void)
{
    const char *denom_txt[] = {"10", "4"};
    const char *esperado[] = {"0", "1"};
    BigInt monto = {0};
    BigInt cubierto = {0};
    BigIntArray denom = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "7"), "inicializa monto cercano ilimitado");
    check_int(crear_arreglo(&denom, denom_txt, 2), "crea denominaciones cercano ilimitado");
    check_int(calcular_cambio_cercano_con_rango(&monto, &denom, 1, 1, &cubierto, &solucion),
              "calcula cambio cercano con rango en modo ilimitado");
    check_bigint_texto(&cubierto, "4", "monto cubierto cercano ilimitado");
    check_arreglo_texto(&solucion, esperado, 2, "combinacion cercana ilimitada esperada");

    bigint_free(&monto);
    bigint_free(&cubierto);
    bigint_array_free(&denom);
    bigint_array_free(&solucion);
}

/* funcion test_cambio_cercano_con_rango_stock: contiene la logica principal de esta operacion. */
static void test_cambio_cercano_con_rango_stock(void)
{
    const char *denom_txt[] = {"10", "4", "1"};
    const char *stock_txt[] = {"1", "1", "0"};
    const char *esperado[] = {"1", "1", "0"};
    BigInt monto = {0};
    BigInt cubierto = {0};
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "15"), "inicializa monto cercano con stock");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones cercano con stock");
    check_int(crear_arreglo(&stock, stock_txt, 3), "crea stock cercano");
    check_int(calcular_cambio_cercano_stock_con_rango(&monto, &denom, &stock, 2, 2, &cubierto, &solucion),
              "calcula cambio cercano con rango en modo stock");
    check_bigint_texto(&cubierto, "14", "monto cubierto cercano con stock");
    check_arreglo_texto(&solucion, esperado, 3, "combinacion cercana con stock esperada");

    bigint_free(&monto);
    bigint_free(&cubierto);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    bigint_array_free(&solucion);
}

/* funcion test_cambio_exacto_con_rango_ilimitado: contiene la logica principal de esta operacion. */
static void test_cambio_exacto_con_rango_ilimitado(void)
{
    const char *denom_txt[] = {"10", "6", "1"};
    const char *esperado[] = {"0", "2", "0"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "12"), "inicializa monto exacto con rango");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones exacto con rango");
    check_int(calcular_cambio_optimo_con_rango(&monto, &denom, 2, 2, &solucion),
              "calcula cambio exacto con dos monedas");
    check_arreglo_texto(&solucion, esperado, 3, "combinacion exacta con rango esperada");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&solucion);
}

/* funcion test_cambio_exacto_con_rango_stock: contiene la logica principal de esta operacion. */
static void test_cambio_exacto_con_rango_stock(void)
{
    const char *denom_txt[] = {"10", "6", "1"};
    const char *stock_txt[] = {"1", "2", "10"};
    const char *esperado[] = {"0", "2", "0"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "12"), "inicializa monto exacto con stock y rango");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones exacto con stock y rango");
    check_int(crear_arreglo(&stock, stock_txt, 3), "crea stock exacto con rango");
    check_int(calcular_cambio_optimo_stock_con_rango(&monto, &denom, &stock, 2, 2, &solucion),
              "calcula cambio exacto con stock y rango");
    check_arreglo_texto(&solucion, esperado, 3, "combinacion exacta con stock y rango esperada");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    bigint_array_free(&solucion);
}

/* funcion test_limite_monedas_ilimitado: contiene la logica principal de esta operacion. */
static void test_limite_monedas_ilimitado(void)
{
    const char *denom_txt[] = {"9", "6", "3"};
    const char *esperado[] = {"1", "0", "1"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "12"), "inicializa monto con limite ilimitado");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones con limite ilimitado");
    check_int(calcular_cambio_optimo_con_limite(&monto, &denom, 2, &solucion),
              "calcula cambio exacto dentro del limite");
    check_arreglo_texto(&solucion, esperado, 3, "respeta limite de monedas en modo ilimitado");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&solucion);
}

/* funcion test_limite_monedas_stock_sin_solucion: contiene la logica principal de esta operacion. */
static void test_limite_monedas_stock_sin_solucion(void)
{
    const char *denom_txt[] = {"10", "6", "1"};
    const char *stock_txt[] = {"1", "1", "10"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray stock = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "12"), "inicializa monto sin solucion por limite y stock");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones sin solucion por limite y stock");
    check_int(crear_arreglo(&stock, stock_txt, 3), "crea stock sin solucion por limite y stock");
    check_int(!calcular_cambio_optimo_stock_con_limite(&monto, &denom, &stock, 2, &solucion),
              "detecta ausencia de solucion por limite y stock");
    check_int(solucion.items == NULL && solucion.len == 0, "no deja solucion parcial tras fallo por limite y stock");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
    bigint_array_free(&solucion);
}

/* funcion test_cambio_cero_con_rango: contiene la logica principal de esta operacion. */
static void test_cambio_cero_con_rango(void)
{
    const char *denom_txt[] = {"10", "5", "1"};
    const char *esperado[] = {"0", "0", "0"};
    BigInt monto = {0};
    BigIntArray denom = {0};
    BigIntArray solucion = {0};

    check_int(bigint_init(&monto, "0"), "inicializa monto cero");
    check_int(crear_arreglo(&denom, denom_txt, 3), "crea denominaciones para monto cero");
    check_int(calcular_cambio_optimo_con_rango(&monto, &denom, 0, 3, &solucion),
              "resuelve monto cero cuando el minimo permitido es cero");
    check_arreglo_texto(&solucion, esperado, 3, "devuelve vector nulo para monto cero");

    bigint_free(&monto);
    bigint_array_free(&denom);
    bigint_array_free(&solucion);
}

/* funcion main: contiene la logica principal de esta operacion. */
static void test_cercano_bigint_un_tipo(void)
{
    const char *denom_text[] = {"50000"};
    const char *stock_text[] = {"2"};
    const char *una[] = {"1"};
    const char *dos[] = {"2"};
    BigInt monto = {0}, cubierto = {0};
    BigIntArray denom = {0}, stock = {0}, solucion = {0};

    check_int(bigint_init(&monto, "50001"), "crea importe fuera de DP");
    check_int(crear_arreglo(&denom, denom_text, 1), "denominacion unica grande");
    check_int(crear_arreglo(&stock, stock_text, 1), "stock acotado del importe");
    check_int(calcular_cambio_cercano_con_rango(&monto, &denom, 0, 1,
               &cubierto, &solucion), "cercano grande exacto con maximo");
    check_bigint_texto(&cubierto, "50000", "cercano grande cubierto");
    check_arreglo_texto(&solucion, una, 1, "cercano grande cantidad maxima");
    bigint_free(&cubierto); bigint_array_free(&solucion);

    check_int(calcular_cambio_cercano_stock_con_rango(&monto, &denom, &stock,
               1, 1, &cubierto, &solucion), "cercano grande stock y rango");
    check_bigint_texto(&cubierto, "50000", "cercano grande stock cubierto");
    check_arreglo_texto(&solucion, una, 1, "cercano grande stock cantidad");
    bigint_free(&cubierto); bigint_array_free(&solucion);
    check_int(!calcular_cambio_cercano_stock_con_rango(&monto, &denom, &stock,
               3, 3, &cubierto, &solucion), "cercano grande minimo imposible");
    bigint_free(&cubierto); bigint_array_free(&solucion);

    bigint_free(&monto);
    check_int(bigint_init(&monto, "150000"), "importe grande con dos piezas");
    check_int(calcular_cambio_cercano_stock_con_rango(&monto, &denom, &stock,
               0, 3, &cubierto, &solucion), "stock limita cercano alto");
    check_bigint_texto(&cubierto, "100000", "stock cercano cubierto");
    check_arreglo_texto(&solucion, dos, 1, "stock cercano usa dos piezas");

    bigint_free(&cubierto); bigint_array_free(&solucion);
    bigint_free(&monto); bigint_array_free(&denom); bigint_array_free(&stock);
}

static void test_bigint_rango_exacto_un_tipo(void)
{
    const char *d[] = {"50001"};
    const char *stocks[] = {"1"};
    const char *expected[] = {"1"};
    BigInt amount = {0};
    BigIntArray denom = {0}, stock = {0}, result = {0};
    check_int(bigint_init(&amount, "50001"), "importe unico rango BigInt");
    check_int(crear_arreglo(&denom, d, 1), "denominacion unica");
    check_int(crear_arreglo(&stock, stocks, 1), "stock una unidad");
    check_int(calcular_cambio_optimo_con_rango(&amount, &denom, 1, 1, &result),
              "acepta minimo positivo por encima del umbral");
    check_arreglo_texto(&result, expected, 1, "rango exacto con 1 pieza");
    bigint_array_free(&result);
    check_int(calcular_cambio_optimo_stock_con_rango(
                  &amount, &denom, &stock, 1, 1, &result),
              "stock rango positivo BigInt");
    check_arreglo_texto(&result, expected, 1, "stock BigInt 1 pieza");
    bigint_array_free(&result);
    check_int(!calcular_cambio_optimo_stock_con_rango(
                   &amount, &denom, &stock, 0, 0, &result),
              "rechaza maximo cero con importe positivo");
    bigint_array_free(&result);
    check_int(!calcular_cambio_optimo_con_rango(
                   &amount, &denom, 2, 2, &result),
              "rechaza dos piezas cuando solo existe una");
    bigint_array_free(&result);
    bigint_free(&amount);
    bigint_array_free(&denom);
    bigint_array_free(&stock);
}

static void test_cercano_bigint_dos_tipos(void)
{
    const char *d[]={"50000","30000"};
    const char *first[]={"1","0"};
    const char *second[]={"11","2"};
    BigIntArray denom={0},solucion={0};
    BigInt monto={0},cubierto={0};

    check_int(crear_arreglo(&denom,d,2),"cercano dos denom inicializadas");
    check_int(bigint_init(&monto,"50001"),"cercano dos monto fuera DP");
    check_int(calcular_cambio_cercano_con_rango(&monto,&denom,0,
        SIZE_MAX,&cubierto,&solucion),"cercano dos cubre parcial grande");
    check_bigint_texto(&cubierto,"50000","cercano dos monto correcto");
    check_arreglo_texto(&solucion,first,2,"cercano dos piezas minimas");
    bigint_array_free(&solucion);bigint_free(&cubierto);bigint_free(&monto);

    check_int(bigint_init(&monto,"610001"),"cercano dos monto no canonico");
    check_int(calcular_cambio_cercano_con_rango(&monto,&denom,0,
        SIZE_MAX,&cubierto,&solucion),"cercano dos soluciona no canonico");
    check_bigint_texto(&cubierto,"610000","cercano dos mejor monto mixto");
    check_arreglo_texto(&solucion,second,2,"cercano dos usa minimo de piezas");
    bigint_array_free(&solucion);bigint_free(&cubierto);bigint_free(&monto);
    bigint_array_free(&denom);
}

static void test_cercano_bigint_dos_tipos_stock_mas_512(void)
{
    const char *coins[]={"50000","29999"};
    const char *s1[]={"1000","1000"},*s2[]={"1","513"},*s3[]={"0","513"};
    const char *e1[]={"10","0"},*e2[]={"1","15"},*e3[]={"0","16"};
    BigInt amount={0},covered={0};
    BigIntArray denom={0},stock={0},answer={0};

    check_int(bigint_init(&amount,"500001"),"dos tipos stock monto > limite DP");
    check_int(crear_arreglo(&denom,coins,2),"dos tipos stock denominaciones");
    check_int(crear_arreglo(&stock,s1,2),"dos tipos stock 2000 unidades");
    check_int(calcular_cambio_cercano_stock_con_rango(&amount,&denom,&stock,
        0,SIZE_MAX,&covered,&answer),"stock >512 closest");
    check_bigint_texto(&covered,"500000","stock >512 importe correcto");
    check_arreglo_texto(&answer,e1,2,"stock >512 cuenta minima");
    bigint_array_free(&answer);bigint_array_free(&stock);bigint_free(&covered);

    check_int(crear_arreglo(&stock,s2,2),"moneda mayor limitada stock");
    check_int(calcular_cambio_cercano_stock_con_rango(&amount,&denom,&stock,
        0,SIZE_MAX,&covered,&answer),"stock >512 mayor limitada");
    check_bigint_texto(&covered,"499985","stock mayor limitada importe correcto");
    check_arreglo_texto(&answer,e2,2,"stock mayor limitada cuenta minima");
    bigint_array_free(&answer);bigint_array_free(&stock);bigint_free(&covered);

    check_int(crear_arreglo(&stock,s3,2),"moneda mayor agotada stock");
    check_int(calcular_cambio_cercano_stock_con_rango(&amount,&denom,&stock,
        0,SIZE_MAX,&covered,&answer),"stock >512 mayor agotada");
    check_bigint_texto(&covered,"479984","stock mayor agotada importe correcto");
    check_arreglo_texto(&answer,e3,2,"stock mayor agotada cuenta minima");
    bigint_array_free(&answer);bigint_array_free(&stock);bigint_free(&covered);

    bigint_array_free(&denom);bigint_free(&amount);
}

int main(void)
{
    test_cercano_bigint_dos_tipos_stock_mas_512();
    test_cercano_bigint_dos_tipos();
    test_cercano_bigint_un_tipo();
    test_bigint_rango_exacto_un_tipo();
    test_bigint_basico();
    test_cambio_no_canonico_ilimitado();
    test_cambio_no_canonico_con_stock();
    test_sin_solucion_con_stock();
    test_cambio_cercano_con_rango_ilimitado();
    test_cambio_cercano_con_rango_stock();
    test_cambio_exacto_con_rango_ilimitado();
    test_cambio_exacto_con_rango_stock();
    test_limite_monedas_ilimitado();
    test_limite_monedas_stock_sin_solucion();
    test_cambio_cero_con_rango();

    /* if: comprueba fallos != 0 antes de ejecutar esta rama. */
    if (fallos != 0)
    {
        printf("%d prueba(s) fallaron.\n", fallos);
        return 1;
    }

    printf("Todas las pruebas pasaron.\n");
    return 0;
}

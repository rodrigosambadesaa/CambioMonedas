# Especificacion formal adoptada para BigInt (entero no negativo)

Esta especificacion se basa en operaciones nucleares comunes de tipos BigInt en bibliotecas estandar y documentacion publica:

- Java `java.math.BigInteger` (constructor desde texto, comparacion, suma, resta, multiplicacion, division, remainder, pow, min/max, equals).
- Definicion general de aritmetica de precision arbitraria (representacion en arreglo/cadena de digitos y operaciones exactas).
- Referencia de alcance de API y operaciones avanzadas: `983/bigint` (https://github.com/983/bigint, Unlicense).

## Modelo de datos

- `BigInt` representa un entero decimal no negativo.
- Forma canonica: sin ceros a la izquierda, salvo el valor `0`.
- `digits` es una cadena terminada en `\0` con caracteres `0..9`.

## Contrato de API

- Todas las funciones retornan `1` en exito y `0` en error.
- Error incluye: punteros nulos, texto invalido, divisor cero y fallos de memoria.
- En operaciones aritmeticas, `out` recibe un valor canonico en exito.

### Construccion y ciclo de vida

- `bigint_init`
- `bigint_set`
- `bigint_free`

### Comparacion

- `bigint_compare`
- `bigint_is_zero`
- `bigint_equals`
- `bigint_min`
- `bigint_max`

### Aritmetica

- `bigint_add`
- `bigint_subtract` (precondicion: `a >= b`)
- `bigint_multiply`
- `bigint_divmod` (precondicion: divisor != 0)
- `bigint_divide` (atajo de cociente)
- `bigint_remainder` (atajo de residuo)
- `bigint_pow_u32` (exponente entero no negativo)

### Arreglos de BigInt

- `bigint_array_create`
- `bigint_array_set`
- `bigint_array_free`

## Alcance explicito

- No se soportan valores negativos.
- No se soportan bases distintas de decimal en la interfaz publica.
- No se incluyen operaciones bit a bit ni primalidad.

## Nota de integracion y referencia

Se integraron capacidades inspiradas en `983/bigint` adaptadas al modelo local (decimal, no negativo, API por retorno `1/0`):

- `bigint_pow_mod_u32`
- `bigint_gcd`
- `bigint_bit_length`
- `bigint_shift_left_bits`
- `bigint_shift_right_bits`

La implementacion en este repositorio es propia y adaptada; no replica literalmente el codigo fuente del proyecto de referencia.

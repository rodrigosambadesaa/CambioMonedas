#ifndef ALGORITMO_CAMBIO_H
#define ALGORITMO_CAMBIO_H

#include "bigint.h"

/*
 * Calcula el cambio con el menor numero de monedas en modo ilimitado cuando
 * el monto cabe en la ruta de programacion dinamica. Para montos BigInt muy
 * grandes conserva una busqueda descendente compatible con precision arbitraria.
 * Retorna 1 si encuentra solución, 0 en caso contrario.
 */
int calcular_cambio_optimo(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion);

/*
 * Calcula el cambio con el menor numero de monedas respetando stock cuando
 * el monto cabe en la ruta de programacion dinamica. Para montos BigInt muy
 * grandes conserva una busqueda descendente con poda por stock.
 * Retorna 1 si encuentra solución, 0 en caso contrario.
 */
int calcular_cambio_optimo_stock(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, BigIntArray *solucion);

#endif

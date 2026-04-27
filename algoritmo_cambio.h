#ifndef ALGORITMO_CAMBIO_H
#define ALGORITMO_CAMBIO_H

#include "bigint.h"

/*
 * Calcula el cambio óptimo (modo ilimitado) usando backtracking.
 * Retorna 1 si encuentra solución, 0 en caso contrario.
 */
int calcular_cambio_optimo(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion);

/*
 * Calcula el cambio óptimo respetando el stock disponible usando backtracking.
 * Retorna 1 si encuentra solución, 0 en caso contrario.
 */
int calcular_cambio_optimo_stock(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, BigIntArray *solucion);

#endif

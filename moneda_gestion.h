#ifndef MONEDA_GESTION_H
#define MONEDA_GESTION_H

#include "bigint.h"

/*
 * Carga las denominaciones de una moneda desde monedas.txt.
 * Devuelve 1 si la moneda existe y los datos son validos; 0 en caso contrario.
 */
int cargar_denominaciones_moneda(const char *nombreMoneda, BigIntArray *resultado);

/*
 * Carga el stock de una moneda desde stock.txt.
 * El valor -1 en archivo se normaliza a 0 en memoria.
 * Devuelve 1 si la moneda existe y los datos son validos; 0 en caso contrario.
 */
int cargar_stock_moneda(const char *nombreMoneda, BigIntArray *resultado);

/*
 * Persiste en stock.txt el vector de stock de la moneda indicada,
 * escribiendo primero en un archivo temporal y luego reemplazando stock.txt.
 * Devuelve 1 si pudo actualizar; 0 en caso contrario.
 */
int actualizar_stock_moneda(const char *nombreMoneda, const BigIntArray *stock);

/*
 * Valida que una moneda tenga igual cantidad de denominaciones y stock,
 * y que las denominaciones esten en orden descendente estricto.
 * Devuelve 1 si es consistente; 0 en caso contrario.
 */
int validar_consistencia_moneda(const char *nombreMoneda);

/*
 * Crea una copia completa del archivo stock.txt en la ruta indicada.
 * Si rutaSnapshot es NULL o vacia, usa "stock_snapshot.txt".
 * Devuelve 1 si pudo copiar; 0 en caso contrario.
 */
int crear_snapshot_stock(const char *rutaSnapshot);

/*
 * Restaura stock.txt desde una copia previa.
 * Si rutaSnapshot es NULL o vacia, usa "stock_snapshot.txt".
 * Devuelve 1 si pudo restaurar; 0 en caso contrario.
 */
int restaurar_snapshot_stock(const char *rutaSnapshot);

/*
 * Exporta un reporte global por moneda con cantidad de denominaciones,
 * total de piezas y valor total de stock.
 * Si rutaReporte es NULL o vacia, usa "reporte_global.txt".
 * Devuelve 1 si pudo generar el archivo; 0 en caso contrario.
 */
int exportar_reporte_global(const char *rutaReporte);

#endif

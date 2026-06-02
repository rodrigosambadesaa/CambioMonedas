#ifndef BATCH_CLI_H
#define BATCH_CLI_H

#include <stddef.h>

/* Procesa un archivo CSV en modo batch.
 * Cada fila: modo,moneda,monto[,min-max]
 * Si rutaSalida es NULL escribe a stdout.
 * Retorna 1 si pudo procesar, 0 en caso de error.
 */
int batch_process_file(const char *rutaEntrada, const char *rutaSalida, const char *rutaLog);

/* Procesa entradas desde stdin y escribe resultados a stdout (modo streaming).
 * Retorna 1 si pudo procesar, 0 en caso de error.
 */
int batch_process_stream(const char *rutaLog);

#endif

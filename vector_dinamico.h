#ifndef VECTOR_DINAMICO_H
#define VECTOR_DINAMICO_H

#include <stddef.h>

typedef int TELEMENTO;
typedef struct STVECTOR *vectorP;

/* Los vectores son propietarios SOLO del bloque de bytes, no de posibles
 * punteros/recursos dentro de los elementos. Para BigInt usar la API
 * bigint_array_resize, que gestiona los recursos de cada elemento.
 * crear/crear_bytes exigen un puntero de salida inicializado a NULL;
 * rechazan sobrescribir un vector vivo para evitar fugas.
 */
void crear_bytes(vectorP *v1, unsigned long tam1, size_t tam_elemento);
void *datos(vectorP v1);
size_t tam_elemento_vector(vectorP v1);
void crear(vectorP *v1, unsigned long tam1);
void asignar(vectorP v1, unsigned long posicion, TELEMENTO valor);
TELEMENTO recuperar(vectorP v1, unsigned long posicion);
void liberar(vectorP *v1);
unsigned long tamano(vectorP *v1);

/* Capacidad y longitud son distintas. Reserva y redimensionamiento devuelven
 * 1 si tuvieron exito, 0 si fallaron sin perder los datos existentes.
 * Las posiciones nuevas se inicializan a cero; la reduccion descarta bytes
 * pero NO destruye los recursos de cada elemento.
 * IMPORTANTE: realloc puede mover el bloque; llamar a datos() otra vez
 * tras cada operacion de reserva/cambio de tamano.
 */
unsigned long capacidad_vector(vectorP v1);
int reservar_bytes(vectorP v1, unsigned long capacidad);
int redimensionar_bytes(vectorP v1, unsigned long nuevo_tam);
int agregar_bytes(vectorP v1, const void *elemento);
int reservar(vectorP v1, unsigned long capacidad);
int redimensionar(vectorP v1, unsigned long nuevo_tam);
int agregar(vectorP v1, TELEMENTO valor);

#endif

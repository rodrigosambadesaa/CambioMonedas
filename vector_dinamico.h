#ifndef VECTOR_DINAMICO_H
#define VECTOR_DINAMICO_H

#include <stddef.h>

/* Tipo base almacenado por el vector dinamico. */
typedef int TELEMENTO;

/* Puntero opaco a la estructura interna del vector. */
typedef struct STVECTOR *vectorP;

/*
 * Reserva memoria para un vector generico de 'tam1' elementos,
 * cada uno de tamano 'tam_elemento' bytes.
 */
void crear_bytes(vectorP *v1, unsigned long tam1, size_t tam_elemento);

/*
 * Retorna el puntero a la memoria contigua del vector generico.
 * Devuelve NULL si el vector no existe.
 */
void *datos(vectorP v1);

/*
 * Retorna el tamano en bytes de cada elemento del vector generico.
 * Devuelve 0 si el vector no existe.
 */
size_t tam_elemento_vector(vectorP v1);

/*
 * Reserva memoria para un vector de tam1 elementos.
 * Inicializa cada posicion a 0.
 */
void crear(vectorP *v1, unsigned long tam1);

/*
 * Asigna un valor en una posicion concreta.
 * Si v1 es nulo o posicion esta fuera de rango, no hace nada.
 */
void asignar(vectorP v1, unsigned long posicion, TELEMENTO valor);

/*
 * Recupera el valor de una posicion.
 * Devuelve 0 si v1 es nulo o la posicion es invalida.
 */
TELEMENTO recuperar(vectorP v1, unsigned long posicion);

/*
 * Libera la memoria del vector y deja el puntero a NULL.
 */
void liberar(vectorP *v1);

/*
 * Devuelve la cantidad de elementos reservados en el vector.
 * Si no existe, devuelve 0.
 */
unsigned long tamano(vectorP *v1);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector_dinamico.h"

/*
 * Estructura real del vector dinamico.
 * - datos: memoria contigua de elementos.
 * - tam: cantidad total de posiciones disponibles.
 */
struct STVECTOR
{
    void *datos;
    unsigned long tam;
    size_t tam_elemento;
};

void crear_bytes(vectorP *v1, unsigned long tam1, size_t tam_elemento)
{
    if (v1 == NULL)
        return;

    if (tam1 == 0 || tam_elemento == 0)
    {
        *v1 = NULL;
        return;
    }

    if ((size_t)tam1 > ((size_t)-1) / tam_elemento)
    {
        *v1 = NULL;
        return;
    }

    *v1 = (vectorP)malloc(sizeof(struct STVECTOR));
    if (*v1 == NULL)
        return;

    (*v1)->datos = calloc(tam1, tam_elemento);
    if ((*v1)->datos == NULL)
    {
        free(*v1);
        *v1 = NULL;
        return;
    }

    (*v1)->tam = tam1;
    (*v1)->tam_elemento = tam_elemento;
}

void *datos(vectorP v1)
{
    if (v1 == NULL)
        return NULL;

    return v1->datos;
}

size_t tam_elemento_vector(vectorP v1)
{
    if (v1 == NULL)
        return 0;

    return v1->tam_elemento;
}

/*
 * Crea un vector de tam1 posiciones.
 * Flujo:
 * 1) valida parametros
 * 2) reserva cabecera
 * 3) reserva arreglo interno
 * 4) inicializa en cero
 */
void crear(vectorP *v1, unsigned long tam1)
{
    crear_bytes(v1, tam1, sizeof(TELEMENTO));
}

/* Asigna un valor en posicion si el vector y el indice son validos. */
void asignar(vectorP v1, unsigned long posicion, TELEMENTO valor)
{
    TELEMENTO *arr;

    if (v1 == NULL || v1->datos == NULL || posicion >= v1->tam ||
        v1->tam_elemento != sizeof(TELEMENTO))
        return;

    arr = (TELEMENTO *)v1->datos;
    arr[posicion] = valor;
}

/* Libera memoria del vector y anula el puntero externo. */
void liberar(vectorP *v1)
{
    if (v1 == NULL || *v1 == NULL)
        return;

    /* Se puede liberar NULL sin error, por eso no hace falta condicional extra. */
    free((*v1)->datos);
    free(*v1);
    (*v1) = NULL;
}

/* Obtiene valor en posicion; retorna 0 si indice/puntero es invalido. */
TELEMENTO recuperar(vectorP v1, unsigned long posicion)
{
    TELEMENTO *arr;

    if (v1 == NULL || v1->datos == NULL || posicion >= v1->tam ||
        v1->tam_elemento != sizeof(TELEMENTO))
        return 0;

    arr = (TELEMENTO *)v1->datos;
    return arr[posicion];
}

/* Devuelve tamano reservado del vector. */
unsigned long tamano(vectorP *v1)
{
    if (v1 == NULL || *v1 == NULL)
        return 0;

    return (*v1)->tam;
}

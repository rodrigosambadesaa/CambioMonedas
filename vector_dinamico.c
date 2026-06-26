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

/* funcion crear_bytes: contiene la logica principal de esta operacion. */
void crear_bytes(vectorP *v1, unsigned long tam1, size_t tam_elemento)
{
    /* if: comprueba v1 == NULL antes de ejecutar esta rama. */
    if (v1 == NULL)
        return;

    /* if: comprueba tam1 == 0 || tam_elemento == 0 antes de ejecutar esta rama. */
    if (tam1 == 0 || tam_elemento == 0)
    {
        *v1 = NULL;
        return;
    }

    /* if: comprueba (size_t)tam1 > ((size_t)-1) / tam_elemento antes de ejecutar esta rama. */
    if ((size_t)tam1 > ((size_t)-1) / tam_elemento)
    {
        *v1 = NULL;
        return;
    }

    *v1 = (vectorP)malloc(sizeof(struct STVECTOR));
    /* if: comprueba *v1 == NULL antes de ejecutar esta rama. */
    if (*v1 == NULL)
        return;

    (*v1)->datos = calloc(tam1, tam_elemento);
    /* if: comprueba (*v1)->datos == NULL antes de ejecutar esta rama. */
    if ((*v1)->datos == NULL)
    {
        free(*v1);
        *v1 = NULL;
        return;
    }

    (*v1)->tam = tam1;
    (*v1)->tam_elemento = tam_elemento;
}

/* funcion datos: contiene la logica principal de esta operacion. */
void *datos(vectorP v1)
{
    /* if: comprueba v1 == NULL antes de ejecutar esta rama. */
    if (v1 == NULL)
        return NULL;

    return v1->datos;
}

/* funcion tam_elemento_vector: contiene la logica principal de esta operacion. */
size_t tam_elemento_vector(vectorP v1)
{
    /* if: comprueba v1 == NULL antes de ejecutar esta rama. */
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
/* funcion crear: contiene la logica principal de esta operacion. */
void crear(vectorP *v1, unsigned long tam1)
{
    crear_bytes(v1, tam1, sizeof(TELEMENTO));
}

/* Asigna un valor en posicion si el vector y el indice son validos. */
/* funcion asignar: contiene la logica principal de esta operacion. */
void asignar(vectorP v1, unsigned long posicion, TELEMENTO valor)
{
    TELEMENTO *arr;

    /* if: comprueba v1 == NULL || v1->datos == NULL || posicion >= v1->tam || antes de ejecutar esta rama. */
    if (v1 == NULL || v1->datos == NULL || posicion >= v1->tam ||
        v1->tam_elemento != sizeof(TELEMENTO))
        return;

    arr = (TELEMENTO *)v1->datos;
    arr[posicion] = valor;
}

/* Libera memoria del vector y anula el puntero externo. */
/* funcion liberar: contiene la logica principal de esta operacion. */
void liberar(vectorP *v1)
{
    /* if: comprueba v1 == NULL || *v1 == NULL antes de ejecutar esta rama. */
    if (v1 == NULL || *v1 == NULL)
        return;

    /* Se puede liberar NULL sin error, por eso no hace falta condicional extra. */
    free((*v1)->datos);
    free(*v1);
    (*v1) = NULL;
}

/* Obtiene valor en posicion; retorna 0 si indice/puntero es invalido. */
/* funcion recuperar: contiene la logica principal de esta operacion. */
TELEMENTO recuperar(vectorP v1, unsigned long posicion)
{
    TELEMENTO *arr;

    /* if: comprueba v1 == NULL || v1->datos == NULL || posicion >= v1->tam || antes de ejecutar esta rama. */
    if (v1 == NULL || v1->datos == NULL || posicion >= v1->tam ||
        v1->tam_elemento != sizeof(TELEMENTO))
        return 0;

    arr = (TELEMENTO *)v1->datos;
    return arr[posicion];
}

/* Devuelve tamano reservado del vector. */
/* funcion tamano: contiene la logica principal de esta operacion. */
unsigned long tamano(vectorP *v1)
{
    /* if: comprueba v1 == NULL || *v1 == NULL antes de ejecutar esta rama. */
    if (v1 == NULL || *v1 == NULL)
        return 0;

    return (*v1)->tam;
}

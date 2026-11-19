#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vector_dinamico.h"

struct STVECTOR
{
    void *datos;
    unsigned long tam;
    unsigned long capacidad;
    size_t tam_elemento;
};

void crear_bytes(vectorP *v1, unsigned long tam1, size_t tam_elemento)
{
    vectorP nuevo;
    /* El puntero de salida debe inicializarse a NULL. Evitar fugas al
     * intentar crear encima de un vector que todavia esta vivo. */
    if (v1 == NULL || *v1 != NULL)
        return;
    if (tam_elemento == 0 || (uintmax_t)tam1 > SIZE_MAX / tam_elemento)
        return;
    nuevo = (vectorP)calloc(1, sizeof(*nuevo));
    if (nuevo == NULL)
        return;
    nuevo->tam_elemento = tam_elemento;
    if (tam1 > 0)
    {
        nuevo->datos = calloc((size_t)tam1, tam_elemento);
        if (nuevo->datos == NULL)
        {
            free(nuevo);
            return;
        }
    }
    nuevo->tam = tam1;
    nuevo->capacidad = tam1;
    *v1 = nuevo;
}

void crear(vectorP *v1, unsigned long tam1)
{
    crear_bytes(v1, tam1, sizeof(TELEMENTO));
}

void *datos(vectorP v1)
{
    return v1 == NULL ? NULL : v1->datos;
}

size_t tam_elemento_vector(vectorP v1)
{
    return v1 == NULL ? 0 : v1->tam_elemento;
}

unsigned long capacidad_vector(vectorP v1)
{
    return v1 == NULL ? 0 : v1->capacidad;
}

int reservar_bytes(vectorP v1, unsigned long capacidad)
{
    void *nuevo;
    size_t anterior;
    if (v1 == NULL || v1->tam_elemento == 0)
        return 0;
    if (capacidad <= v1->capacidad)
        return 1;
    if ((uintmax_t)capacidad > SIZE_MAX / v1->tam_elemento)
        return 0;
    anterior = (size_t)v1->capacidad * v1->tam_elemento;
    nuevo = realloc(v1->datos, (size_t)capacidad * v1->tam_elemento);
    if (nuevo == NULL)
        return 0;
    memset((unsigned char *)nuevo + anterior, 0,
           ((size_t)capacidad - (size_t)v1->capacidad) * v1->tam_elemento);
    v1->datos = nuevo;
    v1->capacidad = capacidad;
    return 1;
}

int redimensionar_bytes(vectorP v1, unsigned long nuevo_tam)
{
    unsigned long capacidad;
    if (v1 == NULL)
        return 0;
    if (nuevo_tam > v1->capacidad)
    {
        capacidad = v1->capacidad ? v1->capacidad : 1;
        while (capacidad < nuevo_tam)
        {
            if (capacidad > ULONG_MAX / 2)
            {
                capacidad = nuevo_tam;
                break;
            }
            capacidad *= 2;
        }
        if (!reservar_bytes(v1, capacidad))
            return 0;
    }
    if (nuevo_tam != v1->tam)
    {
        unsigned long inicio = nuevo_tam < v1->tam ? nuevo_tam : v1->tam;
        unsigned long fin = nuevo_tam > v1->tam ? nuevo_tam : v1->tam;
        memset((unsigned char *)v1->datos + (size_t)inicio * v1->tam_elemento, 0,
               (size_t)(fin - inicio) * v1->tam_elemento);
    }
    v1->tam = nuevo_tam;
    return 1;
}

int agregar_bytes(vectorP v1, const void *elemento)
{
    void *copia;
    unsigned long anterior;
    if (v1 == NULL || elemento == NULL || v1->tam == ULONG_MAX)
        return 0;
    /* Copia previa: elemento puede apuntar al buffer movido por realloc. */
    copia = malloc(v1->tam_elemento);
    if (copia == NULL)
        return 0;
    memcpy(copia, elemento, v1->tam_elemento);
    anterior = v1->tam;
    if (!redimensionar_bytes(v1, anterior + 1))
    {
        free(copia);
        return 0;
    }
    memcpy((unsigned char *)v1->datos + (size_t)anterior * v1->tam_elemento,
           copia, v1->tam_elemento);
    free(copia);
    return 1;
}

int reservar(vectorP v1, unsigned long capacidad)
{
    return v1 != NULL && v1->tam_elemento == sizeof(TELEMENTO)
               ? reservar_bytes(v1, capacidad) : 0;
}

int redimensionar(vectorP v1, unsigned long nuevo_tam)
{
    return v1 != NULL && v1->tam_elemento == sizeof(TELEMENTO)
               ? redimensionar_bytes(v1, nuevo_tam) : 0;
}

int agregar(vectorP v1, TELEMENTO valor)
{
    return v1 != NULL && v1->tam_elemento == sizeof(TELEMENTO)
               ? agregar_bytes(v1, &valor) : 0;
}

void asignar(vectorP v1, unsigned long posicion, TELEMENTO valor)
{
    if (v1 == NULL || posicion >= v1->tam ||
        v1->tam_elemento != sizeof(TELEMENTO))
        return;
    ((TELEMENTO *)v1->datos)[posicion] = valor;
}

TELEMENTO recuperar(vectorP v1, unsigned long posicion)
{
    if (v1 == NULL || posicion >= v1->tam ||
        v1->tam_elemento != sizeof(TELEMENTO))
        return 0;
    return ((TELEMENTO *)v1->datos)[posicion];
}

void liberar(vectorP *v1)
{
    if (v1 == NULL || *v1 == NULL)
        return;
    free((*v1)->datos);
    free(*v1);
    *v1 = NULL;
}

unsigned long tamano(vectorP *v1)
{
    return (v1 == NULL || *v1 == NULL) ? 0 : (*v1)->tam;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef int TELEMENTO;

typedef struct {
TELEMENTO *datos;
unsigned long tam;
}STVECTOR;
typedef STVECTOR *vectorP;


void crear(vectorP *v1,unsigned long tam1)
{
unsigned long i=0;
*v1=(vectorP)malloc(sizeof(STVECTOR));
(*v1)->datos=(TELEMENTO*)malloc(tam1*sizeof(TELEMENTO));
(*v1)->tam=tam1;
for(i=0; i < tam1; i++)
*((*v1)->datos+i) = 0;
}

void asignar(vectorP v1,unsigned long posicion, TELEMENTO valor)
{
*(v1->datos+posicion)=valor;
}



void liberar (vectorP *v1)
{
    if(*v1==NULL)
        printf("Vector inexistente...\n");
    else
    {
        free((*v1)->datos);
        free(*v1); 
        (*v1)=NULL;
        
    }
}

TELEMENTO recuperar (vectorP v1, unsigned long posicion)
{
    TELEMENTO dato;
    if(v1==NULL || posicion>v1->tam)
        return 0;
    else
    {
        dato=*(v1->datos+posicion);
        return dato;
    }
}

unsigned long tamano (vectorP *v1)
{
    unsigned long tamano=0;
    if(*v1==NULL)
    {
        printf("El vector no existe...\n");
        return -1;
    }
    else
        tamano= (*v1)->tam;
         return tamano;
    
}


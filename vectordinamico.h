

typedef int TELEMENTO;

typedef void * vectorP;

void crear(vectorP *v1,unsigned long tam1);

void asignar(vectorP v1,unsigned long posicion, TELEMENTO valor);

TELEMENTO recuperar (vectorP v1, unsigned long posicion);

void liberar (vectorP *v1);

unsigned long tamano (vectorP *v1);

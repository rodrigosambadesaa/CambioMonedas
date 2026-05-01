/*
 * File:   main.c
 * Author: rodrigo.sambade
 *
 * Created on 28 de abril de 2014, 17:58
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vectordinamico.h"
/*
 *
 */
int cambio(int x, vectorP valor, vectorP *solucion)
{
    int i = 0, suma = 0;
    TELEMENTO aux;
    crear(solucion, tamano(&valor));
    while (suma < x && i < tamano(&valor))
    {
        aux = recuperar(valor, i);
        if (suma + aux <= x)
        {
            asignar(*solucion, i, recuperar(*solucion, i) + 1);
            suma = suma + recuperar(valor, i);
        }
        else
            i++;
    }
    if (suma == x)
        return 1;
    else
    {
        for (i = 0; i < tamano(&valor); i++)
        {
            asignar(*solucion, i, 0);
        }
        return 0;
    }
}

int cambio_stock(int x, vectorP valor, vectorP *solucion, vectorP stock)
{
    int i = 0, suma = 0;
    TELEMENTO aux, aux2;
    crear(solucion, tamano(&valor));
    while (suma < x && i < tamano(&valor))
    {
        aux = recuperar(valor, i);
        aux2 = recuperar(stock, i);
        if (suma + aux <= x && aux2 >= 1)
        {
            asignar(*solucion, i, recuperar(*solucion, i) + 1);
            suma = suma + recuperar(valor, i);
            asignar(stock, i, aux2 - 1);
        }
        else
            i++;
    }
    if (suma == x)
        return 1;
    else
    {
        for (i = 0; i < tamano(&valor); i++)
        {
            asignar(*solucion, i, 0);
        }
        return 0;
    }
}

vectorP leer(char n[20])
{
    FILE *FP;
    TELEMENTO vector[20], elem;
    char moneda[10];
    int i = 0, j = 0;
    vectorP res;
    FP = fopen("monedas.txt", "r");
    if (FP == NULL)
    {
        printf("Error en la apertura del archivo");
        exit(0);
    }

    do
    {
        fscanf(FP, "%s", moneda);
        if ((strcmp(moneda, n)) == 0)
        {
            do
            {
                fscanf(FP, "%s", moneda);
                elem = atoi(moneda);
                if (elem == 0)
                {
                    break;
                }

                vector[i] = elem;
                i++;
            } while (feof(FP) == 0);
        }
    } while (feof(FP) == 0);
    crear(&res, (unsigned long)i);
    for (j = 0; j <= i; j++)
    {
        asignar(res, j, vector[j]);
    }

    return res;
}

vectorP leer_stock(char n[20])
{
    FILE *archivo;
    TELEMENTO vector[20], elem;
    char moneda[10];
    int i = 0, j = 0;
    vectorP res;
    archivo = fopen("stock.txt", "r");
    if (archivo == NULL)
    {
        printf("Error en la apertura del archivo");
        exit(0);
    }
    /*Si el stock de la moneda coincide con -1 significa que no
     hay stock*/
    do
    {
        fscanf(archivo, "%s", moneda);
        if ((strcmp(moneda, n)) == 0)
        {
            do
            {
                fscanf(archivo, "%s", moneda);
                elem = atoi(moneda);
                if (elem == 0)
                {
                    break;
                }
                else if (elem == -1)
                {
                    elem = 0;
                }
                vector[i] = elem;
                i++;
            } while (feof(archivo) == 0);
        }
    } while (feof(archivo) == 0);
    crear(&res, (unsigned long)i);
    for (j = 0; j <= i; j++)
    {
        asignar(res, j, vector[j]);
    }

    return res;
}

int guardar_stock(char n[20], vectorP stock)
{
    FILE *entrada, *salida;
    char token[64];
    int encontrado = 0;
    int i;

    entrada = fopen("stock.txt", "r");
    if (entrada == NULL)
    {
        return 0;
    }

    salida = fopen("stock.tmp", "w");
    if (salida == NULL)
    {
        fclose(entrada);
        return 0;
    }

    while (fscanf(entrada, "%63s", token) == 1)
    {
        if (strcmp(token, n) == 0)
        {
            encontrado = 1;
            fprintf(salida, "%s\n", token);

            for (i = 0; i < tamano(&stock); i++)
            {
                int actual = recuperar(stock, i);
                if (actual <= 0)
                {
                    fprintf(salida, "-1\n");
                }
                else
                {
                    fprintf(salida, "%d\n", actual);
                }
            }

            while (fscanf(entrada, "%63s", token) == 1)
            {
                if (atoi(token) == 0)
                {
                    fprintf(salida, "0\n");
                    break;
                }
            }
        }
        else
        {
            fprintf(salida, "%s\n", token);
        }
    }

    fclose(entrada);
    fclose(salida);

    if (!encontrado)
    {
        remove("stock.tmp");
        return 0;
    }

    if (remove("stock.txt") != 0)
    {
        remove("stock.tmp");
        return 0;
    }

    if (rename("stock.tmp", "stock.txt") != 0)
    {
        return 0;
    }

    return 1;
}

int main(int argc, char **argv)
{
    char opcion, moneda[20];
    int i;
    int cantidad, resultado;

    vectorP monedas, *solucion, stock;

    printf("a) Monedas infinitas.");
    printf("\nb) Monedas limitadas.\n");
    printf("\nHaga su eleccion: ");
    scanf("%c", &opcion);

    printf("\nNombre de la moneda: ");
    /*Aquí el usuario teclea el nombre de la moneda. El nombre tiene
    que ser igual a uno de los nombres del fichero donde se guardan las monedas.*/
    scanf("%s", moneda);

    monedas = leer(moneda);
    if (tamano(&monedas) < 1)
    {
        printf("Moneda no existente.");
        exit(0);
    }
    printf("\n");
    switch (opcion)
    {
    case 'a':
        do
        {
            printf("Introduzca la cantidad a devolver en centimos (si quiere salir del programa teclee 0): ");
            scanf("%d", &cantidad);
            if (cantidad == 0)
            {
                printf("Gracias por utilizar este programa.");
                exit(0);
            }
            else
            {
                resultado = cambio(cantidad, monedas, solucion);
                if (resultado == 1)
                {
                    for (i = 0; i < tamano(&monedas); i++)
                    {
                        printf("Moneda: %d cent\t Cantidad: %d\n", recuperar(monedas, i), recuperar(*solucion, i));
                    }
                }
                else
                {
                    printf("No existe cambio para esta cantidad\n");
                }
            }
        } while (cantidad > 0);
        break;
    case 'b':
        stock = leer_stock(moneda);
        do
        {
            printf("Introduzca la cantidad a devolver en centimos (si quiere salir del programa teclee 0): ");
            scanf("%d", &cantidad);
            if (cantidad == 0)
            {
                printf("Gracias por utilizar este programa.");
                exit(0);
            }
            else
            {
                resultado = cambio_stock(cantidad, monedas, solucion, stock);
                if (resultado == 1)
                {
                    if (!guardar_stock(moneda, stock))
                    {
                        printf("No se pudo actualizar el archivo de stock.\n");
                    }
                    for (i = 0; i < tamano(&monedas); i++)
                    {
                        printf("Moneda: %d cent\t Cantidad: %d\t Stock: %d\n", recuperar(monedas, i), recuperar(*solucion, i), recuperar(stock, i));
                    }
                }
                else
                {
                    printf("No existe cambio para esta cantidad\n");
                }
            }
        } while (cantidad > 0);

        break;

        ;
    }

    return (EXIT_SUCCESS);
}

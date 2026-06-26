#include "json_io.h"
#include "moneda_gestion.h"
#include "bigint.h"
#include <stdio.h>
#include <string.h>

/* funcion export_stock_json: contiene la logica principal de esta operacion. */
int export_stock_json(const char *rutaJson)
{
    const char *ruta = rutaJson && rutaJson[0] ? rutaJson : "stock.json";
    FILE *fp = fopen(ruta, "w");
    /* if: comprueba !fp antes de ejecutar esta rama. */
    if (!fp)
        return 0;

    fprintf(fp, "{\n  \"monedas\": [\n");

    char claves[128][256];
    size_t nmon = 0;
    /* Leer monedas.txt extrayendo tokens no numericos y unicos. */
    {
        FILE *fm = fopen("monedas.txt", "r");
        /* if: comprueba !fm antes de ejecutar esta rama. */
        if (!fm)
        {
            fclose(fp);
            return 0;
        }
        char token[256];
        /* while: repite el bloque mientras se cumpla fscanf(fm, "%255s", token) == 1. */
        while (fscanf(fm, "%255s", token) == 1)
        {
            int es_num = 1;
            /* if: comprueba strcmp(token, "-1") == 0 antes de ejecutar esta rama. */
            if (strcmp(token, "-1") == 0)
                es_num = 1;
            else
            {
                /* for: itera segun size_t k = 0; token[k] != '\0'; k++ para recorrer el bloque. */
                for (size_t k = 0; token[k] != '\0'; k++)
                {
                    /* if: comprueba token[k] < '0' || token[k] > '9' antes de ejecutar esta rama. */
                    if (token[k] < '0' || token[k] > '9')
                    {
                        es_num = 0;
                        break;
                    }
                }
            }
            /* if: comprueba es_num antes de ejecutar esta rama. */
            if (es_num)
                continue;
            int repetida = 0;
            /* for: itera segun size_t x = 0; x < nmon; x++ para recorrer el bloque. */
            for (size_t x = 0; x < nmon; x++)
            {
                /* if: comprueba strcmp(claves[x], token) == 0 antes de ejecutar esta rama. */
                if (strcmp(claves[x], token) == 0)
                {
                    repetida = 1;
                    break;
                }
            }
            /* if: comprueba repetida antes de ejecutar esta rama. */
            if (repetida)
                continue;
            /* if: comprueba nmon >= 128 antes de ejecutar esta rama. */
            if (nmon >= 128)
                break;
            strncpy(claves[nmon], token, sizeof(claves[nmon]) - 1);
            claves[nmon][sizeof(claves[nmon]) - 1] = '\0';
            nmon++;
        }
        fclose(fm);
    }

    /* for: itera segun size_t i = 0; i < nmon; i++ para recorrer el bloque. */
    for (size_t i = 0; i < nmon; i++)
    {
        BigIntArray denom = {0};
        BigIntArray stock = {0};
        /* if: comprueba !cargar_denominaciones_moneda(claves[i], &denom) || !cargar_stock_mon... antes de ejecutar esta rama. */
        if (!cargar_denominaciones_moneda(claves[i], &denom) || !cargar_stock_moneda(claves[i], &stock))
        {
            bigint_array_free(&denom);
            bigint_array_free(&stock);
            continue;
        }

        fprintf(fp, "    {\"nombre\": \"%s\", \"denominaciones\": [", claves[i]);
        /* for: itera segun size_t j = 0; j < denom.len; j++ para recorrer el bloque. */
        for (size_t j = 0; j < denom.len; j++)
        {
            fprintf(fp, "%s\"%s\"", j ? ", \"" : "\"", denom.items[j].digits);
            /* if: comprueba j + 1 < denom.len antes de ejecutar esta rama. */
            if (j + 1 < denom.len)
                fprintf(fp, ", ");
        }
        fprintf(fp, "], \"stock\": [");
        /* for: itera segun size_t j = 0; j < stock.len; j++ para recorrer el bloque. */
        for (size_t j = 0; j < stock.len; j++)
        {
            fprintf(fp, "%s%s", stock.items[j].digits, j + 1 < stock.len ? ", " : "");
        }
        fprintf(fp, "]}%s\n", i + 1 < nmon ? "," : "");

        bigint_array_free(&denom);
        bigint_array_free(&stock);
    }

    fprintf(fp, "  ]\n}\n");
    fclose(fp);
    return 1;
}

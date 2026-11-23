#include "algoritmo_cambio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DP_MONTO_MAX ((size_t)50000)
/* With <=8 types the bounded-stock DP choice table remains <=12.8 MiB. */
#define DP_MONTO_MAX_SMALL_ALPHABET ((size_t)200000)
#define DP_NO_APLICA (-1)
#define DP_SIN_SOLUCION 0
#define DP_OK 1

/* funcion bigint_a_size_limitado: contiene la logica principal de esta operacion. */
static int bigint_a_size_limitado(const BigInt *n, size_t limite, size_t *valor_out)
{
    size_t valor = 0;
    const char *p;

    /* if: comprueba n == NULL || n->digits == NULL || valor_out == NULL antes de ejecutar esta rama. */
    if (n == NULL || n->digits == NULL || valor_out == NULL)
        return 0;

    /* for: itera segun p = n->digits; *p != '\0'; p++ para recorrer el bloque. */
    for (p = n->digits; *p != '\0'; p++)
    {
        unsigned int digito;

        /* if: comprueba *p < '0' || *p > '9' antes de ejecutar esta rama. */
        if (*p < '0' || *p > '9')
            return 0;

        digito = (unsigned int)(*p - '0');
        /* if: comprueba valor > limite / 10 || (valor == limite / 10 && digito > limite % 10) antes de ejecutar esta rama. */
        if (valor > limite / 10 || (valor == limite / 10 && digito > limite % 10))
            return 0;

        valor = valor * 10 + digito;
    }

    *valor_out = valor;
    return 1;
}

/* funcion bigint_a_size_con_tope: contiene la logica principal de esta operacion. */
static int bigint_a_size_con_tope(const BigInt *n, size_t tope, size_t *valor_out)
{
    size_t valor = 0;
    const char *p;

    /* if: comprueba n == NULL || n->digits == NULL || valor_out == NULL antes de ejecutar esta rama. */
    if (n == NULL || n->digits == NULL || valor_out == NULL)
        return 0;

    /* for: itera segun p = n->digits; *p != '\0'; p++ para recorrer el bloque. */
    for (p = n->digits; *p != '\0'; p++)
    {
        unsigned int digito;

        /* if: comprueba *p < '0' || *p > '9' antes de ejecutar esta rama. */
        if (*p < '0' || *p > '9')
            return 0;

        digito = (unsigned int)(*p - '0');
        /* if: comprueba valor > tope / 10 || (valor == tope / 10 && digito > tope % 10) antes de ejecutar esta rama. */
        if (valor > tope / 10 || (valor == tope / 10 && digito > tope % 10))
        {
            *valor_out = tope;
            return 1;
        }

        valor = valor * 10 + digito;
    }

    *valor_out = valor;
    return 1;
}

/* funcion size_a_bigint: contiene la logica principal de esta operacion. */
static int size_a_bigint(size_t valor, BigInt *out)
{
    char texto[32];

    /* if: comprueba out == NULL antes de ejecutar esta rama. */
    if (out == NULL)
        return 0;

    snprintf(texto, sizeof(texto), "%zu", valor);
    return bigint_init(out, texto);
}

/* funcion cargar_solucion_size: contiene la logica principal de esta operacion. */
static int cargar_solucion_size(const size_t *cantidades, size_t len, BigIntArray *solucion)
{
    size_t i;

    /* if: comprueba cantidades == NULL || solucion == NULL || len == 0 antes de ejecutar esta rama. */
    if (cantidades == NULL || solucion == NULL || len == 0)
        return 0;

    /* if: comprueba !bigint_array_create(solucion, len) antes de ejecutar esta rama. */
    if (!bigint_array_create(solucion, len))
        return 0;

    /* for: itera segun i = 0; i < len; i++ para recorrer el bloque. */
    for (i = 0; i < len; i++)
    {
        BigInt tmp = {0};

        /* if: comprueba !size_a_bigint(cantidades[i], &tmp) || antes de ejecutar esta rama. */
        if (!size_a_bigint(cantidades[i], &tmp) ||
            !bigint_array_set(solucion, i, &tmp))
        {
            bigint_free(&tmp);
            bigint_array_free(solucion);
            return 0;
        }

        bigint_free(&tmp);
    }

    return 1;
}

/* funcion calcular_dp_ilimitado: contiene la logica principal de esta operacion. */
static int calcular_dp_ilimitado(const BigInt *monto, const BigIntArray *denom, BigIntArray *solucion)
{
    const size_t inf = ((size_t)-1) / 4;
    size_t amount;
    size_t *valores = NULL;
    size_t *dp = NULL;
    size_t *previo = NULL;
    size_t *moneda = NULL;
    size_t *cantidades = NULL;
    size_t i;
    int resultado = DP_NO_APLICA;

    /* if: comprueba !bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount) antes de ejecutar esta rama. */
    const size_t max_dp_amount =
        (denom != NULL && denom->len <= 8)
            ? DP_MONTO_MAX_SMALL_ALPHABET : DP_MONTO_MAX;
    if (!bigint_a_size_limitado(monto, max_dp_amount, &amount))
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    dp = (size_t *)malloc((amount + 1) * sizeof(size_t));
    previo = (size_t *)malloc((amount + 1) * sizeof(size_t));
    moneda = (size_t *)malloc((amount + 1) * sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));

    /* if: comprueba valores == NULL || dp == NULL || previo == NULL || moneda == NULL ||... antes de ejecutar esta rama. */
    if (valores == NULL || dp == NULL || previo == NULL || moneda == NULL || cantidades == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        /* if: comprueba !bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) antes de ejecutar esta rama. */
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]))
            goto cleanup;
    }

    /* for: itera segun i = 0; i <= amount; i++ para recorrer el bloque. */
    for (i = 0; i <= amount; i++)
    {
        dp[i] = inf;
        previo[i] = 0;
        moneda[i] = denom->len;
    }

    dp[0] = 0;

    /* for: itera segun i = 1; i <= amount; i++ para recorrer el bloque. */
    for (i = 1; i <= amount; i++)
    {
        size_t j;

        /* for: itera segun j = 0; j < denom->len; j++ para recorrer el bloque. */
        for (j = 0; j < denom->len; j++)
        {
            size_t valor = valores[j];

            /* if: comprueba valor == 0 || valor > i || dp[i - valor] == inf antes de ejecutar esta rama. */
            if (valor == 0 || valor > i || dp[i - valor] == inf)
                continue;

            /* if: comprueba dp[i - valor] + 1 < dp[i] antes de ejecutar esta rama. */
            if (dp[i - valor] + 1 < dp[i])
            {
                dp[i] = dp[i - valor] + 1;
                previo[i] = i - valor;
                moneda[i] = j;
            }
        }
    }

    /* if: comprueba dp[amount] == inf antes de ejecutar esta rama. */
    if (dp[amount] == inf)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    /* for: itera segun i = amount; i > 0; para recorrer el bloque. */
    for (i = amount; i > 0;)
    {
        size_t idx = moneda[i];

        /* if: comprueba idx >= denom->len || previo[i] >= i antes de ejecutar esta rama. */
        if (idx >= denom->len || previo[i] >= i)
            goto cleanup;

        cantidades[idx]++;
        i = previo[i];
    }

    resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;

cleanup:
    free(valores);
    free(dp);
    free(previo);
    free(moneda);
    free(cantidades);
    return resultado;
}

/* funcion calcular_dp_stock: contiene la logica principal de esta operacion. */
static int calcular_dp_stock(const BigInt *monto,
                             const BigIntArray *denom,
                             const BigIntArray *stock,
                             BigIntArray *solucion)
{
    const size_t inf = ((size_t)-1) / 4;
    size_t amount;
    size_t estados;
    size_t *valores = NULL;
    size_t *topes = NULL;
    size_t *prev_dp = NULL;
    size_t *curr_dp = NULL;
    size_t *choice = NULL;
    size_t *cantidades = NULL;
    size_t *cola_pos = NULL;
    long long *cola_score = NULL;
    size_t i;
    int resultado = DP_NO_APLICA;

    /* if: comprueba !bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount) antes de ejecutar esta rama. */
    const size_t max_dp_amount =
        (denom != NULL && denom->len <= 8)
            ? DP_MONTO_MAX_SMALL_ALPHABET : DP_MONTO_MAX;
    if (!bigint_a_size_limitado(monto, max_dp_amount, &amount))
        return DP_NO_APLICA;

    /* if: comprueba denom->len > ((size_t)-1) / (amount + 1) antes de ejecutar esta rama. */
    if (denom->len > ((size_t)-1) / (amount + 1))
        return DP_NO_APLICA;

    estados = denom->len * (amount + 1);
    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    topes = (size_t *)calloc(denom->len, sizeof(size_t));
    prev_dp = (size_t *)malloc((amount + 1) * sizeof(size_t));
    curr_dp = (size_t *)malloc((amount + 1) * sizeof(size_t));
    choice = (size_t *)calloc(estados, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    cola_pos = (size_t *)malloc((amount + 1) * sizeof(size_t));
    cola_score = (long long *)malloc((amount + 1) * sizeof(long long));

    /* if: comprueba valores == NULL || topes == NULL || prev_dp == NULL || curr_dp == NUL... antes de ejecutar esta rama. */
    if (valores == NULL || topes == NULL || prev_dp == NULL || curr_dp == NULL ||
        choice == NULL || cantidades == NULL || cola_pos == NULL || cola_score == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        /* if: comprueba !bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) || antes de ejecutar esta rama. */
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) ||
            !bigint_a_size_con_tope(&stock->items[i], amount, &topes[i]))
        {
            goto cleanup;
        }
    }

    /* for: itera segun i = 0; i <= amount; i++ para recorrer el bloque. */
    for (i = 0; i <= amount; i++)
        prev_dp[i] = inf;
    prev_dp[0] = 0;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        size_t valor = valores[i];
        size_t max_usos = 0;
        size_t v;

        /* for: itera segun v = 0; v <= amount; v++ para recorrer el bloque. */
        for (v = 0; v <= amount; v++)
            curr_dp[v] = inf;

        /* if: comprueba valor > 0 && valor <= amount antes de ejecutar esta rama. */
        if (valor > 0 && valor <= amount)
        {
            max_usos = amount / valor;
            /* if: comprueba topes[i] < max_usos antes de ejecutar esta rama. */
            if (topes[i] < max_usos)
                max_usos = topes[i];
        }

        /* if: comprueba max_usos == 0 antes de ejecutar esta rama. */
        if (max_usos == 0)
        {
            /* for: itera segun v = 0; v <= amount; v++ para recorrer el bloque. */
            for (v = 0; v <= amount; v++)
                curr_dp[v] = prev_dp[v];
        }
        else
        {
            size_t residuo;

            /* Cola monotona por residuo: evita iterar una vez por cada unidad de stock. */
            /* for: itera segun residuo = 0; residuo < valor && residuo <= amount; residuo++ para recorrer el bloque. */
            for (residuo = 0; residuo < valor && residuo <= amount; residuo++)
            {
                size_t cabeza = 0;
                size_t cola = 0;
                size_t m = 0;

                /* for: itera segun v = residuo; v <= amount; v += valor, m++ para recorrer el bloque. */
                for (v = residuo; v <= amount; v += valor, m++)
                {
                    /* if: comprueba prev_dp[v] != inf antes de ejecutar esta rama. */
                    if (prev_dp[v] != inf)
                    {
                        long long score = (long long)prev_dp[v] - (long long)m;

                        /* while: repite el bloque mientras se cumpla cola > cabeza && cola_score[cola - 1] >= score. */
                        while (cola > cabeza && cola_score[cola - 1] >= score)
                            cola--;

                        cola_pos[cola] = m;
                        cola_score[cola] = score;
                        cola++;
                    }

                    /* while: repite el bloque mientras se cumpla cola > cabeza && cola_pos[cabeza] + max_usos < m. */
                    while (cola > cabeza && cola_pos[cabeza] + max_usos < m)
                        cabeza++;

                    /* if: comprueba cola > cabeza antes de ejecutar esta rama. */
                    if (cola > cabeza)
                    {
                        size_t usos = m - cola_pos[cabeza];
                        size_t candidato = (size_t)(cola_score[cabeza] + (long long)m);

                        curr_dp[v] = candidato;
                        choice[i * (amount + 1) + v] = usos;
                    }
                }
            }
        }

        {
            size_t *tmp = prev_dp;
            prev_dp = curr_dp;
            curr_dp = tmp;
        }
    }

    /* if: comprueba prev_dp[amount] == inf antes de ejecutar esta rama. */
    if (prev_dp[amount] == inf)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    /*
     * The DP choice table has a fixed row stride of original_amount + 1.
     * Reconstruction changes the remaining amount, not the table stride.
     */
    {
        const size_t stride = amount + 1;
        size_t remaining = amount;

        for (i = denom->len; i > 0; i--)
        {
            size_t idx = i - 1;
            size_t uses = choice[idx * stride + remaining];

            /* Reject corrupt predecessors rather than underflowing remaining. */
            if ((valores[idx] == 0 && uses != 0) ||
                (valores[idx] != 0 && uses > remaining / valores[idx]))
                goto cleanup;

            cantidades[idx] = uses;
            remaining -= uses * valores[idx];
        }

        if (remaining != 0)
            goto cleanup;
    }

    resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;

cleanup:
    free(valores);
    free(topes);
    free(prev_dp);
    free(curr_dp);
    free(choice);
    free(cantidades);
    free(cola_pos);
    free(cola_score);
    return resultado;
}

/* funcion calcular_dp_ilimitado_rango: contiene la logica principal de esta operacion. */
static int calcular_dp_ilimitado_rango(const BigInt *monto,
                                       const BigIntArray *denom,
                                       size_t min_monedas,
                                       size_t max_monedas,
                                       BigIntArray *solucion)
{
    size_t amount;
    size_t stride;
    size_t estados;
    size_t *valores = NULL;
    size_t *cantidades = NULL;
    unsigned char *alcanzable = NULL;
    size_t *prev_monto = NULL;
    size_t *moneda = NULL;
    size_t min_valor = (size_t)-1;
    size_t i;
    size_t k;
    size_t objetivo_k = (size_t)-1;
    int resultado = DP_NO_APLICA;

    /* if: comprueba !bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount) antes de ejecutar esta rama. */
    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    /* if: comprueba valores == NULL || cantidades == NULL antes de ejecutar esta rama. */
    if (valores == NULL || cantidades == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        /* if: comprueba !bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) antes de ejecutar esta rama. */
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]))
            goto cleanup;

        /* if: comprueba valores[i] > 0 && valores[i] < min_valor antes de ejecutar esta rama. */
        if (valores[i] > 0 && valores[i] < min_valor)
            min_valor = valores[i];
    }

    /* if: comprueba amount == 0 antes de ejecutar esta rama. */
    if (amount == 0)
    {
        /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
        if (min_monedas == 0)
        {
            resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;
        }
        else
        {
            resultado = DP_SIN_SOLUCION;
        }
        goto cleanup;
    }

    /* if: comprueba min_valor == (size_t)-1 antes de ejecutar esta rama. */
    if (min_valor == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t tope_natural = amount / min_valor;
        /* if: comprueba max_monedas > tope_natural antes de ejecutar esta rama. */
        if (max_monedas > tope_natural)
            max_monedas = tope_natural;
    }

    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    /* if: comprueba max_monedas > ((size_t)-1) / (amount + 1) - 1 antes de ejecutar esta rama. */
    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    moneda = (size_t *)malloc(estados * sizeof(size_t));
    /* if: comprueba alcanzable == NULL || prev_monto == NULL || moneda == NULL antes de ejecutar esta rama. */
    if (alcanzable == NULL || prev_monto == NULL || moneda == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < estados; i++ para recorrer el bloque. */
    for (i = 0; i < estados; i++)
    {
        prev_monto[i] = 0;
        moneda[i] = denom->len;
    }

    alcanzable[0] = 1;
    /* for: itera segun k = 1; k <= max_monedas; k++ para recorrer el bloque. */
    for (k = 1; k <= max_monedas; k++)
    {
        size_t monto_k;

        /* for: itera segun monto_k = 0; monto_k <= amount; monto_k++ para recorrer el bloque. */
        for (monto_k = 0; monto_k <= amount; monto_k++)
        {
            size_t j;
            size_t idx = k * stride + monto_k;

            /* for: itera segun j = 0; j < denom->len; j++ para recorrer el bloque. */
            for (j = 0; j < denom->len; j++)
            {
                size_t valor = valores[j];
                size_t idx_prev;

                /* if: comprueba valor == 0 || valor > monto_k antes de ejecutar esta rama. */
                if (valor == 0 || valor > monto_k)
                    continue;

                idx_prev = (k - 1) * stride + (monto_k - valor);
                /* if: comprueba !alcanzable[idx_prev] antes de ejecutar esta rama. */
                if (!alcanzable[idx_prev])
                    continue;

                alcanzable[idx] = 1;
                prev_monto[idx] = monto_k - valor;
                moneda[idx] = j;
                break;
            }
        }
    }

    /* for: itera segun k = min_monedas; k <= max_monedas; k++ para recorrer el bloque. */
    for (k = min_monedas; k <= max_monedas; k++)
    {
        /* if: comprueba alcanzable[k * stride + amount] antes de ejecutar esta rama. */
        if (alcanzable[k * stride + amount])
        {
            objetivo_k = k;
            break;
        }
    }

    /* if: comprueba objetivo_k == (size_t)-1 antes de ejecutar esta rama. */
    if (objetivo_k == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t monto_actual = amount;
        size_t k_actual = objetivo_k;

        /* while: repite el bloque mientras se cumpla k_actual > 0. */
        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t j = moneda[idx];

            /* if: comprueba j >= denom->len || prev_monto[idx] > monto_actual antes de ejecutar esta rama. */
            if (j >= denom->len || prev_monto[idx] > monto_actual)
                goto cleanup;

            cantidades[j]++;
            monto_actual = prev_monto[idx];
            k_actual--;
        }

        /* if: comprueba monto_actual != 0 antes de ejecutar esta rama. */
        if (monto_actual != 0)
            goto cleanup;
    }

    resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;

cleanup:
    free(valores);
    free(cantidades);
    free(alcanzable);
    free(prev_monto);
    free(moneda);
    return resultado;
}

typedef struct
{
    size_t idx_denom;
    size_t monedas;
    size_t valor;
} DPItemStock;

/* funcion push_item_stock: contiene la logica principal de esta operacion. */
static int push_item_stock(DPItemStock **items, size_t *len, size_t *cap, size_t idx_denom, size_t monedas, size_t valor)
{
    DPItemStock *nuevo;

    /* if: comprueba items == NULL || len == NULL || cap == NULL antes de ejecutar esta rama. */
    if (items == NULL || len == NULL || cap == NULL)
        return 0;

    /* if: comprueba *len >= *cap antes de ejecutar esta rama. */
    if (*len >= *cap)
    {
        size_t nueva_cap = (*cap == 0) ? 16 : (*cap * 2);
        /* if: comprueba nueva_cap < *cap antes de ejecutar esta rama. */
        if (nueva_cap < *cap)
            return 0;
        nuevo = (DPItemStock *)realloc(*items, nueva_cap * sizeof(DPItemStock));
        /* if: comprueba nuevo == NULL antes de ejecutar esta rama. */
        if (nuevo == NULL)
            return 0;
        *items = nuevo;
        *cap = nueva_cap;
    }

    (*items)[*len].idx_denom = idx_denom;
    (*items)[*len].monedas = monedas;
    (*items)[*len].valor = valor;
    (*len)++;
    return 1;
}

/* funcion calcular_dp_stock_rango: contiene la logica principal de esta operacion. */
static int calcular_dp_stock_rango(const BigInt *monto,
                                   const BigIntArray *denom,
                                   const BigIntArray *stock,
                                   size_t min_monedas,
                                   size_t max_monedas,
                                   BigIntArray *solucion)
{
    size_t amount;
    size_t stride;
    size_t estados;
    size_t *valores = NULL;
    size_t *topes = NULL;
    size_t *cantidades = NULL;
    unsigned char *alcanzable = NULL;
    size_t *prev_k = NULL;
    size_t *prev_monto = NULL;
    size_t *item_usado = NULL;
    DPItemStock *items = NULL;
    size_t items_len = 0;
    size_t items_cap = 0;
    size_t i;
    size_t objetivo_k = (size_t)-1;
    size_t tope_monedas = 0;
    const size_t invalid = (size_t)-1;
    int resultado = DP_NO_APLICA;

    /* if: comprueba !bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount) antes de ejecutar esta rama. */
    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    topes = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    /* if: comprueba valores == NULL || topes == NULL || cantidades == NULL antes de ejecutar esta rama. */
    if (valores == NULL || topes == NULL || cantidades == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;

        /* if: comprueba !bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) || antes de ejecutar esta rama. */
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) ||
            !bigint_a_size_con_tope(&stock->items[i], amount, &topes[i]))
        {
            goto cleanup;
        }

        /* if: comprueba valores[i] == 0 antes de ejecutar esta rama. */
        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        /* if: comprueba valores[i] > 0 antes de ejecutar esta rama. */
        if (valores[i] > 0)
        {
            size_t cap_por_monto = amount / valores[i];
            /* if: comprueba cap_por_monto < cap_uso antes de ejecutar esta rama. */
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }

        tope_monedas += cap_uso;
    }

    /* if: comprueba amount == 0 antes de ejecutar esta rama. */
    if (amount == 0)
    {
        /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
        if (min_monedas == 0)
        {
            resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;
        }
        else
        {
            resultado = DP_SIN_SOLUCION;
        }
        goto cleanup;
    }

    /* if: comprueba max_monedas > tope_monedas antes de ejecutar esta rama. */
    if (max_monedas > tope_monedas)
        max_monedas = tope_monedas;

    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;
        size_t chunk = 1;

        /* if: comprueba valores[i] == 0 antes de ejecutar esta rama. */
        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        {
            size_t cap_por_monto = amount / valores[i];
            /* if: comprueba cap_por_monto < cap_uso antes de ejecutar esta rama. */
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }
        /* if: comprueba cap_uso > max_monedas antes de ejecutar esta rama. */
        if (cap_uso > max_monedas)
            cap_uso = max_monedas;

        /* while: repite el bloque mientras se cumpla cap_uso > 0. */
        while (cap_uso > 0)
        {
            size_t take = (chunk < cap_uso) ? chunk : cap_uso;
            size_t valor_item = take * valores[i];

            /* if: comprueba !push_item_stock(&items, &items_len, &items_cap, i, take, valor_item) antes de ejecutar esta rama. */
            if (!push_item_stock(&items, &items_len, &items_cap, i, take, valor_item))
                goto cleanup;

            cap_uso -= take;
            /* if: comprueba chunk <= ((size_t)-1) / 2 antes de ejecutar esta rama. */
            if (chunk <= ((size_t)-1) / 2)
                chunk *= 2;
            else
                chunk = cap_uso;
        }
    }

    /* if: comprueba max_monedas > ((size_t)-1) / (amount + 1) - 1 antes de ejecutar esta rama. */
    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_k = (size_t *)malloc(estados * sizeof(size_t));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    item_usado = (size_t *)malloc(estados * sizeof(size_t));
    /* if: comprueba alcanzable == NULL || prev_k == NULL || prev_monto == NULL || item_us... antes de ejecutar esta rama. */
    if (alcanzable == NULL || prev_k == NULL || prev_monto == NULL || item_usado == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < estados; i++ para recorrer el bloque. */
    for (i = 0; i < estados; i++)
    {
        prev_k[i] = invalid;
        prev_monto[i] = 0;
        item_usado[i] = invalid;
    }

    alcanzable[0] = 1;
    /* for: itera segun i = 0; i < items_len; i++ para recorrer el bloque. */
    for (i = 0; i < items_len; i++)
    {
        size_t k;
        size_t valor_item = items[i].valor;
        size_t monedas_item = items[i].monedas;

        /* for: itera segun k = max_monedas + 1; k > monedas_item; para recorrer el bloque. */
        for (k = max_monedas + 1; k > monedas_item;)
        {
            size_t kk = k - 1;
            size_t m;

            /* for: itera segun m = amount + 1; m > valor_item; para recorrer el bloque. */
            for (m = amount + 1; m > valor_item;)
            {
                size_t mm = m - 1;
                size_t idx = kk * stride + mm;
                size_t idx_prev = (kk - monedas_item) * stride + (mm - valor_item);

                /* if: comprueba !alcanzable[idx] && alcanzable[idx_prev] antes de ejecutar esta rama. */
                if (!alcanzable[idx] && alcanzable[idx_prev])
                {
                    alcanzable[idx] = 1;
                    prev_k[idx] = kk - monedas_item;
                    prev_monto[idx] = mm - valor_item;
                    item_usado[idx] = i;
                }

                m--;
            }

            k--;
        }
    }

    /* for: itera segun i = min_monedas; i <= max_monedas; i++ para recorrer el bloque. */
    for (i = min_monedas; i <= max_monedas; i++)
    {
        /* if: comprueba alcanzable[i * stride + amount] antes de ejecutar esta rama. */
        if (alcanzable[i * stride + amount])
        {
            objetivo_k = i;
            break;
        }
    }

    /* if: comprueba objetivo_k == (size_t)-1 antes de ejecutar esta rama. */
    if (objetivo_k == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t k_actual = objetivo_k;
        size_t monto_actual = amount;

        /* while: repite el bloque mientras se cumpla k_actual > 0. */
        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t idx_item = item_usado[idx];

            /* if: comprueba idx_item == invalid || idx_item >= items_len antes de ejecutar esta rama. */
            if (idx_item == invalid || idx_item >= items_len)
                goto cleanup;

            cantidades[items[idx_item].idx_denom] += items[idx_item].monedas;
            monto_actual = prev_monto[idx];
            k_actual = prev_k[idx];
        }

        /* if: comprueba monto_actual != 0 antes de ejecutar esta rama. */
        if (monto_actual != 0)
            goto cleanup;
    }

    resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;

cleanup:
    free(valores);
    free(topes);
    free(cantidades);
    free(alcanzable);
    free(prev_k);
    free(prev_monto);
    free(item_usado);
    free(items);
    return resultado;
}

/* funcion calcular_dp_ilimitado_cercano_rango: contiene la logica principal de esta operacion. */
static int calcular_dp_ilimitado_cercano_rango(const BigInt *monto,
                                               const BigIntArray *denom,
                                               size_t min_monedas,
                                               size_t max_monedas,
                                               size_t *monto_cubierto,
                                               BigIntArray *solucion)
{
    size_t amount;
    size_t stride;
    size_t estados;
    size_t *valores = NULL;
    size_t *cantidades = NULL;
    unsigned char *alcanzable = NULL;
    size_t *prev_monto = NULL;
    size_t *moneda = NULL;
    size_t min_valor = (size_t)-1;
    size_t i;
    size_t k;
    size_t objetivo_k = (size_t)-1;
    size_t objetivo_monto = (size_t)-1;
    int resultado = DP_NO_APLICA;

    /* if: comprueba !bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount) antes de ejecutar esta rama. */
    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    /* if: comprueba monto_cubierto == NULL antes de ejecutar esta rama. */
    if (monto_cubierto == NULL)
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    /* if: comprueba valores == NULL || cantidades == NULL antes de ejecutar esta rama. */
    if (valores == NULL || cantidades == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        /* if: comprueba !bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) antes de ejecutar esta rama. */
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]))
            goto cleanup;

        /* if: comprueba valores[i] > 0 && valores[i] < min_valor antes de ejecutar esta rama. */
        if (valores[i] > 0 && valores[i] < min_valor)
            min_valor = valores[i];
    }

    /* if: comprueba amount == 0 antes de ejecutar esta rama. */
    if (amount == 0)
    {
        /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
        if (min_monedas == 0)
        {
            *monto_cubierto = 0;
            resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;
        }
        else
        {
            resultado = DP_SIN_SOLUCION;
        }
        goto cleanup;
    }

    /* if: comprueba min_valor == (size_t)-1 antes de ejecutar esta rama. */
    if (min_valor == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t tope_natural = amount / min_valor;
        /* if: comprueba max_monedas > tope_natural antes de ejecutar esta rama. */
        if (max_monedas > tope_natural)
            max_monedas = tope_natural;
    }

    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    /* if: comprueba max_monedas > ((size_t)-1) / (amount + 1) - 1 antes de ejecutar esta rama. */
    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    moneda = (size_t *)malloc(estados * sizeof(size_t));
    /* if: comprueba alcanzable == NULL || prev_monto == NULL || moneda == NULL antes de ejecutar esta rama. */
    if (alcanzable == NULL || prev_monto == NULL || moneda == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < estados; i++ para recorrer el bloque. */
    for (i = 0; i < estados; i++)
    {
        prev_monto[i] = 0;
        moneda[i] = denom->len;
    }

    alcanzable[0] = 1;
    /* for: itera segun k = 1; k <= max_monedas; k++ para recorrer el bloque. */
    for (k = 1; k <= max_monedas; k++)
    {
        size_t monto_k;

        /* for: itera segun monto_k = 0; monto_k <= amount; monto_k++ para recorrer el bloque. */
        for (monto_k = 0; monto_k <= amount; monto_k++)
        {
            size_t j;
            size_t idx = k * stride + monto_k;

            /* for: itera segun j = 0; j < denom->len; j++ para recorrer el bloque. */
            for (j = 0; j < denom->len; j++)
            {
                size_t valor = valores[j];
                size_t idx_prev;

                /* if: comprueba valor == 0 || valor > monto_k antes de ejecutar esta rama. */
                if (valor == 0 || valor > monto_k)
                    continue;

                idx_prev = (k - 1) * stride + (monto_k - valor);
                /* if: comprueba !alcanzable[idx_prev] antes de ejecutar esta rama. */
                if (!alcanzable[idx_prev])
                    continue;

                alcanzable[idx] = 1;
                prev_monto[idx] = monto_k - valor;
                moneda[idx] = j;
                break;
            }
        }
    }

    {
        size_t monto_busqueda = amount;
        int encontrado = 0;

        /* while: repite el bloque mientras se cumpla 1. */
        while (1)
        {
            /* for: itera segun k = min_monedas; k <= max_monedas; k++ para recorrer el bloque. */
            for (k = min_monedas; k <= max_monedas; k++)
            {
                /* if: comprueba alcanzable[k * stride + monto_busqueda] antes de ejecutar esta rama. */
                if (alcanzable[k * stride + monto_busqueda])
                {
                    objetivo_k = k;
                    objetivo_monto = monto_busqueda;
                    encontrado = 1;
                    break;
                }
            }

            /* if: comprueba encontrado antes de ejecutar esta rama. */
            if (encontrado)
                break;

            /* if: comprueba monto_busqueda == 0 antes de ejecutar esta rama. */
            if (monto_busqueda == 0)
                break;

            monto_busqueda--;
        }
    }

    /* if: comprueba objetivo_k == (size_t)-1 || objetivo_monto == (size_t)-1 || objetivo_... antes de ejecutar esta rama. */
    if (objetivo_k == (size_t)-1 || objetivo_monto == (size_t)-1 || objetivo_monto == 0)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t monto_actual = objetivo_monto;
        size_t k_actual = objetivo_k;

        /* while: repite el bloque mientras se cumpla k_actual > 0. */
        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t j = moneda[idx];

            /* if: comprueba j >= denom->len || prev_monto[idx] > monto_actual antes de ejecutar esta rama. */
            if (j >= denom->len || prev_monto[idx] > monto_actual)
                goto cleanup;

            cantidades[j]++;
            monto_actual = prev_monto[idx];
            k_actual--;
        }

        /* if: comprueba monto_actual != 0 antes de ejecutar esta rama. */
        if (monto_actual != 0)
            goto cleanup;
    }

    *monto_cubierto = objetivo_monto;
    resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;

cleanup:
    free(valores);
    free(cantidades);
    free(alcanzable);
    free(prev_monto);
    free(moneda);
    return resultado;
}

/* funcion calcular_dp_stock_cercano_rango: contiene la logica principal de esta operacion. */
static int calcular_dp_stock_cercano_rango(const BigInt *monto,
                                           const BigIntArray *denom,
                                           const BigIntArray *stock,
                                           size_t min_monedas,
                                           size_t max_monedas,
                                           size_t *monto_cubierto,
                                           BigIntArray *solucion)
{
    size_t amount;
    size_t stride;
    size_t estados;
    size_t *valores = NULL;
    size_t *topes = NULL;
    size_t *cantidades = NULL;
    unsigned char *alcanzable = NULL;
    size_t *prev_k = NULL;
    size_t *prev_monto = NULL;
    size_t *item_usado = NULL;
    DPItemStock *items = NULL;
    size_t items_len = 0;
    size_t items_cap = 0;
    size_t i;
    size_t objetivo_k = (size_t)-1;
    size_t objetivo_monto = (size_t)-1;
    size_t tope_monedas = 0;
    const size_t invalid = (size_t)-1;
    int resultado = DP_NO_APLICA;

    /* if: comprueba !bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount) antes de ejecutar esta rama. */
    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    /* if: comprueba monto_cubierto == NULL antes de ejecutar esta rama. */
    if (monto_cubierto == NULL)
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    topes = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    /* if: comprueba valores == NULL || topes == NULL || cantidades == NULL antes de ejecutar esta rama. */
    if (valores == NULL || topes == NULL || cantidades == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;

        /* if: comprueba !bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) || antes de ejecutar esta rama. */
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) ||
            !bigint_a_size_con_tope(&stock->items[i], amount, &topes[i]))
        {
            goto cleanup;
        }

        /* if: comprueba valores[i] == 0 antes de ejecutar esta rama. */
        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        {
            size_t cap_por_monto = amount / valores[i];
            /* if: comprueba cap_por_monto < cap_uso antes de ejecutar esta rama. */
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }

        tope_monedas += cap_uso;
    }

    /* if: comprueba amount == 0 antes de ejecutar esta rama. */
    if (amount == 0)
    {
        /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
        if (min_monedas == 0)
        {
            *monto_cubierto = 0;
            resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;
        }
        else
        {
            resultado = DP_SIN_SOLUCION;
        }
        goto cleanup;
    }

    /* if: comprueba max_monedas > tope_monedas antes de ejecutar esta rama. */
    if (max_monedas > tope_monedas)
        max_monedas = tope_monedas;

    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    /* for: itera segun i = 0; i < denom->len; i++ para recorrer el bloque. */
    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;
        size_t chunk = 1;

        /* if: comprueba valores[i] == 0 antes de ejecutar esta rama. */
        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        {
            size_t cap_por_monto = amount / valores[i];
            /* if: comprueba cap_por_monto < cap_uso antes de ejecutar esta rama. */
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }
        /* if: comprueba cap_uso > max_monedas antes de ejecutar esta rama. */
        if (cap_uso > max_monedas)
            cap_uso = max_monedas;

        /* while: repite el bloque mientras se cumpla cap_uso > 0. */
        while (cap_uso > 0)
        {
            size_t take = (chunk < cap_uso) ? chunk : cap_uso;
            size_t valor_item = take * valores[i];

            /* if: comprueba !push_item_stock(&items, &items_len, &items_cap, i, take, valor_item) antes de ejecutar esta rama. */
            if (!push_item_stock(&items, &items_len, &items_cap, i, take, valor_item))
                goto cleanup;

            cap_uso -= take;
            /* if: comprueba chunk <= ((size_t)-1) / 2 antes de ejecutar esta rama. */
            if (chunk <= ((size_t)-1) / 2)
                chunk *= 2;
            else
                chunk = cap_uso;
        }
    }

    /* if: comprueba max_monedas > ((size_t)-1) / (amount + 1) - 1 antes de ejecutar esta rama. */
    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_k = (size_t *)malloc(estados * sizeof(size_t));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    item_usado = (size_t *)malloc(estados * sizeof(size_t));
    /* if: comprueba alcanzable == NULL || prev_k == NULL || prev_monto == NULL || item_us... antes de ejecutar esta rama. */
    if (alcanzable == NULL || prev_k == NULL || prev_monto == NULL || item_usado == NULL)
        goto cleanup;

    /* for: itera segun i = 0; i < estados; i++ para recorrer el bloque. */
    for (i = 0; i < estados; i++)
    {
        prev_k[i] = invalid;
        prev_monto[i] = 0;
        item_usado[i] = invalid;
    }

    alcanzable[0] = 1;
    /* for: itera segun i = 0; i < items_len; i++ para recorrer el bloque. */
    for (i = 0; i < items_len; i++)
    {
        size_t k;
        size_t valor_item = items[i].valor;
        size_t monedas_item = items[i].monedas;

        /* for: itera segun k = max_monedas + 1; k > monedas_item; para recorrer el bloque. */
        for (k = max_monedas + 1; k > monedas_item;)
        {
            size_t kk = k - 1;
            size_t m;

            /* for: itera segun m = amount + 1; m > valor_item; para recorrer el bloque. */
            for (m = amount + 1; m > valor_item;)
            {
                size_t mm = m - 1;
                size_t idx = kk * stride + mm;
                size_t idx_prev = (kk - monedas_item) * stride + (mm - valor_item);

                /* if: comprueba !alcanzable[idx] && alcanzable[idx_prev] antes de ejecutar esta rama. */
                if (!alcanzable[idx] && alcanzable[idx_prev])
                {
                    alcanzable[idx] = 1;
                    prev_k[idx] = kk - monedas_item;
                    prev_monto[idx] = mm - valor_item;
                    item_usado[idx] = i;
                }

                m--;
            }

            k--;
        }
    }

    {
        size_t monto_busqueda = amount;
        int encontrado = 0;

        /* while: repite el bloque mientras se cumpla 1. */
        while (1)
        {
            /* for: itera segun i = min_monedas; i <= max_monedas; i++ para recorrer el bloque. */
            for (i = min_monedas; i <= max_monedas; i++)
            {
                /* if: comprueba alcanzable[i * stride + monto_busqueda] antes de ejecutar esta rama. */
                if (alcanzable[i * stride + monto_busqueda])
                {
                    objetivo_k = i;
                    objetivo_monto = monto_busqueda;
                    encontrado = 1;
                    break;
                }
            }

            /* if: comprueba encontrado antes de ejecutar esta rama. */
            if (encontrado)
                break;

            /* if: comprueba monto_busqueda == 0 antes de ejecutar esta rama. */
            if (monto_busqueda == 0)
                break;

            monto_busqueda--;
        }
    }

    /* if: comprueba objetivo_k == (size_t)-1 || objetivo_monto == (size_t)-1 || objetivo_... antes de ejecutar esta rama. */
    if (objetivo_k == (size_t)-1 || objetivo_monto == (size_t)-1 || objetivo_monto == 0)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t k_actual = objetivo_k;
        size_t monto_actual = objetivo_monto;

        /* while: repite el bloque mientras se cumpla k_actual > 0. */
        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t idx_item = item_usado[idx];

            /* if: comprueba idx_item == invalid || idx_item >= items_len antes de ejecutar esta rama. */
            if (idx_item == invalid || idx_item >= items_len)
                goto cleanup;

            cantidades[items[idx_item].idx_denom] += items[idx_item].monedas;
            monto_actual = prev_monto[idx];
            k_actual = prev_k[idx];
        }

        /* if: comprueba monto_actual != 0 antes de ejecutar esta rama. */
        if (monto_actual != 0)
            goto cleanup;
    }

    *monto_cubierto = objetivo_monto;
    resultado = cargar_solucion_size(cantidades, denom->len, solucion) ? DP_OK : DP_NO_APLICA;

cleanup:
    free(valores);
    free(topes);
    free(cantidades);
    free(alcanzable);
    free(prev_k);
    free(prev_monto);
    free(item_usado);
    free(items);
    return resultado;
}

/* funcion solucion_cumple_max_monedas: contiene la logica principal de esta operacion. */
static int solucion_cumple_max_monedas(const BigIntArray *solucion, size_t max_monedas)
{
    size_t total = 0;
    size_t i;

    /* if: comprueba solucion == NULL || solucion->items == NULL antes de ejecutar esta rama. */
    if (solucion == NULL || solucion->items == NULL)
        return 0;

    /* for: itera segun i = 0; i < solucion->len; i++ para recorrer el bloque. */
    for (i = 0; i < solucion->len; i++)
    {
        size_t valor;

        /* if: comprueba !bigint_a_size_limitado(&solucion->items[i], max_monedas - total, &va... antes de ejecutar esta rama. */
        if (!bigint_a_size_limitado(&solucion->items[i], max_monedas - total, &valor))
            return 0;

        total += valor;
    }

    return total <= max_monedas;
}

/* funcion copiar_bigint: contiene la logica principal de esta operacion. */
static int copiar_bigint(BigInt *dest, const BigInt *src)
{
    /* if: comprueba dest == NULL || src == NULL || src->digits == NULL antes de ejecutar esta rama. */
    if (dest == NULL || src == NULL || src->digits == NULL)
        return 0;

    /* if: comprueba bigint_set(dest, src) antes de ejecutar esta rama. */
    if (bigint_set(dest, src))
        return 1;

    return bigint_init(dest, src->digits);
}

/*
 * Closest amount with two unlimited positive denominations <=50,000.
 * Let a>b and g=gcd(a,b). Every minimum-piece representation can use
 * fewer than a/g coins of value b: a/g small coins can always be replaced
 * with b/g large coins of equal value and strictly fewer total pieces.
 * For each such count y, take x=floor((amount-b*y)/a). The residual amount
 * is <a and can be compared using size_t without truncating BigInt.
 * Thus at most 50,000 integer iterations and a handful of BigInt operations
 * prove both maximum reachable coverage and minimum piece count.
 * Return -1 when outside this proven special case, 0 on runtime failure.
 */
static int cercano_dos_denominaciones_bigint(const BigInt *monto,
        const BigIntArray *denom, BigInt *monto_cubierto, BigIntArray *solucion)
{
    size_t d[2], high, low, a, b, g, max_y, cap, clipped = 0;
    size_t remaining_mod = 0, best_rem = SIZE_MAX, best_y = 0;
    BigInt large = {0}, big_q = {0}, big_r = {0};
    BigInt paid = {0}, left = {0}, high_count = {0}, residual = {0};
    BigInt partial = {0}, low_count = {0};
    int outcome = 0;
    if (!monto || !monto->digits || !denom || !denom->items ||
        denom->len != 2 || !monto_cubierto || !solucion)
        return -1;
    if (!bigint_a_size_limitado(&denom->items[0],50000,&d[0]) ||
        !bigint_a_size_limitado(&denom->items[1],50000,&d[1]) ||
        d[0] == 0 || d[1] == 0 || d[0] == d[1]) return -1;

    high = d[0] > d[1] ? 0 : 1;
    low = 1-high;
    a = d[high];
    b = d[low];
    g = a;
    { size_t t=b;
      while (t != 0) { size_t r=g%t;g=t;t=r; }
    }
    max_y = a/g;
    if (b != 0 && max_y-1 > SIZE_MAX/b) return -1;
    cap = (max_y-1)*b;
    if (!bigint_a_size_con_tope(monto,cap,&clipped) ||
        !size_a_bigint(a,&large) ||
        !bigint_divmod(monto,&large,&big_q,&big_r) ||
        !bigint_a_size_limitado(&big_r,a-1,&remaining_mod))
        goto cleanup;

    for (size_t y=0; y<max_y; ++y)
    {
        size_t y_value=y*b;
        if (y_value>clipped) break;
        size_t rem=(remaining_mod+a-(y_value%a))%a;
        if (rem<best_rem) {best_rem=rem;best_y=y;}
    }
    if (best_rem==SIZE_MAX) goto cleanup;
    if (!size_a_bigint(best_y*b,&paid) ||
        !bigint_subtract(monto,&paid,&left) ||
        !bigint_divmod(&left,&large,&high_count,&residual) ||
        !bigint_subtract(monto,&residual,&partial) ||
        !size_a_bigint(best_y,&low_count))
        goto cleanup;
    if (bigint_is_zero(&partial)) goto cleanup;

    if (!bigint_array_create(solucion,2)) goto cleanup;
    if (!bigint_array_set(solucion,high,&high_count) ||
        !bigint_array_set(solucion,low,&low_count) ||
        !copiar_bigint(monto_cubierto,&partial))
    {
        bigint_array_free(solucion);
        goto cleanup;
    }
    outcome = 1;

cleanup:
    bigint_free(&large);bigint_free(&big_q);bigint_free(&big_r);
    bigint_free(&paid);bigint_free(&left);bigint_free(&high_count);
    bigint_free(&residual);bigint_free(&partial);bigint_free(&low_count);
    return outcome;
}

/*
 * Exact nearest change for TWO bounded denominations when the amount and
 * relevant stock quantities fit size_t. Intended for the large-amount path
 * above 50,000. For y small coins modulo a/gcd(a,b), coverage increases
 * until the high-coin stock stops binding, then coverage is constant and the
 * earliest y gives the fewest coins. Two candidates per residue suffice.
 */
typedef struct
{
    size_t amount, a, b, high_stock;
    size_t covered, high_count, low_count, pieces;
    int found;
} NearStockTwo;

static void consider_nearest_stock_two(NearStockTwo *state, size_t y)
{
    size_t small_value = y * state->b;
    size_t x = (state->amount - small_value) / state->a;
    if (x > state->high_stock)
        x = state->high_stock;
    size_t paid = x * state->a + small_value;
    size_t pieces = x + y;
    if (paid == 0)
        return;
    if (!state->found || paid > state->covered ||
        (paid == state->covered && pieces < state->pieces))
    {
        state->found = 1;
        state->covered = paid;
        state->high_count = x;
        state->low_count = y;
        state->pieces = pieces;
    }
}

static int cercano_dos_denominaciones_stock_size(const BigInt *monto,
        const BigIntArray *denom, const BigIntArray *stock,
        BigInt *monto_cubierto, BigIntArray *solucion)
{
    size_t amount = 0, d[2] = {0}, high = 0, low = 1;
    size_t high_stock = 0, max_y = 0, gcd, period, saturated;
    BigInt hi = {0}, lo = {0};
    int result = 0;

    if (!monto || !denom || !denom->items || denom->len != 2 ||
        !stock || !stock->items || stock->len != 2 ||
        !monto_cubierto || !solucion ||
        !bigint_a_size_limitado(monto, SIZE_MAX, &amount) ||
        !bigint_a_size_limitado(&denom->items[0],50000,&d[0]) ||
        !bigint_a_size_limitado(&denom->items[1],50000,&d[1]) ||
        !d[0] || !d[1] || d[0] == d[1])
        return -1;

    if (d[1] > d[0]) { high = 1; low = 0; }
    size_t a = d[high], b = d[low];
    if (!bigint_a_size_con_tope(&stock->items[high],amount/a,&high_stock) ||
        !bigint_a_size_con_tope(&stock->items[low],amount/b,&max_y))
        return 0;
    gcd = a;
    { size_t x=b;
      while(x) { size_t rem=gcd%x; gcd=x; x=rem; }
    }
    period = a/gcd;
    saturated = (amount - high_stock*a)/b;
    NearStockTwo best = { .amount=amount, .a=a, .b=b,
                          .high_stock=high_stock };

    for (size_t r=0; r<period && r<=max_y; ++r)
    {
        size_t top = saturated < max_y ? saturated : max_y;
        if (top >= r)
        {
            size_t y = r + ((top-r)/period)*period;
            consider_nearest_stock_two(&best,y);
        }
        if (saturated < r)
        {
            consider_nearest_stock_two(&best,r);
        }
        else if (saturated < max_y)
        {
            size_t steps=(saturated-r)/period;
            /* Bound the addition by max_y to prevent size_t overflow. */
            if (steps < (max_y-r)/period)
                consider_nearest_stock_two(&best,r+(steps+1)*period);
        }
    }

    if (!best.found)
        return 0;
    if (!size_a_bigint(best.high_count,&hi) ||
        !size_a_bigint(best.low_count,&lo) ||
        !bigint_array_create(solucion,2))
        goto cleanup;
    if (!bigint_array_set(solucion,high,&hi) ||
        !bigint_array_set(solucion,low,&lo) ||
        !size_a_bigint(best.covered,monto_cubierto))
    {
        bigint_array_free(solucion);
        goto cleanup;
    }
    result = 1;
cleanup:
    bigint_free(&hi);
    bigint_free(&lo);
    return result;
}

/* funcion construir_sufijos_stock: contiene la logica principal de esta operacion. */
static int construir_sufijos_stock(const BigIntArray *denom,
                                   const BigIntArray *stock,
                                   BigInt **sufijos_out)
{
    BigInt *sufijos;
    size_t i;

    /* if: comprueba denom == NULL || stock == NULL || sufijos_out == NULL || denom->len !... antes de ejecutar esta rama. */
    if (denom == NULL || stock == NULL || sufijos_out == NULL || denom->len != stock->len)
        return 0;

    sufijos = (BigInt *)calloc(denom->len + 1, sizeof(BigInt));
    /* if: comprueba sufijos == NULL antes de ejecutar esta rama. */
    if (sufijos == NULL)
        return 0;

    /* if: comprueba !bigint_init(&sufijos[denom->len], "0") antes de ejecutar esta rama. */
    if (!bigint_init(&sufijos[denom->len], "0"))
    {
        free(sufijos);
        return 0;
    }

    /* for: itera segun i = denom->len; i > 0; i-- para recorrer el bloque. */
    for (i = denom->len; i > 0; i--)
    {
        BigInt aporte = {0};
        BigInt acumulado = {0};

        /* if: comprueba !bigint_multiply(&denom->items[i - 1], &stock->items[i - 1], &aporte) || antes de ejecutar esta rama. */
        if (!bigint_multiply(&denom->items[i - 1], &stock->items[i - 1], &aporte) ||
            !bigint_add(&aporte, &sufijos[i], &acumulado) ||
            !copiar_bigint(&sufijos[i - 1], &acumulado))
        {
            size_t j;
            bigint_free(&aporte);
            bigint_free(&acumulado);
            /* for: itera segun j = 0; j <= denom->len; j++ para recorrer el bloque. */
            for (j = 0; j <= denom->len; j++)
                bigint_free(&sufijos[j]);
            free(sufijos);
            return 0;
        }

        bigint_free(&aporte);
        bigint_free(&acumulado);
    }

    *sufijos_out = sufijos;
    return 1;
}

/* funcion liberar_sufijos_stock: contiene la logica principal de esta operacion. */
static void liberar_sufijos_stock(BigInt *sufijos, size_t len)
{
    size_t i;

    /* if: comprueba sufijos == NULL antes de ejecutar esta rama. */
    if (sufijos == NULL)
        return;

    /* for: itera segun i = 0; i <= len; i++ para recorrer el bloque. */
    for (i = 0; i <= len; i++)
        bigint_free(&sufijos[i]);
    free(sufijos);
}

/* funcion backtrack_ilimitado: contiene la logica principal de esta operacion. */
static int backtrack_ilimitado(const BigIntArray *denom,
                               const BigInt *one,
                               const BigInt *zero,
                               BigInt *restante,
                               BigIntArray *solucion,
                               size_t index)
{
    BigInt max_usable = {0};
    BigInt residuo = {0};
    BigInt cuenta = {0};
    BigInt uso = {0};
    BigInt actual_restante = {0};
    int ok = 0;

    /* if: comprueba bigint_is_zero(restante) antes de ejecutar esta rama. */
    if (bigint_is_zero(restante))
        return 1;
    /* if: comprueba index >= denom->len antes de ejecutar esta rama. */
    if (index >= denom->len)
        return 0;

    /* if: comprueba bigint_is_zero(&denom->items[index]) antes de ejecutar esta rama. */
    if (bigint_is_zero(&denom->items[index]))
    {
        /* if: comprueba !bigint_array_set(solucion, index, zero) antes de ejecutar esta rama. */
        if (!bigint_array_set(solucion, index, zero))
            return 0;
        return backtrack_ilimitado(denom, one, zero, restante, solucion, index + 1);
    }

    /* if: comprueba !bigint_divmod(restante, &denom->items[index], &max_usable, &residuo) || antes de ejecutar esta rama. */
    if (!bigint_divmod(restante, &denom->items[index], &max_usable, &residuo) ||
        !copiar_bigint(&cuenta, &max_usable) ||
        !bigint_multiply(&denom->items[index], &cuenta, &uso) ||
        !bigint_subtract(restante, &uso, &actual_restante))
        goto cleanup;

    /* while: repite el bloque mientras se cumpla 1. */
    while (1)
    {
        /* if: comprueba !bigint_array_set(solucion, index, &cuenta) antes de ejecutar esta rama. */
        if (!bigint_array_set(solucion, index, &cuenta))
            goto cleanup;

        /* if: comprueba backtrack_ilimitado(denom, one, zero, &actual_restante, solucion, ind... antes de ejecutar esta rama. */
        if (backtrack_ilimitado(denom, one, zero, &actual_restante, solucion, index + 1))
        {
            ok = 1;
            goto cleanup;
        }

        /* if: comprueba bigint_compare(&cuenta, zero) == 0 antes de ejecutar esta rama. */
        if (bigint_compare(&cuenta, zero) == 0)
            break;

        {
            BigInt sig_cuenta = {0};
            BigInt sig_restante = {0};

            /* if: comprueba !bigint_subtract(&cuenta, one, &sig_cuenta) || antes de ejecutar esta rama. */
            if (!bigint_subtract(&cuenta, one, &sig_cuenta) ||
                !bigint_add(&actual_restante, &denom->items[index], &sig_restante))
            {
                bigint_free(&sig_cuenta);
                bigint_free(&sig_restante);
                goto cleanup;
            }

            bigint_free(&cuenta);
            bigint_free(&actual_restante);
            cuenta = sig_cuenta;
            actual_restante = sig_restante;
        }
    }

    /* if: comprueba !bigint_array_set(solucion, index, zero) antes de ejecutar esta rama. */
    if (!bigint_array_set(solucion, index, zero))
        goto cleanup;

cleanup:
    bigint_free(&max_usable);
    bigint_free(&residuo);
    bigint_free(&cuenta);
    bigint_free(&uso);
    bigint_free(&actual_restante);
    return ok;
}

/* funcion backtrack_stock: contiene la logica principal de esta operacion. */
static int backtrack_stock(const BigIntArray *denom,
                           const BigIntArray *stock,
                           const BigIntArray *sufijos,
                           const BigInt *one,
                           const BigInt *zero,
                           BigInt *restante,
                           BigIntArray *solucion,
                           size_t index)
{
    BigInt max_usable = {0};
    BigInt residuo = {0};
    BigInt cuenta = {0};
    BigInt uso = {0};
    BigInt actual_restante = {0};
    int ok = 0;

    /* if: comprueba bigint_is_zero(restante) antes de ejecutar esta rama. */
    if (bigint_is_zero(restante))
        return 1;
    /* if: comprueba index >= denom->len antes de ejecutar esta rama. */
    if (index >= denom->len)
        return 0;
    /* if: comprueba sufijos != NULL && index < sufijos->len && bigint_compare(restante, &... antes de ejecutar esta rama. */
    if (sufijos != NULL && index < sufijos->len && bigint_compare(restante, &sufijos->items[index]) > 0)
        return 0;

    /* if: comprueba bigint_is_zero(&denom->items[index]) antes de ejecutar esta rama. */
    if (bigint_is_zero(&denom->items[index]))
    {
        /* if: comprueba !bigint_array_set(solucion, index, zero) antes de ejecutar esta rama. */
        if (!bigint_array_set(solucion, index, zero))
            return 0;
        return backtrack_stock(denom, stock, sufijos, one, zero, restante, solucion, index + 1);
    }

    /* if: comprueba !bigint_divmod(restante, &denom->items[index], &max_usable, &residuo) antes de ejecutar esta rama. */
    if (!bigint_divmod(restante, &denom->items[index], &max_usable, &residuo))
        goto cleanup;

    /* if: comprueba bigint_compare(&max_usable, &stock->items[index]) > 0 antes de ejecutar esta rama. */
    if (bigint_compare(&max_usable, &stock->items[index]) > 0)
    {
        /* if: comprueba !copiar_bigint(&cuenta, &stock->items[index]) antes de ejecutar esta rama. */
        if (!copiar_bigint(&cuenta, &stock->items[index]))
            goto cleanup;
    }
    else
    {
        /* if: comprueba !copiar_bigint(&cuenta, &max_usable) antes de ejecutar esta rama. */
        if (!copiar_bigint(&cuenta, &max_usable))
            goto cleanup;
    }

    /* if: comprueba !bigint_multiply(&denom->items[index], &cuenta, &uso) || antes de ejecutar esta rama. */
    if (!bigint_multiply(&denom->items[index], &cuenta, &uso) ||
        !bigint_subtract(restante, &uso, &actual_restante))
        goto cleanup;

    /* while: repite el bloque mientras se cumpla 1. */
    while (1)
    {
        /* if: comprueba !bigint_array_set(solucion, index, &cuenta) antes de ejecutar esta rama. */
        if (!bigint_array_set(solucion, index, &cuenta))
            goto cleanup;

        /* if: comprueba backtrack_stock(denom, stock, sufijos, one, zero, &actual_restante, s... antes de ejecutar esta rama. */
        if (backtrack_stock(denom, stock, sufijos, one, zero, &actual_restante, solucion, index + 1))
        {
            ok = 1;
            goto cleanup;
        }

        /* if: comprueba bigint_compare(&cuenta, zero) == 0 antes de ejecutar esta rama. */
        if (bigint_compare(&cuenta, zero) == 0)
            break;

        {
            BigInt sig_cuenta = {0};
            BigInt sig_restante = {0};

            /* if: comprueba !bigint_subtract(&cuenta, one, &sig_cuenta) || antes de ejecutar esta rama. */
            if (!bigint_subtract(&cuenta, one, &sig_cuenta) ||
                !bigint_add(&actual_restante, &denom->items[index], &sig_restante))
            {
                bigint_free(&sig_cuenta);
                bigint_free(&sig_restante);
                goto cleanup;
            }

            bigint_free(&cuenta);
            bigint_free(&actual_restante);
            cuenta = sig_cuenta;
            actual_restante = sig_restante;
        }
    }

    /* if: comprueba !bigint_array_set(solucion, index, zero) antes de ejecutar esta rama. */
    if (!bigint_array_set(solucion, index, zero))
        goto cleanup;

cleanup:
    bigint_free(&max_usable);
    bigint_free(&residuo);
    bigint_free(&cuenta);
    bigint_free(&uso);
    bigint_free(&actual_restante);
    return ok;
}

/*
 * Certifies optimal BigInt change up to 8 pieces for <=8 types
 * (up to four for larger alphabets). Tries counts in ascending order and enumerates
 * nondecreasing denomination indices. This never scales memory with amount.
 * A missing witness is inconclusive, not proof of infeasibility.
 */
static int testigo_corto_rec(const BigInt *remaining, const BigIntArray *denom,
                            const BigIntArray *stock, size_t start, size_t depth,
                            size_t *counts)
{
    size_t i;
    if (depth == 0) return bigint_is_zero(remaining) ? 1 : 0;
    if (bigint_is_zero(remaining)) return 0;
    for (i=start; i<denom->len; ++i)
    {
        BigInt next={0};
        size_t limit=0;
        int rc;
        if (!denom->items[i].digits || bigint_is_zero(&denom->items[i]) ||
            bigint_compare(&denom->items[i],remaining)>0)
            continue;
        if (stock)
        {
            if (!bigint_a_size_con_tope(&stock->items[i],12,&limit))
                return -1;
            if (counts[i]>=limit) continue;
        }
        if (!bigint_subtract(remaining,&denom->items[i],&next))
        {
            bigint_free(&next);
            return -1;
        }
        counts[i]++;
        rc=testigo_corto_rec(&next,denom,stock,i,depth-1,counts);
        /* Keep the successful path for result reconstruction. */
        if (rc!=1) counts[i]--;
        bigint_free(&next);
        if (rc!=0) return rc;
    }
    return 0;
}

static int testigo_corto_bigint(const BigInt *amount,const BigIntArray *denom,
                               const BigIntArray *stock,size_t min,size_t max,
                               BigIntArray *out)
{
    size_t counts[24]={0},depth,i;
    int rc=0;
    if (!amount || !amount->digits || !denom || !denom->items || !out ||
        !denom->len || denom->len>24 || min>max ||
        min>(denom->len<=5 ? 12 : denom->len<=8 ? 8 : 4) ||
        (stock && (!stock->items || stock->len!=denom->len)))
        return 0;
    if(min<1)min=1;
    if(max>(denom->len<=5 ? 12 : denom->len<=8 ? 8 : 4))
        max=(denom->len<=5 ? 12 : denom->len<=8 ? 8 : 4);
    for(depth=min;depth<=max;++depth)
    {
        rc=testigo_corto_rec(amount,denom,stock,0,depth,counts);
        if(rc!=0)break;
    }
    if(rc!=1)return rc;
    if(!bigint_array_create(out,denom->len))return -1;
    for(i=0;i<denom->len;++i)
    {
        BigInt qty={0};
        int ok=size_a_bigint(counts[i],&qty) &&
               bigint_array_set(out,i,&qty);
        bigint_free(&qty);
        if(!ok){bigint_array_free(out);return -1;}
    }
    return 1;
}


/* funcion calcular_cambio_optimo: contiene la logica principal de esta operacion. */
int calcular_cambio_optimo(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)
{
    BigInt restante = {0};
    BigInt one = {0};
    BigInt zero = {0};
    int dp;
    int res;

    /* if: comprueba monto == NULL || denominaciones == NULL || solucion == NULL antes de ejecutar esta rama. */
    if (monto == NULL || denominaciones == NULL || solucion == NULL)
        return 0;

    dp = calcular_dp_ilimitado(monto, denominaciones, solucion);
    /* if: comprueba dp != DP_NO_APLICA antes de ejecutar esta rama. */
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (testigo_corto_bigint(monto,denominaciones,NULL,0,4,solucion)==1)
        return 1;

    /* A 5..8 piece witness remains exact for at most eight types. */
    if (denominaciones->len<=8 &&
        testigo_corto_bigint(monto,denominaciones,NULL,5,8,solucion)==1)
        return 1;

    /* 9..12 coins: only a small denomination alphabet is affordable. */
    if (denominaciones->len<=5 &&
        testigo_corto_bigint(monto,denominaciones,NULL,9,12,solucion)==1)
        return 1;

    /* if: comprueba !bigint_array_create(solucion, denominaciones->len) antes de ejecutar esta rama. */
    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;
    /* if: comprueba !copiar_bigint(&restante, monto) || antes de ejecutar esta rama. */
    if (!copiar_bigint(&restante, monto) ||
        !bigint_init(&one, "1") ||
        !bigint_init(&zero, "0"))
    {
        bigint_free(&restante);
        bigint_free(&one);
        bigint_free(&zero);
        bigint_array_free(solucion);
        return 0;
    }

    res = backtrack_ilimitado(denominaciones, &one, &zero, &restante, solucion, 0);

    bigint_free(&restante);
    bigint_free(&one);
    bigint_free(&zero);
    /* if: comprueba !res antes de ejecutar esta rama. */
    if (!res)
        bigint_array_free(solucion);
    return res;
}

/* funcion calcular_cambio_optimo_stock: contiene la logica principal de esta operacion. */
int calcular_cambio_optimo_stock(const BigInt *monto,
                                 const BigIntArray *denominaciones,
                                 const BigIntArray *stock,
                                 BigIntArray *solucion)
{
    BigInt restante = {0};
    BigInt one = {0};
    BigInt zero = {0};
    BigInt *sufijos_raw = NULL;
    BigIntArray sufijos = {0};
    int dp;
    int res;

    /* if: comprueba monto == NULL || denominaciones == NULL || stock == NULL || solucion... antes de ejecutar esta rama. */
    if (monto == NULL || denominaciones == NULL || stock == NULL || solucion == NULL)
        return 0;
    /* if: comprueba denominaciones->len != stock->len antes de ejecutar esta rama. */
    if (denominaciones->len != stock->len)
        return 0;

    dp = calcular_dp_stock(monto, denominaciones, stock, solucion);
    /* if: comprueba dp != DP_NO_APLICA antes de ejecutar esta rama. */
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (testigo_corto_bigint(monto,denominaciones,stock,0,4,solucion)==1)
        return 1;

    /* A 5..8 piece witness remains exact for at most eight types. */
    if (denominaciones->len<=8 &&
        testigo_corto_bigint(monto,denominaciones,stock,5,8,solucion)==1)
        return 1;

    /* 9..12 coins: only a small denomination alphabet is affordable. */
    if (denominaciones->len<=5 &&
        testigo_corto_bigint(monto,denominaciones,stock,9,12,solucion)==1)
        return 1;

    /* if: comprueba !bigint_array_create(solucion, denominaciones->len) antes de ejecutar esta rama. */
    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;

    /* if: comprueba !copiar_bigint(&restante, monto) || antes de ejecutar esta rama. */
    if (!copiar_bigint(&restante, monto) ||
        !bigint_init(&one, "1") ||
        !bigint_init(&zero, "0") ||
        !construir_sufijos_stock(denominaciones, stock, &sufijos_raw))
    {
        bigint_free(&restante);
        bigint_free(&one);
        bigint_free(&zero);
        liberar_sufijos_stock(sufijos_raw, denominaciones != NULL ? denominaciones->len : 0);
        bigint_array_free(solucion);
        return 0;
    }

    sufijos.items = sufijos_raw;
    sufijos.len = denominaciones->len + 1;

    res = backtrack_stock(denominaciones, stock, &sufijos, &one, &zero, &restante, solucion, 0);

    bigint_free(&restante);
    bigint_free(&one);
    bigint_free(&zero);
    liberar_sufijos_stock(sufijos_raw, denominaciones->len);

    /* if: comprueba !res antes de ejecutar esta rama. */
    if (!res)
        bigint_array_free(solucion);
    return res;
}

/* funcion calcular_cambio_optimo_stock_con_limite: contiene la logica principal de esta operacion. */
int calcular_cambio_optimo_stock_con_limite(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, size_t limite_monedas, BigIntArray *solucion)
{
    /* if: comprueba limite_monedas == 0 antes de ejecutar esta rama. */
    if (limite_monedas == 0)
        return calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion);

    return calcular_cambio_optimo_stock_con_rango(monto, denominaciones, stock, 0, limite_monedas, solucion);
}

/* funcion calcular_cambio_optimo_con_limite: contiene la logica principal de esta operacion. */
int calcular_cambio_optimo_con_limite(const BigInt *monto, const BigIntArray *denominaciones, size_t limite_monedas, BigIntArray *solucion)
{
    /* if: comprueba limite_monedas == 0 antes de ejecutar esta rama. */
    if (limite_monedas == 0)
        return calcular_cambio_optimo(monto, denominaciones, solucion);

    return calcular_cambio_optimo_con_rango(monto, denominaciones, 0, limite_monedas, solucion);
}

static int calcular_cercano_una_denominacion_bigint(
    const BigInt *monto, const BigIntArray *denom, const BigIntArray *stock,
    size_t min_monedas, size_t max_monedas,
    BigInt *monto_cubierto, BigIntArray *solucion);

/* A single denomination permits an exact large-range decision in constant
 * auxiliary space, unlike the general BigInt fallback with min>0. */
static int calcular_exacto_una_denominacion_bigint(
    const BigInt *monto, const BigIntArray *denom, const BigIntArray *stock,
    size_t min_monedas, size_t max_monedas, BigIntArray *solucion)
{
    BigInt cubierto = {0};
    int ok = calcular_cercano_una_denominacion_bigint(
        monto, denom, stock, min_monedas, max_monedas, &cubierto, solucion);
    if (ok && bigint_compare(&cubierto, monto) != 0)
    {
        bigint_array_free(solucion);
        ok = 0;
    }
    bigint_free(&cubierto);
    return ok;
}

/*
 * Exact O(n) solver for a single-coin constraint. Also computes the closest
 * nonzero amount representable with exactly one coin, without allocating
 * proportional to the BigInt amount.
 */
static int calcular_cambio_una_moneda(const BigInt *monto,
                                     const BigIntArray *denom,
                                     const BigIntArray *stock,
                                     int cercano,
                                     BigInt *cubierto,
                                     BigIntArray *solucion)
{
    size_t mejor;
    size_t i;
    BigInt uno = {0};
    BigInt cero = {0};

    if (monto == NULL || denom == NULL || denom->items == NULL ||
        denom->len == 0 || solucion == NULL ||
        (stock != NULL && (stock->items == NULL || stock->len != denom->len)))
        return 0;

    mejor = denom->len;
    for (i = 0; i < denom->len; ++i)
    {
        int cmp;
        if (bigint_is_zero(&denom->items[i]) ||
            (stock != NULL && bigint_is_zero(&stock->items[i])))
            continue;
        cmp = bigint_compare(&denom->items[i], monto);
        if (cmp > 0 || (!cercano && cmp != 0))
            continue;
        if (mejor == denom->len ||
            bigint_compare(&denom->items[i], &denom->items[mejor]) > 0)
            mejor = i;
    }

    if (mejor == denom->len ||
        !bigint_init(&uno, "1") || !bigint_init(&cero, "0") ||
        !bigint_array_create(solucion, denom->len))
        goto fallo;

    for (i = 0; i < denom->len; ++i)
    {
        if (!bigint_array_set(solucion, i, i == mejor ? &uno : &cero))
            goto fallo;
    }
    if (cubierto != NULL && !copiar_bigint(cubierto, &denom->items[mejor]))
        goto fallo;

    bigint_free(&uno);
    bigint_free(&cero);
    return 1;

fallo:
    bigint_free(&uno);
    bigint_free(&cero);
    bigint_array_free(solucion);
    return 0;
}


/* funcion calcular_cambio_optimo_con_rango: contiene la logica principal de esta operacion. */
int calcular_cambio_optimo_con_rango(const BigInt *monto,
                                     const BigIntArray *denominaciones,
                                     size_t min_monedas,
                                     size_t max_monedas,
                                     BigIntArray *solucion)
{
    int dp;

    /* if: comprueba monto == NULL || denominaciones == NULL || solucion == NULL antes de ejecutar esta rama. */
    if (monto == NULL || denominaciones == NULL || solucion == NULL)
        return 0;
    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
        return 0;

    if (min_monedas <= 1 && max_monedas == 1 && !bigint_is_zero(monto))
        return calcular_cambio_una_moneda(monto,denominaciones,NULL,0,NULL,solucion);

    if (denominaciones->len == 1)
    {
        size_t small = 0;
        if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &small))
            return calcular_exacto_una_denominacion_bigint(
                monto, denominaciones, NULL, min_monedas, max_monedas, solucion);
    }

    dp = calcular_dp_ilimitado_rango(monto, denominaciones, min_monedas, max_monedas, solucion);
    /* if: comprueba dp != DP_NO_APLICA antes de ejecutar esta rama. */
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (testigo_corto_bigint(monto,denominaciones,NULL,min_monedas,max_monedas,solucion)==1)
        return 1;

    /* A 5..8 piece witness remains exact for at most eight types. */
    if (denominaciones->len<=8 &&
        testigo_corto_bigint(monto,denominaciones,NULL,(min_monedas>5 ? min_monedas : 5),(max_monedas<8 ? max_monedas : 8),solucion)==1)
        return 1;

    /* 9..12 coins: only a small denomination alphabet is affordable. */
    if (denominaciones->len<=5 &&
        testigo_corto_bigint(monto,denominaciones,NULL,(min_monedas>9 ? min_monedas : 9),(max_monedas<12 ? max_monedas : 12),solucion)==1)
        return 1;

    /*
     * Para montos BigInt fuera del tope DP mantenemos una ruta compatible con el
     * comportamiento previo: solo se soporta restriccion de maximo (min=0).
     */
    /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
    if (min_monedas == 0)
    {
        /* if: comprueba !calcular_cambio_optimo(monto, denominaciones, solucion) antes de ejecutar esta rama. */
        if (!calcular_cambio_optimo(monto, denominaciones, solucion))
            return 0;

        /* if: comprueba !solucion_cumple_max_monedas(solucion, max_monedas) antes de ejecutar esta rama. */
        if (!solucion_cumple_max_monedas(solucion, max_monedas))
        {
            bigint_array_free(solucion);
            return 0;
        }

        return 1;
    }

    return 0;
}

/* funcion calcular_cambio_optimo_stock_con_rango: contiene la logica principal de esta operacion. */
int calcular_cambio_optimo_stock_con_rango(const BigInt *monto,
                                           const BigIntArray *denominaciones,
                                           const BigIntArray *stock,
                                           size_t min_monedas,
                                           size_t max_monedas,
                                           BigIntArray *solucion)
{
    int dp;

    /* if: comprueba monto == NULL || denominaciones == NULL || stock == NULL || solucion... antes de ejecutar esta rama. */
    if (monto == NULL || denominaciones == NULL || stock == NULL || solucion == NULL)
        return 0;
    /* if: comprueba denominaciones->len != stock->len antes de ejecutar esta rama. */
    if (denominaciones->len != stock->len)
        return 0;
    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
        return 0;

    if (min_monedas <= 1 && max_monedas == 1 && !bigint_is_zero(monto))
        return calcular_cambio_una_moneda(monto,denominaciones,stock,0,NULL,solucion);

    if (denominaciones->len == 1)
    {
        size_t small = 0;
        if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &small))
            return calcular_exacto_una_denominacion_bigint(
                monto, denominaciones, stock, min_monedas, max_monedas, solucion);
    }

    dp = calcular_dp_stock_rango(monto, denominaciones, stock, min_monedas, max_monedas, solucion);
    /* if: comprueba dp != DP_NO_APLICA antes de ejecutar esta rama. */
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (testigo_corto_bigint(monto,denominaciones,stock,min_monedas,max_monedas,solucion)==1)
        return 1;

    /* A 5..8 piece witness remains exact for at most eight types. */
    if (denominaciones->len<=8 &&
        testigo_corto_bigint(monto,denominaciones,stock,(min_monedas>5 ? min_monedas : 5),(max_monedas<8 ? max_monedas : 8),solucion)==1)
        return 1;

    /* 9..12 coins: only a small denomination alphabet is affordable. */
    if (denominaciones->len<=5 &&
        testigo_corto_bigint(monto,denominaciones,stock,(min_monedas>9 ? min_monedas : 9),(max_monedas<12 ? max_monedas : 12),solucion)==1)
        return 1;

    /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
    if (min_monedas == 0)
    {
        /* if: comprueba !calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion) antes de ejecutar esta rama. */
        if (!calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion))
            return 0;

        /* if: comprueba !solucion_cumple_max_monedas(solucion, max_monedas) antes de ejecutar esta rama. */
        if (!solucion_cumple_max_monedas(solucion, max_monedas))
        {
            bigint_array_free(solucion);
            return 0;
        }

        return 1;
    }

    return 0;
}

/*
 * One-denomination nearest change is solvable exactly with arbitrary-size
 * integers. This route prevents the large-amount fallback from ignoring
 * min/max coin limits or rejecting legitimate partial coverage.
 */
static int calcular_cercano_una_denominacion_bigint(
    const BigInt *monto, const BigIntArray *denom, const BigIntArray *stock,
    size_t min_monedas, size_t max_monedas,
    BigInt *monto_cubierto, BigIntArray *solucion)
{
    BigInt cantidad = {0}, resto = {0}, minimo = {0};
    BigInt maximo = {0}, cubierto = {0};
    int ok = 0;

    if (!monto || !denom || !denom->items || denom->len != 1 ||
        !denom->items[0].digits || bigint_is_zero(&denom->items[0]) ||
        !monto_cubierto || !solucion ||
        (stock && (!stock->items || stock->len != 1)))
        return 0;

    if (!bigint_divmod(monto, &denom->items[0], &cantidad, &resto))
        goto cleanup;
    if (stock && bigint_compare(&cantidad, &stock->items[0]) > 0 &&
        !bigint_set(&cantidad, &stock->items[0]))
        goto cleanup;
    if (max_monedas != (size_t)-1)
    {
        if (!size_a_bigint(max_monedas, &maximo))
            goto cleanup;
        if (bigint_compare(&cantidad, &maximo) > 0 &&
            !bigint_set(&cantidad, &maximo))
            goto cleanup;
    }
    if (!size_a_bigint(min_monedas, &minimo) ||
        bigint_is_zero(&cantidad) ||
        bigint_compare(&cantidad, &minimo) < 0 ||
        !bigint_multiply(&cantidad, &denom->items[0], &cubierto))
        goto cleanup;

    if (!bigint_array_create(solucion, 1))
        goto cleanup;
    if (!bigint_array_set(solucion, 0, &cantidad) ||
        !bigint_set(monto_cubierto, &cubierto))
    {
        bigint_array_free(solucion);
        goto cleanup;
    }
    ok = 1;

cleanup:
    bigint_free(&cantidad);
    bigint_free(&resto);
    bigint_free(&minimo);
    bigint_free(&maximo);
    bigint_free(&cubierto);
    return ok;
}

/* funcion calcular_cambio_cercano_con_rango: contiene la logica principal de esta operacion. */
int calcular_cambio_cercano_con_rango(const BigInt *monto,
                                      const BigIntArray *denominaciones,
                                      size_t min_monedas,
                                      size_t max_monedas,
                                      BigInt *monto_cubierto,
                                      BigIntArray *solucion)
{
    int dp;
    size_t cubierto_size = 0;

    /* if: comprueba monto == NULL || denominaciones == NULL || monto_cubierto == NULL ||... antes de ejecutar esta rama. */
    if (monto == NULL || denominaciones == NULL || monto_cubierto == NULL || solucion == NULL)
        return 0;
    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
        return 0;

    if (min_monedas <= 1 && max_monedas == 1)
        return calcular_cambio_una_moneda(monto,denominaciones,NULL,1,monto_cubierto,solucion);

    if (denominaciones->len == 1)
    {
        size_t small_amount = 0;
        if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &small_amount))
            return calcular_cercano_una_denominacion_bigint(
                monto, denominaciones, NULL, min_monedas, max_monedas,
                monto_cubierto, solucion);
    }

    if (denominaciones->len == 2 && min_monedas == 0 &&
        max_monedas == SIZE_MAX)
    {
        size_t small_amount=0;
        if (!bigint_a_size_limitado(monto,DP_MONTO_MAX,&small_amount))
        {
            int two=cercano_dos_denominaciones_bigint(monto,denominaciones,
                                                      monto_cubierto,solucion);
            if (two>=0) return two;
        }
    }

    dp = calcular_dp_ilimitado_cercano_rango(monto, denominaciones, min_monedas, max_monedas, &cubierto_size, solucion);
    /* if: comprueba dp != DP_NO_APLICA antes de ejecutar esta rama. */
    if (dp != DP_NO_APLICA)
    {
        /* if: comprueba dp != DP_OK antes de ejecutar esta rama. */
        if (dp != DP_OK)
            return 0;

        return size_a_bigint(cubierto_size, monto_cubierto);
    }

    /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
    if (min_monedas == 0)
    {
        /* if: comprueba !calcular_cambio_optimo(monto, denominaciones, solucion) antes de ejecutar esta rama. */
        if (!calcular_cambio_optimo(monto, denominaciones, solucion))
            return 0;

        // A BigInt fallback must not violate the requested maximum.
        if (!solucion_cumple_max_monedas(solucion, max_monedas) ||
            !copiar_bigint(monto_cubierto, monto))
        {
            bigint_array_free(solucion);
            return 0;
        }
        return 1;
    }

    return 0;
}

/* funcion calcular_cambio_cercano_stock_con_rango: contiene la logica principal de esta operacion. */
int calcular_cambio_cercano_stock_con_rango(const BigInt *monto,
                                            const BigIntArray *denominaciones,
                                            const BigIntArray *stock,
                                            size_t min_monedas,
                                            size_t max_monedas,
                                            BigInt *monto_cubierto,
                                            BigIntArray *solucion)
{
    int dp;
    size_t cubierto_size = 0;

    /* if: comprueba monto == NULL || denominaciones == NULL || stock == NULL || monto_cub... antes de ejecutar esta rama. */
    if (monto == NULL || denominaciones == NULL || stock == NULL || monto_cubierto == NULL || solucion == NULL)
        return 0;
    /* if: comprueba denominaciones->len != stock->len antes de ejecutar esta rama. */
    if (denominaciones->len != stock->len)
        return 0;
    /* if: comprueba min_monedas > max_monedas antes de ejecutar esta rama. */
    if (min_monedas > max_monedas)
        return 0;

    if (min_monedas <= 1 && max_monedas == 1)
        return calcular_cambio_una_moneda(monto,denominaciones,stock,1,monto_cubierto,solucion);

    if (denominaciones->len == 1)
    {
        size_t small_amount = 0;
        if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &small_amount))
            return calcular_cercano_una_denominacion_bigint(
                monto, denominaciones, stock, min_monedas, max_monedas,
                monto_cubierto, solucion);
    }

    if (denominaciones->len == 2 && min_monedas == 0 &&
        max_monedas == SIZE_MAX)
    {
        size_t small_amount = 0;
        if (!bigint_a_size_limitado(monto,DP_MONTO_MAX,&small_amount))
        {
            int exact = cercano_dos_denominaciones_stock_size(
                monto,denominaciones,stock,monto_cubierto,solucion);
            if (exact >= 0)
                return exact;
        }
    }

    dp = calcular_dp_stock_cercano_rango(monto, denominaciones, stock, min_monedas, max_monedas, &cubierto_size, solucion);
    /* if: comprueba dp != DP_NO_APLICA antes de ejecutar esta rama. */
    if (dp != DP_NO_APLICA)
    {
        /* if: comprueba dp != DP_OK antes de ejecutar esta rama. */
        if (dp != DP_OK)
            return 0;

        return size_a_bigint(cubierto_size, monto_cubierto);
    }

    /* if: comprueba min_monedas == 0 antes de ejecutar esta rama. */
    if (min_monedas == 0)
    {
        /* if: comprueba !calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion) antes de ejecutar esta rama. */
        if (!calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion))
            return 0;

        // A BigInt fallback must not violate the requested maximum.
        if (!solucion_cumple_max_monedas(solucion, max_monedas) ||
            !copiar_bigint(monto_cubierto, monto))
        {
            bigint_array_free(solucion);
            return 0;
        }
        return 1;
    }

    return 0;
}

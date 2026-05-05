#include "algoritmo_cambio.h"
#include <stdio.h>
#include <stdlib.h>

#define DP_MONTO_MAX ((size_t)50000)
#define DP_NO_APLICA (-1)
#define DP_SIN_SOLUCION 0
#define DP_OK 1

static int bigint_a_size_limitado(const BigInt *n, size_t limite, size_t *valor_out)
{
    size_t valor = 0;
    const char *p;

    if (n == NULL || n->digits == NULL || valor_out == NULL)
        return 0;

    for (p = n->digits; *p != '\0'; p++)
    {
        unsigned int digito;

        if (*p < '0' || *p > '9')
            return 0;

        digito = (unsigned int)(*p - '0');
        if (valor > limite / 10 || (valor == limite / 10 && digito > limite % 10))
            return 0;

        valor = valor * 10 + digito;
    }

    *valor_out = valor;
    return 1;
}

static int bigint_a_size_con_tope(const BigInt *n, size_t tope, size_t *valor_out)
{
    size_t valor = 0;
    const char *p;

    if (n == NULL || n->digits == NULL || valor_out == NULL)
        return 0;

    for (p = n->digits; *p != '\0'; p++)
    {
        unsigned int digito;

        if (*p < '0' || *p > '9')
            return 0;

        digito = (unsigned int)(*p - '0');
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

static int size_a_bigint(size_t valor, BigInt *out)
{
    char texto[32];

    if (out == NULL)
        return 0;

    snprintf(texto, sizeof(texto), "%zu", valor);
    return bigint_init(out, texto);
}

static int cargar_solucion_size(const size_t *cantidades, size_t len, BigIntArray *solucion)
{
    size_t i;

    if (cantidades == NULL || solucion == NULL || len == 0)
        return 0;

    if (!bigint_array_create(solucion, len))
        return 0;

    for (i = 0; i < len; i++)
    {
        BigInt tmp = {0};

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

    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    dp = (size_t *)malloc((amount + 1) * sizeof(size_t));
    previo = (size_t *)malloc((amount + 1) * sizeof(size_t));
    moneda = (size_t *)malloc((amount + 1) * sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));

    if (valores == NULL || dp == NULL || previo == NULL || moneda == NULL || cantidades == NULL)
        goto cleanup;

    for (i = 0; i < denom->len; i++)
    {
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]))
            goto cleanup;
    }

    for (i = 0; i <= amount; i++)
    {
        dp[i] = inf;
        previo[i] = 0;
        moneda[i] = denom->len;
    }

    dp[0] = 0;

    for (i = 1; i <= amount; i++)
    {
        size_t j;

        for (j = 0; j < denom->len; j++)
        {
            size_t valor = valores[j];

            if (valor == 0 || valor > i || dp[i - valor] == inf)
                continue;

            if (dp[i - valor] + 1 < dp[i])
            {
                dp[i] = dp[i - valor] + 1;
                previo[i] = i - valor;
                moneda[i] = j;
            }
        }
    }

    if (dp[amount] == inf)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    for (i = amount; i > 0;)
    {
        size_t idx = moneda[i];

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

    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

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

    if (valores == NULL || topes == NULL || prev_dp == NULL || curr_dp == NULL ||
        choice == NULL || cantidades == NULL || cola_pos == NULL || cola_score == NULL)
        goto cleanup;

    for (i = 0; i < denom->len; i++)
    {
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) ||
            !bigint_a_size_con_tope(&stock->items[i], amount, &topes[i]))
        {
            goto cleanup;
        }
    }

    for (i = 0; i <= amount; i++)
        prev_dp[i] = inf;
    prev_dp[0] = 0;

    for (i = 0; i < denom->len; i++)
    {
        size_t valor = valores[i];
        size_t max_usos = 0;
        size_t v;

        for (v = 0; v <= amount; v++)
            curr_dp[v] = inf;

        if (valor > 0 && valor <= amount)
        {
            max_usos = amount / valor;
            if (topes[i] < max_usos)
                max_usos = topes[i];
        }

        if (max_usos == 0)
        {
            for (v = 0; v <= amount; v++)
                curr_dp[v] = prev_dp[v];
        }
        else
        {
            size_t residuo;

            /* Cola monotona por residuo: evita iterar una vez por cada unidad de stock. */
            for (residuo = 0; residuo < valor && residuo <= amount; residuo++)
            {
                size_t cabeza = 0;
                size_t cola = 0;
                size_t m = 0;

                for (v = residuo; v <= amount; v += valor, m++)
                {
                    if (prev_dp[v] != inf)
                    {
                        long long score = (long long)prev_dp[v] - (long long)m;

                        while (cola > cabeza && cola_score[cola - 1] >= score)
                            cola--;

                        cola_pos[cola] = m;
                        cola_score[cola] = score;
                        cola++;
                    }

                    while (cola > cabeza && cola_pos[cabeza] + max_usos < m)
                        cabeza++;

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

    if (prev_dp[amount] == inf)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    for (i = denom->len; i > 0; i--)
    {
        size_t idx = i - 1;
        size_t usos = choice[idx * (amount + 1) + amount];

        cantidades[idx] = usos;
        amount -= usos * valores[idx];
    }

    if (amount != 0)
        goto cleanup;

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

    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    if (valores == NULL || cantidades == NULL)
        goto cleanup;

    for (i = 0; i < denom->len; i++)
    {
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]))
            goto cleanup;

        if (valores[i] > 0 && valores[i] < min_valor)
            min_valor = valores[i];
    }

    if (amount == 0)
    {
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

    if (min_valor == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t tope_natural = amount / min_valor;
        if (max_monedas > tope_natural)
            max_monedas = tope_natural;
    }

    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    moneda = (size_t *)malloc(estados * sizeof(size_t));
    if (alcanzable == NULL || prev_monto == NULL || moneda == NULL)
        goto cleanup;

    for (i = 0; i < estados; i++)
    {
        prev_monto[i] = 0;
        moneda[i] = denom->len;
    }

    alcanzable[0] = 1;
    for (k = 1; k <= max_monedas; k++)
    {
        size_t monto_k;

        for (monto_k = 0; monto_k <= amount; monto_k++)
        {
            size_t j;
            size_t idx = k * stride + monto_k;

            for (j = 0; j < denom->len; j++)
            {
                size_t valor = valores[j];
                size_t idx_prev;

                if (valor == 0 || valor > monto_k)
                    continue;

                idx_prev = (k - 1) * stride + (monto_k - valor);
                if (!alcanzable[idx_prev])
                    continue;

                alcanzable[idx] = 1;
                prev_monto[idx] = monto_k - valor;
                moneda[idx] = j;
                break;
            }
        }
    }

    for (k = min_monedas; k <= max_monedas; k++)
    {
        if (alcanzable[k * stride + amount])
        {
            objetivo_k = k;
            break;
        }
    }

    if (objetivo_k == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t monto_actual = amount;
        size_t k_actual = objetivo_k;

        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t j = moneda[idx];

            if (j >= denom->len || prev_monto[idx] > monto_actual)
                goto cleanup;

            cantidades[j]++;
            monto_actual = prev_monto[idx];
            k_actual--;
        }

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

static int push_item_stock(DPItemStock **items, size_t *len, size_t *cap, size_t idx_denom, size_t monedas, size_t valor)
{
    DPItemStock *nuevo;

    if (items == NULL || len == NULL || cap == NULL)
        return 0;

    if (*len >= *cap)
    {
        size_t nueva_cap = (*cap == 0) ? 16 : (*cap * 2);
        if (nueva_cap < *cap)
            return 0;
        nuevo = (DPItemStock *)realloc(*items, nueva_cap * sizeof(DPItemStock));
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

    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    topes = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    if (valores == NULL || topes == NULL || cantidades == NULL)
        goto cleanup;

    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;

        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) ||
            !bigint_a_size_con_tope(&stock->items[i], amount, &topes[i]))
        {
            goto cleanup;
        }

        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        if (valores[i] > 0)
        {
            size_t cap_por_monto = amount / valores[i];
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }

        tope_monedas += cap_uso;
    }

    if (amount == 0)
    {
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

    if (max_monedas > tope_monedas)
        max_monedas = tope_monedas;

    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;
        size_t chunk = 1;

        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        {
            size_t cap_por_monto = amount / valores[i];
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }
        if (cap_uso > max_monedas)
            cap_uso = max_monedas;

        while (cap_uso > 0)
        {
            size_t take = (chunk < cap_uso) ? chunk : cap_uso;
            size_t valor_item = take * valores[i];

            if (!push_item_stock(&items, &items_len, &items_cap, i, take, valor_item))
                goto cleanup;

            cap_uso -= take;
            if (chunk <= ((size_t)-1) / 2)
                chunk *= 2;
            else
                chunk = cap_uso;
        }
    }

    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_k = (size_t *)malloc(estados * sizeof(size_t));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    item_usado = (size_t *)malloc(estados * sizeof(size_t));
    if (alcanzable == NULL || prev_k == NULL || prev_monto == NULL || item_usado == NULL)
        goto cleanup;

    for (i = 0; i < estados; i++)
    {
        prev_k[i] = invalid;
        prev_monto[i] = 0;
        item_usado[i] = invalid;
    }

    alcanzable[0] = 1;
    for (i = 0; i < items_len; i++)
    {
        size_t k;
        size_t valor_item = items[i].valor;
        size_t monedas_item = items[i].monedas;

        for (k = max_monedas + 1; k > monedas_item;)
        {
            size_t kk = k - 1;
            size_t m;

            for (m = amount + 1; m > valor_item;)
            {
                size_t mm = m - 1;
                size_t idx = kk * stride + mm;
                size_t idx_prev = (kk - monedas_item) * stride + (mm - valor_item);

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

    for (i = min_monedas; i <= max_monedas; i++)
    {
        if (alcanzable[i * stride + amount])
        {
            objetivo_k = i;
            break;
        }
    }

    if (objetivo_k == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t k_actual = objetivo_k;
        size_t monto_actual = amount;

        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t idx_item = item_usado[idx];

            if (idx_item == invalid || idx_item >= items_len)
                goto cleanup;

            cantidades[items[idx_item].idx_denom] += items[idx_item].monedas;
            monto_actual = prev_monto[idx];
            k_actual = prev_k[idx];
        }

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

    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    if (monto_cubierto == NULL)
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    if (valores == NULL || cantidades == NULL)
        goto cleanup;

    for (i = 0; i < denom->len; i++)
    {
        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]))
            goto cleanup;

        if (valores[i] > 0 && valores[i] < min_valor)
            min_valor = valores[i];
    }

    if (amount == 0)
    {
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

    if (min_valor == (size_t)-1)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t tope_natural = amount / min_valor;
        if (max_monedas > tope_natural)
            max_monedas = tope_natural;
    }

    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    moneda = (size_t *)malloc(estados * sizeof(size_t));
    if (alcanzable == NULL || prev_monto == NULL || moneda == NULL)
        goto cleanup;

    for (i = 0; i < estados; i++)
    {
        prev_monto[i] = 0;
        moneda[i] = denom->len;
    }

    alcanzable[0] = 1;
    for (k = 1; k <= max_monedas; k++)
    {
        size_t monto_k;

        for (monto_k = 0; monto_k <= amount; monto_k++)
        {
            size_t j;
            size_t idx = k * stride + monto_k;

            for (j = 0; j < denom->len; j++)
            {
                size_t valor = valores[j];
                size_t idx_prev;

                if (valor == 0 || valor > monto_k)
                    continue;

                idx_prev = (k - 1) * stride + (monto_k - valor);
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

        while (1)
        {
            for (k = min_monedas; k <= max_monedas; k++)
            {
                if (alcanzable[k * stride + monto_busqueda])
                {
                    objetivo_k = k;
                    objetivo_monto = monto_busqueda;
                    encontrado = 1;
                    break;
                }
            }

            if (encontrado)
                break;

            if (monto_busqueda == 0)
                break;

            monto_busqueda--;
        }
    }

    if (objetivo_k == (size_t)-1 || objetivo_monto == (size_t)-1 || objetivo_monto == 0)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t monto_actual = objetivo_monto;
        size_t k_actual = objetivo_k;

        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t j = moneda[idx];

            if (j >= denom->len || prev_monto[idx] > monto_actual)
                goto cleanup;

            cantidades[j]++;
            monto_actual = prev_monto[idx];
            k_actual--;
        }

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

    if (!bigint_a_size_limitado(monto, DP_MONTO_MAX, &amount))
        return DP_NO_APLICA;

    if (monto_cubierto == NULL)
        return DP_NO_APLICA;

    valores = (size_t *)calloc(denom->len, sizeof(size_t));
    topes = (size_t *)calloc(denom->len, sizeof(size_t));
    cantidades = (size_t *)calloc(denom->len, sizeof(size_t));
    if (valores == NULL || topes == NULL || cantidades == NULL)
        goto cleanup;

    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;

        if (!bigint_a_size_con_tope(&denom->items[i], amount + 1, &valores[i]) ||
            !bigint_a_size_con_tope(&stock->items[i], amount, &topes[i]))
        {
            goto cleanup;
        }

        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        {
            size_t cap_por_monto = amount / valores[i];
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }

        tope_monedas += cap_uso;
    }

    if (amount == 0)
    {
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

    if (max_monedas > tope_monedas)
        max_monedas = tope_monedas;

    if (min_monedas > max_monedas)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    for (i = 0; i < denom->len; i++)
    {
        size_t cap_uso;
        size_t chunk = 1;

        if (valores[i] == 0)
            continue;

        cap_uso = topes[i];
        {
            size_t cap_por_monto = amount / valores[i];
            if (cap_por_monto < cap_uso)
                cap_uso = cap_por_monto;
        }
        if (cap_uso > max_monedas)
            cap_uso = max_monedas;

        while (cap_uso > 0)
        {
            size_t take = (chunk < cap_uso) ? chunk : cap_uso;
            size_t valor_item = take * valores[i];

            if (!push_item_stock(&items, &items_len, &items_cap, i, take, valor_item))
                goto cleanup;

            cap_uso -= take;
            if (chunk <= ((size_t)-1) / 2)
                chunk *= 2;
            else
                chunk = cap_uso;
        }
    }

    if (max_monedas > ((size_t)-1) / (amount + 1) - 1)
        goto cleanup;

    stride = amount + 1;
    estados = (max_monedas + 1) * stride;

    alcanzable = (unsigned char *)calloc(estados, sizeof(unsigned char));
    prev_k = (size_t *)malloc(estados * sizeof(size_t));
    prev_monto = (size_t *)malloc(estados * sizeof(size_t));
    item_usado = (size_t *)malloc(estados * sizeof(size_t));
    if (alcanzable == NULL || prev_k == NULL || prev_monto == NULL || item_usado == NULL)
        goto cleanup;

    for (i = 0; i < estados; i++)
    {
        prev_k[i] = invalid;
        prev_monto[i] = 0;
        item_usado[i] = invalid;
    }

    alcanzable[0] = 1;
    for (i = 0; i < items_len; i++)
    {
        size_t k;
        size_t valor_item = items[i].valor;
        size_t monedas_item = items[i].monedas;

        for (k = max_monedas + 1; k > monedas_item;)
        {
            size_t kk = k - 1;
            size_t m;

            for (m = amount + 1; m > valor_item;)
            {
                size_t mm = m - 1;
                size_t idx = kk * stride + mm;
                size_t idx_prev = (kk - monedas_item) * stride + (mm - valor_item);

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

        while (1)
        {
            for (i = min_monedas; i <= max_monedas; i++)
            {
                if (alcanzable[i * stride + monto_busqueda])
                {
                    objetivo_k = i;
                    objetivo_monto = monto_busqueda;
                    encontrado = 1;
                    break;
                }
            }

            if (encontrado)
                break;

            if (monto_busqueda == 0)
                break;

            monto_busqueda--;
        }
    }

    if (objetivo_k == (size_t)-1 || objetivo_monto == (size_t)-1 || objetivo_monto == 0)
    {
        resultado = DP_SIN_SOLUCION;
        goto cleanup;
    }

    {
        size_t k_actual = objetivo_k;
        size_t monto_actual = objetivo_monto;

        while (k_actual > 0)
        {
            size_t idx = k_actual * stride + monto_actual;
            size_t idx_item = item_usado[idx];

            if (idx_item == invalid || idx_item >= items_len)
                goto cleanup;

            cantidades[items[idx_item].idx_denom] += items[idx_item].monedas;
            monto_actual = prev_monto[idx];
            k_actual = prev_k[idx];
        }

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

static int solucion_cumple_max_monedas(const BigIntArray *solucion, size_t max_monedas)
{
    size_t total = 0;
    size_t i;

    if (solucion == NULL || solucion->items == NULL)
        return 0;

    for (i = 0; i < solucion->len; i++)
    {
        size_t valor;

        if (!bigint_a_size_limitado(&solucion->items[i], max_monedas - total, &valor))
            return 0;

        total += valor;
    }

    return total <= max_monedas;
}

static int copiar_bigint(BigInt *dest, const BigInt *src)
{
    if (dest == NULL || src == NULL || src->digits == NULL)
        return 0;

    if (bigint_set(dest, src))
        return 1;

    return bigint_init(dest, src->digits);
}

static int construir_sufijos_stock(const BigIntArray *denom,
                                   const BigIntArray *stock,
                                   BigInt **sufijos_out)
{
    BigInt *sufijos;
    size_t i;

    if (denom == NULL || stock == NULL || sufijos_out == NULL || denom->len != stock->len)
        return 0;

    sufijos = (BigInt *)calloc(denom->len + 1, sizeof(BigInt));
    if (sufijos == NULL)
        return 0;

    if (!bigint_init(&sufijos[denom->len], "0"))
    {
        free(sufijos);
        return 0;
    }

    for (i = denom->len; i > 0; i--)
    {
        BigInt aporte = {0};
        BigInt acumulado = {0};

        if (!bigint_multiply(&denom->items[i - 1], &stock->items[i - 1], &aporte) ||
            !bigint_add(&aporte, &sufijos[i], &acumulado) ||
            !copiar_bigint(&sufijos[i - 1], &acumulado))
        {
            size_t j;
            bigint_free(&aporte);
            bigint_free(&acumulado);
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

static void liberar_sufijos_stock(BigInt *sufijos, size_t len)
{
    size_t i;

    if (sufijos == NULL)
        return;

    for (i = 0; i <= len; i++)
        bigint_free(&sufijos[i]);
    free(sufijos);
}

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

    if (bigint_is_zero(restante))
        return 1;
    if (index >= denom->len)
        return 0;

    if (bigint_is_zero(&denom->items[index]))
    {
        if (!bigint_array_set(solucion, index, zero))
            return 0;
        return backtrack_ilimitado(denom, one, zero, restante, solucion, index + 1);
    }

    if (!bigint_divmod(restante, &denom->items[index], &max_usable, &residuo) ||
        !copiar_bigint(&cuenta, &max_usable) ||
        !bigint_multiply(&denom->items[index], &cuenta, &uso) ||
        !bigint_subtract(restante, &uso, &actual_restante))
        goto cleanup;

    while (1)
    {
        if (!bigint_array_set(solucion, index, &cuenta))
            goto cleanup;

        if (backtrack_ilimitado(denom, one, zero, &actual_restante, solucion, index + 1))
        {
            ok = 1;
            goto cleanup;
        }

        if (bigint_compare(&cuenta, zero) == 0)
            break;

        {
            BigInt sig_cuenta = {0};
            BigInt sig_restante = {0};

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

    if (bigint_is_zero(restante))
        return 1;
    if (index >= denom->len)
        return 0;
    if (sufijos != NULL && index < sufijos->len && bigint_compare(restante, &sufijos->items[index]) > 0)
        return 0;

    if (bigint_is_zero(&denom->items[index]))
    {
        if (!bigint_array_set(solucion, index, zero))
            return 0;
        return backtrack_stock(denom, stock, sufijos, one, zero, restante, solucion, index + 1);
    }

    if (!bigint_divmod(restante, &denom->items[index], &max_usable, &residuo))
        goto cleanup;

    if (bigint_compare(&max_usable, &stock->items[index]) > 0)
    {
        if (!copiar_bigint(&cuenta, &stock->items[index]))
            goto cleanup;
    }
    else
    {
        if (!copiar_bigint(&cuenta, &max_usable))
            goto cleanup;
    }

    if (!bigint_multiply(&denom->items[index], &cuenta, &uso) ||
        !bigint_subtract(restante, &uso, &actual_restante))
        goto cleanup;

    while (1)
    {
        if (!bigint_array_set(solucion, index, &cuenta))
            goto cleanup;

        if (backtrack_stock(denom, stock, sufijos, one, zero, &actual_restante, solucion, index + 1))
        {
            ok = 1;
            goto cleanup;
        }

        if (bigint_compare(&cuenta, zero) == 0)
            break;

        {
            BigInt sig_cuenta = {0};
            BigInt sig_restante = {0};

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

int calcular_cambio_optimo(const BigInt *monto, const BigIntArray *denominaciones, BigIntArray *solucion)
{
    BigInt restante = {0};
    BigInt one = {0};
    BigInt zero = {0};
    int dp;
    int res;

    if (monto == NULL || denominaciones == NULL || solucion == NULL)
        return 0;

    dp = calcular_dp_ilimitado(monto, denominaciones, solucion);
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;
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
    if (!res)
        bigint_array_free(solucion);
    return res;
}

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

    if (monto == NULL || denominaciones == NULL || stock == NULL || solucion == NULL)
        return 0;
    if (denominaciones->len != stock->len)
        return 0;

    dp = calcular_dp_stock(monto, denominaciones, stock, solucion);
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (!bigint_array_create(solucion, denominaciones->len))
        return 0;

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

    if (!res)
        bigint_array_free(solucion);
    return res;
}

int calcular_cambio_optimo_stock_con_limite(const BigInt *monto, const BigIntArray *denominaciones, const BigIntArray *stock, size_t limite_monedas, BigIntArray *solucion)
{
    if (limite_monedas == 0)
        return calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion);

    return calcular_cambio_optimo_stock_con_rango(monto, denominaciones, stock, 0, limite_monedas, solucion);
}

int calcular_cambio_optimo_con_limite(const BigInt *monto, const BigIntArray *denominaciones, size_t limite_monedas, BigIntArray *solucion)
{
    if (limite_monedas == 0)
        return calcular_cambio_optimo(monto, denominaciones, solucion);

    return calcular_cambio_optimo_con_rango(monto, denominaciones, 0, limite_monedas, solucion);
}

int calcular_cambio_optimo_con_rango(const BigInt *monto,
                                     const BigIntArray *denominaciones,
                                     size_t min_monedas,
                                     size_t max_monedas,
                                     BigIntArray *solucion)
{
    int dp;

    if (monto == NULL || denominaciones == NULL || solucion == NULL)
        return 0;
    if (min_monedas > max_monedas)
        return 0;

    dp = calcular_dp_ilimitado_rango(monto, denominaciones, min_monedas, max_monedas, solucion);
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    /*
     * Para montos BigInt fuera del tope DP mantenemos una ruta compatible con el
     * comportamiento previo: solo se soporta restriccion de maximo (min=0).
     */
    if (min_monedas == 0)
    {
        if (!calcular_cambio_optimo(monto, denominaciones, solucion))
            return 0;

        if (!solucion_cumple_max_monedas(solucion, max_monedas))
        {
            bigint_array_free(solucion);
            return 0;
        }

        return 1;
    }

    return 0;
}

int calcular_cambio_optimo_stock_con_rango(const BigInt *monto,
                                           const BigIntArray *denominaciones,
                                           const BigIntArray *stock,
                                           size_t min_monedas,
                                           size_t max_monedas,
                                           BigIntArray *solucion)
{
    int dp;

    if (monto == NULL || denominaciones == NULL || stock == NULL || solucion == NULL)
        return 0;
    if (denominaciones->len != stock->len)
        return 0;
    if (min_monedas > max_monedas)
        return 0;

    dp = calcular_dp_stock_rango(monto, denominaciones, stock, min_monedas, max_monedas, solucion);
    if (dp != DP_NO_APLICA)
        return dp == DP_OK;

    if (min_monedas == 0)
    {
        if (!calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion))
            return 0;

        if (!solucion_cumple_max_monedas(solucion, max_monedas))
        {
            bigint_array_free(solucion);
            return 0;
        }

        return 1;
    }

    return 0;
}

int calcular_cambio_cercano_con_rango(const BigInt *monto,
                                      const BigIntArray *denominaciones,
                                      size_t min_monedas,
                                      size_t max_monedas,
                                      BigInt *monto_cubierto,
                                      BigIntArray *solucion)
{
    int dp;
    size_t cubierto_size = 0;

    if (monto == NULL || denominaciones == NULL || monto_cubierto == NULL || solucion == NULL)
        return 0;
    if (min_monedas > max_monedas)
        return 0;

    dp = calcular_dp_ilimitado_cercano_rango(monto, denominaciones, min_monedas, max_monedas, &cubierto_size, solucion);
    if (dp != DP_NO_APLICA)
    {
        if (dp != DP_OK)
            return 0;

        return size_a_bigint(cubierto_size, monto_cubierto);
    }

    if (min_monedas == 0)
    {
        if (!calcular_cambio_optimo(monto, denominaciones, solucion))
            return 0;

        return copiar_bigint(monto_cubierto, monto);
    }

    return 0;
}

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

    if (monto == NULL || denominaciones == NULL || stock == NULL || monto_cubierto == NULL || solucion == NULL)
        return 0;
    if (denominaciones->len != stock->len)
        return 0;
    if (min_monedas > max_monedas)
        return 0;

    dp = calcular_dp_stock_cercano_rango(monto, denominaciones, stock, min_monedas, max_monedas, &cubierto_size, solucion);
    if (dp != DP_NO_APLICA)
    {
        if (dp != DP_OK)
            return 0;

        return size_a_bigint(cubierto_size, monto_cubierto);
    }

    if (min_monedas == 0)
    {
        if (!calcular_cambio_optimo_stock(monto, denominaciones, stock, solucion))
            return 0;

        return copiar_bigint(monto_cubierto, monto);
    }

    return 0;
}

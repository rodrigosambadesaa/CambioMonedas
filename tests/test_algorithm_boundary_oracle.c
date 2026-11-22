#include "algoritmo_cambio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failed = 0;
static unsigned long checked = 0;

static int init_text_array(BigIntArray *out, const char *const *words, size_t n) {
    if (!bigint_array_create(out, n)) return 0;
    for (size_t i = 0; i < n; ++i) {
        BigInt x = {0};
        int ok = bigint_init(&x, words[i]) && bigint_array_set(out, i, &x);
        bigint_free(&x);
        if (!ok) { bigint_array_free(out); return 0; }
    }
    return 1;
}

static int init_int_array(BigIntArray *out, const unsigned *values, size_t n) {
    char buf[32];
    if (!bigint_array_create(out, n)) return 0;
    for (size_t i = 0; i < n; ++i) {
        BigInt x = {0};
        snprintf(buf, sizeof buf, "%u", values[i]);
        int ok = bigint_init(&x, buf) && bigint_array_set(out, i, &x);
        bigint_free(&x);
        if (!ok) { bigint_array_free(out); return 0; }
    }
    return 1;
}

static void oracle(unsigned amount, const unsigned *d, const unsigned *stock,
                   int bounded, unsigned min, unsigned max, int nearest,
                   int *best_covered, int *best_count) {
    *best_covered = -1;
    *best_count = -1;
    unsigned lim0 = amount / d[0], lim1 = amount / d[1], lim2 = amount / d[2];
    if (bounded) {
        if (stock[0] < lim0) lim0 = stock[0];
        if (stock[1] < lim1) lim1 = stock[1];
        if (stock[2] < lim2) lim2 = stock[2];
    }
    for (unsigned a = 0; a <= lim0; ++a)
      for (unsigned b = 0; b <= lim1; ++b)
        for (unsigned c = 0; c <= lim2; ++c) {
            unsigned count = a + b + c;
            unsigned sum = a*d[0] + b*d[1] + c*d[2];
            if (sum > amount || count < min || count > max) continue;
            if (nearest ? sum == 0 : sum != amount) continue;
            if ((int)sum > *best_covered ||
                ((int)sum == *best_covered && (int)count < *best_count)) {
                *best_covered = (int)sum;
                *best_count = (int)count;
            }
        }
}

static void verify(const char *name, int rc, const BigIntArray *solution,
                   const BigInt *covered, int expected_covered, int expected_count,
                   const unsigned *d, const unsigned *stock, size_t n, int bounded) {
    ++checked;
    if (expected_covered < 0) {
        if (rc) {
            fprintf(stderr, "FAIL %s: unexpected solution\n", name);
            ++failed;
        }
        return;
    }
    if (!rc || !solution->items || solution->len != n) {
        fprintf(stderr, "FAIL %s: missing or malformed solution, rc=%d\n", name, rc);
        ++failed;
        return;
    }
    unsigned long long sum = 0, count = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!solution->items[i].digits) { ++failed; return; }
        unsigned long long num = strtoull(solution->items[i].digits, NULL, 10);
        count += num;
        sum += num * d[i];
        if (bounded && num > stock[i]) {
            fprintf(stderr, "FAIL %s: inventory exceeded\n", name);
            ++failed;
            return;
        }
    }
    if ((int)sum != expected_covered || (int)count != expected_count ||
        (covered && (!covered->digits || strtoull(covered->digits, NULL, 10) != sum))) {
        fprintf(stderr, "FAIL %s: expected amount=%d count=%d; actual amount=%llu count=%llu\n",
                name, expected_covered, expected_count, sum, count);
        ++failed;
    }
}

static void small_case(const BigIntArray *d, const BigIntArray *s,
                       const unsigned *values, const unsigned *stocks,
                       unsigned amount) {
    char buf[32], label[100];
    BigInt value = {0};
    snprintf(buf, sizeof buf, "%u", amount);
    if (!bigint_init(&value, buf)) { ++failed; return; }
    for (int mode = 0; mode < 6; ++mode) {
        int nearest = mode >= 4;
        if (nearest && amount == 0) continue;
        unsigned min = (mode == 2 || mode == 3) ? 1 : 0;
        unsigned max = mode >= 2 ? 4 : amount;
        int bounded = (mode % 2 == 1);
        int expected_value, expected_count;
        oracle(amount, values, stocks, bounded, min, max, nearest,
               &expected_value, &expected_count);
        BigIntArray solution = {0};
        BigInt covered = {0};
        int ok = 0;
        switch (mode) {
        case 0: ok = calcular_cambio_optimo(&value, d, &solution); break;
        case 1: ok = calcular_cambio_optimo_stock(&value, d, s, &solution); break;
        case 2: ok = calcular_cambio_optimo_con_rango(&value, d, min, max, &solution); break;
        case 3: ok = calcular_cambio_optimo_stock_con_rango(&value, d, s, min, max, &solution); break;
        case 4: ok = calcular_cambio_cercano_con_rango(&value, d, min, max, &covered, &solution); break;
        case 5: ok = calcular_cambio_cercano_stock_con_rango(&value, d, s, min, max, &covered, &solution); break;
        }
        snprintf(label, sizeof label, "small mode=%d denom=%u,%u,%u amount=%u",
                 mode, values[0], values[1], values[2], amount);
        verify(label, ok, &solution, nearest ? &covered : NULL,
               expected_value, expected_count, values, stocks, 3, bounded);
        bigint_free(&covered);
        bigint_array_free(&solution);
    }
    bigint_free(&value);
}

static void exhaustive_small_oracle(void) {
    const unsigned stocks[3] = {2, 3, 4};
    BigIntArray stock = {0};
    if (!init_int_array(&stock, stocks, 3)) { ++failed; return; }
    for (unsigned a=1; a<=5; ++a) for (unsigned b=1; b<=5; ++b)
      for (unsigned c=1; c<=5; ++c) {
        unsigned denoms[3] = {a, b, c};
        BigIntArray d = {0};
        if (!init_int_array(&d, denoms, 3)) { ++failed; continue; }
        for (unsigned amount=0; amount<=18; ++amount)
            small_case(&d, &stock, denoms, stocks, amount);
        bigint_array_free(&d);
      }
    bigint_array_free(&stock);
}

static void boundary(const char *name, int mode, const char *amount_text,
                     const char *const *d_text, const char *const *s_text, size_t n,
                     unsigned min, unsigned max,
                     int expected_covered, int expected_count) {
    BigInt amount = {0}, covered = {0};
    BigIntArray d = {0}, s = {0}, sol = {0};
    unsigned dval[3] = {0}, sval[3] = {0};
    int ok = 0, bounded = mode == 1 || mode == 3 || mode == 5;
    for (size_t i=0; i<n; ++i) {
        dval[i] = (unsigned)strtoul(d_text[i], NULL, 10);
        sval[i] = s_text ? (unsigned)strtoul(s_text[i], NULL, 10) : 0;
    }
    if (!bigint_init(&amount, amount_text) ||
        !init_text_array(&d, d_text, n) ||
        (bounded && !init_text_array(&s, s_text, n))) {
        fprintf(stderr, "FAIL %s: setup\n", name);
        ++failed;
        goto cleanup;
    }
    switch (mode) {
    case 0: ok = calcular_cambio_optimo(&amount, &d, &sol); break;
    case 1: ok = calcular_cambio_optimo_stock(&amount, &d, &s, &sol); break;
    case 2: ok = calcular_cambio_optimo_con_rango(&amount, &d, min, max, &sol); break;
    case 3: ok = calcular_cambio_optimo_stock_con_rango(&amount, &d, &s, min, max, &sol); break;
    case 4: ok = calcular_cambio_cercano_con_rango(&amount, &d, min, max, &covered, &sol); break;
    case 5: ok = calcular_cambio_cercano_stock_con_rango(&amount, &d, &s, min, max, &covered, &sol); break;
    }
    verify(name, ok, &sol, mode >= 4 ? &covered : NULL,
           expected_covered, expected_count, dval, sval, n, bounded);
cleanup:
    bigint_array_free(&sol);
    bigint_array_free(&d);
    bigint_array_free(&s);
    bigint_free(&amount);
    bigint_free(&covered);
}

int main(void) {
    const char *d431[] = {"4", "3", "1"};
    const char *s431[] = {"20000", "20000", "20000"};
    const char *d1[] = {"1"};
    const char *s1[] = {"50001"};
    const char *d50000[] = {"50000"};
    const char *s50000[] = {"1"};
    const char *d50001[] = {"50001"};
    const char *s50001[] = {"1"};
    exhaustive_small_oracle();
    boundary("optimal above DP threshold", 0, "50002", d431, NULL, 3, 0, 0, 50002, 12501);
    boundary("stock optimal above DP threshold", 1, "50002", d431, s431, 3, 0, 0, 50002, 12501);
    boundary("exact positive minimum above threshold", 2, "50001", d50001, NULL, 1, 1, 1, 50001, 1);
    boundary("stock exact positive minimum above threshold", 3, "50001", d50001, s50001, 1, 1, 1, 50001, 1);
    boundary("near above threshold must enforce max", 4, "50001", d1, NULL, 1, 0, 1, 1, 1);
    boundary("stock near above threshold must enforce max", 5, "50001", d1, s1, 1, 0, 1, 1, 1);
    boundary("near above threshold should find partial", 4, "50001", d50000, NULL, 1, 0, 1, 50000, 1);
    boundary("stock near above threshold should find partial", 5, "50001", d50000, s50000, 1, 0, 1, 50000, 1);
    fprintf(stderr, "Boundary audit: checked %lu scenarios, failures=%d\n", checked, failed);
    return failed ? 1 : 0;
}

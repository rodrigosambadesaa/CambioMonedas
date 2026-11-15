#ifndef EXCHANGE_API_H
#define EXCHANGE_API_H

#include <stddef.h>
#include "bigint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Fetches the exchange rate from `from` currency to `to` currency.
     * Writes the rate (target per 1 source) to `rate_out`.
     * Returns 1 on success, 0 on failure.
     */
    int fetch_exchange_rate(const char *from, const char *to, double *rate_out);

    /* Returns a decimal rate. Stub mode preserves all decimal digits. */
    int fetch_exchange_rate_decimal(const char *from, const char *to,
                                    char *text_out, size_t output_size);

    /* Exact non-negative integer-cent conversion with half-up rounding.
     * The decimal rate is represented as a rational integer pair.
     * out must be initialized to zero or contain a valid BigInt.
     */
    int convert_cents_decimal(const char *amount_cents, const char *decimal_rate,
                              BigInt *out);

#ifdef __cplusplus
}
#endif

#endif

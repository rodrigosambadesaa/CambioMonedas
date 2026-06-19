#ifndef EXCHANGE_API_H
#define EXCHANGE_API_H

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

#ifdef __cplusplus
}
#endif

#endif

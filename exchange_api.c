#include "exchange_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void normalize_currency_token(const char *input, char *output, size_t output_size)
{
    size_t i = 0;
    size_t j = 0;

    if (output == NULL || output_size == 0)
        return;

    output[0] = '\0';
    if (input == NULL)
        return;

    while (input[i] != '\0' && j + 1 < output_size)
    {
        unsigned char c = (unsigned char)input[i];

        if (c == 0xC3 && input[i + 1] != '\0')
        {
            unsigned char s = (unsigned char)input[i + 1];
            char replacement = '\0';

            if (s == 0xA1 || s == 0x81)
                replacement = 'a';
            else if (s == 0xA9 || s == 0x89)
                replacement = 'e';
            else if (s == 0xAD || s == 0x8D)
                replacement = 'i';
            else if (s == 0xB3 || s == 0x93)
                replacement = 'o';
            else if (s == 0xBA || s == 0x9A || s == 0xBC || s == 0x9C)
                replacement = 'u';
            else if (s == 0xB1 || s == 0x91)
                replacement = 'n';

            if (replacement != '\0')
            {
                output[j++] = replacement;
                i += 2;
                continue;
            }
        }

        if (c == ' ' || c == '-')
            output[j++] = '_';
        else
            output[j++] = (char)tolower(c);
        i++;
    }

    output[j] = '\0';
}

static int currency_to_iso_code(const char *input, char *output, size_t output_size)
{
    char normalized[64];

    if (output == NULL || output_size < 4)
        return 0;

    normalize_currency_token(input, normalized, sizeof(normalized));
    if (normalized[0] == '\0')
        return 0;

    if (strcmp(normalized, "eur") == 0 || strcmp(normalized, "euro") == 0 || strcmp(normalized, "euros") == 0)
        strncpy(output, "EUR", output_size);
    else if (strcmp(normalized, "usd") == 0 || strcmp(normalized, "us") == 0 ||
             strcmp(normalized, "dolar") == 0 || strcmp(normalized, "dolares") == 0 ||
             strcmp(normalized, "dollar") == 0 || strcmp(normalized, "dollars") == 0)
        strncpy(output, "USD", output_size);
    else if (strcmp(normalized, "jpy") == 0 || strcmp(normalized, "yen") == 0 || strcmp(normalized, "yenes") == 0)
        strncpy(output, "JPY", output_size);
    else if (strcmp(normalized, "gbp") == 0 || strcmp(normalized, "libra") == 0 ||
             strcmp(normalized, "libras") == 0 || strcmp(normalized, "pound") == 0 ||
             strcmp(normalized, "pounds") == 0)
        strncpy(output, "GBP", output_size);
    else if (strcmp(normalized, "chf") == 0 || strcmp(normalized, "franco_suizo") == 0 ||
             strcmp(normalized, "francos_suizos") == 0)
        strncpy(output, "CHF", output_size);
    else if (strcmp(normalized, "cad") == 0 || strcmp(normalized, "dolar_canadiense") == 0 ||
             strcmp(normalized, "dolares_canadienses") == 0)
        strncpy(output, "CAD", output_size);
    else if (strcmp(normalized, "aud") == 0 || strcmp(normalized, "dolar_australiano") == 0 ||
             strcmp(normalized, "dolares_australianos") == 0)
        strncpy(output, "AUD", output_size);
    else if (strcmp(normalized, "cny") == 0 || strcmp(normalized, "rmb") == 0 ||
             strcmp(normalized, "yuan_chino") == 0 || strcmp(normalized, "yuanes_chinos") == 0)
        strncpy(output, "CNY", output_size);
    else if (strcmp(normalized, "mxn") == 0 || strcmp(normalized, "peso_mexicano") == 0 ||
             strcmp(normalized, "pesos_mexicanos") == 0)
        strncpy(output, "MXN", output_size);
    else if (strlen(normalized) == 3)
    {
        output[0] = (char)toupper((unsigned char)normalized[0]);
        output[1] = (char)toupper((unsigned char)normalized[1]);
        output[2] = (char)toupper((unsigned char)normalized[2]);
        output[3] = '\0';
    }
    else
    {
        return 0;
    }

    output[output_size - 1] = '\0';
    return 1;
}

static int fetch_exchange_rate_from_stub(const char *from, const char *to, double *rate_out)
{
    char from_code[8];
    char to_code[8];
    char key[64];
    FILE *f;
    char buf[4096];
    size_t r;
    char *p;
    double rate;

    if (!currency_to_iso_code(from, from_code, sizeof(from_code)) ||
        !currency_to_iso_code(to, to_code, sizeof(to_code)))
        return 0;

    snprintf(key, sizeof(key), "%s_%s", from_code, to_code);

    f = fopen("rates_stub.json", "r");
    if (!f)
        return 0;

    r = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[r] = '\0';

    p = strstr(buf, key);
    if (!p)
        return 0;

    p = strchr(p, ':');
    if (!p)
        return 0;

    rate = atof(p + 1);
    if (rate <= 0.0)
        return 0;

    *rate_out = rate;
    return 1;
}

/*
 * Lightweight HTTP client to fetch exchange rates.
 *
 * This implementation prefers libcurl when available. If libcurl is not
 * available at compile time, it falls back to a minimal builtin stub that
 * reads rates from a local file `rates_stub.json` if present.
 *
 * To keep changes small and portable, the compile-time option
 * `USE_LIBCURL` enables libcurl usage and the Makefile/CI must link with -lcurl.
 */

#if defined(USE_LIBCURL)
#include <curl/curl.h>

struct memchunk
{
    char *data;
    size_t size;
};

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    struct memchunk *m = (struct memchunk *)userdata;
    char *tmp = realloc(m->data, m->size + realsize + 1);
    if (!tmp)
        return 0;
    m->data = tmp;
    memcpy(&(m->data[m->size]), ptr, realsize);
    m->size += realsize;
    m->data[m->size] = '\0';
    return realsize;
}

int fetch_exchange_rate(const char *from, const char *to, double *rate_out)
{
    char from_code[8];
    char to_code[8];
    const char *source_mode;

    if (!from || !to || !rate_out)
        return 0;

    if (!currency_to_iso_code(from, from_code, sizeof(from_code)) ||
        !currency_to_iso_code(to, to_code, sizeof(to_code)))
        return 0;

    source_mode = getenv("PROGVORAZ_EXCHANGE_SOURCE");
    if (source_mode != NULL && strcmp(source_mode, "stub") == 0)
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);

    CURL *curl = curl_easy_init();
    if (!curl)
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);

    char url[256];
    /* Using exchangerate.host free API as default (no key required). */
    snprintf(url, sizeof(url), "https://api.exchangerate.host/convert?from=%s&to=%s&amount=1", from_code, to_code);

    struct memchunk chunk = {0};
    CURLcode res;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        free(chunk.data);
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);
    }

    /* Try to find "result":<number> in the JSON */
    double rate = 0.0;
    char *p = strstr(chunk.data, "\"result\"");
    if (p)
    {
        p = strchr(p, ':');
        if (p)
            rate = atof(p + 1);
    }

    curl_easy_cleanup(curl);
    free(chunk.data);

    if (rate <= 0.0)
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);

    *rate_out = rate;
    return 1;
}

#else

/* Fallback: read a local stub file `rates_stub.json` with a simple mapping.
 * Format example:
 * { "EUR_USD": 1.09, "USD_EUR": 0.917 }
 */

int fetch_exchange_rate(const char *from, const char *to, double *rate_out)
{
    return fetch_exchange_rate_from_stub(from, to, rate_out);
}

#endif

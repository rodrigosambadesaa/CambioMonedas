#include "exchange_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

/* funcion normalize_currency_token: contiene la logica principal de esta operacion. */
static void normalize_currency_token(const char *input, char *output, size_t output_size)
{
    size_t i = 0;
    size_t j = 0;

    /* if: comprueba output == NULL || output_size == 0 antes de ejecutar esta rama. */
    if (output == NULL || output_size == 0)
        return;

    output[0] = '\0';
    /* if: comprueba input == NULL antes de ejecutar esta rama. */
    if (input == NULL)
        return;

    /* while: repite el bloque mientras se cumpla input[i] != '\0' && j + 1 < output_size. */
    while (input[i] != '\0' && j + 1 < output_size)
    {
        unsigned char c = (unsigned char)input[i];

        /* if: comprueba c == 0xC3 && input[i + 1] != '\0' antes de ejecutar esta rama. */
        if (c == 0xC3 && input[i + 1] != '\0')
        {
            unsigned char s = (unsigned char)input[i + 1];
            char replacement = '\0';

            /* if: comprueba s == 0xA1 || s == 0x81 antes de ejecutar esta rama. */
            if (s == 0xA1 || s == 0x81)
                replacement = 'a';
            /* if: comprueba s == 0xA9 || s == 0x89 antes de continuar con esta alternativa. */
            else if (s == 0xA9 || s == 0x89)
                replacement = 'e';
            /* if: comprueba s == 0xAD || s == 0x8D antes de continuar con esta alternativa. */
            else if (s == 0xAD || s == 0x8D)
                replacement = 'i';
            /* if: comprueba s == 0xB3 || s == 0x93 antes de continuar con esta alternativa. */
            else if (s == 0xB3 || s == 0x93)
                replacement = 'o';
            /* if: comprueba s == 0xBA || s == 0x9A || s == 0xBC || s == 0x9C antes de continuar con esta alternativa. */
            else if (s == 0xBA || s == 0x9A || s == 0xBC || s == 0x9C)
                replacement = 'u';
            /* if: comprueba s == 0xB1 || s == 0x91 antes de continuar con esta alternativa. */
            else if (s == 0xB1 || s == 0x91)
                replacement = 'n';

            /* if: comprueba replacement != '\0' antes de ejecutar esta rama. */
            if (replacement != '\0')
            {
                output[j++] = replacement;
                i += 2;
                continue;
            }
        }

        /* if: comprueba c == ' ' || c == '-' antes de ejecutar esta rama. */
        if (c == ' ' || c == '-')
            output[j++] = '_';
        else
            output[j++] = (char)tolower(c);
        i++;
    }

    output[j] = '\0';
}

/* funcion currency_to_iso_code: contiene la logica principal de esta operacion. */
static int currency_to_iso_code(const char *input, char *output, size_t output_size)
{
    char normalized[64];

    /* if: comprueba output == NULL || output_size < 4 antes de ejecutar esta rama. */
    if (output == NULL || output_size < 4)
        return 0;

    normalize_currency_token(input, normalized, sizeof(normalized));
    /* if: comprueba normalized[0] == '\0' antes de ejecutar esta rama. */
    if (normalized[0] == '\0')
        return 0;

    /* if: comprueba strcmp(normalized, "eur") == 0 || strcmp(normalized, "euro") == 0 ||... antes de ejecutar esta rama. */
    if (strcmp(normalized, "eur") == 0 || strcmp(normalized, "euro") == 0 || strcmp(normalized, "euros") == 0)
        strncpy(output, "EUR", output_size);
    /* if: comprueba strcmp(normalized, "usd") == 0 || strcmp(normalized, "us") == 0 || antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "usd") == 0 || strcmp(normalized, "us") == 0 ||
             strcmp(normalized, "dolar") == 0 || strcmp(normalized, "dolares") == 0 ||
             strcmp(normalized, "dollar") == 0 || strcmp(normalized, "dollars") == 0)
        strncpy(output, "USD", output_size);
    /* if: comprueba strcmp(normalized, "jpy") == 0 || strcmp(normalized, "yen") == 0 || s... antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "jpy") == 0 || strcmp(normalized, "yen") == 0 || strcmp(normalized, "yenes") == 0)
        strncpy(output, "JPY", output_size);
    /* if: comprueba strcmp(normalized, "gbp") == 0 || strcmp(normalized, "libra") == 0 || antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "gbp") == 0 || strcmp(normalized, "libra") == 0 ||
             strcmp(normalized, "libras") == 0 || strcmp(normalized, "pound") == 0 ||
             strcmp(normalized, "pounds") == 0)
        strncpy(output, "GBP", output_size);
    /* if: comprueba strcmp(normalized, "chf") == 0 || strcmp(normalized, "franco_suizo")... antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "chf") == 0 || strcmp(normalized, "franco_suizo") == 0 ||
             strcmp(normalized, "francos_suizos") == 0)
        strncpy(output, "CHF", output_size);
    /* if: comprueba strcmp(normalized, "cad") == 0 || strcmp(normalized, "dolar_canadiens... antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "cad") == 0 || strcmp(normalized, "dolar_canadiense") == 0 ||
             strcmp(normalized, "dolares_canadienses") == 0)
        strncpy(output, "CAD", output_size);
    /* if: comprueba strcmp(normalized, "aud") == 0 || strcmp(normalized, "dolar_australia... antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "aud") == 0 || strcmp(normalized, "dolar_australiano") == 0 ||
             strcmp(normalized, "dolares_australianos") == 0)
        strncpy(output, "AUD", output_size);
    /* if: comprueba strcmp(normalized, "cny") == 0 || strcmp(normalized, "rmb") == 0 || antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "cny") == 0 || strcmp(normalized, "rmb") == 0 ||
             strcmp(normalized, "yuan_chino") == 0 || strcmp(normalized, "yuanes_chinos") == 0)
        strncpy(output, "CNY", output_size);
    /* if: comprueba strcmp(normalized, "mxn") == 0 || strcmp(normalized, "peso_mexicano")... antes de continuar con esta alternativa. */
    else if (strcmp(normalized, "mxn") == 0 || strcmp(normalized, "peso_mexicano") == 0 ||
             strcmp(normalized, "pesos_mexicanos") == 0)
        strncpy(output, "MXN", output_size);
    /* if: comprueba strlen(normalized) == 3 antes de continuar con esta alternativa. */
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

/* funcion fetch_exchange_rate_from_stub: contiene la logica principal de esta operacion. */
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

    /* if: comprueba !currency_to_iso_code(from, from_code, sizeof(from_code)) || antes de ejecutar esta rama. */
    if (!currency_to_iso_code(from, from_code, sizeof(from_code)) ||
        !currency_to_iso_code(to, to_code, sizeof(to_code)))
        return 0;

    if (strcmp(from_code, to_code) == 0) { *rate_out = 1.0; return 1; }
    snprintf(key, sizeof(key), "\"%s_%s\"", from_code, to_code);

    f = fopen("rates_stub.json", "r");
    /* if: comprueba !f antes de ejecutar esta rama. */
    if (!f)
        return 0;

    r = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[r] = '\0';

    p = strstr(buf, key);
    /* if: comprueba !p antes de ejecutar esta rama. */
    if (!p)
        return 0;

    p = strchr(p, ':');
    /* if: comprueba !p antes de ejecutar esta rama. */
    if (!p)
        return 0;

    char *end = NULL;
    rate = strtod(p + 1, &end);
    while (end && isspace((unsigned char)*end)) ++end;
    if (!end || !isfinite(rate) || rate <= 0.0 || (*end != ',' && *end != '}'))
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

/* funcion write_cb: contiene la logica principal de esta operacion. */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    struct memchunk *m = (struct memchunk *)userdata;
    if (size && realsize / size != nmemb) return 0;
    if (realsize > 16384 || m->size > 16384 - realsize) return 0;
    char *tmp = realloc(m->data, m->size + realsize + 1);
    /* if: comprueba !tmp antes de ejecutar esta rama. */
    if (!tmp)
        return 0;
    m->data = tmp;
    memcpy(&(m->data[m->size]), ptr, realsize);
    m->size += realsize;
    m->data[m->size] = '\0';
    return realsize;
}

/* Parse only the flat JSON object returned by Frankfurter v2.
 * Reject mismatched currency identifiers and nonfinite rates.
 */
static int json_string_equals(const char *json, const char *field, const char *wanted)
{
    char key[32];
    snprintf(key, sizeof(key), "\"%s\"", field);
    const char *p = strstr(json, key);
    if (!p) return 0;
    p += strlen(key);
    while (isspace((unsigned char)*p)) ++p;
    if (*p++ != ':') return 0;
    while (isspace((unsigned char)*p)) ++p;
    if (*p++ != '"') return 0;
    size_t n = strlen(wanted);
    return strncmp(p, wanted, n) == 0 && p[n] == '"';
}

static int json_numeric_rate(const char *json, double *rate_out)
{
    const char *p = strstr(json, "\"rate\"");
    if (!p) return 0;
    p += strlen("\"rate\"");
    while (isspace((unsigned char)*p)) ++p;
    if (*p++ != ':') return 0;
    while (isspace((unsigned char)*p)) ++p;
    char *end = NULL;
    double value = strtod(p, &end);
    if (end == p || !isfinite(value) || value <= 0.0) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != ',' && *end != '}') return 0;
    *rate_out = value;
    return 1;
}

/* funcion fetch_exchange_rate: contiene la logica principal de esta operacion. */
int fetch_exchange_rate(const char *from, const char *to, double *rate_out)
{
    char from_code[8];
    char to_code[8];
    const char *source_mode;

    /* if: comprueba !from || !to || !rate_out antes de ejecutar esta rama. */
    if (!from || !to || !rate_out)
        return 0;

    /* if: comprueba !currency_to_iso_code(from, from_code, sizeof(from_code)) || antes de ejecutar esta rama. */
    if (!currency_to_iso_code(from, from_code, sizeof(from_code)) ||
        !currency_to_iso_code(to, to_code, sizeof(to_code)))
        return 0;

    if (strcmp(from_code, to_code) == 0) { *rate_out = 1.0; return 1; }
    source_mode = getenv("PROGVORAZ_EXCHANGE_SOURCE");
    /* if: comprueba source_mode != NULL && strcmp(source_mode, "stub") == 0 antes de ejecutar esta rama. */
    if (source_mode != NULL && strcmp(source_mode, "stub") == 0)
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);

    CURL *curl = curl_easy_init();
    /* if: comprueba !curl antes de ejecutar esta rama. */
    if (!curl)
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);

    char url[256];
    /* Frankfurter v2 serves reference rates without API credentials. */
    snprintf(url, sizeof(url), "https://api.frankfurter.dev/v2/rate/%s/%s", from_code, to_code);

    struct memchunk chunk = {0};
    CURLcode res;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ProgVoraz/1.0");

    res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (res != CURLE_OK || status != 200 || !chunk.data)
    {
        curl_easy_cleanup(curl);
        free(chunk.data);
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);
    }

    double rate = 0.0;
    int verified = json_string_equals(chunk.data, "base", from_code) &&
        json_string_equals(chunk.data, "quote", to_code) &&
        json_numeric_rate(chunk.data, &rate);

    curl_easy_cleanup(curl);
    free(chunk.data);

    if (!verified)
        return fetch_exchange_rate_from_stub(from_code, to_code, rate_out);

    *rate_out = rate;
    return 1;
}

#else

/* Fallback: read a local stub file `rates_stub.json` with a simple mapping.
 * Format example:
 * { "EUR_USD": 1.09, "USD_EUR": 0.917 }
 */

/* funcion fetch_exchange_rate: contiene la logica principal de esta operacion. */
int fetch_exchange_rate(const char *from, const char *to, double *rate_out)
{
    return fetch_exchange_rate_from_stub(from, to, rate_out);
}

#endif


/* The test-rate path preserves decimal digits instead of traversing a double. */
static int stub_decimal_text(const char *from, const char *to,
                             char *output, size_t capacity)
{
    char key[32], content[4096];
    FILE *input = fopen("rates_stub.json", "rb");
    if (!input) return 0;
    size_t bytes = fread(content, 1, sizeof(content) - 1, input);
    int too_large = !feof(input), read_error = ferror(input);
    fclose(input);
    if (too_large || read_error) return 0;
    content[bytes] = '\0';
    if (snprintf(key, sizeof(key), "\"%s_%s\"", from, to) >= (int)sizeof(key))
        return 0;
    const char *start = strstr(content, key);
    if (!start || strstr(start + strlen(key), key)) return 0;
    const char *p = start + strlen(key);
    while (isspace((unsigned char)*p)) ++p;
    if (*p++ != ':') return 0;
    while (isspace((unsigned char)*p)) ++p;
    const char *begin = p;
    int digits = 0, period = 0;
    while (*p) {
        if (isdigit((unsigned char)*p)) {
            ++digits;
        } else if (*p == '.' && !period) {
            period = 1;
        } else {
            break;
        }
        ++p;
    }
    if (digits == 0 || p == begin || p[-1] == '.' || *begin == '.') return 0;
    size_t length = (size_t)(p - begin);
    while (isspace((unsigned char)*p)) ++p;
    if (*p != ',' && *p != '}') return 0;
    if (length >= capacity || length > 120) return 0;
    memcpy(output, begin, length);
    output[length] = '\0';
    return 1;
}

int fetch_exchange_rate_decimal(const char *from, const char *to,
                                char *text_out, size_t output_size)
{
    char source[8], destination[8];
    if (!text_out || output_size == 0 ||
        !currency_to_iso_code(from, source, sizeof(source)) ||
        !currency_to_iso_code(to, destination, sizeof(destination)))
        return 0;
    if (strcmp(source, destination) == 0) {
        if (output_size < 2) return 0;
        memcpy(text_out, "1", 2);
        return 1;
    }
    const char *mode = getenv("PROGVORAZ_EXCHANGE_SOURCE");
    if (mode && strcmp(mode, "stub") == 0)
        return stub_decimal_text(source, destination, text_out, output_size);
    double rate = 0.0;
    if (!fetch_exchange_rate(source, destination, &rate) || !isfinite(rate) || rate <= 0.0)
        return 0;
    /* Historical double-valued HTTP API: format a bounded decimal.
       The deterministic stub case above never loses literal digits. */
    int bytes = snprintf(text_out, output_size, "%.15g", rate);
    return bytes > 0 && (size_t)bytes < output_size;
}

int convert_cents_decimal(const char *amount_cents, const char *decimal_rate,
                          BigInt *out)
{
    char numerator_text[384], denominator_text[384];
    size_t length = 0;
    long fractional = 0, exponent = 0;
    int dot = 0, have_digit = 0, ok = 0;
    const char *p = decimal_rate;
    BigInt amount = {0}, numerator = {0}, denominator = {0};
    BigInt product = {0}, quotient = {0}, remainder = {0};
    BigInt twice = {0}, two = {0}, one = {0}, rounded = {0};

    if (!amount_cents || !*amount_cents || !out || !decimal_rate || !*decimal_rate ||
        strspn(amount_cents, "0123456789") != strlen(amount_cents))
        return 0;

    /* Decimal numerator / power-of-ten denominator; round half up exactly. */
    while (*p && *p != 'e' && *p != 'E') {
        if (*p == '.' && !dot) { dot = 1; ++p; continue; }
        if (!isdigit((unsigned char)*p) || length >= 120) return 0;
        numerator_text[length++] = *p++;
        if (dot) ++fractional;
        have_digit = 1;
    }
    if (!have_digit) return 0;
    if (*p) {
        char *end = NULL;
        exponent = strtol(p + 1, &end, 10);
        if (end == p + 1 || *end || exponent < -200 || exponent > 200) return 0;
    }
    long scale = fractional - exponent;
    if (scale < -200 || scale > 200) return 0;
    if (scale < 0) {
        for (long i = 0; i < -scale; ++i)
            numerator_text[length++] = '0';
        scale = 0;
    }
    numerator_text[length] = '\0';
    denominator_text[0] = '1';
    for (long i = 0; i < scale; ++i)
        denominator_text[i + 1] = '0';
    denominator_text[scale + 1] = '\0';

    if (!bigint_init(&amount, amount_cents) ||
        !bigint_init(&numerator, numerator_text) ||
        !bigint_init(&denominator, denominator_text) ||
        bigint_is_zero(&numerator) ||
        !bigint_init(&two, "2") || !bigint_init(&one, "1") ||
        !bigint_multiply(&amount, &numerator, &product) ||
        !bigint_divmod(&product, &denominator, &quotient, &remainder) ||
        !bigint_multiply(&remainder, &two, &twice))
        goto cleanup;
    if (bigint_compare(&twice, &denominator) >= 0) {
        if (!bigint_add(&quotient, &one, &rounded)) goto cleanup;
        ok = bigint_set(out, &rounded);
    } else {
        ok = bigint_set(out, &quotient);
    }

cleanup:
    bigint_free(&amount);
    bigint_free(&numerator);
    bigint_free(&denominator);
    bigint_free(&product);
    bigint_free(&quotient);
    bigint_free(&remainder);
    bigint_free(&twice);
    bigint_free(&two);
    bigint_free(&one);
    bigint_free(&rounded);
    return ok;
}

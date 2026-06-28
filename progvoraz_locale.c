#include "progvoraz_locale.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static ProgVorazLang g_progvoraz_lang = PROGVORAZ_LANG_ES;

/* funcion strings_equal_ignore_case: contiene la logica principal de esta operacion. */
static int strings_equal_ignore_case(const char *a, const char *b)
{
    size_t i = 0;

    /* if: comprueba a == NULL || b == NULL antes de ejecutar esta rama. */
    if (a == NULL || b == NULL)
        return 0;

    /* while: repite el bloque mientras se cumpla a[i] != '\0' && b[i] != '\0'. */
    while (a[i] != '\0' && b[i] != '\0')
    {
        /* if: comprueba tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]) antes de ejecutar esta rama. */
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return 0;
        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

/* funcion normalize_token: contiene la logica principal de esta operacion. */
static void normalize_token(const char *input, char *output, size_t output_size)
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

        /* if: comprueba isspace(c) || c == '-' antes de ejecutar esta rama. */
        if (isspace(c) || c == '-')
            output[j++] = '_';
        else
            output[j++] = (char)tolower(c);
        i++;
    }

    output[j] = '\0';
}

/* funcion is_token_alias: contiene la logica principal de esta operacion. */
static int is_token_alias(const char *normalized, const char *alias_a, const char *alias_b, const char *alias_c, const char *alias_d)
{
    return (alias_a != NULL && strings_equal_ignore_case(normalized, alias_a)) ||
           (alias_b != NULL && strings_equal_ignore_case(normalized, alias_b)) ||
           (alias_c != NULL && strings_equal_ignore_case(normalized, alias_c)) ||
           (alias_d != NULL && strings_equal_ignore_case(normalized, alias_d));
}

/* funcion progvoraz_locale_init_from_env: contiene la logica principal de esta operacion. */
void progvoraz_locale_init_from_env(void)
{
    const char *env = getenv("PROGVORAZ_LANG");

    /* if: comprueba env != NULL antes de ejecutar esta rama. */
    if (env != NULL)
        progvoraz_locale_set_from_code(env);
}

/* funcion progvoraz_locale_set: contiene la logica principal de esta operacion. */
void progvoraz_locale_set(ProgVorazLang lang)
{
    g_progvoraz_lang = lang;
}

/* funcion progvoraz_locale_set_from_code: contiene la logica principal de esta operacion. */
int progvoraz_locale_set_from_code(const char *code)
{
    char normalized[32];

    normalize_token(code, normalized, sizeof(normalized));
    /* if: comprueba strings_equal_ignore_case(normalized, "en") || strings_equal_ignore_case(normalized, "english") antes de ejecutar esta rama. */
    if (strings_equal_ignore_case(normalized, "en") || strings_equal_ignore_case(normalized, "english"))
    {
        g_progvoraz_lang = PROGVORAZ_LANG_EN;
        return 1;
    }

    /* if: comprueba strings_equal_ignore_case(normalized, "es") || strings_equal_ignore_case(normalized, "spanish") || strings_equal_ignore_case(normalized, "espanol") antes de ejecutar esta rama. */
    if (strings_equal_ignore_case(normalized, "es") || strings_equal_ignore_case(normalized, "spanish") || strings_equal_ignore_case(normalized, "espanol"))
    {
        g_progvoraz_lang = PROGVORAZ_LANG_ES;
        return 1;
    }

    return 0;
}

/* funcion progvoraz_locale_get: contiene la logica principal de esta operacion. */
ProgVorazLang progvoraz_locale_get(void)
{
    return g_progvoraz_lang;
}

/* funcion progvoraz_lang_is_english: contiene la logica principal de esta operacion. */
int progvoraz_lang_is_english(void)
{
    return g_progvoraz_lang == PROGVORAZ_LANG_EN;
}

/* funcion progvoraz_lang_code: contiene la logica principal de esta operacion. */
const char *progvoraz_lang_code(void)
{
    return progvoraz_lang_is_english() ? "en" : "es";
}

/* funcion progvoraz_tr: contiene la logica principal de esta operacion. */
const char *progvoraz_tr(const char *es, const char *en)
{
    return progvoraz_lang_is_english() ? en : es;
}

/* funcion progvoraz_map_currency_key: contiene la logica principal de esta operacion. */
const char *progvoraz_map_currency_key(const char *input)
{
    static char normalized[64];

    normalize_token(input, normalized, sizeof(normalized));
    /* if: comprueba normalized[0] == '\0' antes de ejecutar esta rama. */
    if (normalized[0] == '\0')
        return NULL;

    /* if: comprueba is_token_alias(normalized, "eur", "e", "euro", NULL) antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "eur", "e", "euro", NULL))
        return "euro";
    /* if: comprueba is_token_alias(normalized, "usd", "us", "dolar", "dollar") || strings_equal_ignore_case(normalized, "us_dollar") antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "usd", "us", "dolar", "dollar") || strings_equal_ignore_case(normalized, "us_dollar"))
        return "dolar";
    /* if: comprueba is_token_alias(normalized, "jpy", "yen", "japanese_yen", NULL) antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "jpy", "yen", "japanese_yen", NULL))
        return "yen";
    /* if: comprueba is_token_alias(normalized, "gbp", "libra", "pound", "british_pound") antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "gbp", "libra", "pound", "british_pound"))
        return "libra";
    /* if: comprueba is_token_alias(normalized, "chf", "franc", "franco_suizo", "swiss_franc") antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "chf", "franc", "franco_suizo", "swiss_franc"))
        return "franco_suizo";
    /* if: comprueba is_token_alias(normalized, "cad", "dolar_canadiense", "canadian_dollar", NULL) antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "cad", "dolar_canadiense", "canadian_dollar", NULL))
        return "dolar_canadiense";
    /* if: comprueba is_token_alias(normalized, "aud", "dolar_australiano", "australian_dollar", NULL) antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "aud", "dolar_australiano", "australian_dollar", NULL))
        return "dolar_australiano";
    /* if: comprueba is_token_alias(normalized, "cny", "rmb", "yuan_chino", "chinese_yuan") antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "cny", "rmb", "yuan_chino", "chinese_yuan"))
        return "yuan_chino";
    /* if: comprueba is_token_alias(normalized, "mxn", "peso_mexicano", "mexican_peso", NULL) antes de ejecutar esta rama. */
    if (is_token_alias(normalized, "mxn", "peso_mexicano", "mexican_peso", NULL))
        return "peso_mexicano";

    return normalized;
}

/* funcion cmd_equals: contiene la logica principal de esta operacion. */
static int cmd_equals(const char *text, const char *es, const char *en)
{
    char normalized_text[64];
    char normalized_es[64];
    char normalized_en[64];

    normalize_token(text, normalized_text, sizeof(normalized_text));
    normalize_token(es, normalized_es, sizeof(normalized_es));
    normalize_token(en, normalized_en, sizeof(normalized_en));

    return strings_equal_ignore_case(normalized_text, normalized_es) || strings_equal_ignore_case(normalized_text, normalized_en);
}

int progvoraz_cmd_is_exit(const char *text) { return cmd_equals(text, "salir", "exit") || strings_equal_ignore_case(text, "quit"); }
int progvoraz_cmd_is_back(const char *text) { return cmd_equals(text, "volver", "back"); }
int progvoraz_cmd_is_mode(const char *text) { return cmd_equals(text, "modo", "mode"); }
int progvoraz_cmd_is_history(const char *text) { return cmd_equals(text, "historial", "history"); }
int progvoraz_cmd_is_summary(const char *text) { return cmd_equals(text, "resumen", "summary"); }
int progvoraz_cmd_is_snapshot(const char *text) { return cmd_equals(text, "snapshot", "snapshot"); }
int progvoraz_cmd_is_restore(const char *text) { return cmd_equals(text, "restaurar", "restore"); }
int progvoraz_cmd_is_report(const char *text) { return cmd_equals(text, "reporte", "report"); }
int progvoraz_cmd_is_json(const char *text) { return cmd_equals(text, "json", "json"); }
int progvoraz_cmd_is_convert(const char *text) { return cmd_equals(text, "convertir", "convert"); }
int progvoraz_cmd_is_limited(const char *text) { return cmd_equals(text, "limitado", "limited"); }
int progvoraz_cmd_is_unlimited(const char *text) { return cmd_equals(text, "ilimitado", "unlimited"); }
int progvoraz_cmd_is_add(const char *text) { return cmd_equals(text, "anadir", "add"); }
int progvoraz_cmd_is_remove(const char *text) { return cmd_equals(text, "quitar", "remove"); }
int progvoraz_cmd_is_calculate(const char *text) { return cmd_equals(text, "calcular", "calculate"); }
int progvoraz_cmd_is_register(const char *text) { return cmd_equals(text, "caja", "register"); }
int progvoraz_cmd_is_specific(const char *text) { return cmd_equals(text, "especifico", "specific"); }
int progvoraz_cmd_is_traditional(const char *text) { return cmd_equals(text, "tradicional", "traditional"); }
int progvoraz_cmd_is_limit(const char *text) { return cmd_equals(text, "limite", "limit"); }

#ifndef PROGVORAZ_LOCALE_H
#define PROGVORAZ_LOCALE_H

typedef enum
{
    PROGVORAZ_LANG_ES = 0,
    PROGVORAZ_LANG_EN = 1
} ProgVorazLang;

void progvoraz_locale_init_from_env(void);
void progvoraz_locale_set(ProgVorazLang lang);
int progvoraz_locale_set_from_code(const char *code);
ProgVorazLang progvoraz_locale_get(void);
int progvoraz_lang_is_english(void);
const char *progvoraz_lang_code(void);
const char *progvoraz_tr(const char *es, const char *en);
const char *progvoraz_map_currency_key(const char *input);

int progvoraz_cmd_is_exit(const char *text);
int progvoraz_cmd_is_back(const char *text);
int progvoraz_cmd_is_mode(const char *text);
int progvoraz_cmd_is_history(const char *text);
int progvoraz_cmd_is_summary(const char *text);
int progvoraz_cmd_is_snapshot(const char *text);
int progvoraz_cmd_is_restore(const char *text);
int progvoraz_cmd_is_report(const char *text);
int progvoraz_cmd_is_json(const char *text);
int progvoraz_cmd_is_convert(const char *text);
int progvoraz_cmd_is_limited(const char *text);
int progvoraz_cmd_is_unlimited(const char *text);
int progvoraz_cmd_is_add(const char *text);
int progvoraz_cmd_is_remove(const char *text);
int progvoraz_cmd_is_calculate(const char *text);
int progvoraz_cmd_is_register(const char *text);
int progvoraz_cmd_is_specific(const char *text);
int progvoraz_cmd_is_traditional(const char *text);
int progvoraz_cmd_is_limit(const char *text);

#endif

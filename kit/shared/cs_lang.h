/*
 * cs_lang.h - the interface in five languages: English, German, French, Spanish and Italian.
 *
 * Words are written in English in the source and passed through tr() where they are shown. The
 * table of translations is the program's (made by kit/shared/tools/mklang.py from lang/strings.txt
 * into a header of rows, see docs/shared/08-translation.md) and handed over with cs_lang_init().
 * A word the table hasn't got, or a row without that language, comes back in English, so new
 * strings can go in before they are translated. Text is Latin-1 throughout (the Amiga's fonts);
 * other platforms convert at their edge. Plain C99: tested natively.
 */
#ifndef CS_LANG_H
#define CS_LANG_H

enum { CS_LANG_EN, CS_LANG_DE, CS_LANG_FR, CS_LANG_ES, CS_LANG_IT, CS_NLANG };
typedef struct { const char *s[CS_NLANG]; } CsWords;    /* one row: English, then each other (NULL: as English) */

extern const char *const cs_language_names[CS_NLANG];  /* each in itself: "Deutsch", "Fran\347ais"... */

/* the program's table (rows as lang_table.h has them; n rows). Sorted here once, by the English */
void cs_lang_init(const CsWords *table, int n);
void cs_lang_set(int lang);                    /* CS_LANG_; anything else is English */
int  cs_lang_current(void);
const char *tr(const char *english);           /* in the language chosen, or english itself */
/* a NULL-ended list of English words (cycle gadget entries, list labels) translated: a list kept
   for as long as the program runs, made again if the list at that address has changed since. The
   list MUST end with NULL: a list without one is returned untranslated (Quill's Insert Caption
   crashed when tr_list ran on past the end of two lists) */
const char *const *tr_list(const char *const *english);
const char *cs_lang_code(int lang);            /* "en", "de", ... (what the prefs file keeps) */
int  cs_lang_by_code(const char *code);        /* -1 if not one of ours */
/* one of Locale prefs' language names ("deutsch", "deutsch.language", "fran\347ais"; any case),
   -1 if not ours. English counts, so English before German in the user's list keeps English */
int  cs_lang_by_locale_name(const char *name);
int  cs_lang_rows(void);                       /* rows in the table */

#endif

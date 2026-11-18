/* cs_lang.c - see cs_lang.h. The Locale part (which language the user prefers) is the platform's:
   kit/amiga/cs_locale.c on the Amiga. */
#include <string.h>
#include <stdlib.h>
#include "cs_lang.h"

const char *const cs_language_names[CS_NLANG] = { "English", "Deutsch", "Fran\347ais", "Espa\361ol", "Italiano" };
static const char *const codes[CS_NLANG] = { "en", "de", "fr", "es", "it" };
static const char *const locnames[CS_NLANG] = { "english", "deutsch", "fran\347ais", "espa\361ol", "italiano" };

#define CS_LANG_MAXROWS 4000
static const CsWords *byen[CS_LANG_MAXROWS];   /* the rows sorted by their English, for bsearch */
static int nby, cur;

static int cmp_words(const void *a, const void *b) { return strcmp((*(const CsWords *const *)a)->s[0], (*(const CsWords *const *)b)->s[0]); }
static int cmp_key(const void *k, const void *b) { return strcmp((const char *)k, (*(const CsWords *const *)b)->s[0]); }

void cs_lang_init(const CsWords *table, int n)
{
  int i;
  nby = 0;
  for (i = 0; i < n && nby < CS_LANG_MAXROWS; i++) if (table[i].s[0]) byen[nby++] = &table[i];
  qsort(byen, (size_t)nby, sizeof byen[0], cmp_words);
}
void cs_lang_set(int lang) { cur = lang >= 0 && lang < CS_NLANG ? lang : CS_LANG_EN; }
int cs_lang_current(void) { return cur; }
int cs_lang_rows(void) { return nby; }
/* a pointer into the program's table (not to be changed), or english itself */
const char *tr(const char *english)
{
  const CsWords *const *w;
  if (cur == CS_LANG_EN || !english || !*english || !nby) return english;
  w = (const CsWords *const *)bsearch(english, byen, (size_t)nby, sizeof byen[0], cmp_key);
  return w && (*w)->s[cur] ? (*w)->s[cur] : english;
}
/* the lists made so far: the address asked for, the English it held then (to see whether a list
   on the stack, or filled again, has changed) and its translation. The oldest is reused when full */
#define NLISTS 64
#define LISTMAX 64
static struct { const char *const *src; int lang; const char *en[LISTMAX + 1]; const char *out[LISTMAX + 1]; } lists[NLISTS];
static int nlists, nextlist;
const char *const *tr_list(const char *const *english)
{
  int i, k, n;
  if (!english) return english;
  for (n = 0; n < LISTMAX && english[n]; n++) ;
  if (n == LISTMAX && english[n]) return english;          /* longer than a list kept here (or no NULL) */
  for (i = 0; i < nlists; i++) {
    if (lists[i].src != english) continue;
    for (k = 0; k <= n && lists[i].en[k] == english[k]; k++) ;
    if (k > n && lists[i].lang == cur) return lists[i].out;
    break;
  }
  if (i == nlists) { if (nlists < NLISTS) i = nlists++; else { i = nextlist; nextlist = (nextlist + 1) % NLISTS; } }
  lists[i].src = english; lists[i].lang = cur;
  for (k = 0; k < n; k++) { lists[i].en[k] = english[k]; lists[i].out[k] = tr(english[k]); }
  lists[i].en[n] = NULL; lists[i].out[n] = NULL;
  return lists[i].out;
}
const char *cs_lang_code(int lang) { return codes[lang >= 0 && lang < CS_NLANG ? lang : 0]; }
int cs_lang_by_code(const char *code)
{
  int i;
  for (i = 0; code && i < CS_NLANG; i++) if (!strcmp(code, codes[i])) return i;
  return -1;
}
int cs_lang_by_locale_name(const char *p)
{
  int i;
  size_t n;
  for (i = 0; p && i < CS_NLANG; i++) {
    const char *q = locnames[i];
    for (n = 0; q[n] && p[n] && (p[n] | 0x20) == (q[n] | 0x20); n++) ;
    if (!q[n] && (!p[n] || p[n] == '.')) return i;
  }
  return -1;
}

/* cs_cmd.c - see cs_cmd.h: the command registry, the command search and the menu layout check */
#include <string.h>
#include <stdio.h>
#include "cs_cmd.h"
#include "cs_lang.h"
#include "cs_util.h"

static struct { const CsCommand *t; int n; } tables[CS_CMD_MAXTABLES];
static int ntables;
static int (*state_fn)(unsigned long id);
static int has_doc = 1;

int cs_cmd_register(const CsCommand *table, int n)
{
  if (ntables >= CS_CMD_MAXTABLES || !table || n <= 0) return 0;
  tables[ntables].t = table; tables[ntables].n = n; ntables++;
  return 1;
}
void cs_cmd_clear(void) { ntables = 0; state_fn = NULL; has_doc = 1; }
/* the tables are searched in order, each from its start: a few hundred commands take no time */
const CsCommand *cs_cmd_find(unsigned long id)
{
  int t, i;
  if (!id) return NULL;
  for (t = 0; t < ntables; t++)
    for (i = 0; i < tables[t].n; i++) if (tables[t].t[i].id == id) return &tables[t].t[i];
  return NULL;
}
int cs_cmd_count(void)
{
  int t, n = 0;
  for (t = 0; t < ntables; t++) n += tables[t].n;
  return n;
}
const CsCommand *cs_cmd_at(int i)
{
  int t;
  for (t = 0; t < ntables && i >= 0; t++) { if (i < tables[t].n) return &tables[t].t[i]; i -= tables[t].n; }
  return NULL;
}
/* the translated label with "..." and the "_" marker taken out */
char *cs_cmd_name(unsigned long id, char *out, int cap)
{
  const CsCommand *c = cs_cmd_find(id);
  const char *s = c && c->label ? tr(c->label) : "";
  int k = 0;
  size_t n = strlen(s);
  if (cap <= 0) return out;
  if (n >= 3 && !strcmp(s + n - 3, "...")) n -= 3;
  for (; *s && n && k < cap - 1; s++, n--) if (*s != '_') out[k++] = *s;
  out[k] = 0;
  return out;
}
unsigned long cs_cmd_by_key(int key)
{
  int i, n = cs_cmd_count();
  key = cs_fold(key);
  for (i = 0; i < n; i++) {
    const CsCommand *c = cs_cmd_at(i);
    if (c->key && c->key[0] && !c->key[1] && cs_fold((unsigned char)c->key[0]) == key) return c->id;
  }
  return 0;
}
void cs_cmd_set_state(int (*state)(unsigned long id)) { state_fn = state; }
void cs_cmd_set_doc(int on) { has_doc = on != 0; }
int cs_cmd_state(unsigned long id)
{
  const CsCommand *c = cs_cmd_find(id);
  int s = state_fn ? state_fn(id) : CSS_ENABLED;
  if (c && (c->flags & CSC_NEEDDOC) && !has_doc) s &= ~CSS_ENABLED;
  return s;
}

/* Latin-1: lower case, and the accented letters as their plain one, so "Ubersicht" finds
   "\334bersicht" and "fenetre" finds "fen\352tre" */
int cs_fold(int c)
{
  static const char map[] =                      /* 0xC0..0xFF */
    "aaaaaaaceeeeiiiidnooooo\327ouuuuyts"         /* C0-DF (D7 times, DE thorn, DF sharp s) */
    "aaaaaaaceeeeiiiidnooooo\367ouuuuyty";        /* E0-FF (F7 divide) */
  c &= 255;
  if (c >= 'A' && c <= 'Z') return c + 32;
  if (c >= 0xC0) return (unsigned char)map[c - 0xC0];
  return c;
}
/* word w (wl bytes) found in s, folded; at_start: it starts s */
static int has_word(const char *s, const char *w, size_t wl, int *at_start)
{
  size_t i, k;
  for (i = 0; s[i]; i++) {
    for (k = 0; k < wl && s[i + k] && cs_fold((unsigned char)s[i + k]) == cs_fold((unsigned char)w[k]); k++) ;
    if (k == wl) { if (at_start) *at_start = i == 0; return 1; }
  }
  return 0;
}
int cs_cmd_search(const char *query, unsigned long *ids, int max)
{
  int i, n = cs_cmd_count(), got = 0, pass;
  char name[96];
  while (*query == ' ') query++;
  if (!*query || max <= 0) return 0;
  /* two passes: names starting with the query, then the rest */
  for (pass = 0; pass < 2 && got < max; pass++)
    for (i = 0; i < n && got < max; i++) {
      const CsCommand *c = cs_cmd_at(i);
      const char *q = query, *help = c->help ? tr(c->help) : "";
      int all = 1, start = 0, first = 1, k;
      if (!c->label || (c->flags & (CSC_HIDDEN | CSC_NOSEARCH)) || !(cs_cmd_state(c->id) & CSS_ENABLED)) continue;
      for (k = 0; k < got && ids[k] != c->id; k++) ;
      if (k < got) continue;                                   /* (a command listed in two tables) */
      cs_cmd_name(c->id, name, sizeof name);
      while (*q && all) {
        size_t wl = 0;
        int st = 0;
        while (q[wl] && q[wl] != ' ') wl++;
        if (!has_word(name, q, wl, &st) && !has_word(help, q, wl, NULL)) all = 0;
        if (first) start = st && has_word(name, q, wl, NULL);
        first = 0;
        q += wl;
        while (*q == ' ') q++;
      }
      if (all && (pass == 0) == (start != 0)) ids[got++] = c->id;
    }
  return got;
}

/* ---- the menu layout's check */
static void say(void (*report)(const char *), int *bad, const char *fmt, unsigned long a, const char *s)
{
  char line[160];
  (*bad)++;
  if (!report) return;
  snprintf(line, sizeof line, fmt, a, s ? s : "");
  report(line);
}
int cs_menu_check(const CsMenuItem *m, void (*report)(const char *line))
{
  int bad = 0, menus = 0, items = 0, subs = 0, i, j;
  const char *menu = "";
  for (i = 0; m[i].kind != CSM_END; i++) {
    switch (m[i].kind) {
    case CSM_TITLE:
      menu = m[i].title ? m[i].title : "";
      if (++menus > CS_MAXMENUS) say(report, &bad, "more than 31 menus at \"%s\"%s", 0, menu);
      items = 0; subs = 0;
      break;
    case CSM_ITEM: case CSM_BAR: case CSM_SUB:
      subs = 0;
      if (++items > CS_MAXITEMS) say(report, &bad, "menu %lu \"%s\" has more than 63 items", (unsigned long)menus, menu);
      break;
    case CSM_SUBITEM: case CSM_SUBBAR:
      if (++subs > CS_MAXSUBS) say(report, &bad, "a submenu of menu %lu \"%s\" has more than 31 items", (unsigned long)menus, menu);
      break;
    }
    if ((m[i].kind == CSM_ITEM || m[i].kind == CSM_SUBITEM)) {
      const CsCommand *c = cs_cmd_find(m[i].id);
      if (!c) { say(report, &bad, "command %lu is in the menus but in no table%s", m[i].id, ""); continue; }
      for (j = 0; j < i; j++)
        if ((m[j].kind == CSM_ITEM || m[j].kind == CSM_SUBITEM) && m[j].id == m[i].id)
          say(report, &bad, "command %lu \"%s\" is in the menus twice", m[i].id, c->label);
    }
  }
  /* shortcut letters: each used once */
  {
    int n = cs_cmd_count();
    for (i = 0; i < n; i++) {
      const CsCommand *a = cs_cmd_at(i);
      if (!a->key || !a->key[0]) continue;
      for (j = i + 1; j < n; j++) {
        const CsCommand *b = cs_cmd_at(j);
        if (b->key && b->id != a->id && cs_fold((unsigned char)b->key[0]) == cs_fold((unsigned char)a->key[0]) && !strcmp(b->key + 1, a->key + 1))
          say(report, &bad, "shortcut of command %lu is also \"%s\"'s", b->id, a->label);
      }
    }
  }
  return bad;
}

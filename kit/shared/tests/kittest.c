/*
 * kittest.c - native tests of Cornerstone's portable kit (kit/shared): the command registry and
 * search, menu checks, translation, preferences, progress, wrapping, KeyTips, the icon language and
 * the ribbon layout. Built with the sanitizers by kit/shared/tools/test.sh:
 *   cc -std=c99 -Wall -Wextra -fsanitize=address,undefined -Ikit/shared kit/shared/cs_*.c kit/shared/tests/kittest.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cs_cmd.h"
#include "cs_lang.h"
#include "cs_util.h"
#include "cs_icon.h"
#include "cs_ribbon.h"

static int tests, failed;
#define CHECK(c) do { tests++; if (!(c)) { failed++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

enum { ID_NEW = 100, ID_OPEN, ID_SAVE, ID_SAVEAS, ID_QUIT, ID_UNDO = 200, ID_CUT, ID_COPY, ID_PASTE, ID_FIND, ID_WRAP = 300, ID_TABLETOOLS };
static const CsCommand cmds[] = {
  { ID_NEW, "New", "N", CS_IC_NEW, "Starts a new, empty document.", 0 },
  { ID_OPEN, "Open...", "O", CS_IC_OPEN, "Opens a document from disk.", 0 },
  { ID_SAVE, "Save", "S", CS_IC_SAVE, "Saves the document.", CSC_NEEDDOC },
  { ID_SAVEAS, "Save As...", 0, CS_IC_SAVEAS, "Saves the document under another name.", CSC_NEEDDOC },
  { ID_QUIT, "Quit", "Q", CS_IC_QUIT, "Ends the program.", 0 },
  { ID_UNDO, "Undo", "Z", CS_IC_UNDO, "Takes back the last change.", CSC_NEEDDOC },
  { ID_CUT, "Cut", "X", CS_IC_CUT, "Moves the selection to the clipboard.", CSC_NEEDDOC },
  { ID_COPY, "Copy", "C", CS_IC_COPY, "Copies the selection to the clipboard.", CSC_NEEDDOC },
  { ID_PASTE, "Paste", "V", CS_IC_PASTE, "Puts in what the clipboard holds.", CSC_NEEDDOC },
  { ID_FIND, "Find...", "F", CS_IC_FIND, "Looks for words in the document.", CSC_NEEDDOC },
  { ID_WRAP, "Word Wrap", "W", CS_IC_WRAP, "Breaks long lines at the window's edge.", CSC_TOGGLE },
  { ID_TABLETOOLS, "Table Tools", 0, 0, "", CSC_HIDDEN },
};
static const CsWords words[] = {
  { { "Save As...", "Speichern unter...", "Enregistrer sous...", "Guardar como...", "Salva con nome..." } },
  { { "Find...", "Suchen...", "Rechercher...", "Buscar...", "Trova..." } },
  { { "Word Wrap", "Zeilenumbruch", "Retour automatique", "Ajuste de l\355nea", "A capo automatico" } },
  { { "Home", "Start", "Accueil", "Inicio", "Home" } },
  { { "Clipboard", "Zwischenablage", "Presse-papiers", "Portapapeles", NULL } },
};
static int wrap_on, table_on;
static int state(unsigned long id)
{
  if (id == ID_WRAP) return CSS_ENABLED | (wrap_on ? CSS_CHECKED : 0);
  if (id == ID_TABLETOOLS) return table_on ? CSS_CHECKED | CSS_ENABLED : CSS_ENABLED;
  return CSS_ENABLED;
}
static char lines[8][160];
static int nlines;
static void report(const char *l) { if (nlines < 8) cs_strlcpy(lines[nlines++], l, sizeof lines[0]); }

static void test_commands(void)
{
  char b[64];
  unsigned long ids[8];
  int n;
  cs_cmd_clear();
  CHECK(cs_cmd_register(cmds, (int)(sizeof cmds / sizeof cmds[0])));
  CHECK(cs_cmd_count() == 12);
  CHECK(cs_cmd_find(ID_SAVE) && !strcmp(cs_cmd_find(ID_SAVE)->label, "Save"));
  CHECK(!cs_cmd_find(999));
  CHECK(!strcmp(cs_cmd_name(ID_SAVEAS, b, sizeof b), "Save As"));
  CHECK(cs_cmd_by_key('s') == ID_SAVE && cs_cmd_by_key('S') == ID_SAVE && !cs_cmd_by_key('j'));
  cs_cmd_set_state(state);
  cs_cmd_set_doc(0);
  CHECK(!(cs_cmd_state(ID_SAVE) & CSS_ENABLED) && (cs_cmd_state(ID_NEW) & CSS_ENABLED));
  cs_cmd_set_doc(1);
  CHECK(cs_cmd_state(ID_SAVE) & CSS_ENABLED);
  wrap_on = 1; CHECK(cs_cmd_state(ID_WRAP) & CSS_CHECKED); wrap_on = 0;
  /* search: name starts first, then words anywhere, help counts, any case */
  n = cs_cmd_search("sa", ids, 8);
  CHECK(n == 2 && ids[0] == ID_SAVE && ids[1] == ID_SAVEAS);
  n = cs_cmd_search("clipboard", ids, 8);
  CHECK(n == 3);
  n = cs_cmd_search("SAVE name", ids, 8);
  CHECK(n == 1 && ids[0] == ID_SAVEAS);
  CHECK(cs_cmd_search("table", ids, 8) == 0);                  /* hidden */
  cs_cmd_set_doc(0);
  CHECK(cs_cmd_search("save", ids, 8) == 0);                   /* disabled */
  cs_cmd_set_doc(1);
}
static void test_menus(void)
{
  static const CsMenuItem good[] = { CS_MENU("File"), CS_ITEM(ID_NEW), CS_ITEM(ID_OPEN), CS_BAR, CS_ITEM(ID_SAVE), CS_ITEM(ID_SAVEAS),
                                     CS_BAR, CS_ITEM(ID_QUIT), CS_MENU("Edit"), CS_ITEM(ID_UNDO), CS_SUB("Clipboard"), CS_SUBITEM(ID_CUT),
                                     CS_SUBITEM(ID_COPY), CS_SUBITEM(ID_PASTE), CS_MENUEND };
  static const CsMenuItem twice[] = { CS_MENU("File"), CS_ITEM(ID_NEW), CS_ITEM(ID_NEW), CS_ITEM(4242), CS_MENUEND };
  CsMenuItem big[80];
  int i;
  nlines = 0;
  CHECK(cs_menu_check(good, report) == 0);
  CHECK(cs_menu_check(twice, report) == 2 && nlines == 2);
  big[0].kind = CSM_TITLE; big[0].title = "Long"; big[0].id = 0;
  for (i = 1; i < 66; i++) { big[i].kind = CSM_BAR; big[i].id = 0; big[i].title = 0; }
  big[66].kind = CSM_END;
  nlines = 0;
  CHECK(cs_menu_check(big, report) == 2);                      /* items 64 and 65 */
}
static void test_lang(void)
{
  static const char *const list[] = { "Home", "Word Wrap", "Other", NULL };
  const char *const *t;
  cs_lang_init(words, (int)(sizeof words / sizeof words[0]));
  CHECK(cs_lang_rows() == 5);
  cs_lang_set(CS_LANG_EN);
  CHECK(!strcmp(tr("Find..."), "Find..."));
  cs_lang_set(CS_LANG_DE);
  CHECK(!strcmp(tr("Find..."), "Suchen..."));
  CHECK(!strcmp(tr("Not there"), "Not there"));
  cs_lang_set(CS_LANG_IT);
  CHECK(!strcmp(tr("Clipboard"), "Clipboard"));                /* no Italian in the row: English */
  cs_lang_set(CS_LANG_FR);
  t = tr_list(list);
  CHECK(!strcmp(t[0], "Accueil") && !strcmp(t[1], "Retour automatique") && !strcmp(t[2], "Other") && !t[3]);
  cs_lang_set(CS_LANG_ES);
  t = tr_list(list);                                           /* the same list in another language */
  CHECK(!strcmp(t[0], "Inicio"));
  CHECK(cs_lang_by_code("it") == CS_LANG_IT && cs_lang_by_code("xx") == -1 && !strcmp(cs_lang_code(CS_LANG_DE), "de"));
  CHECK(cs_lang_by_locale_name("Deutsch.language") == CS_LANG_DE && cs_lang_by_locale_name("fran\347ais") == CS_LANG_FR);
  CHECK(cs_lang_by_locale_name("dansk") == -1 && cs_lang_by_locale_name("deutschland") == -1);
  cs_lang_set(CS_LANG_DE);
  {
    unsigned long ids[4];
    CHECK(cs_cmd_search("zeilen", ids, 4) == 1 && ids[0] == ID_WRAP);   /* search in the language shown */
  }
  cs_lang_set(CS_LANG_EN);
}
static void test_prefs(const char *dir)
{
  char path[300];
  snprintf(path, sizeof path, "%s/cs_kittest.prefs", dir);
  cs_prefs_clear();
  CHECK(cs_prefs_set("language", "de") && cs_prefs_set_int("width", 640) && cs_prefs_set("odd key", "a=b"));
  CHECK(!cs_prefs_set("bad=key", "x"));
  CHECK(cs_prefs_save(path));
  cs_prefs_clear();
  CHECK(!strcmp(cs_prefs_get("language", "en"), "en"));
  CHECK(cs_prefs_load(path));
  CHECK(!strcmp(cs_prefs_get("language", "en"), "de") && cs_prefs_int("width", 0) == 640 && !strcmp(cs_prefs_get("odd key", ""), "a=b"));
  CHECK(cs_prefs_int("language", 7) == 7);                     /* not a number */
  CHECK(!cs_prefs_load("/nonexistent/x.prefs"));
  remove(path);
  cs_prefs_clear();
}
static long seen[4];
static int nseen, abort_at = -1;
static int hook(long d, long t) { if (nseen < 4) seen[nseen++] = d; (void)t; return d >= abort_at && abort_at >= 0; }
static void test_progress(void)
{
  long s;
  CHECK(cs_progress(5, 10) == 0);                              /* no hook */
  cs_progress_hook = hook;
  nseen = 0;
  cs_progress(5, 10);
  s = cs_progress_push(500, 1000);
  cs_progress(0, 4); cs_progress(4, 4);
  cs_progress_pop(s);
  CHECK(nseen == 3 && seen[0] == 500 && seen[1] == 500 && seen[2] == 1000);
  abort_at = 300;
  CHECK(cs_progress(1, 2) != 0);
  cs_progress_hook = NULL; abort_at = -1;
  CHECK(cs_muldiv(2000000000L, 3, 4) == 1500000000L && cs_muldiv(7, 6, 4) == 10 && cs_muldiv(-10, 3, 2) == -15);
}
static void test_wrap(void)
{
  char out[200];
  cs_wrap("one two three four", out, sizeof out, 9);
  CHECK(!strcmp(out, "one two\nthree\nfour"));
  cs_wrap("averyveryverylongword x", out, sizeof out, 5);
  CHECK(!strcmp(out, "averyveryverylongword\nx"));
  cs_wrap("a\nb c", out, sizeof out, 66);
  CHECK(!strcmp(out, "a\nb c"));
  cs_wrap("abc def", out, 5, 66);
  CHECK(!strcmp(out, "abc "));
}
static void test_keytips(void)
{
  const char *names[] = { "Home", "Insert", "View", "Help", NULL, "\326ffnen" };
  char codes[6][3];
  cs_keytips_assign(names, 6, codes);
  CHECK(!strcmp(codes[1], "I") && !strcmp(codes[2], "V") && !strcmp(codes[5], "O") && !codes[4][0]);
  CHECK(codes[0][0] == 'H' && codes[3][0] == 'H' && strcmp(codes[0], codes[3]) && strlen(codes[0]) == 2);
  CHECK(cs_keytips_match(codes, 6, "V") == 2 && cs_keytips_match(codes, 6, "H") == -1 && cs_keytips_match(codes, 6, "Z") == -2);
}
/* a recorder of what an icon draws */
static int nops, lastx;
static void r_line(void *c, int a, int b, int e, int f) { (void)c; (void)b; (void)e; (void)f; nops++; lastx = a; }
static void r_box(void *c, int a, int b, int e, int f, int fl) { (void)c; (void)b; (void)e; (void)f; (void)fl; nops++; lastx = a; }
static void test_icons(void)
{
  CsIconOps o;
  int i, bad = 0;
  memset(&o, 0, sizeof o);
  o.line = r_line; o.box = r_box;
  for (i = 1; i < CS_IC_N; i++) if (cs_icon_check(cs_icon_src(i))) { bad++; printf("icon %s: %d\n", cs_icon_name(i), cs_icon_check(cs_icon_src(i))); }
  CHECK(bad == 0);
  CHECK(!strcmp(cs_icon_name(CS_IC_SAVE), "SAVE") && cs_icon_src(CS_IC_N)[0] == 0 && cs_icon_src(-3)[0] == 0);
  nops = 0;
  CHECK(cs_icon_run("ci L 1 2 3 4 R 5 6 7 8", &o, 0) && nops == 2 && lastx == 5);
  CHECK(cs_icon_run(">L 1 2 3 4", &o, 1) && lastx == 14);          /* mirrored */
  CHECK(cs_icon_run("L 1 2 3 4", &o, 1) && lastx == 1);            /* not directional: as it is */
  CHECK(cs_icon_run(">R 1 2 3 4", &o, 1) && lastx == 12);          /* a box mirrored keeps x0 <= x1 */
  CHECK(!cs_icon_run("L 1 2 3", &o, 0) && !cs_icon_run("Q 1", &o, 0));
  CHECK(cs_icon_check("L 1 2 3 16") == 2 && cs_icon_check("E 1 1 3") == 2 && cs_icon_check("X") == 1);
  {
    static const char *const mine[] = { "ci L 1 1 14 14" }, *const mnames[] = { "DIAGONAL" };
    cs_icon_set_app(mine, mnames, 1);
    CHECK(!strcmp(cs_icon_src(CS_IC_APP), "ci L 1 1 14 14") && !strcmp(cs_icon_name(CS_IC_APP), "DIAGONAL") && !cs_icon_src(CS_IC_APP + 1)[0]);
  }
}
/* the ribbon measured with every character 6 pixels wide */
static long w6(void *c, const char *s) { (void)c; return 6 * (long)strlen(s); }
static void test_ribbon(void)
{
  static const CsRbItem clip[] = { { CSK_BUTTON, CSZ_LARGE, 0, ID_PASTE, 0, 0, 0 }, { CSK_BUTTON, CSZ_SMALL, 0, ID_CUT, 0, 0, 0 },
                                   { CSK_BUTTON, CSZ_SMALL, 0, ID_COPY, 0, 0, 0 } };
  static const CsRbItem edit[] = { { CSK_BUTTON, CSZ_SMALL, 0, ID_UNDO, 0, 0, 0 }, { CSK_SPLIT, CSZ_SMALL, 0, ID_FIND, 0, 0, 0 },
                                   { CSK_CHECK, CSZ_SMALL, 0, ID_WRAP, 0, 0, 0 } };
  static const CsRbGroup home[] = { { "Clipboard", CS_RB_ITEMS(clip) }, { "Editing", CS_RB_ITEMS(edit) }, { "More", CS_RB_ITEMS(edit) } };
  static const CsRbTab tabs[] = { { "Home", CS_RB_ITEMS(home), 0 }, { "Table", CS_RB_ITEMS(home), ID_TABLETOOLS } };
  static const unsigned long qa[] = { ID_SAVE, ID_UNDO };
  static const CsRibbonDef def = { tabs, 2, ID_NEW, qa, 2, ID_FIND };
  CsRbMetrics m = { 11, w6, NULL };
  CsRbLayout *l = (CsRbLayout *)malloc(sizeof *l);
  int k, h, i, ok;
  char codes[1 + 2 + 2 + 1][3], ic[CS_RB_MAXITEMS][3];
  cs_lang_set(CS_LANG_EN);
  CHECK(cs_rb_height(11, 0) == 97 && cs_rb_height(11, 1) == 23);
  cs_rb_layout(&def, 0, 640, 0, 0, &m, l);
  CHECK(l->tab == 0 && l->ngroup == 3 && l->nitem == 9 && !l->squeeze && !l->overflow);
  CHECK(l->file.x0 == 2 && l->tabbox[0].x0 > l->file.x1 && l->tabbox[1].x0 < 0);    /* Table hidden */
  CHECK(l->qa[0].x0 > 0 && l->qa[1].x0 > l->qa[0].x1 && l->tell.x0 > l->qa[1].x1 && l->tell.x1 == 637);
  CHECK(l->item[0].y1 - l->item[0].y0 + 1 == 3 * 18 && l->item[1].x0 > l->item[0].x1);     /* Paste large, Cut beside it */
  CHECK(l->item[2].y0 == l->item[1].y1 + 1);                                         /* Copy under Cut */
  for (i = 0, ok = 1; i < l->nitem; i++) if (l->item[i].x1 >= 640 || l->item[i].x0 < 0) ok = 0;
  CHECK(ok);
  CHECK(cs_rb_hit(l, l->item[1].x0 + 2, l->item[1].y0 + 2, &k) == CS_HIT_ITEM && k == 1);
  CHECK(cs_rb_hit(l, l->item[4].x1 - 2, l->item[4].y0 + 2, &k) == CS_HIT_ARROW && k == 4);
  CHECK(cs_rb_hit(l, l->tell.x0 + 2, l->tell.y0 + 2, &k) == CS_HIT_TELL);
  CHECK(cs_rb_hit(l, l->file.x0 + 2, 3, &k) == CS_HIT_FILE);
  cs_rb_keytips_tabs(&def, l, codes);
  CHECK(!strcmp(codes[0], "F") && !strcmp(codes[1], "H") && !codes[2][0] && !strcmp(codes[3], "1") && !strcmp(codes[5], "Q"));
  cs_rb_keytips_items(&def, l, ic);
  CHECK(!strcmp(ic[0], "P") && ic[1][0] == 'C' && ic[2][0] == 'C' && strcmp(ic[1], ic[2]));
  /* the contextual tab shows when its command is checked */
  table_on = 1;
  cs_rb_layout(&def, 1, 640, 0, 0, &m, l);
  CHECK(l->tab == 1 && l->tabbox[1].x0 > l->tabbox[0].x1);
  table_on = 0;
  cs_rb_layout(&def, 1, 640, 0, 0, &m, l);
  CHECK(l->tab == 0);
  /* narrow: squeezed, then groups collapse from the right */
  cs_rb_layout(&def, 0, 200, 0, 0, &m, l);
  CHECK(l->squeeze && l->collapsed[2] && !l->overflow);
  CHECK(cs_rb_hit(l, l->item[l->nitem - 1].x0 + 2, l->item[l->nitem - 1].y0 + 2, &k) == CS_HIT_GROUP);
  for (i = 0, ok = 1; i < l->nitem; i++) if (l->item[i].x1 > 197) ok = 0;
  CHECK(ok);
  /* right to left: mirrored */
  cs_rb_layout(&def, 0, 640, 1, 0, &m, l);
  CHECK(l->file.x1 == 637 && l->tell.x0 == 2 && l->item[1].x1 < l->item[0].x0);
  h = l->height;
  cs_rb_layout(&def, 0, 640, 0, 1, &m, l);
  CHECK(l->nitem == 0 && l->height < h);
  free(l);
}
int main(int argc, char **argv)
{
  test_commands();
  test_menus();
  test_lang();
  test_prefs(argc > 1 ? argv[1] : "/tmp");
  test_progress();
  test_wrap();
  test_keytips();
  test_icons();
  test_ribbon();
  printf("%d tests, %d failed\n", tests, failed);
  return failed != 0;
}

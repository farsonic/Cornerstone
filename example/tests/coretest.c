/*
 * coretest.c - Jotter's native tests: the document (core/text.c: editing, undo and redo, finding
 * and replacing, the line commands, counts, the outline, files) and the interface's data (the
 * command table, menus and ribbon of ui/commands.c checked against the kit: every command known,
 * the menus within Intuition's limits, the ribbon laid out at 640 pixels, every word translated).
 * Built with the sanitizers by tools/test.sh.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../core/text.h"
#include "../ui/commands.h"
#include "cs_lang.h"
#include "cs_util.h"

static int tests, failed;
#define CHECK(c) do { tests++; if (!(c)) { failed++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static const CsWords words[] = {
#include "../amiga/lang_table.h"
  { { NULL } }
};

/* the document's whole text equals s */
static int is(TextDoc *t, const char *s)
{
  long n;
  char *g = text_get(t, &n);
  int ok = g && !strcmp(g, s);
  if (!ok) printf("  text is \"%s\", not \"%s\"\n", g ? g : "(null)", s);
  free(g);
  return ok;
}
static void type(TextDoc *t, const char *s) { while (*s) { text_insert(t, s, 1); s++; } }
static TPos at(long l, long c) { TPos p; p.line = l; p.col = c; return p; }

static void test_edit(void)
{
  TextDoc t;
  CHECK(text_init(&t) && t.nlines == 1 && is(&t, ""));
  type(&t, "Hello");
  CHECK(is(&t, "Hello") && t.caret.col == 5 && t.modified);
  text_insert(&t, "\nworld\nagain", 12);
  CHECK(is(&t, "Hello\nworld\nagain") && t.nlines == 3 && t.caret.line == 2 && t.caret.col == 5);
  text_set_caret(&t, at(1, 0), 0);
  text_backspace(&t);
  CHECK(is(&t, "Helloworld\nagain") && t.caret.line == 0 && t.caret.col == 5);
  text_delete(&t);
  CHECK(is(&t, "Helloorld\nagain"));
  /* a selection across lines, replaced by typing */
  text_set_caret(&t, at(0, 2), 0);
  text_set_caret(&t, at(1, 2), 1);
  CHECK(text_has_sel(&t));
  { long n; char *s = text_sel_get(&t, &n); CHECK(s && !strcmp(s, "lloorld\nag") && n == 10); free(s); }
  text_insert(&t, "X", 1);
  CHECK(is(&t, "HeXain"));
  /* undo: the typing, then the replacement as one step, then the rest */
  CHECK(text_undo(&t) && is(&t, "Helloorld\nagain"));
  CHECK(text_redo(&t) && is(&t, "HeXain"));
  while (text_undo(&t)) ;
  CHECK(is(&t, "") && !text_can_undo(&t) && text_can_redo(&t));
  while (text_redo(&t)) ;
  CHECK(is(&t, "HeXain"));
  /* typing joins into one step; moving the caret starts another */
  text_free(&t); text_init(&t);
  type(&t, "abc");
  text_move(&t, MV_LEFT, 0);
  type(&t, "de");
  CHECK(is(&t, "abdec"));
  text_undo(&t);
  CHECK(is(&t, "abc"));
  text_undo(&t);
  CHECK(is(&t, ""));
  /* a new edit drops what could be redone */
  text_insert(&t, "z", 1);
  CHECK(!text_can_redo(&t));
  text_free(&t);
}
static void test_move(void)
{
  TextDoc t;
  text_init(&t);
  text_insert(&t, "one two  three\nfour", 19);
  text_set_caret(&t, at(0, 0), 0);
  text_move(&t, MV_WORDRIGHT, 0); CHECK(t.caret.col == 4);
  text_move(&t, MV_WORDRIGHT, 0); CHECK(t.caret.col == 9);
  text_move(&t, MV_WORDLEFT, 0); CHECK(t.caret.col == 4);
  text_move(&t, MV_END, 1); CHECK(t.caret.col == 14 && t.anchor.col == 4);
  text_move(&t, MV_LEFT, 0); CHECK(t.caret.col == 4 && !text_has_sel(&t));     /* Left with a selection: to its start */
  text_move(&t, MV_DOWN, 0); CHECK(t.caret.line == 1 && t.caret.col == 4);       /* kept to the shorter line */
  text_move(&t, MV_DOWN, 0); CHECK(t.caret.line == 1 && t.caret.col == 4);
  text_select_word(&t, at(0, 10));
  { long n; char *s = text_sel_get(&t, &n); CHECK(!strcmp(s, "three")); free(s); }
  text_select_all(&t);
  CHECK(text_offset(&t, t.caret) == 19 && text_pos_at(&t, 16).line == 1 && text_pos_at(&t, 16).col == 1);
  text_free(&t);
}
static void test_find(void)
{
  TextDoc t;
  text_init(&t);
  text_insert(&t, "The cat sat.\nA Cat, a catalogue.\ncat", 36);
  text_set_caret(&t, at(0, 0), 0);
  CHECK(text_find(&t, "cat", 0) && t.anchor.line == 0 && t.anchor.col == 4);
  CHECK(text_find(&t, "cat", 0) && t.anchor.line == 1 && t.anchor.col == 2);          /* any case */
  CHECK(text_find(&t, "cat", FIND_CASE) && t.anchor.line == 1 && t.anchor.col == 9);
  CHECK(text_find(&t, "cat", FIND_WORD) && t.anchor.line == 2);                        /* not "catalogue" */
  CHECK(text_find(&t, "cat", FIND_WORD) && t.anchor.line == 0);                        /* round the end */
  CHECK(text_find(&t, "cat", FIND_BACK) && t.anchor.line == 2);
  CHECK(!text_find(&t, "dog", 0));
  CHECK(text_replace_all(&t, "cat", "dog", FIND_WORD) == 3);
  CHECK(is(&t, "The dog sat.\nA dog, a catalogue.\ndog"));
  CHECK(text_undo(&t) && is(&t, "The cat sat.\nA Cat, a catalogue.\ncat"));             /* one step */
  text_set_caret(&t, at(0, 0), 0);
  text_find(&t, "sat", 0);
  CHECK(text_replace(&t, "sat", "lay", 0) && is(&t, "The cat lay.\nA Cat, a catalogue.\ncat"));
  CHECK(text_replace_all(&t, "a", "", 0) == 8);
  text_free(&t);
}
static void test_lines(void)
{
  TextDoc t;
  TCount c;
  TOutline o[8];
  text_init(&t);
  text_insert(&t, "pear\nApple\n\nbanana", 18);
  text_set_caret(&t, at(0, 0), 0);
  CHECK(text_sort_lines(&t) && is(&t, "\nApple\nbanana\npear"));
  CHECK(text_undo(&t) && is(&t, "pear\nApple\n\nbanana"));
  text_set_caret(&t, at(0, 1), 0); text_set_caret(&t, at(1, 2), 1);
  CHECK(text_indent(&t, 0) && is(&t, "  pear\n  Apple\n\nbanana"));
  CHECK(text_indent(&t, 1) && is(&t, "pear\nApple\n\nbanana"));
  text_set_caret(&t, at(3, 2), 0);
  CHECK(text_case(&t, 1) && is(&t, "pear\nApple\n\nBANANA"));                         /* the word at the caret */
  text_select_all(&t);
  CHECK(text_case(&t, 0) && is(&t, "pear\napple\n\nbanana"));
  text_count(&t, &c);
  CHECK(c.words == 3 && c.lines == 4 && c.chars == 18 && c.paragraphs == 2);
  text_free(&t); text_init(&t);
  text_insert(&t, "# One\ntext\n## Two\n#not\n### Three", 32);
  CHECK(text_outline(&t, o, 8) == 3 && o[0].line == 0 && o[0].level == 1 && o[1].level == 2 && o[2].line == 4);
  text_free(&t);
}
static void test_files(const char *dir)
{
  char path[300];
  FILE *f;
  TextDoc t;
  snprintf(path, sizeof path, "%s/jotter_test.txt", dir);
  /* UTF-8 in, Latin-1 kept; CR LF kept for saving */
  f = fopen(path, "wb");
  fputs("Gr\303\274\303\237e\r\nzwei \342\202\254\r\n", f);
  fclose(f);
  text_init(&t);
  CHECK(text_load(&t, path) && t.crlf && t.nlines == 3 && !strcmp(t.lines[0].s, "Gr\374\337e") && !strcmp(t.lines[1].s, "zwei ?"));
  CHECK(!t.modified);
  CHECK(text_save(&t, path));
  f = fopen(path, "rb");
  { char b[40]; size_t n = fread(b, 1, sizeof b, f); CHECK(n == 15 && !memcmp(b, "Gr\374\337e\r\nzwei ?\r\n", 15)); }
  fclose(f);
  /* Latin-1 in (not valid UTF-8): as it is */
  f = fopen(path, "wb"); fputs("caf\351\n", f); fclose(f);
  CHECK(text_load(&t, path) && !strcmp(t.lines[0].s, "caf\351") && !t.crlf);
  CHECK(!text_load(&t, "/nonexistent/x.txt"));
  remove(path);
  text_free(&t);
}
static int problems;
static void report(const char *l) { printf("  %s\n", l); problems++; }
static long w6(void *c, const char *s) { (void)c; return 6 * (long)strlen(s); }
static void test_ui(void)
{
  int i, t, g, k, lang;
  CsRbLayout *l = (CsRbLayout *)malloc(sizeof *l);
  CsRbMetrics m = { 11, w6, NULL };
  jotter_commands_init();
  CHECK(cs_menu_check(jotter_menus, report) == 0);
  /* every ribbon item, Quick Access button and File page entry is a command of the table */
  for (t = 0, k = 0; t < jotter_ribbon.ntabs; t++)
    for (g = 0; g < jotter_ribbon.tabs[t].n; g++)
      for (i = 0; i < jotter_ribbon.tabs[t].groups[g].n; i++) if (!cs_cmd_find(jotter_ribbon.tabs[t].groups[g].items[i].id)) k++;
  for (i = 0; i < jotter_ribbon.nqa; i++) if (!cs_cmd_find(jotter_ribbon.qa[i])) k++;
  for (i = 0; jotter_filepage[i]; i++) if (jotter_filepage[i] != ~0UL && !cs_cmd_find(jotter_filepage[i])) k++;
  for (i = 0; i < jotter_nctx_view; i++) if (jotter_ctx_view[i] && !cs_cmd_find(jotter_ctx_view[i])) k++;
  CHECK(k == 0);
  /* the command blocks don't run into each other */
  CHECK(ID_LAST_FILE < ID_UNDO && ID_LAST_EDIT < ID_DATE && ID_LAST_INSERT < ID_PANE && ID_LAST_VIEW < ID_VIEWCHG);
  /* every visible command has an icon that the icon language accepts, and help */
  for (i = 0, k = 0; i < jotter_ncommands; i++) {
    const CsCommand *c = &jotter_commands[i];
    if (c->flags & CSC_HIDDEN) continue;
    if (c->icon && cs_icon_check(cs_icon_src(c->icon))) { printf("  icon of %s\n", c->label); k++; }
    if (!c->help || !c->help[0]) k++;
  }
  CHECK(k == 0);
  for (i = 0; i < JOTTER_NICONS; i++) CHECK(cs_icon_check(jotter_icons[i]) == 0);
  /* every tab lays out at 640 pixels in every language without overflowing (6 pixels a letter:
     kit/amiga/tools/ribbonfit.py does the same with the real font) */
  cs_lang_init(words, (int)(sizeof words / sizeof words[0]) - 1);
  for (lang = 0, k = 0; lang < CS_NLANG; lang++) {
    cs_lang_set(lang);
    for (t = 0; t < jotter_ribbon.ntabs; t++) { cs_rb_layout(&jotter_ribbon, t, 618, 0, 0, &m, l); if (l->overflow) k++; }
  }
  CHECK(k == 0);
  /* the words: every command's label and help translated in every language */
  for (lang = 1, k = 0; lang < CS_NLANG; lang++) {
    cs_lang_set(lang);
    for (i = 0; i < jotter_ncommands; i++) {
      const CsCommand *c = &jotter_commands[i];
      if (c->flags & CSC_HIDDEN) continue;
      if (tr(c->label) == c->label && strcmp(c->label, "File")) { printf("  %s: \"%s\" untranslated\n", cs_lang_code(lang), c->label); k++; }
    }
  }
  CHECK(k == 0);
  /* Tell Me finds in the language shown */
  {
    unsigned long ids[8];
    cs_lang_set(CS_LANG_DE);
    CHECK(cs_cmd_search("umbruch", ids, 8) >= 1 && ids[0] == ID_WRAP);
    cs_lang_set(CS_LANG_EN);
    CHECK(cs_cmd_search("save", ids, 8) == 3 && ids[0] == ID_SAVE && ids[1] == ID_SAVEAS && ids[2] == ID_REVERT);
  }
  free(l);
}
int main(int argc, char **argv)
{
  test_edit();
  test_move();
  test_find();
  test_lines();
  test_files(argc > 1 ? argv[1] : "/tmp");
  test_ui();
  printf("%d tests, %d failed\n", tests, failed);
  return failed != 0;
}

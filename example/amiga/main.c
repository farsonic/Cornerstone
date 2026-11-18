/*
 * main.c - Jotter, Cornerstone's example program: a plain-text editor laid out the family's way.
 * It is built only on the kit (kit/shared and kit/amiga); docs/platforms/amiga/16-walkthrough.md
 * goes through this file part by part. In order:
 *   the globals and preferences; the command state the menus and ribbon share; the window (the
 *   ribbon, the outline pane, the text view with its scroll bars, the status bar, the File page);
 *   refreshing what shows the document; the commands; files; and main(): start, loop, end.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "jotter.h"
#include "cs_app.h"
#include "cs_theme.h"
#include "cs_classes.h"
#include "cs_dialog.h"
#include "cs_menu.h"
#include "cs_ctx.h"
#include "cs_jobs.h"
#include "cs_lang.h"
#include "cs_util.h"
#include "../ui/commands.h"

const char verstag[] = "$VER: Jotter " JOTTER_VERSION " (" JOTTER_DATE ") Cornerstone's example program";
long __stack = 32768;                                 /* (vbcc: the stack Jotter asks for) */

/* the interface's words in five languages: rows made by mklang.py from lang/strings.txt */
static const CsWords words[] = {
#include "lang_table.h"
  { { NULL } }
};

Object *app, *win, *ribbon, *view, *status;
static Object *strip, *pages, *pane, *balance, *outline, *outlist, *vscroll, *hscroll, *backstage, *infotext, *panehead;
TextDoc doc;
Prefs prefs = { 0, 0, 1, 1, 1, 0, 1, 0 };
static TOutline heads[200];
static int nheads;
static char headlines[200][72];

/* ---------------------------------------------------------------- preferences */
static void prefs_read(void)
{
  cs_prefs_load(cs_prefs_path("Jotter"));
  prefs.dark = (int)cs_prefs_int("dark", 0);
  prefs.large = (int)cs_prefs_int("large", 0);
  prefs.tips = (int)cs_prefs_int("tips", 1);
  prefs.pane = (int)cs_prefs_int("pane", 1);
  prefs.status = (int)cs_prefs_int("status", 1);
  prefs.wrap = (int)cs_prefs_int("wrap", 0);
  prefs.size = (int)cs_prefs_int("size", 1);
  prefs.rtl = (int)cs_prefs_int("rtl", 0);           /* (a test switch: the right-to-left layout) */
}
void prefs_write(void)
{
  cs_prefs_set_int("dark", prefs.dark); cs_prefs_set_int("large", prefs.large); cs_prefs_set_int("tips", prefs.tips);
  cs_prefs_set_int("pane", prefs.pane); cs_prefs_set_int("status", prefs.status); cs_prefs_set_int("wrap", prefs.wrap);
  cs_prefs_set_int("size", prefs.size);
  if (!cs_prefs_save(cs_prefs_path("Jotter"))) CS_LOG("prefs: couldn't write %s", cs_prefs_path("Jotter"));
}

/* ---------------------------------------------------------------- the commands' state */
/* one answer for the menus, the ribbon, the Quick Access buttons, the File page and the context
   menu: whether a command can be used now, and whether it is ticked */
static int state(unsigned long id)
{
  int on = CSS_ENABLED;
  switch (id) {
  case ID_UNDO: return text_can_undo(&doc) ? on : 0;
  case ID_REDO: return text_can_redo(&doc) ? on : 0;
  case ID_CUT: case ID_COPY: case ID_DELETE: return text_has_sel(&doc) ? on : 0;
  case ID_REVERT: return doc.path[0] && doc.modified ? on : 0;
  case ID_FINDNEXT: case ID_FINDPREV: return find_what[0] ? on : 0;
  case ID_PANE: return on | (prefs.pane ? CSS_CHECKED : 0);
  case ID_STATUSBAR: return on | (prefs.status ? CSS_CHECKED : 0);
  case ID_WRAP: return on | (prefs.wrap ? CSS_CHECKED : 0);
  case ID_DARK: return on | (prefs.dark ? CSS_CHECKED : 0);
  case ID_MINIMISE: return on | (cs_get(ribbon, MUIA_CsRibbon_Minimised) ? CSS_CHECKED : 0);
  }
  return on;
}

/* ---------------------------------------------------------------- the window */
static Object *make_window(void)
{
  struct Screen *scr = LockPubScreen(cs_pubscreen[0] ? (STRPTR)cs_pubscreen : NULL);
  LONG sw = scr ? scr->Width : 640, sh = scr ? scr->Height - scr->BarHeight - 1 : 480, top = scr ? scr->BarHeight + 1 : 0;
  Object *w, *main_page, *file_page;
  if (scr) UnlockPubScreen(NULL, scr);
  vscroll = cs_scrollbar(0);
  hscroll = cs_scrollbar(1);
  view = NewObject(view_class()->mcc_Class, NULL, MUIA_FillArea, FALSE, MUIA_View_Text, (ULONG)&doc, MUIA_View_VScroll, (ULONG)vscroll,
                   MUIA_View_HScroll, (ULONG)hscroll, MUIA_View_Wrap, prefs.wrap, MUIA_View_FontSize, prefs.size, TAG_DONE);
  outline = ListviewObject, MUIA_Listview_List, outlist = ListObject, InputListFrame, MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                                                 MUIA_List_DestructHook, MUIV_List_DestructHook_String, End, MUIA_CycleChain, 1, End;
  pane = cs_pane("Outline", outline, &panehead);
  balance = BalanceObject, End;
  ribbon = NewObject(cs_ribbon_class()->mcc_Class, NULL, MUIA_CsRibbon_Def, (ULONG)&jotter_ribbon, MUIA_ShortHelp, (ULONG)" ", TAG_DONE);
  status = NewObject(cs_status_class()->mcc_Class, NULL, MUIA_CsStatus_Fields, (ULONG)"L0 R70 R100 R130", TAG_DONE);
  backstage = NewObject(cs_backstage_class()->mcc_Class, NULL, MUIA_CsBackstage_Ids, (ULONG)jotter_filepage, TAG_DONE);
  main_page = VGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0,
    Child, ribbon,
    Child, HGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0,
      Child, pane,
      Child, balance,
      Child, VGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0,
        Child, HGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0, Child, view, Child, vscroll, End,
        Child, hscroll,
      End,
    End,
    Child, status,
  End;
  file_page = HGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0, MUIA_Background, (ULONG)cs_dialog_back(),
    Child, backstage,
    Child, VGroup, InnerSpacing(16, 16),
      Child, TextObject, MUIA_Font, MUIV_Font_Big, MUIA_Text_Contents, (ULONG)tr("Info"), End,
      Child, infotext = TextObject, MUIA_Text_Contents, (ULONG)"", MUIA_Text_SetVMax, FALSE, End,
      Child, VSpace(0),
    End,
  End;
  w = CsWindowObject,
    MUIA_Window_Title, (ULONG)"Jotter",                /* lang: as is (refresh_all sets the real one) */
    MUIA_Window_ID, MAKE_ID('J', 'O', 'T', 'R'),
    MUIA_Window_Menustrip, (ULONG)strip,
    MUIA_Window_Width, sw, MUIA_Window_Height, sh, MUIA_Window_LeftEdge, 0, MUIA_Window_TopEdge, top,
    WindowContents, pages = VGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0, MUIA_Group_PageMode, TRUE,
      Child, main_page,
      Child, file_page,
    End,
  End;
  if (!w) return NULL;
  /* the classes report through notifications; the main loop gets each as a command id */
  DoMethod(w, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (ULONG)app, 2, MUIM_Application_ReturnID, ID_QUIT);
  DoMethod(ribbon, MUIM_Notify, MUIA_CsRibbon_Command, MUIV_EveryTime, (ULONG)app, 2, MUIM_Application_ReturnID, MUIV_TriggerValue);
  DoMethod(backstage, MUIM_Notify, MUIA_CsBackstage_Pick, MUIV_EveryTime, (ULONG)app, 2, MUIM_Application_ReturnID, ID_BACKSTAGE);
  DoMethod(view, MUIM_Notify, MUIA_View_Changed, MUIV_EveryTime, (ULONG)app, 2, MUIM_Application_ReturnID, ID_VIEWCHG);
  DoMethod(status, MUIM_Notify, MUIA_CsStatus_Click, MUIV_EveryTime, (ULONG)app, 2, MUIM_Application_ReturnID, ID_STATUSCLICK);
  DoMethod(panehead, MUIM_Notify, MUIA_CsPane_Close, MUIV_EveryTime, (ULONG)app, 2, MUIM_Application_ReturnID, ID_PANECLOSE);
  DoMethod(outlist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (ULONG)app, 2, MUIM_Application_ReturnID, ID_OUTLINEPICK);
  DoMethod(outline, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (ULONG)app, 2, MUIM_Application_ReturnID, ID_OUTLINEPICK);
  DoMethod(w, MUIM_Notify, MUIA_Window_InputEvent, (ULONG)"esc", (ULONG)app, 2, MUIM_Application_ReturnID, ID_BACKSTAGE);
  /* the scroll bars move the view; the view keeps them in step itself */
  DoMethod(vscroll, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (ULONG)view, 3, MUIM_Set, MUIA_View_Top, MUIV_TriggerValue);
  DoMethod(hscroll, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (ULONG)view, 3, MUIM_Set, MUIA_View_Left, MUIV_TriggerValue);
  set(pane, MUIA_ShowMe, prefs.pane); set(balance, MUIA_ShowMe, prefs.pane);
  set(status, MUIA_ShowMe, prefs.status);
  set(hscroll, MUIA_ShowMe, !prefs.wrap);
  return w;
}

/* ---------------------------------------------------------------- refreshing */
static int file_page_open(void) { return cs_get(pages, MUIA_Group_ActivePage) == 1; }
/* the outline pane's list, when the headings have changed */
static void refresh_outline(void)
{
  static TOutline old[200];
  static int nold = -1;
  int i, same;
  nheads = text_outline(&doc, heads, 200);
  same = nheads == nold;
  for (i = 0; same && i < nheads; i++) {
    const TLine *l = &doc.lines[heads[i].line];
    same = old[i].line == heads[i].line && !strncmp(headlines[i] + 2 * (heads[i].level - 1), l->s + heads[i].level + (l->len > heads[i].level), 60);
  }
  if (same) return;
  set(outline, MUIA_List_Quiet, TRUE);
  DoMethod(outline, MUIM_List_Clear);
  for (i = 0; i < nheads; i++) {
    const TLine *l = &doc.lines[heads[i].line];
    int k, lv = heads[i].level;
    for (k = 0; k < 2 * (lv - 1) && k < 10; k++) headlines[i][k] = ' ';
    cs_strlcpy(headlines[i] + k, l->s + lv + (l->len > lv), sizeof headlines[i] - (size_t)k);
    DoMethod(outline, MUIM_List_InsertSingle, (ULONG)headlines[i], MUIV_List_Insert_Bottom);
    old[i] = heads[i];
  }
  nold = nheads;
  set(outline, MUIA_List_Quiet, FALSE);
}
void refresh_all(void)
{
  static char title[300], s1[80], s2[80], s3[40];
  const char *name = doc.path[0] ? (const char *)FilePart((STRPTR)doc.path) : tr("Untitled");
  TCount c;
  snprintf(title, sizeof title, "Jotter - %s%s", name, doc.modified ? " *" : "");
  set(win, MUIA_Window_Title, (ULONG)title);
  text_count(&doc, &c);
  snprintf(s1, sizeof s1, tr("Line %ld, Column %ld"), doc.caret.line + 1, doc.caret.col + 1);
  snprintf(s2, sizeof s2, tr("%ld words"), c.words);
  cs_strlcpy(s3, tr(textsizes[prefs.size]), sizeof s3);
  DoMethod(status, MUIM_CsStatus_Set, 0, (ULONG)(doc.modified ? tr("Modified") : doc.path[0] ? tr("Saved") : ""));
  DoMethod(status, MUIM_CsStatus_Set, 1, (ULONG)s3);
  DoMethod(status, MUIM_CsStatus_Set, 2, (ULONG)s2);
  DoMethod(status, MUIM_CsStatus_Set, 3, (ULONG)s1);
  refresh_outline();
  cs_menu_update(strip);
  DoMethod(ribbon, MUIM_CsRibbon_Refresh);
  if (file_page_open()) {
    static char info[600];
    snprintf(info, sizeof info, tr("Name: %s\nLines: %ld\nWords: %ld\nCharacters: %ld (%ld without spaces)\nParagraphs: %ld\nLine ends: %s"),
             doc.path[0] ? doc.path : tr("Untitled"), c.lines, c.words, c.chars, c.chars_nospace, c.paragraphs, doc.crlf ? "CR LF" : "LF");
    set(infotext, MUIA_Text_Contents, (ULONG)info);
  }
}

/* ---------------------------------------------------------------- files */
/* there are no changes, or the user has saved them or let them go: 1 to carry on */
static int may_discard(void)
{
  int r;
  if (!doc.modified) return 1;
  r = cs_ask("_Save|_Don't Save|_Cancel", "The document has changes that aren't saved. Save them first?", NULL);
  if (r == 1) { jotter_command(ID_SAVE, -1); return !doc.modified; }
  return r == 2;
}
static void load(const char *path)
{
  int ok;
  cs_job_begin("Open...");
  ok = text_load(&doc, path);
  cs_job_end();
  if (!ok) { cs_ask("OK", "Jotter couldn't open %s.", path); return; }
  DoMethod(view, MUIM_View_Refresh);
}
static int save_as(void)
{
  char path[256];
  cs_strlcpy(path, doc.path[0] ? doc.path : "RAM:Untitled.txt", sizeof path);
  if (!cs_file_request(1, "Save As", path, sizeof path, "#?.txt")) return 0;
  if (!text_save(&doc, path)) { cs_ask("OK", "Jotter couldn't save %s.", path); return 0; }
  return 1;
}
static void insert_now(const char *fmt)
{
  char b[64];
  time_t now = time(NULL);
  strftime(b, sizeof b, fmt, localtime(&now));
  text_insert(&doc, b, (long)strlen(b));
  text_break_join(&doc);
}
/* the caret's line made a heading of level lv (1-3), or plain text (0) */
static void set_heading(int lv)
{
  TLine *l = &doc.lines[doc.caret.line];
  char b[300];
  long k = 0, n = 0;
  while (k < l->len && l->s[k] == '#') k++;
  if (k && k < l->len && l->s[k] == ' ') k++;
  while (n < lv && n < 6) b[n++] = '#';
  if (lv) b[n++] = ' ';
  cs_strlcpy(b + n, l->s + k, sizeof b - (size_t)n);
  doc.anchor.line = doc.caret.line; doc.anchor.col = 0; doc.caret.col = l->len;
  text_insert(&doc, b, (long)strlen(b));
  text_break_join(&doc);
}
static void show_file_page(int on)
{
  set(pages, MUIA_Group_ActivePage, on ? 1 : 0);
  if (on) set(backstage, MUIA_CsBackstage_Active, ID_INFO);
  refresh_all();
}

/* ---------------------------------------------------------------- commands */
/* every command, whichever way it came (menu, ribbon, key, context menu, Tell Me, CMD=); value is
   a ribbon list's entry, -1 when none */
void jotter_command(ULONG id, LONG value)
{
  char *s;
  long n;
  CS_LOG("command %lu (%ld)", id, value);
  if (!(cs_cmd_state(id) & CSS_ENABLED)) return;     /* (a shortcut or CMD= reaches here too) */
  switch (id) {
  case ID_NEW: if (may_discard()) { text_free(&doc); text_init(&doc); DoMethod(view, MUIM_View_Refresh); } break;
  case ID_OPEN: {
    char path[256];
    cs_strlcpy(path, doc.path, sizeof path);
    if (may_discard() && cs_file_request(0, "Open", path, sizeof path, "#?")) load(path);
    break; }
  case ID_SAVE: if (!doc.path[0]) save_as(); else if (!text_save(&doc, doc.path)) cs_ask("OK", "Jotter couldn't save %s.", doc.path); break;
  case ID_SAVEAS: save_as(); break;
  case ID_REVERT:
    if (cs_ask("_Revert|_Cancel", "Put the document back as it was last saved? The changes since are lost.", NULL)) { char p[256]; cs_strlcpy(p, doc.path, sizeof p); load(p); }
    break;
  case ID_INFO: show_file_page(1); return;
  case ID_FILEPAGE: show_file_page(!file_page_open()); return;
  case ID_OPTIONS: options_dialog(); break;
  case ID_ABOUT: about_box(); break;
  case ID_UNDO: text_undo(&doc); DoMethod(view, MUIM_View_Refresh); break;
  case ID_REDO: text_redo(&doc); DoMethod(view, MUIM_View_Refresh); break;
  case ID_CUT: case ID_COPY:
    if ((s = text_sel_get(&doc, &n))) { if (!clip_write(s, n)) cs_ask("OK", "The clipboard couldn't be written to.", NULL); free(s); }
    if (id == ID_CUT) { text_delete_sel(&doc); DoMethod(view, MUIM_View_Refresh); }
    break;
  case ID_PASTE:
    if ((s = clip_read(&n))) {
      long i, o;
      for (i = 0, o = 0; i < n; i++) if (s[i] != '\r' && s[i]) s[o++] = s[i];
      text_insert(&doc, s, o); text_break_join(&doc); free(s);
      DoMethod(view, MUIM_View_Refresh);
    }
    break;
  case ID_DELETE: text_delete_sel(&doc); DoMethod(view, MUIM_View_Refresh); break;
  case ID_SELECTALL: text_select_all(&doc); DoMethod(view, MUIM_View_Refresh); break;
  case ID_FIND:
    if (value == 1) { find_again(0); break; }        /* (the Find button's list: Find Next) */
    if (value == 2) { find_again(1); break; }
    find_dialog(0); break;
  case ID_FINDNEXT: find_again(0); break;
  case ID_FINDPREV: find_again(1); break;
  case ID_REPLACE: find_dialog(1); break;
  case ID_GOTO: {
    LONG line = doc.caret.line + 1;
    if (cs_ask_number("Go To Line", "Line _number", &line, 1, doc.nlines)) {
      TPos p; p.line = line - 1; p.col = 0;
      text_set_caret(&doc, p, 0);
      DoMethod(view, MUIM_View_Refresh);
    }
    break; }
  case ID_UPPER: case ID_LOWER: text_case(&doc, id == ID_UPPER); DoMethod(view, MUIM_View_Refresh); break;
  case ID_SORT: cs_job_begin("Sort Lines"); text_sort_lines(&doc); cs_job_end(); DoMethod(view, MUIM_View_Refresh); break;
  case ID_INDENT: case ID_OUTDENT: text_indent(&doc, id == ID_OUTDENT); DoMethod(view, MUIM_View_Refresh); break;
  case ID_DATE: insert_now("%Y-%m-%d"); DoMethod(view, MUIM_View_Refresh); break;
  case ID_TIME: insert_now("%H:%M"); DoMethod(view, MUIM_View_Refresh); break;
  case ID_HEADING: set_heading(value < 0 ? 1 : value >= 3 ? 0 : (int)value + 1); DoMethod(view, MUIM_View_Refresh); break;
  case ID_RULE: text_insert(&doc, "\n----------------------------------------\n", 42); text_break_join(&doc); DoMethod(view, MUIM_View_Refresh); break;
  case ID_PANE: case ID_PANECLOSE:
    prefs.pane = id == ID_PANECLOSE ? 0 : !prefs.pane;
    set(pane, MUIA_ShowMe, prefs.pane); set(balance, MUIA_ShowMe, prefs.pane);
    break;
  case ID_STATUSBAR: prefs.status = !prefs.status; set(status, MUIA_ShowMe, prefs.status); break;
  case ID_WRAP: prefs.wrap = !prefs.wrap; set(hscroll, MUIA_ShowMe, !prefs.wrap); set(view, MUIA_View_Wrap, prefs.wrap); break;
  case ID_TEXTSIZE: prefs.size = value >= 0 && value <= 2 ? (int)value : (prefs.size + 1) % 3; set(view, MUIA_View_FontSize, prefs.size); break;
  case ID_DARK:
    prefs.dark = !prefs.dark;
    cs_theme_set_dark(prefs.dark);
    cs_redraw_window();                              /* every themed object draws again in its new pens */
    break;
  case ID_MINIMISE: set(ribbon, MUIA_CsRibbon_Minimised, !cs_get(ribbon, MUIA_CsRibbon_Minimised)); break;
  case ID_TELLME: cs_tellme_open(ribbon); break;
  case ID_KEYTIPS: DoMethod(ribbon, MUIM_CsRibbon_KeyTips); break;
  }
  refresh_all();
}
/* what the classes report (not commands of the table's menus) */
static void report(ULONG id)
{
  switch (id) {
  case ID_VIEWCHG: refresh_all(); break;
  case ID_OUTLINEPICK: {
    LONG a = (LONG)cs_get(outline, MUIA_List_Active);
    if (a >= 0 && a < nheads) {
      TPos p; p.line = heads[a].line; p.col = 0;
      text_set_caret(&doc, p, 0);
      DoMethod(view, MUIM_View_Refresh);
      set(win, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
      refresh_all();
    }
    break; }
  case ID_STATUSCLICK: {
    LONG f = (LONG)cs_get(status, MUIA_CsStatus_Click);
    if (f == 1) jotter_command(ID_TEXTSIZE, -1);
    else if (f == 2) jotter_command(ID_INFO, -1);
    else if (f == 3) jotter_command(ID_GOTO, -1);
    break; }
  case ID_BACKSTAGE: {
    /* the File page: its column's choice (0 Back), or Esc */
    ULONG pick = cs_get(backstage, MUIA_CsBackstage_Pick);
    int was = file_page_open();
    if (!was) break;
    nnset(backstage, MUIA_CsBackstage_Pick, ~0UL);     /* (read: so Esc next time isn't taken for it; nnset: no report of it) */
    if (pick == ~0UL || pick == 0) { show_file_page(0); break; }
    if (pick == ID_INFO) { set(backstage, MUIA_CsBackstage_Active, pick); refresh_all(); break; }
    show_file_page(0);
    jotter_command(pick, -1);
    break; }
  }
}

/* ---------------------------------------------------------------- start, loop, end */
static void report_line(const char *l) { CS_LOG("check: %s", l); }
int main(int argc, char **argv)
{
  CsArgs args;
  ULONG sigs = 0, id;
  int rc = 20;
  if (!cs_libs_open("Jotter")) goto done;
  cs_args(argc, argv, &args);
  CS_LOG("Jotter %s starting", JOTTER_VERSION);
  prefs_read();
  cs_lang_init(words, (int)(sizeof words / sizeof words[0]) - 1);
  cs_lang_choose(args.language);
  cs_rtl = prefs.rtl;
  cs_theme_set_dark(prefs.dark);
  cs_theme_set_large(prefs.large);
  cs_tips = prefs.tips;
  jotter_commands_init();
  cs_cmd_set_state(state);
  if (cs_menu_check(jotter_menus, report_line)) CS_LOG("the menu layout has problems (above)");
  cs_fonts_assign();
  if (!text_init(&doc) || !cs_classes_make() || !view_class()) { cs_ask("OK", "Jotter couldn't start: not enough memory.", NULL); goto done; }
  app = ApplicationObject,
    MUIA_Application_Title, (ULONG)"Jotter",
    MUIA_Application_Version, (ULONG)verstag,
    MUIA_Application_Copyright, (ULONG)"GPL-2.0-only",
    MUIA_Application_Description, (ULONG)tr("A plain-text editor"),
    MUIA_Application_Base, (ULONG)"JOTTER",
  End;
  if (!app) { printf("Jotter: MUI couldn't make the application.\n"); goto done; }
  cs_app_set(app, NULL);
  strip = cs_menu_strip(jotter_menus);
  if (!(win = make_window())) { cs_ask("OK", "Jotter couldn't make its window.", NULL); goto done; }
  DoMethod(app, OM_ADDMEMBER, (ULONG)win);
  cs_app_set(app, win);
  if (args.nfiles) load(args.files[0]);
  set(win, MUIA_Window_Open, TRUE);
  if (!cs_get(win, MUIA_Window_Open)) { cs_ask("OK", "Jotter's window couldn't open.", NULL); goto done; }
  if (!cs_fonts_ok()) cs_ask("OK", "Jotter's fonts drawer wasn't found, so it uses the system's font. (%s)", cs_fonts_why());
  set(win, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
  refresh_all();
  if (args.cmd) jotter_command(args.cmd, -1);         /* CMD=n: a command at once (testing) */
  for (;;) {
    id = cs_input(&sigs);
    if (id == MUIV_Application_ReturnID_Quit || id == ID_QUIT) { if (may_discard()) break; continue; }
    if (!id) continue;
    if (id >= ID_VIEWCHG) { report(id); continue; }
    {
      LONG value = (LONG)cs_get(ribbon, MUIA_CsRibbon_Value);
      const CsCommand *c = cs_cmd_find(id);
      set(ribbon, MUIA_CsRibbon_Value, -1);
      set(app, MUIA_Application_Sleep, TRUE);
      cs_job_begin(c && c->label ? c->label : "Working");
      jotter_command(id, value);
      cs_job_end();
      set(app, MUIA_Application_Sleep, FALSE);
    }
  }
  rc = 0;
  prefs_write();
done:
  if (app) { if (win) set(win, MUIA_Window_Open, FALSE); MUI_DisposeObject(app); }
  app = win = NULL;
  cs_app_set(NULL, NULL);
  cs_ctx_free();
  cs_classes_free();
  view_class_free();
  cs_menu_free();
  cs_theme_free();
  cs_jobs_free();
  cs_fonts_unassign();
  text_free(&doc);
  cs_prefs_clear();
  cs_libs_close();
  return rc;
}

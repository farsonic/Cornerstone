/*
 * dialogs.c - Jotter's dialogs, each built the kit's way (cs_dialog.h): Find and Replace, Options,
 * About. Labels on the left in a ColGroup(2), the field that matters first, OK and Cancel at the
 * bottom right; nothing taller than a 480-line screen allows.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "jotter.h"
#include "cs_dialog.h"
#include "cs_theme.h"
#include "cs_lang.h"
#include "cs_util.h"
#include "../ui/commands.h"

char find_what[128], replace_with[128];
int find_flags;

/* the next match, or a word to say there is none */
void find_again(int back)
{
  if (!find_what[0]) { find_dialog(0); return; }
  if (!text_find(&doc, find_what, (find_flags & ~FIND_BACK) | (back ? FIND_BACK : 0)))
    cs_ask("OK", "Jotter can't find \"%s\".", find_what);
  DoMethod(view, MUIM_View_Refresh);
}
/* Find (or Replace): the words, Match case, Whole words, the direction (radio buttons; a cycle
   gadget on Zune, which has no Radio class) */
void find_dialog(int replace)
{
  static const char *const dirs[] = { "Down", "Up", NULL };
  static const ULONG ends[] = { CS_ID_OK, CS_ID_EXTRA, 0 };
  Object *w, *what, *with = NULL, *mcase, *mword, *dir, *grid;
  ULONG id;
  long sel;
  char *s;
  /* the selection, if it is a few words on one line, is what to look for */
  if (text_has_sel(&doc) && (s = text_sel_get(&doc, &sel))) {
    if (sel < (long)sizeof find_what && !strchr(s, '\n')) cs_strlcpy(find_what, s, sizeof find_what);
    free(s);
  }
  grid = ColGroup(2),
    Child, CsLabel(tr("Find _what"), MUIO_Label_SingleFrame), Child, what = cs_string(find_what, sizeof find_what - 1),
  End;
  if (replace && grid) {
    DoMethod(grid, OM_ADDMEMBER, (ULONG)CsLabel(tr("Replace _with"), MUIO_Label_SingleFrame));
    DoMethod(grid, OM_ADDMEMBER, (ULONG)(with = cs_string(replace_with, sizeof replace_with - 1)));
  }
  {
    Object *body = VGroup,
      Child, grid,
      Child, HGroup,
        Child, VGroup, Child, cs_check("Match _case", (find_flags & FIND_CASE) != 0, &mcase),
                       Child, cs_check("Whole words _only", (find_flags & FIND_WORD) != 0, &mword), End,
        Child, HSpace(0),
        Child, VGroup, GroupFrameT(tr("Direction")), Child, dir = cs_radio(dirs), End,
      End,
    End;
    w = replace ? cs_dialog_window3("Replace", body, "_Replace", "Replace _All") : cs_dialog_window("Find", body, "_Find Next");
  }
  if (!w) return;
  set(dir, CS_RADIO_ACTIVE, (find_flags & FIND_BACK) ? 1 : 0);
  set(w, MUIA_Window_ActiveObject, (ULONG)what);
  DoMethod(what, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_OK);
  for (;;) {
    id = cs_run_modal(w, ends, NULL);
    if (id != CS_ID_OK && id != CS_ID_EXTRA) break;
    cs_strlcpy(find_what, (const char *)cs_get(what, MUIA_String_Contents), sizeof find_what);
    if (with) cs_strlcpy(replace_with, (const char *)cs_get(with, MUIA_String_Contents), sizeof replace_with);
    find_flags = (cs_get(mcase, MUIA_Selected) ? FIND_CASE : 0) | (cs_get(mword, MUIA_Selected) ? FIND_WORD : 0) |
                 (cs_get(dir, CS_RADIO_ACTIVE) ? FIND_BACK : 0);
    if (!find_what[0]) continue;
    if (!replace) { find_again(find_flags & FIND_BACK); if (text_has_sel(&doc)) break; continue; }
    if (id == CS_ID_EXTRA) {
      char n[24];
      sprintf(n, "%ld", text_replace_all(&doc, find_what, replace_with, find_flags));
      DoMethod(view, MUIM_View_Refresh);
      refresh_all();
      cs_ask("OK", "Jotter replaced %s times.", n);
      break;
    }
    if (!text_replace(&doc, find_what, replace_with, find_flags) && !text_has_sel(&doc)) cs_ask("OK", "Jotter can't find \"%s\".", find_what);
    DoMethod(view, MUIM_View_Refresh);
    refresh_all();
  }
  MUI_DisposeObject(w);
}
/* Options: the interface language (from the next start), the theme, large text, bubbles */
void options_dialog(void)
{
  static const char *themes[] = { "Light", "Dark", NULL };
  static const char *langs[CS_NLANG + 2];
  static const ULONG ends[] = { CS_ID_OK, 0 };
  Object *w, *lang, *theme, *large, *tips, *size;
  int i, cur = cs_lang_by_code(cs_prefs_get("language", "")) + 1;
  langs[0] = "Automatic (Locale prefs)";
  for (i = 0; i < CS_NLANG; i++) langs[i + 1] = cs_language_names[i];
  langs[CS_NLANG + 1] = NULL;
  w = cs_dialog_window("Options", VGroup,
    Child, CsTitle(tr("Interface")),
    Child, ColGroup(2),
      Child, CsLabel(tr("_Language"), MUIO_Label_SingleFrame), Child, lang = cs_cycle(langs),
      Child, CsLabel(tr("_Theme"), MUIO_Label_SingleFrame), Child, theme = cs_cycle(themes),
      Child, CsLabel(tr("Te_xt size"), MUIO_Label_SingleFrame), Child, size = cs_cycle(textsizes),
    End,
    Child, cs_check("Lar_ge interface text", prefs.large, &large),
    Child, cs_check("Help _bubbles", prefs.tips, &tips),
    Child, TextObject, MUIA_Text_Contents, (ULONG)tr("The language and the interface's text size change when Jotter starts again."), End,
  End, NULL);
  if (!w) return;
  set(lang, MUIA_Cycle_Active, cur < 0 ? 0 : cur);
  set(theme, MUIA_Cycle_Active, prefs.dark);
  set(size, MUIA_Cycle_Active, prefs.size);
  if (cs_run_modal(w, ends, NULL) == CS_ID_OK) {
    LONG l = (LONG)cs_get(lang, MUIA_Cycle_Active);
    cs_prefs_set("language", l > 0 ? cs_lang_code((int)l - 1) : "");
    prefs.dark = (int)cs_get(theme, MUIA_Cycle_Active);
    prefs.size = (int)cs_get(size, MUIA_Cycle_Active);
    prefs.large = cs_get(large, MUIA_Selected) != 0;
    prefs.tips = cs_get(tips, MUIA_Selected) != 0;
    prefs_write();
    jotter_command(ID_TEXTSIZE, prefs.size);
    if ((cs_theme_is_dark() != 0) != prefs.dark) { prefs.dark = !prefs.dark; jotter_command(ID_DARK, -1); }
    cs_tips = prefs.tips;
  }
  MUI_DisposeObject(w);
}
void about_box(void)
{
  cs_ask("OK", "Jotter %s\n\nCornerstone's example program: a plain-text editor with a ribbon, a File page, an outline pane "
         "and five languages. Free software under the GNU General Public License, version 2.", JOTTER_VERSION " (" JOTTER_DATE ")");
}

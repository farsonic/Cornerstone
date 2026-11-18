/*
 * cs_tellme.c - Tell Me: a small window with a box to type what you want to do and the commands
 * that match (cs_cmd_search: their names, and the words of their help, in the interface's
 * language). Return or a double click runs the one chosen; one on the ribbon is opened there (its
 * tab shown, its list opened), as the upstream programs do, so the user learns where it lives.
 */
#include <string.h>
#include <stdio.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_dialog.h"
#include "cs_theme.h"
#include "cs_cmd.h"
#include "cs_lang.h"
#include "cs_util.h"

#define MAXFOUND 12
static Object *str, *list;
static unsigned long found[MAXFOUND];
static int nfound;
static char lines[MAXFOUND][100];

static void fill(void)
{
  const char *q = (const char *)cs_get(str, MUIA_String_Contents);
  int i;
  nfound = q && *q ? cs_cmd_search(q, found, MAXFOUND) : 0;
  set(list, MUIA_List_Quiet, TRUE);
  DoMethod(list, MUIM_List_Clear);
  for (i = 0; i < nfound; i++) {
    const CsCommand *c = cs_cmd_find(found[i]);
    char name[64];
    cs_cmd_name(found[i], name, sizeof name);
    if (c && c->key && c->key[0]) snprintf(lines[i], sizeof lines[i], "%s  (Amiga-%s)", name, c->key);
    else cs_strlcpy(lines[i], name, sizeof lines[i]);
    DoMethod(list, MUIM_List_InsertSingle, (ULONG)lines[i], MUIV_List_Insert_Bottom);
  }
  set(list, MUIA_List_Quiet, FALSE);
  set(list, MUIA_List_Active, nfound ? 0 : MUIV_List_Active_Off);
}
static void each(ULONG id) { if (id == CS_ID_CHANGE) fill(); }
void cs_tellme_open(Object *ribbon)
{
  static const ULONG ends[] = { CS_ID_OK, 0 };
  Object *w, *lv;
  LONG act;
  ULONG id;
  w = CsWindowObject, MUIA_Window_Title, (ULONG)tr("Tell Me"), MUIA_Window_RefWindow, (ULONG)cs_win,
    MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered, MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
    WindowContents, VGroup, CS_DLG,
      Child, CsLabel(tr("What do you want to do?"), MUIO_Label_LeftAligned),
      Child, str = StringObject, StringFrame, MUIA_String_MaxLen, 60, MUIA_CycleChain, 1, MUIA_FixWidthTxt, (ULONG)"MMMMMMMMMMMMMMMMMMMMMMMMMM", End,
      Child, lv = ListviewObject, MUIA_Listview_List, list = ListObject, InputListFrame,
              MUIA_List_ConstructHook, MUIV_List_ConstructHook_String, MUIA_List_DestructHook, MUIV_List_DestructHook_String, End,
              MUIA_CycleChain, 1, MUIA_FixHeightTxt, (ULONG)"\n\n\n\n\n\n\n", End,
    End,
  End;
  if (!w) return;
  DoMethod(str, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_CHANGE);
  DoMethod(str, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_OK);
  DoMethod(lv, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_OK);
  DoMethod(w, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_CANCEL);
  DoMethod(w, MUIM_Notify, MUIA_Window_InputEvent, (ULONG)"esc", (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_CANCEL);
  /* cursor up and down move in the list while the typing goes on in the box */
  DoMethod(w, MUIM_Notify, MUIA_Window_InputEvent, (ULONG)"down", (ULONG)list, 3, MUIM_Set, MUIA_List_Active, MUIV_List_Active_Down);
  DoMethod(w, MUIM_Notify, MUIA_Window_InputEvent, (ULONG)"up", (ULONG)list, 3, MUIM_Set, MUIA_List_Active, MUIV_List_Active_Up);
  set(w, MUIA_Window_ActiveObject, (ULONG)str);
  nfound = 0;
  id = cs_run_modal(w, ends, each);
  act = (LONG)cs_get(list, MUIA_List_Active);
  MUI_DisposeObject(w);
  if (id != CS_ID_OK || !nfound) return;
  if (act < 0 || act >= nfound) act = 0;
  CS_LOG("tell me: %lu", found[act]);
  if (ribbon) DoMethod(ribbon, MUIM_CsRibbon_Open, found[act]);
  else DoMethod(cs_app, MUIM_Application_ReturnID, found[act]);
}
void cs_tellme_free(void) { str = list = NULL; }

/* cs_ctx.c - see cs_ctx.h */
#include <string.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_ctx.h"
#include "cs_cmd.h"
#include "cs_lang.h"

static Object *dummy, *current;
static char labels[40][64];

/* MUI asks an object for a context menu only if its MUIA_ContextMenu is set: an empty strip will do,
   the real one comes from MUIM_ContextMenuBuild */
void cs_ctx_attach(Object *obj)
{
  if (!dummy) dummy = MenustripObject, End;
  if (dummy) set(obj, MUIA_ContextMenu, (ULONG)dummy);
}
Object *cs_ctx_menu(const char *title, const unsigned long *ids, int n)
{
  Object *menu;
  int i;
  if (current) { MUI_DisposeObject(current); current = NULL; }
  if (!(current = MenustripObject, Child, menu = MenuObject, MUIA_Menu_Title, (ULONG)tr(title), End, End)) return NULL;
  for (i = 0; i < n && i < 40; i++) {
    const CsCommand *c = cs_cmd_find(ids[i]);
    Object *it;
    int s;
    if (!ids[i]) { it = MenuitemObject, MUIA_Menuitem_Title, (ULONG)NM_BARLABEL, End; }
    else {
      if (!c) continue;
      s = cs_cmd_state(ids[i]);
      cs_cmd_name(ids[i], labels[i], sizeof labels[i]);
      if (c->label && strlen(c->label) > 3 && !strcmp(c->label + strlen(c->label) - 3, "...")) strcat(labels[i], "...");
      it = MenuitemObject, MUIA_Menuitem_Title, (ULONG)labels[i], MUIA_UserData, ids[i],
             MUIA_Menuitem_Enabled, (s & CSS_ENABLED) ? TRUE : FALSE,
             (ULONG)((c->flags & CSC_TOGGLE) ? MUIA_Menuitem_Checkit : TAG_IGNORE), TRUE,
             (ULONG)((c->flags & CSC_TOGGLE) ? MUIA_Menuitem_Checked : TAG_IGNORE), (s & CSS_CHECKED) ? TRUE : FALSE,
             (ULONG)(c->key && c->key[0] ? MUIA_Menuitem_Shortcut : TAG_IGNORE), (ULONG)c->key, End;
    }
    if (it) DoMethod(menu, OM_ADDMEMBER, (ULONG)it);
  }
  return current;
}
ULONG cs_ctx_choice(Object *item)
{
  ULONG id = item ? cs_get(item, MUIA_UserData) : 0;
  if (id && cs_app) DoMethod(cs_app, MUIM_Application_ReturnID, id);
  return id;
}
void cs_ctx_free(void)
{
  if (current) { MUI_DisposeObject(current); current = NULL; }
  if (dummy) { MUI_DisposeObject(dummy); dummy = NULL; }
}

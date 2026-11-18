/* cs_menu.c - see cs_menu.h */
#include <string.h>
#include <stdlib.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_menu.h"
#include "cs_lang.h"

static struct NewMenu *nm;              /* the table MUI made the strip from (kept while it lives) */
static ULONG *ids;                      /* each entry's command, for cs_menu_update */
static int nnm;

static void report(const char *line) { CS_LOG("menus: %s", line); }

Object *cs_menu_strip(const CsMenuItem *m)
{
  int n, i;
  cs_menu_free();
  if (cs_menu_check(m, report)) CS_LOG("menus: the layout has problems (above); made all the same");
  for (n = 0; m[n].kind != CSM_END; n++) ;
  if (!(nm = (struct NewMenu *)calloc((size_t)n + 1, sizeof *nm)) || !(ids = (ULONG *)calloc((size_t)n + 1, sizeof *ids))) { cs_menu_free(); return NULL; }
  for (i = 0; i < n; i++) {
    const CsCommand *c = (m[i].kind == CSM_ITEM || m[i].kind == CSM_SUBITEM) ? cs_cmd_find(m[i].id) : NULL;
    struct NewMenu *e = &nm[i];
    switch (m[i].kind) {
    case CSM_TITLE: e->nm_Type = NM_TITLE; e->nm_Label = (STRPTR)tr(m[i].title); break;
    case CSM_SUB:   e->nm_Type = NM_ITEM;  e->nm_Label = (STRPTR)tr(m[i].title); break;
    case CSM_BAR:   e->nm_Type = NM_ITEM;  e->nm_Label = NM_BARLABEL; break;
    case CSM_SUBBAR: e->nm_Type = NM_SUB;  e->nm_Label = NM_BARLABEL; break;
    case CSM_ITEM: case CSM_SUBITEM:
      e->nm_Type = m[i].kind == CSM_ITEM ? NM_ITEM : NM_SUB;
      e->nm_Label = (STRPTR)(c && c->label ? tr(c->label) : "?");
      e->nm_CommKey = (STRPTR)(c && c->key && c->key[0] ? c->key : NULL);
      e->nm_Flags = (UWORD)(c && (c->flags & CSC_TOGGLE) ? CHECKIT | MENUTOGGLE : 0);
      e->nm_UserData = (APTR)(m[i].id);
      ids[i] = m[i].id;
      break;
    }
  }
  nm[n].nm_Type = NM_END;
  nnm = n;
  return MUI_MakeObject(MUIO_MenustripNM, (ULONG)nm, 0);
}
void cs_menu_update(Object *strip)
{
  int i;
  if (!strip) return;
  for (i = 0; i < nnm; i++) {
    Object *it;
    int s;
    if (!ids[i] || !(it = (Object *)DoMethod(strip, MUIM_FindUData, ids[i]))) continue;
    s = cs_cmd_state(ids[i]);
    if ((ULONG)((s & CSS_ENABLED) != 0) != (cs_get(it, MUIA_Menuitem_Enabled) != 0)) set(it, MUIA_Menuitem_Enabled, (s & CSS_ENABLED) ? TRUE : FALSE);
    if (nm[i].nm_Flags & CHECKIT) {
      ULONG on = (s & CSS_CHECKED) ? TRUE : FALSE;
      if ((cs_get(it, MUIA_Menuitem_Checked) != 0) != (on != 0)) nnset(it, MUIA_Menuitem_Checked, on);
    }
  }
}
void cs_menu_set_label(Object *strip, ULONG id, const char *label)
{
  Object *it = strip ? (Object *)DoMethod(strip, MUIM_FindUData, id) : NULL;
  if (it) set(it, MUIA_Menuitem_Title, (ULONG)label);
}
void cs_menu_free(void)
{
  free(nm); free(ids);
  nm = NULL; ids = NULL; nnm = 0;
}

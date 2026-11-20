/* cs_classes.c - see cs_classes.h: every class of the kit made at start and freed at the end */
#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_dialog.h"

/* (each class file's free function) */
void cs_ribbon_free(void);
void cs_popup_free(void);
void cs_scroll_free(void);
void cs_tabs_free(void);
void cs_status_free(void);
void cs_pane_free(void);
void cs_backstage_free(void);

/* a class's OM_NEW passed on with MUIA_FillArea FALSE before its own tags, for a class that paints
   every pixel of its box itself. With MUI's default (TRUE) each MADF_DRAWOBJECT redraw first clears
   the box to the background and the class then paints it again: at every hover that shows as a
   flash (Riptide's first windows did the same) */
ULONG cs_new_nofill(struct IClass *cl, Object *obj, Msg msg)
{
  struct opSet ns = *(struct opSet *)msg;
  struct TagItem t[2];
  t[0].ti_Tag = MUIA_FillArea; t[0].ti_Data = FALSE;
  t[1].ti_Tag = TAG_MORE; t[1].ti_Data = (ULONG)ns.ops_AttrList;
  ns.ops_AttrList = t;
  return DoSuperMethodA(cl, obj, (Msg)&ns);
}
int cs_classes_make(void)
{
  int ok = cs_ribbon_class() && cs_popup_class() && cs_scroll_class() && cs_tabs_class() && cs_status_class() &&
           cs_panehead_class() && cs_backstage_class();
  if (!ok) CS_LOG("classes: one couldn't be made");
  return ok;
}
/* after the application object (every window and object) is disposed */
void cs_classes_free(void)
{
  cs_tellme_free();
  cs_ribbon_free();
  cs_popup_free();
  cs_scroll_free();
  cs_tabs_free();
  cs_status_free();
  cs_pane_free();
  cs_backstage_free();
  cs_dialog_classes_free();
}

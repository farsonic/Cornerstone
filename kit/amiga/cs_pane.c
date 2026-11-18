/*
 * cs_pane.c - a side pane's header: its title in bold on the pane's colour and a close box at its
 * far end (MUIA_CsPane_Close). cs_pane() puts a header over the pane's contents in a group with a
 * line down its inner edge; put cs_balance() between the pane and the document so the user can drag
 * its width (MUI's own Balance splitter; a plain line on the Zune versions that lack that class):
 *
 *   Child, HGroup, MUIA_Group_Spacing, 0,
 *     Child, pane = cs_pane("Navigation", list, &head),
 *     Child, cs_balance(),
 *     Child, view,
 *   End,
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_common.h"
#include "cs_lang.h"

typedef struct { CS_CLASS_HEAD; const char *title; int hot, pressed; } HData;
static struct MUI_CustomClass *hcls;

static void closebox(Object *obj, LONG *x0, LONG *x1)
{
  if (cs_rtl) { *x0 = _mleft(obj) + 2; *x1 = *x0 + 15; }
  else { *x1 = _mright(obj) - 2; *x0 = *x1 - 15; }
}
static void draw(HData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  LONG c0, c1, tw = cs_textlen(rp, cs_bold(), d->title);
  closebox(obj, &c0, &c1);
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_PANEL));
  if (cs_rtl) cs_text_fit(rp, cs_bold(), _mright(obj) - 6 - (tw < _mwidth(obj) - 30 ? tw : _mwidth(obj) - 30), _mtop(obj) + 4, d->title, _mwidth(obj) - 30, cs_pen(CS_T_TEXT));
  else cs_text_fit(rp, cs_bold(), _mleft(obj) + 6, _mtop(obj) + 4, d->title, _mwidth(obj) - 30, cs_pen(CS_T_TEXT));
  if (d->hot) cs_fill(rp, c0, _mtop(obj) + 2, c1, _mtop(obj) + 17, cs_pen(d->pressed ? CS_T_PRESSED : CS_T_HOVER));
  cs_icon(rp, CS_IC_CROSS, c0 + 2, _mtop(obj) + 4, 12, 0);
}
CS_DISPATCHER(hdispatch)
{
  HData *d;
  switch (msg->MethodID) {
  case OM_NEW:
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    d->title = (const char *)GetTagData(MUIA_CsPane_Title, (ULONG)"", ((struct opSet *)msg)->ops_AttrList);
    CS_EHN(d, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);
    return (ULONG)obj;
  case OM_SET: {
    struct TagItem *t = FindTagItem(MUIA_CsPane_Title, ((struct opSet *)msg)->ops_AttrList);
    d = INST_DATA(cl, obj);
    if (t) { d->title = (const char *)t->ti_Data; if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT); }
    break;
  }
  case MUIM_AskMinMax: {
    LONG h = cs_font()->tf_YSize + 10;
    DoSuperMethodA(cl, obj, msg);
    CS_MINMAX(msg, 60, 160, MUI_MAXMAX, h, h, h);
    return 0;
  }
  CS_CLASS_COMMON(HData);
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw(INST_DATA(cl, obj), obj);
    return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    LONG c0, c1;
    int on;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown) return 0;
    closebox(obj, &c0, &c1);
    on = CS_INSIDE(obj, im) && im->MouseX >= c0 && im->MouseX <= c1;
    if (im->Class == IDCMP_MOUSEMOVE) { if (on != d->hot) { d->hot = on; MUI_Redraw(obj, MADF_DRAWOBJECT); } return 0; }
    if (im->Class == IDCMP_MOUSEBUTTONS && im->Code == SELECTDOWN && on) { d->pressed = 1; MUI_Redraw(obj, MADF_DRAWOBJECT); return MUI_EventHandlerRC_Eat; }
    if (im->Class == IDCMP_MOUSEBUTTONS && im->Code == SELECTUP && d->pressed) {
      d->pressed = 0;
      MUI_Redraw(obj, MADF_DRAWOBJECT);
      if (on) SetAttrs(obj, MUIA_CsPane_Close, TRUE, TAG_DONE);
      return MUI_EventHandlerRC_Eat;
    }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_panehead_class(void)
{
  if (!hcls) hcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(HData), (APTR)hdispatch);
  return hcls;
}
void cs_pane_free(void) { if (hcls) { MUI_DeleteCustomClass(hcls); hcls = NULL; } }
Object *cs_pane(const char *title, Object *contents, Object **header)
{
  Object *h = cs_panehead_class() ? (Object *)NewObject(hcls->mcc_Class, NULL, MUIA_CsPane_Title, (ULONG)title, TAG_DONE) : NULL;
  if (header) *header = h;
  return VGroup, MUIA_Group_Spacing, 0, InnerSpacing(0, 0), MUIA_HorizWeight, 30,
    Child, h,
    Child, contents,
  End;
}

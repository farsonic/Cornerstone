/*
 * cs_scroll.c - the flat scroll bar: a thin bar with a rounded thumb, no arrows, that speaks a Prop
 * gadget's attributes (MUIA_Prop_Entries, _Visible, _First), so it is wired with the same
 * notifications. Clicking beside the thumb moves by nine tenths of a page; dragging moves with the
 * pointer. Integer arithmetic only (cs_muldiv), so it needs no FPU.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_common.h"
#include "cs_util.h"

typedef struct { CS_CLASS_HEAD; LONG entries, visible, first, grab; int drag, hot, horiz; } FData;
static struct MUI_CustomClass *fcls;

static LONG f0(FData *d, Object *obj) { return (d->horiz ? _mleft(obj) : _mtop(obj)) + 2; }
static LONG flen(FData *d, Object *obj) { return (d->horiz ? _mwidth(obj) : _mheight(obj)) - 4; }
/* the thumb's start and end along the bar, at least 13 long and kept inside */
static void thumb(FData *d, Object *obj, LONG *a, LONG *b)
{
  LONG t = f0(d, obj), len = flen(d, obj), e = d->entries > d->visible ? d->entries : d->visible;
  if (e <= 0) e = 1;
  *a = t + cs_muldiv(len, d->first, e);
  *b = t + cs_muldiv(len, d->first + d->visible, e) - 1;
  if (*b < *a + 12) *b = *a + 12;
  if (*b > t + len - 1) { *a -= *b - (t + len - 1); *b = t + len - 1; }
  if (*a < t) *a = t;
}
static void draw(FData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  LONG a, b, pen = cs_pen(d->drag ? CS_T_DIM : d->hot ? CS_T_GROUPLABEL : CS_T_BORDER2);
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_PANEL));
  if (d->entries <= d->visible) return;
  thumb(d, obj, &a, &b);
  if (d->horiz) {
    LONG t = (_mtop(obj) + _mbottom(obj)) / 2 - 3, u = t + 6;
    cs_fill(rp, a + 1, t, b - 1, u, pen);
    cs_fill(rp, a, t + 1, b, u - 1, pen);
  } else {
    LONG l = (_mleft(obj) + _mright(obj)) / 2 - 3, r = l + 6;
    cs_fill(rp, l, a + 1, r, b - 1, pen);
    cs_fill(rp, l + 1, a, r - 1, b, pen);
  }
}
static void set_first(FData *d, Object *obj, LONG f)
{
  LONG max = d->entries - d->visible;
  if (f > max) f = max;
  if (f < 0) f = 0;
  if (f != d->first) SetAttrs(obj, MUIA_Prop_First, f, TAG_DONE);     /* through OM_SET: notifications go */
}
CS_DISPATCHER(fdispatch)
{
  FData *d;
  switch (msg->MethodID) {
  case OM_NEW:
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    d->entries = d->visible = 1;
    d->horiz = (int)GetTagData(MUIA_CsScroll_Horiz, FALSE, ((struct opSet *)msg)->ops_AttrList);
    CS_EHN(d, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);
    return (ULONG)obj;
  case OM_SET: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList, *t;
    int redraw = 0;
    d = INST_DATA(cl, obj);
    while ((t = NextTagItem(&tags))) {
      if (t->ti_Tag == MUIA_Prop_Entries && d->entries != (LONG)t->ti_Data) { d->entries = (LONG)t->ti_Data; redraw = 1; }
      else if (t->ti_Tag == MUIA_Prop_Visible && d->visible != (LONG)t->ti_Data) { d->visible = (LONG)t->ti_Data; redraw = 1; }
      else if (t->ti_Tag == MUIA_Prop_First && d->first != (LONG)t->ti_Data) { d->first = (LONG)t->ti_Data; redraw = 1; }
    }
    if (redraw && d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
    break;
  }
  case OM_GET: {
    struct opGet *g = (struct opGet *)msg;
    d = INST_DATA(cl, obj);
    if (g->opg_AttrID == MUIA_Prop_Entries) { *g->opg_Storage = (ULONG)d->entries; return TRUE; }
    if (g->opg_AttrID == MUIA_Prop_Visible) { *g->opg_Storage = (ULONG)d->visible; return TRUE; }
    if (g->opg_AttrID == MUIA_Prop_First) { *g->opg_Storage = (ULONG)d->first; return TRUE; }
    break;
  }
  case MUIM_AskMinMax:
    DoSuperMethodA(cl, obj, msg);
    if (((FData *)INST_DATA(cl, obj))->horiz) CS_MINMAX(msg, 30, 200, MUI_MAXMAX, 10, 10, 10);
    else CS_MINMAX(msg, 10, 10, 10, 30, 200, MUI_MAXMAX);
    return 0;
  CS_CLASS_COMMON(FData);
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw(INST_DATA(cl, obj), obj);
    return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    LONG a, b, mp;
    int inside;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown || d->entries <= d->visible) return 0;
    inside = CS_INSIDE(obj, im);
    thumb(d, obj, &a, &b);
    mp = d->horiz ? im->MouseX : im->MouseY;
    if (im->Class == IDCMP_MOUSEMOVE) {
      if (d->drag) {
        LONG len = flen(d, obj);
        set_first(d, obj, cs_muldiv(mp - d->grab - f0(d, obj), d->entries, len > 0 ? len : 1));
        return MUI_EventHandlerRC_Eat;
      }
      if ((inside && mp >= a && mp <= b) != d->hot) { d->hot = !d->hot; MUI_Redraw(obj, MADF_DRAWOBJECT); }
      return 0;
    }
    if (im->Class != IDCMP_MOUSEBUTTONS) return 0;
    if (im->Code == SELECTUP && d->drag) { d->drag = 0; MUI_Redraw(obj, MADF_DRAWOBJECT); return MUI_EventHandlerRC_Eat; }
    if (im->Code != SELECTDOWN || !inside) return 0;
    if (mp < a) set_first(d, obj, d->first - d->visible * 9 / 10);
    else if (mp > b) set_first(d, obj, d->first + d->visible * 9 / 10);
    else { d->drag = 1; d->grab = mp - a; MUI_Redraw(obj, MADF_DRAWOBJECT); }
    return MUI_EventHandlerRC_Eat;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_scroll_class(void)
{
  if (!fcls) fcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(FData), (APTR)fdispatch);
  return fcls;
}
void cs_scroll_free(void) { if (fcls) { MUI_DeleteCustomClass(fcls); fcls = NULL; } }
Object *cs_scrollbar(int horiz)
{
  return cs_scroll_class() ? (Object *)NewObject(fcls->mcc_Class, NULL, MUIA_CsScroll_Horiz, horiz, TAG_DONE) : NULL;
}

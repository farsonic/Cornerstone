/*
 * cs_tabs.c - a strip of tabs: labels side by side, the active one in the text colour with the
 * accent line under it, the others dimmed, the one under the pointer lit. Its MUIA_CsTabs_Active is
 * notified straight into a page group's MUIA_Group_ActivePage, which is all a tabbed pane needs:
 *
 *   DoMethod(tabs, MUIM_Notify, MUIA_CsTabs_Active, MUIV_EveryTime,
 *            (ULONG)pages, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
 *
 * With MUIA_CsTabs_Closable each tab has a close box (document tabs), reported by MUIA_CsTabs_Close.
 * Right to left, the tabs run from the right.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_common.h"
#include "cs_lang.h"

#define MAXT 24
typedef struct { CS_CLASS_HEAD; const char *const *labels; int n, active, hot, closable, hotclose; LONG x0[MAXT], x1[MAXT]; } TData;
static struct MUI_CustomClass *tcls;

static void layout(TData *d, Object *obj)
{
  LONG x = _mleft(obj);
  int i;
  for (i = 0; i < d->n; i++) {
    LONG w = cs_textlen(_rp(obj), cs_font(), d->labels[i]) + 20 + (d->closable ? 14 : 0);
    if (cs_rtl) { d->x1[i] = _mright(obj) - (x - _mleft(obj)); d->x0[i] = d->x1[i] - w + 1; }
    else { d->x0[i] = x; d->x1[i] = x + w - 1; }
    x += w;
  }
}
static int hit(TData *d, LONG x, int *onclose)
{
  int i;
  *onclose = 0;
  for (i = 0; i < d->n; i++)
    if (x >= d->x0[i] && x <= d->x1[i]) {
      if (d->closable) *onclose = cs_rtl ? x < d->x0[i] + 14 : x > d->x1[i] - 14;
      return i;
    }
  return -1;
}
static void draw(TData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  int i;
  layout(d, obj);
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_TABSTRIP));
  cs_fill(rp, _mleft(obj), _mbottom(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_BORDER));
  for (i = 0; i < d->n; i++) {
    LONG tx = cs_rtl && d->closable ? d->x0[i] + 24 : d->x0[i] + 10;
    if (i == d->active) cs_fill(rp, d->x0[i], _mtop(obj), d->x1[i], _mbottom(obj), cs_pen(CS_T_PANEL));
    else if (i == d->hot) cs_fill(rp, d->x0[i], _mtop(obj), d->x1[i], _mbottom(obj) - 1, cs_pen(CS_T_HOVER));
    cs_text(rp, i == d->active ? cs_bold() : cs_font(), tx, _mtop(obj) + 4, d->labels[i], cs_pen(i == d->active ? CS_T_TEXT : CS_T_DIM));
    if (i == d->active) cs_fill(rp, d->x0[i] + 6, _mbottom(obj) - 2, d->x1[i] - 6, _mbottom(obj), cs_pen(CS_T_ACCENT));
    if (d->closable) {
      LONG cx = cs_rtl ? d->x0[i] + 3 : d->x1[i] - 13;
      if (i == d->hot && d->hotclose) cs_fill(rp, cx - 1, _mtop(obj) + 3, cx + 10, _mtop(obj) + 14, cs_pen(CS_T_PRESSED));
      cs_icon(rp, CS_IC_CROSS, cx, _mtop(obj) + 4, 10, i != d->active && i != d->hot);
    }
  }
}
CS_DISPATCHER(tdispatch)
{
  TData *d;
  switch (msg->MethodID) {
  case OM_NEW: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList;
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    d->hot = -1;
    d->labels = (const char *const *)GetTagData(MUIA_CsTabs_Labels, 0, tags);
    d->closable = (int)GetTagData(MUIA_CsTabs_Closable, 0, tags);
    d->active = (int)GetTagData(MUIA_CsTabs_Active, 0, tags);
    while (d->labels && d->labels[d->n] && d->n < MAXT) d->n++;
    CS_EHN(d, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);
    return (ULONG)obj;
  }
  case OM_SET: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList, *t;
    d = INST_DATA(cl, obj);
    if ((t = FindTagItem(MUIA_CsTabs_Labels, tags))) {
      d->labels = (const char *const *)t->ti_Data; d->n = 0;
      while (d->labels && d->labels[d->n] && d->n < MAXT) d->n++;
      if (d->active >= d->n) d->active = d->n ? d->n - 1 : 0;
      if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
    }
    if ((t = FindTagItem(MUIA_CsTabs_Active, tags)) && d->active != (int)t->ti_Data) {
      d->active = (int)t->ti_Data;
      if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
    }
    break;
  }
  case OM_GET:
    if (((struct opGet *)msg)->opg_AttrID == MUIA_CsTabs_Active) { *((struct opGet *)msg)->opg_Storage = (ULONG)((TData *)INST_DATA(cl, obj))->active; return TRUE; }
    break;
  case MUIM_AskMinMax: {
    LONG h = cs_font()->tf_YSize + 9;
    DoSuperMethodA(cl, obj, msg);
    CS_MINMAX(msg, 40, 200, MUI_MAXMAX, h, h, h);
    return 0;
  }
  CS_CLASS_COMMON(TData);
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw(INST_DATA(cl, obj), obj);
    return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    int i, oc;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown) return 0;
    i = CS_INSIDE(obj, im) ? hit(d, im->MouseX, &oc) : (oc = 0, -1);
    if (im->Class == IDCMP_MOUSEMOVE) {
      if (i != d->hot || oc != d->hotclose) { d->hot = i; d->hotclose = oc; MUI_Redraw(obj, MADF_DRAWOBJECT); }
      return 0;
    }
    if (im->Class == IDCMP_MOUSEBUTTONS && im->Code == SELECTDOWN && i >= 0) {
      if (oc) SetAttrs(obj, MUIA_CsTabs_Close, i, TAG_DONE);
      else SetAttrs(obj, MUIA_CsTabs_Active, i, TAG_DONE);
      return MUI_EventHandlerRC_Eat;
    }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_tabs_class(void)
{
  if (!tcls) tcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(TData), (APTR)tdispatch);
  return tcls;
}
void cs_tabs_free(void) { if (tcls) { MUI_DeleteCustomClass(tcls); tcls = NULL; } }
Object *cs_tabs(const char *const *labels)
{
  return cs_tabs_class() ? (Object *)NewObject(tcls->mcc_Class, NULL, MUIA_CsTabs_Labels, (ULONG)labels, TAG_DONE) : NULL;
}

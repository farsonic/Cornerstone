/*
 * cs_backstage.c - the File page's command column: a band in the accent colour down the left with
 * a Back arrow at its top and the File commands under it (New, Open, Save, Save As, Export,
 * Options...), each its icon in white and its name. Clicking one reports its command id
 * (MUIA_CsBackstage_Pick; 0 is Back). The program shows the File page in place of the ribbon, the
 * document and the panes (a page group: the File page is page 1), with the page of the command
 * chosen to the right of the column; Esc and Back return to the document.
 */
#include <string.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_common.h"
#include "cs_cmd.h"
#include "cs_lang.h"

#define MAXB 24
typedef struct { CS_CLASS_HEAD; const ULONG *ids; int n, hot, active; } BData;
static struct MUI_CustomClass *bcls;

static LONG rowh(void) { return cs_font()->tf_YSize + 14; }
/* row i's top; -1 is Back, then the commands, a line (~0UL) taking half a row */
static LONG row_y(BData *d, Object *obj, int i)
{
  LONG y = _mtop(obj) + 6 + rowh() + 6;
  int k;
  if (i < 0) return _mtop(obj) + 6;
  for (k = 0; k < i; k++) y += d->ids[k] == ~0UL ? rowh() / 2 : rowh();
  return y;
}
static int hit(BData *d, Object *obj, LONG y)
{
  int i;
  if (y >= row_y(d, obj, -1) && y < row_y(d, obj, -1) + rowh()) return -1;
  for (i = 0; i < d->n; i++) if (d->ids[i] != ~0UL && y >= row_y(d, obj, i) && y < row_y(d, obj, i) + rowh()) return i;
  return -2;
}
static void draw(BData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  LONG l = _mleft(obj), r = _mright(obj);
  int i;
  char name[64];
  cs_fill(rp, l, _mtop(obj), r, _mbottom(obj), cs_pen(CS_T_ACCENT));
  if (d->hot == -1) cs_fill(rp, l, row_y(d, obj, -1), r, row_y(d, obj, -1) + rowh() - 1, cs_pen(CS_T_ACCENTHOVER));
  cs_icon_pen(rp, CS_IC_BACK, cs_rtl ? r - 28 : l + 12, row_y(d, obj, -1) + (rowh() - 16) / 2, 16, cs_pen(CS_T_ONACCENT));
  for (i = 0; i < d->n; i++) {
    LONG y = row_y(d, obj, i);
    const CsCommand *c;
    if (d->ids[i] == ~0UL) { cs_fill(rp, l + 12, y + rowh() / 4, r - 12, y + rowh() / 4, cs_pen(CS_T_ACCENTHOVER)); continue; }
    c = cs_cmd_find(d->ids[i]);
    if (i == d->active) cs_fill(rp, l, y, r, y + rowh() - 1, cs_pen(CS_T_ACCENTDARK));
    else if (i == d->hot) cs_fill(rp, l, y, r, y + rowh() - 1, cs_pen(CS_T_ACCENTHOVER));
    if (c && c->icon) cs_icon_pen(rp, c->icon, cs_rtl ? r - 28 : l + 12, y + (rowh() - 16) / 2, 16, cs_pen(CS_T_ONACCENT));
    cs_cmd_name(d->ids[i], name, sizeof name);
    if (cs_rtl) cs_text(rp, cs_font(), r - 36 - cs_textlen(rp, cs_font(), name), y + 7, name, cs_pen(CS_T_ONACCENT));
    else cs_text(rp, cs_font(), l + 36, y + 7, name, cs_pen(CS_T_ONACCENT));
  }
}
CS_DISPATCHER(bdispatch)
{
  BData *d;
  switch (msg->MethodID) {
  case OM_NEW:
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    d->ids = (const ULONG *)GetTagData(MUIA_CsBackstage_Ids, 0, ((struct opSet *)msg)->ops_AttrList);
    while (d->ids && d->ids[d->n] && d->n < MAXB) d->n++;
    d->hot = -2; d->active = -1;
    CS_EHN(d, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE);
    return (ULONG)obj;
  case OM_SET: {
    struct TagItem *t = FindTagItem(MUIA_CsBackstage_Active, ((struct opSet *)msg)->ops_AttrList);
    int i;
    d = INST_DATA(cl, obj);
    if (t) {
      for (i = 0; i < d->n && d->ids[i] != t->ti_Data; i++) ;
      d->active = i < d->n ? i : -1;
      if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
    }
    break;
  }
  case MUIM_AskMinMax: {
    struct RastPort *rp = &_screen(obj)->RastPort;
    LONG w = 120;
    int i;
    char name[64];
    DoSuperMethodA(cl, obj, msg);
    d = INST_DATA(cl, obj);
    for (i = 0; i < d->n; i++) if (d->ids[i] != ~0UL) { LONG tw = cs_textlen(rp, cs_font(), cs_cmd_name(d->ids[i], name, sizeof name)) + 52; if (tw > w) w = tw; }
    CS_MINMAX(msg, w, w, w, 100, 300, MUI_MAXMAX);
    return 0;
  }
  CS_CLASS_COMMON(BData);
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw(INST_DATA(cl, obj), obj);
    return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    int i;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown) return 0;
    i = CS_INSIDE(obj, im) ? hit(d, obj, im->MouseY) : -2;
    if (im->Class == IDCMP_MOUSEMOVE) { if (i != d->hot) { d->hot = i; MUI_Redraw(obj, MADF_DRAWOBJECT); } return 0; }
    if (im->Class == IDCMP_MOUSEBUTTONS && im->Code == SELECTUP && i >= -1) {
      SetAttrs(obj, MUIA_CsBackstage_Pick, i < 0 ? 0 : d->ids[i], TAG_DONE);
      return MUI_EventHandlerRC_Eat;
    }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_backstage_class(void)
{
  if (!bcls) bcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(BData), (APTR)bdispatch);
  return bcls;
}
void cs_backstage_free(void) { if (bcls) { MUI_DeleteCustomClass(bcls); bcls = NULL; } }

/*
 * cs_popup.c - the drop-down list: a borderless window below a point listing entries in the theme
 * (with an icon each, if given; disabled and ticked entries), for every drop-down of the ribbon,
 * the toolbars and panes. The choice comes back as set(owner, attr, index), pushed so that it
 * arrives after the list has closed (closing a window from inside its own event is not safe). It
 * closes when it loses activation, and the window it lay over is repaired (MUI doesn't always).
 * Keys: cursor up and down, Return to choose, Esc to close.
 */
#include <string.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_theme.h"
#include "cs_lang.h"
#include "cs_icon.h"

#define POPMAX 64
struct PData { Object *owner; ULONG attr; const char *labels[POPMAX]; int icons[POPMAX]; int n, hot, cur, anyicon; LONG w; };
static struct MUI_CustomClass *pcls;
static Object *popwin, *poplist;
#define MUIM_CsPop_Close CS_TAG(0x7f0)

static LONG rowh(void) { return cs_font()->tf_YSize + 5; }
static void pop_close(struct PData *p)
{
  Object *root = NULL, *w;
  if (!popwin || !cs_get(popwin, MUIA_Window_Open)) return;
  set(popwin, MUIA_Window_Open, FALSE);
  if (!p->owner || !(w = _win(p->owner))) return;
  get(w, MUIA_Window_RootObject, &root);
  if (root && DoMethod(root, MUIM_Group_InitChange)) DoMethod(root, MUIM_Group_ExitChange);
}
static int usable(struct PData *p, int i) { return i >= 0 && i < p->n && !(p->icons[i] & CS_POP_OFF); }
static void choose(Object *obj, struct PData *p, int i)
{
  if (!usable(p, i)) return;
  DoMethod(_app(obj), MUIM_Application_PushMethod, (ULONG)obj, 1, MUIM_CsPop_Close);
  DoMethod(_app(obj), MUIM_Application_PushMethod, (ULONG)p->owner, 3, MUIM_Set, p->attr, (ULONG)i);
}
static ULONG popdispatch(__reg("a0") struct IClass *cl, __reg("a2") Object *obj, __reg("a1") Msg msg)
{
  struct PData *p;
  switch (msg->MethodID) {
  case OM_NEW:
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    p = INST_DATA(cl, obj);
    memset(p, 0, sizeof *p);
    p->hot = -1;
    return (ULONG)obj;
  case MUIM_Setup:
    if (!DoSuperMethodA(cl, obj, msg)) return FALSE;
    cs_theme_setup(obj);
    MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY);
    return TRUE;
  case MUIM_Cleanup:
    MUI_RejectIDCMP(obj, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY);
    cs_theme_cleanup(obj);
    break;
  case MUIM_AskMinMax: {
    struct MUIP_AskMinMax *m = (struct MUIP_AskMinMax *)msg;
    LONG h;
    p = INST_DATA(cl, obj);
    DoSuperMethodA(cl, obj, msg);
    h = (p->n ? p->n : 1) * rowh() + 4;
    m->MinMaxInfo->MinWidth += p->w; m->MinMaxInfo->DefWidth += p->w; m->MinMaxInfo->MaxWidth += p->w;
    m->MinMaxInfo->MinHeight += h; m->MinMaxInfo->DefHeight += h; m->MinMaxInfo->MaxHeight += h;
    return 0;
  }
  case MUIM_Draw: {
    struct RastPort *rp = _rp(obj);
    int i;
    DoSuperMethodA(cl, obj, msg);
    p = INST_DATA(cl, obj);
    cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_PANEL));
    cs_rect(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_BORDER2));
    for (i = 0; i < p->n; i++) {
      LONG y = _mtop(obj) + 2 + i * rowh(), x = _mleft(obj) + 12;
      int off = (p->icons[i] & CS_POP_OFF) != 0, tick = (p->icons[i] & CS_POP_TICK) || i == p->cur;
      int icon = p->icons[i] & 0xFFFF;
      if (!p->labels[i] || !p->labels[i][0]) {          /* "" : a separator line */
        cs_fill(rp, _mleft(obj) + 4, y + rowh() / 2, _mright(obj) - 4, y + rowh() / 2, cs_pen(CS_T_BORDER));
        continue;
      }
      if (i == p->hot && !off) cs_fill(rp, _mleft(obj) + 1, y, _mright(obj) - 1, y + rowh() - 1, cs_pen(CS_T_HOVER));
      if (tick) cs_icon(rp, CS_IC_TICK, _mleft(obj) + 2, y + (rowh() - 10) / 2, 10, off);
      if (p->anyicon) { if (icon) cs_icon(rp, icon, x, y + (rowh() - 16) / 2, 16, off); x += 20; }
      cs_text(rp, cs_font(), x, y + 2, p->labels[i], cs_pen(off ? CS_T_DISABLED : CS_T_TEXT));
    }
    return 0;
  }
  case MUIM_CsPop_Close:
    pop_close(INST_DATA(cl, obj));
    return 0;
  case MUIM_HandleInput: {
    struct MUIP_HandleInput *m = (struct MUIP_HandleInput *)msg;
    struct IntuiMessage *im = m->imsg;
    p = INST_DATA(cl, obj);
    if (!im) return 0;
    if (im->Class == IDCMP_MOUSEMOVE || im->Class == IDCMP_MOUSEBUTTONS) {
      WORD mx = im->MouseX, my = im->MouseY;
      int i = -1;
      if (mx >= _mleft(obj) && mx <= _mright(obj) && my >= _mtop(obj) + 2) i = (int)((my - _mtop(obj) - 2) / rowh());
      if (i >= p->n) i = -1;
      if (im->Class == IDCMP_MOUSEMOVE && i != p->hot) { p->hot = i; MUI_Redraw(obj, MADF_DRAWOBJECT); }
      if (im->Class == IDCMP_MOUSEBUTTONS && im->Code == SELECTUP) choose(obj, p, i);
    } else if (im->Class == IDCMP_RAWKEY && !(im->Code & IECODE_UP_PREFIX)) {
      int k = p->hot, step = im->Code == 0x4D ? 1 : im->Code == 0x4C ? -1 : 0, tries;
      if (step) {
        for (tries = 0; tries < p->n; tries++) { k = (k + step + p->n) % p->n; if (usable(p, k) && p->labels[k][0]) break; }
        p->hot = k; MUI_Redraw(obj, MADF_DRAWOBJECT);
      } else if (im->Code == 0x44 || im->Code == 0x43) choose(obj, p, p->hot);   /* Return, Enter */
      else if (im->Code == 0x45) DoMethod(_app(obj), MUIM_Application_PushMethod, (ULONG)obj, 1, MUIM_CsPop_Close);   /* Esc */
    }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_popup_class(void)
{
  if (!pcls) pcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(struct PData), (APTR)popdispatch);
  return pcls;
}
void cs_popup(Object *owner, LONG x, LONG y, LONG w, const char *const *labels, const int *icons, int n, int cur, ULONG attr)
{
  struct PData *p;
  struct Screen *scr = _screen(owner);
  struct RastPort *rp = _rp(owner);
  int i;
  if (!cs_popup_class()) return;
  if (n > POPMAX) n = POPMAX;
  if (!popwin) {
    poplist = NewObject(pcls->mcc_Class, NULL, MUIA_FillArea, FALSE, TAG_DONE);
    popwin = CsWindowObject, MUIA_Window_Borderless, TRUE, MUIA_Window_CloseGadget, FALSE, MUIA_Window_DepthGadget, FALSE,
      MUIA_Window_SizeGadget, FALSE, MUIA_Window_DragBar, FALSE, MUIA_Window_Activate, TRUE,
      WindowContents, VGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0, Child, poplist, End, End;
    if (!popwin) { poplist = NULL; return; }
    DoMethod(_app(owner), OM_ADDMEMBER, (ULONG)popwin);
    DoMethod(popwin, MUIM_Notify, MUIA_Window_Activate, FALSE, (ULONG)poplist, 1, MUIM_CsPop_Close);
  }
  set(popwin, MUIA_Window_Open, FALSE);
  p = INST_DATA(pcls->mcc_Class, poplist);
  p->owner = owner; p->attr = attr; p->n = n; p->cur = cur; p->hot = cur; p->anyicon = 0;
  for (i = 0; i < n; i++) {
    p->labels[i] = labels[i] ? labels[i] : "";
    p->icons[i] = icons ? icons[i] : 0;
    if (p->icons[i] & 0xFFFF) p->anyicon = 1;
  }
  for (i = 0; i < n; i++) { LONG tw = cs_textlen(rp, cs_font(), p->labels[i]) + 24 + (p->anyicon ? 20 : 0); if (tw > w) w = tw; }
  p->w = w;
  if (y + n * rowh() + 12 > scr->Height) y = scr->Height - n * rowh() - 12;
  if (y < 0) y = 0;
  if (cs_rtl) x -= w;                                   /* right to left: the list's right edge at x */
  if (x + w > scr->Width) x = scr->Width - w;
  if (x < 0) x = 0;
  SetAttrs(popwin, MUIA_Window_LeftEdge, x, MUIA_Window_TopEdge, y, TAG_DONE);
  set(popwin, MUIA_Window_Open, TRUE);
}
void cs_popup_close(void)
{
  if (popwin && poplist) pop_close(INST_DATA(pcls->mcc_Class, poplist));
}
/* (at the end: the window was the application's and went with it) */
void cs_popup_free(void)
{
  popwin = poplist = NULL;
  if (pcls) { MUI_DeleteCustomClass(pcls); pcls = NULL; }
}

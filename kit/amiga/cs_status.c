/*
 * cs_status.c - the status bar along the window's foot: fields of text, some from the left (the
 * page, the line and column, the word count), some from the right (the zoom, the language), each
 * with a thin line between. Its height is the font's and 6 pixels. A click on a field is reported
 * (MUIA_CsStatus_Click: its number), so a field can open what it shows (the zoom its dialog, the
 * language its list). Right to left, "left" fields start at the right.
 */
#include <string.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_common.h"
#include "cs_util.h"

#define MAXF 8
typedef struct { CS_CLASS_HEAD; int n; char side[MAXF]; LONG w[MAXF], x0[MAXF], x1[MAXF]; char text[MAXF][80]; } SData;
static struct MUI_CustomClass *scls;

/* the field boxes: left ones from the left, right ones from the right, a width of 0 takes the rest */
static void layout(SData *d, Object *obj)
{
  LONG l = _mleft(obj) + 4, r = _mright(obj) - 4;
  int i, rest = -1;
  for (i = 0; i < d->n; i++) {
    if (!d->w[i]) { rest = i; continue; }
    if (d->side[i] == 'L') { d->x0[i] = l; d->x1[i] = l + d->w[i] - 1; l += d->w[i] + 1; }
    else { d->x1[i] = r; d->x0[i] = r - d->w[i] + 1; r -= d->w[i] + 1; }
  }
  if (rest >= 0) { d->x0[rest] = l; d->x1[rest] = r; }
  if (cs_rtl) for (i = 0; i < d->n; i++) {
    LONG a = _mleft(obj) + _mright(obj) - d->x1[i], b = _mleft(obj) + _mright(obj) - d->x0[i];
    d->x0[i] = a; d->x1[i] = b;
  }
}
static void draw(SData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  int i;
  layout(d, obj);
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_STATUS));
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mtop(obj), cs_pen(CS_T_BORDER));
  for (i = 0; i < d->n; i++) {
    if (d->x1[i] < d->x0[i]) continue;
    cs_text_fit(rp, cs_font(), d->x0[i] + 3, _mtop(obj) + 3, d->text[i], d->x1[i] - d->x0[i] - 5, cs_pen(CS_T_DIM));
    if (i + 1 < d->n && d->w[i]) cs_fill(rp, d->side[i] == 'L' ? d->x1[i] + 1 : d->x0[i] - 1, _mtop(obj) + 3,
                                         d->side[i] == 'L' ? d->x1[i] + 1 : d->x0[i] - 1, _mbottom(obj) - 2, cs_pen(CS_T_BORDER2));
  }
}
CS_DISPATCHER(sdispatch)
{
  SData *d;
  switch (msg->MethodID) {
  case OM_NEW: {
    const char *spec;
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    /* "L120 L80 R60 L0": a side and a width each */
    for (spec = (const char *)GetTagData(MUIA_CsStatus_Fields, (ULONG)"L0", ((struct opSet *)msg)->ops_AttrList); *spec && d->n < MAXF; ) {
      char *end;
      while (*spec == ' ') spec++;
      if (*spec != 'L' && *spec != 'R') break;
      d->side[d->n] = *spec++;
      d->w[d->n++] = strtol(spec, &end, 10);
      spec = end;
    }
    CS_EHN(d, IDCMP_MOUSEBUTTONS);
    return (ULONG)obj;
  }
  case MUIM_CsStatus_Set: {
    LONG f = ((LONG *)msg)[1];
    const char *t = (const char *)((ULONG *)msg)[2];
    d = INST_DATA(cl, obj);
    if (f < 0 || f >= d->n || !strcmp(d->text[f], t ? t : "")) return 0;
    cs_strlcpy(d->text[f], t ? t : "", sizeof d->text[f]);
    if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
    return 1;
  }
  case MUIM_AskMinMax: {
    LONG h = cs_font()->tf_YSize + 6;
    DoSuperMethodA(cl, obj, msg);
    CS_MINMAX(msg, 60, 400, MUI_MAXMAX, h, h, h);
    return 0;
  }
  CS_CLASS_COMMON(SData);
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw(INST_DATA(cl, obj), obj);
    return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    int i;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown || im->Class != IDCMP_MOUSEBUTTONS || im->Code != SELECTDOWN || !CS_INSIDE(obj, im)) return 0;
    for (i = 0; i < d->n; i++) if (im->MouseX >= d->x0[i] && im->MouseX <= d->x1[i]) { SetAttrs(obj, MUIA_CsStatus_Click, i, TAG_DONE); return MUI_EventHandlerRC_Eat; }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_status_class(void)
{
  if (!scls) scls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(SData), (APTR)sdispatch);
  return scls;
}
void cs_status_free(void) { if (scls) { MUI_DeleteCustomClass(scls); scls = NULL; } }

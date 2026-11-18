/*
 * view.c - Jotter's text view: the document's lines (or rows, with Word Wrap) in QuillMono on the
 * paper colour, the selection lit, the caret a bar in the accent colour. It is the kit's pattern
 * for a document view (docs/platforms/amiga/04-classes.md, "document views"):
 *   - everything is drawn into an off-screen buffer (cs_buffer.c) and shown in one blit;
 *   - scrolling a few rows moves the buffer's picture with the blitter (ScrollRaster) and draws
 *     only the rows that came into view;
 *   - the view never changes the document itself: keys and clicks call the core (core/text.c),
 *     then the view sets MUIA_View_Changed and main.c refreshes the rest;
 *   - MUIA_FillArea is FALSE (it paints every pixel), and nothing is drawn while it is hidden.
 * Tabs are shown as one space each (Jotter is a plain-text editor; see the guide's limits).
 */
#include <string.h>
#include <stdlib.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "jotter.h"
#include "cs_theme.h"
#include "cs_classes.h"
#include "cs_ctx.h"
#include "cs_util.h"
#include "../ui/commands.h"

typedef struct { long line, col, len; } Row;
typedef struct {
  struct MUI_EventHandlerNode ehn;
  TextDoc *t;
  Object *vs, *hs;
  CsBuffer buf;
  Row *rows; long nrows, caprows, top, maxw;
  LONG left, width;
  int shown, wrap, size, dirty, dragging, ctx;
  struct TextFont *tf;
  ULONG lastsec, lastmic;
} VData;
static struct MUI_CustomClass *vcls;
#define PAD 4                                   /* pixels round the text */

static LONG lineh(VData *d) { return d->tf ? d->tf->tf_YSize + 1 : 9; }
static LONG visrows(VData *d, Object *obj) { LONG n = (_mheight(obj) - 2 * PAD) / lineh(d); return n > 0 ? n : 1; }
/* a string's width in the view's font (tabs and all, one character each) */
static LONG tlen(struct RastPort *rp, const char *s, long n) { return n > 0 ? TextLength(rp, (STRPTR)s, (ULONG)n) : 0; }
static void use_font(VData *d, struct RastPort *rp) { if (d->tf) SetFont(rp, d->tf); SetSoftStyle(rp, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED); }
/* the rows: one a line, or with Word Wrap each line broken at its last space that fits */
static void build_rows(VData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  LONG avail = _mwidth(obj) - 2 * PAD;
  long i;
  d->nrows = 0; d->maxw = 0;
  use_font(d, rp);
  for (i = 0; i < d->t->nlines; i++) {
    TLine *l = &d->t->lines[i];
    long col = 0;
    do {
      long n = l->len - col;
      if (d->nrows >= d->caprows) {
        long cap = d->caprows ? d->caprows * 2 : 256;
        Row *r = (Row *)realloc(d->rows, (size_t)cap * sizeof *r);
        if (!r) return;
        d->rows = r; d->caprows = cap;
      }
      if (d->wrap && avail > 20 && n > 0) {
        struct TextExtent te;
        long fit = (long)TextFit(rp, (STRPTR)l->s + col, (ULONG)n, &te, NULL, 1, (ULONG)avail, 32767), k;
        if (fit < n) {
          for (k = fit; k > 0 && l->s[col + k - 1] != ' '; k--) ;
          n = k > 0 ? k : (fit > 0 ? fit : 1);
        }
      } else if (!d->wrap) { LONG w = tlen(rp, l->s, l->len); if (w > d->maxw) d->maxw = w; }
      d->rows[d->nrows].line = i; d->rows[d->nrows].col = col; d->rows[d->nrows].len = n;
      d->nrows++;
      col += n;
    } while (col < l->len);
  }
  d->width = _mwidth(obj);
  d->dirty = 0;
}
/* the row holding p */
static long row_of(VData *d, TPos p)
{
  long lo = 0, hi = d->nrows - 1;
  while (lo < hi) {                                 /* the last row starting at or before p */
    long mid = (lo + hi + 1) / 2;
    if (d->rows[mid].line < p.line || (d->rows[mid].line == p.line && d->rows[mid].col <= p.col)) lo = mid; else hi = mid - 1;
  }
  /* a wrapped row's end belongs to the next row, but the line's very end to its last */
  return lo;
}
static void update_scrollbars(VData *d, Object *obj)
{
  if (d->vs) SetAttrs(d->vs, MUIA_NoNotify, TRUE, MUIA_Prop_Entries, d->nrows, MUIA_Prop_Visible, visrows(d, obj), MUIA_Prop_First, d->top, TAG_DONE);
  if (d->hs) SetAttrs(d->hs, MUIA_NoNotify, TRUE, MUIA_Prop_Entries, d->wrap ? 1 : d->maxw + 2 * PAD, MUIA_Prop_Visible, _mwidth(obj),
                      MUIA_Prop_First, d->left, TAG_DONE);
}
/* the caret brought into view by scrolling, if it isn't */
static void show_caret(VData *d, Object *obj)
{
  long r = row_of(d, d->t->caret), vr = visrows(d, obj);
  struct RastPort *rp = _rp(obj);
  LONG cx;
  if (r < d->top) d->top = r;
  if (r >= d->top + vr) d->top = r - vr + 1;
  if (d->top > d->nrows - vr) d->top = d->nrows - vr;
  if (d->top < 0) d->top = 0;
  if (!d->wrap) {
    use_font(d, rp);
    cx = tlen(rp, d->t->lines[d->t->caret.line].s, d->t->caret.col);
    if (cx < d->left) d->left = cx - _mwidth(obj) / 4;
    if (cx > d->left + _mwidth(obj) - 2 * PAD - 4) d->left = cx - _mwidth(obj) * 3 / 4;
    if (d->left < 0) d->left = 0;
  } else d->left = 0;
}
static TPos mkpos(long line, long col) { TPos p; p.line = line; p.col = col; return p; }
/* row r drawn at its place in rp (the buffer), its selection lit, the caret if it is there */
static void draw_row(VData *d, Object *obj, struct RastPort *rp, long r)
{
  LONG y = _mtop(obj) + PAD + (r - d->top) * lineh(d), x0 = _mleft(obj) + PAD - d->left;
  TPos a, b;
  Row *row;
  const char *s;
  cs_fill(rp, _mleft(obj), y, _mright(obj), y + lineh(d) - 1, cs_pen(CS_T_WHITE));
  if (r < 0 || r >= d->nrows) return;
  row = &d->rows[r];
  s = d->t->lines[row->line].s + row->col;
  use_font(d, rp);
  text_sel_range(d->t, &a, &b);
  if (text_has_sel(d->t)) {
    /* the part of this row inside the selection */
    long c0 = row->col, c1 = row->col + row->len;
    int eol = 0;
    if (text_cmp(a, mkpos(row->line, c1)) <= 0 && text_cmp(b, mkpos(row->line, c0)) >= 0) {
      long s0 = a.line == row->line && a.col > c0 ? a.col : (a.line < row->line || a.col <= c0 ? c0 : c1);
      long s1 = b.line == row->line && b.col < c1 ? b.col : (b.line > row->line ? c1 : (b.col >= c1 ? c1 : c0));
      eol = b.line > row->line && c1 == d->t->lines[row->line].len;
      if (s1 > s0 || eol) {
        LONG sx = x0 + tlen(rp, d->t->lines[row->line].s + c0, s0 - c0), ex = x0 + tlen(rp, d->t->lines[row->line].s + c0, s1 - c0) - 1 + (eol ? 6 : 0);
        cs_fill(rp, sx, y, ex, y + lineh(d) - 1, cs_pen(CS_T_SELECTION));
      }
    }
  }
  if (row->len) {
    SetAPen(rp, cs_pen(CS_T_TEXT)); SetDrMd(rp, JAM1);
    Move(rp, x0, y + (d->tf ? d->tf->tf_Baseline : 6));
    Text(rp, (STRPTR)s, (ULONG)row->len);
  }
  if (row_of(d, d->t->caret) == r) {
    LONG cx = x0 + tlen(rp, s, d->t->caret.col - row->col);
    cs_fill(rp, cx, y, cx + 1, y + lineh(d) - 1, cs_pen(CS_T_ACCENT));
  }
}
static void draw_rows(VData *d, Object *obj, long from, long to)
{
  struct RastPort *rp = cs_buf_begin(&d->buf, obj);
  long r;
  for (r = from; r <= to; r++) draw_row(d, obj, rp, r);
}
static void draw_all(VData *d, Object *obj)
{
  struct RastPort *rp;
  long vr = visrows(d, obj);
  LONG yend;
  if (d->dirty || d->width != _mwidth(obj)) build_rows(d, obj);
  rp = cs_buf_begin(&d->buf, obj);
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mtop(obj) + PAD - 1, cs_pen(CS_T_WHITE));
  draw_rows(d, obj, d->top, d->top + vr - 1);
  yend = _mtop(obj) + PAD + vr * lineh(d);
  cs_fill(rp, _mleft(obj), yend, _mright(obj), _mbottom(obj), cs_pen(CS_T_WHITE));
  cs_buf_show(&d->buf, obj, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
  update_scrollbars(d, obj);
}
/* scrolled to row top: with the blitter when it is a short way, else drawn again */
static void scroll_to(VData *d, Object *obj, long top)
{
  long vr = visrows(d, obj), old = d->top;
  struct Rectangle box[2];
  int nb;
  if (top > d->nrows - vr) top = d->nrows - vr;
  if (top < 0) top = 0;
  if (top == old) return;
  d->top = top;
  if (!d->shown) return;
  if (cs_buf_scroll(&d->buf, obj, 0, (top - old) * lineh(d), box, &nb)) {
    /* only the rows that came into view (and the margin at the foot, which the shift moved) */
    if (top > old) draw_rows(d, obj, old + vr - 1, top + vr);
    else draw_rows(d, obj, top, old);
    cs_fill(cs_buf_begin(&d->buf, obj), _mleft(obj), _mtop(obj), _mright(obj), _mtop(obj) + PAD - 1, cs_pen(CS_T_WHITE));
    cs_fill(cs_buf_begin(&d->buf, obj), _mleft(obj), _mtop(obj) + PAD + vr * lineh(d), _mright(obj), _mbottom(obj), cs_pen(CS_T_WHITE));
    cs_buf_show(&d->buf, obj, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj));
    update_scrollbars(d, obj);
  } else MUI_Redraw(obj, MADF_DRAWOBJECT);
}
/* the position under the pointer */
static TPos pos_at(VData *d, Object *obj, LONG mx, LONG my)
{
  long r = d->top + (my - _mtop(obj) - PAD) / lineh(d);
  TPos p;
  struct RastPort *rp = _rp(obj);
  struct TextExtent te;
  Row *row;
  LONG x = mx - _mleft(obj) - PAD + d->left;
  long fit;
  if (my < _mtop(obj) + PAD) r = d->top - (my < _mtop(obj) ? 1 : 0);
  if (r < 0) r = 0;
  if (r >= d->nrows) r = d->nrows - 1;
  row = &d->rows[r];
  p.line = row->line;
  use_font(d, rp);
  fit = x <= 0 ? 0 : (long)TextFit(rp, (STRPTR)d->t->lines[row->line].s + row->col, (ULONG)row->len, &te, NULL, 1, (ULONG)x, 32767);
  /* nearer the next character's left edge: after it */
  if (fit < row->len && x - te.te_Width > tlen(rp, d->t->lines[row->line].s + row->col + fit, 1) / 2) fit++;
  p.col = row->col + fit;
  if (d->wrap && r + 1 < d->nrows && d->rows[r + 1].line == row->line && p.col >= row->col + row->len && row->len) p.col = row->col + row->len - 1;
  return p;
}
static void changed(VData *d, Object *obj, int text)
{
  if (text) d->dirty = 1;
  if (d->dirty) build_rows(d, obj);
  show_caret(d, obj);
  if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT);
  SetAttrs(obj, MUIA_View_Changed, TRUE, TAG_DONE);
}
/* a key: 1 if the view used it */
static int key(VData *d, Object *obj, struct IntuiMessage *im)
{
  UWORD q = im->Qualifier, c = im->Code;
  int sh = (q & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0, alt = (q & (IEQUALIFIER_LALT | IEQUALIFIER_RALT)) != 0;
  TextDoc *t = d->t;
  if (q & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) return 0;      /* the menus' shortcuts */
  switch (c) {
  case 0x4C: text_move(t, alt ? MV_TOP : MV_UP, sh); changed(d, obj, 0); return 1;              /* cursor up (Alt: the top) */
  case 0x4D: text_move(t, alt ? MV_BOTTOM : MV_DOWN, sh); changed(d, obj, 0); return 1;
  case 0x4F: text_move(t, alt ? MV_WORDLEFT : MV_LEFT, sh); changed(d, obj, 0); return 1;
  case 0x4E: text_move(t, alt ? MV_WORDRIGHT : MV_RIGHT, sh); changed(d, obj, 0); return 1;
  case 0x70: text_move(t, MV_HOME, sh); changed(d, obj, 0); return 1;                 /* Home (PC keyboards) */
  case 0x71: text_move(t, MV_END, sh); changed(d, obj, 0); return 1;                  /* End */
  case 0x48: case 0x49: {                                                            /* Page Up, Page Down */
    long vr = visrows(d, obj), i;
    for (i = 0; i < vr - 1; i++) text_move(t, c == 0x48 ? MV_UP : MV_DOWN, sh);
    changed(d, obj, 0);
    return 1; }
  case 0x41: text_backspace(t); changed(d, obj, 1); return 1;
  case 0x46: text_delete(t); changed(d, obj, 1); return 1;
  case 0x44: case 0x43: text_insert(t, "\n", 1); changed(d, obj, 1); return 1;
  case 0x42: if (sh) text_indent(t, 1); else text_insert(t, "  ", 2); changed(d, obj, 1); return 1;   /* Tab: two spaces */
  case 0x7A: scroll_to(d, obj, d->top - 3); return 1;                                 /* mouse wheel (NewMouse) */
  case 0x7B: scroll_to(d, obj, d->top + 3); return 1;
  }
  if (!(c & IECODE_UP_PREFIX) && c < 0x60) {
    struct InputEvent ie;
    UBYTE buf[8];
    LONG n;
    ie.ie_NextEvent = NULL; ie.ie_Class = IECLASS_RAWKEY; ie.ie_SubClass = 0;
    ie.ie_Code = c; ie.ie_Qualifier = q; ie.ie_EventAddress = (APTR *)*((ULONG *)im->IAddress);
    n = MapRawKey(&ie, (STRPTR)buf, sizeof buf, NULL);
    if (n == 1 && (buf[0] >= 32 && buf[0] != 127)) { text_insert(t, (const char *)buf, 1); changed(d, obj, 1); return 1; }
  }
  return 0;
}
static ULONG vdispatch(__reg("a0") struct IClass *cl, __reg("a2") Object *obj, __reg("a1") Msg msg)
{
  VData *d;
  switch (msg->MethodID) {
  case OM_NEW: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList;
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    d->t = (TextDoc *)GetTagData(MUIA_View_Text, 0, tags);
    d->vs = (Object *)GetTagData(MUIA_View_VScroll, 0, tags);
    d->hs = (Object *)GetTagData(MUIA_View_HScroll, 0, tags);
    d->wrap = (int)GetTagData(MUIA_View_Wrap, 0, tags);
    d->size = (int)GetTagData(MUIA_View_FontSize, 1, tags);
    d->dirty = 1;
    d->ehn.ehn_Object = obj; d->ehn.ehn_Class = cl; d->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY;
    return (ULONG)obj;
  }
  case OM_DISPOSE:
    d = INST_DATA(cl, obj);
    free(d->rows);
    break;
  case OM_SET: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList, *t;
    d = INST_DATA(cl, obj);
    if ((t = FindTagItem(MUIA_View_Top, tags))) scroll_to(d, obj, (long)t->ti_Data);
    if ((t = FindTagItem(MUIA_View_Left, tags)) && (LONG)t->ti_Data != d->left) { d->left = (LONG)t->ti_Data; if (d->shown) MUI_Redraw(obj, MADF_DRAWOBJECT); }
    if ((t = FindTagItem(MUIA_View_Wrap, tags)) && (int)t->ti_Data != d->wrap) { d->wrap = (int)t->ti_Data; d->left = 0; d->dirty = 1; if (d->shown) { build_rows(d, obj); show_caret(d, obj); MUI_Redraw(obj, MADF_DRAWOBJECT); } }
    if ((t = FindTagItem(MUIA_View_FontSize, tags)) && (int)t->ti_Data != d->size) {
      d->size = (int)t->ti_Data;
      if (d->tf) {                                  /* the font changed while shown: opened again here */
        cs_close_font(d->tf);
        d->tf = cs_open_font("QuillMono.font", (UWORD)(d->size == 0 ? 11 : d->size == 2 ? 17 : 14));
        d->dirty = 1;
        if (d->shown) { build_rows(d, obj); show_caret(d, obj); MUI_Redraw(obj, MADF_DRAWOBJECT); }
      }
    }
    break;
  }
  case OM_GET:
    if (((struct opGet *)msg)->opg_AttrID == MUIA_View_Wrap) { *((struct opGet *)msg)->opg_Storage = (ULONG)((VData *)INST_DATA(cl, obj))->wrap; return TRUE; }
    if (((struct opGet *)msg)->opg_AttrID == MUIA_View_Changed) { *((struct opGet *)msg)->opg_Storage = TRUE; return TRUE; }
    break;
  case MUIM_AskMinMax: {
    struct MUI_MinMax *mm = ((struct MUIP_AskMinMax *)msg)->MinMaxInfo;
    DoSuperMethodA(cl, obj, msg);
    mm->MinWidth += 80; mm->DefWidth += 400; mm->MaxWidth += MUI_MAXMAX;
    mm->MinHeight += 40; mm->DefHeight += 300; mm->MaxHeight += MUI_MAXMAX;
    return 0;
  }
  case MUIM_Setup:
    if (!DoSuperMethodA(cl, obj, msg)) return FALSE;
    d = INST_DATA(cl, obj);
    cs_theme_setup(obj);
    cs_theme_handler(obj, &d->ehn);
    cs_ctx_attach(obj);
    d->tf = cs_open_font("QuillMono.font", (UWORD)(d->size == 0 ? 11 : d->size == 2 ? 17 : 14));
    d->dirty = 1;
    return TRUE;
  case MUIM_Cleanup:
    d = INST_DATA(cl, obj);
    cs_buf_free(&d->buf);
    if (d->tf) { cs_close_font(d->tf); d->tf = NULL; }
    cs_theme_cleanup(obj);
    break;
  case MUIM_Show: ((VData *)INST_DATA(cl, obj))->shown = 1; break;
  case MUIM_Hide: d = INST_DATA(cl, obj); d->shown = 0; cs_buf_free(&d->buf); break;
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw_all(INST_DATA(cl, obj), obj);
    return 0;
  case MUIM_View_Refresh:
    d = INST_DATA(cl, obj);
    d->dirty = 1;
    if (d->shown) { build_rows(d, obj); show_caret(d, obj); MUI_Redraw(obj, MADF_DRAWOBJECT); }
    return 0;
  case MUIM_ContextMenuBuild: {
    struct MUIP_ContextMenuBuild *m = (struct MUIP_ContextMenuBuild *)msg;
    d = INST_DATA(cl, obj);
    /* a click outside the selection moves the caret there first, as a left click would */
    if (d->shown && !d->dirty) {
      TPos p = pos_at(d, obj, m->mx, m->my), a, b;
      text_sel_range(d->t, &a, &b);
      if (!text_has_sel(d->t) || text_cmp(p, a) < 0 || text_cmp(p, b) > 0) { text_set_caret(d->t, p, 0); changed(d, obj, 0); }
    }
    return (ULONG)cs_ctx_menu("Text", jotter_ctx_view, jotter_nctx_view);
  }
  case MUIM_ContextMenuChoice:
    cs_ctx_choice(((struct MUIP_ContextMenuChoice *)msg)->item);
    return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown || !d->nrows) return 0;
    if (im->Class == IDCMP_RAWKEY) return key(d, obj, im) ? MUI_EventHandlerRC_Eat : 0;
    if (im->Class == IDCMP_MOUSEBUTTONS) {
      if (im->Code == SELECTDOWN && (im->MouseX >= _mleft(obj) && im->MouseX <= _mright(obj) && im->MouseY >= _mtop(obj) && im->MouseY <= _mbottom(obj))) {
        TPos p = pos_at(d, obj, im->MouseX, im->MouseY);
        int sh = (im->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
        set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
        if (!sh && DoubleClick(d->lastsec, d->lastmic, im->Seconds, im->Micros) && !text_cmp(p, d->t->caret)) text_select_word(d->t, p);
        else text_set_caret(d->t, p, sh);
        d->lastsec = im->Seconds; d->lastmic = im->Micros;
        d->dragging = 1;
        changed(d, obj, 0);
        return MUI_EventHandlerRC_Eat;
      }
      if (im->Code == SELECTUP && d->dragging) { d->dragging = 0; return MUI_EventHandlerRC_Eat; }
      return 0;
    }
    if (im->Class == IDCMP_MOUSEMOVE && d->dragging) {
      TPos p = pos_at(d, obj, im->MouseX, im->MouseY);
      if (text_cmp(p, d->t->caret)) { text_set_caret(d->t, p, 1); changed(d, obj, 0); }
      return MUI_EventHandlerRC_Eat;
    }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *view_class(void)
{
  if (!vcls) vcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(VData), (APTR)vdispatch);
  return vcls;
}
void view_class_free(void) { if (vcls) { MUI_DeleteCustomClass(vcls); vcls = NULL; } }

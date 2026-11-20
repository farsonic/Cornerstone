/*
 * cs_ribbonmui.c - the ribbon as an MUI class: it lays a CsRibbonDef out with cs_rb_layout()
 * (kit/shared/cs_ribbon.c) at the window's width and draws it in the theme; reports a command
 * chosen by setting MUIA_CsRibbon_Command (with the list entry in MUIA_CsRibbon_Value), so a tool and
 * its menu item reach the main loop as the same number:
 *
 *   DoMethod(ribbon, MUIM_Notify, MUIA_CsRibbon_Command, MUIV_EveryTime,
 *            (ULONG)app, 2, MUIM_Application_ReturnID, MUIV_TriggerValue);
 *
 * Every item's look (enabled, ticked) comes from cs_cmd_state(), asked again on
 * MUIM_CsRibbon_Refresh; groups that don't fit collapse to buttons that open their tools as a
 * list; Alt (or F10) shows KeyTips; the File tab, Quick Access buttons and the Tell Me box send
 * their commands like any tool. Bubbles name the tool, its shortcut and its help.
 */
#include <string.h>
#include <stdio.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/keymap.h>
#include "cs_amiga.h"
#include "cs_classes.h"
#include "cs_common.h"
#include "cs_cmd.h"
#include "cs_lang.h"
#include "cs_util.h"
#include "cs_icon.h"

#define MUIA_CsRibbon_PopChoice CS_TAG(0x120)   /* (private) the entry chosen in a list the ribbon opened */
#define NCODES (1 + CS_RB_MAXTABS + CS_RB_MAXQA + 1)

typedef struct {
  CS_CLASS_HEAD;
  const CsRibbonDef *def;
  CsRbLayout *l;                               /* (AllocVec: it is a few KB) */
  int tab, minimised, dirty, hotkind, hot, presskind, press;
  LONG value, width, ox, oy;
  int popitem, popgroup;                       /* the list open: for item popitem, or group popgroup's tools */
  int popmap[64];                              /* a group's list: entry -> item */
  const char *poplabels[64];
  int popicons[64];
  char popnames[64][48];
  int kt;                                      /* KeyTips: 0 off, 1 tabs, 2 items */
  char ktyped[4], tcodes[NCODES][3], icodes[CS_RB_MAXITEMS][3];
  UWORD lastkey;
  int vis[CS_RB_MAXTABS];                      /* the tabs visible at the last layout */
  UBYTE st[CS_RB_MAXITEMS];                    /* each item's state when last drawn */
} RData;
static struct MUI_CustomClass *rcls;

static long measure(void *ctx, const char *s) { struct RastPort *rp = (struct RastPort *)ctx; SetFont(rp, cs_font()); return TextLength(rp, (STRPTR)s, (ULONG)strlen(s)); }
static void relayout(RData *d, Object *obj)
{
  CsRbMetrics m;
  int t;
  m.fh = cs_font()->tf_YSize; m.textw = measure; m.ctx = _rp(obj);
  cs_rb_layout(d->def, d->tab, _mwidth(obj), cs_rtl, d->minimised, &m, d->l);
  d->tab = d->l->tab;
  d->width = _mwidth(obj);
  d->ox = _mleft(obj); d->oy = _mtop(obj);
  for (t = 0; t < CS_RB_MAXTABS; t++) d->vis[t] = cs_rb_tab_visible(d->def, t);
  d->dirty = 0;
  if (d->kt == 2) cs_rb_keytips_items(d->def, d->l, d->icodes);
}
static int item_state(const CsRbItem *it) { return it && it->kind != CSK_LABEL ? cs_cmd_state(it->id) : CSS_ENABLED; }

/* ---------------------------------------------------------------- drawing */
#define BX(b) (d->ox + (b).x0)
#define BX1(b) (d->ox + (b).x1)
#define BY(b) (d->oy + (b).y0)
#define BY1(b) (d->oy + (b).y1)
static int fh(void) { return cs_font()->tf_YSize; }
/* a label on one line, or two at its best space when it is too wide, centred under a large icon */
static void large_label(struct RastPort *rp, const char *s, LONG cx, LONG y, LONG maxw, LONG pen, int arrow)
{
  int n = (int)strlen(s), best = 0, i;
  LONG w = cs_textlen(rp, cs_font(), s), bw = w;
  char a[64];
  if (w > maxw) for (i = 1; i < n - 1; i++) if (s[i] == ' ' && i < 63) {
    LONG wa, wb;
    memcpy(a, s, (size_t)i); a[i] = 0;
    wa = TextLength(rp, (STRPTR)a, (ULONG)i); wb = TextLength(rp, (STRPTR)s + i + 1, (ULONG)(n - i - 1));
    if ((wa > wb ? wa : wb) < bw) { bw = wa > wb ? wa : wb; best = i; }
  }
  if (!best) {
    cs_text_fit(rp, cs_font(), cx - (w < maxw ? w : maxw) / 2, y, s, maxw, pen);
    if (arrow) cs_icon(rp, CS_IC_ARROWDOWN, cx - 4, y + fh(), 8, 0);
    return;
  }
  memcpy(a, s, (size_t)best); a[best] = 0;
  cs_text(rp, cs_font(), cx - TextLength(rp, (STRPTR)a, (ULONG)best) / 2, y, a, pen);
  w = TextLength(rp, (STRPTR)s + best + 1, (ULONG)(n - best - 1));
  cs_text(rp, cs_font(), cx - (w + (arrow ? 10 : 0)) / 2, y + fh(), s + best + 1, pen);
  if (arrow) cs_icon(rp, CS_IC_ARROWDOWN, cx - (w + 10) / 2 + w + 2, y + fh() + (fh() - 8) / 2, 8, 0);
}
static void draw_item(RData *d, Object *obj, int k)
{
  struct RastPort *rp = _rp(obj);
  CsRbLayout *l = d->l;
  const CsRbItem *it = l->it[k];
  LONG x0 = BX(l->item[k]), y0 = BY(l->item[k]), x1 = BX1(l->item[k]), y1 = BY1(l->item[k]);
  int s = item_state(it), en = (s & CSS_ENABLED) != 0, on = (s & CSS_CHECKED) != 0, icon;
  int hot = d->hotkind >= CS_HIT_ITEM && d->hot == k, pr = d->presskind >= CS_HIT_ITEM && d->press == k;
  char name[64];
  LONG bg = cs_pen(CS_T_RIBBON), pen;
  d->st[k] = (UBYTE)s;
  if (!it) {                                      /* a collapsed group: its first icon, its name, an arrow */
    const CsRbGroup *g = &d->def->tabs[d->tab].groups[l->igroup[k]];
    int i, ic = 0;
    for (i = 0; i < g->n && !ic; i++) ic = cs_rb_item_icon(&g->items[i]);
    cs_fill(rp, x0, y0, x1, y1, pr ? cs_pen(CS_T_PRESSED) : hot ? cs_pen(CS_T_HOVER) : bg);
    cs_icon(rp, ic ? ic : CS_IC_MORE, (x0 + x1 + 1) / 2 - 12, y0 + 4, 24, 0);
    large_label(rp, tr(g->name), (x0 + x1 + 1) / 2, y0 + 30, x1 - x0 - 1, cs_pen(CS_T_TEXT), 1);
    return;
  }
  if (it->kind != CSK_LABEL && en && (on || hot || pr))
    bg = pr ? cs_pen(CS_T_PRESSED) : on && it->kind != CSK_CHECK ? cs_pen(CS_T_CHECKED) : cs_pen(CS_T_HOVER);
  cs_fill(rp, x0, y0, x1, y1, bg);
  if (on && it->kind != CSK_CHECK && en) cs_rect(rp, x0, y0, x1, y1, cs_pen(CS_T_ACCENT));
  pen = cs_pen(en ? CS_T_TEXT : CS_T_DISABLED);
  cs_rb_item_name(it, name, sizeof name);
  icon = cs_rb_item_icon(it);
  if (it->kind == CSK_LABEL) { cs_text(rp, cs_font(), x0 + 3, y0 + 3, name, cs_pen(CS_T_DIM)); return; }
  if (it->size == CSZ_LARGE) {
    cs_icon(rp, icon, (x0 + x1 + 1) / 2 - 12, y0 + 4, 24, !en);
    large_label(rp, name, (x0 + x1 + 1) / 2, y0 + 30, x1 - x0 - 1, pen, it->kind == CSK_DROP || it->kind == CSK_SPLIT);
    if (it->kind == CSK_SPLIT && (hot || pr)) cs_fill(rp, x0 + 2, y1 - (y1 - y0) / 3, x1 - 2, y1 - (y1 - y0) / 3, cs_pen(CS_T_BORDER2));
    return;
  }
  {
    LONG ix = cs_rtl ? x1 - 19 : x0 + 3, iy = y0 + (y1 - y0 + 1 - 16) / 2, tx;
    int arrow = it->kind == CSK_DROP || it->kind == CSK_SPLIT, bare = it->size == CSZ_ICON || (l->squeeze && icon && it->kind != CSK_CHECK);
    if (it->kind == CSK_CHECK) {
      LONG bx = cs_rtl ? x1 - 15 : x0 + 4, by = y0 + (y1 - y0 + 1 - 11) / 2;
      cs_fill(rp, bx, by, bx + 10, by + 10, cs_pen(on ? CS_T_ACCENT : CS_T_WHITE));
      cs_rect(rp, bx, by, bx + 10, by + 10, cs_pen(en ? (on ? CS_T_ACCENT : CS_T_INPUTBORDER) : CS_T_DISABLED));
      if (on) cs_icon_pen(rp, CS_IC_TICK, bx, by, 11, cs_pen(CS_T_ONACCENT));
    } else if (icon) cs_icon(rp, icon, bare ? (cs_rtl ? x1 - 19 - (arrow ? 0 : 0) : x0 + 3) : ix, iy, 16, !en);
    if (!bare || it->kind == CSK_CHECK) {
      tx = cs_rtl ? x1 - 22 - cs_textlen(rp, cs_font(), name) : x0 + 22;
      cs_text(rp, cs_font(), tx, y0 + (y1 - y0 + 1 - fh()) / 2, name, pen);
    }
    if (arrow) {
      LONG ax = cs_rtl ? x0 + 1 : x1 - 9;
      if (it->kind == CSK_SPLIT && (hot || pr)) cs_fill(rp, cs_rtl ? x0 + 10 : x1 - 10, y0 + 2, cs_rtl ? x0 + 10 : x1 - 10, y1 - 2, cs_pen(CS_T_BORDER2));
      cs_icon(rp, CS_IC_ARROWDOWN, ax, y0 + (y1 - y0 + 1 - 8) / 2, 8, !en);
    }
  }
}
static void draw_tabs(RData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  CsRbLayout *l = d->l;
  LONG th = cs_rb_tabh(fh());
  int t, i;
  cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mtop(obj) + th + 1, cs_pen(CS_T_RIBBON));
  if (l->file.x0 >= 0) {
    int hot = d->hotkind == CS_HIT_FILE;
    cs_fill(rp, BX(l->file), BY(l->file) + 2, BX1(l->file), BY1(l->file), cs_pen(hot ? CS_T_ACCENTHOVER : CS_T_ACCENT));
    cs_text_fit(rp, cs_font(), BX(l->file) + (l->file.x1 - l->file.x0 + 1 - cs_textlen(rp, cs_font(), "File")) / 2, BY(l->file) + 6, "File",
                l->file.x1 - l->file.x0 - 2, cs_pen(CS_T_ONACCENT));
  }
  for (t = 0; t < d->def->ntabs && t < CS_RB_MAXTABS; t++) {
    const CsBox *b = &l->tabbox[t];
    const char *nm = tr(d->def->tabs[t].name);
    LONG w;
    if (b->x0 < 0) continue;
    if (d->hotkind == CS_HIT_TAB && d->hot == t && t != d->tab) cs_fill(rp, BX(*b), BY(*b) + 2, BX1(*b), BY1(*b), cs_pen(CS_T_HOVER));
    w = cs_textlen(rp, t == d->tab && !d->minimised ? cs_bold() : cs_font(), nm);
    if (l->tabcap && w > l->tabcap) w = l->tabcap;
    cs_text_fit(rp, t == d->tab && !d->minimised ? cs_bold() : cs_font(), BX(*b) + (b->x1 - b->x0 + 1 - w) / 2, BY(*b) + 6, nm, w,
                cs_pen(t == d->tab ? CS_T_ACCENT : CS_T_TEXT));
    if (t == d->tab && !d->minimised) cs_fill(rp, BX(*b) + 4, BY1(*b) - 2, BX1(*b) - 4, BY1(*b), cs_pen(CS_T_ACCENT));
    if (d->def->tabs[t].context) cs_fill(rp, BX(*b) + 2, BY(*b) + 2, BX1(*b) - 2, BY(*b) + 2, cs_pen(CS_T_ACCENT));   /* contextual: a line over it */
  }
  for (i = 0; i < CS_RB_MAXQA && i < d->def->nqa; i++) {
    const CsBox *b = &l->qa[i];
    const CsCommand *c;
    if (b->x0 < 0) continue;
    c = cs_cmd_find(d->def->qa[i]);
    if (d->hotkind == CS_HIT_QA && d->hot == i) cs_fill(rp, BX(*b), BY(*b) + 1, BX1(*b), BY1(*b) - 1, cs_pen(d->presskind == CS_HIT_QA ? CS_T_PRESSED : CS_T_HOVER));
    if (c) cs_icon(rp, c->icon, BX(*b) + 3, BY(*b) + (b->y1 - b->y0 + 1 - 16) / 2, 16, !(cs_cmd_state(c->id) & CSS_ENABLED));
  }
  if (l->tell.x0 >= 0) {
    const CsBox *b = &l->tell;
    cs_fill(rp, BX(*b), BY(*b), BX1(*b), BY1(*b), cs_pen(d->hotkind == CS_HIT_TELL ? CS_T_HOVER : CS_T_WHITE));
    cs_rect(rp, BX(*b), BY(*b), BX1(*b), BY1(*b), cs_pen(CS_T_INPUTBORDER));
    cs_icon(rp, CS_IC_SEARCH, cs_rtl ? BX1(*b) - 17 : BX(*b) + 3, BY(*b) + (b->y1 - b->y0 + 1 - 14) / 2, 14, 0);
    if (b->x1 - b->x0 > 40) cs_text_fit(rp, cs_font(), cs_rtl ? BX(*b) + 4 : BX(*b) + 21, BY(*b) + (b->y1 - b->y0 + 1 - fh()) / 2, "Tell me", b->x1 - b->x0 - 24, cs_pen(CS_T_DIM));
  }
}
static void kt_badge(struct RastPort *rp, LONG cx, LONG y, const char *code, int dim)
{
  LONG w = cs_textlen(rp, cs_small(), code) + 6, h = cs_small()->tf_YSize + 2;
  cs_fill(rp, cx - w / 2, y, cx - w / 2 + w - 1, y + h - 1, cs_pen(dim ? CS_T_RIBBON : CS_T_TEXT));
  cs_rect(rp, cx - w / 2, y, cx - w / 2 + w - 1, y + h - 1, cs_pen(CS_T_BORDER2));
  cs_text(rp, cs_small(), cx - w / 2 + 3, y + 1, code, cs_pen(dim ? CS_T_DIM : CS_T_WHITE));
}
static void kt_draw(RData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  CsRbLayout *l = d->l;
  size_t tl = strlen(d->ktyped);
  int i, nt = d->def->ntabs;
  if (d->kt == 1) {
    for (i = 0; i < 1 + nt + d->def->nqa + 1 && i < NCODES; i++) {
      const CsBox *b = i == 0 ? &l->file : i <= nt ? &l->tabbox[i - 1] : i <= nt + d->def->nqa ? &l->qa[i - 1 - nt] : &l->tell;
      if (!d->tcodes[i][0] || b->x0 < 0 || strncmp(d->tcodes[i], d->ktyped, tl)) continue;
      kt_badge(rp, d->ox + (b->x0 + b->x1) / 2, d->oy + b->y1 - 4, d->tcodes[i], 0);
    }
  } else if (d->kt == 2)
    for (i = 0; i < l->nitem; i++) {
      if (!d->icodes[i][0] || strncmp(d->icodes[i], d->ktyped, tl)) continue;
      kt_badge(rp, d->ox + (l->item[i].x0 + l->item[i].x1) / 2, d->oy + l->item[i].y1 - cs_small()->tf_YSize, d->icodes[i],
               !(item_state(l->it[i]) & CSS_ENABLED));
    }
}
static void draw_all(RData *d, Object *obj)
{
  struct RastPort *rp = _rp(obj);
  CsRbLayout *l = d->l;
  int g, k;
  if (d->dirty || d->width != _mwidth(obj) || d->ox != _mleft(obj) || d->oy != _mtop(obj)) relayout(d, obj);
  draw_tabs(d, obj);
  if (!d->minimised) {
    LONG y0 = _mtop(obj) + cs_rb_tabh(fh()) + 2;
    cs_fill(rp, _mleft(obj), y0, _mright(obj), _mbottom(obj), cs_pen(CS_T_RIBBON));
    for (g = 0; g < l->ngroup; g++) {
      const CsRbGroup *gr = &d->def->tabs[d->tab].groups[g];
      LONG gx0 = BX(l->group[g]), gx1 = BX1(l->group[g]), cy = BY1(l->group[g]) - fh() - 1, w;
      if (!l->collapsed[g]) {
        w = cs_textlen(rp, cs_font(), gr->name);
        cs_text_fit(rp, cs_font(), (gx0 + gx1) / 2 - (w < gx1 - gx0 - 6 ? w : gx1 - gx0 - 6) / 2, cy, gr->name, gx1 - gx0 - 6, cs_pen(CS_T_GROUPLABEL));
      }
      if (cs_rtl) cs_fill(rp, gx0, y0 + 4, gx0, BY1(l->group[g]) - 2, cs_pen(CS_T_BORDER));
      else cs_fill(rp, gx1, y0 + 4, gx1, BY1(l->group[g]) - 2, cs_pen(CS_T_BORDER));
    }
    for (k = 0; k < l->nitem; k++) draw_item(d, obj, k);
  }
  cs_fill(rp, _mleft(obj), _mbottom(obj), _mright(obj), _mbottom(obj), cs_pen(CS_T_BORDER));
  if (d->kt) kt_draw(d, obj);
}

/* ---------------------------------------------------------------- acting */
static void send(Object *obj, RData *d, ULONG id, LONG value)
{
  if (!id) return;
  d->value = value;
  SetAttrs(obj, MUIA_CsRibbon_Command, id, TAG_DONE);
}
static void count_list(const char *const *list, int *n) { *n = 0; while (list && list[*n] && *n < 64) (*n)++; }
/* item k's list opened under it */
static void open_list(RData *d, Object *obj, int k)
{
  const CsRbItem *it = d->l->it[k];
  struct Window *w = _window(obj);
  int n;
  if (!it || !it->list || !w) return;
  count_list(it->list, &n);
  d->popitem = k; d->popgroup = -1;
  cs_popup(obj, w->LeftEdge + (cs_rtl ? BX1(d->l->item[k]) : BX(d->l->item[k])), w->TopEdge + BY1(d->l->item[k]) + 1,
           d->l->item[k].x1 - d->l->item[k].x0 + 1, it->list, NULL, n, -1, MUIA_CsRibbon_PopChoice);
}
/* a collapsed group's tools as a list under its button */
static void open_group(RData *d, Object *obj, int k)
{
  const CsRbGroup *g = &d->def->tabs[d->tab].groups[d->l->igroup[k]];
  struct Window *w = _window(obj);
  int i, n = 0;
  if (!w) return;
  for (i = 0; i < g->n && n < 64; i++) {
    const CsRbItem *it = &g->items[i];
    int s;
    if (it->kind == CSK_LABEL) continue;
    s = cs_cmd_state(it->id);
    cs_rb_item_name(it, d->popnames[n], sizeof d->popnames[n] - 4);
    if (it->kind == CSK_DROP) strcat(d->popnames[n], "...");
    d->poplabels[n] = d->popnames[n];
    d->popicons[n] = cs_rb_item_icon(it) | ((s & CSS_ENABLED) ? 0 : CS_POP_OFF) | ((s & CSS_CHECKED) ? CS_POP_TICK : 0);
    d->popmap[n++] = i;
  }
  d->popgroup = d->l->igroup[k]; d->popitem = k;
  cs_popup(obj, w->LeftEdge + (cs_rtl ? BX1(d->l->item[k]) : BX(d->l->item[k])), w->TopEdge + BY1(d->l->item[k]) + 1,
           d->l->item[k].x1 - d->l->item[k].x0 + 1, d->poplabels, d->popicons, n, -1, MUIA_CsRibbon_PopChoice);
}
static void pop_choice(RData *d, Object *obj, LONG i)
{
  if (d->popgroup >= 0) {
    const CsRbGroup *g = &d->def->tabs[d->tab].groups[d->popgroup];
    const CsRbItem *it;
    int gi = d->popgroup;
    d->popgroup = -1;
    if (i < 0 || i >= 64) return;
    it = &g->items[d->popmap[i]];
    if (it->kind == CSK_DROP && it->list) {
      /* the tool's own list, where the group's was */
      struct Window *w = _window(obj);
      int n, k = d->popitem;
      count_list(it->list, &n);
      d->popitem = -1;
      d->popgroup = -2 - gi;                             /* (remembered: the entry then goes to this tool) */
      d->popmap[0] = (int)(it - g->items);
      if (w) cs_popup(obj, w->LeftEdge + BX(d->l->item[k]), w->TopEdge + BY1(d->l->item[k]) + 1, 60, it->list, NULL, n, -1, MUIA_CsRibbon_PopChoice);
      return;
    }
    send(obj, d, it->id, -1);
    return;
  }
  if (d->popgroup <= -2) {                               /* a list opened from a collapsed group */
    const CsRbItem *it = &d->def->tabs[d->tab].groups[-2 - d->popgroup].items[d->popmap[0]];
    d->popgroup = -1;
    send(obj, d, it->id, i);
    return;
  }
  if (d->popitem >= 0 && d->popitem < d->l->nitem && d->l->it[d->popitem]) send(obj, d, d->l->it[d->popitem]->id, i);
  d->popitem = -1;
}
/* the thing at k (an item index) pressed: by a click, a KeyTip or Tell Me */
static void press(RData *d, Object *obj, int kind, int k)
{
  const CsRbItem *it;
  switch (kind) {
  case CS_HIT_FILE: send(obj, d, d->def->file, -1); return;
  case CS_HIT_TELL: send(obj, d, d->def->tellme, -1); return;
  case CS_HIT_QA: if (k >= 0 && k < d->def->nqa && (cs_cmd_state(d->def->qa[k]) & CSS_ENABLED)) send(obj, d, d->def->qa[k], -1); return;
  case CS_HIT_TAB: if (k != d->tab || d->minimised) { if (d->minimised) d->minimised = 0; SetAttrs(obj, MUIA_CsRibbon_Tab, k, TAG_DONE); } return;
  case CS_HIT_GROUP: open_group(d, obj, k); return;
  }
  if (k < 0 || k >= d->l->nitem || !(it = d->l->it[k]) || !(item_state(it) & CSS_ENABLED)) return;
  if (kind == CS_HIT_ARROW || it->kind == CSK_DROP) open_list(d, obj, k);
  else send(obj, d, it->id, -1);
}
static void redraw(Object *obj) { MUI_Redraw(obj, MADF_DRAWOBJECT); }
static void kt_stop(RData *d, Object *obj) { if (d->kt) { d->kt = 0; d->ktyped[0] = 0; redraw(obj); } }
/* a key while the KeyTips are shown: 1 if it was theirs */
static int kt_key(RData *d, Object *obj, int c)
{
  int m, len;
  if (c == 27) {
    if (d->ktyped[0]) d->ktyped[0] = 0; else if (d->kt == 2) d->kt = 1; else d->kt = 0;
    redraw(obj);
    return 1;
  }
  c = cs_fold(c);
  if (c >= 'a' && c <= 'z') c -= 32;
  if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return 1;
  len = (int)strlen(d->ktyped);
  if (len >= 2) len = 0;
  d->ktyped[len] = (char)c; d->ktyped[len + 1] = 0;
  if (d->kt == 1) {
    m = cs_keytips_match(d->tcodes, 1 + d->def->ntabs + d->def->nqa + 1, d->ktyped);
    if (m == -1) { redraw(obj); return 1; }
    d->ktyped[0] = 0;
    if (m == -2) { redraw(obj); return 1; }
    if (m >= 1 && m <= d->def->ntabs) {
      d->kt = 2; d->tab = m - 1; d->minimised = 0; d->dirty = 1;
      SetAttrs(obj, MUIA_CsRibbon_Tab, d->tab, TAG_DONE);
      relayout(d, obj);
      cs_rb_keytips_items(d->def, d->l, d->icodes);
      redraw(obj);
      return 1;
    }
    d->kt = 0;
    redraw(obj);
    if (m == 0) press(d, obj, CS_HIT_FILE, 0);
    else if (m <= d->def->ntabs + d->def->nqa) press(d, obj, CS_HIT_QA, m - 1 - d->def->ntabs);
    else press(d, obj, CS_HIT_TELL, 0);
    return 1;
  }
  m = cs_keytips_match(d->icodes, d->l->nitem, d->ktyped);
  if (m == -1) { redraw(obj); return 1; }
  d->ktyped[0] = 0;
  if (m == -2) { redraw(obj); return 1; }
  d->kt = 0;
  redraw(obj);
  press(d, obj, d->l->it[m] ? CS_HIT_ITEM : CS_HIT_GROUP, m);
  return 1;
}
static void kt_start(RData *d, Object *obj)
{
  if (d->dirty) relayout(d, obj);
  d->kt = 1; d->ktyped[0] = 0;
  cs_rb_keytips_tabs(d->def, d->l, d->tcodes);
  redraw(obj);
}
static int hit(RData *d, Object *obj, LONG mx, LONG my, int *k)
{
  if (mx < _mleft(obj) || mx > _mright(obj) || my < _mtop(obj) || my > _mbottom(obj)) { *k = -1; return CS_HIT_NONE; }
  return cs_rb_hit(d->l, (int)(mx - d->ox), (int)(my - d->oy), k);
}

CS_DISPATCHER(rdispatch)
{
  RData *d;
  switch (msg->MethodID) {
  case OM_NEW: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList;
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    memset(d, 0, sizeof *d);
    d->def = (const CsRibbonDef *)GetTagData(MUIA_CsRibbon_Def, 0, tags);
    d->tab = (int)GetTagData(MUIA_CsRibbon_Tab, 0, tags);
    d->minimised = (int)GetTagData(MUIA_CsRibbon_Minimised, 0, tags);
    if (!d->def || !(d->l = (CsRbLayout *)AllocVec(sizeof(CsRbLayout), MEMF_ANY | MEMF_CLEAR))) { CoerceMethod(cl, obj, OM_DISPOSE); return 0; }
    d->dirty = 1; d->hot = d->press = d->popitem = -1; d->popgroup = -1; d->value = -1;
    CS_EHN(d, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY);
    d->ehn.ehn_Priority = 1;                             /* before the document view: KeyTips take keys first */
    return (ULONG)obj;
  }
  case OM_DISPOSE:
    d = INST_DATA(cl, obj);
    if (d->l) FreeVec(d->l);
    d->l = NULL;
    break;
  case OM_SET: {
    struct TagItem *tags = ((struct opSet *)msg)->ops_AttrList, *t;
    d = INST_DATA(cl, obj);
    if ((t = FindTagItem(MUIA_CsRibbon_Tab, tags)) && (int)t->ti_Data != d->tab) { d->tab = (int)t->ti_Data; d->dirty = 1; if (d->shown) redraw(obj); }
    if ((t = FindTagItem(MUIA_CsRibbon_Minimised, tags)) && (int)t->ti_Data != d->minimised) {
      d->minimised = (int)t->ti_Data; d->dirty = 1;
      if (_win(obj) && d->shown) { Object *p = (Object *)cs_get(obj, MUIA_Parent); if (p && DoMethod(p, MUIM_Group_InitChange)) DoMethod(p, MUIM_Group_ExitChange); }
    }
    if ((t = FindTagItem(MUIA_CsRibbon_PopChoice, tags))) pop_choice(d, obj, (LONG)t->ti_Data);
    if ((t = FindTagItem(MUIA_CsRibbon_Value, tags))) d->value = (LONG)t->ti_Data;
    break;
  }
  case OM_GET: {
    struct opGet *g = (struct opGet *)msg;
    d = INST_DATA(cl, obj);
    if (g->opg_AttrID == MUIA_CsRibbon_Tab) { *g->opg_Storage = (ULONG)d->tab; return TRUE; }
    if (g->opg_AttrID == MUIA_CsRibbon_Value) { *g->opg_Storage = (ULONG)d->value; return TRUE; }
    if (g->opg_AttrID == MUIA_CsRibbon_Minimised) { *g->opg_Storage = (ULONG)d->minimised; return TRUE; }
    break;
  }
  case MUIM_AskMinMax: {
    int h;
    d = INST_DATA(cl, obj);
    h = cs_rb_height(cs_font()->tf_YSize, d->minimised);
    DoSuperMethodA(cl, obj, msg);
    CS_MINMAX(msg, 200, 640, MUI_MAXMAX, h, h, h);
    return 0;
  }
  CS_CLASS_COMMON(RData);
  case MUIM_Draw:
    DoSuperMethodA(cl, obj, msg);
    d = INST_DATA(cl, obj);
    if (((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) draw_all(d, obj);
    return 0;
  case MUIM_CsRibbon_Refresh: {
    int t, k, changed = 0;
    d = INST_DATA(cl, obj);
    if (!d->shown) { d->dirty = 1; return 0; }
    for (t = 0; t < CS_RB_MAXTABS; t++) if (cs_rb_tab_visible(d->def, t) != d->vis[t]) d->dirty = 1;
    if (!d->dirty) for (k = 0; k < d->l->nitem && !changed; k++) if (item_state(d->l->it[k]) != d->st[k]) changed = 1;
    if (d->dirty || changed || d->def->nqa) redraw(obj);
    return 0;
  }
  case MUIM_CsRibbon_KeyTips:
    d = INST_DATA(cl, obj);
    if (d->shown) kt_start(d, obj);
    return 0;
  case MUIM_CsRibbon_Open: {
    ULONG id = ((ULONG *)msg)[1];
    int t, g, i, k;
    d = INST_DATA(cl, obj);
    for (t = -1; t < d->def->ntabs; t++) {               /* the tab shown first, then the others */
      int tt = t < 0 ? d->tab : t;
      if (!cs_rb_tab_visible(d->def, tt)) continue;
      for (g = 0; g < d->def->tabs[tt].n; g++)
        for (i = 0; i < d->def->tabs[tt].groups[g].n; i++) {
          const CsRbItem *it = &d->def->tabs[tt].groups[g].items[i];
          if (it->id != id || it->kind == CSK_LABEL) continue;
          if (tt != d->tab || d->minimised) { d->minimised = 0; SetAttrs(obj, MUIA_CsRibbon_Tab, tt, TAG_DONE); }
          relayout(d, obj);
          redraw(obj);
          for (k = 0; k < d->l->nitem; k++) if (d->l->it[k] == it) { press(d, obj, CS_HIT_ITEM, k); return 1; }
          if (it->kind != CSK_DROP) send(obj, d, id, -1);   /* (in a collapsed group) */
          return 1;
        }
    }
    send(obj, d, id, -1);                                /* on no tab: just run it */
    return 0;
  }
  case MUIM_CreateShortHelp: {
    struct MUIP_CreateShortHelp *m = (struct MUIP_CreateShortHelp *)msg;
    const CsCommand *c = NULL;
    const char *nm = NULL;
    char name[80];
    int k, kind;
    d = INST_DATA(cl, obj);
    kind = hit(d, obj, m->mx, m->my, &k);
    if (kind == CS_HIT_ITEM || kind == CS_HIT_ARROW) { c = cs_cmd_find(d->l->it[k]->id); if (d->l->it[k]->label) nm = tr(d->l->it[k]->label); }
    else if (kind == CS_HIT_QA) c = cs_cmd_find(d->def->qa[k]);
    else if (kind == CS_HIT_FILE) c = cs_cmd_find(d->def->file);
    else if (kind == CS_HIT_TELL) c = cs_cmd_find(d->def->tellme);
    else if (kind == CS_HIT_GROUP) return (ULONG)cs_bubble(tr(d->def->tabs[d->tab].groups[d->l->igroup[k]].name), tr("This group's tools, as a list: the window is too narrow to show them."));
    if (!c) return 0;
    if (!nm) nm = cs_cmd_name(c->id, name, sizeof name); else cs_strlcpy(name, nm, sizeof name);
    if (c->key && c->key[0]) { size_t n = strlen(name); snprintf(name + n, sizeof name - n, " (Amiga-%s)", c->key); }
    return (ULONG)cs_bubble(name, c->help ? tr(c->help) : "");
  }
  case MUIM_DeleteShortHelp: return 0;
  case MUIM_HandleEvent: {
    struct IntuiMessage *im = ((struct MUIP_HandleEvent *)msg)->imsg;
    int k, kind;
    d = INST_DATA(cl, obj);
    if (!im || !d->shown) return 0;
    if (im->Class == IDCMP_RAWKEY) {
      UWORD code = im->Code, last = d->lastkey;
      d->lastkey = code;
      /* Alt pressed and let go alone, or F10: the KeyTips */
      if ((code == (0x64 | IECODE_UP_PREFIX) && last == 0x64) || (code == (0x65 | IECODE_UP_PREFIX) && last == 0x65) || code == 0x59) {
        if (d->kt) kt_stop(d, obj); else kt_start(d, obj);
        return MUI_EventHandlerRC_Eat;
      }
      if (!d->kt || (code & IECODE_UP_PREFIX) || code == 0x64 || code == 0x65 || code == 0x60 || code == 0x61) return 0;
      {
        struct InputEvent ie;
        UBYTE buf[4];
        LONG n;
        ie.ie_NextEvent = NULL; ie.ie_Class = IECLASS_RAWKEY; ie.ie_SubClass = 0;
        ie.ie_Code = code; ie.ie_Qualifier = im->Qualifier & ~(IEQUALIFIER_LALT | IEQUALIFIER_RALT);
        ie.ie_EventAddress = (APTR *)*((ULONG *)im->IAddress);
        n = code == 0x45 ? (buf[0] = 27, 1) : MapRawKey(&ie, (STRPTR)buf, sizeof buf, NULL);
        if (n == 1) kt_key(d, obj, buf[0]); else kt_stop(d, obj);
      }
      return MUI_EventHandlerRC_Eat;
    }
    kind = hit(d, obj, im->MouseX, im->MouseY, &k);
    if (im->Class == IDCMP_MOUSEMOVE) {
      int hk = kind == CS_HIT_ARROW ? CS_HIT_ITEM : kind;
      if (hk != d->hotkind || k != d->hot) { d->hotkind = hk; d->hot = k; redraw(obj); }
      return 0;
    }
    if (im->Class != IDCMP_MOUSEBUTTONS) return 0;
    if (im->Code == SELECTDOWN) {
      if (kind == CS_HIT_NONE) return 0;
      kt_stop(d, obj);
      if (kind == CS_HIT_TAB) {                          /* a tab acts at once; a double click minimises */
        static ULONG ls, lm;
        static int lt = -1;
        if (k == lt && DoubleClick(ls, lm, im->Seconds, im->Micros)) { lt = -1; SetAttrs(obj, MUIA_CsRibbon_Minimised, !d->minimised, TAG_DONE); return MUI_EventHandlerRC_Eat; }
        lt = k; ls = im->Seconds; lm = im->Micros;
        press(d, obj, kind, k);
        return MUI_EventHandlerRC_Eat;
      }
      d->presskind = kind == CS_HIT_ARROW ? CS_HIT_ITEM : kind; d->press = k;
      redraw(obj);
      if (kind == CS_HIT_GROUP || kind == CS_HIT_ARROW || (kind == CS_HIT_ITEM && d->l->it[k]->kind == CSK_DROP)) {
        d->presskind = 0; d->press = -1;
        press(d, obj, kind, k);                          /* lists open on the press */
      }
      return MUI_EventHandlerRC_Eat;
    }
    if (im->Code == SELECTUP && d->presskind) {
      int pk = d->presskind, pi = d->press, same = (kind == CS_HIT_ARROW ? CS_HIT_ITEM : kind) == pk && k == pi;
      d->presskind = 0; d->press = -1;
      redraw(obj);
      if (same) press(d, obj, kind, k);
      return MUI_EventHandlerRC_Eat;
    }
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
struct MUI_CustomClass *cs_ribbon_class(void)
{
  if (!rcls) rcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(RData), (APTR)rdispatch);
  return rcls;
}
void cs_ribbon_free(void) { if (rcls) { MUI_DeleteCustomClass(rcls); rcls = NULL; } }

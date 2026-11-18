/* cs_ribbon.c - see cs_ribbon.h: the ribbon's layout, its hit testing and its KeyTips */
#include <string.h>
#include "cs_ribbon.h"
#include "cs_cmd.h"
#include "cs_lang.h"
#include "cs_util.h"

int cs_rb_tabh(int fh) { return fh + 10; }
int cs_rb_rowh(int fh) { return fh + 7; }
static int bodyh(int fh) { return 3 * cs_rb_rowh(fh) + 4; }
static int caph(int fh) { return fh + 3; }
int cs_rb_height(int fh, int tabs_only) { return tabs_only ? cs_rb_tabh(fh) + 2 : cs_rb_tabh(fh) + 2 + bodyh(fh) + caph(fh) + 2; }

int cs_rb_tab_visible(const CsRibbonDef *def, int t)
{
  if (t < 0 || t >= def->ntabs) return 0;
  return !def->tabs[t].context || (cs_cmd_state(def->tabs[t].context) & CSS_CHECKED);
}
char *cs_rb_item_name(const CsRbItem *it, char *out, int cap)
{
  if (it->label) cs_strlcpy(out, tr(it->label), (size_t)cap);
  else cs_cmd_name(it->id, out, cap);
  return out;
}
int cs_rb_item_icon(const CsRbItem *it)
{
  const CsCommand *c;
  if (it->icon) return it->icon;
  c = cs_cmd_find(it->id);
  return c ? c->icon : 0;
}
static long tw(const CsRbMetrics *m, const char *s) { return m->textw(m->ctx, s); }
/* a two-line label's break: the space that makes the wider line narrowest (0: one line), *w that width */
static int split_at(const CsRbMetrics *m, const char *s, long *w)
{
  char a[96];
  int n = (int)strlen(s), best = 0, i;
  long bw = tw(m, s);
  if (n >= (int)sizeof a) n = (int)sizeof a - 1;
  for (i = 1; i < n - 1; i++)
    if (s[i] == ' ') {
      long wa, wb;
      memcpy(a, s, (size_t)i); a[i] = 0;
      wa = tw(m, a); wb = tw(m, s + i + 1);
      if ((wa > wb ? wa : wb) < bw) { bw = wa > wb ? wa : wb; best = i; }
    }
  *w = bw;
  return best;
}
/* an item's width; its height comes from its size */
static long item_w(const CsRbMetrics *m, const CsRbItem *it, int squeeze)
{
  char name[96];
  long lw = 0, arrow = (it->kind == CSK_DROP || it->kind == CSK_SPLIT) ? 9 : 0;
  int icon = cs_rb_item_icon(it);
  cs_rb_item_name(it, name, sizeof name);
  if (name[0]) lw = tw(m, name);
  if (it->kind == CSK_LABEL) return lw + 6;
  if (it->size == CSZ_LARGE) { if (squeeze) split_at(m, name, &lw); return (lw > 30 ? lw : 30) + 10; }
  if (it->size == CSZ_ICON || (squeeze && icon && it->kind != CSK_CHECK)) return 22 + arrow;
  return 16 + 6 + lw + 8 + arrow;
}
/* a group's natural width: small items three to a column, a large one a column of its own; at
   least its caption's width unless squeezed */
static long group_w(const CsRbMetrics *m, const CsRbGroup *g, int squeeze)
{
  long w = 0, colw = 0, cap = tw(m, tr(g->name)) + 12;
  int row = 0, i;
  for (i = 0; i < g->n; i++) {
    const CsRbItem *it = &g->items[i];
    long iw = item_w(m, it, squeeze);
    if (it->size == CSZ_LARGE) { if (colw) { w += colw + 2; colw = 0; row = 0; } w += iw + 2; continue; }
    if (row == 3 || (it->flags & CSF_NEWCOL)) { w += colw + 2; colw = 0; row = 0; }
    if (iw > colw) colw = iw;
    row++;
  }
  w += colw + 8;
  if (w < cap && !squeeze) w = cap;
  return w;
}
/* a collapsed group's button: its name (on two lines when squeezed), at least 40 */
static long group_button_w(const CsRbMetrics *m, const CsRbGroup *g, int squeeze)
{
  long w;
  if (squeeze) split_at(m, tr(g->name), &w); else w = tw(m, tr(g->name));
  w += 12;
  return w < 40 ? 40 : w;
}
static void setbox(CsBox *b, long x0, long y0, long x1, long y1) { b->x0 = (short)x0; b->y0 = (short)y0; b->x1 = (short)x1; b->y1 = (short)y1; }
static void hide(CsBox *b) { b->x0 = b->x1 = -1; b->y0 = b->y1 = -1; }
static void mirror(CsBox *b, int width)
{
  short t;
  if (b->x0 < 0) return;
  t = (short)(width - 1 - b->x1); b->x1 = (short)(width - 1 - b->x0); b->x0 = t;
}

void cs_rb_layout(const CsRibbonDef *def, int tab, int width, int rtl, int tabs_only, const CsRbMetrics *m, CsRbLayout *l)
{
  int fh = m->fh, t, g, i, n = 0, ntabs = def->ntabs < CS_RB_MAXTABS ? def->ntabs : CS_RB_MAXTABS;
  long x, right, y0 = cs_rb_tabh(fh) + 2, gw[CS_RB_MAXGROUPS], total, avail = width - 4;
  const CsRbTab *tp;
  memset(l, 0, sizeof *l);
  l->width = width; l->rtl = rtl; l->tabs_only = tabs_only;
  l->height = cs_rb_height(fh, tabs_only);
  /* the strip's right end: Quick Access icons (22 each), the Tell Me box (a magnifier, and its words
     when there is room), from the right */
  {
    long x1 = width - 3, need = 0;
    int nq = def->nqa < CS_RB_MAXQA ? def->nqa : CS_RB_MAXQA;
    for (t = 0; t < ntabs; t++) if (cs_rb_tab_visible(def, t)) { long w = tw(m, tr(def->tabs[t].name)); need += (w < 36 ? w : 36) + 6; }
    if (def->file) need += tw(m, tr("File")) + 6;
    hide(&l->tell);
    for (i = 0; i < CS_RB_MAXQA; i++) hide(&l->qa[i]);
    /* as many Quick Access buttons as leave the tabs their least (36 pixels a name), from the first */
    {
      int shown = nq;
      while (shown > 0 && need + 23L * shown + (def->tellme ? 26 : 0) + 4 > width) shown--;
      x1 = width - 3;
      if (def->tellme) {
        long ww = tw(m, tr("Tell me")), w = 22;
        if (x1 - need - 23L * shown > 22 + ww + 8 + 60) w = 22 + ww + 8;
        setbox(&l->tell, x1 - w + 1, 3, x1, cs_rb_tabh(fh) - 3);
        x1 -= w + 4;
      }
      for (i = shown - 1; i >= 0; i--) { setbox(&l->qa[i], x1 - 21, 1, x1, cs_rb_tabh(fh) - 1); x1 -= 23; }
      right = x1;
    }
  }
  /* the tabs: 16 pixels round each name, down to 6, then the longest cut to a common width */
  {
    long sum = 0, pad = 16, cap = 0, lo, hi, room = right - 2;
    int nv = 0;
    long fw = def->file ? tw(m, tr("File")) : 0;
    for (t = 0; t < ntabs; t++) if (cs_rb_tab_visible(def, t)) { sum += tw(m, tr(def->tabs[t].name)); nv++; }
    if (def->file) { sum += fw; nv++; }
    if (nv && sum + nv * pad > room) {
      pad = (room - sum) / nv;
      if (pad < 6) {
        pad = 6;
        lo = 8; hi = 400;
        while (lo < hi) {
          long mid = (lo + hi + 1) / 2, tot = 0;
          for (t = 0; t < ntabs; t++) if (cs_rb_tab_visible(def, t)) { long w = tw(m, tr(def->tabs[t].name)); tot += (w < mid ? w : mid) + 6; }
          if (def->file) tot += (fw < mid ? fw : mid) + 6;
          if (tot <= room) lo = mid; else hi = mid - 1;
        }
        cap = lo;
      }
    }
    l->tabcap = (int)cap;
    x = 2;
    hide(&l->file);
    if (def->file) {
      long w = (cap && fw > cap ? cap : fw) + pad;
      setbox(&l->file, x, 0, x + w - 1, cs_rb_tabh(fh) - 1);
      x += w;
    }
    for (t = 0; t < CS_RB_MAXTABS; t++) hide(&l->tabbox[t]);
    for (t = 0; t < ntabs; t++) {
      long w;
      if (!cs_rb_tab_visible(def, t)) continue;
      w = tw(m, tr(def->tabs[t].name));
      if (cap && w > cap) w = cap;
      w += pad;
      setbox(&l->tabbox[t], x, 0, x + w - 1, cs_rb_tabh(fh) - 1);
      x += w;
    }
  }
  if (!cs_rb_tab_visible(def, tab)) for (tab = 0; tab < ntabs && !cs_rb_tab_visible(def, tab); tab++) ;
  if (tab >= ntabs) tab = 0;
  l->tab = tab;
  if (tabs_only || !ntabs) goto done;
  tp = &def->tabs[tab];
  /* the groups: natural widths; too wide, squeezed; still too wide, collapsed from the right */
  l->ngroup = tp->n < CS_RB_MAXGROUPS ? tp->n : CS_RB_MAXGROUPS;
  total = 0;
  for (g = 0; g < l->ngroup; g++) total += gw[g] = group_w(m, &tp->groups[g], 0);
  if (total > avail) {
    l->squeeze = 1;
    total = 0;
    for (g = 0; g < l->ngroup; g++) total += gw[g] = group_w(m, &tp->groups[g], 1);
  }
  for (g = l->ngroup - 1; g >= 0 && total > avail; g--) {
    long cw = group_button_w(m, &tp->groups[g], l->squeeze) + 8;
    if (cw >= gw[g]) continue;
    total += cw - gw[g]; gw[g] = cw; l->collapsed[g] = 1;
  }
  if (total > avail) {
    /* even so: the widest narrowed to one width, as buttons with their names cut short */
    long lo = 30, hi = 400;
    while (lo < hi) {
      long mid = (lo + hi + 1) / 2, tot = 0;
      for (g = 0; g < l->ngroup; g++) tot += gw[g] < mid ? gw[g] : mid;
      if (tot <= avail) lo = mid; else hi = mid - 1;
    }
    total = 0;
    for (g = 0; g < l->ngroup; g++) { if (gw[g] > lo) { gw[g] = lo; l->collapsed[g] = 1; } total += gw[g]; }
  }
  l->overflow = total > avail ? (int)(total - avail) : 0;
  /* squeezed with room to spare: groups narrower than their caption widened, from the left */
  if (l->squeeze) {
    long slack = avail - total;
    for (g = 0; g < l->ngroup && slack > 0; g++) {
      long cap = tw(m, tr(tp->groups[g].name)) + 12, add;
      if (l->collapsed[g] || gw[g] >= cap) continue;
      add = cap - gw[g] < slack ? cap - gw[g] : slack;
      gw[g] += add; slack -= add;
    }
  }
  /* the boxes */
  x = 2;
  for (g = 0; g < l->ngroup; g++) {
    const CsRbGroup *gr = &tp->groups[g];
    long w = gw[g], top = y0 + 2, rowh = cs_rb_rowh(fh);
    if (l->collapsed[g]) {
      if (n < CS_RB_MAXITEMS) {
        setbox(&l->item[n], x + 3, top, x + w - 6, top + 3 * rowh - 1);
        l->it[n] = NULL; l->igroup[n] = g; n++;
      }
    } else {
      long cx = x + 4, colw = 0;
      int row = 0, colstart = n, k;
      for (i = 0; i < gr->n && n < CS_RB_MAXITEMS; i++) {
        const CsRbItem *it = &gr->items[i];
        long iw = item_w(m, it, l->squeeze);
        if (it->size == CSZ_LARGE) {
          if (colw) { for (k = colstart; k < n; k++) l->item[k].x1 = (short)(l->item[k].x0 + colw - 1); cx += colw + 2; colw = 0; row = 0; }
          setbox(&l->item[n], cx, top, cx + iw - 1, top + 3 * rowh - 1);
          l->it[n] = it; l->igroup[n] = g; n++;
          cx += iw + 2; colstart = n;
          continue;
        }
        if (row == 3 || (it->flags & CSF_NEWCOL)) {
          for (k = colstart; k < n; k++) l->item[k].x1 = (short)(l->item[k].x0 + colw - 1);
          cx += colw + 2; colw = 0; row = 0; colstart = n;
        }
        setbox(&l->item[n], cx, top + row * rowh, cx + iw - 1, top + (row + 1) * rowh - 1);
        l->it[n] = it; l->igroup[n] = g; n++;
        if (iw > colw) colw = iw;
        row++;
      }
      for (k = colstart; k < n; k++) if (l->item[k].x1 < l->item[k].x0 + colw - 1) l->item[k].x1 = (short)(l->item[k].x0 + colw - 1);
    }
    setbox(&l->group[g], x, y0, x + w - 1, y0 + bodyh(fh) + caph(fh));
    x += w;
  }
  l->nitem = n;
done:
  if (rtl) {
    mirror(&l->file, width); mirror(&l->tell, width);
    for (t = 0; t < CS_RB_MAXTABS; t++) mirror(&l->tabbox[t], width);
    for (i = 0; i < CS_RB_MAXQA; i++) mirror(&l->qa[i], width);
    for (g = 0; g < l->ngroup; g++) mirror(&l->group[g], width);
    for (i = 0; i < l->nitem; i++) mirror(&l->item[i], width);
  }
}
static int in(const CsBox *b, int x, int y) { return b->x0 >= 0 && x >= b->x0 && x <= b->x1 && y >= b->y0 && y <= b->y1; }
int cs_rb_hit(const CsRbLayout *l, int x, int y, int *index)
{
  int i;
  *index = -1;
  if (in(&l->file, x, y)) return CS_HIT_FILE;
  for (i = 0; i < CS_RB_MAXTABS; i++) if (in(&l->tabbox[i], x, y)) { *index = i; return CS_HIT_TAB; }
  for (i = 0; i < CS_RB_MAXQA; i++) if (in(&l->qa[i], x, y)) { *index = i; return CS_HIT_QA; }
  if (in(&l->tell, x, y)) return CS_HIT_TELL;
  for (i = 0; i < l->nitem; i++)
    if (in(&l->item[i], x, y)) {
      const CsRbItem *it = l->it[i];
      *index = i;
      if (!it) return CS_HIT_GROUP;
      if (it->kind == CSK_LABEL) return CS_HIT_NONE;
      if (it->kind == CSK_SPLIT) {
        /* the arrow: the right 10 pixels of a small one (left in right-to-left), the bottom third of a large one */
        if (it->size == CSZ_LARGE ? y > l->item[i].y1 - (l->item[i].y1 - l->item[i].y0) / 3
                                  : (l->rtl ? x < l->item[i].x0 + 10 : x > l->item[i].x1 - 10)) return CS_HIT_ARROW;
      }
      return CS_HIT_ITEM;
    }
  return CS_HIT_NONE;
}
void cs_rb_keytips_tabs(const CsRibbonDef *def, const CsRbLayout *l, char (*codes)[3])
{
  const char *names[1 + CS_RB_MAXTABS + CS_RB_MAXQA + 1];
  int t, i, n = 1 + def->ntabs, nq = def->nqa < CS_RB_MAXQA ? def->nqa : CS_RB_MAXQA, q = 0;
  names[0] = def->file && l->file.x0 >= 0 ? tr("File") : NULL;
  for (t = 0; t < def->ntabs && t < CS_RB_MAXTABS; t++) names[1 + t] = l->tabbox[t].x0 >= 0 ? tr(def->tabs[t].name) : NULL;
  cs_keytips_assign(names, n, codes);
  /* the Quick Access buttons are numbered; the Tell Me box is Q if no tab has Q */
  for (i = 0; i < nq; i++) {
    codes[n + i][0] = 0;
    if (l->qa[i].x0 >= 0 && i < 9) { codes[n + i][0] = (char)('1' + i); codes[n + i][1] = 0; }
  }
  codes[n + nq][0] = 0;
  for (t = 0; t < n; t++) if (codes[t][0] == 'Q') q = 1;
  if (def->tellme && l->tell.x0 >= 0 && !q) { codes[n + nq][0] = 'Q'; codes[n + nq][1] = 0; }
}
void cs_rb_keytips_items(const CsRibbonDef *def, const CsRbLayout *l, char (*codes)[3])
{
  static char nm[CS_RB_MAXITEMS][48];
  const char *names[CS_RB_MAXITEMS];
  int k;
  for (k = 0; k < l->nitem; k++) {
    const CsRbItem *it = l->it[k];
    if (!it) cs_strlcpy(nm[k], tr(def->tabs[l->tab].groups[l->igroup[k]].name), sizeof nm[k]);
    else if (it->kind == CSK_LABEL) nm[k][0] = 0;
    else cs_rb_item_name(it, nm[k], sizeof nm[k]);
    names[k] = nm[k][0] ? nm[k] : NULL;
  }
  cs_keytips_assign(names, l->nitem, codes);
}

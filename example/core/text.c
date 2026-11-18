/* text.c - see text.h: Jotter's document */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "text.h"

/* ---------------------------------------------------------------- lines */
static int line_room(TLine *l, long need)
{
  char *n;
  long cap;
  if (need + 1 <= l->cap) return 1;
  cap = l->cap ? l->cap : 16;
  while (cap < need + 1) cap *= 2;
  if (!(n = (char *)realloc(l->s, (size_t)cap))) return 0;
  l->s = n; l->cap = cap;
  return 1;
}
static int lines_room(TextDoc *t, long need)
{
  TLine *n;
  long cap;
  if (need <= t->caplines) return 1;
  cap = t->caplines ? t->caplines : 64;
  while (cap < need) cap *= 2;
  if (!(n = (TLine *)realloc(t->lines, (size_t)cap * sizeof *n))) return 0;
  memset(n + t->caplines, 0, (size_t)(cap - t->caplines) * sizeof *n);
  t->lines = n; t->caplines = cap;
  return 1;
}
static void undo_clear(TextDoc *t)
{
  int i;
  for (i = 0; i < t->nundo; i++) free(t->undo[i].text);
  t->nundo = t->ndone = 0;
  t->typing = 0;
}
int text_init(TextDoc *t)
{
  memset(t, 0, sizeof *t);
  if (!lines_room(t, 1) || !line_room(&t->lines[0], 0)) return 0;
  t->nlines = 1;
  t->lines[0].s[0] = 0;
  return 1;
}
void text_free(TextDoc *t)
{
  long i;
  for (i = 0; i < t->caplines; i++) free(t->lines[i].s);
  free(t->lines);
  undo_clear(t);
  free(t->undo);
  memset(t, 0, sizeof *t);
}
char *text_get(const TextDoc *t, long *len)
{
  long n = 0, i;
  char *s, *p;
  for (i = 0; i < t->nlines; i++) n += t->lines[i].len + (i + 1 < t->nlines);
  if (!(s = (char *)malloc((size_t)n + 1))) return NULL;
  for (p = s, i = 0; i < t->nlines; i++) {
    memcpy(p, t->lines[i].s, (size_t)t->lines[i].len); p += t->lines[i].len;
    if (i + 1 < t->nlines) *p++ = '\n';
  }
  *p = 0;
  if (len) *len = n;
  return s;
}
TPos text_clamp(const TextDoc *t, TPos p)
{
  if (p.line < 0) { p.line = 0; p.col = 0; }
  if (p.line >= t->nlines) { p.line = t->nlines - 1; p.col = t->lines[p.line].len; }
  if (p.col < 0) p.col = 0;
  if (p.col > t->lines[p.line].len) p.col = t->lines[p.line].len;
  return p;
}
int text_cmp(TPos a, TPos b) { return a.line != b.line ? (a.line < b.line ? -1 : 1) : a.col != b.col ? (a.col < b.col ? -1 : 1) : 0; }
int text_has_sel(const TextDoc *t) { return text_cmp(t->caret, t->anchor) != 0; }
void text_sel_range(const TextDoc *t, TPos *a, TPos *b)
{
  if (text_cmp(t->anchor, t->caret) <= 0) { *a = t->anchor; *b = t->caret; } else { *a = t->caret; *b = t->anchor; }
}
long text_offset(const TextDoc *t, TPos p)
{
  long o = 0, i;
  p = text_clamp(t, p);
  for (i = 0; i < p.line; i++) o += t->lines[i].len + 1;
  return o + p.col;
}
TPos text_pos_at(const TextDoc *t, long off)
{
  TPos p = { 0, 0 };
  while (p.line < t->nlines - 1 && off > t->lines[p.line].len) { off -= t->lines[p.line].len + 1; p.line++; }
  p.col = off;
  return text_clamp(t, p);
}
/* the text between a and b (a <= b), malloc'd */
static char *range_get(const TextDoc *t, TPos a, TPos b, long *len)
{
  long n = text_offset(t, b) - text_offset(t, a), k = 0, i;
  char *s = (char *)malloc((size_t)n + 1);
  if (!s) return NULL;
  for (i = a.line; i <= b.line; i++) {
    long c0 = i == a.line ? a.col : 0, c1 = i == b.line ? b.col : t->lines[i].len;
    memcpy(s + k, t->lines[i].s + c0, (size_t)(c1 - c0)); k += c1 - c0;
    if (i < b.line) s[k++] = '\n';
  }
  s[k] = 0;
  if (len) *len = k;
  return s;
}
char *text_sel_get(const TextDoc *t, long *len)
{
  TPos a, b;
  text_sel_range(t, &a, &b);
  return range_get(t, a, b, len);
}
/* s (len bytes, "\n" splitting lines) put in at p; the position after it, or p.line -1 without memory */
static TPos raw_insert(TextDoc *t, TPos p, const char *s, long len)
{
  long nl = 0, i, k, tail;
  TPos end;
  TLine *l;
  for (i = 0; i < len; i++) nl += s[i] == '\n';
  p = text_clamp(t, p);
  if (!lines_room(t, t->nlines + nl)) { end.line = -1; end.col = 0; return end; }
  l = &t->lines[p.line];
  tail = l->len - p.col;
  if (!nl) {
    if (!line_room(l, l->len + len)) { end.line = -1; end.col = 0; return end; }
    memmove(l->s + p.col + len, l->s + p.col, (size_t)tail + 1);
    memcpy(l->s + p.col, s, (size_t)len);
    l->len += len;
    end.line = p.line; end.col = p.col + len;
    return end;
  }
  {
    /* the new lines made first (so nothing changes without memory for them) */
    TLine *nw = (TLine *)calloc((size_t)nl, sizeof *nw);
    long start = 0, seg = 0;
    if (!nw) { end.line = -1; end.col = 0; return end; }
    for (i = 0, k = 0; i <= len; i++) {
      if (i < len && s[i] != '\n') continue;
      if (k > 0) {
        long n = i - start, extra = k == nl ? tail : 0;
        if (!line_room(&nw[k - 1], n + extra)) { for (k = 0; k < nl; k++) free(nw[k].s); free(nw); end.line = -1; end.col = 0; return end; }
        memcpy(nw[k - 1].s, s + start, (size_t)n);
        if (extra) memcpy(nw[k - 1].s + n, l->s + p.col, (size_t)tail);
        nw[k - 1].len = n + extra; nw[k - 1].s[n + extra] = 0;
      } else seg = i;
      k++; start = i + 1;
    }
    if (!line_room(l, p.col + seg)) { for (k = 0; k < nl; k++) free(nw[k].s); free(nw); end.line = -1; end.col = 0; return end; }
    l = &t->lines[p.line];
    memcpy(l->s + p.col, s, (size_t)seg);
    l->len = p.col + seg; l->s[l->len] = 0;
    /* the lines after moved down; the spare slots at the end hold nothing worth keeping */
    for (k = t->nlines + nl - 1; k >= t->nlines; k--) free(t->lines[k].s);
    memmove(&t->lines[p.line + 1 + nl], &t->lines[p.line + 1], (size_t)(t->nlines - p.line - 1) * sizeof(TLine));
    memcpy(&t->lines[p.line + 1], nw, (size_t)nl * sizeof(TLine));
    free(nw);
    t->nlines += nl;
    end.line = p.line + nl; end.col = t->lines[end.line].len - tail;
    return end;
  }
}
/* the text a..b (a <= b) taken out */
static void raw_delete(TextDoc *t, TPos a, TPos b)
{
  TLine *la = &t->lines[a.line], *lb = &t->lines[b.line];
  long i, tail = lb->len - b.col;
  if (a.line == b.line) {
    memmove(la->s + a.col, la->s + b.col, (size_t)tail + 1);
    la->len -= b.col - a.col;
    return;
  }
  if (!line_room(la, a.col + tail)) return;          /* (it only shrinks or stays: never fails in practice) */
  lb = &t->lines[b.line];
  memcpy(la->s + a.col, lb->s + b.col, (size_t)tail + 1);
  la->len = a.col + tail;
  for (i = a.line + 1; i <= b.line; i++) free(t->lines[i].s);
  memmove(&t->lines[a.line + 1], &t->lines[b.line + 1], (size_t)(t->nlines - b.line - 1) * sizeof(TLine));
  t->nlines -= b.line - a.line;
  memset(&t->lines[t->nlines], 0, (size_t)(b.line - a.line) * sizeof(TLine));
}
/* a step recorded (what was redoable is forgotten); 0 without memory */
static int record(TextDoc *t, int insert, TPos at, const char *s, long len, TPos before, int chain, int typing)
{
  TUndo *u;
  int i;
  for (i = t->ndone; i < t->nundo; i++) free(t->undo[i].text);
  t->nundo = t->ndone;
  if (t->nundo >= t->capundo) {
    int cap = t->capundo ? t->capundo * 2 : 64;
    TUndo *n = (TUndo *)realloc(t->undo, (size_t)cap * sizeof *n);
    if (!n) return 0;
    t->undo = n; t->capundo = cap;
  }
  u = &t->undo[t->nundo];
  if (!(u->text = (char *)malloc((size_t)len + 1))) return 0;
  memcpy(u->text, s, (size_t)len); u->text[len] = 0;
  u->len = len; u->insert = insert; u->at = at; u->caret_before = before; u->chain = chain; u->typing = typing;
  t->ndone = ++t->nundo;
  t->modified = 1;
  return 1;
}
static int del_range(TextDoc *t, TPos a, TPos b, int chain)
{
  long n;
  char *s = range_get(t, a, b, &n);
  TPos before = t->caret;
  if (!s) return 0;
  if (!record(t, 0, a, s, n, before, chain, 0)) { free(s); return 0; }
  free(s);
  raw_delete(t, a, b);
  t->caret = t->anchor = a;
  t->typing = 0;
  return 1;
}
int text_delete_sel(TextDoc *t)
{
  TPos a, b;
  if (!text_has_sel(t)) return 0;
  text_sel_range(t, &a, &b);
  return del_range(t, a, b, 0);
}
int text_insert(TextDoc *t, const char *s, long len)
{
  TPos before, end;
  int chain = 0, typing = len == 1 && s[0] != '\n' && s[0] != ' ';
  if (text_has_sel(t)) { if (!text_delete_sel(t)) return 0; chain = 1; typing = 0; }
  before = t->caret;
  if (!len) return chain;
  /* typing a letter right after the last one typed: the same undo step */
  if (typing && t->typing && t->ndone == t->nundo && t->ndone > 0) {
    TUndo *u = &t->undo[t->ndone - 1];
    TPos uend = u->at;
    uend.col += u->len;
    if (u->insert && u->typing && !strchr(u->text, '\n') && !text_cmp(uend, before)) {
      char *n = (char *)realloc(u->text, (size_t)u->len + 2);
      if (n) {
        end = raw_insert(t, before, s, 1);
        if (end.line < 0) return 0;
        u->text = n; u->text[u->len++] = s[0]; u->text[u->len] = 0;
        t->caret = t->anchor = end;
        t->modified = 1;
        return 1;
      }
    }
  }
  end = raw_insert(t, before, s, len);
  if (end.line < 0) return 0;
  if (!record(t, 1, before, s, len, before, chain, typing)) { raw_delete(t, before, end); return 0; }
  t->caret = t->anchor = end;
  t->typing = typing;
  return 1;
}
int text_backspace(TextDoc *t)
{
  TPos a = t->caret;
  if (text_has_sel(t)) return text_delete_sel(t);
  if (a.col > 0) a.col--;
  else if (a.line > 0) { a.line--; a.col = t->lines[a.line].len; }
  else return 0;
  return del_range(t, a, t->caret, 0);
}
int text_delete(TextDoc *t)
{
  TPos b = t->caret;
  if (text_has_sel(t)) return text_delete_sel(t);
  if (b.col < t->lines[b.line].len) b.col++;
  else if (b.line + 1 < t->nlines) { b.line++; b.col = 0; }
  else return 0;
  return del_range(t, t->caret, b, 0);
}
int text_can_undo(const TextDoc *t) { return t->ndone > 0; }
int text_can_redo(const TextDoc *t) { return t->ndone < t->nundo; }
void text_break_join(TextDoc *t) { t->typing = 0; }
static TPos end_of(TPos at, const char *s, long len)
{
  long i;
  for (i = 0; i < len; i++) if (s[i] == '\n') { at.line++; at.col = 0; } else at.col++;
  return at;
}
int text_undo(TextDoc *t)
{
  int chained;
  if (!t->ndone) return 0;
  do {
    TUndo *u = &t->undo[--t->ndone];
    if (u->insert) raw_delete(t, u->at, end_of(u->at, u->text, u->len));
    else raw_insert(t, u->at, u->text, u->len);
    t->caret = t->anchor = u->caret_before;
    chained = u->chain;
  } while (chained && t->ndone > 0);
  t->caret = t->anchor = text_clamp(t, t->caret);
  t->typing = 0; t->modified = 1;
  return 1;
}
int text_redo(TextDoc *t)
{
  if (t->ndone >= t->nundo) return 0;
  do {
    TUndo *u = &t->undo[t->ndone++];
    if (u->insert) t->caret = raw_insert(t, u->at, u->text, u->len);
    else { raw_delete(t, u->at, end_of(u->at, u->text, u->len)); t->caret = u->at; }
    t->anchor = t->caret;
  } while (t->ndone < t->nundo && t->undo[t->ndone].chain);
  t->typing = 0; t->modified = 1;
  return 1;
}
int text_set(TextDoc *t, const char *s, long len)
{
  TextDoc n;
  TPos end;
  if (!text_init(&n)) return 0;
  end = raw_insert(&n, n.caret, s, len);
  if (end.line < 0) { text_free(&n); return 0; }
  memcpy(n.path, t->path, sizeof n.path);
  n.crlf = t->crlf;
  text_free(t);
  *t = n;
  return 1;
}

/* ---------------------------------------------------------------- moving */
static int is_word(int c) { c &= 255; return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || (c >= 0xC0 && c != 0xD7 && c != 0xF7); }
void text_set_caret(TextDoc *t, TPos p, int extend)
{
  t->caret = text_clamp(t, p);
  if (!extend) t->anchor = t->caret;
  t->typing = 0;
}
void text_move(TextDoc *t, int how, int extend)
{
  TPos p = t->caret;
  TLine *l = &t->lines[p.line];
  if (!extend && text_has_sel(t) && (how == MV_LEFT || how == MV_RIGHT)) {
    TPos a, b;
    text_sel_range(t, &a, &b);
    text_set_caret(t, how == MV_LEFT ? a : b, 0);
    return;
  }
  switch (how) {
  case MV_LEFT: if (p.col > 0) p.col--; else if (p.line > 0) { p.line--; p.col = t->lines[p.line].len; } break;
  case MV_RIGHT: if (p.col < l->len) p.col++; else if (p.line + 1 < t->nlines) { p.line++; p.col = 0; } break;
  case MV_UP: if (p.line > 0) p.line--; else p.col = 0; break;
  case MV_DOWN: if (p.line + 1 < t->nlines) p.line++; else p.col = l->len; break;
  case MV_HOME: p.col = 0; break;
  case MV_END: p.col = l->len; break;
  case MV_TOP: p.line = 0; p.col = 0; break;
  case MV_BOTTOM: p.line = t->nlines - 1; p.col = t->lines[p.line].len; break;
  case MV_WORDLEFT:
    if (p.col == 0) { if (p.line > 0) { p.line--; p.col = t->lines[p.line].len; } break; }
    while (p.col > 0 && !is_word(l->s[p.col - 1])) p.col--;
    while (p.col > 0 && is_word(l->s[p.col - 1])) p.col--;
    break;
  case MV_WORDRIGHT:
    if (p.col == l->len) { if (p.line + 1 < t->nlines) { p.line++; p.col = 0; } break; }
    while (p.col < l->len && is_word(l->s[p.col])) p.col++;
    while (p.col < l->len && !is_word(l->s[p.col])) p.col++;
    break;
  }
  text_set_caret(t, p, extend);
}
void text_select_all(TextDoc *t)
{
  t->anchor.line = 0; t->anchor.col = 0;
  t->caret.line = t->nlines - 1; t->caret.col = t->lines[t->caret.line].len;
  t->typing = 0;
}
void text_select_word(TextDoc *t, TPos p)
{
  TLine *l;
  p = text_clamp(t, p);
  l = &t->lines[p.line];
  t->anchor = t->caret = p;
  while (t->anchor.col > 0 && is_word(l->s[t->anchor.col - 1])) t->anchor.col--;
  while (t->caret.col < l->len && is_word(l->s[t->caret.col])) t->caret.col++;
  t->typing = 0;
}

/* ---------------------------------------------------------------- finding */
static int fold(int c)
{
  c &= 255;
  if ((c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7)) return c + 32;
  return c;
}
/* what matches line l at col */
static int match_at(const TLine *l, long col, const char *w, long wl, int flags)
{
  long i;
  if (col + wl > l->len) return 0;
  for (i = 0; i < wl; i++) {
    int a = (unsigned char)l->s[col + i], b = (unsigned char)w[i];
    if (flags & FIND_CASE ? a != b : fold(a) != fold(b)) return 0;
  }
  if (flags & FIND_WORD) {
    if (col > 0 && is_word(l->s[col - 1])) return 0;
    if (col + wl < l->len && is_word(l->s[col + wl])) return 0;
  }
  return 1;
}
int text_find(TextDoc *t, const char *w, int flags)
{
  long wl = (long)strlen(w), n, k, line, col;
  TPos a, b;
  if (!wl || strchr(w, '\n')) return 0;
  text_sel_range(t, &a, &b);
  /* every place once, from the caret on, round the end */
  if (!(flags & FIND_BACK)) {
    line = b.line; col = b.col;
    for (n = 0; n <= t->nlines; n++) {
      const TLine *l = &t->lines[line];
      for (k = col; k + wl <= l->len; k++)
        if (match_at(l, k, w, wl, flags)) {
          t->anchor.line = t->caret.line = line; t->anchor.col = k; t->caret.col = k + wl; t->typing = 0;
          return 1;
        }
      line = (line + 1) % t->nlines; col = 0;
    }
  } else {
    line = a.line; col = a.col - 1;
    for (n = 0; n <= t->nlines; n++) {
      const TLine *l = &t->lines[line];
      for (k = col < l->len - wl ? col : l->len - wl; k >= 0; k--)
        if (match_at(l, k, w, wl, flags)) {
          t->anchor.line = t->caret.line = line; t->anchor.col = k; t->caret.col = k + wl; t->typing = 0;
          return 1;
        }
      line = (line + t->nlines - 1) % t->nlines; col = t->lines[line].len;
    }
  }
  return 0;
}
static int sel_matches(const TextDoc *t, const char *w, int flags)
{
  TPos a, b;
  long wl = (long)strlen(w);
  text_sel_range(t, &a, &b);
  return a.line == b.line && b.col - a.col == wl && match_at(&t->lines[a.line], a.col, w, wl, flags);
}
int text_replace(TextDoc *t, const char *w, const char *with, int flags)
{
  int done = 0;
  if (text_has_sel(t) && sel_matches(t, w, flags)) { text_insert(t, with, (long)strlen(with)); text_break_join(t); done = 1; }
  text_find(t, w, flags & ~FIND_BACK);
  return done;
}
long text_replace_all(TextDoc *t, const char *w, const char *with, int flags)
{
  long wl = (long)strlen(w), ql = (long)strlen(with), n = 0, line, k;
  int first = 1;
  TPos keep = t->caret;
  if (!wl || strchr(w, '\n') || strchr(with, '\n')) return 0;
  for (line = 0; line < t->nlines; line++)
    for (k = 0; k + wl <= t->lines[line].len; ) {
      if (!match_at(&t->lines[line], k, w, wl, flags)) { k++; continue; }
      {
        TPos a, b;
        a.line = b.line = line; a.col = k; b.col = k + wl;
        if (!del_range(t, a, b, !first)) return n;
        first = 0;
        t->caret = t->anchor = a;
        if (ql && !text_insert(t, with, ql)) return n;
        if (ql) t->undo[t->ndone - 1].chain = 1;
        text_break_join(t);
        k += ql; n++;
      }
    }
  t->caret = t->anchor = text_clamp(t, keep);
  return n;
}
/* the selection, or the caret's line, as whole lines a..b */
static void sel_lines(const TextDoc *t, long *a, long *b)
{
  TPos p, q;
  text_sel_range(t, &p, &q);
  *a = p.line; *b = q.line;
  if (q.line > p.line && q.col == 0) (*b)--;
}
/* lines a..b replaced by s (one undo step), the new lines selected */
static int replace_lines(TextDoc *t, long a, long b, const char *s, long len)
{
  TPos from, to;
  from.line = a; from.col = 0; to.line = b; to.col = t->lines[b].len;
  t->anchor = from; t->caret = to;
  if (!text_insert(t, s, len)) return 0;
  text_break_join(t);
  t->anchor = from;
  return 1;
}
int text_case(TextDoc *t, int upper)
{
  long n, i;
  char *s;
  TPos a, b;
  if (!text_has_sel(t)) { text_select_word(t, t->caret); if (!text_has_sel(t)) return 0; }
  text_sel_range(t, &a, &b);
  if (!(s = text_sel_get(t, &n))) return 0;
  for (i = 0; i < n; i++) {
    int c = (unsigned char)s[i];
    if (upper && ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7))) s[i] = (char)(c - 32);
    else if (!upper) s[i] = (char)fold(c);
  }
  i = text_insert(t, s, n);
  free(s);
  text_break_join(t);
  t->anchor = a;
  return (int)i;
}
static int cmp_lines(const void *x, const void *y)
{
  const char *a = *(const char *const *)x, *b = *(const char *const *)y;
  for (; *a && fold(*a) == fold(*b); a++, b++) ;
  return fold(*a) - fold(*b);
}
int text_sort_lines(TextDoc *t)
{
  long a, b, i, n = 0;
  char **v, *s, *p;
  int ok;
  sel_lines(t, &a, &b);
  if (a == b) { a = 0; b = t->nlines - 1; }
  if (!(v = (char **)malloc((size_t)(b - a + 1) * sizeof *v))) return 0;
  for (i = a; i <= b; i++) { v[i - a] = t->lines[i].s; n += t->lines[i].len + 1; }
  qsort(v, (size_t)(b - a + 1), sizeof *v, cmp_lines);
  if (!(s = (char *)malloc((size_t)n + 1))) { free(v); return 0; }
  for (p = s, i = 0; i <= b - a; i++) { size_t k = strlen(v[i]); memcpy(p, v[i], k); p += k; if (i < b - a) *p++ = '\n'; }
  ok = replace_lines(t, a, b, s, (long)(p - s));
  free(s); free(v);
  return ok;
}
int text_indent(TextDoc *t, int out)
{
  long a, b, i, n = 0;
  char *s, *p;
  int ok;
  sel_lines(t, &a, &b);
  for (i = a; i <= b; i++) n += t->lines[i].len + 3;
  if (!(s = (char *)malloc((size_t)n + 1))) return 0;
  for (p = s, i = a; i <= b; i++) {
    const char *l = t->lines[i].s;
    if (out) { int k = 0; while (k < 2 && *l == ' ') { l++; k++; } }
    else { *p++ = ' '; *p++ = ' '; }
    memcpy(p, l, strlen(l)); p += strlen(l);
    if (i < b) *p++ = '\n';
  }
  ok = replace_lines(t, a, b, s, (long)(p - s));
  free(s);
  return ok;
}

/* ---------------------------------------------------------------- counting, outline */
void text_count(const TextDoc *t, TCount *c)
{
  long i, k;
  memset(c, 0, sizeof *c);
  c->lines = t->nlines;
  for (i = 0; i < t->nlines; i++) {
    const TLine *l = &t->lines[i];
    int inword = 0, blank = 1;
    for (k = 0; k < l->len; k++) {
      int ch = (unsigned char)l->s[k], sp = ch == ' ' || ch == '\t';
      c->chars++;
      if (!sp) { c->chars_nospace++; blank = 0; }
      if (!sp && !inword) c->words++;
      inword = !sp;
    }
    if (!blank && (i == 0 || t->lines[i - 1].len == 0)) c->paragraphs++;
  }
  c->chars += t->nlines - 1;                        /* the line breaks */
}
int text_outline(const TextDoc *t, TOutline *out, int max)
{
  long i;
  int n = 0;
  for (i = 0; i < t->nlines && n < max; i++) {
    const TLine *l = &t->lines[i];
    int lv = 0;
    while (lv < l->len && l->s[lv] == '#') lv++;
    if (lv && lv <= 6 && (lv == l->len || l->s[lv] == ' ')) { out[n].line = i; out[n].level = lv; n++; }
  }
  return n;
}

/* ---------------------------------------------------------------- files */
long utf8_to_latin1(char *s, long len)
{
  long i = 0, o = 0;
  while (i < len) {
    unsigned char c = (unsigned char)s[i];
    unsigned long u;
    int n;
    if (c < 0x80) { s[o++] = (char)c; i++; continue; }
    n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
    u = c & (0x3F >> n);
    if (i + n >= len) { s[o++] = '?'; break; }
    {
      int k;
      for (k = 1; k <= n && i + k < len; k++) u = (u << 6) | ((unsigned char)s[i + k] & 0x3F);
    }
    s[o++] = u < 256 ? (char)u : '?';
    i += n + 1;
  }
  return o;
}
/* s is valid UTF-8 with at least one character past ASCII */
static int is_utf8(const char *s, long len)
{
  long i = 0;
  int any = 0;
  while (i < len) {
    unsigned char c = (unsigned char)s[i];
    int n, k;
    if (c < 0x80) { i++; continue; }
    if (c >= 0xC2 && c <= 0xDF) n = 1; else if (c >= 0xE0 && c <= 0xEF) n = 2; else if (c >= 0xF0 && c <= 0xF4) n = 3; else return 0;
    for (k = 1; k <= n; k++) if (i + k >= len || ((unsigned char)s[i + k] & 0xC0) != 0x80) return 0;
    any = 1;
    i += n + 1;
  }
  return any;
}
int text_load(TextDoc *t, const char *path)
{
  FILE *f = fopen(path, "rb");
  char *buf;
  long n = 0, cap = 4096, k, o;
  size_t got;
  int crlf = 0;
  if (!f) return 0;
  if (!(buf = (char *)malloc((size_t)cap))) { fclose(f); return 0; }
  while ((got = fread(buf + n, 1, (size_t)(cap - n), f)) > 0) {
    n += (long)got;
    if (n == cap) {
      char *nb = (char *)realloc(buf, (size_t)cap * 2);
      if (!nb) { free(buf); fclose(f); return 0; }
      buf = nb; cap *= 2;
    }
  }
  fclose(f);
  /* CR LF and lone CR made LF; NUL bytes dropped */
  for (k = 0, o = 0; k < n; k++) {
    if (buf[k] == '\r') { if (k + 1 < n && buf[k + 1] == '\n') { crlf = 1; continue; } buf[o++] = '\n'; continue; }
    if (buf[k]) buf[o++] = buf[k];
  }
  n = o;
  if (is_utf8(buf, n)) n = utf8_to_latin1(buf, n);
  k = text_set(t, buf, n);
  free(buf);
  if (!k) return 0;
  t->crlf = crlf;
  t->modified = 0;
  strncpy(t->path, path, sizeof t->path - 1); t->path[sizeof t->path - 1] = 0;
  return 1;
}
int text_save(TextDoc *t, const char *path)
{
  FILE *f = fopen(path, "wb");
  long i;
  int ok = 1;
  if (!f) return 0;
  for (i = 0; i < t->nlines && ok; i++) {
    if (t->lines[i].len && fwrite(t->lines[i].s, 1, (size_t)t->lines[i].len, f) != (size_t)t->lines[i].len) ok = 0;
    if (i + 1 < t->nlines && fputs(t->crlf ? "\r\n" : "\n", f) == EOF) ok = 0;
  }
  if (fclose(f)) ok = 0;
  if (!ok) return 0;
  t->modified = 0;
  if (path != t->path) { strncpy(t->path, path, sizeof t->path - 1); t->path[sizeof t->path - 1] = 0; }
  return 1;
}

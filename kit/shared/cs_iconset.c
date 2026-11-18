/*
 * cs_iconset.c - the shared interface icons, in the icon language (cs_icon.h; the specification is
 * docs/shared/12-icons.md). They are the family's own drawings, made for Quill, Ledger, Easel and
 * Folio and gathered here; none is copied or traced from another product.
 *
 * The array is indexed by the CS_IC_ enum: keep the two in the same order, one line each with
 * its name in a comment. tools/iconsheet.py draws every one of them to docs/img/icon-sheet.png.
 */
#include <string.h>
#include "cs_icon.h"

static const char *const set[CS_IC_N][2] = {
  { "NONE", "" },
  /* ---- file */
  { "NEW",        "ci P 5 3 1 10 1 13 4 13 15 3 15 L 3 15 3 1 L 10 1 10 4 L 10 4 13 4" },          /* a page, corner folded */
  { "OPEN",       "ci P 4 1 13 1 4 6 4 7 6 L 7 6 14 6 L 14 6 14 13 L 14 13 1 13 ca P 4 3 13 5 9 15 9 13 13" },   /* a folder, opening */
  { "SAVE",       "ci R 1 1 14 14 F 4 1 11 5 R 3 9 12 14" },                                       /* a disk: shutter and label */
  { "SAVEAS",     "ci R 1 1 11 11 F 3 1 8 4 R 3 7 9 11 ca L 9 14 14 9 L 12 9 14 9 L 14 9 14 11" }, /* a smaller disk and a pen-stroke arrow */
  { "CLOSE",      "ci R 2 1 13 15 cr L 5 6 10 11 L 10 6 5 11" },                                  /* a page crossed out */
  { "PRINT",      "ci R 4 1 11 5 R 1 5 14 11 R 4 9 11 15 ca F 11 7 12 7" },                        /* a printer with paper */
  { "EXPORT",    ">ci R 2 4 10 15 ca L 7 9 14 2 L 10 2 14 2 L 14 2 14 6" },                        /* an arrow leaving a page */
  { "IMPORT",    ">ci R 5 1 14 15 cd L 7 5 12 5 L 7 8 12 8 L 7 11 12 11 ca L 1 8 8 8 L 5 5 8 8 L 5 11 8 8" },   /* an arrow into a page */
  { "INFO",       "ci O 7 7 6 ca F 7 6 8 11 F 7 3 8 4" },                                         /* i in a ring */
  { "RECENT",     "ci O 8 8 6 ca L 8 8 8 4 L 8 8 11 8 ci L 1 3 3 5 L 3 5 5 3" },                   /* a clock with a turn arrow */
  { "OPTIONS",    "ci O 7 7 3 L 7 1 7 3 L 7 11 7 13 L 1 7 3 7 L 11 7 13 7 L 3 3 4 4 L 10 10 11 11 L 3 11 4 10 L 10 4 11 3" },   /* a cog */
  { "ABOUT",      "ci O 7 7 6 ca L 6 5 7 4 L 7 4 9 4 L 9 4 9 6 L 9 6 7 8 L 7 8 7 9 F 7 11 7 11" }, /* ? in a ring */
  { "QUIT",      ">ci P 4 9 4 9 1 2 1 2 14 P 3 2 14 9 14 9 11 ca L 6 8 14 8 L 11 5 14 8 L 11 11 14 8" },   /* a door and an arrow out */
  { "BACK",      ">ci L 9 3 5 8 L 5 8 9 13" },                                                    /* a chevron back */
  /* ---- edit */
  { "UNDO",      ">ci P 4 13 9 13 5 9 2 3 2 L 3 2 5 0 L 3 2 5 4" },                                /* a curved arrow back */
  { "REDO",      ">ci P 4 2 9 2 5 6 2 12 2 L 12 2 10 0 L 12 2 10 4" },                             /* and forward */
  { "CUT",        "ci O 4 12 2 O 11 12 2 L 5 10 11 1 L 10 10 4 1" },                               /* scissors */
  { "COPY",       "ci R 2 1 9 11 cw F 3 2 8 10 ca R 6 5 13 15 cw F 7 6 12 14 ci L 8 8 11 8 L 8 10 11 10 L 8 12 11 12" },   /* two sheets */
  { "PASTE",      "ci R 3 2 12 15 F 6 1 9 3 cw F 4 4 11 14 ca R 7 7 14 15 cw F 8 8 13 14 ci L 9 10 12 10 L 9 12 12 12" },  /* a clipboard and a sheet */
  { "DELETE",     "ci R 2 2 13 13 cr L 4 4 11 11 L 11 4 4 11" },                                   /* a box with a red cross */
  { "SELECTALL",  "ci L 1 1 3 1 L 6 1 9 1 L 12 1 14 1 L 1 14 3 14 L 6 14 9 14 L 12 14 14 14 L 1 4 1 6 L 1 9 1 11 L 14 4 14 6 L 14 9 14 11 ca L 4 5 11 5 L 4 8 11 8 L 4 11 9 11" },   /* a dotted frame round lines */
  { "FIND",       "ci O 6 6 4 L 9 9 14 14 L 10 9 15 14" },                                        /* a magnifier */
  { "REPLACE",    "ca B b ci L 9 4 14 4 L 12 2 14 4 L 12 6 14 4 ca L 2 12 7 12 L 4 10 2 12 L 4 14 2 12 ci B c" },   /* b to c and back */
  { "GOTO",      ">ci R 7 1 14 15 cw F 8 2 13 14 ca L 1 8 10 8 L 7 5 10 8 L 7 11 10 8" },          /* an arrow to a page */
  { "CLEAR",      "ci L 1 12 4 3 L 4 3 7 12 L 2 9 6 9 cr F 9 10 14 14 cw L 10 13 13 11" },         /* A and an eraser */
  /* ---- text */
  { "BOLD",       "B B" },                                                                                     /* B in the bold interface font */
  { "ITALIC",     "I I" },                                                                                     /* I, slanted */
  { "UNDERLINE",  "U U" },                                                                                     /* U, underlined */
  { "FONT",       "ci L 1 12 5 2 L 5 2 9 12 L 3 8 7 8 ca R 10 10 15 15 L 12 12 15 15 L 15 12 15 15 L 12 15 15 15" },   /* A and the dialog's corner */
  { "GROW",       "ci L 1 14 5 4 L 5 4 9 14 L 3 10 7 10 ca L 11 5 13 2 L 13 2 15 5" },             /* A and up */
  { "SHRINK",     "ci L 1 14 5 4 L 5 4 9 14 L 3 10 7 10 ca L 11 2 13 5 L 13 5 15 2" },             /* A and down */
  { "CASE",       "ci L 1 13 4 4 L 4 4 7 13 L 2 10 6 10 O 11 11 2 L 13 9 13 13" },                 /* Aa */
  { "ALIGNLEFT",  "ci L 1 2 14 2 L 1 5 10 5 L 1 8 14 8 L 1 11 10 11 L 1 14 12 14" },                           /* lines flush left */
  { "ALIGNCENTRE","ci L 1 2 14 2 L 3 5 12 5 L 1 8 14 8 L 3 11 12 11 L 2 14 13 14" },                           /* lines centred */
  { "ALIGNRIGHT", "ci L 1 2 14 2 L 5 5 14 5 L 1 8 14 8 L 5 11 14 11 L 3 14 14 14" },                           /* lines flush right */
  { "JUSTIFY",    "ci L 1 2 14 2 L 1 5 14 5 L 1 8 14 8 L 1 11 14 11 L 1 14 9 14" },                            /* lines flush both sides, the last short */
  { "BULLETS",   ">ci E 2 3 1 E 2 8 1 E 2 13 1 L 5 3 14 3 L 5 8 14 8 L 5 13 14 13" },                          /* three dots and their lines */
  { "NUMBERS",    "ca L 2 1 2 5 L 1 8 3 8 L 3 8 1 11 L 1 11 3 11 ci L 6 3 14 3 L 6 8 14 8 L 6 13 14 13" },     /* 1, 2, 3 in the accent and their lines */
  { "INDENT",    ">ci L 1 2 14 2 L 7 6 14 6 L 7 10 14 10 L 1 14 14 14 ca P 3 1 5 4 8 1 11" },                  /* lines moved in, an arrow pointing in */
  { "OUTDENT",   ">ci L 1 2 14 2 L 7 6 14 6 L 7 10 14 10 L 1 14 14 14 ca P 3 4 5 1 8 4 11" },                  /* lines moved out, an arrow pointing out */
  { "SORT",       "ca L 1 2 4 2 L 1 2 4 6 L 1 6 4 6 ci L 11 1 11 14 L 9 12 11 14 L 13 12 11 14 ca L 1 10 4 10 L 2 10 2 14 L 1 14 4 14" }, /* A over Z beside a down arrow */
  { "WRAP",      ">ci L 1 3 14 3 L 1 8 11 8 L 1 13 6 13 ca P 4 11 8 14 8 14 12 9 12 L 9 12 11 10 L 9 12 11 14" },   /* lines with a return arrow */
  { "SPELLING",   "ci B abc cg L 5 12 8 15 L 8 15 14 8" },                                                     /* abc and a green tick */
  { "WORDCOUNT",  "ci R 1 1 14 14 ca B 123" },                                                                 /* 123 in a box */
  /* ---- insert */
  { "DATE",       "ci R 1 3 14 14 F 1 3 14 5 L 4 1 4 3 L 11 1 11 3 ca F 4 8 5 9 F 7 8 8 9 F 10 8 11 9 F 4 11 5 12 F 7 11 8 12" },   /* a calendar */
  { "TIME",       "ci O 8 8 6 ca L 8 8 8 4 L 8 8 11 10" },                                        /* a clock */
  { "SYMBOL",     "ci O 7 6 4 L 3 13 5 13 L 9 13 12 13 L 5 10 5 13 L 9 10 9 13" },               /* omega */
  { "PICTURE",    "ci R 1 2 14 13 ca E 5 6 2 cg P 5 2 12 6 8 9 10 12 7 14 12" },                  /* sun over hills */
  { "TABLE",      "ci R 1 2 14 14 ca F 2 3 13 5 ci L 1 6 14 6 L 1 10 14 10 L 5 6 5 14 L 10 6 10 14" },         /* a grid with its heading row in the accent */
  { "LINK",       "ci R 1 5 9 11 R 2 6 8 10 ca R 7 5 15 11 R 8 6 14 10" },                        /* two links of a chain */
  { "COMMENT",    "ci R 1 2 14 11 L 4 11 4 14 L 4 14 7 11 ca L 4 5 11 5 L 4 8 9 8" },              /* a speech balloon */
  { "PAGEBREAK",  "ci R 3 1 12 6 R 3 10 12 15 cd L 1 8 3 8 L 5 8 7 8 L 9 8 11 8 L 13 8 15 8" },                /* two pages and a dashed line between */
  { "BOOKMARK",   "ca F 5 2 11 9 P 6 4 1 12 1 12 15 8 11 4 15 4 1" },                              /* a ribbon marker */
  /* ---- view */
  { "ZOOMIN",     "ci O 6 6 4 L 9 9 14 14 ca L 4 6 8 6 L 6 4 6 8" },                                           /* a magnifier with a plus */
  { "ZOOMOUT",    "ci O 6 6 4 L 9 9 14 14 ca L 4 6 8 6" },                                                     /* a magnifier with a minus */
  { "ZOOM100",    "ci R 3 1 12 15 ca B 1" },                                                                   /* a page with 1 on it */
  { "PANE",      ">ci R 1 2 14 13 ca F 2 3 5 12 cd L 7 5 12 5 L 7 8 12 8 L 7 11 10 11" },          /* a window with a side pane */
  { "STATUSBAR",  "ci R 1 2 14 13 ca F 2 10 13 12 cd L 3 5 12 5 L 3 7 10 7" },                    /* a window with a bar along its foot */
  { "RULER",      "ci R 1 5 14 10 L 3 5 3 7 L 6 5 6 8 L 9 5 9 7 L 12 5 12 8" },                                /* a ruler with its ticks */
  { "LINENUM",    "ca B 1 ci L 7 4 15 4 L 7 8 15 8 L 7 12 13 12 cd L 1 8 4 8 L 1 12 4 12" },                   /* a 1 beside lines */
  { "FULLSCREEN", "ci L 1 4 1 1 L 1 1 4 1 L 11 1 14 1 L 14 1 14 4 L 14 11 14 14 L 14 14 11 14 L 4 14 1 14 L 1 14 1 11 ca L 4 7 11 7" }, /* four corners and a line: the window opened out */
  { "DARK",       "ci O 8 8 6 E 8 8 5 cw E 10 6 4" },                                             /* a crescent moon */
  { "LANGUAGE",   "ci O 8 8 6 L 2 8 14 8 L 8 2 8 14 ca P 4 8 2 5 5 5 11 8 14 P 4 8 2 11 5 11 11 8 14" },   /* a globe */
  { "HELP",       "ca O 8 8 6 ci L 6 6 7 5 L 7 5 9 5 L 9 5 10 6 L 10 6 8 8 L 8 8 8 10 F 8 12 8 12" },          /* ? in an accent ring */
  { "SEARCH",     "cd O 6 6 4 L 9 9 13 13" },                                                    /* the Tell Me box's magnifier */
  /* ---- arrows and marks */
  { "ARROWDOWN",  "ci L 4 6 8 10 L 8 10 12 6" },                                                               /* a chevron down (drop-down arrows) */
  { "ARROWUP",    "ci L 4 10 8 6 L 8 6 12 10" },                                                               /* a chevron up */
  { "ARROWLEFT",  "ci L 10 4 6 8 L 6 8 10 12" },                                                               /* a chevron left */
  { "ARROWRIGHT", "ci L 6 4 10 8 L 10 8 6 12" },                                                               /* a chevron right */
  { "TICK",       "cg L 3 8 6 11 L 6 11 13 4" },                                                               /* a green tick */
  { "CROSS",      "ci L 4 4 11 11 L 11 4 4 11" },                                                              /* an x (close boxes) */
  { "PIN",        "ci L 8 9 8 15 R 5 2 10 6 L 3 9 12 9 L 5 6 4 9 L 10 6 11 9" },                               /* a drawing pin */
  { "MORE",       "ci F 2 7 3 8 F 7 7 8 8 F 12 7 13 8" },                                         /* three dots */
  { "WARNING",    "co P 4 8 1 15 14 1 14 8 1 ci F 8 5 8 10 F 8 12 8 12" },                        /* a triangle with ! */
  { "STOP",       "cr E 8 8 6 cw F 4 7 12 9" },                                                  /* a red disc with a bar */
  { "NOTE",       "ca E 8 8 6 cw F 8 7 8 11 F 8 4 8 5" },                                         /* i on a blue disc */
};

static const char *const *app_src, *const *app_names;
static int napp;
void cs_icon_set_app(const char *const *srcs, const char *const *names, int n) { app_src = srcs; app_names = names; napp = n; }
int cs_icon_count_app(void) { return napp; }
const char *cs_icon_src(int icon)
{
  if (icon > CS_IC_NONE && icon < CS_IC_N) return set[icon][1];
  if (icon >= CS_IC_APP && icon < CS_IC_APP + napp && app_src) return app_src[icon - CS_IC_APP];
  return "";
}
const char *cs_icon_name(int icon)
{
  if (icon >= CS_IC_NONE && icon < CS_IC_N) return set[icon][0];
  if (icon >= CS_IC_APP && icon < CS_IC_APP + napp && app_names) return app_names[icon - CS_IC_APP];
  return "";
}

/* ---------------------------------------------------------------- the interpreter */
int cs_ink_of(char k)
{
  static const char letters[] = "iargoydwlts";
  const char *p = strchr(letters, k);
  return p && k ? (int)(p - letters) : CS_INK_ICON;
}
/* the next number, spaces before it skipped; *ok cleared if there isn't one */
static int num(const char **p, int *ok)
{
  int v = 0, neg = 0, any = 0;
  while (**p == ' ') (*p)++;
  if (**p == '-') { neg = 1; (*p)++; }
  while (**p >= '0' && **p <= '9') { v = v * 10 + (**p - '0'); (*p)++; any = 1; }
  if (!any) *ok = 0;
  return neg ? -v : v;
}
#define MX(x) (mir ? 15 - (x) : (x))
int cs_icon_run(const char *src, const CsIconOps *o, int mirror)
{
  const char *p = src;
  int mir, ok = 1;
  if (!p) return 1;
  mir = *p == '>' && mirror;
  if (*p == '>') p++;
  if (o->ink) o->ink(o->ctx, CS_INK_ICON);
  while (*p && ok) {
    char c = *p++;
    int a, b, e, f;
    switch (c) {
    case ' ': break;
    case 'c': if (o->ink) o->ink(o->ctx, cs_ink_of(*p ? *p : 'i')); if (*p) p++; break;
    case 'L': a = num(&p, &ok); b = num(&p, &ok); e = num(&p, &ok); f = num(&p, &ok);
      if (ok && o->line) o->line(o->ctx, MX(a), b, MX(e), f);
      break;
    case 'R': case 'F': a = num(&p, &ok); b = num(&p, &ok); e = num(&p, &ok); f = num(&p, &ok);
      if (ok && o->box) { int x0 = MX(a), x1 = MX(e); if (x0 > x1) { int t = x0; x0 = x1; x1 = t; } o->box(o->ctx, x0, b, x1, f, c == 'F'); }
      break;
    case 'O': case 'E': a = num(&p, &ok); b = num(&p, &ok); e = num(&p, &ok);
      if (ok && o->circle) o->circle(o->ctx, MX(a), b, e, c == 'E');
      break;
    case 'P': {
      int n = num(&p, &ok), i, xy[64];
      if (n < 1 || n > 32) { ok = 0; break; }
      for (i = 0; i < n && ok; i++) { xy[2 * i] = num(&p, &ok); xy[2 * i] = MX(xy[2 * i]); xy[2 * i + 1] = num(&p, &ok); }
      if (ok && o->poly) o->poly(o->ctx, xy, n);
      break; }
    case 'B': case 'I': case 'U': {
      const char *t;
      int n = 0;
      while (*p == ' ') p++;
      t = p;
      while (*p && *p != ' ' && n < 7) { p++; n++; }
      if (!n) { ok = 0; break; }
      if (o->text) o->text(o->ctx, c, t, n);
      break; }
    default: ok = 0;
    }
  }
  return ok;
}
#undef MX
/* a check run: every number kept in 0..15 (a circle's centre and radius kept inside too) */
struct Chk { int bad; };
static void ck_line(void *c, int a, int b, int e, int f) { if (a < 0 || a > 15 || b < 0 || b > 15 || e < 0 || e > 15 || f < 0 || f > 15) ((struct Chk *)c)->bad = 1; }
static void ck_box(void *c, int a, int b, int e, int f, int fl) { ck_line(c, a, b, e, f); if (b > f || fl < 0) ((struct Chk *)c)->bad = 1; }
static void ck_circle(void *c, int x, int y, int r, int fl) { if (fl < 0 || r < 0 || x - r < 0 || x + r > 15 || y - r < 0 || y + r > 15) ((struct Chk *)c)->bad = 1; }
static void ck_poly(void *c, const int *xy, int n) { int i; for (i = 0; i < n; i++) ck_line(c, xy[2 * i], xy[2 * i + 1], xy[2 * i], xy[2 * i + 1]); }
int cs_icon_check(const char *src)
{
  struct Chk k = { 0 };
  CsIconOps o;
  memset(&o, 0, sizeof o);
  o.ctx = &k; o.line = ck_line; o.box = ck_box; o.circle = ck_circle; o.poly = ck_poly;
  if (!cs_icon_run(src, &o, 0)) return 1;
  return k.bad ? 2 : 0;
}

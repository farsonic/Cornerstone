/*
 * iconsheet.c - (built and run by iconsheet.py) every icon of the shared set, and of a program's own
 * set when ICON_INIT is defined, run through the kit's own interpreter (cs_icon_run) and scaled the
 * way the Amiga renderer (kit/amiga/cs_theme.c) scales them, written out as drawing operations in
 * pixels for the Python side to rasterise:
 *   icon NAME SIZE MIRROR / ink K / line X0 Y0 X1 Y1 / box X0 Y0 X1 Y1 / fill X0 Y0 X1 Y1 /
 *   circle CX CY R / text STYLE STRING / end
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include "cs_icon.h"
#ifdef ICON_INIT
void ICON_INIT(void);
#endif

static int size;
#define SC(v) ((v) * size / 16)
static void ink(void *c, int k) { (void)c; printf("ink %d\n", k); }
static void line(void *c, int a, int b, int e, int f) { (void)c; printf("line %d %d %d %d\n", SC(a), SC(b), SC(e), SC(f)); }
static void box(void *c, int a, int b, int e, int f, int filled)
{
  int grow = size > 16 ? size / 16 - 1 : 0;
  (void)c;
  if (filled) printf("fill %d %d %d %d\n", SC(a), SC(b), SC(e) + grow, SC(f) + grow);
  else printf("box %d %d %d %d\n", SC(a), SC(b), SC(e), SC(f));
}
static void circle(void *c, int x, int y, int r, int filled)
{
  int rr = SC(r) > 0 ? SC(r) : 1, dy;
  (void)c;
  if (!filled) { printf("circle %d %d %d\n", SC(x), SC(y), rr); return; }
  for (dy = -rr; dy <= rr; dy++) {                  /* the same spans cs_theme.c fills */
    int dx = 0;
    while ((dx + 1) * (dx + 1) + dy * dy <= rr * rr + rr) dx++;
    printf("fill %d %d %d %d\n", SC(x) - dx, SC(y) + dy, SC(x) + dx, SC(y) + dy);
  }
}
static void poly(void *c, const int *xy, int n)
{
  int i;
  (void)c;
  for (i = 1; i < n; i++) printf("line %d %d %d %d\n", SC(xy[2 * i - 2]), SC(xy[2 * i - 1]), SC(xy[2 * i]), SC(xy[2 * i + 1]));
}
static void text(void *c, int style, const char *s, int n) { (void)c; printf("text %c %.*s\n", style, n, s); }
static void one(int icon, int sz, int mirror)
{
  CsIconOps o;
  memset(&o, 0, sizeof o);
  o.ink = ink; o.line = line; o.box = box; o.circle = circle; o.poly = poly; o.text = text;
  size = sz;
  printf("icon %s %d %d\n", cs_icon_name(icon), sz, mirror);
  if (!cs_icon_run(cs_icon_src(icon), &o, mirror)) printf("bad\n");
  printf("end\n");
}
int main(void)
{
  static const int sizes[] = { 16, 24, 48 };
  int i, k, n;
#ifdef ICON_INIT
  ICON_INIT();
#endif
  n = cs_icon_count_app();
  for (i = 1; i < CS_IC_N + n; i++) {
    int icon = i < CS_IC_N ? i : CS_IC_APP + (i - CS_IC_N);
    for (k = 0; k < 3; k++) one(icon, sizes[k], 0);
    if (cs_icon_src(icon)[0] == '>') one(icon, 16, 1);
  }
  return 0;
}

# 9. Right to left on the Amiga

Shared chapter 9 has the approach. The kit's switch is `cs_rtl` (`cs_theme.h`), set before the
window is made:

```c
cs_rtl = prefs.rtl;                 /* Jotter: rtl=1 in Jotter.prefs, a test switch */
```

With it on, `cs_rb_layout()` mirrors the ribbon, `cs_icon()` mirrors icons whose string starts with
`>`, the tabs, status bar, pane headers, the File page column and pop-up lists turn round
(each class has an `if (cs_rtl)` where it places things; the layout of the ribbon is mirrored in
one place, its last step).

For a document view that must draw right to left, copy Ledger's way: lay out and draw in logical
coordinates, and include a header after every other one that replaces graphics.library's calls with
mirroring versions while a flag is on (`ledger/amiga/gmirror.h`):

```c
#define GMX(x) gg_mx(&grid_mirror, (long)(x))          /* x mirrored while grid_mirror.on */
static void gm_move(struct RastPort *rp, LONG x, LONG y) { Move(rp, GMX(x), y); }
static void gm_rectfill(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1)
{
  if (grid_mirror.on) { LONG t = GMX(x1); x1 = GMX(x0); x0 = t; }
  RectFill(rp, x0, y0, x1, y1);
}
/* text: its logical span mirrored, the pen left at its logical end */
static void gm_text(struct RastPort *rp, STRPTR s, ULONG n) { ... }
#undef Move
#define Move(rp, x, y) gm_move(rp, x, y)
...
```

Text drawn this way is placed mirrored but reads left to right. Quill's paragraphs go further
(bidi runs, Arabic shaping: `quill/core/bidi.c`).

/*
 * cs_theme.c - see cs_theme.h: the pens, the font loader, drawing helpers, the icon renderer and
 * help bubbles. From the theme.c that Quill, Ledger, Easel and Folio share, without their
 * program-specific parts (TrueType previews, AAText).
 */
#include <string.h>
#include <stdio.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <diskfont/diskfont.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/diskfont.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_theme.h"
#include "cs_lang.h"
#include "cs_icon.h"
#include "cs_util.h"

extern struct Library *DiskfontBase;     /* (optional: NULL if diskfont.library isn't there) */

/* WordCraft's light tokens, and its dark ones (inputs dark; text on the lighter accent dark) */
static const ULONG light_def[CS_T_NPENS] = {
  0xF9F9F9, 0xF0F0F0, 0xE6E6E6, 0xFAFAFA, 0xE1E1E1, 0xC8C8C8, 0x242424, 0x616161, 0xA8A8A8, 0x424242,
  0xE6E6E6, 0xD6D6D6, 0xDDE3F8, 0x3B5BDB, 0xFFFFFF, 0x8A8A8A, 0x707070, 0xFFFFFF, 0xD4D4D4,
  0x707070, 0xF0F0F0, 0xC42B1C, 0x107C10, 0xCA5010, 0xFFE03D, 0xEBEBEB, 0xC8D2F4, 0xFFFFFF, 0xF0F0F0,
  0x2C47B8, 0x5470E0, 0xDCE3FA, 0xFFFFFF
};
static const ULONG dark_def[CS_T_NPENS] = {
  0x323232, 0x262626, 0x282828, 0x323232, 0x1E1E1E, 0x5A5A5A, 0xDCDCDC, 0xA8A8A8, 0x666666, 0xD2D2D2,
  0x3E3E3E, 0x505050, 0x2E3A66, 0x2D8CEB, 0x1E1E1E, 0x5A5A5A, 0x8A8A8A, 0x454545, 0x2E2E2E,
  0xB4B4B4, 0x2A2A2A, 0xEE6A6A, 0x5BC08A, 0xF0A050, 0xE0C040, 0x3A3A3A, 0x4E4E4E, 0x2B2B2B, 0x1B1B1B,
  0x1473E6, 0x4AA0F0, 0x27315C, 0x11162E
};
static const ULONG *light = light_def, *dark = dark_def, *rgb = light_def;
static LONG pens[CS_T_NPENS];
static struct ColorMap *pencm;
static int users, large;
static char fam_reg[32] = "QuillSans.font", fam_bold[32] = "QuillSansB.font";

void cs_theme_colours(const ULONG *l, const ULONG *d)
{
  int was_dark = rgb == dark;
  light = l ? l : light_def; dark = d ? d : dark_def;
  rgb = was_dark ? dark : light;
}
ULONG cs_theme_rgb(int t) { return t >= 0 && t < CS_T_NPENS ? rgb[t] : 0; }
/* each token's pen: ObtainBestPen shares a pen that has the colour already, takes a free one, or
   settles for the nearest (on a screen of few colours the theme comes out approximate). The colour
   map wants 32 bits a gun, hence each 8-bit value times 0x01010101. Every pen goes back with ReleasePen */
static void obtain_pens(void)
{
  int i;
  for (i = 0; i < CS_T_NPENS; i++) {
    ULONG c = rgb[i];
    pens[i] = ObtainBestPen(pencm, ((c >> 16) & 255) * 0x01010101UL, ((c >> 8) & 255) * 0x01010101UL, (c & 255) * 0x01010101UL,
                            OBP_Precision, PRECISION_GUI, TAG_DONE);
  }
}
static void release_pens(void)
{
  int i;
  for (i = 0; i < CS_T_NPENS; i++) if (pens[i] >= 0) ReleasePen(pencm, (ULONG)pens[i]);
}
void cs_theme_set_dark(int on)
{
  if ((rgb == dark) == (on != 0)) return;
  if (users) release_pens();
  rgb = on ? dark : light;
  if (users) obtain_pens();
}
int cs_theme_is_dark(void) { return rgb == dark; }
void cs_theme_set_large(int on) { large = on != 0; }
int cs_theme_is_large(void) { return large; }
void cs_theme_set_family(const char *r, const char *b)
{
  if (r) { cs_strlcpy(fam_reg, r, sizeof fam_reg - 5); if (!strstr(fam_reg, ".font")) strcat(fam_reg, ".font"); }
  if (b) { cs_strlcpy(fam_bold, b, sizeof fam_bold - 5); if (!strstr(fam_bold, ".font")) strcat(fam_bold, ".font"); }
}

/* ---------------------------------------------------------------- the font loader
   A size's file is a hunk file (one hunk: MOVEQ #-1,D0; RTS, then the DiskFontHeader). It is read
   here into chip memory (the blitter draws letters from it) with its relocations applied, neither
   by diskfont.library (some versions won't open fonts from the program's drawer) nor by LoadSeg()
   (some Kickstarts don't honour the hunk's memory flags). */
#define MAXOWN 32
#define KEEPOWN 8
#define MAXFAM 8
#define MAXSIZES 48
struct OwnFont { struct TextFont *tf; UBYTE *mem; char name[32]; UWORD ysize, users; };
struct Family { char name[32]; int n; UWORD ys[MAXSIZES]; char file[MAXSIZES][32]; };
static struct OwnFont own[MAXOWN];
static int nown;
static struct Family fams[MAXFAM];
static int nfams;
static char font_why[120];
const char *cs_fonts_why(void) { return font_why; }
static void why(const char *what, const char *path)
{
  char e[60] = "";
  LONG err = IoErr();
  if (err) Fault(err, NULL, (STRPTR)e, sizeof e);
  snprintf(font_why, sizeof font_why, "%s %s%s%s", what, path, e[0] ? ": " : "", e);
  CS_LOG("font: %s", font_why);
}
static UBYTE *load_hunks(const char *path)
{
  BPTR fh;
  LONG n;
  ULONG *b, *end, *p, size, i;
  UBYTE *mem = NULL;
  if (!(fh = Open((STRPTR)path, MODE_OLDFILE))) { why("can't open", path); return NULL; }
  Seek(fh, 0, OFFSET_END);
  n = Seek(fh, 0, OFFSET_BEGINNING);
  if (n < 32 || n > 1000000 || !(b = (ULONG *)AllocVec((ULONG)n, MEMF_ANY))) { Close(fh); SetIoErr(0); why("can't read", path); return NULL; }
  if (Read(fh, b, n) != n) { Close(fh); FreeVec(b); why("can't read", path); return NULL; }
  Close(fh);
  end = b + n / 4;
  p = b;
  SetIoErr(0);
  if (*p++ != 0x3F3) goto bad;                       /* HUNK_HEADER, no resident names, one hunk */
  while (p < end && *p) p += *p + 1;
  if (++p + 3 > end || p[0] != 1 || p[1] != 0 || p[2] != 0) goto bad;
  p += 3;
  size = *p & 0x3FFFFFFF;
  if ((*p++ & 0xC0000000) == 0xC0000000) p++;
  if (!size || size > 250000 || !(mem = (UBYTE *)AllocVec(size * 4, MEMF_CHIP | MEMF_CLEAR))) goto bad;
  while (p < end) {
    ULONG t = *p++ & 0x3FFFFFFF;
    if (t == 0x3E9 || t == 0x3EA) {                   /* code or data */
      ULONG k = p < end ? *p++ : 0;
      if (p + k > end) goto bad;
      memcpy(mem, p, (k < size ? k : size) * 4);
      p += k;
    } else if (t == 0x3EB) p++;                        /* bss: cleared already */
    else if (t == 0x3EC) {                             /* 32-bit relocations within the hunk */
      while (p < end && *p) {
        ULONG c = *p++, h = p < end ? *p++ : 1;
        if (h != 0 || p + c > end) goto bad;
        for (i = 0; i < c; i++, p++) if (*p + 4 <= size * 4) *(ULONG *)(mem + *p) += (ULONG)mem;
      }
      p++;
    } else if (t == 0x3F0 || t == 0x3F1) {             /* symbols, debug: skipped */
      if (t == 0x3F1) p += (p < end ? *p : 0) + 1;
      else { while (p < end && *p) p += (*p & 0xFFFFFF) + 2; p++; }
    } else if (t == 0x3F2) break;                      /* HUNK_END */
    else goto bad;
  }
  FreeVec(b);
  return mem;
bad:
  FreeVec(b);
  if (mem) FreeVec(mem);
  why("not a font file:", path);
  return NULL;
}
/* a family of PROGDIR:Fonts: its sizes and their files, read from its .font once (a failure too) */
static struct Family *family(const char *name)
{
  struct Family *f;
  struct FontContentsHeader h;
  struct FontContents fc;
  char path[80];
  BPTR fh;
  int i;
  for (i = 0; i < nfams; i++) if (!strcmp(fams[i].name, name)) return fams[i].n ? &fams[i] : NULL;
  if (nfams >= MAXFAM || strlen(name) >= sizeof fams[0].name) return NULL;
  f = &fams[nfams++];
  memset(f, 0, sizeof *f);
  strcpy(f->name, name);
  snprintf(path, sizeof path, "PROGDIR:Fonts/%s", name);
  if (!(fh = Open((STRPTR)path, MODE_OLDFILE))) { why("can't open", path); return NULL; }
  if (Read(fh, &h, sizeof h) == sizeof h && h.fch_FileID == FCH_ID)
    for (i = 0; i < h.fch_NumEntries && f->n < MAXSIZES; i++) {
      if (Read(fh, &fc, sizeof fc) != sizeof fc) break;
      fc.fc_FileName[MAXFONTPATH - 1] = 0;
      if (strlen(fc.fc_FileName) >= sizeof f->file[0]) continue;
      f->ys[f->n] = fc.fc_YSize;
      strcpy(f->file[f->n++], fc.fc_FileName);
    }
  Close(fh);
  if (!f->n) { SetIoErr(0); why("no sizes in", path); }
  return f->n ? f : NULL;
}
/* the nearest size the family has, shared and counted if it is loaded already */
static struct TextFont *load_own(const char *name, UWORD ysize)
{
  struct Family *f = family(name);
  struct DiskFontHeader *dfh;
  char path[80];
  UBYTE *mem;
  int i, best = -1;
  if (!f) return NULL;
  for (i = 0; i < f->n; i++) {
    int dd = f->ys[i] > ysize ? f->ys[i] - ysize : ysize - f->ys[i];
    int bd = best < 0 ? 0 : (f->ys[best] > ysize ? f->ys[best] - ysize : ysize - f->ys[best]);
    if (best < 0 || dd < bd || (dd == bd && f->ys[i] < f->ys[best])) best = i;
  }
  if (best < 0) return NULL;
  for (i = 0; i < nown; i++) if (own[i].ysize == f->ys[best] && !strcmp(own[i].name, name)) { own[i].users++; return own[i].tf; }
  if (nown >= MAXOWN) return NULL;
  snprintf(path, sizeof path, "PROGDIR:Fonts/%s", f->file[best]);
  if (!(mem = load_hunks(path))) return NULL;
  dfh = (struct DiskFontHeader *)(mem + 4);         /* after MOVEQ #-1,D0; RTS */
  if (dfh->dfh_FileID != DFH_ID || dfh->dfh_TF.tf_YSize == 0) { SetIoErr(0); why("not a font:", path); FreeVec(mem); return NULL; }
  dfh->dfh_Segment = 0;
  ExtendFont(&dfh->dfh_TF, NULL);
  own[nown].tf = &dfh->dfh_TF; own[nown].mem = mem; own[nown].ysize = f->ys[best]; own[nown].users = 1;
  cs_strlcpy(own[nown].name, name, sizeof own[nown].name);
  CS_LOG("font: loaded %s (%u high)", path, (unsigned)dfh->dfh_TF.tf_YSize);
  return own[nown++].tf;
}
static void unload(int i)
{
  StripFont(own[i].tf);
  FreeVec(own[i].mem);
  own[i] = own[--nown];
}
struct TextFont *cs_open_font(const char *name, UWORD ysize)
{
  struct TextFont *tf = load_own(name, ysize);
  struct TextAttr ta;
  if (tf || !DiskfontBase) return tf;
  ta.ta_Name = (STRPTR)name; ta.ta_YSize = ysize; ta.ta_Style = 0; ta.ta_Flags = 0;
  return OpenDiskFont(&ta);                        /* installed in FONTS: after all */
}
void cs_close_font(struct TextFont *tf)
{
  int i, idle = 0;
  if (!tf) return;
  for (i = 0; i < nown && own[i].tf != tf; i++) ;
  if (i == nown) { CloseFont(tf); return; }
  if (own[i].users) own[i].users--;
  for (i = 0; i < nown; i++) idle += !own[i].users;
  for (i = nown - 1; i >= 0 && idle > KEEPOWN; i--) if (!own[i].users) { unload(i); idle--; }
}
static struct TextFont *tf_reg, *tf_bold, *tf_big, *tf_small, *dlgfont;
static int dlgtried;
void cs_theme_free(void)
{
  while (nown > 0) unload(nown - 1);
  nfams = 0;
  dlgfont = NULL; dlgtried = 0;
}
int cs_fonts_ok(void) { return tf_reg != NULL || cs_dialog_font() != NULL; }
struct TextFont *cs_dialog_font(void)
{
  if (!dlgtried) { dlgtried = 1; dlgfont = load_own(fam_reg, large ? 15 : 11); }
  return dlgfont;
}
/* the ribbon's colour as an MUI image spec ("2:" and three 32-bit guns) */
const char *cs_dialog_back(void)
{
  static char spec[40];
  ULONG c = rgb[CS_T_RIBBON];
  sprintf(spec, "2:%08lx,%08lx,%08lx", ((c >> 16) & 255) * 0x01010101UL, ((c >> 8) & 255) * 0x01010101UL, (c & 255) * 0x01010101UL);
  return spec;
}

/* ---------------------------------------------------------------- setup and cleanup */
#define MAXUI 96
static struct { Object *obj; struct MUI_EventHandlerNode *ehn; } ui[MAXUI];
static int nui;
static int ui_find(Object *obj)
{
  int i;
  for (i = 0; i < nui && ui[i].obj != obj; i++) ;
  return i < nui ? i : -1;
}
/* the first object to be set up obtains the pens on its screen and opens the fonts; the program's
   objects are taken to be on one screen, so one set of pens serves them all */
void cs_theme_setup(Object *obj)
{
  if (ui_find(obj) >= 0 || nui >= MAXUI) return;
  ui[nui].obj = obj; ui[nui++].ehn = NULL;
  if (users++) return;
  pencm = _screen(obj)->ViewPort.ColorMap;
  obtain_pens();
  tf_reg = load_own(fam_reg, large ? 15 : 11);
  tf_bold = load_own(fam_bold, large ? 15 : 11);
  tf_big = load_own(fam_reg, large ? 35 : 27);
  tf_small = load_own(fam_reg, large ? 12 : 9);
}
void cs_theme_handler(Object *obj, struct MUI_EventHandlerNode *ehn)
{
  int k = ui_find(obj);
  if (k < 0 || ui[k].ehn) return;
  ui[k].ehn = ehn;
  DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)ehn);
}
void cs_theme_cleanup(Object *obj)
{
  int k = ui_find(obj);
  if (k < 0) return;
  if (ui[k].ehn) DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)ui[k].ehn);
  ui[k] = ui[--nui];
  if (!users || --users) return;
  release_pens();
  cs_close_font(tf_reg); cs_close_font(tf_bold); cs_close_font(tf_big); cs_close_font(tf_small);
  tf_reg = tf_bold = tf_big = tf_small = NULL;
}
LONG cs_pen(int t) { return t >= 0 && t < CS_T_NPENS && users ? pens[t] : 1; }
static struct TextFont *deffont(void) { return ((struct GfxBase *)GfxBase)->DefaultFont; }
struct TextFont *cs_font(void) { return tf_reg ? tf_reg : deffont(); }
struct TextFont *cs_bold(void) { return tf_bold ? tf_bold : cs_font(); }
struct TextFont *cs_big(void) { return tf_big ? tf_big : cs_bold(); }
struct TextFont *cs_small(void) { return tf_small ? tf_small : cs_font(); }

/* ---------------------------------------------------------------- drawing */
LONG cs_textlen(struct RastPort *rp, struct TextFont *tf, const char *s)
{
  SetFont(rp, tf);
  SetSoftStyle(rp, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
  s = tr(s);
  return TextLength(rp, (STRPTR)s, (ULONG)strlen(s));
}
void cs_text(struct RastPort *rp, struct TextFont *tf, LONG x, LONG y, const char *s, LONG pen)
{
  SetFont(rp, tf);
  SetSoftStyle(rp, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
  SetAPen(rp, pen);
  SetDrMd(rp, JAM1);
  Move(rp, x, y + tf->tf_Baseline);
  s = tr(s);
  Text(rp, (STRPTR)s, (ULONG)strlen(s));
}
LONG cs_text_fit(struct RastPort *rp, struct TextFont *tf, LONG x, LONG y, const char *s, LONG maxw, LONG pen)
{
  struct TextExtent te;
  ULONG n, fit;
  LONG w, dots;
  s = tr(s);
  n = (ULONG)strlen(s);
  SetFont(rp, tf);
  SetSoftStyle(rp, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
  SetAPen(rp, pen);
  SetDrMd(rp, JAM1);
  if (maxw <= 0) return 0;
  w = TextLength(rp, (STRPTR)s, n);
  Move(rp, x, y + tf->tf_Baseline);
  if (w <= maxw) { Text(rp, (STRPTR)s, n); return w; }
  dots = TextLength(rp, (STRPTR)"...", 3);
  fit = maxw > dots ? TextFit(rp, (STRPTR)s, n, &te, NULL, 1, (ULONG)(maxw - dots), 200) : 0;
  Text(rp, (STRPTR)s, fit);
  if (maxw >= dots) Text(rp, (STRPTR)"...", 3);
  return TextLength(rp, (STRPTR)s, fit) + (maxw >= dots ? dots : 0);
}
void cs_rect(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1, LONG pen)
{
  SetAPen(rp, pen);
  Move(rp, x0, y0); Draw(rp, x1, y0); Draw(rp, x1, y1); Draw(rp, x0, y1); Draw(rp, x0, y0);
}
void cs_fill(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1, LONG pen)
{
  if (x1 < x0 || y1 < y0) return;
  SetAPen(rp, pen);
  RectFill(rp, x0, y0, x1, y1);
}

/* ---------------------------------------------------------------- icons */
int cs_rtl;
struct IconCtx { struct RastPort *rp; LONG x, y, size, mono; int dim; };
#define SC(c, v) ((v) * (c)->size / 16)
static void ic_ink(void *p, int ink)
{
  static const UBYTE tok[CS_NINKS] = { CS_T_ICON, CS_T_ACCENT, CS_T_RED, CS_T_GREEN, CS_T_ORANGE, CS_T_YELLOW, CS_T_DIM,
                                       CS_T_WHITE, CS_T_BORDER2, CS_T_TEXT, CS_T_SELECTION };
  struct IconCtx *c = (struct IconCtx *)p;
  int t = ink >= 0 && ink < CS_NINKS ? tok[ink] : CS_T_ICON;
  if (c->dim && t != CS_T_WHITE) t = CS_T_DISABLED;
  SetAPen(c->rp, c->mono >= 0 ? (t == CS_T_WHITE ? cs_pen(CS_T_ACCENT) : c->mono) : cs_pen(t));
}
static void ic_line(void *p, int a, int b, int e, int f)
{
  struct IconCtx *c = (struct IconCtx *)p;
  Move(c->rp, c->x + SC(c, a), c->y + SC(c, b)); Draw(c->rp, c->x + SC(c, e), c->y + SC(c, f));
}
static void ic_box(void *p, int a, int b, int e, int f, int filled)
{
  struct IconCtx *c = (struct IconCtx *)p;
  LONG grow = c->size > 16 ? c->size / 16 - 1 : 0;   /* filled boxes grow so strokes keep their weight */
  if (filled) RectFill(c->rp, c->x + SC(c, a), c->y + SC(c, b), c->x + SC(c, e) + grow, c->y + SC(c, f) + grow);
  else {
    struct RastPort *rp = c->rp;
    Move(rp, c->x + SC(c, a), c->y + SC(c, b)); Draw(rp, c->x + SC(c, e), c->y + SC(c, b));
    Draw(rp, c->x + SC(c, e), c->y + SC(c, f)); Draw(rp, c->x + SC(c, a), c->y + SC(c, f)); Draw(rp, c->x + SC(c, a), c->y + SC(c, b));
  }
}
static void ic_circle(void *p, int a, int b, int e, int filled)
{
  struct IconCtx *c = (struct IconCtx *)p;
  LONG r = SC(c, e) > 0 ? SC(c, e) : 1, dy;
  if (!filled) { DrawEllipse(c->rp, c->x + SC(c, a), c->y + SC(c, b), r, r); return; }
  for (dy = -r; dy <= r; dy++) {
    LONG dx = 0;
    while ((dx + 1) * (dx + 1) + dy * dy <= r * r + r) dx++;
    RectFill(c->rp, c->x + SC(c, a) - dx, c->y + SC(c, b) + dy, c->x + SC(c, a) + dx, c->y + SC(c, b) + dy);
  }
}
static void ic_poly(void *p, const int *xy, int n)
{
  struct IconCtx *c = (struct IconCtx *)p;
  int i;
  for (i = 0; i < n; i++) {
    if (i) Draw(c->rp, c->x + SC(c, xy[2 * i]), c->y + SC(c, xy[2 * i + 1]));
    else Move(c->rp, c->x + SC(c, xy[0]), c->y + SC(c, xy[1]));
  }
}
static void ic_text(void *p, int style, const char *s, int n)
{
  struct IconCtx *c = (struct IconCtx *)p;
  struct TextFont *tf = style == 'B' ? cs_bold() : cs_font();
  LONG w;
  SetFont(c->rp, tf);
  SetSoftStyle(c->rp, style == 'I' ? FSF_ITALIC : style == 'U' ? FSF_UNDERLINED : 0, FSF_ITALIC | FSF_UNDERLINED | FSF_BOLD);
  if (style == 'B' && tf == cs_font()) SetSoftStyle(c->rp, FSF_BOLD, FSF_BOLD);   /* (no bold font: smeared) */
  w = TextLength(c->rp, (STRPTR)s, (ULONG)n);
  Move(c->rp, c->x + (c->size - w) / 2, c->y + (c->size - tf->tf_YSize) / 2 + tf->tf_Baseline);
  Text(c->rp, (STRPTR)s, (ULONG)n);
  SetSoftStyle(c->rp, 0, FSF_ITALIC | FSF_UNDERLINED | FSF_BOLD);
}
#undef SC
static void run(struct RastPort *rp, const char *src, LONG x, LONG y, LONG size, int dim, LONG mono)
{
  struct IconCtx c;
  CsIconOps o;
  if (!src || !*src) return;
  c.rp = rp; c.x = x; c.y = y; c.size = size; c.mono = mono; c.dim = dim;
  o.ctx = &c; o.ink = ic_ink; o.line = ic_line; o.box = ic_box; o.circle = ic_circle; o.poly = ic_poly; o.text = ic_text;
  SetDrMd(rp, JAM1);
  cs_icon_run(src, &o, cs_rtl);
}
void cs_icon(struct RastPort *rp, int icon, LONG x, LONG y, LONG size, int dim) { run(rp, cs_icon_src(icon), x, y, size, dim, -1); }
void cs_icon_draw(struct RastPort *rp, const char *src, LONG x, LONG y, LONG size, int dim) { run(rp, src, x, y, size, dim, -1); }
void cs_icon_pen(struct RastPort *rp, int icon, LONG x, LONG y, LONG size, LONG pen) { run(rp, cs_icon_src(icon), x, y, size, 0, pen); }

/* ---------------------------------------------------------------- bubbles */
int cs_tips = 1;
const char *cs_bubble(const char *name, const char *body)
{
  static char out[600];
  size_t n;
  if (!cs_tips) return NULL;
  snprintf(out, sizeof out, "\33b%s\33n", name);
  if (!body || !body[0]) return out;
  n = strlen(out);
  out[n++] = '\n';
  cs_wrap(body, out + n, sizeof out - n, CS_WRAP_BUBBLE);
  return out;
}

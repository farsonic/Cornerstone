/*
 * cs_theme.h - the look every Cornerstone class shares: colours as named tokens obtained as pens on
 * the screen (light and dark tables), the interface fonts (QuillSans, the family's own bitmap font,
 * loaded from PROGDIR:Fonts by a loader of our own), drawing helpers that translate as they draw,
 * the icon renderer for the icon language (kit/shared/cs_icon.h) and help bubbles.
 * docs/platforms/amiga/03-theme.md explains each part.
 */
#ifndef CS_THEME_H
#define CS_THEME_H

#include <exec/types.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

/* the colour tokens (the family's: WordCraft's light and dark design tokens) */
enum {
  CS_T_RIBBON, CS_T_TABSTRIP, CS_T_CANVAS, CS_T_PANEL, CS_T_BORDER, CS_T_BORDER2, CS_T_TEXT, CS_T_DIM,
  CS_T_DISABLED, CS_T_ICON, CS_T_HOVER, CS_T_PRESSED, CS_T_CHECKED, CS_T_ACCENT, CS_T_WHITE, CS_T_INPUTBORDER,
  CS_T_GROUPLABEL, CS_T_RULER, CS_T_RULERMARGIN, CS_T_RULERTICK, CS_T_STATUS, CS_T_RED, CS_T_GREEN,
  CS_T_ORANGE, CS_T_YELLOW, CS_T_NAVHOVER, CS_T_SELECTION, CS_T_BALLOON, CS_T_MARKUP, CS_T_ACCENTDARK,
  CS_T_ACCENTHOVER, CS_T_ACCENTTEXT, CS_T_ONACCENT,
  CS_T_NPENS
};
/* the program's own colours in place of the family's (0xRRGGBB, CS_T_NPENS each); NULL keeps those */
void cs_theme_colours(const ULONG *light, const ULONG *dark);
ULONG cs_theme_rgb(int token);                 /* a token's colour in the theme in use */
void cs_theme_set_dark(int dark);              /* pens taken again; the objects redraw themselves */
int  cs_theme_is_dark(void);
void cs_theme_set_large(int large);            /* large interface text: takes effect at the next setup */
int  cs_theme_is_large(void);
/* the interface font's family ("QuillSans": PROGDIR:Fonts/QuillSans.font and QuillSansB.font) */
void cs_theme_set_family(const char *regular, const char *bold);

/* called from each themed object's MUIM_Setup and MUIM_Cleanup: the first obtains the pens and
   opens the fonts, the last gives them back. MUI may send Setup twice or Cleanup without Setup;
   these count each object once. cs_theme_handler adds the object's event handler (removed again by
   cs_theme_cleanup) */
void cs_theme_setup(Object *obj);
void cs_theme_handler(Object *obj, struct MUI_EventHandlerNode *ehn);
void cs_theme_cleanup(Object *obj);
LONG cs_pen(int token);                        /* a token's pen (valid between setup and cleanup) */

/* the fonts: never NULL (each falls back on the next, in the end on the system's default) */
struct TextFont *cs_font(void);                /* 11 pixels (15 large) */
struct TextFont *cs_bold(void);
struct TextFont *cs_big(void);                 /* headings (27, 35 large) */
struct TextFont *cs_small(void);               /* rulers, badges (9, 12 large) */
struct TextFont *cs_dialog_font(void);         /* the dialogs' font (NULL: MUI's) */
const char *cs_dialog_back(void);              /* the dialogs' background as an MUI image spec */
int  cs_fonts_ok(void);                        /* the interface font was found */
const char *cs_fonts_why(void);                /* and if not, why (for the warning at start) */
struct TextFont *cs_open_font(const char *name, UWORD ysize);   /* one of PROGDIR:Fonts' (nearest size) */
void cs_close_font(struct TextFont *tf);
void cs_theme_free(void);                      /* at the very end: every font the loader holds */
/* the dialogs' tags: their font and background (put after VGroup in a dialog's root) */
#define CS_DLG (ULONG)(cs_dialog_font() ? MUIA_Font : TAG_IGNORE), (ULONG)cs_dialog_font(), \
               (ULONG)MUIA_Background, (ULONG)cs_dialog_back()

/* drawing: text translated (tr) and placed by its top; boxes inclusive at both corners */
LONG cs_textlen(struct RastPort *rp, struct TextFont *tf, const char *s);
void cs_text(struct RastPort *rp, struct TextFont *tf, LONG x, LONG y, const char *s, LONG pen);
/* the same, cut to maxw pixels (ending in "..." if it had to be): the width drawn */
LONG cs_text_fit(struct RastPort *rp, struct TextFont *tf, LONG x, LONG y, const char *s, LONG maxw, LONG pen);
void cs_rect(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1, LONG pen);
void cs_fill(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1, LONG pen);

/* icons (cs_icon.h's numbers, or CS_IC_APP + n for the program's own): drawn in a size x size
   square at x, y. dim: disabled (every colour but white the disabled grey) */
void cs_icon(struct RastPort *rp, int icon, LONG x, LONG y, LONG size, int dim);
void cs_icon_draw(struct RastPort *rp, const char *src, LONG x, LONG y, LONG size, int dim);
/* in one pen throughout (white on the accent, say); white parts become the accent */
void cs_icon_pen(struct RastPort *rp, int icon, LONG x, LONG y, LONG size, LONG pen);
extern int cs_rtl;                             /* the interface runs right to left (cs_rtl.h) */

/* a help bubble: the name in bold on the first line (with its shortcut), then the body wrapped at
   46 characters. A static buffer, good until the next call (MUI copies it at once). NULL when
   bubbles are turned off (cs_tips = 0) */
const char *cs_bubble(const char *name, const char *body);
extern int cs_tips;

#endif

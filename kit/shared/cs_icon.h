/*
 * cs_icon.h - the icon language: every interface icon of a Cornerstone program is a short string of
 * drawing commands on a 16 x 16 grid, drawn at any size in the theme's colours (docs/shared/12-icons.md
 * is the full specification). This file is the portable part: the interpreter, which turns a string
 * into calls of a platform's drawing functions, and the shared icon set (cs_iconset.c).
 *
 *   cX           colour for what follows (CS_INK_ below: i a r g o y d w l t s)
 *   L x0 y0 x1 y1   line                 R x0 y0 x1 y1   outline of a box
 *   F x0 y0 x1 y1   filled box           O cx cy r       circle
 *   E cx cy r       filled circle        P n x0 y0 ...   polyline through n points
 *   B text / I text / U text    a few letters, bold / italic / underlined, centred
 *   >  (first character only)   a directional icon: mirrored in right-to-left layouts
 *
 * Coordinates are grid units 0..15 (keep to 1..14: a one-unit margin); the platform scales them by
 * size/16. Colours are tokens, not RGB: the theme decides what "accent" is, light or dark.
 */
#ifndef CS_ICON_H
#define CS_ICON_H

/* the inks an icon names (cX), in the order of their letters "iarg oydw lts" */
enum { CS_INK_ICON, CS_INK_ACCENT, CS_INK_RED, CS_INK_GREEN, CS_INK_ORANGE, CS_INK_YELLOW, CS_INK_DIM,
       CS_INK_WHITE, CS_INK_LIGHT, CS_INK_TEXT, CS_INK_SELECT, CS_NINKS };

/* what the platform does for each command, in grid units (already mirrored if asked) */
typedef struct CsIconOps {
  void *ctx;
  void (*ink)(void *ctx, int ink);                                   /* CS_INK_ */
  void (*line)(void *ctx, int x0, int y0, int x1, int y1);
  void (*box)(void *ctx, int x0, int y0, int x1, int y1, int filled); /* corners inclusive, x0 <= x1 */
  void (*circle)(void *ctx, int cx, int cy, int r, int filled);
  void (*poly)(void *ctx, const int *xy, int n);                     /* n points, xy[2n] */
  void (*text)(void *ctx, int style, const char *s, int n);          /* style 'B', 'I' or 'U' */
} CsIconOps;

/* src drawn through ops; mirror: the layout runs right to left (only '>' icons are mirrored).
   Returns 0 if src is malformed (a command letter it doesn't know, a missing number): what came
   before is drawn, the rest isn't */
int cs_icon_run(const char *src, const CsIconOps *ops, int mirror);
/* src checked: 0 well formed and inside 0..15, 1 malformed, 2 something outside the grid */
int cs_icon_check(const char *src);
int cs_ink_of(char letter);                     /* 'a' -> CS_INK_ACCENT; unknown letters: CS_INK_ICON */

/* ---- the shared icon set: names CS_IC_*, strings in cs_iconset.c. A program's own icons are
   numbered from CS_IC_APP and handed over with cs_icon_set_app() */
enum {
  CS_IC_NONE,
  /* file */
  CS_IC_NEW, CS_IC_OPEN, CS_IC_SAVE, CS_IC_SAVEAS, CS_IC_CLOSE, CS_IC_PRINT, CS_IC_EXPORT, CS_IC_IMPORT,
  CS_IC_INFO, CS_IC_RECENT, CS_IC_OPTIONS, CS_IC_ABOUT, CS_IC_QUIT, CS_IC_BACK,
  /* edit */
  CS_IC_UNDO, CS_IC_REDO, CS_IC_CUT, CS_IC_COPY, CS_IC_PASTE, CS_IC_DELETE, CS_IC_SELECTALL, CS_IC_FIND,
  CS_IC_REPLACE, CS_IC_GOTO, CS_IC_CLEAR,
  /* text */
  CS_IC_BOLD, CS_IC_ITALIC, CS_IC_UNDERLINE, CS_IC_FONT, CS_IC_GROW, CS_IC_SHRINK, CS_IC_CASE, CS_IC_ALIGNLEFT,
  CS_IC_ALIGNCENTRE, CS_IC_ALIGNRIGHT, CS_IC_JUSTIFY, CS_IC_BULLETS, CS_IC_NUMBERS, CS_IC_INDENT, CS_IC_OUTDENT,
  CS_IC_SORT, CS_IC_WRAP, CS_IC_SPELLING, CS_IC_WORDCOUNT,
  /* insert */
  CS_IC_DATE, CS_IC_TIME, CS_IC_SYMBOL, CS_IC_PICTURE, CS_IC_TABLE, CS_IC_LINK, CS_IC_COMMENT, CS_IC_PAGEBREAK,
  CS_IC_BOOKMARK,
  /* view */
  CS_IC_ZOOMIN, CS_IC_ZOOMOUT, CS_IC_ZOOM100, CS_IC_PANE, CS_IC_STATUSBAR, CS_IC_RULER, CS_IC_LINENUM,
  CS_IC_FULLSCREEN, CS_IC_DARK, CS_IC_LANGUAGE, CS_IC_HELP, CS_IC_SEARCH,
  /* arrows and marks, drawn small in controls */
  CS_IC_ARROWDOWN, CS_IC_ARROWUP, CS_IC_ARROWLEFT, CS_IC_ARROWRIGHT, CS_IC_TICK, CS_IC_CROSS, CS_IC_PIN,
  CS_IC_MORE, CS_IC_WARNING, CS_IC_STOP, CS_IC_NOTE,
  CS_IC_N,
  CS_IC_APP = 256
};
const char *cs_icon_src(int icon);              /* the string, "" for none or out of range */
const char *cs_icon_name(int icon);             /* "SAVE"... (the previewer's labels) */
void cs_icon_set_app(const char *const *srcs, const char *const *names, int n);   /* icons CS_IC_APP.. */
int  cs_icon_count_app(void);

#endif

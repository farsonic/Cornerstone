/*
 * jotter.h - what the parts of Jotter's Amiga front end share: the document, the main objects,
 * the text view class and the dialogs. Jotter is Cornerstone's example program: every part of it
 * is built on kit/ and explained in docs/platforms/amiga/16-walkthrough.md.
 */
#ifndef JOTTER_H
#define JOTTER_H

#include "cs_amiga.h"
#include "../core/text.h"

#define JOTTER_VERSION "1.0"
#define JOTTER_DATE "18.11.2026"

/* the tags of Jotter's own class: its own block, apart from the kit's (TAG_USER | 'JT' << 16) */
#define JT_TAG(n) (TAG_USER | 0x4A540000UL | (ULONG)(n))

/* ---- the text view (view.c) */
struct MUI_CustomClass *view_class(void);
void view_class_free(void);
#define MUIA_View_Text      JT_TAG(0x100)   /* (init) TextDoc *: the document shown */
#define MUIA_View_VScroll   JT_TAG(0x101)   /* (init) the vertical scroll bar, kept in step */
#define MUIA_View_HScroll   JT_TAG(0x102)   /* (init) the horizontal one */
#define MUIA_View_Top       JT_TAG(0x103)   /* (set) the first row shown (the scroll bar sets it) */
#define MUIA_View_Left      JT_TAG(0x104)   /* (set) pixels scrolled across */
#define MUIA_View_Changed   JT_TAG(0x105)   /* (notify) TRUE: the text or the caret changed */
#define MUIA_View_Wrap      JT_TAG(0x106)   /* (set, get) long lines broken at the edge */
#define MUIA_View_FontSize  JT_TAG(0x107)   /* (set) 0 small, 1 normal, 2 large */
#define MUIM_View_Refresh   JT_TAG(0x110)   /* the document changed: rows again, caret shown, drawn */

/* ---- main.c */
extern Object *app, *win, *ribbon, *view, *status;
extern TextDoc doc;
void jotter_command(ULONG id, LONG value);
void refresh_all(void);

/* ---- dialogs.c */
void find_dialog(int replace);
void find_again(int back);
void options_dialog(void);
void about_box(void);
extern char find_what[128], replace_with[128];
extern int find_flags;

/* ---- clip.c: the clipboard (iffparse, FTXT) */
int  clip_write(const char *s, long len);
char *clip_read(long *len);              /* malloc'd; NULL if empty */

/* ---- preferences, applied at start (main.c keeps them in Jotter.prefs) */
typedef struct { int dark, large, tips, pane, status, wrap, size, rtl; } Prefs;
extern Prefs prefs;
void prefs_write(void);

#endif

/*
 * cs_classes.h - Cornerstone's custom MUI classes: what each is for, its attributes and methods.
 * Each is a subclass of Area drawn in the theme (cs_theme.h); each file has a cs_<name>_class()
 * that makes the class on first call and a free function for the end. cs_classes_make() makes
 * them all at start (stop with a message if it fails), cs_classes_free() frees them after the
 * application object is disposed. docs/platforms/amiga/04-classes.md documents every one with an
 * annotated listing.
 *
 * Attribute notes: (init) at creation only, (set) may be set, (get) may be read, (notify) others
 * may watch it with MUIM_Notify.
 */
#ifndef CS_CLASSES_H
#define CS_CLASSES_H

#include "cs_amiga.h"
#include "cs_ribbon.h"

int  cs_classes_make(void);                    /* 0 if one couldn't be made */
void cs_classes_free(void);

/* ---- the ribbon (cs_ribbonmui.c): tabs, groups, items from a CsRibbonDef (cs_ribbon.h) */
struct MUI_CustomClass *cs_ribbon_class(void);
#define MUIA_CsRibbon_Def      CS_TAG(0x100)   /* (init) const CsRibbonDef * */
#define MUIA_CsRibbon_Tab      CS_TAG(0x101)   /* (set, get, notify) the tab shown, an index into def->tabs */
#define MUIA_CsRibbon_Command  CS_TAG(0x102)   /* (notify) a command chosen: its id (set each time) */
#define MUIA_CsRibbon_Value    CS_TAG(0x103)   /* (get, set) the list entry that came with the command, -1
                                                  none: read it, then set it back to -1 */
#define MUIA_CsRibbon_Minimised CS_TAG(0x104)  /* (set, get) only the tab strip shown */
#define MUIM_CsRibbon_Refresh  CS_TAG(0x110)   /* states changed: redraw what differs (and re-lay out
                                                  if a contextual tab came or went) */
#define MUIM_CsRibbon_KeyTips  CS_TAG(0x111)   /* show the KeyTips (Alt or F10): letters on the tabs */
#define MUIM_CsRibbon_Open     CS_TAG(0x112)   /* ULONG id: the item for command id shown and opened
                                                  (its list, or the command run): Tell Me does this */
/* the ribbon takes Alt (pressed and let go alone) and F10 for its KeyTips itself, with an event
   handler of priority 1, so it sees keys before the document view; it eats keys only while the
   KeyTips are shown */

/* ---- the drop-down list (cs_popup.c): a borderless window of entries under a point; the entry
   chosen comes back as set(owner, attr, index), after the list has closed */
void cs_popup(Object *owner, LONG x, LONG y, LONG minw, const char *const *labels, const int *icons, int n, int cur, ULONG attr);
#define CS_POP_OFF 0x40000000                  /* an icons[] entry ORed with this: shown disabled */
#define CS_POP_TICK 0x20000000                 /* and with this: ticked */
void cs_popup_close(void);
struct MUI_CustomClass *cs_popup_class(void);

/* ---- the flat scroll bar (cs_scroll.c): a drop-in for a Prop gadget */
struct MUI_CustomClass *cs_scroll_class(void);
#define MUIA_CsScroll_Horiz    CS_TAG(0x200)   /* (init) a horizontal bar */
/* takes MUIA_Prop_Entries, MUIA_Prop_Visible, MUIA_Prop_First (set, get, notify) as a Prop does */
Object *cs_scrollbar(int horiz);

/* ---- tabs (cs_tabs.c): a strip of labels, one active; the accent line under it */
struct MUI_CustomClass *cs_tabs_class(void);
#define MUIA_CsTabs_Labels     CS_TAG(0x300)   /* (init, set) const char *const *, NULL-ended, English */
#define MUIA_CsTabs_Active     CS_TAG(0x301)   /* (set, get, notify) */
#define MUIA_CsTabs_Closable   CS_TAG(0x302)   /* (init) each tab has a close box (document tabs) */
#define MUIA_CsTabs_Close      CS_TAG(0x303)   /* (notify) the tab whose close box was clicked */
Object *cs_tabs(const char *const *labels);

/* ---- the status bar (cs_status.c): fields of text from the left, and from the right */
struct MUI_CustomClass *cs_status_class(void);
#define MUIA_CsStatus_Fields   CS_TAG(0x400)   /* (init) const char *: field widths, "L120 L80 R60"
                                                  (L from the left, R from the right; width in
                                                  pixels, 0 = the rest) */
#define MUIA_CsStatus_Click    CS_TAG(0x401)   /* (get, notify) a field was clicked: its number */
#define MUIM_CsStatus_Set      CS_TAG(0x410)   /* LONG field, const char *text (copied) */

/* ---- a pane's header (cs_pane.c): its title and a close box. cs_pane() makes header and
   contents into one pane group */
struct MUI_CustomClass *cs_panehead_class(void);
#define MUIA_CsPane_Title      CS_TAG(0x500)   /* (init, set) English */
#define MUIA_CsPane_Close      CS_TAG(0x501)   /* (notify) TRUE when its close box is clicked */
Object *cs_pane(const char *title, Object *contents, Object **header);

/* ---- the File page's command column (cs_backstage.c): Back, then the File commands */
struct MUI_CustomClass *cs_backstage_class(void);
#define MUIA_CsBackstage_Ids   CS_TAG(0x600)   /* (init) const ULONG *, 0-ended: command ids (~0UL: a line) */
#define MUIA_CsBackstage_Pick  CS_TAG(0x601)   /* (get, notify) the command clicked (0: Back) */
#define MUIA_CsBackstage_Active CS_TAG(0x602)  /* (set) the command whose page is shown, lit */

/* ---- Tell Me (cs_tellme.c): a box to type in; the commands found, run with Return */
void cs_tellme_open(Object *ribbon);           /* ribbon: commands on it are opened there (lists) */
void cs_tellme_free(void);

/* ---- the off-screen buffer of a document view (cs_buffer.c): drawn into, shown in one blit,
   scrolled with the blitter (ScrollRaster) so only the strip that comes into view is drawn */
typedef struct CsBuffer {
  struct BitMap *bm; struct Layer_Info *li; struct Layer *ly; struct Region *base;
  LONG x0, y0, x1, y1;                         /* the view's box in window coordinates */
  int ok;                                      /* 0: no memory, drawing goes straight to the window */
} CsBuffer;
struct RastPort *cs_buf_begin(CsBuffer *b, Object *obj);   /* the rastport to draw into (window coordinates) */
void cs_buf_show(CsBuffer *b, Object *obj, LONG x0, LONG y0, LONG x1, LONG y1);   /* that part to the window */
/* the picture moved by dx, dy pixels (positive: the content moves left/up, as when scrolling
   down); the strips that came into view are returned in boxes[] (at most 2) for the caller to
   draw, clipped to them, then shown. 0 if it can't (no buffer, or most of the view moved) */
int  cs_buf_scroll(CsBuffer *b, Object *obj, LONG dx, LONG dy, struct Rectangle *boxes, int *nboxes);
void cs_buf_clip(CsBuffer *b, const struct Rectangle *boxes, int n);   /* drawing kept to boxes (n 0: all) */
void cs_buf_free(CsBuffer *b);

#endif

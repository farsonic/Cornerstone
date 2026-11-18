/*
 * cs_ribbon.h - the ribbon as data, and its layout. A ribbon is tabs of groups of items; an item
 * names a command of the registry (cs_cmd.h), which gives it its label, icon, help and state, so
 * a tool on the ribbon and its menu item can never disagree. This file is portable: the layout
 * (cs_ribbon.c) works in pixels given a text-measuring function, so it is tested natively and
 * tools/ribbonfit checks, with the real screen font, that every tab fits a 640-pixel window in
 * every language. The Amiga class (kit/amiga/cs_ribbonmui.c) draws what the layout says; another
 * platform maps the same data to its own toolbar or palettes (docs/shared/04-window.md).
 *
 * The layout follows the family's ribbons (Quill's and Ledger's):
 *   - the tab strip: the File tab first (it opens the File page), the tabs, and at the right end the
 *     Quick Access buttons and the Tell Me box; names squeeze, then are cut, when they don't fit;
 *   - each group: its items in columns (a large item takes a column; small ones go three to a
 *     column), its caption under them;
 *   - too wide: first squeezed (small items lose their labels, large labels go on two lines), then
 *     groups from the right collapse to one button each that opens their tools as a list, then the
 *     widest groups are narrowed to one width.
 *
 * Sizes, from the interface font's height fh (11 for QuillSans 11):
 *   tab strip fh+10, a small item's row fh+7, the groups' body 3 rows + 4, the caption fh+3:
 *   the whole ribbon fh+10 + 2 + 3(fh+7)+4 + fh+3 + 2 = 5fh + 42 (97 pixels at 11).
 */
#ifndef CS_RIBBON_H
#define CS_RIBBON_H

/* item kinds and sizes */
enum { CSK_BUTTON,     /* runs its command */
       CSK_DROP,       /* opens its list; the entry chosen goes to the command as its value */
       CSK_SPLIT,      /* the icon and label run the command (value -1), the arrow opens the list */
       CSK_CHECK,      /* a tick box instead of an icon, ticked while the command is CSS_CHECKED */
       CSK_LABEL };    /* a caption, no command */
enum { CSZ_SMALL,      /* a 16-pixel icon and its label, three to a column */
       CSZ_LARGE,      /* a 24-pixel icon over its label (two lines if need be), the full height */
       CSZ_ICON };     /* a 16-pixel icon alone */
#define CSF_NEWCOL 1   /* start a new column with this item */

typedef struct {
  unsigned char kind, size;
  unsigned short flags;          /* CSF_ */
  unsigned long id;              /* the command (cs_cmd.h) */
  const char *label;             /* NULL: the command's name (cs_cmd_name); "" none */
  int icon;                      /* 0: the command's icon */
  const char *const *list;       /* CSK_DROP, CSK_SPLIT: the entries, English, NULL-ended */
} CsRbItem;
typedef struct { const char *name; const CsRbItem *items; int n; } CsRbGroup;
/* a tab; contextual tabs (context != 0) show only while that command is CSS_CHECKED, as the
   upstream programs show Table Design while the caret is in a table */
typedef struct { const char *name; const CsRbGroup *groups; int n; unsigned long context; } CsRbTab;
typedef struct {
  const CsRbTab *tabs; int ntabs;
  unsigned long file;            /* the File tab's command (opens the File page); 0: no File tab */
  const unsigned long *qa; int nqa;   /* Quick Access commands, shown as icons on the tab strip */
  unsigned long tellme;          /* the Tell Me box's command (opens the search); 0: no box */
} CsRibbonDef;

#define CS_RB_ITEMS(a) (a), (int)(sizeof(a) / sizeof((a)[0]))   /* a table and its length */

/* ---- the layout */
typedef struct { short x0, y0, x1, y1; } CsBox;   /* corners inclusive; x0 < 0: not shown */
#define CS_RB_MAXTABS 16
#define CS_RB_MAXGROUPS 20
#define CS_RB_MAXITEMS 120
#define CS_RB_MAXQA 12
typedef struct {
  int fh;                                        /* the interface font's height */
  long (*textw)(void *ctx, const char *s);       /* a string's width in it (s already translated) */
  void *ctx;
} CsRbMetrics;
typedef struct {
  int width, height, rtl, squeeze, tabs_only;
  int tab;                                       /* the tab shown (an index into def->tabs) */
  CsBox file, tabbox[CS_RB_MAXTABS], qa[CS_RB_MAXQA], tell;
  int tabcap;                                    /* tab names cut to this width (0: whole) */
  int ngroup;
  CsBox group[CS_RB_MAXGROUPS];
  int collapsed[CS_RB_MAXGROUPS];
  int nitem;
  CsBox item[CS_RB_MAXITEMS];
  const CsRbItem *it[CS_RB_MAXITEMS];            /* NULL: a collapsed group's button */
  int igroup[CS_RB_MAXITEMS];
  int overflow;                                  /* pixels the tab is still too wide by (0: fits) */
} CsRbLayout;

/* the shown tab laid out in width pixels (tab: wanted; moved to the first visible one if it is
   hidden). rtl mirrors every box (right-to-left interface languages). tabs_only: the ribbon
   minimised to its tab strip. Text is measured translated */
void cs_rb_layout(const CsRibbonDef *def, int tab, int width, int rtl, int tabs_only, const CsRbMetrics *m, CsRbLayout *out);
int  cs_rb_tab_visible(const CsRibbonDef *def, int t);
/* what is at x, y */
enum { CS_HIT_NONE, CS_HIT_FILE, CS_HIT_TAB, CS_HIT_QA, CS_HIT_TELL, CS_HIT_ITEM, CS_HIT_ARROW, CS_HIT_GROUP };
int  cs_rb_hit(const CsRbLayout *l, int x, int y, int *index);
/* an item's name as shown: its own label or its command's (translated), into out */
char *cs_rb_item_name(const CsRbItem *it, char *out, int cap);
int  cs_rb_item_icon(const CsRbItem *it);
/* the sizes of the parts, from the font height (see the head of this file) */
int  cs_rb_tabh(int fh);
int  cs_rb_rowh(int fh);
int  cs_rb_height(int fh, int tabs_only);
/* the KeyTips: codes for the File tab, the visible tabs, the Quick Access buttons and the Tell Me
   box (codes[0] File, [1 + t] tab t, [1 + ntabs + i] Quick Access i, [1 + ntabs + nqa] Tell Me),
   or for the laid-out items (codes[k] item k). The Tell Me box is Q where no tab has Q */
void cs_rb_keytips_tabs(const CsRibbonDef *def, const CsRbLayout *l, char (*codes)[3]);
void cs_rb_keytips_items(const CsRibbonDef *def, const CsRbLayout *l, char (*codes)[3]);

#endif

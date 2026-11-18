/*
 * cs_cmd.h - Cornerstone's command registry: one table of every command a program has, which the
 * menus, the ribbon or toolbar, the keyboard shortcuts, the context menus and the command search
 * (Tell Me) are all made from. Plain C99, no platform calls: tested natively.
 *
 * A command is its number (the program's ID_ block, see docs/shared/03-commands.md), its English
 * label as the menu shows it ("Save As..."), its shortcut letter, its icon, one or two sentences of
 * help and a few flags. Menus are a separate layout list naming commands by number, so a command
 * appears in a menu, on the ribbon and in a context menu with the same words, icon, shortcut and
 * enabled state everywhere.
 *
 * The program answers one question for all of them: cs_cmd_state(id), set with cs_cmd_set_state().
 */
#ifndef CS_CMD_H
#define CS_CMD_H

/* command flags */
#define CSC_TOGGLE   0x0001      /* shown with a tick when its state says CSS_CHECKED */
#define CSC_NEEDDOC  0x0002      /* disabled while no document is open (cs_cmd_state does it for you) */
#define CSC_NOSEARCH 0x0004      /* not offered by the command search */
#define CSC_HIDDEN   0x0008      /* internal (a class reporting, a dialog's button): never shown */

typedef struct CsCommand {
  unsigned long id;              /* the command's number, > 0 */
  const char *label;             /* English, as the menu shows it ("Save As..."); through tr() */
  const char *key;               /* the menu shortcut letter ("S" for Amiga-S / Command-S), or NULL */
  int icon;                      /* CS_IC_ (cs_icon.h) or the program's own icon number, 0 none */
  const char *help;              /* English: one or two plain sentences (bubbles, search results) */
  unsigned flags;                /* CSC_ */
} CsCommand;

/* the state of a command, as the program sees it right now */
#define CSS_ENABLED 1
#define CSS_CHECKED 2

/* the tables (a program may register several: one per part, as Quill's work packages do) */
#define CS_CMD_MAXTABLES 16
int  cs_cmd_register(const CsCommand *table, int n);     /* 0 if there is no room for another */
void cs_cmd_clear(void);                                 /* every table forgotten (tests) */
const CsCommand *cs_cmd_find(unsigned long id);          /* NULL if no table has it */
int  cs_cmd_count(void);                                 /* every command of every table */
const CsCommand *cs_cmd_at(int i);                       /* the i-th, in registration order */
/* a command's label as a button or a search result shows it: translated, without the "..." and
   without a "_" shortcut marker. Into out (cap bytes); returns out */
char *cs_cmd_name(unsigned long id, char *out, int cap);
/* the command whose shortcut letter is key (either case), 0 if none */
unsigned long cs_cmd_by_key(int key);

/* the program's answer: CSS_ bits for a command. With no function set, every command is enabled;
   CSC_NEEDDOC commands are disabled whenever cs_cmd_set_doc(0) was last said */
void cs_cmd_set_state(int (*state)(unsigned long id));
void cs_cmd_set_doc(int has_doc);
int  cs_cmd_state(unsigned long id);

/* the command search (Tell Me): the commands whose translated name or help contains every word of
   query (any case, accents folded), names that start with the query first. Fills ids (at most max);
   returns how many. Hidden, CSC_NOSEARCH and disabled commands are left out */
int  cs_cmd_search(const char *query, unsigned long *ids, int max);
/* Latin-1 letter folded for comparing: lower case, accents taken off (A-grave -> a) */
int  cs_fold(int c);

/* ---- menus: a layout list naming commands. The platform builds its own menus from it (Amiga:
   cs_menu.c makes a NewMenu table; macOS: an NSMenu), translated as they are made */
enum { CSM_END, CSM_TITLE, CSM_ITEM, CSM_BAR, CSM_SUB, CSM_SUBITEM, CSM_SUBBAR };
typedef struct { int kind; unsigned long id; const char *title; } CsMenuItem;   /* title: CSM_TITLE, CSM_SUB */
#define CS_MENU(t)     { CSM_TITLE, 0, t }
#define CS_ITEM(id)    { CSM_ITEM, (id), 0 }
#define CS_BAR         { CSM_BAR, 0, 0 }
#define CS_SUB(t)      { CSM_SUB, 0, t }
#define CS_SUBITEM(id) { CSM_SUBITEM, (id), 0 }
#define CS_SUBBAR      { CSM_SUBBAR, 0, 0 }
#define CS_MENUEND     { CSM_END, 0, 0 }
/* the limits a menu layout has to keep (Intuition's: 31 menus, 63 items in a menu, 31 in a
   submenu; macOS has none, but a menu that long can't be read either) */
#define CS_MAXMENUS 31
#define CS_MAXITEMS 63
#define CS_MAXSUBS  31
/* the layout checked: limits kept, every command known, no command twice, no shortcut letter used
   by two commands. Each problem is written to report (one line, English) if it isn't NULL.
   Returns the number of problems */
int  cs_menu_check(const CsMenuItem *layout, void (*report)(const char *line));

#endif

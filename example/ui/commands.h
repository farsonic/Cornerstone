/*
 * commands.h - Jotter's commands: their numbers (in blocks per menu, so adding one never moves
 * another's number: CMD=n and saved settings refer to them), the command table, the menu layout,
 * the ribbon and the context menu. All of it is plain data on Cornerstone's portable kit
 * (kit/shared), so tests/coretest.c checks it natively: every command known, the menus within
 * Intuition's limits, every string translated.
 */
#ifndef JOTTER_COMMANDS_H
#define JOTTER_COMMANDS_H

#include "cs_cmd.h"
#include "cs_ribbon.h"
#include "cs_icon.h"

enum {
  /* File */
  ID_NEW = 100, ID_OPEN, ID_SAVE, ID_SAVEAS, ID_REVERT, ID_INFO, ID_OPTIONS, ID_ABOUT, ID_QUIT, ID_FILEPAGE,
  /* Edit */
  ID_UNDO = 200, ID_REDO, ID_CUT, ID_COPY, ID_PASTE, ID_DELETE, ID_SELECTALL, ID_FIND, ID_FINDNEXT, ID_FINDPREV,
  ID_REPLACE, ID_GOTO, ID_UPPER, ID_LOWER, ID_SORT, ID_INDENT, ID_OUTDENT,
  /* Insert */
  ID_DATE = 300, ID_TIME, ID_HEADING, ID_RULE,
  /* View */
  ID_PANE = 400, ID_STATUSBAR, ID_WRAP, ID_TEXTSIZE, ID_DARK, ID_MINIMISE, ID_TELLME, ID_KEYTIPS,
  /* not in the menus: classes reporting */
  ID_VIEWCHG = 900, ID_OUTLINEPICK, ID_STATUSCLICK, ID_PANECLOSE, ID_BACKSTAGE
};
#define ID_LAST_FILE ID_FILEPAGE
#define ID_LAST_EDIT ID_OUTDENT
#define ID_LAST_INSERT ID_RULE
#define ID_LAST_VIEW ID_KEYTIPS

/* Jotter's own icons (CS_IC_APP + n), in jotter_icons[] */
enum { IC_HEADING = CS_IC_APP, IC_RULE, IC_OUTLINE, IC_TEXTSIZE, IC_UPPER, IC_LOWER, IC_MINIMISE, IC_JOTTER_N };
extern const char *const jotter_icons[], *const jotter_icon_names[];
#define JOTTER_NICONS (IC_JOTTER_N - CS_IC_APP)

extern const CsCommand jotter_commands[];
extern const int jotter_ncommands;
extern const CsMenuItem jotter_menus[];
extern const CsRibbonDef jotter_ribbon;
extern const char *const textsizes[];         /* the Text Size list (ID_TEXTSIZE's values) */
extern const unsigned long jotter_ctx_view[];  /* the text's context menu (0: a line) */
extern const int jotter_nctx_view;
extern const unsigned long jotter_filepage[];  /* the File page's column (~0UL: a line), 0-ended */

void jotter_commands_init(void);               /* the table registered, the icons handed over */

#endif

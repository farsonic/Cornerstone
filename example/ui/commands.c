/*
 * commands.c - Jotter's command table, menus, ribbon and context menu (see commands.h). One line
 * per command: its number, its label as the menu shows it, its shortcut, its icon, its help (what
 * the bubble and the command search show) and its flags. The menus, the ribbon, the Quick Access
 * buttons, the File page and the context menu then only name commands by number.
 */
#include <stddef.h>
#include "commands.h"

const CsCommand jotter_commands[] = {
  /* File */
  { ID_NEW,      "New",            "N", CS_IC_NEW,    "Starts a new, empty document.", 0 },
  { ID_OPEN,     "Open...",        "O", CS_IC_OPEN,   "Opens a text file from disk.", 0 },
  { ID_SAVE,     "Save",           "S", CS_IC_SAVE,   "Saves the document under its name.", 0 },
  { ID_SAVEAS,   "Save As...",     0,   CS_IC_SAVEAS, "Saves the document under a new name.", 0 },
  { ID_REVERT,   "Revert",         0,   CS_IC_RECENT, "Puts the document back as it was last saved.", 0 },
  { ID_INFO,     "Info",           0,   CS_IC_INFO,   "Shows the document's name, size and counts.", 0 },
  { ID_OPTIONS,  "Options...",     0,   CS_IC_OPTIONS, "The interface's language, theme and text size.", 0 },
  { ID_ABOUT,    "About Jotter...", "?", CS_IC_ABOUT, "Jotter's version and licence.", 0 },
  { ID_QUIT,     "Quit",           "Q", CS_IC_QUIT,   "Ends Jotter (it asks first if there are changes).", 0 },
  { ID_FILEPAGE, "File",           0,   CS_IC_NONE,   "New, Open, Save, Info and Options, on a page of their own.", CSC_NOSEARCH },
  /* Edit */
  { ID_UNDO,     "Undo",           "Z", CS_IC_UNDO,   "Takes back the last change.", 0 },
  { ID_REDO,     "Redo",           "Y", CS_IC_REDO,   "Does again what Undo took back.", 0 },
  { ID_CUT,      "Cut",            "X", CS_IC_CUT,    "Moves the selected text to the clipboard.", 0 },
  { ID_COPY,     "Copy",           "C", CS_IC_COPY,   "Copies the selected text to the clipboard.", 0 },
  { ID_PASTE,    "Paste",          "V", CS_IC_PASTE,  "Puts in the text the clipboard holds.", 0 },
  { ID_DELETE,   "Delete",         0,   CS_IC_DELETE, "Removes the selected text.", 0 },
  { ID_SELECTALL, "Select All",    "A", CS_IC_SELECTALL, "Selects the whole document.", 0 },
  { ID_FIND,     "Find...",        "F", CS_IC_FIND,   "Looks for words in the document.", 0 },
  { ID_FINDNEXT, "Find Next",      "G", CS_IC_ARROWDOWN, "Finds the next place the words are.", 0 },
  { ID_FINDPREV, "Find Previous",  0,   CS_IC_ARROWUP, "Finds the place before.", 0 },
  { ID_REPLACE,  "Replace...",     "R", CS_IC_REPLACE, "Puts other words in place of the ones found.", 0 },
  { ID_GOTO,     "Go To Line...",  "L", CS_IC_GOTO,   "Moves to a line by its number.", 0 },
  { ID_UPPER,    "UPPER CASE",     0,   IC_UPPER,     "Makes the selected letters capitals.", 0 },
  { ID_LOWER,    "lower case",     0,   IC_LOWER,     "Makes the selected letters small.", 0 },
  { ID_SORT,     "Sort Lines",     0,   CS_IC_SORT,   "Puts the selected lines (or all) in alphabetical order.", 0 },
  { ID_INDENT,   "Indent",         "]", CS_IC_INDENT, "Moves the selected lines in by two spaces.", 0 },
  { ID_OUTDENT,  "Outdent",        "[", CS_IC_OUTDENT, "Moves the selected lines out by two spaces.", 0 },
  /* Insert */
  { ID_DATE,     "Date",           0,   CS_IC_DATE,   "Puts in today's date.", 0 },
  { ID_TIME,     "Time",           0,   CS_IC_TIME,   "Puts in the time now.", 0 },
  { ID_HEADING,  "Heading",        0,   IC_HEADING,   "Makes the line a heading (# at its start): headings are listed in the Outline pane.", 0 },
  { ID_RULE,     "Line",           0,   IC_RULE,      "Puts in a line of dashes across.", 0 },
  /* View */
  { ID_PANE,     "Outline Pane",   "1", IC_OUTLINE,   "Shows or hides the list of headings beside the text.", CSC_TOGGLE },
  { ID_STATUSBAR, "Status Bar",    0,   CS_IC_STATUSBAR, "Shows or hides the bar at the window's foot.", CSC_TOGGLE },
  { ID_WRAP,     "Word Wrap",      "W", CS_IC_WRAP,   "Breaks long lines at the window's edge (only on screen: the file keeps its lines).", CSC_TOGGLE },
  { ID_TEXTSIZE, "Text Size",      0,   IC_TEXTSIZE,  "The size of the document's letters.", 0 },
  { ID_DARK,     "Dark Theme",     0,   CS_IC_DARK,   "Light letters on a dark background, or the other way round.", CSC_TOGGLE },
  { ID_MINIMISE, "Collapse the Ribbon", 0, IC_MINIMISE, "Only the ribbon's tabs are shown; click a tab to see its tools.", CSC_TOGGLE },
  { ID_TELLME,   "Tell Me...",     0,   CS_IC_SEARCH, "Type what you want to do, and Jotter finds the command.", 0 },
  { ID_KEYTIPS,  "Show KeyTips",   0,   CS_IC_HELP,   "A letter on each tab and tool: type it to choose the tool (also F10, or Alt).", 0 },
  /* the classes' reports */
  { ID_VIEWCHG, 0, 0, 0, 0, CSC_HIDDEN }, { ID_OUTLINEPICK, 0, 0, 0, 0, CSC_HIDDEN }, { ID_STATUSCLICK, 0, 0, 0, 0, CSC_HIDDEN },
  { ID_PANECLOSE, 0, 0, 0, 0, CSC_HIDDEN }, { ID_BACKSTAGE, 0, 0, 0, 0, CSC_HIDDEN },
};
const int jotter_ncommands = (int)(sizeof jotter_commands / sizeof jotter_commands[0]);

const CsMenuItem jotter_menus[] = {
  CS_MENU("File"),
  CS_ITEM(ID_NEW), CS_ITEM(ID_OPEN), CS_BAR, CS_ITEM(ID_SAVE), CS_ITEM(ID_SAVEAS), CS_ITEM(ID_REVERT), CS_BAR,
  CS_ITEM(ID_INFO), CS_ITEM(ID_OPTIONS), CS_ITEM(ID_ABOUT), CS_BAR, CS_ITEM(ID_QUIT),
  CS_MENU("Edit"),
  CS_ITEM(ID_UNDO), CS_ITEM(ID_REDO), CS_BAR, CS_ITEM(ID_CUT), CS_ITEM(ID_COPY), CS_ITEM(ID_PASTE), CS_ITEM(ID_DELETE),
  CS_ITEM(ID_SELECTALL), CS_BAR, CS_ITEM(ID_FIND), CS_ITEM(ID_FINDNEXT), CS_ITEM(ID_FINDPREV), CS_ITEM(ID_REPLACE),
  CS_ITEM(ID_GOTO), CS_BAR,
  CS_SUB("Lines"), CS_SUBITEM(ID_UPPER), CS_SUBITEM(ID_LOWER), CS_SUBBAR, CS_SUBITEM(ID_SORT), CS_SUBITEM(ID_INDENT), CS_SUBITEM(ID_OUTDENT),
  CS_MENU("Insert"),
  CS_ITEM(ID_DATE), CS_ITEM(ID_TIME), CS_BAR, CS_ITEM(ID_HEADING), CS_ITEM(ID_RULE),
  CS_MENU("View"),
  CS_ITEM(ID_PANE), CS_ITEM(ID_STATUSBAR), CS_ITEM(ID_WRAP), CS_BAR, CS_ITEM(ID_TEXTSIZE), CS_ITEM(ID_DARK), CS_BAR,
  CS_ITEM(ID_MINIMISE), CS_ITEM(ID_TELLME), CS_ITEM(ID_KEYTIPS),
  CS_MENUEND
};

const char *const textsizes[] = { "Small", "Normal", "Large", NULL };
static const char *const finds[] = { "Find...", "Find Next", "Find Previous", NULL };
static const char *const levels[] = { "Heading 1", "Heading 2", "Heading 3", "Not a Heading", NULL };

static const CsRbItem h_clip[] = {
  { CSK_BUTTON, CSZ_LARGE, 0, ID_PASTE, 0, 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_CUT, 0, 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_COPY, 0, 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_DELETE, 0, 0, 0 },
};
static const CsRbItem h_undo[] = {
  { CSK_BUTTON, CSZ_SMALL, 0, ID_UNDO, 0, 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_REDO, 0, 0, 0 },
};
static const CsRbItem h_edit[] = {
  { CSK_SPLIT, CSZ_LARGE, 0, ID_FIND, "Find", 0, finds },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_REPLACE, "Replace", 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_GOTO, "Go To", 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_SELECTALL, 0, 0, 0 },
};
static const CsRbItem h_lines[] = {
  { CSK_BUTTON, CSZ_SMALL, 0, ID_UPPER, 0, 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_LOWER, 0, 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_SORT, 0, 0, 0 },
  { CSK_BUTTON, CSZ_ICON, CSF_NEWCOL, ID_INDENT, 0, 0, 0 },
  { CSK_BUTTON, CSZ_ICON, 0, ID_OUTDENT, 0, 0, 0 },
};
static const CsRbGroup home[] = {
  { "Clipboard", CS_RB_ITEMS(h_clip) }, { "Undo", CS_RB_ITEMS(h_undo) }, { "Editing", CS_RB_ITEMS(h_edit) }, { "Lines", CS_RB_ITEMS(h_lines) },
};
static const CsRbItem i_when[] = {
  { CSK_BUTTON, CSZ_LARGE, 0, ID_DATE, 0, 0, 0 },
  { CSK_BUTTON, CSZ_LARGE, 0, ID_TIME, 0, 0, 0 },
};
static const CsRbItem i_text[] = {
  { CSK_DROP, CSZ_LARGE, 0, ID_HEADING, 0, 0, levels },
  { CSK_BUTTON, CSZ_LARGE, 0, ID_RULE, 0, 0, 0 },
};
static const CsRbGroup insert[] = { { "Date & Time", CS_RB_ITEMS(i_when) }, { "Text", CS_RB_ITEMS(i_text) } };
static const CsRbItem v_show[] = {
  { CSK_CHECK, CSZ_SMALL, 0, ID_PANE, 0, 0, 0 },
  { CSK_CHECK, CSZ_SMALL, 0, ID_STATUSBAR, 0, 0, 0 },
  { CSK_CHECK, CSZ_SMALL, 0, ID_WRAP, 0, 0, 0 },
};
static const CsRbItem v_look[] = {
  { CSK_DROP, CSZ_LARGE, 0, ID_TEXTSIZE, 0, 0, textsizes },
  { CSK_CHECK, CSZ_SMALL, 0, ID_DARK, "Dark", 0, 0 },
};
static const CsRbItem v_ribbon[] = {
  { CSK_BUTTON, CSZ_SMALL, 0, ID_MINIMISE, "Collapse", 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_TELLME, "Tell Me", 0, 0 },
  { CSK_BUTTON, CSZ_SMALL, 0, ID_KEYTIPS, "KeyTips", 0, 0 },
};
static const CsRbGroup view[] = { { "Show", CS_RB_ITEMS(v_show) }, { "Look", CS_RB_ITEMS(v_look) }, { "Ribbon", CS_RB_ITEMS(v_ribbon) } };
static const CsRbTab tabs[] = { { "Home", CS_RB_ITEMS(home), 0 }, { "Insert", CS_RB_ITEMS(insert), 0 }, { "View", CS_RB_ITEMS(view), 0 } };
static const unsigned long qa[] = { ID_SAVE, ID_UNDO, ID_REDO };
const CsRibbonDef jotter_ribbon = { tabs, 3, ID_FILEPAGE, qa, 3, ID_TELLME };

const unsigned long jotter_ctx_view[] = { ID_CUT, ID_COPY, ID_PASTE, 0, ID_SELECTALL, 0, ID_UPPER, ID_LOWER, 0, ID_FIND };
const int jotter_nctx_view = (int)(sizeof jotter_ctx_view / sizeof jotter_ctx_view[0]);
const unsigned long jotter_filepage[] = { ID_INFO, ~0UL, ID_NEW, ID_OPEN, ID_SAVE, ID_SAVEAS, ID_REVERT, ~0UL, ID_OPTIONS, ID_ABOUT, ID_QUIT, 0 };

/* Jotter's own icons, in the icon language (docs/shared/12-icons.md) */
const char *const jotter_icons[] = {
  /* HEADING: a hash and two lines of text */ "ca L 3 2 2 13 L 7 2 6 13 L 1 5 8 5 L 1 10 8 10 ci L 10 6 14 6 L 10 10 14 10",
  /* RULE: text, a line across, text */ "cd L 2 3 13 3 ca L 1 8 14 8 F 1 7 14 8 cd L 2 13 11 13",
  /* OUTLINE: a pane of headings, one in, beside the text */ ">ci R 1 2 14 13 ca L 3 5 6 5 L 4 8 6 8 L 3 11 6 11 cd L 8 5 12 5 L 8 8 12 8 L 8 11 11 11",
  /* TEXTSIZE: a small and a large A */ "ci L 1 13 3 8 L 3 8 5 13 L 2 11 4 11 L 7 13 10 3 L 10 3 13 13 L 8 10 12 10",
  /* UPPER: AB */ "ci B AB",
  /* LOWER: ab */ "ci B ab",
  /* MINIMISE: the ribbon's tabs and an arrow folding it up */ "ci L 1 3 14 3 R 1 1 5 3 ca L 5 12 8 8 L 8 8 11 12",
};
const char *const jotter_icon_names[] = { "HEADING", "RULE", "OUTLINE", "TEXTSIZE", "UPPER", "LOWER", "MINIMISE" };

void jotter_commands_init(void)
{
  cs_cmd_clear();
  cs_cmd_register(jotter_commands, jotter_ncommands);
  cs_icon_set_app(jotter_icons, jotter_icon_names, JOTTER_NICONS);
}

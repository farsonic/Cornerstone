# 3. Commands: one table drives everything

Every program has one **command table**. A command is a number, an English label as the menu
shows it, a shortcut letter, an icon, one or two sentences of help, and a few flags. Everything
the user can choose is made from it:

```
                       +--> menu bar (built from a layout list of command numbers)
                       +--> ribbon or toolbar (tabs > groups > items naming commands)
  command table  ------+--> Quick Access buttons, the File page's column
  (cs_cmd.h)           +--> context menus (lists of command numbers)
                       +--> keyboard shortcuts (the table's letters; KeyTips from the names)
                       +--> command search, "Tell Me" (the labels and help, in the language shown)
                       +--> help bubbles (name, shortcut, help)

  state(id) ---------->  enabled? ticked?  asked by every one of them
```

and all of them deliver the same number to the same place: one `command(id, value)` function.
A menu item, a ribbon button, a shortcut, a context-menu item, a Tell Me result and the `CMD=n`
start argument used for testing all arrive there as the same id.

## The table

`kit/shared/cs_cmd.h`:

```c
typedef struct CsCommand {
  unsigned long id;      /* the command's number, > 0 */
  const char *label;     /* English, as the menu shows it ("Save As...") */
  const char *key;       /* the shortcut letter ("S": Amiga-S, Command-S), or NULL */
  int icon;              /* CS_IC_ (cs_icon.h) or the program's own icon, 0 none */
  const char *help;      /* English: one or two plain sentences */
  unsigned flags;        /* CSC_TOGGLE, CSC_NEEDDOC, CSC_NOSEARCH, CSC_HIDDEN */
} CsCommand;
```

Jotter's, in full, is `example/ui/commands.c`; a few lines:

```c
const CsCommand jotter_commands[] = {
  { ID_NEW,     "New",        "N", CS_IC_NEW,    "Starts a new, empty document.", 0 },
  { ID_SAVEAS,  "Save As...", 0,   CS_IC_SAVEAS, "Saves the document under a new name.", 0 },
  { ID_WRAP,    "Word Wrap",  "W", CS_IC_WRAP,   "Breaks long lines at the window's edge (only on screen: the file keeps its lines).", CSC_TOGGLE },
  { ID_VIEWCHG, 0, 0, 0, 0, CSC_HIDDEN },          /* a class reporting: not a command anyone chooses */
  ...
};
```

Rules for the table:

- **Labels are the upstream program's names**, in title case, with "..." when the command opens a
  dialog before doing anything. The menu shows the label as it is; buttons and the search show it
  without the "..." (`cs_cmd_name()`).
- **Help says what the command does, in the user's terms** ("Puts other words in place of the ones
  found"), not how ("Runs a substitution"). It becomes the bubble and the search result's line.
- **Every command has an icon** if it may appear on a ribbon, toolbar or the File page; menus on
  the Amiga don't show them, the Mac's may not either, but the table keeps them together.
- **Hidden commands** (`CSC_HIDDEN`) are numbers classes report with ("the caret moved"). They
  stay in the table so that numbers can't collide, and are never shown.

## Numbers in blocks

Command numbers are in blocks per menu with fixed starts, so adding a command never moves
another's number (saved settings, customised ribbons and `CMD=n` refer to them):

```c
enum {
  ID_NEW = 100, ID_OPEN, ID_SAVE, ...                 /* File */
  ID_UNDO = 200, ID_REDO, ...                         /* Edit */
  ID_DATE = 300, ...                                  /* Insert */
  ID_PANE = 400, ...                                  /* View */
  ID_VIEWCHG = 900, ...                               /* not in the menus: classes reporting */
};
```

New commands go at the end of their block. A test checks that no block has run into the next
(Jotter's `tests/coretest.c`, `test_ui`). Bigger programs give each part a block of its own:
Quill's work packages each have 64 ids from `4000 + 64 x n` (`amiga/wphooks.h`), so a package can
be added or left out without touching another's numbers. Numbers below 100 belong to the kit's
dialogs (`CS_ID_OK` and friends).

## State: one answer for everyone

The program answers one question, `state(id)`: is the command usable now (`CSS_ENABLED`) and is
it on (`CSS_CHECKED`)? Menus, the ribbon, Quick Access, the File page and context menus all ask it
(`cs_cmd_state()`), after every command and whenever a class reports a change, so a button and its
menu item can never disagree. Commands that need a document (`CSC_NEEDDOC`) are disabled by the
kit when none is open. **Disable, don't hide**: a command that can't be used now is shown dimmed,
so users learn where it lives.

A command must also check for itself: shortcuts and `CMD=n` reach `command()` without passing
through a menu. Jotter's `jotter_command()` begins with
`if (!(cs_cmd_state(id) & CSS_ENABLED)) return;`.

## The menu layout

Menus are a list naming commands (`CsMenuItem`, built with the `CS_MENU`, `CS_ITEM`, `CS_BAR`,
`CS_SUB`, `CS_SUBITEM` macros). Menu titles and their order follow the upstream program (File,
Edit, View... as its menus or its ribbon's tabs suggest); the platform builds its own menus from
the list (Amiga: `kit/amiga/cs_menu.c` makes an MUI menu strip; classic Mac: `MENU` resources).
`cs_menu_check()` checks the list: every command known, none twice, no shortcut used by two
commands, and the limits a menu system has. Intuition allows 31 menus, 63 items in a menu and 31
in a submenu; Quill's 805 menu entries are counted against these by its debug log. Keep to them
on every platform: a menu longer than that can't be read either.

## Ribbon, toolbar, palettes

The ribbon (or a toolbar, or a classic Mac tool palette) never invents a command: each item names
a command from the table and borrows its label, icon, help and state, and may give a shorter label
of its own ("Find" for "Find..."). Lists that belong to a tool (text sizes, heading levels) are the
item's, and the entry chosen reaches `command()` as the value. Chapter 4 describes the layout.

## Context menus

A context menu is a list of command numbers (0 for a separator) chosen for what was clicked:
nothing a context menu offers is missing from the menu bar. Deciding the list is plain C (it can
be tested); showing it is the platform's (Amiga: `kit/amiga/cs_ctx.c`). Before a context menu
opens, whatever was clicked becomes current, as a left click would make it: right-click a word
outside the selection and the caret moves there first.

## The command search (Tell Me)

`cs_cmd_search(query)` finds the commands whose name or help contains every word typed, in the
language shown, with case and accents folded ("ubersicht" finds "Übersicht"), names that start
with the query first. Disabled and hidden commands are left out. The result is run (or, for a
ribbon tool with a list, its tab is shown and its list opened: the user learns where it lives).

## Keyboard

The table's letters are the menu shortcuts (Amiga-S, Command-S). The upstream program's Ctrl
shortcuts also work where they don't clash, so users of both systems are at home.
KeyTips (Alt alone, or F10) put a letter on every tab and then on every tool, assigned from the
names in the language shown (`cs_keytips_assign()`), so the whole ribbon is usable without a
mouse. Chapter 10 has the rest.

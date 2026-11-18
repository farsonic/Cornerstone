# Cornerstone on the classic Mac OS (System 7)

This is where the family's Mac ports start. Its reference is Apple's *Macintosh Human Interface
Guidelines* (Addison-Wesley, 1992), the book that describes System 7; page numbers below ("HIG
p. 196") are that book's. The chapter maps each shared Cornerstone concept to System 7 practice,
says where Apple's guidelines win over the family's habits (they do whenever the two disagree on a
Mac: users expect their Mac to behave like their Mac), and plans the first port. It is the chapter
the Mac ports will extend; nothing here has been built yet.

There is no classic Mac program in the family so far. [The last section](#a-first-port-jotter-for-system-7)
plans the first: Jotter, the example program, for System 7.

## What carries over unchanged

Everything in `kit/shared` is portable C99 with no platform calls: the command registry and
search, the menu layout check, `tr()` and the five languages, the preferences file format, the
progress hook, wrapping, KeyTips codes (unused on the Mac), the icon language's interpreter, and the
ribbon's data. A port links them as they are, with its own front end in `mac/` beside `amiga/`. The
program's `core/` and `ui/` carry over the same way, and so do the tests: they run natively,
whatever the target.

Shared principles that need no change on the Mac: every command in the menus (Apple's own rule,
HIG p. 162: put most features in the menus as commands), one command table, own icons, five
languages from one strings source, progress and a way to stop for anything slow, memory checked
first, plain kind wording, portable core tested natively.

## The menu bar (HIG chapter 4, pp. 49-130)

The menu bar is mandatory and the **command table drives it**, as on the Amiga:

- **Standard menus first**, in Apple's order: the Apple menu (its first item **About Program...**,
  then a line, then the system's items; HIG p. 98), **File** (New, Open..., Close, Save, Save As...,
  Revert, Page Setup..., Print..., Quit; HIG pp. 99-109), **Edit** (Undo, then Cut, Copy, Paste,
  Clear, Select All, Show Clipboard; HIG pp. 109-117), then the program's own (the upstream tabs'
  names: Insert, View...), then the system's Help (Balloon Help) and Application menus at the right
  (HIG pp. 125-127). The command table's layout list is reordered for the Mac: About moves from File
  to the Apple menu, Options becomes **Preferences...** (in the Edit menu, the System 7 custom; HIG
  p. 37 says only that preferences should be few and remembered between sessions).
- **Item names** are verbs or attributes, title case, "..." when a dialog follows (HIG pp. 58, 67);
  the table's labels already follow that.
- **Toggles** show a checkmark, or change their name (Show Ruler / Hide Ruler; HIG p. 75): the
  table's `CSC_TOGGLE` commands become checkmarked items.
- **Dimmed, not removed**: an item that can't be used in the current context is shown in grey and
  doesn't highlight (HIG pp. 54, 60, 63); `cs_cmd_state()` gives the answer, as on the Amiga.
- **Hierarchical menus** sparingly, for attribute lists (HIG p. 79); Intuition's limits (31 menus,
  63 items) are stricter than any sensible Mac menu, so `cs_menu_check` still applies. A menu bar on
  a 9-inch screen has room for few titles (HIG p. 80): keep the program's own menus to four or five.
- **Command-key equivalents** (HIG p. 128): Apple reserves N, O, W, S, P, Q, Z, X, C, V, A and
  period for New, Open, Close, Save, Print, Quit, Undo, Cut, Copy, Paste, Select All and stopping an
  operation, and Command with Space and the arrows for keyboard scripts; F, G, T, B, I, U are common
  (Find, Find Again, Plain, Bold, Italic, Underline) but not reserved. Equivalents only for the most
  used commands. **This differs from the Amiga's table**: Jotter's Amiga-W is Word Wrap, which on the
  Mac is Close. The port needs a per-platform override of the table's letters (a short list of
  `{ id, key }` applied after registering the table: the kit doesn't have it yet; it is the first
  thing a Mac port adds to `cs_cmd`).

Menus can be `MENU` resources made by a tool from the layout list and the strings file (one set per
language), or built at start with `NewMenu`/`AppendMenu` from the registry in the language chosen,
as the Amiga does; the second keeps one binary for all five languages.

## No ribbon on the classic Mac

System 7 has no ribbon, and Apple's guidelines would not welcome one. The command table's ribbon
data maps to what the Mac has:

| Ribbon part | On System 7 |
|---|---|
| Tabs | Menus. Each tab's commands are in its menu already (that is the family's rule anyway). |
| A group's frequent tools | A **tool palette** in the document window, along the top under the title bar or down the left side (HIG p. 97: never where scroll bars or the close box are expected), with the current tool highlighted and the pointer showing it (HIG p. 96). The items are the ribbon's: each names a command, its icon from the icon language. |
| A whole group's tools | A **utility window** (a floating palette; HIG p. 137) the user opens from the View menu, or a **tear-off menu** (HIG p. 93). Utility windows float over document windows and act on the active one. |
| Drop-down lists (sizes, levels) | **Pop-up menus** (HIG pp. 82-91), or hierarchical menu items. |
| Check items | Checkmarked menu items; a checkbox in a palette. |
| Quick Access | Not needed: the reserved Command keys. |
| KeyTips | Not used: Command-key equivalents and the keyboard navigation of dialogs. |
| Tell Me | Optional: a "Find Command..." item in the Help menu using `cs_cmd_search`; nothing in System 7 asks for it. |
| Collapsing at 640 pixels | Not needed: a palette that doesn't fit the window wraps or becomes a utility window. |

## Windows (HIG chapter 5, pp. 131-174)

| Cornerstone | System 7 |
|---|---|
| One window per document, filling the screen | A **document window** with title bar, close box, zoom box, size box and scroll bars (HIG p. 134), opening at a standard position (staggered from the top left, HIG p. 141 onwards), not necessarily full screen; the zoom box fills the screen. |
| Panes (navigation, outline) | A pane of the document window (HIG pp. 170-173 describe split windows; one split per orientation), or a utility window. Keep one. |
| Status bar | Apple's suggestion: a small status area at the left end of the horizontal scroll bar (HIG p. 161), e.g. the line number or page. Not a full-width bar. |
| Flat scroll bars | The system's scroll bars (HIG p. 158), inactive when the document fits. |
| The File page (backstage) | Not used. Its commands are the File menu; Info becomes a **Get Info**-style modeless dialog. |
| Dialogs | Modal dialogs (no title bar, or movable modal with a title bar and no close box, HIG p. 185), modeless dialogs (Find and Replace is the classic modeless one, HIG p. 178), alerts. |
| The progress window with Abort | A **movable modal dialog** showing the status of a long operation (HIG p. 185 mentions exactly this use), with a progress bar and a Stop or Cancel button; Command-period stops (HIG p. 128, p. 206). |

## Dialogs and alerts (HIG chapter 6, pp. 175-202; chapter 7, pp. 203-228)

Apple's rules win here, and some differ from the Amiga programs' habits:

- **Button order**: the action button at the bottom right, **Cancel to its left** (HIG p. 196). The
  Amiga programs put the action first and Cancel last. `cs_dialog_window` keeps "action then
  Cancel" on the Amiga; the Mac's dialog builder puts Cancel first.
- **The default button** is the most likely, not dangerous, action, marked with a heavy three-pixel
  outline, and Return and Enter press it (HIG p. 206). No default when the likely action loses data.
  No default ring if Return is used inside a text field of the dialog.
- **Cancel** answers Command-period and Esc as well as a click, and the button flashes for the key
  (HIG p. 206). Other keyboard equivalents on buttons are discouraged (HIG p. 206): the family's
  underlined letters are an Amiga habit; on the Mac, labels carry no `_` (the strings keep it; the
  Mac builder drops it).
- **Buttons** are named with a verb, one word if possible, at most three (HIG p. 206); OK and Cancel
  are 59 pixels wide and buttons 20 high (HIG pp. 204-205).
- **Alerts** come in three levels (HIG pp. 193-196): **note** (an "i" icon: information, no threat),
  **caution** (a triangle with "!": a dangerous action ahead, OK or Continue and Cancel), **stop**
  (a hexagon with a hand: it can't be done, OK only). The Amiga's `cs_ask` has no levels; the port
  gives `cs_ask` a level (the icon language already has `NOTE`, `WARNING` and `STOP`) and builds an
  `ALRT`/`DITL`.
- **Radio buttons** for two to five mutually exclusive choices, **checkboxes** for on/off
  (HIG pp. 210-211): the same gadgets the Amiga dialogs use (and no Zune fallback is needed).
- **Messages** (HIG pp. 199, 310): say what happened and what to do, in the user's terms; the
  shared wording chapter already says so.
- **Dialogs** are `DLOG`/`DITL` resources (or built in code); text lengths change with translation,
  so items are laid out with room or per language (HIG p. 17).

## Icons (HIG chapter 8, pp. 229-256)

Two kinds, as on the Amiga:

- **Interface icons** (palette tools, buttons): drawn from the icon language. At run time they can
  be drawn with QuickDraw exactly as the Amiga renderer draws them (`MoveTo`/`LineTo`, `PaintRect`,
  `FrameOval`/`PaintOval`), in Color QuickDraw's colours or, on a black-and-white screen, in black
  with white fills. Or a build tool renders them into `cicn` and `ics#`/`ics8` resources; the icon
  sheet's previewer already rasterises them, so it is the starting point.
- **Finder icons** (the application, its documents, its preferences file): an **icon family**:
  large 32 x 32 and small 16 x 16, each in black and white (`ICN#`, `ics#`, which hold the mask
  too), 4-bit (`icl4`, `ics4`) and 8-bit (`icl8`, `ics8`), with the `BNDL`/`FREF` resources that
  tie them to the application's signature and document types (HIG pp. 234-235). Apple's process
  (HIG p. 236): design the black-and-white large icon first, colour it in 8 bits from the Apple icon
  colour set (HIG p. 240), make the 4-bit version, then the mask, then the small icons. A Mac version
  of `mkicons.py` follows that order: its pictures are drawn at 4x, reduced to one bit by threshold
  for `ICN#` (the mask is the shape filled), quantised to the 34-colour icon set for `icl8` and to
  the 16-colour system palette for `icl4`. Document icons keep the page shape the system uses for documents
  (HIG p. 247), as Apple asks for icon elements to be used consistently (HIG p. 233).

## Colour and screens (HIG chapter 9, pp. 257-266)

- **Black and white first** (HIG p. 263): every design must work on a one-bit screen (the compact
  Macs' 512 x 342), then be coloured. The family's rule "don't rely on colour alone" is the same rule.
- **Few colours** (HIG p. 264), and the system's own colours for windows, menus and highlighting
  (HIG p. 258). The theme's tokens map to Color QuickDraw's colours on a colour screen, and on a
  one-bit screen to black, white and patterns (grey 50% for dimmed). A dark theme isn't a System 7
  convention: the port can offer it but should default to the system's look.
- **Screen sizes**: 512 x 342 (9-inch compact Macs) and 640 x 480 (13-inch) are the ones to design
  for. A document window and its palette must work at 512 x 342 for a small tool, 640 x 480 for a
  document program.

## Fonts

The system font is **Chicago 12** (menus, buttons, dialogs' labels: HIG pp. 60, 207), the
application font usually **Geneva** (9 or 10 for palettes and small labels), **Monaco** the
fixed-width one (Jotter's text). Ask the system for its fonts rather than hard-coding Chicago (HIG
p. 23: other script systems use other fonts and sizes). The shared chapters' fh is Chicago 12's
line height (about 16 pixels) for menus and dialogs, Geneva 9's (about 12) inside palettes.

## Balloon Help (HIG pp. 316-325)

The help bubbles become **Balloon Help**: `hmnu` balloons for menu items, `hdlg` for dialog items,
`hrct`/`hwin` for palettes and window parts. The command table's help is the balloon text, and it
fits: balloons are best short, and at most 255 characters (HIG p. 318). Balloons describe what an
item is and does, and why it may be dimmed (HIG pp. 319-325): the table's help covers the first two;
dimmed states need a second sentence per command where it isn't obvious.

## Keyboard (HIG chapter 10, pp. 267-304)

Return and Enter for the default button, Esc and Command-period for Cancel (HIG p. 277), arrow keys
move the insertion point with Shift extending and modifier keys moving further (by words, to line and
document ends; HIG pp. 282-283), Tab moves between fields. The view's key handling in Jotter's
`view.c` maps over with Option for the Amiga's Alt.

## Language and localisation (HIG pp. 15-24, 305-325)

- **Text in resources** so it can be translated without touching code (HIG p. 17): every string
  comes from the same `lang/strings.txt`, turned into `STR#` resources (or a C table, as on the
  Amiga, with the language chosen at start) and into localised `MENU`, `DITL` and balloon resources.
- **Mac Roman, not Latin-1.** The two share the ASCII range and most accented letters but at
  different codes, and Mac Roman lacks some Latin-1 characters (the multiplication sign, the
  superscript digits, the vulgar fractions, Icelandic letters among them). The generator converts
  and fails on a character Mac Roman lacks; the strings file should keep to letters both have (the
  family's five languages need none of the missing ones).
- **Length**: translations run longer; leave room in dialogs (HIG p. 17), check them laid out in
  German.
- **Alignment and direction** follow the script system (HIG p. 20); the shared right-to-left
  chapter's approach applies to Hebrew and Arabic systems.
- **Terminology**: the system's own terms in each language for its menus (Ablage/Fichier/Archivo/
  Archivio for File on the localised systems), the upstream program's for the rest.

## What is left out, as on the Amiga

Printing to printers (PDF export instead; but Page Setup and Print are standard File items: leave
them out of the menu rather than show them dimmed forever), scripting (AppleScript, beyond the four
Apple events System 7 asks every program to answer: open the application, open documents, print
documents, quit), network features, Publish and Subscribe.

## Building and testing

- **Toolchain**: Retro68 (a GCC cross-compiler for 68k and PowerPC Macs with Rez for resources)
  builds on the same Linux machine that runs the native tests; THINK C or CodeWarrior on a real or
  emulated Mac are the alternatives. The kit's C99 compiles with GCC as it is.
- **Emulators**: Basilisk II (68040, System 7.5 to 8.1, colour) and Mini vMac (compact Macs, black
  and white 512 x 342) for the two ends; a real Mac for speed.
- **The same rules as on the Amiga**: `CMD=`-like start-up testing through a preferences key,
  a debug log, screenshots of every dialog at 512 x 342 and 640 x 480, black and white and colour,
  English and German.
- **Packaging**: a StuffIt or BinHex'd disk image with the application, a Read Me in SimpleText, and
  the source; the key-material refusal runs on the staged files as on the Amiga.

## A first port: Jotter for System 7

The first Mac program should be Jotter, as it was the kit's first Amiga program: small, and every
shared concept in it.

1. **`mac/` beside `amiga/`**, with the same core (`core/text.c`) and data (`ui/commands.c`), the
   kit's `shared` part unchanged. The tests stay as they are.
2. **The menus** from `jotter_menus`, reordered for the Mac (Apple menu with About; File; Edit with
   Find and the Lines submenu; Insert; View), letters overridden where Apple reserves them (Word
   Wrap loses W; Find Next keeps G, Apple's own "Find Again").
3. **A document window** with TextEdit (enough for Jotter's sizes) or the core's own layout drawn
   with QuickDraw (the Amiga view's code maps closely), a line-number status at the left of the
   horizontal scroll bar, the Outline as a pane on the left.
4. **A tool strip** under the title bar with the Home tab's most used commands (Cut, Copy, Paste,
   Undo, Find, Upper and lower case), icons drawn from the icon language.
5. **Find and Replace** as one modeless dialog; Go To Line and Preferences as modal ones; Cancel to
   the left of the action button; alerts with their levels.
6. **Balloon Help** from the command table's help.
7. **Resources** generated from the strings file and the tables: `MENU`s (or menus built at start),
   `STR#`, `DLOG`/`DITL`, `ALRT`, `vers`, `SIZE`, and the Finder icon family with `BNDL`/`FREF`.
8. **Screenshots** in Basilisk II and Mini vMac, in English and German, for this chapter.

What the kit needs for it, in order: per-platform command letters, an alert level for `cs_ask`,
the Mac Roman conversion and check in `mklang.py`, a resource generator for menus and strings, and
the Mac renderer of the icon language.

Appendix: [Modern macOS, briefly](modern-macos.md).

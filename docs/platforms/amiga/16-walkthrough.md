# 16. Walkthrough: Jotter from nothing

Jotter (`example/`) is a plain-text editor built only on the kit: a ribbon with Home, Insert and
View, the File page, an Outline pane, a status bar, Find and Replace, Options, a context menu, five
languages, its own interface and Workbench icons, an Installer script, a package script and native
tests. This chapter brings it up from an empty directory to a running program, in the order a new
program should be built, and explains every file. The files are short enough to read whole: each
section links to one and says what to look for in it.

| File | Lines | What it is |
|---|---|---|
| [`core/text.h`](../../../example/core/text.h), [`core/text.c`](../../../example/core/text.c) | 91, 673 | the document: lines, caret and selection, editing with undo, find and replace, counts, the outline, files |
| [`ui/commands.h`](../../../example/ui/commands.h), [`ui/commands.c`](../../../example/ui/commands.c) | 49, 156 | the command table, menus, ribbon, context menu, File page column, Jotter's own icons |
| [`tests/coretest.c`](../../../example/tests/coretest.c) | 235 | native tests of the core and of the interface's data |
| [`lang/strings.txt`](../../../example/lang/strings.txt), `lang/ignore.txt` | 166, 7 | every string in five languages |
| `amiga/lang_table.h` | 173 | made from the strings by `mklang.py` |
| [`amiga/jotter.h`](../../../example/amiga/jotter.h) | 54 | what the front end's files share |
| [`amiga/main.c`](../../../example/amiga/main.c) | 462 | start-up, the window, refreshing, commands, files, the loop |
| [`amiga/view.c`](../../../example/amiga/view.c) | 393 | the text view class |
| [`amiga/dialogs.c`](../../../example/amiga/dialogs.c) | 136 | Find, Replace, Options, About |
| [`amiga/clip.c`](../../../example/amiga/clip.c) | 64 | the clipboard |
| [`cornerstone.json`](../../../example/cornerstone.json) | 12 | where the kit's tools find things |
| [`build.sh`](../../../example/build.sh), [`tools/test.sh`](../../../example/tools/test.sh) | 19, 19 | the build and the native tests |
| [`tools/wbicons.py`](../../../example/tools/wbicons.py) | 42 | the Workbench icons' pictures |
| [`tools/Install-Jotter`](../../../example/tools/Install-Jotter) | 56 | the Installer script |
| [`tools/package.sh`](../../../example/tools/package.sh) | 32 | the package |
| [`tools/emucheck.sh`](../../../example/tools/emucheck.sh) | | the emulator check and its screenshots |
| [`docs/notepad-gap.md`](../../../example/docs/notepad-gap.md) | 50 | the gap document, against a plain-text editor's usual commands (the tests recount it) |
| `sample/Welcome.txt`, `README.md` | 23, 38 | the example document, the program's README |

About 2,400 lines of Jotter's own, on about 5,000 of the kit's.

## Step 0: the tree

```sh
mkdir -p Jotter/{core,ui,amiga,lang,tests,tools,sample} && cd Jotter
cp -r ../cornerstone/kit kit                 # the kit, kept as it is
cp ../cornerstone/LICENSE .                  # GPL-2.0-only
cp kit/shared/templates/CLAUDE.md .          # then fill it in (shared chapter 15)
mkdir -p .github/workflows && cp kit/shared/templates/workflows/*.yml .github/workflows/
```

and `cornerstone.json` (shared chapter 2). In this repository Jotter sits beside the kit, so its
paths say `../kit/`; in a program of its own they say `kit/` (the package script rewrites them so).

## Step 1: the core, and its tests

Write the document before any window. [`core/text.h`](../../../example/core/text.h) is the whole
interface of it; read it first. Things to notice in `text.c`:

- **Lines** (`TLine`: a string, its length, its capacity), a **caret** and an **anchor**; the
  selection runs between them. Every function keeps positions valid (`text_clamp`).
- **Two raw operations** do every change: `raw_insert` (text with line breaks at a position; the
  new lines made before anything changes, so it fails cleanly without memory) and `raw_delete`.
- **Undo** records each change (`record`): inserted or removed text, where, and the caret before.
  Typing letters joins one step until the caret moves (`typing`); replacing a selection is two
  records chained, undone together; Replace All chains all of its records into one step.
- **Find** walks every line once from the caret, round the end, with case and whole-word options,
  forwards or back; **Replace All** is one undo step.
- **Files**: CR LF and lone CR made LF on loading (CR LF remembered and written back), UTF-8 turned
  into Latin-1 when the file is valid UTF-8 with characters past ASCII, otherwise taken as Latin-1.
- **Counts** and **the outline** (lines that start with `#`) are computed here, not in the view.

[`tests/coretest.c`](../../../example/tests/coretest.c) tests all of it with a `CHECK` macro and an
`is(t, "expected whole text")` helper: editing across lines, the undo chain, typing joined into one
step, word movement, find in every mode and round the end, Replace All as one step, sorting,
indenting, case, counts, the outline, and files (UTF-8 in, CR LF kept, Latin-1 in, a missing file).
`tools/test.sh` builds it with ASan and UBSan:

```
$ sh tools/test.sh
amiga/lang_table.h is up to date (163 rows)
langcheck: 163 rows, 163 used, 0 strings missing, 0 shown without tr(), 0 unused rows, 0 row problems
76 tests, 0 failed          (the kit's own)
66 tests, 0 failed          (Jotter's)
ribbonfit: every tab fits 618 pixels in every language
langfit: everything fits; 69 translations grow over half again (--all lists them)
```

## Step 2: the commands

Before any window too: [`ui/commands.h`](../../../example/ui/commands.h) numbers the commands in
blocks (File 100, Edit 200, Insert 300, View 400, reports 900), and
[`ui/commands.c`](../../../example/ui/commands.c) holds, in this order:

1. **The command table**: one line per command, its label as the menu shows it, its letter, its
   icon, its help, its flags (`CSC_TOGGLE` for Outline Pane, Status Bar, Word Wrap, Dark Theme,
   Collapse the Ribbon; `CSC_HIDDEN` for the classes' reports).
2. **The menus**: File, Edit (with a Lines submenu), Insert, View, naming commands.
3. **The ribbon**: Home (Clipboard with a large Paste, Undo, Editing with a split Find button and its
   list, Lines with two icon-only items starting a new column), Insert (Date & Time, Text with a
   Heading drop-down), View (Show with three check items, Look with the Text Size drop-down and a
   Dark check, Ribbon); the Quick Access buttons (Save, Undo, Redo), the File tab's command and the
   Tell Me box's.
4. **The context menu** of the text, and **the File page's column**.
5. **Jotter's own icons**, in the icon language, numbered from `CS_IC_APP`.

`jotter_commands_init()` registers the table and hands the icons over. Because this is plain data,
`coretest.c`'s `test_ui` checks it: the menus within Intuition's limits, every ribbon item, Quick
Access button, File page entry and context-menu entry a known command, the blocks not overlapping,
every icon well formed, every label translated, every tab laid out at 618 pixels in every language,
and Tell Me finding "umbruch" in German.

## Step 3: the words

Run `python3 ../kit/shared/tools/langcheck.py --rows`: it prints a row for every string in the
sources that has none, English in every column. Paste them into
[`lang/strings.txt`](../../../example/lang/strings.txt) under sections named after the files,
translate the four columns (Word's and Notepad's terms: Zwischenablage, Presse-papiers,
Portapapeles, Appunti...), put the literals that aren't words in `lang/ignore.txt` (the window
title "Jotter - %s%s", "CR LF"), and run `mklang.py`. The kit's own strings (OK, Cancel, Working,
Tell me...) are in `kit/shared/lang/kit-strings.txt` already.

## Step 4: a window

[`amiga/main.c`](../../../example/amiga/main.c) from its end: `main()` is chapter 2's skeleton line
for line. Then `make_window()`:

```c
main_page = VGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0,
  Child, ribbon,                                   /* cs_ribbon_class, MUIA_CsRibbon_Def */
  Child, HGroup, InnerSpacing(0, 0), MUIA_Group_Spacing, 0,
    Child, pane,                                   /* cs_pane("Outline", a Listview) */
    Child, balance,                                /* MUI's splitter */
    Child, VGroup, ...
      Child, HGroup, ..., Child, view, Child, vscroll, End,
      Child, hscroll,
    End,
  End,
  Child, status,                                   /* cs_status_class, "L0 R70 R100 R130" */
End;
file_page = HGroup, ..., Child, backstage, Child, VGroup, ...Info..., End, End;
w = CsWindowObject, ..., WindowContents, pages = VGroup, MUIA_Group_PageMode, TRUE,
      Child, main_page, Child, file_page, End, End;
```

then one notification per class turning its reports into ids for the main loop (the ribbon's
command, the File page's pick, the view's change, the status bar's click, the pane's close box, the
outline list's choice, Esc), and the scroll bars wired to the view. The window fills the screen
below its title bar.

At this point, with a stub view, `sh build.sh` gives a program that opens, shows its ribbon, menus
and status bar, and quits: build it this early, and run it in the emulator.

## Step 5: the commands' state and the refresh

`state(id)` answers for every command (Undo while there is something to undo, Cut and Copy while
there is a selection, the toggles' ticks); `cs_cmd_set_state(state)` gives it to the kit.
`refresh_all()` runs after every command and every report: the title (with `*` when modified), the
status bar's fields, the outline list (rebuilt only when the headings changed), the menus
(`cs_menu_update`) and the ribbon (`MUIM_CsRibbon_Refresh`), and the File page's Info when it is
open. Nothing else updates the interface: one place, so nothing is forgotten.

## Step 6: the view

[`amiga/view.c`](../../../example/amiga/view.c), the program's own class (its tags in its own
block, `JT_TAG`, declared in [`amiga/jotter.h`](../../../example/amiga/jotter.h)). In order:

- **Rows** (`build_rows`): one per line, or with Word Wrap each line broken at its last space that
  fits (`TextFit`); rebuilt when the text or the width changes. `row_of` finds the caret's row.
- **Drawing** (`draw_row`, `draw_all`): each row into the back buffer (`cs_buf_begin`): the paper,
  the selection's part of the row in the selection colour, the text, the caret as a two-pixel bar in
  the accent; then one blit (`cs_buf_show`). The scroll bars are kept in step without notifying
  (`MUIA_NoNotify`), so they don't tell the view what it told them.
- **Scrolling** (`scroll_to`): a short way, `cs_buf_scroll` moves the picture with the blitter and
  only the rows that came into view are drawn; a long way, everything.
- **Keys** (`key`): cursor keys (Shift selects, Alt by words and to the ends), Home, End, Page Up
  and Down, Backspace, Del, Return, Tab (two spaces; Shift-Tab outdents), the wheel, and printable
  characters through `MapRawKey`. Amiga-key combinations are left to the menus.
- **The mouse**: a click places the caret (`pos_at`, with `TextFit` and rounding to the nearer
  edge), Shift-click extends, a double click selects a word, dragging selects.
- **The context menu**: a right click outside the selection moves the caret there first, then
  `cs_ctx_menu("Text", jotter_ctx_view, ...)`.
- **Reporting**: after any change, `MUIA_View_Changed` is set; main.c's notification returns
  `ID_VIEWCHG` and `refresh_all()` does the rest.
- **Fonts**: QuillMono 11, 14 or 17 by the Text Size setting, opened in Setup and closed in Cleanup.

## Step 7: commands

`jotter_command(id, value)` in main.c: one `case` per command, whatever sent it. It begins by
refusing a command that is disabled (a shortcut or `CMD=` can reach it), and ends with
`refresh_all()`. Things to notice: the ribbon's list entries arrive as `value` (Find's list: Find,
Find Next, Find Previous; Heading's levels; Text Size's sizes), and the menu sends -1 (Text Size
then steps to the next size); files go through `may_discard()` first; Sort Lines runs as a job.
The File page: `ID_FILEPAGE` shows or hides it, `report(ID_BACKSTAGE)` handles its column (Back,
Info stays, anything else closes the page and runs).

## Step 8: dialogs and the clipboard

[`amiga/dialogs.c`](../../../example/amiga/dialogs.c): Find and Replace are one function: the grid
of labels and fields, Match case and Whole words only, the direction as `cs_radio` (a cycle gadget
on Zune), `cs_dialog_window3` with Replace All as the extra button, then a loop round
`cs_run_modal` so Replace and Find Next can be pressed again and again. Options: the language (from
the next start), the theme, the text size, large interface text, help bubbles; saved to
`Jotter.prefs`. About: a `cs_ask` with the version.

[`amiga/clip.c`](../../../example/amiga/clip.c): unit 0 of the clipboard as an IFF FORM FTXT with a
CHRS chunk, through iffparse.library opened only while it is used.

## Step 9: checks, icons, package

- `ribbonfit.py` and `langfit.py` (in `tools/test.sh`) prove the ribbon and menus fit 640 pixels in
  every language with the real font.
- `iconsheet.py --root .` draws Jotter's icons beside the shared set (`docs/img/icon-sheet.png`).
- [`tools/wbicons.py`](../../../example/tools/wbicons.py) draws a spiral-bound pad with a pencil for
  the program, the pad small on the drawer; `mkicons.py` writes the `.info` files.
- [`tools/Install-Jotter`](../../../example/tools/Install-Jotter) passes `checkinst.py`.
- [`tools/package.sh`](../../../example/tools/package.sh) `1.0 <workdir>` stages the drawer, makes
  the icons and calls `mkpackage.sh`, which refuses key material and makes `Jotter-1.0.lha`.

## Step 10: the emulator

[`tools/emucheck.sh`](../../../example/tools/emucheck.sh) runs Jotter in FS-UAE (AROS, Zune, a
68040, 640 x 480) under the shared lock (chapter 12): four copies (English, German, dark, right to
left) and two more for Replace and Options with `CMD=`, keys for KeyTips, the File page and Tell Me,
a screenshot of each. The results are in the next section.

## Jotter in the emulator

See [`../../img/jotter/`](../../img/jotter/) and the notes in
[the emulator results](16-walkthrough-emulator.md).

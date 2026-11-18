# Jotter

Jotter is a plain-text editor for AmigaOS 3.x with MUI 3.8 (or AROS and MorphOS with Zune). It is
Cornerstone's example program: small enough to read in an afternoon, and laid out the way the
family's bigger programs (Quill, Ledger, Easel, Folio) are.

- A ribbon with Home, Insert and View tabs, a File tab that opens the File page (Info, New, Open,
  Save, Save As, Revert, Options, About, Quit), Quick Access buttons (Save, Undo, Redo) and a
  Tell Me box: type what you want to do and Jotter finds the command.
- Every command is also in the menus (File, Edit, Insert, View) with its Amiga-key shortcut.
- KeyTips: press F10 (or Alt alone) for a letter on every tab, then on every tool.
- An Outline pane listing the document's headings (lines that start with #); click one to go to it.
- A status bar: the text size, the word count, the line and column (click them).
- Find and Replace (match case, whole words, up or down), Go To Line, UPPER and lower case, Sort
  Lines, Indent and Outdent, the date and time, headings, a ruled line.
- Word Wrap (on screen only), three text sizes, a light and a dark theme.
- Undo and Redo without limit; the clipboard (text) shared with other programs.
- Right-click the text for its menu.
- Five languages: English, German, French, Spanish and Italian, chosen from Locale prefs or in
  Options.
- Files: plain text in Latin-1; UTF-8 is read too. A file keeps its line ends (LF or CR LF).

## Starting it

Double-click its icon, or from the Shell:

    Jotter [file] [PUBSCREEN=name] [LANGUAGE=de] [DEBUG=file] [CMD=n]

`CMD=n` runs command n once Jotter is up (the numbers are in `ui/commands.h`), for testing.

## Building

See `docs/platforms/amiga/16-walkthrough.md` in Cornerstone: `sh build.sh` runs the native tests
and builds Jotter with vbcc; `sh tools/package.sh 1.0 <workdir>` makes the `.lha`.

Jotter is free software under the GNU General Public License, version 2 (see `LICENSE`). Its
screen fonts are renderings of Liberation Sans and Mono under the SIL Open Font License
(`Fonts/LICENSE-fonts.txt`).

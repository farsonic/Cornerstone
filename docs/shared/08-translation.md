# 8. Text and translation

Every program speaks five languages: English, German, French, Spanish and Italian, in the upstream
PC program's own terms for each (Word's, Excel's, Photoshop's, Acrobat's: Datei/Fichier/Archivo/
File, Zwischenablage/Presse-papiers/Portapapeles/Appunti, Ebene/calque/capa/livello). More can be
added later (Quill plans Dutch, Swedish, Danish and Portuguese), but these five are there from the
first release, because translation done late is never done.

## How it works

Words are written **in English in the source**, passed through `tr()` where they are shown:

```c
cs_ask("OK", tr("Jotter couldn't open %s."), path);     /* (cs_ask calls tr() itself, in fact) */
snprintf(s, sizeof s, tr("Line %ld, Column %ld"), line, col);
```

`tr()` (`kit/shared/cs_lang.c`) finds the English in a table of rows sorted once at start, and
gives the chosen language's words, or the English when the table has no row or the row has no
such column, so a new string works before it is translated. Lists (cycle gadgets, pop-ups) go
through `tr_list()`, which keeps a translated copy of a NULL-ended list. **A list must end with
NULL**: Quill's Insert Caption crashed when `tr_list` ran on past the end of two lists without one.

Where possible the platform's drawing helpers call `tr()` themselves (on the Amiga, `cs_text()`
and `cs_textlen()`; the ribbon, menus, the File page and bubbles translate their labels as they
show them), so tables of labels are written in plain English and need nothing more.

## The strings file

One file of rows per program, `lang/strings.txt`, and one for the kit
(`kit/shared/lang/kit-strings.txt`), in the same form:

```
## ui/commands.c: the command table
Save As...<TAB>Speichern unter...<TAB>Enregistrer sous...<TAB>Guardar como...<TAB>Salva con nome...
Find _what<TAB>_Suchen nach<TAB>_Rechercher<TAB>_Buscar<TAB>_Trova
Line %ld, Column %ld<TAB>Zeile %ld, Spalte %ld<TAB>Ligne %ld, colonne %ld<TAB>Línea %ld, columna %ld<TAB>Riga %ld, colonna %ld
```

- One row per English string, **five columns split by a TAB**: the English exactly as the source
  writes it, then German, French, Spanish, Italian.
- UTF-8 in the file, but **only Latin-1 characters** (the Amiga's fonts and the classic Mac's
  Mac Roman both cover Latin-1's letters; no curly quotes, no OE ligature, no euro sign).
- C escapes allowed (`\n`, `\t`, `\33` for MUI's text codes, octal).
- `# ` lines are notes; `## title` starts a section (the source file the rows belong to).
- A column may be `-` for "as English".

This file is the **single source** for every platform: the Amiga's table (`amiga/lang_table.h`,
made by `mklang.py`), the classic Mac's `STR#` resources and localised `MENU`s (the classic Mac
chapter), and anything a later platform needs.

## The tools

| Tool | What it does |
|---|---|
| `kit/shared/tools/mklang.py` | Makes the table from the strings files (`--check`: only says whether it is up to date; the tests run that) |
| `kit/shared/tools/langcheck.py` | Every string literal in the sources must be a row, in an ignore file, or plainly not words (icon drawings, identifiers, file names, log lines). Checks each row's translations keep the English's `printf` conversions in order, its requester buttons (`\|`), its leading text codes and its `_` keyboard letter, and that no translation is another row's English (`tr()` run twice would change it). Flags a literal handed to a gadget or window title without `tr()`. Rows the program no longer uses fail too. `--rows` prints the missing ones ready to paste. |
| `kit/amiga/tools/langfit.py` | The menus in every language against a 640-pixel screen (topaz 8), and every row's longest line in QuillSans 11 against a 600-pixel dialog. `--all` lists translations that grow more than half again over the English, to look at by eye. |
| `kit/amiga/tools/ribbonfit.py` | Every ribbon tab laid out by the kit's own layout code in every language, with the real font: fails if a tab doesn't fit 618 pixels. |

Adding a string: write it in English in the source, run `langcheck.py --rows`, paste the rows into
`lang/strings.txt` under the right section, translate the four columns, run `mklang.py`. The tests
fail until every row is translated and used.

## Choosing the language

The first of the user's preferred languages (Locale prefs on the Amiga, the system script on the
Mac) that the program has; English if none. Options lets the user choose one instead (it takes
effect when the program starts again: menus and gadgets are made once). `LANGUAGE=de` as a start
argument chooses one for a session (used for screenshots and tests).

## Writing for translation

- Whole sentences, never pieces joined (chapter 6).
- Give translators room: German is about a third longer than English. Keep ribbon labels to a word
  or two; the long form goes in the help.
- A keyboard letter (`_`) must be unique in its dialog in every language; check by eye in the
  emulator in German at least.
- Words that read the same in two places but translate differently need two rows: make one of
  them longer ("Editing" the group, "Edit" the menu).
- Translators' choices follow the PC program's localisation: "Rückgängig" not "Zurück",
  "Rechercher" not "Trouver".

# 8. Translation on the Amiga

Shared chapter 8 has the strings file and the tools. On the Amiga:

- **Latin-1 throughout.** The fonts (QuillSans, topaz, the system's) are Latin-1, the strings file
  may hold only Latin-1 letters, and `mklang.py` writes non-ASCII as octal escapes in C
  (`"\374"` for ü) so the source stays ASCII.
- **The table** is a header of rows included into an array in the program:

  ```c
  static const CsWords words[] = {
  #include "lang_table.h"
    { { NULL } }
  };
  ...
  cs_lang_init(words, (int)(sizeof words / sizeof words[0]) - 1);
  cs_lang_choose(args.language);       /* LANGUAGE=, the prefs, Locale prefs, English */
  ```
- **Where translation happens.** The kit's drawing (`cs_text`, the ribbon, the File page, the
  status bar's texts as main.c gives them), the menus (when the strip is made), cycle gadgets
  (`cs_cycle` through `tr_list`), requesters (`cs_ask`), dialog titles (`cs_dialog_window`) translate
  for you. Labels you hand to MUI yourself (`CsLabel`, `TextObject`'s contents, a window title) need
  `tr()`: `langcheck.py` flags them when they don't have it.
- **Locale prefs** name languages in their own words, lower case ("deutsch", "français");
  `cs_lang_by_locale_name()` matches them, so English before German in the user's list keeps the
  program English. locale.library is opened only to look.
- **Changing language** takes effect when the program starts again: menus, cycle gadgets and dialog
  contents are made once. Options says so in the window.
- **The keyboard letter** (`_`) must be unique within each dialog in each language; MUI takes the
  first `_` letter. `langcheck` checks a translation keeps one; uniqueness is checked by eye.

## The fit checks

Both run natively with the real QuillSans 11 widths, read from `kit/amiga/fonts/QuillSans/11` by
`amifont.py`:

- `ribbonfit.py` compiles the program's ribbon and command table (`cornerstone.json` "fit") with
  the kit's own layout code (`fit.c`) and lays every tab out at 618 pixels in each language:

  ```
  de Start      fits
  de Einfügen   fits
  fr Affichage  fits, squeezed, collapsed: Apparence      (at 330 pixels)
  ribbonfit: every tab fits 618 pixels in every language
  ```
  Because it runs the same code the class runs, it can't drift from the ribbon as Quill's earlier
  model of its layout could.
- `langfit.py` lays the menus out in topaz 8 against a 640-pixel screen in each language, and
  measures every row's longest line (wrapped at 66 like a requester) against a 600-pixel dialog.

Both are in `tools/test.sh`, so a translation that breaks the layout fails the tests. They don't
see dialogs' layout as MUI makes it (MUI's fonts and spacing are the user's): German at 640 x 480 in
the emulator is still the final check.

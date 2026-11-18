# 3. The theme

`kit/amiga/cs_theme.[ch]` is the heart of the look. Every class draws through it and never picks
its own colours or fonts.

## Colours are tokens

The theme has named colours (`CS_T_RIBBON`, `CS_T_PANEL`, `CS_T_TEXT`, `CS_T_ACCENT`, `CS_T_HOVER`,
`CS_T_BORDER`...: 33 of them, WordCraft's light and dark design tokens) and two tables of RGB values,
light and dark. A class asks for `cs_pen(CS_T_PANEL)` and gets a pen number on the current screen.
A program may replace the tables (`cs_theme_colours(light, dark)`) with its upstream program's
tokens, keeping the names where the meaning is the same so the kit's classes still work. Never
write an RGB value in a class.

| Token | Light | Dark | Used for |
|---|---|---|---|
| RIBBON | F9F9F9 | 323232 | the ribbon, dialogs' background |
| PANEL | FAFAFA | 323232 | panes, pop-up lists, scroll bar troughs |
| TEXT / DIM / DISABLED | 242424 / 616161 / A8A8A8 | DCDCDC / A8A8A8 / 666666 | text, secondary text, disabled |
| ICON | 424242 | D2D2D2 | the icons' ink |
| HOVER / PRESSED / CHECKED | E6E6E6 / D6D6D6 / DDE3F8 | 3E3E3E / 505050 / 2E3A66 | an item under the pointer, held down, on |
| ACCENT | 3B5BDB | 2D8CEB | the one thing that is on: the current tab's line, a ticked box, the File tab and page |
| ACCENTDARK / ACCENTHOVER / ONACCENT | 2C47B8 / 5470E0 / FFFFFF | 1473E6 / 4AA0F0 / 11162E | the File page's chosen row and hover, text on the accent |
| WHITE | FFFFFF | 1E1E1E | paper: the document's background, inputs, icon fills |
| BORDER / BORDER2 / INPUTBORDER | E1E1E1 / C8C8C8 / 8A8A8A | 1E1E1E / 5A5A5A / 5A5A5A | lines between areas, button borders, fields |
| GROUPLABEL | 707070 | 8A8A8A | ribbon group captions |
| STATUS | F0F0F0 | 2A2A2A | the status bar |
| SELECTION | C8D2F4 | 4E4E4E | selected text |
| RED, GREEN, ORANGE, YELLOW | | | icon colours with a meaning |

(The full tables are at the top of `cs_theme.c`.)

## Pens

On the first `MUIM_Setup` of any themed object, `cs_theme_setup()` calls `ObtainBestPen()` for every
token (precision `PRECISION_GUI`). On a palette screen that shares existing pens, takes free ones,
and settles for the nearest colour when the palette is full: the theme comes out approximate but
readable. The last `MUIM_Cleanup` gives them all back with `ReleasePen()`: every pen taken must go
back, or other programs run short. Switching light and dark (`cs_theme_set_dark()`) releases and
takes the pens again; each object then just redraws (Jotter's Dark Theme command calls
`cs_redraw_window()` after it).

MUI can send an object `MUIM_Setup` twice, or `MUIM_Cleanup` without `MUIM_Setup`, when groups are
hidden and shown; `cs_theme_setup()` keeps a table of the objects set up and counts each once.
Always use `cs_theme_setup()`/`cs_theme_cleanup()` in pairs (and `cs_theme_handler()` for the event
handler, which cleanup removes) rather than counting yourself.

## Fonts

The interface uses the family's own bitmap font, **QuillSans** (made from Liberation Sans by
Quill's `tools/mkamigafonts.py`; a rendering of an OFL font may not carry the original's name),
at 11 pixels (15 with large text), plus bold, a big size for headings and a small one for badges:

```c
struct TextFont *cs_font(void), *cs_bold(void), *cs_big(void), *cs_small(void);
```

They never return `NULL`: each falls back on the next, ending at the system's default font, so a
missing Fonts drawer makes the program plainer, not broken (it warns once). The fonts are loaded by
the kit's own hunk loader, not `OpenDiskFont()`: some diskfont.library versions won't open fonts
from the program's drawer, and some Kickstarts' `LoadSeg()` ignores the hunk's memory flags (the
glyphs must be in chip memory for the blitter). The loader reads `PROGDIR:Fonts/<family>.font`
(a FontContentsHeader listing the sizes), picks the nearest size, reads that size's hunk file into
chip memory, applies its relocations and calls `ExtendFont()`. Loaded sizes are shared and counted;
`cs_theme_free()` at the end frees them. `cs_open_font(name, size)` opens any family of the drawer
the same way (Jotter's view uses QuillMono 11, 14 and 17).

`kit/amiga/fonts/` holds the sizes the kit and Jotter use: QuillSans 9, 11, 12, 15, 27, 35,
QuillSansB 11, 15, QuillMono 11, 14, 17, with `LICENSE-fonts.txt`. Dialogs use QuillSans too
(`cs_dialog_font()`, through the `CS_DLG` tags) on the ribbon's colour (`cs_dialog_back()`), so they
match the main window.

## Drawing helpers

```c
LONG cs_textlen(rp, tf, s);                      /* the width of tr(s) in tf */
void cs_text(rp, tf, x, y, s, pen);              /* tr(s) with its TOP at y (not the baseline), JAM1 */
LONG cs_text_fit(rp, tf, x, y, s, maxw, pen);    /* the same, cut with "..." to maxw */
void cs_rect(rp, x0, y0, x1, y1, pen);           /* a one-pixel outline, corners inclusive */
void cs_fill(rp, x0, y0, x1, y1, pen);           /* a filled box, corners inclusive; nothing if empty */
void cs_icon(rp, icon, x, y, size, dim);         /* chapter 11 */
```

`cs_text` and `cs_textlen` call `tr()` themselves, so a class that draws a label is translated
without doing anything. Coordinates are inclusive at both ends and text is placed by its top edge:
no off-by-one or baseline confusion between classes.

## The look

- **Flat**: no 3D bevels and no MUI frames inside the main window; areas are separated by a single
  `BORDER` line. `InnerSpacing(0, 0)` and `MUIA_Group_Spacing, 0` on every group of the main window.
- **Hover** is a filled `HOVER` box, **pressed** a `PRESSED` box, **on** a `CHECKED` box with an
  `ACCENT` outline. The accent colour marks only the one thing that is on (the current tab, a ticked
  toggle, the File tab), never decoration.
- **Text** in `TEXT`, secondary in `DIM`, disabled in `DISABLED`.
- **Spacing in whole pixels derived from the font height**, so large text scales the layout.
- **Light by default** for document programs; Easel defaults to dark (pictures look right on grey).

## Help bubbles

```c
const char *cs_bubble(const char *name, const char *body);  /* "\33bName\33n\nbody wrapped at 46" */
```

A class that draws several things answers `MUIM_CreateShortHelp` for the one under the pointer
(it must have been made with `MUIA_ShortHelp, " "`, or MUI never asks). The ribbon's bubbles are
the command's name, its shortcut and its help. `cs_tips = 0` turns them off (Options).

## Palette screens

Interface colours always go through the theme's pens. Document pixels on a palette screen go
through a cube of pens and a dither (Easel's `draw_rgb()` in `widgets.c`: `WritePixelArray` on a
graphics card, an ordered or error-diffusion dither to 6x6x6 pens on an 8-bit screen, fewer on
shallower ones). Test on a 256-colour screen once per release.

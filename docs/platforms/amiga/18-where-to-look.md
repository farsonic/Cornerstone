# 18. Where to look

The best real example of each technique, in the four programs (paths relative to each program's
directory) and the kit.

| Technique | Best example | What to look at |
|---|---|---|
| The command table, menus, ribbon as data | `example/ui/commands.c` | one table, everything naming it |
| A ribbon | `kit/amiga/cs_ribbonmui.c`; `quill/amiga/ribbon.c` (with galleries, fonts list, work packages' groups) | layout, collapsing, KeyTips |
| A toolbar described as data | `folio/amiga/bars.c` | `StripItem` tables; `strip_state()` |
| A tool palette with pop-out groups | `easel/amiga/toolbox.c` | `groups[]`, the pop-out through `popup_list()` |
| An options bar with drag-to-change fields | `easel/amiga/toolbox.c` | `fields_for()`: `SCRUB`, `CHOICE`, `CHECK`, `BUTTON` |
| Tabbed panes | `kit/amiga/cs_tabs.c`; `easel/amiga/panels.c` | tabs notifying a page group |
| A navigation pane | `quill/amiga/panes.c`, `folio/amiga/panes.c` | `nav_class` |
| A text view with caret, back buffer and blitter scrolling | `example/amiga/view.c`; `quill/amiga/view.c` | rows, `cs_buf_scroll` |
| A picture view: zoom, overlays, timers | `easel/amiga/canvas.c` | composite buffers, COMPLEMENT overlays |
| A page view rendering in the background | `folio/amiga/view.c` | the timer, stopping half way, the page cache |
| A spreadsheet grid | `ledger/amiga/grid.c` | headings, selection, typing into cells |
| A ruler talking to the view | `quill/amiga/ruler.c` | `MUIM_Ruler_Set` |
| The File page | `kit/amiga/cs_backstage.c` + Jotter's `make_window`; `quill/amiga/backstage.c` | covering the window, Esc to go back |
| Context menus by tool and state | `easel/amiga/ctxmenu.c` (+ `tests/ctxtest.c`), `ctxmui.c` | the lists in plain C |
| A drop-down list window | `kit/amiga/cs_popup.c` | pushing the close and the result |
| A flat scroll bar | `kit/amiga/cs_scroll.c` | Prop's attributes |
| Dialog buttons and labels | `kit/amiga/cs_dialog.c`; `easel/amiga/qbutton.c` | `CsButton`, `CsLabel` |
| A dialog with live preview, Reset and Cancel | `easel/amiga/dialogs.c` | `slider_dialog()` |
| A colour picker | `easel/amiga/colorpick.c` | HSB, RGB, Lab and hex in step |
| Preferences with a page list | `easel/amiga/prefs.c` | pages, applying on OK |
| A keyboard shortcut editor from the menu table | `easel/amiga/keys.c` | reading `NewMenu`, saving a keys file |
| Progress with Abort | `kit/amiga/cs_jobs.c`; `easel/amiga/jobs.c` (with its undo) | the hook, Esc from the matrix |
| A splash window | `easel/amiga/splash.c` | Intuition before MUI |
| Workbench drops and iconifying | `easel/amiga/appdrop.c` | `MUIA_Window_AppWindow`, `MUIA_AppMessage` |
| The clipboard | `example/amiga/clip.c` (text); `easel/amiga/clipboard.c` (ILBM) | FTXT/CHRS through iffparse |
| Installed fonts | `quill/amiga/sysfonts.c` | scanning FONTS:, TrueType |
| Right-to-left drawing | `ledger/amiga/gmirror.h`, `core/gridgeo.c` | logical layout, mirrored drawing |
| Work packages plugging into a big program | `quill/amiga/wphooks.h` | id blocks, hooks, ribbon groups per package |
| The interface strings | `kit/shared/tools/*`; `quill/tools/mklang.py` (rows in Python files) | five languages |
| Workbench icons | `kit/amiga/tools/mkicons.py`; `easel/tools/mkicons.py` | GlowIcon writing |
| Amiga fonts from TrueType | `quill/tools/mkamigafonts.py` | the font file format, OFL renaming |
| Robustness: hostile input, allocation failure, fuzzing | `folio/tests/robusttest.c`, `allocfail.c`, `tools/fuzz.c` | |
| A non-MUI program (Intuition and RTG directly) | `farsonic/troika` | its own drawing on an RTG screen without MUI: when MUI is too much |
| Commodities and small tools | `farsonic/commodities`, `farsonic/utilities` | `mkicon.py` (classic icons), Installer scripts made by script |

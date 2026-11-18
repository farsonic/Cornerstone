# Cornerstone

Cornerstone is the style guide, reusable kit and example program for a family of applications:
Quill (words), Ledger (spreadsheets), Easel (pictures) and Folio (PDF) for AmigaOS 3.x with MUI,
each following an upstream "Craft" program the way that program follows its PC original. It writes
down how they are built (the window's anatomy, one command table behind menus, ribbon, keys,
context menus and search, the theme, the icon language, dialogs, wording, translation into five
languages, long operations, testing, packaging) so that any new program can be told "follow
Cornerstone" and come out consistent with them. It is generic: the Amiga is its reference platform,
and the classic Mac OS ports will adapt it to Apple's guidelines.

GPL-2.0-only (see `LICENSE`).

## What is in it

| Path | What |
|---|---|
| `docs/` | The guide. `docs/shared/`: the platform-neutral chapters (principles, architecture, commands, window, dialogs, wording, long operations, translation, right to left, accessibility, testing, the icon language, release, the gap method, the CLAUDE.md template). `docs/platforms/amiga/`: everything MUI and AmigaOS (toolchain, skeleton, theme, every class, menus, dialogs, jobs, Workbench icons, Installer scripts, packaging, the emulator, pitfalls, and a walkthrough of the example). `docs/platforms/classic-mac/`: System 7, after Apple's 1992 guidelines. `docs/img/`: screenshots. |
| `kit/shared/` | Portable C99, no platform calls: the command registry and search (`cs_cmd`), `tr()` in five languages (`cs_lang`), preferences, progress, wrapping and KeyTips (`cs_util`), the icon language and the shared icon set (`cs_icon`, `cs_iconset`), the ribbon as data and its layout (`cs_ribbon`); their tests; the kit's own strings; tools (`mklang.py`, `langcheck.py`, `gapcount.py`, `iconsheet.py`); templates (CLAUDE.md, CI workflows, issue forms). |
| `kit/amiga/` | The MUI kit: the theme (pens, light and dark, the QuillSans font loader, drawing, the icon renderer, bubbles), the classes (ribbon, pop-up list, flat scroll bar, tabs, status bar, pane header, File page column, Tell Me, back buffer), dialogs with Zune fallbacks, requesters, the progress window with Abort, menus and context menus from the command table, start-up; screen fonts; tools (`ribbonfit.py`, `langfit.py`, `mkicons.py`, `checkinst.py`, `srcfix.py`, `mkpackage.sh`, the FPU vbcc config). |
| `example/` | **Jotter**, a plain-text editor built only on the kit: ribbon (Home, Insert, View), File page, Outline pane, status bar, Find and Replace, Options, a context menu, five languages, its own icons, an Installer script, a package script, native tests. The guide's walkthrough builds it from nothing. |
| `tools/test.sh` | Every native test, what CI runs. |

## How a new program adopts it

1. **Copy `kit/`** into the new program's tree as `kit/`, and keep it as it is (improvements go
   back to Cornerstone, then out again).
2. **Read the guide**: `docs/shared/01-principles.md` first, then the architecture and commands
   chapters, then the platform's chapters; follow the Amiga walkthrough
   (`docs/platforms/amiga/16-walkthrough.md`) with Jotter's source open beside it.
3. **Start from Jotter's shape**: `core/` and its tests first; then `ui/commands.c` (the command
   table, menus, ribbon); then `lang/strings.txt`; then the front end (`amiga/main.c` from the
   skeleton, the program's document view as its own class).
4. **Write `cornerstone.json`** so the tools find things, copy the CI workflow and the CLAUDE.md
   template from `kit/shared/templates/`, and fill in the CLAUDE.md.
5. **Run the tools**: `tools/test.sh` with `mklang.py --check`, `langcheck.py`, the native tests,
   `ribbonfit.py` and `langfit.py`; `iconsheet.py` for new icons; `mkicons.py`, `checkinst.py` and
   `mkpackage.sh` for a release.
6. **Make a gap document** against the upstream program (`docs/shared/14-gap-method.md`) and work
   from it.

## Building and testing

    sh tools/test.sh                      # the kit's and Jotter's native tests, the checks (C compiler, Python 3 with Pillow)
    sh example/build.sh                   # Jotter's native tests, then Jotter with vbcc
    sh example/tools/package.sh 1.0 DIR   # Jotter-1.0.lha, refusing any key material

The vbcc build needs vbcc 0.9h, the NDK 3.2 headers and the MUI 3.8 developer headers (not in this
repository: they may only be redistributed whole); see `docs/platforms/amiga/01-toolchain.md`.

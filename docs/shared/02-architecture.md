# 2. Architecture and project layout

## The one rule

**The front end never computes; it only shows and asks.** Anything that can be tested without a
screen lives in `core/` as portable C99 and has a native test. The front end calls it and redraws.
This is what lets Quill run 1,800 tests, Ledger check 3,600 worksheet functions against
LibreOffice, Easel compare its codecs with Pillow and Folio fuzz its PDF parser for half a million
inputs, all on a Linux machine with the address and undefined-behaviour sanitizers, and it is what
lets a second platform reuse the whole core.

What belongs in the core, beyond the obvious (document model, file formats, undo, algorithms):

- **the command table, menu layout, ribbon definition and context-menu lists** (they are data: see
  chapter 3; Jotter's are in `example/ui/commands.c`, compiled natively by its tests);
- **what a context menu offers in a given state** (Easel's `amiga/ctxmenu.c` is plain C for that
  reason, with `tests/ctxtest.c`);
- **layout that isn't drawing**: the ribbon's layout (`kit/shared/cs_ribbon.c`), a grid's column
  positions and right-to-left mirroring (Ledger's `core/gridgeo.c`), text wrapping;
- **anything with a number in it**: counts, measurements, search.

## A program's tree

```
Program/
  core/            portable C99: the document, formats, undo, algorithms. No platform headers.
  ui/              portable data the interface is made from: commands.c (the command table, menus,
                   ribbon, context menus) - optional, but it lets tests check the interface's data
  amiga/           the MUI front end: main.c, the program's own classes, dialogs, lang_table.h
  mac/             (a classic Mac port: its front end, resources made from the same tables)
  kit/             Cornerstone's kit, copied in: kit/shared (portable) and kit/amiga (MUI)
  lang/            strings.txt (every interface string in five languages), ignore.txt
  tests/           native tests of core/ and ui/
  tools/           test.sh (what CI runs), package.sh, the Installer script, wbicons.py
  docs/            the gap document against the upstream program, anything else long-lived
  cornerstone.json where the kit's tools find things (strings, sources, fonts, ribbon)
  build.sh         native tests first, then the platform build; "sh build.sh fast" skips the tests
  CLAUDE.md        the house rules and where work stands (template: chapter 15)
  README.md        what the program does, version by version
  LICENSE          GPL-2.0-only
```

Quill, Ledger, Easel and Folio grew before the kit existed and keep their files in `amiga/`
under their own names (`theme.c`, `qbutton.c`, `widgets.c`, `ribbon.c`); the kit is those files
generalised. A new program starts from the kit; an existing one may move to it piece by piece.

## The kit

```
kit/shared/          portable C99 and tools: no platform calls
  cs_cmd.[ch]        the command registry, the command search, the menu layout check   (chapter 3)
  cs_lang.[ch]       tr(), tr_list(), the five languages                               (chapter 8)
  cs_util.[ch]       preferences file, progress hook, wrapping, KeyTips codes, muldiv  (chapters 7, 10)
  cs_icon.h, cs_iconset.c  the icon language's interpreter and the shared icon set     (chapter 12)
  cs_ribbon.[ch]     the ribbon as data and its layout (collapsing, KeyTips, hit test)  (chapter 4)
  lang/              the kit's own strings in five languages
  tests/kittest.c    the kit's native tests
  tools/             mklang.py, langcheck.py, gapcount.py, iconsheet.py, csconfig.py
  templates/         CI workflows, issue templates, the CLAUDE.md template
kit/amiga/           the MUI front-end kit: theme, classes, dialogs, jobs, menus, start-up
  fonts/             QuillSans and QuillMono screen fonts (SIL OFL renderings)
  tools/             ribbonfit.py, langfit.py, fit.c, mkicons.py, amifont.py, checkinst.py,
                     srcfix.py, mkpackage.sh, vbcc-aos68k-fpu
```

A program copies `kit/` into its tree and keeps it as it is; improvements go back to Cornerstone
and are copied out again. If a program must change a kit file, it says so at the top of the file
and in its CLAUDE.md, so the change can be carried back.

## cornerstone.json

One small file tells the tools where the program keeps things (`kit/shared/tools/csconfig.py`
documents every key). Jotter's:

```json
{
  "program": "Jotter",
  "strings": ["lang/strings.txt", "../kit/shared/lang/kit-strings.txt"],
  "ignore": ["lang/ignore.txt", "../kit/shared/lang/kit-ignore.txt"],
  "table": "amiga/lang_table.h",
  "sources": ["amiga/*.c", "amiga/*.h", "ui/*.c", "ui/*.h", "../kit/amiga/*.c", "../kit/shared/cs_*.c"],
  "exclude": ["amiga/lang_table.h"],
  "fonts": "../kit/amiga/fonts",
  "fit": { "sources": ["ui/commands.c"], "init": "jotter_commands_init", "ribbon": "jotter_ribbon",
           "menus": "jotter_menus", "width": 618 },
  "gap": "docs/notepad-gap.md"
}
```

(Jotter lives beside the kit in this repository, hence `../kit`; in a program of its own the kit
is at `kit/`.)

## Builds and CI

- `tools/test.sh` runs everything that needs only a C compiler and Python 3 with Pillow: the
  interface-string checks, the native tests with ASan and UBSan, the fit checks. CI (GitHub
  Actions, `kit/shared/templates/workflows/tests.yml`) runs exactly this on every push.
- `build.sh` runs `tools/test.sh`, then the platform build. Platform builds need toolchains
  GitHub's runners don't have; they run on the build machine (the platform chapter says where).
- Heavy builds on shared machines run under a lock (`flock`) and with at most two jobs.

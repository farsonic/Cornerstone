# CLAUDE.md - working on PROGRAM

PROGRAM is a WHAT-IT-IS for AmigaOS 3.x (CPU, MUI 3.8, AROS or MorphOS), written in C and laid
out the way UPSTREAM (a PC-ORIGINAL-like program) is. It is built on Cornerstone (`kit/`, its guide
in Cornerstone's `docs/`). GPL-2.0-only (see `LICENSE`). This file is for anyone, person or agent,
picking the work up. Read it first, then `README.md` (what PROGRAM does) and
`docs/UPSTREAM-gap.md` (what is still missing).

## Layout

| Path | What |
|---|---|
| `core/` | Portable C99: the document, ... No AmigaOS calls: built and tested on Linux/macOS. |
| `ui/` | The command table, menus, ribbon and context menus (`commands.c`): portable data the tests check. |
| `amiga/` | The MUI front end: `main.c`, the program's own classes, dialogs, `lang_table.h` (generated). |
| `kit/` | Cornerstone's kit, copied in unchanged (`kit/shared` portable, `kit/amiga` MUI). Changes go back to Cornerstone. |
| `lang/` | `strings.txt`: every interface string in English, German, French, Spanish and Italian (UPSTREAM-TERMS's terms); `ignore.txt`. |
| `tests/` | Native tests (`coretest.c`, ...). |
| `tools/` | `test.sh` (what CI runs), `package.sh`, `Install-PROGRAM`, `wbicons.py`. |
| `docs/` | `UPSTREAM-gap.md`: every UPSTREAM command with PROGRAM's status and evidence. |

## Build and test

Native tests (need a C compiler and Python 3 with Pillow; CI runs them and they must stay green):

    sh tools/test.sh            # TMP=<dir> to build somewhere other than /tmp

Expected: `N tests, 0 failed`, the language table up to date, langcheck with nothing missing,
ribbonfit and langfit passing.

Amiga binary: `./build.sh` (`./build.sh fast` skips the tests). It needs, at these default paths (or
set `VBCC`, `NDK`, `MUI`):

- **vbcc** 0.9h with vasm and vlink and the m68k-amigaos target, in `/opt/amiga/vbcc`
  (http://www.compilers.de/vbcc.html) (with `-lm040` and `kit/amiga/tools/vbcc-aos68k-fpu` for an
  FPU build).
- **NDK 3.2** C headers in `/opt/amiga/ndk/Include_H` (Aminet `dev/misc/NDK3.2.lha`).
- **MUI 3.8 developer headers** in `/opt/amiga/mui/include` (Aminet `dev/mui/mui38dev.lha`, its
  `C/Include`; not in this repository: it may only be redistributed whole).

Release: `sh tools/package.sh <version> <workdir>` builds `PROGRAM-<version>.lha` and refuses to
finish if any certificate or key material went in. Packages are not committed; attach them to
GitHub releases.

Testing on an Amiga: FS-UAE with the AROS ROM and Zune covers most checks (shared: take the lock,
restore the startup-sequence at once, move your test drawer aside afterwards); AmigaOS 3.x with
MUI 3.8 is the reference. `PROGRAM file CMD=n DEBUG=log` opens any dialog by its command number.

## Rules of the project

- C99 that vbcc compiles (watch for: `memchr` with bytes > 0x7F passed as int, `%.*s` with
  precision 0 printing everything, rare `-O2` miscompiles: rewrite the loop; `floor`/`ceil` past
  2^31 with an FPU).
- Every command in the command table (`ui/commands.c`) and in the menus; the ribbon, keys, context
  menus and Tell Me only name commands from it.
- The window fits 640x480; long jobs show progress with Abort; memory is checked before big
  allocations and the user is told how much is needed.
- Every interface string goes through `tr()` and has a row in `lang/strings.txt` with all four
  translations: `python3 kit/shared/tools/langcheck.py --rows` prints the missing ones, then
  `python3 kit/shared/tools/mklang.py`; `tools/test.sh` fails while one is missing.
- PROGRAM's own icons, in the icon language; no copied logos.
- A new feature gets a native test, its line in `docs/UPSTREAM-gap.md` updated with evidence (then
  `python3 kit/shared/tools/gapcount.py docs/UPSTREAM-gap.md --write`), and a README note.
- PROGRAM leaves out on purpose: printing to printers, ARexx and other scripting, network features.
- Don't use `rm -rf` in scripts or by hand on shared machines; move things aside.
- Commit identity: `Claude <noreply@anthropic.com>` or `PROGRAM <program@localhost>`. Never put a
  personal email address in commits, code or docs.

## Where work stands

(What is released, what is merged, what is on branches, what was and wasn't checked on an Amiga,
what comes next. Keep it current: it is the first thing the next person reads.)

## Issues

Bugs and requests come in as GitHub issues (templates in `.github/ISSUE_TEMPLATE/`). To work on them:

1. `gh issue list --repo OWNER/PROGRAM` and `gh issue view N --repo OWNER/PROGRAM --comments`.
2. Reproduce first. Document bugs can usually be reproduced natively: load the attached file with
   the core, write a failing test in `tests/`, then fix it in `core/`. Interface bugs need the Amiga
   build; say what you could and couldn't verify.
3. Fix on a branch named `issue-N-short-name`, keep `tools/test.sh` green, and open a pull request
   whose description says `Fixes #N`, what changed and how it was tested. Ask on the issue when
   information is missing rather than guessing. Don't close issues by hand unless asked; merging the
   PR closes them.
4. Attached files may be private: never commit them; make a small test file of your own instead.

`.github/workflows/claude.yml` lets people mention `@claude` in an issue or pull request to have
Claude Code look at it (it needs the `ANTHROPIC_API_KEY` repository secret or the Claude GitHub App).

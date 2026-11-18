# CLAUDE.md - working on Cornerstone

Cornerstone is the style guide, reusable kit and example program for the family of applications
built like Quill, Ledger, Easel and Folio (AmigaOS 3.x, MUI 3.8 or Zune; classic Mac OS ports to
come). A new program is told "follow Cornerstone": it copies `kit/`, follows `docs/` and runs the
tools. GPL-2.0-only (see `LICENSE`). This file is for anyone, person or agent, picking the work up.
Read it first, then `README.md` and `docs/README.md`.

## Layout

| Path | What |
|---|---|
| `docs/shared/` | The platform-neutral chapters (01 principles to 15 the CLAUDE.md template). |
| `docs/platforms/amiga/` | The MUI/AmigaOS chapters (01 toolchain to 18 where to look; 16 is Jotter's walkthrough). |
| `docs/platforms/classic-mac/` | System 7 after Apple's 1992 HIG (with page references), a plan for the first port, an appendix on modern macOS. |
| `docs/img/` | Screenshots and generated pictures (`icon-sheet.png` from `iconsheet.py`; `jotter/` from the emulator). |
| `kit/shared/` | Portable C99 (`cs_cmd`, `cs_lang`, `cs_util`, `cs_icon`/`cs_iconset`, `cs_ribbon`), `tests/kittest.c`, `lang/` (the kit's strings), `tools/` (mklang, langcheck, gapcount, iconsheet, csconfig), `templates/`. |
| `kit/amiga/` | The MUI kit (`cs_theme`, `cs_ribbonmui`, `cs_popup`, `cs_scroll`, `cs_tabs`, `cs_status`, `cs_pane`, `cs_backstage`, `cs_tellme`, `cs_buffer`, `cs_dialog`, `cs_jobs`, `cs_menu`, `cs_ctx`, `cs_app`), `fonts/`, `tools/` (ribbonfit + fit.c, langfit, amifont, mkicons, checkinst, srcfix, mkpackage.sh, vbcc-aos68k-fpu). |
| `example/` | Jotter, the example program: `core/`, `ui/`, `amiga/`, `lang/`, `tests/`, `tools/`, `sample/`, `build.sh`, `cornerstone.json`. |
| `tools/test.sh` | All native tests (what CI runs). |

## Build and test

    sh tools/test.sh            # kit tests, icon sheet, Installer check, Jotter's tests and fit checks, Workbench icons

Expected: `76 tests, 0 failed` (kit), `66 tests, 0 failed` (Jotter), langcheck clean, ribbonfit and
langfit passing, `all native tests passed`. Needs a C compiler and Python 3 with Pillow.

Jotter for the Amiga: `sh example/build.sh` (`fast` skips the tests; `OUT=dir` for the binary,
default `$TMP/jotter-build`). Needs vbcc 0.9h in `/opt/amiga/vbcc`, NDK 3.2 headers in
`/opt/amiga/ndk/Include_H`, MUI 3.8 developer headers in `/opt/amiga/mui/include` (`VBCC`, `NDK`,
`MUI`). Package: `sh example/tools/package.sh 1.0 <workdir>` (needs `jlha` or `lha`).

Emulator: `example/tools/emucheck.sh` (FS-UAE, AROS ROM, Zune; shared: it takes
`/tmp/lagoon-emu.lock`, restores the startup-sequence, moves its drawer aside; see
`docs/platforms/amiga/12-testing.md`).

## Rules of the project

- The kit is generic: no program's names, ids or strings in `kit/`. Program-specific things belong
  in `example/` or in the programs. Prefixes: `cs_` for functions, `Cs` for types and objects,
  `CS_` for constants, `CS_TAG(n)` for MUI tags.
- `kit/shared` stays portable C99 with no platform calls and is tested natively (ASan/UBSan). The
  Amiga kit compiles with vbcc without warnings; check with `sh example/build.sh fast`.
- Every interface string in the kit has a row in `kit/shared/lang/kit-strings.txt` in all five
  languages; Jotter's in `example/lang/strings.txt`. `langcheck.py` must stay clean.
- The guide and the code agree: a change to the kit updates its chapter (and Jotter, if Jotter uses
  it). Chapters say what was and wasn't checked on real hardware.
- British spelling and plain words in docs and user text. Page references for the 1992 HIG; never
  copy Apple's text or figures.
- Don't include the MUI SDK headers (redistribution only whole) or the NativeCall SDK. No
  certificate or key material anywhere; packages refuse it.
- Shared machine: heavy builds under `flock /tmp/claude-0/easel-test.lock`, at most two jobs; build
  under `$TMP`; don't `rm -rf` (move aside); the emulator only under its lock.
- Commit identity: `Claude <noreply@anthropic.com>`. Never put a personal email address in
  commits, code or docs.

## Where work stands

Made in November 2026 (first version): the guide (15 shared chapters, 18 Amiga chapters, the
classic Mac chapter), the kit, Jotter. Jotter's native tests pass, it links with vbcc, its package
builds, and it runs in FS-UAE with AROS/Zune (ribbon, KeyTips, File page, Tell Me, typing, Replace,
Options, German, dark, right to left: `docs/platforms/amiga/16-walkthrough-emulator.md`, with what
the runs found and fixed: no Balance class on Zune, Esc sending a close request, `OM_GET` for
reported attributes).

Next:
- Not yet checked on real AmigaOS 3.x with MUI 3.8 (only AROS/Zune in FS-UAE).
- The programs (Quill, Ledger, Easel, Folio) still use their own copies of what the kit
  generalises; moving them onto the kit is per program, piece by piece.
- The kit lacks (from the family): the toolbox, options bar, colour well and sign pad classes
  (documented in `docs/platforms/amiga/04-classes.md` with where they live), a splash window,
  AppWindow drops, galleries in the ribbon.
- For the classic Mac ports (see that chapter's last section): per-platform command letters, alert
  levels for `cs_ask`, Mac Roman in `mklang.py`, a resource generator, a QuickDraw icon renderer.

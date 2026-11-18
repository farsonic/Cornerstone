# 13. Packaging and the release checklist

## What a package holds

One archive per release (`.lha` on the Amiga; a StuffIt or BinHex'd disk image on the classic
Mac), attached to the GitHub release, never committed. Unpacked, a drawer named after the
program holds:

- the program;
- `README.md` (what it does, version by version) and `LICENSE` (GPL-2.0-only);
- the Installer script (or the Mac's installer);
- the fonts it needs, with their licence (`Fonts/`, `LICENSE-fonts.txt`);
- example documents;
- `Source/`: the whole source with the kit it is built on, so the GPL's offer is met in the
  archive itself;
- an icon for each of them;
- `Licenses.txt` crediting every piece of third-party code or font, where there is any (Easel's
  lists dav1d's BSD licence and its changes).

## The key-material refusal

Build machines are shared and hold a test-only TLS certificate and other secrets that must never
ship. Every packaging script **searches what it is about to pack, and the finished archive again,
for certificate or key material, and refuses** (the archive is renamed `.refused`) if it finds
any: text matching `BEGIN CERTIFICATE`, `PRIVATE KEY` or the sandbox's marker, and files named
`*.crt`, `*.pem`, `*.key`, `*.p12`, `*.pfx`. `kit/amiga/tools/mkpackage.sh` does both; its own
patterns are written so that they don't match themselves (it ships in `Source/`). Tests that need
keys (Folio's signing) make them at test time in `$TMP`, never in the repository.

## The release checklist

1. **Everything merged**; `tools/test.sh` green; the platform build links (`build.sh`).
2. **The gap document recounted** (`gapcount.py --write`) and its summary current.
3. **Every string translated**: `langcheck.py` clean; `ribbonfit.py` and `langfit.py` pass.
4. **Checked on the platform**: in the emulator, every window and dialog added since the last
   release at the smallest screen, in English and German, light and dark, and on a screen of few
   colours; then on real hardware (the reference: AmigaOS 3.x with MUI 3.8; the emulator's AROS
   and Zune differ, see the Amiga pitfalls). Write down what was and wasn't checked.
5. **Speed on the slowest supported machine** for anything new and heavy (a 68040 at 25 MHz; a
   68030 Mac).
6. **README**: a "New in x.y" section in plain words.
7. **Version strings**: the program's `$VER:` (Amiga) or `vers` resource (Mac), the Installer
   script's, the About box, the README.
8. **Workbench / Finder icons** made (`mkicons.py`); the Installer script checked (`checkinst.py`).
9. **Package** with the refusal; unpack it somewhere else and look: every file there, every icon
   with its picture, the Installer script runs (on AmigaOS, InstallerLG and AROS for the Amiga).
10. **Tag** the release, attach the archive, and update CLAUDE.md's "Where work stands".

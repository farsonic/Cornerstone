# The Cornerstone guide

How the family's programs are built, so that a new one can be told "follow Cornerstone" and come
out consistent. Plain words, British spelling, every rule with its reason. Shared chapters hold
for every platform; platform chapters say how, with the code.

## Shared (every platform)

1. [Principles and ground rules](shared/01-principles.md)
2. [Architecture and project layout](shared/02-architecture.md)
3. [Commands: one table drives everything](shared/03-commands.md)
4. [The main window](shared/04-window.md)
5. [Dialogs and requesters](shared/05-dialogs.md)
6. [Wording](shared/06-wording.md)
7. [Long operations: progress, Abort, memory](shared/07-long-operations.md)
8. [Text and translation](shared/08-translation.md)
9. [Right to left](shared/09-right-to-left.md)
10. [Accessibility and the keyboard](shared/10-accessibility.md)
11. [Testing](shared/11-testing.md)
12. [The icon language](shared/12-icons.md)
13. [Packaging and the release checklist](shared/13-release.md)
14. [Following an upstream program: the gap document](shared/14-gap-method.md)
15. [The CLAUDE.md template](shared/15-claude-md-template.md)

## Platforms

- [Amiga](platforms/amiga/README.md): AmigaOS 3.x with MUI 3.8, AROS and MorphOS with Zune. The
  reference platform: 18 chapters, the kit's code, and the walkthrough of Jotter, the example program.
- [Classic Mac OS](platforms/classic-mac/README.md): System 7, after Apple's 1992 Human Interface
  Guidelines. Where the Mac ports start; with [a short appendix on modern macOS](platforms/classic-mac/modern-macos.md).

## Pictures

`img/` holds the screenshots and generated pictures the chapters use: the four programs and their
labelled classes, Quill's emulator checks (German ribbon at 640 pixels, 1024 x 768, Zune
fallbacks, right-to-left text), Ledger's right-to-left sheet, the icon sheet (made by
`kit/shared/tools/iconsheet.py`), Jotter's Workbench icons and its emulator screenshots.

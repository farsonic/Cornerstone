# 1. Principles and ground rules

Cornerstone is how a family of programs is built: Quill (words), Ledger (spreadsheets), Easel
(pictures) and Folio (PDF) on the Amiga, each following an upstream "Craft" program (WordCraft,
GridCraft, PhotoCraft, PDFCraft) the way that program follows its PC original. This chapter holds
for every program and every platform. Platform chapters (`docs/platforms/`) say how each rule is
kept there; where a platform's own guidelines say otherwise (Apple's, for the classic Mac), the
platform chapter says which wins and why.

## The rules

| Rule | Why |
|---|---|
| **Every command is in the menus.** Ribbons, toolbars, palettes, panes and context menus are quicker ways to menu commands, never the only way. | Users expect the menu bar (the Amiga's right mouse button, the Mac's menu bar). It gives each command one number, one shortcut, one place to be enabled or disabled, and one entry for the command search. |
| **One table of commands drives everything** (chapter 3). | A tool, its menu item, its shortcut and its search entry can't disagree. |
| **Every icon is our own**, drawn in code from the icon language (chapter 12). Nothing is copied or traced from another product; no Microsoft, Adobe or Apple icon. | Licensing, and one look across the family. |
| **The interface fits the smallest screen it may meet** and adapts to bigger ones. On the Amiga that is 640 x 480 (and 640 x 512 PAL); on the classic Mac 512 x 342 for small tools, 640 x 480 for anything with a document. | Plenty of real machines have those screens. A ribbon that doesn't fit collapses its groups (chapter 4); a dialog that doesn't fit is split. |
| **Every string the user sees goes through translation** (`tr()`), with all five languages (English, German, French, Spanish, Italian) present before a release (chapter 8). | Translation done late is never done. Missing rows fail the tests. |
| **Long operations show progress and can be stopped** (chapter 7). Nothing blocks the interface for more than half a second without saying so. | A 68040 or a 68030 Mac takes seconds to minutes over what a PC does at once. |
| **Memory is checked before big allocations**, and when it runs short the user is told how much is needed and how much there is. | Machines have 8 to 64 MB; failing quietly loses work. |
| **No network features, no printing to real printers, no scripting** (ARexx, AppleScript, Apple events beyond the required four) unless the program is about them. PDF export takes the place of printing. | Scope: none has been asked for, and each is a large surface to test. A program that does one says so in its README. |
| **Colours are tokens, not RGB values** (chapter 4, and the platform's theme). The program works in a light and a dark theme and on a screen of few colours. | AGA and 8-bit screens are common; so are users who want dark. |
| **Text is plain and kind** (chapter 6): say what happens in the user's words, give the reason and what to do. British spelling ("colour", "centre", "grey") in user text, except command names users search for in the upstream program. | The family's voice. |
| **The program's logic is portable C**, tested natively with the sanitizers; the front end only shows and asks (chapter 2). | The core can be tested on any computer, fuzzed, and shared between platforms. |
| **Code reads like the code around it**: C99 the oldest compiler accepts, a comment above each function saying what it does in ordinary words. | Others, and later sessions, maintain it. |
| **Nothing is deleted on shared machines**: no `rm -rf` in scripts; move things aside. Packages refuse key material (chapter 13). | Build machines are shared and hold secrets that must never ship. |
| **No personal email address** in commits, code or documents. | Commits use `Claude <noreply@anthropic.com>` or `<Program> <program@localhost>`. |

## What "following the upstream program" means

The upstream program decides **what** there is and **what it is called**: its tabs, groups,
commands and their names, its dialogs' contents, its terms in each language (taken from the PC
program it follows: Word's, Excel's, Photoshop's, Acrobat's German, French, Spanish and Italian).
Users coming from it, and its manuals and tutorials, then apply. Cornerstone decides **how** it is
built and **how it looks**: the window's anatomy, the theme, the icons, the dialogs' layout, the
words of help and messages. The gap document (chapter 14) records, command by command, how far a
program has followed.

## Measurements

Shared chapters give sizes in **fh**, the interface font's line height (11 pixels for QuillSans 11
on the Amiga, 12 for Geneva 9 on the classic Mac, about 16 for a modern system font), and in
grid units for icons (a sixteenth of the icon's size). A platform chapter turns them into pixels.
A layout written in fh grows by itself when the user asks for large text.

## The chapters

Shared (this directory): [2 architecture](02-architecture.md) · [3 commands](03-commands.md) ·
[4 window](04-window.md) · [5 dialogs](05-dialogs.md) · [6 wording](06-wording.md) ·
[7 long operations](07-long-operations.md) · [8 translation](08-translation.md) ·
[9 right to left](09-right-to-left.md) · [10 accessibility and keyboard](10-accessibility.md) ·
[11 testing](11-testing.md) · [12 the icon language](12-icons.md) · [13 release](13-release.md) ·
[14 the gap method](14-gap-method.md) · [15 CLAUDE.md template](15-claude-md-template.md).

Platforms: [Amiga](../platforms/amiga/README.md) (the reference: every program so far) ·
[classic Mac OS](../platforms/classic-mac/README.md) (where the ports start).

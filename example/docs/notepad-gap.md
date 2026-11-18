# Jotter against a plain-text editor's usual commands

A small gap document, to show the method (`docs/shared/14-gap-method.md`): Jotter has no upstream
"Craft" program, so it is compared with what the usual plain-text editors of PCs offer (their File,
Edit, Format and View menus). Every ✅ names the command id (`ui/commands.h`) or function that does
it; a ⬜ was looked for in the source and not found. Recount with
`python3 ../kit/shared/tools/gapcount.py docs/notepad-gap.md --write`.

Marks: ✅ Jotter has it · 🟨 partly (what is missing is said) · ⬜ missing · ➖ left out on purpose.

## Summary

- **Coverage:** 30 items, 27 of them applicable: **23** done, **2** partly, **2** missing; 3 left out on purpose. 85% of what applies is done, 93% at least in part.

| Section | Items | ✅ | 🟨 | ⬜ | ➖ |
|---|---|---|---|---|---|
| File | 9 | 7 | 0 | 0 | 2 |
| Edit | 13 | 11 | 1 | 1 | 0 |
| Format | 3 | 1 | 1 | 0 | 1 |
| View | 5 | 4 | 0 | 1 | 0 |
| **All** | **30** | **23** | **2** | **2** | **3** |

## 1. The commands

### File (9: ✅ 7 · 🟨 0 · ⬜ 0 · ➖ 2)
- ✅ New: `ID_NEW`, `text_init`
- ✅ Open: `ID_OPEN`, `text_load` (Latin-1; UTF-8 read and converted)
- ✅ Save: `ID_SAVE`, `text_save` (line ends kept)
- ✅ Save As: `ID_SAVEAS`
- ✅ Revert: `ID_REVERT`
- ✅ Document information: `ID_INFO` (the File page's Info: lines, words, characters, paragraphs, line ends)
- ✅ Quit, asking to save changes: `ID_QUIT`, `may_discard`
- ➖ Page Setup: printing is left out (shared chapter 1)
- ➖ Print: printing is left out

### Edit (13: ✅ 11 · 🟨 1 · ⬜ 1 · ➖ 0)
- ✅ Undo: `ID_UNDO`, `text_undo` (without limit; typing joined into steps)
- ✅ Redo: `ID_REDO`, `text_redo`
- ✅ Cut, Copy, Paste: `ID_CUT`, `ID_COPY`, `ID_PASTE`, `clip.c` (the Amiga clipboard, FTXT)
- ✅ Delete: `ID_DELETE`
- ✅ Select All: `ID_SELECTALL`
- ✅ Find: `ID_FIND`, `text_find` (match case, whole words, up or down)
- ✅ Find Next: `ID_FINDNEXT`
- ✅ Find Previous: `ID_FINDPREV`
- ✅ Replace: `ID_REPLACE`, `text_replace`, `text_replace_all` (Replace All one undo step)
- ✅ Go To line: `ID_GOTO`
- ✅ Time and date: `ID_DATE`, `ID_TIME`
- 🟨 Change case: `ID_UPPER`, `ID_LOWER`; no Title Case or Toggle Case
- ⬜ Search with wildcards or patterns

### Format (3: ✅ 1 · 🟨 1 · ⬜ 0 · ➖ 1)
- ✅ Word Wrap: `ID_WRAP` (on screen only)
- 🟨 Font: Text Size (`ID_TEXTSIZE`: three sizes of QuillMono); no choice of font
- ➖ Bold, italic and the like: Jotter is a plain-text editor

### View (5: ✅ 4 · 🟨 0 · ⬜ 1 · ➖ 0)
- ✅ Status bar: `ID_STATUSBAR` (state, text size, words, line and column)
- ✅ Zoom (text size): `ID_TEXTSIZE`
- ✅ An outline of headings: `ID_PANE` (the Outline pane, lines starting with #)
- ✅ Dark theme: `ID_DARK`
- ⬜ Line numbers in the margin

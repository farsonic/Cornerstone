# 17. Pitfalls

What the family has hit, so the next program doesn't.

## vbcc

| Symptom | Cause | Fix |
|---|---|---|
| `memchr` misses a byte | vbcc's `memchr` misses bytes above 0x7F passed as an int | a loop, or cast the byte `(char)` |
| `printf("%.*s", 0, s)` prints all of s | precision 0 ignored | test for 0 first |
| Right natively and at -O1, wrong on the Amiga at -O2 (Easel's text: every line on the first baseline; Quill: invalid `lea dN,aN`) | optimiser bugs | -O1, or rewrite the loop / make the variable `volatile` with a comment why; test new core code in the emulator |
| `floor`/`ceil` wrong past 2,147,483,647 with an FPU | vbcc's inline versions go through 32-bit integers | the program's own (Ledger has them) |
| Numbers formatted differently from glibc | the Amiga C library's `printf` | test number formatting on the Amiga build (Ledger's FIXED and DOLLAR) |
| AROS compares large negative doubles wrongly | its mathieeedoubbas.library | avoid relying on it; test on AmigaOS too |
| A type called `Text` (or `Draw`, `Move`) breaks compiling | graphics.library's calls are macros or protos of those names | name types differently (`TextDoc`) |
| `ID_BASE + n` cast wrongly in a macro | cast precedence | `(APTR)(id)` |
| Garbage from `sprintf` inside a hook on Zune | the stack two bytes off a long word | Quill's `hook.s` realigns it |
| `MAKE_ID` undefined | it lives in `libraries/iffparse.h` | include it |
| A 4 KB stack overflows | the Shell's default | `long __stack = 32768;`, or Easel's `bigstack.s` |

## MUI and Zune

| Symptom | Cause | Fix |
|---|---|---|
| Grey rectangles left after a requester, dialog or pop-up closes | MUI doesn't always repair what a closing window uncovered over custom-drawn objects | `cs_redraw_window()` (`MUIM_Group_InitChange`/`ExitChange` on the root); `cs_run_modal`, `cs_ask` and the pop-up do it |
| A crash when a pop-up closes itself from its own click | closing a window inside its own event | `MUIM_Application_PushMethod` the close and the result |
| Pens run out; other programs' colours go wrong | pens obtained and not released, or released twice | only `cs_theme_setup`/`cs_theme_cleanup`, which count objects |
| An object draws over another pane | drawing while hidden | the `shown` flag from Show/Hide, checked before every redraw |
| A class never gets bubble requests | no `MUIA_ShortHelp` | `MUIA_ShortHelp, " "` |
| The document view flickers | Area clears the background before Draw | `MUIA_FillArea, FALSE` |
| Dark-theme dialogs: invisible labels | MUI's labels are black | `CsLabel`, `CsTitle` |
| Setting a gadget triggers your own handler | `set()` fires notifications | `nnset()` |
| A dialog never opens on Zune | no Radio class (some Zune versions) | `cs_radio()` falls back to a cycle gadget |
| The main window can't be made on Zune; nothing says why | no Balance class there; any NULL child fails the whole window | `cs_balance()`; log each part's pointer (`DEBUG=`) as Jotter's `make_window` does |
| Esc quits the program on Zune | Esc sends the window a close request as well as its "esc" input event (Zune; MUI with Esc set to close windows) | take the close request that follows an Esc as Esc (Jotter's `esc_just`); dialogs map both to Cancel and ignore the leftover |
| A long dialog doesn't fit on Zune | no Scrollgroup/Virtgroup | two to a row, or pages |
| Frame titles faint on Zune | Zune's look | check on MUI 3.8 |
| A window too tall doesn't open at all | MUI refuses it silently | keep to 480 lines; `cs_run_modal` logs it |
| Fonts don't load from the program's drawer on some systems | diskfont.library versions | the kit's own hunk loader, plus `AssignAdd FONTS: PROGDIR:Fonts` |
| `tr_list` crash | a list without its NULL | every list ends with NULL; `tr_list` returns an unterminated one untranslated |
| An icon shows the wrong picture, and so does every one after it | a line missing from the icon array | one line per icon in enum order, a comment each; `cs_icon_check` |
| Two menus' ids collide after adding items | a block ran into the next | fixed block starts; a test of the last id of each block |
| Items past the 63rd vanish from a menu | Intuition's limit | `cs_menu_check`; submenus |
| A dialog's preview is slow while dragging | rendering on every notification | note it, render on idle |
| Alt shortcuts can't be tested | Alt doesn't reach the emulated Amiga | F10 for KeyTips; test natively |
| The package refused itself | the refusal's patterns matched its own source | patterns that don't match themselves |

# 10. The keyboard on the Amiga

- **Menu shortcuts** are Intuition's: the right Amiga key with the command's letter. Intuition turns
  them into menu picks before any class sees a key; a class ignores raw keys with an Amiga
  qualifier (Jotter's view: `if (q & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) return 0;`).
- **Raw keys in a class**: ask for `IDCMP_RAWKEY` in the event handler; turn a key into a character
  with keymap.library's `MapRawKey()` (pass the previous dead key: `ie_EventAddress =
  *(APTR *)im->IAddress`), so the user's keymap and dead keys work. Cursor keys are 0x4C-0x4F, Return
  0x44, Enter 0x43, Backspace 0x41, Del 0x46, Tab 0x42, Esc 0x45, Help 0x5F, F1-F10 0x50-0x59; PC
  keyboards' Home, End, Page Up and Down arrive as 0x70, 0x71, 0x48, 0x49; NewMouse wheels as 0x7A
  and 0x7B. Shift selects, Alt moves by words (and to the ends with up and down).
- **KeyTips**: the ribbon's event handler, at priority 1, sees keys first. Alt is noticed when its
  key-up follows its own key-down with nothing between (0x64/0x65, then the same with
  `IECODE_UP_PREFIX`); F10 (0x59) works too. While KeyTips are shown it eats keys; otherwise it lets
  them through to the view.
- **Dialogs**: MUI's cycle chain (`MUIA_CycleChain, 1` on every gadget; the kit's do it), Tab to move,
  each button's `MUIA_ControlChar`, `MUIA_Window_DefaultObject` for Return, `"esc"` as an input event
  for Cancel.
- **The emulator**: in the FS-UAE setup used so far, **Alt doesn't reach the emulated Amiga**.
  Anything that needs Alt must also work another way (F10 for KeyTips) and be checked natively;
  F-keys, letters and Esc go through `xdotool key` reliably.

# 10. Accessibility and the keyboard

The family's programs must be usable without a mouse, at a larger text size, in a dark theme and
without colour.

## Everything from the keyboard

- **Menus** carry every command (chapter 3) and the most used ones have Amiga-key (Command-key)
  shortcuts, shown in the menu.
- **KeyTips**: Alt pressed and released alone, or F10, shows a letter (or two) on the File tab,
  every tab, the Quick Access buttons and the Tell Me box; typing a tab's letter shows that tab
  with a letter on every tool, and typing a tool's letter presses it (a list opens, a collapsed
  group shows its tools). Esc goes back a level. The letters are assigned from the names in the
  language shown (`cs_keytips_assign()`: a name's first letter if no other has it, else the first
  and a later letter of its own, else a digit), so they follow translation. The Tell Me box is Q
  where no tab has Q; Quick Access buttons are numbered.
- **Tell Me** finds any command by typing a few words of its name or help.
- **Dialogs**: every gadget in the cycle chain (Tab moves between them), every button and label
  with a keyboard letter, Return for the default, Esc for Cancel.
- **The document view** takes the cursor keys (Shift selects, Alt moves by words and to the ends),
  Home, End, Page Up and Down, and the mouse wheel.
- **Pop-up lists** take cursor up and down, Return and Esc.
- Some emulator setups don't pass Alt to the Amiga: anything reached with Alt must also be
  reachable another way (F10 for KeyTips), and checked natively.

## Seeing

- **Large text**: an option that makes the interface font 15 pixels instead of 11 (every size is
  derived from the font height, chapter 4), taking effect when the program starts again.
- **Dark theme**: every colour is a token with a light and a dark value; switching takes the pens
  again and redraws.
- **Don't rely on colour alone**: a disabled icon is one grey shape and must still be recognised
  by its outline; a checked toggle has an outline as well as a fill; two icons that differ only in
  colour are two icons too many.
- **Contrast**: text on the accent colour uses the "on accent" token (white in the light theme,
  near-black in the dark one), not a colour picked by eye.
- **Help bubbles** say what each thing is and does; they can be turned off.

## Upstream accessibility features

Where the upstream program has accessibility tools (Quill has Read Aloud through
narrator.device, an accessibility checker and alt text for pictures; Folio checks PDF tagging),
they are commands like any other, in the gap document.

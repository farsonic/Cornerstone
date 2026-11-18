# 6. Wording

The words matter as much as the drawing. The family's voice is plain, kind and exact.

- **Name commands as the upstream program does**, so its manuals, tutorials and users' habits
  apply: "Save As...", "Track Changes", "Color Range" (a command users search for keeps its
  upstream spelling even in British text).
- **Say what happens, in the user's words.** "Abort puts the picture back as it was", not
  "Operation cancelled, state restored". "Breaks long lines at the window's edge", not "Enables
  soft wrap".
- **Messages give the reason and what to do.** "Jotter couldn't save RAM:a.txt. The disk is
  full." "There isn't enough memory for that. A 2000 x 1500 picture needs 12 MB; 6 MB is free."
- **Questions say what the buttons will do**, and the buttons answer them: "Put the document back
  as it was last saved? The changes since are lost." [Revert] [Cancel].
- **Title case for commands, buttons and tabs** ("Save As...", "Go To Line"); **sentence case for
  help, descriptions and messages**.
- **"..." after a command means a dialog comes first.** Not on buttons inside dialogs.
- **British spelling** in user text and documentation: colour, centre, grey, licence (noun),
  organise. Keep the upstream spelling only in command names users would search for.
- **No blame, no jargon, no exclamation marks.** "Jotter can't find "cat"", not "Search failed!".
- **Short labels.** German runs about a third longer than English, French and Spanish a quarter;
  ribbon labels and pane titles must still fit 640 pixels (chapter 8's fit checks).
- **Don't build sentences from pieces.** Translate a whole format string ("Delete %ld layers?"),
  never "Delete " + n + " layers"; word order differs between languages.
- **Help bubbles**: the name, then one or two sentences saying what it does and, where it isn't
  obvious, when to use it. Write the help in the command table, next to the label, so it is
  written once and is complete.
- **Code comments** are ordinary English too: a comment above every function saying what it does,
  in words someone new can follow ("the caret brought into view by scrolling, if it isn't").

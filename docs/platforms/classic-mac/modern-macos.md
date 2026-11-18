# Appendix: modern macOS, briefly

The family's Mac ports target the classic Mac OS. Should a modern macOS port come later, the shared
concepts map as follows (Apple's current Human Interface Guidelines then win where they differ):

| Cornerstone | Modern macOS |
|---|---|
| The command table | Drives the menu bar (mandatory, as on System 7), `NSMenuItem`s with key equivalents; the command search is the system's Help menu search, fed by the same labels. |
| Ribbon | A toolbar (`NSToolbar`) of the most used commands, customisable; inspectors in a sidebar for the rest. Not a ribbon. |
| Panes | A sidebar (navigation) and an inspector (properties). |
| Dialogs | Sheets attached to the document window for document-modal questions; alerts with the app icon; Cancel left of the default button, Esc and Command-period cancel. |
| Interface icons | SF Symbols where one fits; the icon language for the program's own, rendered to template images (one colour, tinted by the system). |
| Dark theme | The system's Dark Mode: tokens map to semantic colours. |
| Translation | `.strings` or `.xcstrings` generated from the same `lang/strings.txt` (UTF-8: the Latin-1 restriction no longer needed but harmless). |
| Long operations | A progress sheet or the window's toolbar progress, with Command-period. |
| Packaging | A signed and notarised app bundle in a disk image, instead of an Installer script. |
| Keyboard | Command-key equivalents per Apple's reserved list; full keyboard access for dialogs. |

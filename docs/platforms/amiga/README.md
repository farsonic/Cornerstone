# Cornerstone on the Amiga (AmigaOS 3.x, MUI 3.8, AROS and MorphOS with Zune)

The Amiga is Cornerstone's reference platform: Quill, Ledger, Easel and Folio are built this way,
and the kit (`kit/amiga`) is their shared code generalised. These chapters turn the shared
concepts (`docs/shared/`) into vbcc, NDK and MUI practice. Jotter (`example/`) is the program the
walkthrough (chapter 16) builds from nothing.

| Chapter | |
|---|---|
| [1. Toolchain and project layout](01-toolchain.md) | vbcc, NDK 3.2, MUI 3.8 headers; build.sh, tools/test.sh; 68000, 68020 and 68040 with FPU |
| [2. The application skeleton](02-skeleton.md) | start-up and shutdown, libraries, arguments, prefs, PROGDIR:, public screens, the main loop |
| [3. The theme](03-theme.md) | colour tokens as pens, light and dark, QuillSans and the font loader, drawing helpers, bubbles |
| [4. The classes](04-classes.md) | MUI in five minutes, writing a class, every kit class's API with annotated listings, and the family's other classes |
| [5. Menus and context menus](05-menus.md) | menu strips from the command table, Intuition's limits, updating states, context menus |
| [6. Dialogs](06-dialogs.md) | the modal pattern, buttons and labels, requesters, file requesters, Zune fallbacks |
| [7. Long jobs](07-jobs.md) | the progress window with Abort, Esc from the keyboard matrix, memory checks |
| [8. Translation](08-translation.md) | Latin-1, tr() in MUI, Locale prefs, the fit checks with the real font |
| [9. Right to left](09-rtl.md) | cs_rtl, mirrored drawing in a view |
| [10. Keyboard](10-keyboard.md) | menu shortcuts, KeyTips, raw keys in classes, the emulator's Alt |
| [11. Interface icons](11-icons.md) | the renderer, sizes, pens, the previewer |
| [12. Testing](12-testing.md) | native tests, -m32, fuzzing, the emulator harness and its rules |
| [13. Workbench icons](13-workbench-icons.md) | mkicons.py: GlowIcons, planar images, tool types |
| [14. Installer scripts](14-installer.md) | the rules that keep a script working on Commodore's Installer, InstallerLG and AROS |
| [15. Packaging](15-packaging.md) | the .lha's layout, Source/, the key-material refusal |
| [16. Walkthrough: Jotter from nothing](16-walkthrough.md) | every file of the example, in the order it is built |
| [17. Pitfalls](17-pitfalls.md) | vbcc, MUI and Zune differences, things we have hit |
| [18. Where to look](18-where-to-look.md) | the best real example of each technique in the four programs |

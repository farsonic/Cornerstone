# 12. Testing on the Amiga

Shared chapter 11 has the philosophy. Here: the native tests as the Amiga programs run them, and
the emulator harness.

## Native

`tools/test.sh` (chapter 1). Beyond the kit's and the program's tests, the Amiga-specific checks
that run natively:

- **32-bit builds** (`cc -m32` where the host has the libraries): the Amiga's `long` is 32 bits,
  and an overflow that a 64-bit test machine hides shows up there (Folio's lexer, page sizes, xref
  offsets).
- **valgrind** on a few runs for what the sanitizers miss (Folio's tokens freed uninitialised).
- **Allocation failure** (Folio's `tests/allocfail.c`): the core built with `-DFOLIO_ALLOC_TEST` so
  each allocation can be made to fail in turn.
- **Fuzzing** (Folio's `tools/fuzz.c`, `tools/fuzz.sh SECONDS`; `M32=1` for 32 bits): crashes kept
  as files and replayed; each finding becomes a robustness test.
- The **interface's data** checked by the kit's tools: `langcheck`, `ribbonfit`, `langfit`,
  `cs_menu_check`, `cs_icon_check`.

## The emulator

FS-UAE runs an A4000 with a 68040, a uaegfx graphics card and 64 MB, on an Xvfb display, from a
hard-disk directory on the host (`hard_drive_0 = <dir>`, so files written on the Amiga appear on the
host and the other way round). The AROS ROM and its Zune stand in for Kickstart and MUI; AmigaOS 3.x
with MUI 3.8 on real hardware stays the reference (chapter 17 lists the differences found).

### The rules on a shared machine

The emulator setup is **shared** between sessions:

1. **Take the lock** for the whole run: `flock -w 5400 /tmp/lagoon-emu.lock sh run.sh`. Another
   session may be using the same hard-disk directory or display.
2. **Your own display** (`Xvfb :97 ...`) unless one is free, and stop only what you started (kill
   your own PIDs; never `pkill fs-uae` broadly, other sessions have theirs).
3. **The startup-sequence** is the way in: back it up, write yours, boot, and **restore it as soon
   as your run is done**, before you let go of the lock. Put commands in the startup-sequence itself:
   `Execute` there can't make its temporary files.
4. **Your files in a drawer of your own** on the shared disk (`DH0:Cornerstone/`), moved aside
   (not deleted) afterwards.
5. **Keep sessions short**, and don't run many heavy builds at once beside the emulator (about 15
   sanitizer builds at once once restarted the container).

### Driving it

- **`CMD=n`** opens the program and runs command n: any dialog without the mouse. The numbers are in
  the command table's blocks (`ID_REPLACE` = 210 in Jotter).
- **`DEBUG=file`** writes the log: watch for its first line on the host to know the program is up,
  and read it afterwards (any window that couldn't open is logged).
- **Several runs in one boot**: `Run >NIL: Program ... CMD=n` lines with `Wait` between them in the
  startup-sequence; each window opens over the last.
- **Keys** with `xdotool key F10`, `xdotool key Escape`, `xdotool type "save"`. Mouse moves are
  relative and coarse, and the first right click is often lost: prefer keys. **Alt doesn't reach
  the Amiga** in this setup.
- **Screenshots** with ImageMagick's `import -window root`, cropped to the Amiga's screen and scaled
  to 640 x 480.

Jotter's run, `example/tools/emucheck.sh`, does all of this: four copies of Jotter (light, German,
dark, right to left), then Replace and Options with `CMD=`, KeyTips and the File page and Tell Me
with keys, a screenshot of each, the startup-sequence restored and the drawer moved aside.

### What to look at

Every new class or dialog at 640 x 480 and a larger screen, light and dark, English and German, on a
256-colour screen once per release; leftovers after dialogs and pop-ups close; the window
underneath repainted whole. Then on real hardware: MUI 3.8 (radio buttons, scroll groups, frame
titles as MUI draws them), a real 68040's speed, AGA screens.

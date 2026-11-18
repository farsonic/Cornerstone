# 11. Testing

The portable core is tested natively, hard; the interface is checked by tools natively where it is
data, and by eye in an emulator and on real machines where it is drawing.

## Native tests (what CI runs)

`tools/test.sh` builds every test with

```sh
cc -std=c99 -O1 -g -Wall -Wextra -fsanitize=address,undefined ...
```

and runs it. A test is a plain C program: a `CHECK(condition)` macro counting passes and failures,
one function per area, a summary line `N tests, M failed`, exit status nonzero on failure. No test
framework, nothing to install. Jotter's `example/tests/coretest.c` and the kit's
`kit/shared/tests/kittest.c` are the models; Quill's has 1,800 checks, Ledger's 1,000 plus 3,600
worksheet-function rows.

What to test:

- **every core function**, including its failure paths (a file that isn't there, a broken one);
- **the interface's data**: the menu layout within its limits, every ribbon item a known command,
  command blocks not overlapping, every icon well formed, every command translated, every tab
  fitting 640 pixels (Jotter's `test_ui`);
- **files against other programs**: Quill's .docx and .odt against LibreOffice, Easel's codecs
  against Pillow and ImageMagick, Folio's rendering against poppler, Ledger's functions against
  LibreOffice (with Excel's documented behaviour winning where they disagree);
- **a bug's test before its fix**: reproduce first, write the failing test, then fix.

## Harder: robustness

Folio, whose input is hostile by nature (PDFs from anywhere), shows the full set:

- `tests/robusttest.c`: hostile inputs made in memory, each run in a child process with a time
  limit, every sanitizer report fatal, `-fsanitize=float-cast-overflow` on;
- a **32-bit build** (`-m32`) where possible: the Amiga's `long` is 32 bits, and overflows that a
  64-bit test machine hides show up there;
- **valgrind** for what the sanitizers don't see (uninitialised reads);
- `tests/allocfail.c`: the core built with every allocation able to fail, each failing in turn;
- **a fuzzer** (`tools/fuzz.c`, `tools/fuzz.sh SECONDS`): mutated inputs from a seed corpus, crashes
  kept for replay; each finding gets a robusttest case that failed before its fix. About 500,000
  inputs have run against Folio's engine.

Any program that reads files from elsewhere should have at least the robustness tests and a short
fuzzing run.

## The interface

What only a screen shows (layout, drawing, input, the feel of it) is checked in an emulator, then
on real machines. Each platform chapter has its harness. The rules:

- **A way in without the mouse**: the `CMD=n` start argument runs command n once the program is up,
  so any dialog can be opened for a screenshot; `DEBUG=file` writes a log (the screen and window
  sizes at start, every command, slow steps' timings, any window that couldn't open).
- **Every new class or dialog at the smallest screen** (640 x 480 on the Amiga) and a larger one,
  in both themes, in German (the longest) as well as English, and on a screen of few colours once
  per release.
- **Look for leftovers**: after every dialog and pop-up closes, is the window underneath repainted
  whole?
- **Say what was and wasn't checked** in the commit, the pull request and the CLAUDE.md: "not
  checked on an Amiga" is a fact worth writing down.
- **Shared emulators are locked** (`flock`) and their start-up files restored at once (the Amiga
  chapter has the procedure).

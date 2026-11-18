# 7. Long operations: progress, Abort, memory

Anything that can take more than half a second reports progress and can be stopped. A 68040
takes seconds to minutes over what a PC does at once (a filter on a big picture, rendering a PDF
page, recalculating a large sheet, OCR), and a program that freezes without a word looks broken.

## In the core

Slow loops call a hook once a row or so (`kit/shared/cs_util.h`):

```c
extern int (*cs_progress_hook)(long done, long total);
int  cs_progress(long done, long total);   /* nonzero once Abort was pressed */
long cs_progress_push(long lo, long hi);   /* this stage is lo..hi thousandths of the whole */
void cs_progress_pop(long saved);

for (y = y0; y < y1 && !cs_progress(y - y0, y1 - y0); y++) { ...one row... }
```

With no hook set, `cs_progress()` is a single test, so tests and batch tools pay nothing. A long
operation made of stages maps each stage onto part of the whole with `push`/`pop`, so the gauge
moves steadily.

## In the front end

Every command the main loop runs is a **job**: the platform sets the hook before the command and
clears it after. Once a command has run half a second, a small window opens over the main one
with the command's name, a gauge and an Abort button (Esc works too, read straight from the
keyboard so it works whichever window is active; Command-period on the Mac). The gauge moves at
most ten times a second, and each move lets the platform handle that window's input. When Abort is
pressed, `cs_progress()` returns nonzero and the core stops.

## Three rules make stopping safe

1. **Every operation that changes the document starts an undo step before it changes anything**,
   so there is always something to take back. When a job is aborted, the front end takes back its
   unfinished step and the document is exactly as it was.
2. **A loop that stops early leaves memory consistent**: it frees what it allocated and leaves no
   half-written pointers. The undo restores the document's contents, not the program's structures.
3. **Previews in dialogs run without the progress window** (the dialog is already a window the
   user is looking at); an aborted preview leaves nothing behind.

## Memory

Machines have from 2 to 64 MB, much of it often in use. Before an allocation that depends on the
document (a picture's pixels, a page's raster, an undo copy of a big selection), check:

```c
if (!cs_memory_check(bytes, "Image Size")) return;   /* tells the user what's needed and what's free */
```

and when an allocation fails anyway, say so in words (chapter 6) and leave the document as it was.
Folio's `tests/allocfail.c` fails each allocation of its core in turn and checks nothing crashes
or leaks: the strongest test of this rule, and a pattern to copy.

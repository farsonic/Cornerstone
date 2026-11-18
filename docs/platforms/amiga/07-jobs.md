# 7. Long jobs

Shared chapter 7 has the rules; `kit/amiga/cs_jobs.[ch]` is the Amiga side.

```c
cs_job_begin(label);          /* the core's progress hook set; the clock started */
jotter_command(id, value);    /* slow loops call cs_progress(done, total) */
aborted = cs_job_end();       /* the hook as it was; 1 if Abort was pressed */
```

- `cs_job_begin()` sets `cs_progress_hook` and notes the time. While the command has run less than
  half a second, each `cs_progress()` call only checks the Esc key. After that, a small window opens
  over the main one ("Working", the command's name, a gauge, "Abort (Esc)"), and is woken although
  the application sleeps (`MUIA_Window_Sleep` is counted, hence the loop that wakes it).
- Each later `cs_progress()` call moves the gauge at most ten times a second and lets MUI handle
  that window's input (`MUIM_Application_InputBuffered`), so Abort and the close gadget are seen.
- **Esc is read straight from the keyboard's matrix** (`keyboard.device`, `KBD_READMATRIX`, raw key
  0x45), so it works whichever window is active.
- Jobs nest: each restores the hook it found, and the window goes with the job that opened it.
- `cs_job_end()` closes the window and redraws the main one. The program then takes back the
  unfinished undo step if the job was aborted (Easel's `jobs.c` does that itself; the kit leaves it
  to the program, which owns its undo).
- Dialogs' previews run with the hook cleared (`cs_run_modal` does it), so no progress window opens
  over a dialog.

Memory: `cs_memory_check(bytes, "Image Size")` compares with `AvailMem(MEMF_ANY | MEMF_LARGEST)`
less a 256 KB margin for the system, and if it is short tells the user what is needed and what the
largest free block is, in their language.

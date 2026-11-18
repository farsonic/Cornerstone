/*
 * cs_jobs.h - long operations with a progress window and an Abort button. Every command the main
 * loop runs is a job: cs_job_begin() sets the core's progress hook (kit/shared/cs_util.h), and once
 * the command has run half a second a small window opens over the main one with its name, a gauge
 * and Abort (or Esc, read straight from the keyboard so it works whichever window is active). Each
 * cs_progress() call then moves the gauge (at most ten times a second) and lets MUI deal with the
 * window. When Abort is pressed cs_progress() returns nonzero; the core stops and the command takes
 * its unfinished step back (the program's undo). Jobs nest.
 */
#ifndef CS_JOBS_H
#define CS_JOBS_H

#include "cs_amiga.h"

void cs_job_begin(const char *what);            /* what: the command's name, shown in the window (English) */
int  cs_job_end(void);                          /* 1 if the job was aborted */
int  cs_job_aborted(void);                      /* Abort was pressed in the job running */
void cs_jobs_free(void);                        /* at the end: the keyboard device given back */
extern int cs_jobs_window;                      /* 0 while starting up (no window over the splash) */
extern int cs_jobs_shown;                       /* (testing) how often the window has opened */

/* memory: is there room for bytes more (fast or chip, as MEMF_ANY says) and a margin for the
   system? If not, the user is told how much is needed and how much is free; returns 0 */
int cs_memory_check(unsigned long bytes, const char *what);

#endif

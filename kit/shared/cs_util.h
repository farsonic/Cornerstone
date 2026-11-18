/*
 * cs_util.h - small portable pieces every Cornerstone program needs: the preferences file, the
 * progress hook long operations report to, word wrapping for requesters and bubbles, KeyTips codes,
 * and safe string copying. Plain C99, no platform calls: tested natively (kit/shared/tests).
 */
#ifndef CS_UTIL_H
#define CS_UTIL_H

#include <stddef.h>

/* ---- strings */
/* src copied into dst (cap bytes, always ended); returns dst */
char *cs_strlcpy(char *dst, const char *src, size_t cap);
/* "a" and "b" equal, letters compared by cs_fold (case and accents folded) */
int cs_strieq(const char *a, const char *b);

/* ---- preferences: a plain text file of key=value lines ("# " lines are notes), as every program
   in the family keeps in PROGDIR:<Program>.prefs. Keys are case-sensitive, at most 32 characters;
   values at most 255. Unknown keys are kept and written back, so an older version doesn't lose a
   newer one's settings */
#define CS_PREFS_MAX 128
int  cs_prefs_load(const char *path);                     /* 1 if read; 0 if missing (defaults then) */
int  cs_prefs_save(const char *path);                     /* 1 if written */
const char *cs_prefs_get(const char *key, const char *def);
long cs_prefs_int(const char *key, long def);
int  cs_prefs_set(const char *key, const char *value);    /* 0 if full */
int  cs_prefs_set_int(const char *key, long value);
void cs_prefs_clear(void);

/* ---- progress: slow loops in the core call cs_progress(done, total) every row or so. With no hook
   set it costs a test and returns 0. The front end's hook (Amiga: cs_jobs.c) shows a window after
   half a second and returns nonzero once Abort was pressed: the loop must then stop and leave its
   data consistent. A stage of a longer operation maps its own 0..total onto part of the whole */
extern int (*cs_progress_hook)(long done, long total);
int  cs_progress(long done, long total);                  /* nonzero: abort */
long cs_progress_push(long lo, long hi);                  /* this stage is lo..hi thousandths */
void cs_progress_pop(long saved);

/* ---- wrapping. Requesters (ask()) wrap at 66 characters, help bubbles at 46. A word longer than
   the width goes on a line of its own. Existing line breaks are kept. Returns out */
#define CS_WRAP_ASK 66
#define CS_WRAP_BUBBLE 46
char *cs_wrap(const char *in, char *out, size_t cap, int width);

/* ---- KeyTips (Alt or F10 shows a letter on every tab, then on every tool of the tab): codes of
   one or two characters for n names (NULL names get none). A name's first letter when no other
   name starts with it; else that letter and a later letter of its own, or a digit */
void cs_keytips_assign(const char *const *names, int n, char (*codes)[3]);
/* the code typed so far matched against codes: the index of the one it names, -1 if it is the
   start of some (wait for the next key), -2 if none */
int  cs_keytips_match(char (*codes)[3], int n, const char *typed);

/* ---- integer arithmetic without overflow on 32-bit long (scroll bars and the like): a * b / c */
long cs_muldiv(long a, long b, long c);

#endif

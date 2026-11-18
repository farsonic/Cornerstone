/*
 * cs_amiga.h - what every part of Cornerstone's Amiga kit shares: the tag range of its classes,
 * the application and main window it works with, the public screen, the debug log and the window
 * macro. Include it in each file of the program's amiga/ that uses the kit.
 *
 * The kit is built for AmigaOS 3.x with MUI 3.8 (or AROS/MorphOS's Zune) by vbcc, 68020 and up;
 * it uses no floating point, so it serves 68000-class programs and FPU programs alike.
 */
#ifndef CS_AMIGA_H
#define CS_AMIGA_H

#include <exec/types.h>
#include <libraries/mui.h>
#include <stdio.h>

/* the kit's tags and methods: TAG_USER | 'CS' << 16, a block of 0x100 per class. A program's own
   classes take their own base (CS_APPTAG, set per program: docs/platforms/amiga/04-classes.md) */
#define CS_TAG(n) (TAG_USER | 0x43530000UL | (ULONG)(n))

/* the application and its main window, set once they are made (cs_app_set): dialogs, requesters,
   the progress window and pop-ups open over that window */
extern Object *cs_app, *cs_win;
void cs_app_set(Object *app, Object *win);
/* the public screen the windows open on ("": the default, usually Workbench) */
extern char cs_pubscreen[64];
/* a WindowObject on that screen: use it for every window, so all of them appear together */
#define CsWindowObject WindowObject, (ULONG)(cs_pubscreen[0] ? MUIA_Window_PublicScreen : TAG_IGNORE), (ULONG)cs_pubscreen

/* the debug log: DEBUG=file at start (cs_app_args), NULL otherwise; CS_LOG costs a test when off */
extern FILE *cs_log;
void cs_logf(const char *fmt, ...);
#define CS_LOG(...) do { if (cs_log) cs_logf(__VA_ARGS__); } while (0)

/* the value of an attribute (get() into a ULONG) */
ULONG cs_get(Object *o, ULONG attr);

/* the command ids the kit's dialogs return through MUIM_Application_ReturnID. Kept below 100,
   where a program's command blocks never start */
enum { CS_ID_OK = 50, CS_ID_CANCEL, CS_ID_CHANGE, CS_ID_EXTRA, CS_ID_IDLE };

#endif

/*
 * cs_app.h - starting and ending a Cornerstone program on the Amiga: the libraries, the start
 * arguments (from the Shell or Workbench's tool types), the preferences file's place, the fonts
 * drawer, the language chosen from Locale prefs, the main loop and the order of shutting down.
 * docs/platforms/amiga/02-skeleton.md walks through it.
 */
#ifndef CS_APP_H
#define CS_APP_H

#include "cs_amiga.h"

/* the libraries every program needs (intuition, graphics, layers, utility, keymap 39; muimaster 19),
   and the optional ones (diskfont, cybergraphics, icon, locale): 0 if a needed one is missing (the
   reason is printed in the Shell, or shown in a requester when started from Workbench) */
int  cs_libs_open(const char *program);
void cs_libs_close(void);
extern struct Library *CyberGfxBase;     /* NULL without a graphics card's library */

/* the start arguments, the same from the Shell (Program FILE... PUBSCREEN=name LANGUAGE=de
   DEBUG=file CMD=n) and from Workbench (icons given with the program's, and the tool types
   PUBSCREEN, LANGUAGE, DEBUG). CMD=n runs command n once the program is up: invaluable for testing
   dialogs in an emulator (docs/platforms/amiga/12-testing.md) */
#define CS_MAXFILES 8
typedef struct {
  int nfiles;
  char files[CS_MAXFILES][256];
  char pubscreen[64];
  char language[8];
  unsigned long cmd;
  int from_wb;
} CsArgs;
void cs_args(int argc, char **argv, CsArgs *out);   /* opens the log too when DEBUG= is given */

/* "PROGDIR:<program>.prefs" */
const char *cs_prefs_path(const char *program);
/* the program's Fonts drawer added to FONTS: (other programs then find QuillSans too), and taken
   off again at the end */
void cs_fonts_assign(void);
void cs_fonts_unassign(void);
/* the language to use: the prefs file's "language" if set, else the first of Locale prefs'
   languages that the program has, else English (cs_locale.c) */
int  cs_lang_system(void);
int  cs_lang_choose(const char *from_args);

/* the main loop's step: MUIM_Application_NewInput and the Wait, with Ctrl-C. Returns the id
   MUI gave (0 none), or MUIV_Application_ReturnID_Quit for Ctrl-C too */
ULONG cs_input(ULONG *sigs);

#endif

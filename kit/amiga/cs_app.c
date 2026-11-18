/* cs_app.c - see cs_app.h: libraries, start arguments, the log, the fonts assign, Locale */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dos/dos.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <libraries/locale.h>
#include <proto/locale.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_app.h"
#include "cs_lang.h"
#include "cs_util.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *MUIMasterBase, *LayersBase, *DiskfontBase, *KeymapBase, *UtilityBase, *CyberGfxBase, *IconBase;
struct LocaleBase *LocaleBase;
extern struct WBStartup *_WBenchMsg;

Object *cs_app, *cs_win;
char cs_pubscreen[64];
FILE *cs_log;

void cs_app_set(Object *app, Object *win) { cs_app = app; cs_win = win; }
ULONG cs_get(Object *o, ULONG attr) { ULONG v = 0; if (o) get(o, attr, &v); return v; }
void cs_logf(const char *fmt, ...)
{
  va_list ap;
  if (!cs_log) return;
  va_start(ap, fmt);
  vfprintf(cs_log, fmt, ap);
  va_end(ap);
  fputc('\n', cs_log);
  fflush(cs_log);
}
/* a message where the user will see it: the Shell's window, or an Intuition requester (MUI may be
   the very thing that is missing) */
static void complain(const char *program, const char *msg)
{
  if (!_WBenchMsg) { printf("%s: %s\n", program, msg); return; }
  if (IntuitionBase) {
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, NULL, NULL, NULL };
    es.es_Title = (UBYTE *)program; es.es_TextFormat = (UBYTE *)"%s"; es.es_GadgetFormat = (UBYTE *)"OK";
    EasyRequest(NULL, &es, NULL, (ULONG)msg);
  }
}
int cs_libs_open(const char *program)
{
  if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library", 39)) ||
      !(GfxBase = (struct GfxBase *)OpenLibrary((STRPTR)"graphics.library", 39)) ||
      !(LayersBase = OpenLibrary((STRPTR)"layers.library", 39)) ||
      !(UtilityBase = OpenLibrary((STRPTR)"utility.library", 39)) ||
      !(KeymapBase = OpenLibrary((STRPTR)"keymap.library", 37))) {
    complain(program, "This program needs AmigaOS 3.0 or later.");
    return 0;
  }
  if (!(MUIMasterBase = OpenLibrary((STRPTR)MUIMASTER_NAME, 19))) {
    complain(program, "This program needs MUI 3.8 or later (muimaster.library 19).");
    return 0;
  }
  DiskfontBase = OpenLibrary((STRPTR)"diskfont.library", 39);
  CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library", 40);
  IconBase = OpenLibrary((STRPTR)"icon.library", 37);
  return 1;
}
void cs_libs_close(void)
{
  if (cs_log) { fclose(cs_log); cs_log = NULL; }
  if (IconBase) CloseLibrary(IconBase);
  if (CyberGfxBase) CloseLibrary(CyberGfxBase);
  if (DiskfontBase) CloseLibrary(DiskfontBase);
  if (MUIMasterBase) CloseLibrary(MUIMasterBase);
  if (KeymapBase) CloseLibrary(KeymapBase);
  if (UtilityBase) CloseLibrary(UtilityBase);
  if (LayersBase) CloseLibrary(LayersBase);
  if (GfxBase) CloseLibrary((struct Library *)GfxBase);
  if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
  IconBase = CyberGfxBase = DiskfontBase = MUIMasterBase = KeymapBase = UtilityBase = LayersBase = NULL;
  GfxBase = NULL; IntuitionBase = NULL;
}
/* one KEY=value argument (the family's style, so a Shell line and a tool type read alike) */
static int keyarg(const char *a, CsArgs *o)
{
  if (!Strnicmp((STRPTR)a, (STRPTR)"PUBSCREEN=", 10)) { cs_strlcpy(o->pubscreen, a + 10, sizeof o->pubscreen); return 1; }
  if (!Strnicmp((STRPTR)a, (STRPTR)"LANGUAGE=", 9)) { cs_strlcpy(o->language, a + 9, sizeof o->language); return 1; }
  if (!Strnicmp((STRPTR)a, (STRPTR)"CMD=", 4)) { o->cmd = strtoul(a + 4, NULL, 10); return 1; }
  if (!Strnicmp((STRPTR)a, (STRPTR)"DEBUG=", 6)) { if (!cs_log) cs_log = fopen(a + 6, "w"); return 1; }
  return 0;
}
void cs_args(int argc, char **argv, CsArgs *o)
{
  int i;
  memset(o, 0, sizeof *o);
  for (i = 1; i < argc; i++)
    if (!keyarg(argv[i], o) && o->nfiles < CS_MAXFILES) cs_strlcpy(o->files[o->nfiles++], argv[i], sizeof o->files[0]);
  if (argc == 0 && _WBenchMsg) {
    struct WBArg *wa = &_WBenchMsg->sm_ArgList[0];
    o->from_wb = 1;
    /* the tool types of the program's icon */
    if (IconBase) {
      BPTR old = CurrentDir(wa->wa_Lock);
      struct DiskObject *dob = GetDiskObject((STRPTR)wa->wa_Name);
      if (dob) {
        static const char *const keys[] = { "PUBSCREEN", "LANGUAGE", "DEBUG", NULL };
        int k;
        for (k = 0; keys[k]; k++) {
          STRPTR v = FindToolType((STRPTR *)dob->do_ToolTypes, (STRPTR)keys[k]);
          char line[300];
          if (v) { snprintf(line, sizeof line, "%s=%s", keys[k], (char *)v); keyarg(line, o); }
        }
        FreeDiskObject(dob);
      }
      CurrentDir(old);
    }
    /* the project icons given with it (shift-clicked, or the program as their default tool) */
    for (i = 1; i < _WBenchMsg->sm_NumArgs && o->nfiles < CS_MAXFILES; i++) {
      wa = &_WBenchMsg->sm_ArgList[i];
      if (NameFromLock(wa->wa_Lock, (STRPTR)o->files[o->nfiles], 256) && AddPart((STRPTR)o->files[o->nfiles], wa->wa_Name, 256)) o->nfiles++;
    }
  }
  /* a public screen that isn't there: the default one */
  if (o->pubscreen[0]) {
    struct Screen *sc = LockPubScreen((STRPTR)o->pubscreen);
    if (sc) UnlockPubScreen(NULL, sc); else o->pubscreen[0] = 0;
  }
  cs_strlcpy(cs_pubscreen, o->pubscreen, sizeof cs_pubscreen);
}
const char *cs_prefs_path(const char *program)
{
  static char p[64];
  snprintf(p, sizeof p, "PROGDIR:%s.prefs", program);
  return p;
}
static int assigned;
void cs_fonts_assign(void)
{
  BPTR l;
  struct Process *me = (struct Process *)FindTask(NULL);
  APTR w = me->pr_WindowPtr;
  me->pr_WindowPtr = (APTR)-1;                 /* no "please insert volume" if there is no drawer */
  if ((l = Lock((STRPTR)"PROGDIR:Fonts", ACCESS_READ))) {
    if (AssignAdd((STRPTR)"FONTS", l)) assigned = 1; else UnLock(l);
  }
  me->pr_WindowPtr = w;
}
void cs_fonts_unassign(void)
{
  BPTR l;
  if (!assigned) return;
  if ((l = Lock((STRPTR)"PROGDIR:Fonts", ACCESS_READ))) { RemAssignList((STRPTR)"FONTS", l); UnLock(l); }
  assigned = 0;
}
/* the first of the user's preferred languages (Locale prefs, named in their own words: "deutsch")
   that the program speaks; locale.library is opened only for this */
int cs_lang_system(void)
{
  int found = -1, k;
  struct Locale *loc;
  if (!(LocaleBase = (struct LocaleBase *)OpenLibrary((STRPTR)"locale.library", 38))) return CS_LANG_EN;
  if ((loc = OpenLocale(NULL))) {
    for (k = 0; k < 10 && loc->loc_PrefLanguages[k] && found < 0; k++)
      found = cs_lang_by_locale_name((const char *)loc->loc_PrefLanguages[k]);
    CloseLocale(loc);
  }
  CloseLibrary((struct Library *)LocaleBase); LocaleBase = NULL;
  return found < 0 ? CS_LANG_EN : found;
}
int cs_lang_choose(const char *from_args)
{
  int l = from_args && from_args[0] ? cs_lang_by_code(from_args) : -1;
  if (l < 0) l = cs_lang_by_code(cs_prefs_get("language", ""));
  if (l < 0) l = cs_lang_system();
  cs_lang_set(l);
  return l;
}
ULONG cs_input(ULONG *sigs)
{
  ULONG id = DoMethod(cs_app, MUIM_Application_NewInput, (ULONG)sigs);
  if (*sigs) {
    *sigs = Wait(*sigs | SIGBREAKF_CTRL_C);
    if (*sigs & SIGBREAKF_CTRL_C) return MUIV_Application_ReturnID_Quit;
  }
  return id;
}

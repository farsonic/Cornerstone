/* cs_jobs.c - see cs_jobs.h. From Easel's jobs.c, without its undo (the program takes its own step back). */
#include <string.h>
#include <stdio.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/keyboard.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_jobs.h"
#include "cs_dialog.h"
#include "cs_theme.h"
#include "cs_lang.h"
#include "cs_util.h"

#define MAXJOBS 8
static struct { int (*hook)(long, long); LONG started; char what[80]; } jobs[MAXJOBS];
static int njobs, pwin_depth;
static Object *pwin, *pgauge;
static LONG abort_flag;
int cs_jobs_shown, cs_jobs_window = 1;

/* Esc held down, from the keyboard's matrix */
static struct MsgPort *kport;
static struct IOStdReq *kreq;
static int kopen;
static int esc_down(void)
{
  UBYTE matrix[16];
  if (!kopen) {
    kopen = -1;
    if ((kport = CreateMsgPort()) && (kreq = (struct IOStdReq *)CreateIORequest(kport, sizeof *kreq)) &&
        !OpenDevice((STRPTR)"keyboard.device", 0, (struct IORequest *)kreq, 0)) kopen = 1;
  }
  if (kopen < 0) return 0;
  memset(matrix, 0, sizeof matrix);
  kreq->io_Command = KBD_READMATRIX; kreq->io_Data = matrix; kreq->io_Length = sizeof matrix;
  if (DoIO((struct IORequest *)kreq)) return 0;
  return (matrix[0x45 >> 3] >> (0x45 & 7)) & 1;     /* raw key 0x45: Esc */
}
void cs_jobs_free(void)
{
  if (kopen > 0) CloseDevice((struct IORequest *)kreq);
  if (kreq) DeleteIORequest((struct IORequest *)kreq);
  if (kport) DeleteMsgPort(kport);
  kreq = NULL; kport = NULL; kopen = 0;
}
static LONG ticks(void)
{
  struct DateStamp ds;
  DateStamp(&ds);
  return ds.ds_Days * (24L * 60 * 60 * 50) + ds.ds_Minute * (60L * 50) + ds.ds_Tick;
}
static void pwin_open(const char *what)
{
  Object *abort;
  int i;
  pwin = CsWindowObject, MUIA_Window_Title, (ULONG)tr("Working"), MUIA_Window_RefWindow, (ULONG)cs_win,
    MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered, MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
    MUIA_Window_CloseGadget, TRUE, MUIA_Window_SizeGadget, FALSE,
    WindowContents, VGroup, CS_DLG,
      Child, TextObject, MUIA_Text_Contents, (ULONG)tr(what), End,
      Child, pgauge = GaugeObject, GaugeFrame, MUIA_Gauge_Horiz, TRUE, MUIA_Gauge_Max, 1000, MUIA_FixHeightTxt, (ULONG)"M",
                      MUIA_FixWidthTxt, (ULONG)"MMMMMMMMMMMMMMMMMMMMMMMM", End,
      Child, HGroup, Child, HSpace(0), Child, abort = CsButton(tr("_Abort (Esc)")), Child, HSpace(0), End,
    End,
  End;
  if (!pwin) return;
  DoMethod(cs_app, OM_ADDMEMBER, (ULONG)pwin);
  DoMethod(abort, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 3, MUIM_WriteLong, 1, (ULONG)&abort_flag);
  DoMethod(pwin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 3, MUIM_WriteLong, 1, (ULONG)&abort_flag);
  set(pwin, MUIA_Window_Open, TRUE);
  /* the application sleeps while a command runs: this window alone is woken */
  for (i = 0; i < 8 && cs_get(pwin, MUIA_Window_Sleep); i++) set(pwin, MUIA_Window_Sleep, FALSE);
  cs_jobs_shown++;
  pwin_depth = njobs;
  CS_LOG("progress window: %s", what);
}
static void pwin_close(void)
{
  if (!pwin) return;
  set(pwin, MUIA_Window_Open, FALSE);
  DoMethod(cs_app, OM_REMMEMBER, (ULONG)pwin);
  MUI_DisposeObject(pwin);
  pwin = pgauge = NULL;
  cs_redraw_window();
}
static int hook(long done, long total)
{
  static LONG shown_at;
  LONG t;
  if (!cs_app || !njobs) return 0;
  if (abort_flag) return 1;
  t = ticks();
  if (!pwin) {
    if (!cs_jobs_window || t - jobs[njobs - 1].started < 25) {
      if (esc_down()) abort_flag = 1;
      return abort_flag != 0;
    }
    pwin_open(jobs[njobs - 1].what);
    shown_at = 0;
  }
  if (t - shown_at >= 5 && pgauge) {                 /* ten times a second at most */
    shown_at = t;
    set(pgauge, MUIA_Gauge_Current, (ULONG)cs_muldiv(done, 1000, total > 0 ? total : 1));
    DoMethod(cs_app, MUIM_Application_InputBuffered);
  }
  if (esc_down()) abort_flag = 1;
  return abort_flag != 0;
}
void cs_job_begin(const char *what)
{
  if (njobs >= MAXJOBS) { njobs++; return; }
  jobs[njobs].hook = cs_progress_hook;
  jobs[njobs].started = ticks();
  cs_strlcpy(jobs[njobs].what, what ? what : "Working", sizeof jobs[0].what);
  njobs++;
  if (njobs == 1) abort_flag = 0;
  cs_progress_hook = hook;
}
int cs_job_end(void)
{
  int aborted = abort_flag != 0;
  if (!njobs) return 0;
  njobs--;
  if (njobs >= MAXJOBS) return aborted;
  cs_progress_hook = jobs[njobs].hook;
  if (pwin && pwin_depth > njobs) pwin_close();
  if (!njobs) abort_flag = 0;
  return aborted;
}
int cs_job_aborted(void) { return abort_flag != 0; }
/* the jobs' clocks started again: time spent in a dialog or requester doesn't count towards the
   half second before the progress window opens */
void cs_jobs_reset_timer(void)
{
  int i;
  LONG t = ticks();
  for (i = 0; i < njobs && i < MAXJOBS; i++) jobs[i].started = t;
}

int cs_memory_check(unsigned long bytes, const char *what)
{
  unsigned long avail = AvailMem(MEMF_ANY | MEMF_LARGEST), margin = 256UL * 1024;
  char msg[300];
  if (bytes + margin <= avail) return 1;
  snprintf(msg, sizeof msg, tr("There isn't enough memory for %s. It needs %lu KB; the largest free block is %lu KB."),
           tr(what), (bytes + 1023) / 1024, avail / 1024);
  cs_ask("OK", "%s", msg);
  return 0;
}

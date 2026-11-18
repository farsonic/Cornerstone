/* cs_dialog.c - see cs_dialog.h: themed buttons and labels, modal dialogs, requesters, Zune fallbacks */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <exec/memory.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_amiga.h"
#include "cs_dialog.h"
#include "cs_theme.h"
#include "cs_lang.h"
#include "cs_util.h"

/* ---------------------------------------------------------------- the button and label classes */
#define MUIA_CsButton_Label CS_TAG(0x700)    /* (init) the label, "_" before its key */
#define MUIA_CsLabel_Flags  CS_TAG(0x701)    /* (init) MUIO_Label_ flags, and CSL_BOLD */
#define CSL_BOLD 0x10000
struct BData { char label[64]; LONG key; ULONG flags; };
static struct MUI_CustomClass *bcls, *lcls;

/* the label without its "_", and which letter it marked (-1: none) */
static void strip_key(const char *l, struct BData *d)
{
  LONG k = 0;
  d->key = -1;
  for (; *l && k < 63; l++) {
    if (*l == '_' && l[1] && d->key < 0) { d->key = k; continue; }
    d->label[k++] = *l;
  }
  d->label[k] = 0;
}
static void draw_label(struct RastPort *rp, struct BData *d, LONG x, LONG y)
{
  Move(rp, x, y);
  Text(rp, (STRPTR)d->label, (ULONG)strlen(d->label));
  if (d->key >= 0) {
    LONG ux = x + TextLength(rp, (STRPTR)d->label, (ULONG)d->key), uw = TextLength(rp, (STRPTR)d->label + d->key, 1);
    Move(rp, ux, y + 1); Draw(rp, ux + uw - 1, y + 1);
  }
}
/* the button: Area as a RelVerify gadget in the cycle chain with its key as MUIA_ControlChar; a
   face in the input colour with a thin border, the accent's tint while pressed */
static ULONG bdispatch(__reg("a0") struct IClass *cl, __reg("a2") Object *obj, __reg("a1") Msg msg)
{
  struct BData *d;
  switch (msg->MethodID) {
  case OM_NEW: {
    struct opSet *m = (struct opSet *)msg;
    struct BData tmp;
    struct TagItem more[] = { { MUIA_InputMode, MUIV_InputMode_RelVerify }, { MUIA_CycleChain, 1 }, { MUIA_ControlChar, 0 },
                              { MUIA_Background, MUII_ButtonBack }, { TAG_MORE, 0 } };
    struct opSet sup;
    more[4].ti_Data = (ULONG)m->ops_AttrList;
    strip_key((const char *)GetTagData(MUIA_CsButton_Label, (ULONG)"", m->ops_AttrList), &tmp);
    if (tmp.key >= 0) more[2].ti_Data = (ULONG)ToLower((ULONG)(UBYTE)tmp.label[tmp.key]); else more[2].ti_Tag = TAG_IGNORE;
    sup.MethodID = OM_NEW; sup.ops_AttrList = more; sup.ops_GInfo = m->ops_GInfo;
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, (Msg)&sup))) return 0;
    d = INST_DATA(cl, obj);
    *d = tmp;
    return (ULONG)obj;
  }
  case MUIM_Setup:
    if (!DoSuperMethodA(cl, obj, msg)) return FALSE;
    cs_theme_setup(obj);
    return TRUE;
  case MUIM_Cleanup:
    cs_theme_cleanup(obj);
    break;
  case MUIM_AskMinMax: {
    struct MUIP_AskMinMax *m = (struct MUIP_AskMinMax *)msg;
    struct RastPort trp;
    struct TextFont *tf = _font(obj) ? _font(obj) : cs_font();
    LONG w;
    DoSuperMethodA(cl, obj, msg);
    d = INST_DATA(cl, obj);
    InitRastPort(&trp);
    SetFont(&trp, tf);
    w = TextLength(&trp, (STRPTR)d->label, (ULONG)strlen(d->label)) + 24;
    if (w < 64) w = 64;                                /* buttons at least this wide: OK beside Cancel */
    m->MinMaxInfo->MinWidth += w; m->MinMaxInfo->DefWidth += w; m->MinMaxInfo->MaxWidth += MUI_MAXMAX;
    m->MinMaxInfo->MinHeight += tf->tf_YSize + 8; m->MinMaxInfo->DefHeight += tf->tf_YSize + 8; m->MinMaxInfo->MaxHeight += tf->tf_YSize + 8;
    return 0;
  }
  case MUIM_Draw: {
    struct RastPort *rp = _rp(obj);
    struct TextFont *tf = _font(obj) ? _font(obj) : cs_font();
    ULONG sel = cs_get(obj, MUIA_Selected), dis = cs_get(obj, MUIA_Disabled);
    LONG tw;
    DoSuperMethodA(cl, obj, msg);
    if (!(((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;
    d = INST_DATA(cl, obj);
    cs_fill(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(sel ? CS_T_CHECKED : CS_T_WHITE));
    cs_rect(rp, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), cs_pen(sel ? CS_T_ACCENT : CS_T_BORDER2));
    SetFont(rp, tf);
    SetSoftStyle(rp, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
    SetDrMd(rp, JAM1);
    SetAPen(rp, cs_pen(dis ? CS_T_DISABLED : CS_T_TEXT));
    tw = TextLength(rp, (STRPTR)d->label, (ULONG)strlen(d->label));
    draw_label(rp, d, _mleft(obj) + (_mwidth(obj) - tw) / 2, _mtop(obj) + (_mheight(obj) - tf->tf_YSize) / 2 + tf->tf_Baseline);
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
/* the label: as wide as its text, as tall as a framed gadget's text so the two share a baseline */
static ULONG ldispatch(__reg("a0") struct IClass *cl, __reg("a2") Object *obj, __reg("a1") Msg msg)
{
  struct BData *d;
  switch (msg->MethodID) {
  case OM_NEW: {
    struct opSet *m = (struct opSet *)msg;
    if (!(obj = (Object *)DoSuperMethodA(cl, obj, msg))) return 0;
    d = INST_DATA(cl, obj);
    strip_key((const char *)GetTagData(MUIA_CsButton_Label, (ULONG)"", m->ops_AttrList), d);
    d->flags = GetTagData(MUIA_CsLabel_Flags, 0, m->ops_AttrList);
    return (ULONG)obj;
  }
  case MUIM_Setup:
    if (!DoSuperMethodA(cl, obj, msg)) return FALSE;
    cs_theme_setup(obj);
    return TRUE;
  case MUIM_Cleanup:
    cs_theme_cleanup(obj);
    break;
  case MUIM_AskMinMax: {
    struct MUIP_AskMinMax *m = (struct MUIP_AskMinMax *)msg;
    struct RastPort trp;
    struct TextFont *tf = _font(obj) ? _font(obj) : cs_font();
    LONG w, h;
    DoSuperMethodA(cl, obj, msg);
    d = INST_DATA(cl, obj);
    InitRastPort(&trp);
    SetFont(&trp, tf);
    SetSoftStyle(&trp, d->flags & CSL_BOLD ? FSF_BOLD : 0, FSF_BOLD);
    w = TextLength(&trp, (STRPTR)d->label, (ULONG)strlen(d->label)) + 2;
    h = tf->tf_YSize + ((d->flags & MUIO_Label_DoubleFrame) ? 6 : (d->flags & MUIO_Label_SingleFrame) ? 4 : 0);
    m->MinMaxInfo->MinWidth += w; m->MinMaxInfo->DefWidth += w; m->MinMaxInfo->MaxWidth += (d->flags & CSL_BOLD) ? MUI_MAXMAX : w;
    m->MinMaxInfo->MinHeight += h; m->MinMaxInfo->DefHeight += h; m->MinMaxInfo->MaxHeight += h;
    return 0;
  }
  case MUIM_Draw: {
    struct RastPort *rp = _rp(obj);
    struct TextFont *tf = _font(obj) ? _font(obj) : cs_font();
    LONG tw, x;
    DoSuperMethodA(cl, obj, msg);
    if (!(((struct MUIP_Draw *)msg)->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;
    d = INST_DATA(cl, obj);
    SetFont(rp, tf);
    SetSoftStyle(rp, d->flags & CSL_BOLD ? FSF_BOLD : 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
    SetDrMd(rp, JAM1);
    SetAPen(rp, cs_pen(CS_T_TEXT));
    tw = TextLength(rp, (STRPTR)d->label, (ULONG)strlen(d->label));
    x = (d->flags & (MUIO_Label_LeftAligned | CSL_BOLD)) ? _mleft(obj) : _mright(obj) - tw;
    draw_label(rp, d, x, _mtop(obj) + (_mheight(obj) - tf->tf_YSize) / 2 + tf->tf_Baseline);
    SetSoftStyle(rp, 0, FSF_BOLD);
    return 0;
  }
  }
  return DoSuperMethodA(cl, obj, msg);
}
static int classes(void)
{
  if (!bcls) bcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(struct BData), (APTR)bdispatch);
  if (!lcls) lcls = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(struct BData), (APTR)ldispatch);
  return bcls && lcls;
}
void cs_dialog_classes_free(void)
{
  if (bcls) { MUI_DeleteCustomClass(bcls); bcls = NULL; }
  if (lcls) { MUI_DeleteCustomClass(lcls); lcls = NULL; }
}
Object *CsButton(const char *label)
{
  if (!classes()) return SimpleButton(label);
  return (Object *)NewObject(bcls->mcc_Class, NULL, MUIA_CsButton_Label, (ULONG)label, MUIA_Font, (ULONG)cs_dialog_font(), TAG_DONE);
}
Object *CsLabel(const char *label, ULONG flags)
{
  if (!cs_theme_is_dark() || !classes()) return MUI_MakeObject(MUIO_Label, (ULONG)label, flags);
  return (Object *)NewObject(lcls->mcc_Class, NULL, MUIA_CsButton_Label, (ULONG)label, MUIA_CsLabel_Flags, flags, TAG_DONE);
}
Object *CsTitle(const char *t)
{
  static char b[8][100];
  static int nb;
  char *s;
  if (cs_theme_is_dark() && classes())
    return (Object *)NewObject(lcls->mcc_Class, NULL, MUIA_CsButton_Label, (ULONG)t, MUIA_CsLabel_Flags, CSL_BOLD, TAG_DONE);
  s = b[nb++ & 7];                                    /* (MUI's Text copies its contents, but be safe) */
  s[0] = 27; s[1] = 'b';
  cs_strlcpy(s + 2, t, sizeof b[0] - 2);
  return TextObject, MUIA_Text_Contents, (ULONG)s, MUIA_Text_SetVMax, TRUE, End;
}

/* ---------------------------------------------------------------- modal dialogs */
void cs_redraw_window(void)
{
  Object *root = NULL;
  if (!cs_win || !cs_get(cs_win, MUIA_Window_Open)) return;
  get(cs_win, MUIA_Window_RootObject, &root);
  if (root && DoMethod(root, MUIM_Group_InitChange)) DoMethod(root, MUIM_Group_ExitChange);
}
static Object *make_dialog(const char *t, Object *contents, const char *oklabel, const char *extra)
{
  Object *ok, *cancel, *ex = NULL, *w, *row;
  row = HGroup, Child, HSpace(0), End;
  if (!row) { if (contents) MUI_DisposeObject(contents); return NULL; }
  if (extra && (ex = CsButton(tr(extra)))) DoMethod(row, OM_ADDMEMBER, (ULONG)ex);
  ok = CsButton(tr(oklabel ? oklabel : "_OK"));
  cancel = CsButton(tr("_Cancel"));
  if (ok) DoMethod(row, OM_ADDMEMBER, (ULONG)ok);
  if (cancel) DoMethod(row, OM_ADDMEMBER, (ULONG)cancel);
  w = CsWindowObject, MUIA_Window_Title, (ULONG)tr(t), MUIA_Window_RefWindow, (ULONG)cs_win,
    MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered, MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
    MUIA_Window_ID, 0,
    WindowContents, VGroup, CS_DLG,
      Child, contents,
      Child, row,
    End,
  End;
  if (!w) { MUI_DisposeObject(row); if (contents) MUI_DisposeObject(contents); return NULL; }
  DoMethod(ok, MUIM_Notify, MUIA_Pressed, FALSE, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_OK);
  DoMethod(cancel, MUIM_Notify, MUIA_Pressed, FALSE, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_CANCEL);
  if (ex) DoMethod(ex, MUIM_Notify, MUIA_Pressed, FALSE, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_EXTRA);
  /* Esc and the close gadget are Cancel */
  DoMethod(w, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_CANCEL);
  DoMethod(w, MUIM_Notify, MUIA_Window_InputEvent, (ULONG)"esc", (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_CANCEL);
  set(w, MUIA_Window_DefaultObject, (ULONG)ok);
  return w;
}
Object *cs_dialog_window(const char *t, Object *contents, const char *oklabel) { return make_dialog(t, contents, oklabel, NULL); }
Object *cs_dialog_window3(const char *t, Object *contents, const char *oklabel, const char *extra) { return make_dialog(t, contents, oklabel, extra); }
ULONG cs_run_modal(Object *w, const ULONG *ends, void (*each)(ULONG id))
{
  ULONG sigs = 0, id = 0;
  int i, done = 0;
  int (*hook)(long, long) = cs_progress_hook;
  if (!w) return CS_ID_CANCEL;
  cs_progress_hook = NULL;                     /* a dialog's previews show no progress window */
  DoMethod(cs_app, OM_ADDMEMBER, (ULONG)w);
  if (cs_win) set(cs_win, MUIA_Window_Sleep, TRUE);
  set(w, MUIA_Window_Open, TRUE);
  if (!cs_get(w, MUIA_Window_Open)) { CS_LOG("dialog: its window couldn't open (too big for the screen?)"); id = CS_ID_CANCEL; done = 1; }
  while (!done) {
    id = DoMethod(cs_app, MUIM_Application_NewInput, (ULONG)&sigs);
    if (id == CS_ID_CANCEL || id == MUIV_Application_ReturnID_Quit) { id = CS_ID_CANCEL; break; }
    for (i = 0; ends && ends[i]; i++) if (id == ends[i]) done = 1;
    if (id == CS_ID_OK && !ends) done = 1;
    if (done) break;
    if (each) each(id ? id : CS_ID_IDLE);
    if (sigs) sigs = Wait(sigs | SIGBREAKF_CTRL_C);
  }
  set(w, MUIA_Window_Open, FALSE);
  if (cs_win) set(cs_win, MUIA_Window_Sleep, FALSE);
  DoMethod(cs_app, OM_REMMEMBER, (ULONG)w);
  cs_redraw_window();
  cs_progress_hook = hook;
  return id;
}

/* ---------------------------------------------------------------- requesters */
int cs_ask(const char *gadgets, const char *fmt, const char *arg)
{
  char text[600], wrapped[640];
  int r;
  /* the text filled in first, so the wrapping sees what will be shown; MUI_Request then gets it
     as a plain string ("%" in a file name can't upset it) */
  snprintf(text, sizeof text, tr(fmt), arg ? arg : "");
  cs_wrap(text, wrapped, sizeof wrapped, CS_WRAP_ASK);
  r = (int)MUI_Request(cs_app, cs_win, 0, NULL, (STRPTR)tr(gadgets), (STRPTR)"%s", (ULONG)wrapped);
  cs_redraw_window();
  return r;
}
int cs_ask_string(const char *t, const char *label, char *buf, int cap)
{
  Object *str, *w;
  static const ULONG ends[] = { CS_ID_OK, 0 };
  int ok;
  w = cs_dialog_window(t, HGroup, Child, CsLabel(tr(label), MUIO_Label_SingleFrame),
                      Child, str = StringObject, StringFrame, MUIA_String_Contents, (ULONG)buf, MUIA_String_MaxLen, cap,
                                   MUIA_CycleChain, 1, MUIA_FixWidthTxt, (ULONG)"MMMMMMMMMMMMMMMMMMMM", End, End, NULL);
  if (!w) return 0;
  /* Return in the field is OK */
  DoMethod(str, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (ULONG)cs_app, 2, MUIM_Application_ReturnID, CS_ID_OK);
  set(w, MUIA_Window_ActiveObject, (ULONG)str);
  ok = cs_run_modal(w, ends, NULL) == CS_ID_OK;
  if (ok) cs_strlcpy(buf, (const char *)cs_get(str, MUIA_String_Contents), (size_t)cap);
  MUI_DisposeObject(w);
  return ok;
}
int cs_ask_number(const char *t, const char *label, LONG *v, LONG min, LONG max)
{
  char buf[24];
  LONG n;
  sprintf(buf, "%ld", (long)*v);
  if (!cs_ask_string(t, label, buf, sizeof buf)) return 0;
  n = atol(buf);
  if (n < min) n = min;
  if (n > max) n = max;
  *v = n;
  return 1;
}
int cs_file_request(int save, const char *title, char *path, int cap, const char *pattern)
{
  struct FileRequester *fr;
  char dir[300], file[108];
  int ok = 0;
  cs_strlcpy(dir, path, sizeof dir);
  *PathPart((STRPTR)dir) = 0;
  cs_strlcpy(file, path[0] ? (const char *)FilePart((STRPTR)path) : "", sizeof file);
  if (!(fr = (struct FileRequester *)MUI_AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, (ULONG)tr(title),
        ASLFR_DoSaveMode, save, ASLFR_InitialDrawer, (ULONG)dir, ASLFR_InitialFile, (ULONG)file,
        (ULONG)(pattern ? ASLFR_InitialPattern : TAG_IGNORE), (ULONG)pattern, (ULONG)(pattern ? ASLFR_DoPatterns : TAG_IGNORE), TRUE,
        (ULONG)(cs_pubscreen[0] ? ASLFR_PubScreenName : TAG_IGNORE), (ULONG)cs_pubscreen, TAG_DONE))) return 0;
  if (MUI_AslRequestTags(fr, ASLFR_Window, cs_get(cs_win, MUIA_Window_Window), TAG_DONE)) {
    cs_strlcpy(path, (const char *)fr->fr_Drawer, (size_t)cap);
    ok = AddPart((STRPTR)path, fr->fr_File, (ULONG)cap) && fr->fr_File[0];
  }
  MUI_FreeAslRequest(fr);
  cs_redraw_window();
  return ok;
}

/* ---------------------------------------------------------------- gadgets and the Zune fallbacks */
int cs_radio_is_cycle;
Object *cs_radio(const char *const *entries)
{
  Object *o = cs_radio_is_cycle ? NULL : MUI_NewObject((STRPTR)MUIC_Radio, MUIA_Radio_Entries, (ULONG)tr_list(entries), MUIA_CycleChain, 1, TAG_DONE);
  if (o) return o;
  cs_radio_is_cycle = 1;                       /* (from now on: all of them, so reading them is the same) */
  CS_LOG("dialog: no Radio class (Zune?): a cycle gadget instead");
  return cs_cycle(entries);
}
Object *cs_cycle(const char *const *entries)
{
  return CycleObject, MUIA_Cycle_Entries, (ULONG)tr_list(entries), MUIA_CycleChain, 1, End;
}
Object *cs_check(const char *label, int on, Object **check)
{
  Object *c = MUI_MakeObject(MUIO_Checkmark, (ULONG)tr(label));
  if (c) { set(c, MUIA_Selected, on ? TRUE : FALSE); set(c, MUIA_CycleChain, 1); }
  if (check) *check = c;
  return HGroup, Child, c, Child, CsLabel(tr(label), MUIO_Label_LeftAligned | MUIO_Label_SingleFrame), Child, HSpace(0), End;
}
Object *cs_string(const char *contents, int maxlen)
{
  return StringObject, StringFrame, MUIA_String_Contents, (ULONG)(contents ? contents : ""), MUIA_String_MaxLen, maxlen, MUIA_CycleChain, 1, End;
}

/*
 * cs_common.h - (private to the kit's classes) the cases every themed class's dispatcher has the
 * same: Setup takes the theme (and adds the event handler, if the class has one), Cleanup gives it
 * back, Show and Hide keep the "shown" flag that every draw checks. A class's instance data starts
 * with CS_CLASS_HEAD so these can reach it.
 */
#ifndef CS_COMMON_H
#define CS_COMMON_H

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>
#include "cs_theme.h"
#include "cs_icon.h"

#define CS_CLASS_HEAD struct MUI_EventHandlerNode ehn; int shown, has_ehn

/* in a dispatcher's switch, for instance data type T */
#define CS_CLASS_COMMON(T) \
  case MUIM_Setup: \
    if (!DoSuperMethodA(cl, obj, msg)) return FALSE; \
    cs_theme_setup(obj); \
    if (((T *)INST_DATA(cl, obj))->has_ehn) cs_theme_handler(obj, &((T *)INST_DATA(cl, obj))->ehn); \
    return TRUE; \
  case MUIM_Cleanup: cs_theme_cleanup(obj); break; \
  case MUIM_Show: ((T *)INST_DATA(cl, obj))->shown = 1; break; \
  case MUIM_Hide: ((T *)INST_DATA(cl, obj))->shown = 0; break

/* the event handler node set up in OM_NEW for the IDCMP classes ev */
#define CS_EHN(d, ev) do { (d)->ehn.ehn_Object = obj; (d)->ehn.ehn_Class = cl; (d)->ehn.ehn_Events = (ev); (d)->has_ehn = 1; } while (0)

/* the mouse inside the object's inner box */
#define CS_INSIDE(obj, im) ((im)->MouseX >= _mleft(obj) && (im)->MouseX <= _mright(obj) && (im)->MouseY >= _mtop(obj) && (im)->MouseY <= _mbottom(obj))

/* MUIM_AskMinMax's sizes added in one line */
#define CS_MINMAX(msg, minw, defw, maxw, minh, defh, maxh) do { \
  struct MUI_MinMax *mm_ = ((struct MUIP_AskMinMax *)(msg))->MinMaxInfo; \
  mm_->MinWidth += (minw); mm_->DefWidth += (defw); mm_->MaxWidth += (maxw); \
  mm_->MinHeight += (minh); mm_->DefHeight += (defh); mm_->MaxHeight += (maxh); } while (0)

#define CS_DISPATCHER(name) static ULONG name(__reg("a0") struct IClass *cl, __reg("a2") Object *obj, __reg("a1") Msg msg)

#endif

/*
 * cs_dialog.h - dialogs in the family's way: MUI's own gadgets with themed buttons and labels, one
 * modal pattern (cs_dialog_window + cs_run_modal), requesters wrapped at 66 characters, ASL file
 * requesters, and the fallbacks Zune needs. docs/platforms/amiga/06-dialogs.md has the rules
 * (layout grid, button order, Escape and Return, fitting 480 lines).
 */
#ifndef CS_DIALOG_H
#define CS_DIALOG_H

#include "cs_amiga.h"

/* a flat dialog button in the theme: "_Reset" underlines R and makes R press it. MUI's plain
   button if the class can't be made */
Object *CsButton(const char *label);
/* a label beside a gadget (MUIO_Label_ flags): MUI's own in the light theme, ours in the dark
   one, where MUI's black text can't be read */
Object *CsLabel(const char *label, ULONG flags);
Object *CsTitle(const char *title);             /* a section's title, bold */
void cs_dialog_classes_free(void);

/* a dialog window: title (English: translated here), contents, the OK button's label (NULL "OK"),
   OK and Cancel at the bottom right (OK first, as the family has it; the platform chapter
   says where others differ), centred over the main window. Esc and the close gadget cancel.
   NULL if it can't be made (contents are then disposed) */
Object *cs_dialog_window(const char *title, Object *contents, const char *oklabel);
/* the same with a third button left of OK (Reset, Apply...): it returns CS_ID_EXTRA */
Object *cs_dialog_window3(const char *title, Object *contents, const char *oklabel, const char *extra);
/* the window run: added to the application, the main window asleep, its own input loop until one
   of ends (0-ended) arrives, or Cancel. each(id) sees every other id, and CS_ID_IDLE when nothing
   is happening (live previews catch up then). The window is closed and removed, not disposed:
   read the gadgets, then MUI_DisposeObject it. Returns the id that ended it */
ULONG cs_run_modal(Object *win, const ULONG *ends, void (*each)(ULONG id));
/* the main window drawn again throughout: MUI doesn't always repair what a closing window
   uncovered over custom-drawn objects. Every requester and dialog here does it for you */
void cs_redraw_window(void);

/* a requester: gadgets "OK|Cancel" (translated; the last is the 0 answer), the text a printf format
   with one string (translated, then wrapped at 66 characters). Returns MUI_Request's answer */
int cs_ask(const char *gadgets, const char *fmt, const char *arg);
/* one line of text or a number */
int cs_ask_string(const char *title, const char *label, char *buf, int cap);
int cs_ask_number(const char *title, const char *label, LONG *v, LONG min, LONG max);
/* the ASL file requester, starting at path's drawer with its file; path is replaced by the choice.
   pattern: "#?.txt" or NULL. 1 if a file was chosen */
int cs_file_request(int save, const char *title, char *path, int cap, const char *pattern);

/* Zune fallbacks (found in the emulator: AROS's Zune has no Radio class and no Scrollgroup or
   Virtgroup in some versions). cs_radio gives MUI's radio buttons, or a cycle gadget where Radio
   can't be made; read and set it with CS_RADIO_ACTIVE. entries: NULL-ended, English */
Object *cs_radio(const char *const *entries);
extern int cs_radio_is_cycle;
#define CS_RADIO_ACTIVE (cs_radio_is_cycle ? MUIA_Cycle_Active : MUIA_Radio_Active)
/* a cycle gadget, translated */
Object *cs_cycle(const char *const *entries);
/* a check mark with its label on its right, in one row */
Object *cs_check(const char *label, int on, Object **check);
/* a string gadget, in the cycle chain */
Object *cs_string(const char *contents, int maxlen);

#endif

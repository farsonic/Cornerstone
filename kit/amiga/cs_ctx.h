/*
 * cs_ctx.h - the right mouse button's menus. What a context menu offers is a list of command ids
 * (0: a separator line), so it does nothing the menu bar doesn't, with the same words, shortcuts
 * and state. A class that has one calls cs_ctx_attach() in MUIM_Setup, answers MUIM_ContextMenuBuild
 * with cs_ctx_menu() (having first made what was clicked current, as a left click would) and
 * MUIM_ContextMenuChoice with cs_ctx_choice(), which hands the chosen command to the main loop.
 * Deciding the list is plain C in the program's core, so it can be tested natively.
 */
#ifndef CS_CTX_H
#define CS_CTX_H

#include "cs_amiga.h"

void    cs_ctx_attach(Object *obj);                       /* MUI then asks obj for its menu */
Object *cs_ctx_menu(const char *title, const unsigned long *ids, int n);   /* the strip to show (kept until the next) */
ULONG   cs_ctx_choice(Object *item);                      /* the command chosen, sent as a ReturnID too */
void    cs_ctx_free(void);                                /* after the application is disposed */

#endif

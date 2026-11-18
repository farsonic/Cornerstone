/*
 * cs_menu.h - the menu bar made from the command registry (kit/shared/cs_cmd.h): a menu layout
 * list becomes an MUI menu strip (MUIO_MenustripNM), each item with its command's translated label,
 * shortcut and tick; cs_menu_update() then enables, disables and ticks every item from the same
 * cs_cmd_state() the ribbon asks, so the two can never disagree. A command's id is its menu
 * item's user data: choosing it arrives in the main loop as that id.
 */
#ifndef CS_MENU_H
#define CS_MENU_H

#include "cs_amiga.h"
#include "cs_cmd.h"

/* the strip for layout (translated now: make it again after the language changes), NULL if it
   can't be made. The layout is checked first (cs_menu_check); problems go to the log */
Object *cs_menu_strip(const CsMenuItem *layout);
void    cs_menu_update(Object *strip);           /* every item's enabled and ticked state */
void    cs_menu_set_label(Object *strip, ULONG id, const char *label);   /* a dynamic label (recent files) */
void    cs_menu_free(void);                      /* the labels' memory, after the strip is disposed */

#endif

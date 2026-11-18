# 5. Menus and context menus

## The menu strip from the command table

```c
strip = cs_menu_strip(jotter_menus);          /* before the window: MUIA_Window_Menustrip, strip */
...
cs_menu_update(strip);                         /* after every command: enabled and ticked states */
```

`kit/amiga/cs_menu.c` turns the program's menu layout (`CsMenuItem`, shared chapter 3) into a
`struct NewMenu` table, translated now (so after a change of language the strip is made again, at
the next start), and gives it to `MUI_MakeObject(MUIO_MenustripNM, ...)`. Each item's label is its
command's (`tr()`), its `nm_CommKey` the command's letter (shown as Amiga-letter), `CHECKIT |
MENUTOGGLE` for `CSC_TOGGLE` commands, and **its command id as user data**: choosing it arrives in
the main loop as that id, the same number a ribbon button sends. The layout is checked first
(`cs_menu_check`): problems go to the debug log.

`cs_menu_update()` finds each item with `MUIM_FindUData` and sets `MUIA_Menuitem_Enabled` and
`MUIA_Menuitem_Checked` from `cs_cmd_state()` (with `nnset` for the tick, so it doesn't report
itself), only where they differ. **Disable, don't hide.** Dynamic labels (a recent-files list) are
set with `cs_menu_set_label(strip, id, label)`.

## Intuition's limits

A menu strip can have **31 menus**, **63 items in a menu** (separators count) and **31 items in a
submenu**; past them items are silently lost. `cs_menu_check()` counts them (Jotter's tests run it),
and Quill's debug log counts its 805 entries at every start. A long menu is split with submenus
(Jotter's Edit › Lines) the way the upstream program groups them.

The menus must also fit the screen: `langfit.py` lays them out in topaz 8 (8 pixels a letter) in
every language and checks the titles fit a 640-pixel bar and each menu (with its widest submenu)
the screen.

## Conventions

- Menu titles and order follow the upstream program: File, Edit, then its tabs or menus.
- "..." after a label means a dialog comes first.
- Amiga-key shortcuts for the common commands (N, O, S, Q, Z, Y, X, C, V, A, F, G, R...): one letter
  per command, checked unique by `cs_menu_check`.
- The menus hold every command, including those the ribbon has; that is the rule, not duplication.

## Context menus

```c
case MUIM_Setup:  ... cs_ctx_attach(obj); ...         /* MUI asks this object for a context menu */
case MUIM_ContextMenuBuild: {
  struct MUIP_ContextMenuBuild *m = (struct MUIP_ContextMenuBuild *)msg;
  /* what was clicked becomes current first, as a left click would make it */
  ...
  return (ULONG)cs_ctx_menu("Text", jotter_ctx_view, jotter_nctx_view);
}
case MUIM_ContextMenuChoice:
  cs_ctx_choice(((struct MUIP_ContextMenuChoice *)msg)->item);   /* the id, as a ReturnID */
  return 0;
```

`cs_ctx_attach` sets `MUIA_ContextMenu` to an empty strip (MUI only asks an object that has one);
`cs_ctx_menu` builds the real strip from a list of command ids (0 is a separator line), each with
its command's label, shortcut, enabled state and tick; `cs_ctx_choice` reads the chosen item's user
data and sends it to the main loop. The strips are not disposed with their objects: `cs_ctx_free()`
after the application. What a context menu holds is plain C (Jotter's list is in `ui/commands.c`;
Easel's `ctxmenu.c` chooses among many by tool and state, tested natively by `tests/ctxtest.c`).

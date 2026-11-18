# 2. The application skeleton

`main()` follows the same order in every program. Jotter's (`example/amiga/main.c`, the end of the
file) is the shortest complete one; `kit/amiga/cs_app.[ch]` holds the reusable parts.

## Starting

```c
int main(int argc, char **argv)
{
  CsArgs args;
  if (!cs_libs_open("Jotter")) goto done;           /* 1. libraries */
  cs_args(argc, argv, &args);                       /* 2. arguments (Shell or Workbench), the log */
  prefs_read();                                     /* 3. PROGDIR:Jotter.prefs */
  cs_lang_init(words, nwords);                      /* 4. the language */
  cs_lang_choose(args.language);
  cs_theme_set_dark(prefs.dark);                    /* 5. the theme, before any object is set up */
  jotter_commands_init();                           /* 6. the command table, the state function */
  cs_cmd_set_state(state);
  cs_fonts_assign();                                /* 7. PROGDIR:Fonts added to FONTS: */
  if (!cs_classes_make() || !view_class()) ...      /* 8. every custom class, up front */
  app = ApplicationObject, ... End;                 /* 9. the application, the menus, the window */
  cs_app_set(app, NULL);
  strip = cs_menu_strip(jotter_menus);
  win = make_window();
  DoMethod(app, OM_ADDMEMBER, (ULONG)win);
  cs_app_set(app, win);
  if (args.nfiles) load(args.files[0]);             /* 10. the files given */
  set(win, MUIA_Window_Open, TRUE);
  refresh_all();
  if (args.cmd) jotter_command(args.cmd, -1);       /* 11. CMD=n, for testing */
  ...the loop...
```

1. **Libraries** (`cs_libs_open`): intuition, graphics, layers, utility (39) and keymap (37), then
   muimaster.library 19 (MUI 3.8). If one is missing the user is told in the Shell, or with an
   Intuition requester when started from Workbench (MUI may be what is missing), and the program
   ends. Optional ones (diskfont, cybergraphics, icon) may fail: the code checks for `NULL` where
   it uses them. vbcc's startup code doesn't open libraries for us: `cs_app.c` defines the bases
   (`IntuitionBase`, `GfxBase`, `MUIMasterBase`...).
2. **Arguments** (`cs_args`): the family's `KEY=value` style, the same from the Shell and as
   Workbench tool types: `Program [file...] PUBSCREEN=name LANGUAGE=de DEBUG=file CMD=n`. From
   Workbench, the program icon's tool types are read and the project icons given with it (shift-
   clicked, or the program as a document's default tool) become the files. A public screen that
   isn't there falls back to the default one.
3. **Preferences**: `PROGDIR:Program.prefs`, plain `key=value` lines (`cs_prefs_*` in
   `kit/shared/cs_util.h`). Unknown keys are kept and written back, so an older version doesn't
   lose a newer one's settings. Read them before anything that depends on them (language, theme,
   large text).
4. **Language**: the `LANGUAGE=` argument, else the prefs' `language`, else the first of Locale
   prefs' languages that the program has (`cs_lang_system`), else English.
5. **Theme**: dark or light and large text are set before the first object is set up, since that
   is when pens are taken and fonts opened.
6. **The command table** registered and the state function set (`docs/shared/03-commands.md`).
7. **Fonts**: `PROGDIR:Fonts` is added to `FONTS:` with `AssignAdd` (other programs then find
   QuillSans too; the kit loads its own fonts from `PROGDIR:Fonts` itself, chapter 3).
8. **Classes**: each kit class file has a `cs_<name>_class()` that makes its class on first call;
   `cs_classes_make()` calls them all up front so a failure stops the program at once, with a
   message, instead of a window with holes. The program's own classes the same way.
9. **The application and window**: `MUIA_Application_Base` gives the ARexx port name (the family
   adds no ARexx commands, but MUI makes the port); the window fills the screen below its title bar
   (`make_window`, chapter 4 and the shared window chapter).
10. **Files**: opened before the window opens, so the first thing drawn is the document.
11. **`CMD=n`**: runs command n once everything is up, so a test can open any dialog without the
    mouse (chapter 12).

Bigger programs add a **splash window** (a plain Intuition window with a progress bar: MUI isn't
running yet) while fonts are scanned and classes made (`easel/amiga/splash.c`), and warn once when
the fonts drawer is missing (Jotter does that with a requester after its window opens).

## The main loop

```c
for (;;) {
  id = cs_input(&sigs);                        /* MUIM_Application_NewInput, then Wait (with Ctrl-C) */
  if (id == MUIV_Application_ReturnID_Quit || id == ID_QUIT) { if (may_discard()) break; continue; }
  if (!id) continue;
  if (id >= ID_VIEWCHG) { report(id); continue; }   /* a class reporting, not a command */
  value = cs_get(ribbon, MUIA_CsRibbon_Value);       /* a ribbon list's entry, -1 none */
  set(ribbon, MUIA_CsRibbon_Value, -1);
  set(app, MUIA_Application_Sleep, TRUE);            /* the busy pointer */
  cs_job_begin(label);                               /* progress and Abort after half a second */
  jotter_command(id, value);
  cs_job_end();
  set(app, MUIA_Application_Sleep, FALSE);
}
```

Everything arrives as an id: menu items carry their command as user data; the ribbon, the File
page, the status bar and the view report through notifications that return ids
(`MUIM_Application_ReturnID`). One `command(id, value)` function handles all of them; afterwards
it calls `refresh_all()`, which updates the title, the status bar, the panes, and asks the menus
and ribbon to show the new states.

## Ending

In reverse, once the user has saved or let go of the changes:

```c
prefs_write();
if (app) { set(win, MUIA_Window_Open, FALSE); MUI_DisposeObject(app); }   /* every window and object */
cs_ctx_free();            /* the context menu strips (not disposed with their objects) */
cs_classes_free();        /* the kit's classes: after every object of them is gone */
view_class_free();        /* the program's own */
cs_menu_free();           /* the menu table's memory */
cs_theme_free();          /* the fonts the loader holds */
cs_jobs_free();           /* the keyboard device */
cs_fonts_unassign();
cs_libs_close();          /* closes the log too */
```

Every pen taken is given back by the objects' cleanup (chapter 3); every class freed after the
application object, never before.

## Screens and windows

- The main window opens on a **public screen**: Workbench by default, or the one named by
  `PUBSCREEN=` / the tool type. `CsWindowObject` (cs_amiga.h) adds `MUIA_Window_PublicScreen` when
  one is named; use it for every window and dialog so they all appear on the same screen. Easel
  can also open a screen of its own (window, own public screen or full screen: its prefs).
- **One window per document**, filling the screen below its title bar
  (`MUIA_Window_Width/Height/LeftEdge/TopEdge` from the screen's size); `MUIA_Window_ID` lets MUI
  remember where the user moved it.
- **Floating windows** only for things opened on purpose (a palette, brush settings, Tell Me).

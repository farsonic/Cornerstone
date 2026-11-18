# Cornerstone's kit

Copy this directory into a program as `kit/` and keep it as it is.

- `shared/`: portable C99 (no platform calls) and the tools that work on any platform's program.
  `cs_cmd` (the command registry, search, menu check), `cs_lang` (`tr`, `tr_list`, five
  languages), `cs_util` (preferences, progress, wrapping, KeyTips, `cs_muldiv`), `cs_icon` and
  `cs_iconset` (the icon language), `cs_ribbon` (the ribbon as data and its layout). `tests/` has
  their tests; `lang/` the kit's own strings; `tools/` mklang, langcheck, gapcount, iconsheet,
  csconfig; `templates/` CLAUDE.md, CI workflows, issue forms.
- `amiga/`: the MUI kit (theme, classes, dialogs, jobs, menus, context menus, start-up), the screen
  fonts, and the Amiga tools (ribbonfit, langfit, mkicons, checkinst, srcfix, mkpackage.sh, the FPU
  vbcc config).

The guide (`../docs/`) explains every file; `../example/` (Jotter) uses all of it.

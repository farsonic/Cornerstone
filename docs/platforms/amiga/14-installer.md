# 14. Installer scripts

Every package has an Installer script (`tools/Install-<Program>`, Jotter's is
`example/tools/Install-Jotter`). It must run on three interpreters that read the same language
differently: **Commodore's Installer 43** (AmigaOS 3.x), **InstallerLG** (its open
reimplementation, AmigaOS 3.2 and 4) and **AROS's Installer**. The rules below were found by
running the family's scripts on all three (Install-Commodities on AROS first).

## The rules

1. **No `(procedure ...)`**: AROS's Installer doesn't understand it. Repeat the few lines instead.
2. **No `(select n ...)` for values**: AROS returns the index, not the element. Use
   `(set #x ...)` and `(if (= #n 1) ...)`.
3. **No `(and ...)` in conditions**: not logical on AROS. Nest `if`s:
   `(if #v43 (if #c40 (set #def (+ #def 4))))`.
4. **One action per `if`**: keep each branch to a single call.
5. **`askoptions` bits tested with `(IN mask n)`**.
6. **Sources through `#src`**: Workbench may start Installer with a current directory other than
   the script's drawer, so every relative `(source "x")` becomes `(source (tackon #src "x"))`, with
   `#src` the drawer of the icon that started it:

   ```lisp
   (if (= @icon "") (set #src "") (set #src (pathonly @icon)))
   ```
7. **Balanced parentheses and strings**, comments with `;`.
8. **Check what the machine has** before offering what needs it (`getversion` of a library,
   `(database "cpu")`), and say why something isn't offered.
9. **`$VER:`** on the first line, the version in a variable, `@app-name` set if empty, and a
   `welcome` that says what will be installed and what the program needs.

## The tools

- `kit/amiga/tools/checkinst.py SCRIPT...`: the static checks of rules 1, 2, 3, 6 and 7 (exit 1 on
  any problem). The tests run it on Jotter's script.
- `kit/amiga/tools/srcfix.py SCRIPT...`: rewrites every relative `(source "x")` to go through
  `#src` and adds the line that sets it. Idempotent.
- Generating a script from what is in the package (the Utilities' `tools/mkinstaller.sh`) keeps a
  script with many optional parts honest: a datatype that wasn't bundled isn't offered.

## Jotter's script

It asks where the Jotter drawer goes (a drawer called Jotter is made there), what to install
(Jotter with its fonts and example; the source), copies with `(infos)` so the icons come along,
warns if MUI is older than 3.8 but installs anyway, and ends by saying where Jotter is. Read it
whole in `example/tools/Install-Jotter`; it passes `checkinst.py`.

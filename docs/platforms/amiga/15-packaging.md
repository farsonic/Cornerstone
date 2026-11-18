# 15. Packaging

Shared chapter 13 has what a package holds and the release checklist. On the Amiga a package is an
`.lha` archive (level-2 headers) of one drawer:

```
Jotter.info                 the drawer's icon
Jotter/
  Jotter, Jotter.info       the program
  Install-Jotter(.info)     the Installer script
  README.md(.info), LICENSE(.info), Welcome.txt(.info)
  Fonts/(.info)             QuillSans, QuillSansB, QuillMono with LICENSE-fonts.txt, README-fonts.md
  Source/(.info)            amiga/ core/ ui/ lang/ tests/ tools/ sample/ build.sh cornerstone.json kit/
```

Jotter's `example/tools/package.sh VERSION WORKDIR` stages that drawer (the program from the build,
the fonts from the kit, the documents, the Installer script checked with `checkinst.py`, the source
with the kit copied in and its paths set for a tree of its own), makes the icons
(`mkicons.py`), then hands the staging directory to the kit's last step:

```sh
sh kit/amiga/tools/mkpackage.sh STAGE OUT.lha
```

which:

1. searches the staged files for **certificate or key material** (text matching `BEGIN
   CERTIFICATE`, `PRIVATE KEY` or the sandbox's marker; files named `*.crt`, `*.pem`, `*.key`,
   `*.p12`, `*.pfx`) and **refuses** if it finds any;
2. moves an older archive of that name aside, and makes the archive with `jlha a2o5q` (or `lha`);
3. unpacks the archive somewhere else and **searches it again**, renaming it `.refused` if anything
   turns up;
4. reports the number of files and the size.

Its own search patterns are written so they don't match themselves (`BEGIN [C]ERTIFICATE`): the
script ships in `Source/`, and the first version refused its own package. Nothing is deleted: old
archives and check directories are moved aside.

Packages are attached to GitHub releases, never committed. Programs that use more fonts than the
kit's (Quill's whole QuillSerif/Sans/Mono family, the TrueType drawer for PDF export) take them from
their own tree; Easel's `tools/package.sh` takes the Fonts and TrueType drawers from the previous
package's work drawer.

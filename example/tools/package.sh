#!/bin/sh
# package.sh VERSION WORKDIR - makes WORKDIR/Jotter-VERSION.lha: the Jotter drawer (the program
# built by build.sh, its Fonts drawer, the README, the licence, the example document, the
# Installer script, the source with the kit it is built on), each with its Workbench icon, then
# kit/amiga/tools/mkpackage.sh checks it for key material and makes the archive.
# SPDX-License-Identifier: GPL-2.0-only
set -e
cd "$(dirname "$0")/.."
ver=$1; work=$2
[ -n "$ver" ] && [ -n "$work" ] || { echo "usage: tools/package.sh VERSION WORKDIR" >&2; exit 2; }
KIT=${KIT:-../kit}
BIN=${BIN:-${TMP:-/tmp}/jotter-build/Jotter}
[ -x "$BIN" ] || { echo "package: build Jotter first (sh build.sh): $BIN" >&2; exit 1; }
stage=$work/stage-$ver
[ -e "$stage" ] && mv "$stage" "$stage.old.$$"
d=$stage/Jotter
mkdir -p "$d/Source/kit"
cp "$BIN" "$d/Jotter"
cp -r "$KIT/amiga/fonts" "$d/Fonts"
mv "$d/Fonts/README.md" "$d/Fonts/README-fonts.md"
cp README.md "$d/README.md"
cp ../LICENSE "$d/LICENSE"
cp sample/Welcome.txt "$d/Welcome.txt"
cp tools/Install-Jotter "$d/Install-Jotter"
python3 "$KIT/amiga/tools/checkinst.py" "$d/Install-Jotter"
# the source: Jotter's own, and the kit (without its fonts, which are in Fonts already)
tar --exclude=__pycache__ -cf - amiga core ui lang tests tools sample build.sh cornerstone.json | (cd "$d/Source" && tar -xf -)
(cd "$KIT/.." && tar --exclude=__pycache__ --exclude=kit/amiga/fonts -cf - kit) | (cd "$d/Source" && tar -xf -)
# (the source tree there refers to the kit as kit/: the paths in cornerstone.json and build.sh)
sed -i 's#\.\./kit/#kit/#g' "$d/Source/cornerstone.json"
python3 "$KIT/amiga/tools/mkicons.py" "$stage" tools/wbicons.py "$work/wb-icons.png"
sh "$KIT/amiga/tools/mkpackage.sh" "$stage" "$(cd "$work" && pwd)/Jotter-$ver.lha"

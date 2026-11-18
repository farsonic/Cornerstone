#!/bin/sh
# Build Jotter, Cornerstone's example program, for AmigaOS 3.x (vbcc + NDK 3.2 + MUI 3.8 headers):
# first the native tests (sh tools/test.sh), then the Amiga binary. "sh build.sh fast" skips the tests.
# Jotter does no floating point, so it is built for a 68020 without an FPU and runs on any Amiga
# with AmigaOS 3.0 and MUI; a program that needs an FPU uses kit/amiga/tools/vbcc-aos68k-fpu
# instead (docs/platforms/amiga/01-toolchain.md).
set -e
export VBCC=${VBCC:-/opt/amiga/vbcc}
export PATH=$VBCC/bin:$PATH
NDK=${NDK:-/opt/amiga/ndk/Include_H}
MUI=${MUI:-/opt/amiga/mui/include}
cd "$(dirname "$0")"
KIT=${KIT:-../kit}
[ "$1" = fast ] || sh tools/test.sh
OUT=${OUT:-${TMP:-/tmp}/jotter-build}
mkdir -p "$OUT"
vc +aos68k -c99 -O1 -cpu=68020 -I"$KIT/shared" -I"$KIT/amiga" -I"$MUI" -I"$NDK" \
   amiga/*.c ui/*.c core/*.c "$KIT"/amiga/*.c "$KIT"/shared/cs_*.c -o "$OUT/Jotter" -lamiga
ls -l "$OUT/Jotter"

#!/bin/sh
# Jotter's native tests, what CI runs: the interface words (the table as the strings files make it,
# every string translated and every row used), the kit's own tests, then the core and interface
# data tests, all with the address and undefined-behaviour sanitizers. TMP=<dir> to build elsewhere.
set -e
cd "$(dirname "$0")/.."
T=${TMP:-/tmp}
KIT=${KIT:-../kit}
python3 "$KIT/shared/tools/mklang.py" --check
python3 "$KIT/shared/tools/langcheck.py" > "$T/jotter_langcheck.txt" || { cat "$T/jotter_langcheck.txt"; exit 1; }
tail -1 "$T/jotter_langcheck.txt"
CC="cc -std=c99 -O1 -g -Wall -Wextra -fsanitize=address,undefined -I$KIT/shared"
$CC "$KIT"/shared/cs_*.c "$KIT/shared/tests/kittest.c" -o "$T/jotter_kittest"
"$T/jotter_kittest" "$T" | tail -1
$CC core/*.c ui/*.c "$KIT"/shared/cs_*.c tests/coretest.c -o "$T/jotter_coretest"
"$T/jotter_coretest" "$T" | tail -1
# the fit at 640x480 with the real screen font: the ribbon in every language, the menus
python3 "$KIT/amiga/tools/ribbonfit.py" | tail -1
python3 "$KIT/amiga/tools/langfit.py" | tail -1

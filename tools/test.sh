#!/bin/sh
# Cornerstone's native tests (what CI runs): the kit's own tests, the example program's tests (its
# words, core, interface data and fit checks: example/tools/test.sh), the icon language's sheet
# drawn, the Workbench icons written and read back, and the example's Installer script checked.
# TMP=<dir> to build somewhere other than /tmp.
set -e
cd "$(dirname "$0")/.."
T=${TMP:-/tmp}
CC="cc -std=c99 -O1 -g -Wall -Wextra -fsanitize=address,undefined -Ikit/shared"
echo "== kit"
$CC kit/shared/cs_*.c kit/shared/tests/kittest.c -o "$T/cs_kittest"
"$T/cs_kittest" "$T" | tail -1
python3 kit/shared/tools/iconsheet.py "$T/cs-icon-sheet.png" --root example | tail -1
python3 kit/amiga/tools/checkinst.py example/tools/Install-Jotter
echo "== example (Jotter)"
TMP=$T sh example/tools/test.sh
echo "== Workbench icons"
mkdir -p "$T/cs-wbicons/Jotter/Fonts" "$T/cs-wbicons/Jotter/Source"
python3 kit/amiga/tools/mkicons.py "$T/cs-wbicons" example/tools/wbicons.py | tail -1
python3 - "$T/cs-wbicons" <<'PY'
import sys; sys.path.insert(0, 'kit/amiga/tools'); import mkicons, os
r = [mkicons.read_info(os.path.join(sys.argv[1], f)) for f in ('Jotter.info', 'Jotter/Jotter.info', 'Jotter/Install-Jotter.info')]
ok = all(x['magic'] == 0xE310 and x['glow'] for x in r) and r[0]['kind'] == 2 and r[1]['kind'] == 3 and r[2]['tool'] == 'Installer'
print('wbicons: read back %s' % ('ok' if ok else 'WRONG')); sys.exit(0 if ok else 1)
PY
echo "all native tests passed"

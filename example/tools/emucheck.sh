#!/bin/sh
# emucheck.sh - Jotter in the emulator, with screenshots (docs/platforms/amiga/12-testing.md).
# Run it under the shared emulator lock:
#   flock -w 5400 /tmp/lagoon-emu.lock sh example/tools/emucheck.sh
# with EMU (the FS-UAE setup's directory: its .fs-uae config and its hd/ drawer), FSUAE (the
# fs-uae binary), BIN (Jotter, built by build.sh) and W (a work directory) set. It puts a test drawer
# DH0:Cornerstone on the emulator's hard disk, a startup-sequence that runs Jotter several times
# (English, German, dark, right to left; then Replace and Options with CMD=), drives KeyTips, the
# File page and Tell Me with keys, takes a screenshot as each window comes up, then restores the
# startup-sequence, stops only its own emulator and display, and moves the drawer aside.
# Needs Xvfb, xdotool, ImageMagick's import and Python 3 with Pillow. SPDX-License-Identifier: GPL-2.0-only
set -u
E=${EMU:?set EMU to the FS-UAE setup}
HD=$E/hd
W=${W:-${TMP:-/tmp}/jotter-emu}
SHOTS=$W/shots
REPO=$(cd "$(dirname "$0")/../.." && pwd)
CONF=${CONF:-$(ls $E/*.fs-uae | head -1)}
FSUAE=${FSUAE:-fs-uae}
BIN=${BIN:-${TMP:-/tmp}/jotter-build/Jotter}
DPY=${DPY:-:97}
mkdir -p $SHOTS
D=$HD/Cornerstone
[ -e $D ] && mv $D $W/old-drawer.$$
mkdir -p $D
for v in Light German Dark RTL; do
  mkdir -p $D/$v
  cp "$BIN" $D/$v/Jotter
  cp -r $REPO/kit/amiga/fonts $D/$v/Fonts
  cp $REPO/example/sample/Welcome.txt $D/$v/Welcome.txt
done
printf 'dark=1\n' > $D/Dark/Jotter.prefs
printf 'rtl=1\n' > $D/RTL/Jotter.prefs
printf 'language=de\n' > $D/German/Jotter.prefs
cp $HD/S/startup-sequence $W/startup-sequence.saved
cat > $HD/S/startup-sequence <<'SEQ'
FailAt 30
Stack 65536
Run >NIL: DH0:Cornerstone/Light/Jotter DH0:Cornerstone/Light/Welcome.txt DEBUG=DH0:Cornerstone/l1.log
Wait 50
Run >NIL: DH0:Cornerstone/German/Jotter DH0:Cornerstone/German/Welcome.txt DEBUG=DH0:Cornerstone/g1.log
Wait 30
Run >NIL: DH0:Cornerstone/Dark/Jotter DH0:Cornerstone/Dark/Welcome.txt DEBUG=DH0:Cornerstone/d1.log
Wait 30
Run >NIL: DH0:Cornerstone/RTL/Jotter DH0:Cornerstone/RTL/Welcome.txt DEBUG=DH0:Cornerstone/r1.log
Wait 30
Run >NIL: DH0:Cornerstone/Light/Jotter DH0:Cornerstone/Light/Welcome.txt CMD=210 DEBUG=DH0:Cornerstone/f1.log
Wait 25
Run >NIL: DH0:Cornerstone/Light/Jotter DH0:Cornerstone/Light/Welcome.txt CMD=106 DEBUG=DH0:Cornerstone/o1.log
Wait 25
Run >NIL: DH0:Cornerstone/Light/Jotter DH0:Cornerstone/Light/Welcome.txt CMD=106 LANGUAGE=de DEBUG=DH0:Cornerstone/o2.log
Wait 25
Echo >DH0:Cornerstone/done "done"
SEQ
export DISPLAY=$DPY
Xvfb $DPY -screen 0 1280x1024x24 >/dev/null 2>&1 &
XPID=$!
sleep 2
sed "s#^hard_drive_0 = .*#hard_drive_0 = $HD#" "$CONF" > $W/cs.fs-uae
cd $E && SDL_AUDIODRIVER=dummy $FSUAE $W/cs.fs-uae > $W/fsuae.log 2>&1 &
UPID=$!
shot() { DISPLAY=$DPY import -window root $W/full.png 2>/dev/null; python3 -c "
from PIL import Image
a=Image.open('$W/full.png').convert('RGB').crop((0,32,1280,992)).resize((640,480),Image.NEAREST); a.save('$SHOTS/$1.png')"; }
waitlog() { i=0; while [ ! -s $D/$1 ] && [ $i -lt 240 ]; do sleep 1; i=$((i+1)); done; sleep ${2:-12}; }
key() { DISPLAY=$DPY xdotool key $1; sleep ${2:-2}; }
waitlog l1.log 14; shot 01-home
key F10 3; shot 02-keytips
key i 3; shot 03-keytips-insert
key Escape 1; key Escape 2; shot 04-insert
key F10 2; key v 2; key Escape 1; key Escape 2; shot 05-view
key F10 2; key f 4; shot 06-filepage
key Escape 3
key F10 2; key q 4; DISPLAY=$DPY xdotool type --delay 300 "save"; sleep 3; shot 07-tellme
key Escape 2
waitlog g1.log 12; shot 08-german
waitlog d1.log 12; shot 09-dark
waitlog r1.log 12; shot 10-rtl
waitlog f1.log 10; shot 11-replace
key Escape 2
waitlog o1.log 10; shot 12-options
key Escape 2
waitlog o2.log 10; shot 13-options-german
i=0; while [ ! -e $D/done ] && [ $i -lt 120 ]; do sleep 1; i=$((i+1)); done
cp $W/startup-sequence.saved $HD/S/startup-sequence
kill $UPID 2>/dev/null; sleep 2; kill -9 $UPID 2>/dev/null
kill $XPID 2>/dev/null
cp $D/*.log $W/ 2>/dev/null
mv $D $W/drawer.$$
echo emu done

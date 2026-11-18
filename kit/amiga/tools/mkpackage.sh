#!/bin/sh
# mkpackage.sh STAGE OUT.lha - Cornerstone's last packaging step: STAGE is a directory holding the
# program's drawer and its .info (as the archive should unpack: Program.info, Program/...), made by
# the program's own tools/package.sh. This checks what is in it, refuses to make the archive if any
# certificate or key material went in (our build machines hold a test-only TLS certificate that
# must never ship), then makes OUT.lha with level-2 headers and checks the archive itself again.
# Nothing is deleted: an earlier OUT.lha is moved aside, a refused one renamed .refused.
# SPDX-License-Identifier: GPL-2.0-only
set -e
stage=$1; out=$2
[ -d "$stage" ] && [ -n "$out" ] || { echo "usage: mkpackage.sh STAGE OUT.lha" >&2; exit 2; }
refuse() { echo "mkpackage: $1, refusing" >&2; [ -e "$out" ] && mv "$out" "$out.refused"; exit 1; }
keys() {   # $1: a directory; certificate or key material anywhere in it?
	# (the patterns are written so that they don't match themselves: this script ships in Source/)
	grep -rlE "BEGIN [C]ERTIFICATE|PRIVATE [K]EY|sandbox-[t]est-only" "$1" 2>/dev/null && return 0
	find "$1" \( -iname '*.crt' -o -iname '*.pem' -o -iname '*.key' -o -iname '*.p12' -o -iname '*.pfx' -o -iname '*extra-ca*' \) | grep . && return 0
	return 1
}
if keys "$stage"; then refuse "certificate or key material in $stage"; fi
[ -e "$out" ] && mv "$out" "$out.old"
if command -v jlha >/dev/null 2>&1; then
	(cd "$stage" && jlha a2o5q "$out" $(find . -mindepth 1 -type f | sed 's|^\./||' | LC_ALL=C sort) >/dev/null)
else
	(cd "$stage" && lha -ao5q "$out" $(find . -mindepth 1 -type f | sed 's|^\./||' | LC_ALL=C sort) >/dev/null)
fi
[ -s "$out" ] || { echo "mkpackage: the archiver failed" >&2; exit 1; }
# the archive itself, unpacked somewhere else and searched again
chk=$(mktemp -d "${TMPDIR:-/tmp}/mkpackage.XXXXXX")
(cd "$chk" && (jlha xq "$out" >/dev/null 2>&1 || lha xq "$out" >/dev/null 2>&1))
if keys "$chk"; then mv "$chk" "$chk.checked"; refuse "certificate or key material in the archive"; fi
n=$(find "$chk" -type f | wc -l)
mv "$chk" "$chk.checked"           # (left for a look; on a shared machine nothing is removed with rm -rf)
echo "mkpackage: $out ($n files, $(wc -c < "$out") bytes; checked, no key material)"

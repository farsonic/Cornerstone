#!/usr/bin/env python3
"""gapcount.py - recounts a gap document (the list of everything the upstream "Craft" program
has, each line marked with how far ours goes: docs/shared/14-gap-method.md).

    python3 kit/shared/tools/gapcount.py GAP.md            print each section's counts
    python3 kit/shared/tools/gapcount.py GAP.md --write    and write them into the document

A section is a "### Name (N: ✅ a · 🟨 b · ⬜ c · ➖ d)" heading; its lines are list items starting
with one of the four marks (✅ done, 🟨 partly, ⬜ missing, ➖ left out on purpose). --write puts the
recount into each section heading, into the summary table's rows ("| Name | N | a | b | c | d |"),
its "| **All** |" row, and a line starting "- **Coverage:**".
SPDX-License-Identifier: GPL-2.0-only"""
import re, sys

MARKS = '✅🟨⬜➖'

def count(lines):
    sec, counts, order = None, {}, []
    for ln in lines:
        m = re.match(r'^### (.+?) \((\d+): ', ln)
        if m:
            sec = m.group(1); counts[sec] = [0, 0, 0, 0]; order.append(sec); continue
        if ln.startswith('## '): sec = None
        if sec and ln.startswith('- ') and len(ln) > 2 and ln[2] in MARKS:
            counts[sec][MARKS.index(ln[2])] += 1
    return counts, order

def main():
    if len(sys.argv) < 2: sys.exit(__doc__)
    path = sys.argv[1]
    lines = open(path, encoding='utf-8').read().split('\n')
    counts, order = count(lines)
    T = [sum(counts[s][i] for s in order) for i in range(4)]
    for s in order: print('%-30s %4d: ✅ %d · 🟨 %d · ⬜ %d · ➖ %d' % ((s, sum(counts[s])) + tuple(counts[s])))
    app = sum(T) - T[3]
    print('%-30s %4d: ✅ %d · 🟨 %d · ⬜ %d · ➖ %d' % (('All', sum(T)) + tuple(T)))
    if '--write' not in sys.argv: return 0
    out = []
    for ln in lines:
        m = re.match(r'^### (.+?) \((\d+): ✅ \d+ · 🟨 \d+ · ⬜ \d+ · ➖ \d+\)$', ln)
        if m and m.group(1) in counts:
            c = counts[m.group(1)]; ln = '### %s (%d: ✅ %d · 🟨 %d · ⬜ %d · ➖ %d)' % (m.group(1), sum(c), c[0], c[1], c[2], c[3])
        m = re.match(r'^\| (.+?) \| (\d+) \| (\d+) \| (\d+) \| (\d+) \| (\d+) \|$', ln)
        if m and m.group(1) in counts:
            c = counts[m.group(1)]; ln = '| %s | %d | %d | %d | %d | %d |' % (m.group(1), sum(c), c[0], c[1], c[2], c[3])
        if ln.startswith('| **All** |'):
            ln = '| **All** | **%d** | **%d** | **%d** | **%d** | **%d** |' % (sum(T), T[0], T[1], T[2], T[3])
        if ln.startswith('- **Coverage:**') and app:
            ln = ('- **Coverage:** %d items, %d of them applicable: **%d** done, **%d** partly, **%d** missing; %d left out on purpose. '
                  '%d%% of what applies is done, %d%% at least in part.' % (sum(T), app, T[0], T[1], T[2], T[3], round(100 * T[0] / app), round(100 * (T[0] + T[1]) / app)))
        out.append(ln)
    open(path, 'w', encoding='utf-8').write('\n'.join(out))
    print('written')
    return 0

if __name__ == '__main__':
    sys.exit(main())

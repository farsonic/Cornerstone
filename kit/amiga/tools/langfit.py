#!/usr/bin/env python3
"""langfit.py - do the translations still fit 640x480?

    python3 kit/amiga/tools/langfit.py [--root DIR] [--all]

Two checks, in every language:
  - the menus (built from the program's menu layout and command table, natively, as ribbonfit.py
    builds the ribbon): the titles across a 640-pixel screen's bar and each menu, with its widest
    submenu beside it, inside the screen, in topaz 8 (8 pixels a letter, the widest usual case);
  - every row of the strings files: where a translation's longest line, measured in QuillSans 11,
    is wider than a 600-pixel dialog while the English isn't. Requesters (cs_ask) wrap at 66
    characters on their own and are measured wrapped.
--all also lists the rows that grow more than half again over the English, to look at by eye.
Exit status 1 if anything doesn't fit. SPDX-License-Identifier: GPL-2.0-only"""
import os, subprocess, sys, textwrap
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'shared', 'tools'))
import csconfig, amifont, mklang, ribbonfit

LANGS = ['en', 'de', 'fr', 'es', 'it']

def longest(f, t):
    lines = []
    for para in t.split('\n'):
        lines += textwrap.wrap(para, 66) or ['']
    return max(sum(f['adv'][b] for b in l.encode('latin-1', 'replace')) for l in lines)

def main():
    cfg = csconfig.Config()
    bad = 0
    if cfg.get('fit', {}).get('menus'):
        exe = ribbonfit.build(cfg, 'menus')
        r = subprocess.run([exe, 'menus'], stdout=subprocess.PIPE, encoding='latin-1')
        sys.stdout.write(r.stdout)
        bad += r.returncode != 0
    f = amifont.load(amifont.find(cfg))
    items, probs = mklang.read_rows(cfg)
    grow = []
    for it in items:
        if it[0] != 'row': continue
        en = longest(f, it[3][0])
        for k in range(1, 5):
            w = longest(f, it[3][k])
            if w > 600 and en <= 600:
                print('%s:%d: %s is %d pixels wide (the English %d)' % (cfg.rel(it[1]), it[2], LANGS[k], w, en)); bad += 1
            if en > 30 and w > en * 1.5: grow.append((w / en, LANGS[k], it[3][0][:50], cfg.rel(it[1]), it[2]))
    if '--all' in sys.argv:
        for g in sorted(grow, reverse=True): print('%s:%d: %s %.0f%% of the English: %s' % (g[3], g[4], g[1], g[0] * 100, g[2]))
    print('langfit: %s; %d translations grow over half again (--all lists them)' % ('everything fits' if not bad else '%d problems' % bad, len(grow)))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())

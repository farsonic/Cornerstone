#!/usr/bin/env python3
"""ribbonfit.py - does every ribbon tab fit a 640-pixel window, in each interface language?

    python3 kit/amiga/tools/ribbonfit.py [--root DIR] [--width PIXELS]   (default: cornerstone.json fit.width, 618)

It compiles the program's own ribbon (cornerstone.json "fit": its sources, init function and ribbon
table) natively with the kit's layout code (kit/shared/cs_ribbon.c, the code the Amiga class runs)
and measures with QuillSans 11's real widths from the fonts drawer. Each tab is reported as
fitting, squeezed (small tools lose their labels) and with which groups collapsed to a button;
a tab that still doesn't fit fails the check (exit 1). 618 is a 640-pixel window's inside.
SPDX-License-Identifier: GPL-2.0-only"""
import os, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'shared', 'tools'))
import csconfig, amifont

def build(cfg, what):
    """the harness built for 'ribbon' or 'menus'; its path"""
    fit = cfg.get('fit', {})
    fp = amifont.find(cfg)
    if not fp:
        sys.exit('no QuillSans/11 in the fonts drawer (cornerstone.json "fonts")')
    f = amifont.load(fp)
    out = os.path.join(os.environ.get('TMP', tempfile.gettempdir()), 'cs-fit-%s' % cfg.get('program', 'prog'))
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, 'fitfont.h'), 'w') as h:
        h.write('static const unsigned char fit_adv[256] = { %s };\nstatic const int fit_height = %d;\n'
                % (', '.join(str(max(0, f['adv'][c])) for c in range(256)), f['ysize']))
    kit = os.path.normpath(os.path.join(HERE, '..', '..'))
    srcs = [cfg.path(s) for s in fit.get('sources', [])] + [os.path.join(kit, 'shared', n) for n in os.listdir(os.path.join(kit, 'shared')) if n.startswith('cs_') and n.endswith('.c')]
    exe = os.path.join(out, 'fit-' + what)
    defs = ['-DFIT_INIT=' + fit['init'], '-DFIT_TABLE="%s"' % cfg.path(cfg.get('table'))]
    if what == 'ribbon': defs.append('-DFIT_RIBBON=' + fit['ribbon'])
    if what == 'menus' and fit.get('menus'): defs.append('-DFIT_MENUS=' + fit['menus'])
    cmd = ['cc', '-std=c99', '-O1', '-w', '-I' + out, '-I' + os.path.join(kit, 'shared')] + defs + [os.path.join(HERE, 'fit.c')] + srcs + ['-o', exe]
    subprocess.check_call(cmd)
    return exe

def main():
    cfg = csconfig.Config()
    w = cfg.get('fit', {}).get('width', 618)
    for i, a in enumerate(sys.argv):
        if a == '--width': w = int(sys.argv[i + 1])
    exe = build(cfg, 'ribbon')
    r = subprocess.run([exe, 'ribbon', str(w)], stdout=subprocess.PIPE, encoding='latin-1')
    sys.stdout.write(r.stdout)
    print('ribbonfit: every tab fits %d pixels in every language' % w if r.returncode == 0 else 'ribbonfit: a tab does not fit (above)')
    return r.returncode

if __name__ == '__main__':
    sys.exit(main())

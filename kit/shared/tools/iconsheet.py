#!/usr/bin/env python3
"""iconsheet.py - the icon language's previewer: every interface icon drawn to one PNG sheet.

    python3 kit/shared/tools/iconsheet.py OUT.png [--root DIR]

Without --root (and outside a program) it draws the kit's shared set; inside a program it adds the
program's own icons (cornerstone.json "fit": its sources and init function, which hands them over
with cs_icon_set_app). Each icon is shown at 16 pixels in the light theme, the dark theme and
dimmed, mirrored if it is directional (right to left), and at 24 and 48 pixels, so a new icon can
be checked by eye before it is tried on an Amiga.

The strings are parsed by the kit's own interpreter (iconsheet.c, compiled natively, scaled as
kit/amiga/cs_theme.c scales them), and drawn here as graphics.library draws: one-pixel lines
(Bresenham), boxes inclusive at both corners, circles as DrawEllipse draws them, letters in
QuillSans 11 (bold for B). The colours are the theme's tokens, read from kit/amiga/cs_theme.c.
SPDX-License-Identifier: GPL-2.0-only"""
import os, re, subprocess, sys, tempfile
from PIL import Image, ImageDraw
HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(KIT, 'amiga', 'tools'))
import amifont

INKS = ['ICON', 'ACCENT', 'RED', 'GREEN', 'ORANGE', 'YELLOW', 'DIM', 'WHITE', 'BORDER2', 'TEXT', 'SELECTION']

def theme_colours():
    """the light and dark tables of cs_theme.c, by token name"""
    src = open(os.path.join(KIT, 'amiga', 'cs_theme.c'), encoding='latin-1').read()
    hdr = open(os.path.join(KIT, 'amiga', 'cs_theme.h'), encoding='latin-1').read()
    names = re.findall(r'CS_T_([A-Z0-9]+)', hdr[hdr.index('enum {'):hdr.index('CS_T_NPENS')])
    def table(name):
        body = re.search(name + r'\[CS_T_NPENS\] = \{([^}]*)\}', src).group(1)
        vals = [int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{6})', body)]
        return {n: ((v >> 16) & 255, (v >> 8) & 255, v & 255) for n, v in zip(names, vals)}
    return table('light_def'), table('dark_def')

def ops(root):
    """the drawing operations of every icon, from the native interpreter"""
    out = os.path.join(os.environ.get('TMP', tempfile.gettempdir()), 'cs-iconsheet')
    os.makedirs(out, exist_ok=True)
    srcs = [os.path.join(KIT, 'shared', n) for n in os.listdir(os.path.join(KIT, 'shared')) if n.startswith('cs_') and n.endswith('.c')]
    defs = []
    if root:
        import csconfig
        cfg = csconfig.Config(['--root', root])
        fit = cfg.get('fit', {})
        srcs += [cfg.path(s) for s in fit.get('sources', [])]
        defs = ['-DICON_INIT=' + fit['init']]
    exe = os.path.join(out, 'iconsheet')
    subprocess.check_call(['cc', '-std=c99', '-O1', '-w', '-I' + os.path.join(KIT, 'shared')] + defs + [os.path.join(HERE, 'iconsheet.c')] + srcs + ['-o', exe])
    text = subprocess.run([exe], stdout=subprocess.PIPE, check=True).stdout.decode('latin-1')
    icons, cur = [], None
    for line in text.split('\n'):
        w = line.split(' ')
        if w[0] == 'icon': cur = {'name': w[1], 'size': int(w[2]), 'mirror': int(w[3]), 'ops': []}
        elif w[0] == 'end': icons.append(cur)
        elif w[0] in ('ink', 'line', 'box', 'fill', 'circle'): cur['ops'].append((w[0], [int(v) for v in w[1:]]))
        elif w[0] == 'text': cur['ops'].append(('text', [w[1], ' '.join(w[2:])]))
        elif w[0] == 'bad': cur['bad'] = True
    return icons

def bresenham(px, x0, y0, x1, y1, c, W, H):
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
    err = dx + dy
    while True:
        if 0 <= x0 < W and 0 <= y0 < H: px[x0, y0] = c
        if x0 == x1 and y0 == y1: break
        e2 = 2 * err
        if e2 >= dy: err += dy; x0 += sx
        if e2 <= dx: err += dx; y0 += sy

def ellipse(px, cx, cy, r, c, W, H):
    x, y, d = r, 0, 1 - r                         # midpoint circle, the outline DrawEllipse draws
    while x >= y:
        for a, b in ((x, y), (y, x), (-y, x), (-x, y), (-x, -y), (-y, -x), (y, -x), (x, -y)):
            if 0 <= cx + a < W and 0 <= cy + b < H: px[cx + a, cy + b] = c
        y += 1
        if d < 0: d += 2 * y + 1
        else: x -= 1; d += 2 * (y - x) + 1

def draw(icon, size, colours, dim, fonts, bg):
    im = Image.new('RGB', (size, size), bg)
    px = im.load()
    c = colours['DISABLED' if dim else 'ICON']
    for op, a in icon['ops']:
        if op == 'ink':
            name = INKS[a[0]] if a[0] < len(INKS) else 'ICON'
            c = colours['DISABLED'] if dim and name != 'WHITE' else colours[name]
        elif op == 'line': bresenham(px, a[0], a[1], a[2], a[3], c, size, size)
        elif op == 'box':
            for x0, y0, x1, y1 in ((a[0], a[1], a[2], a[1]), (a[2], a[1], a[2], a[3]), (a[0], a[3], a[2], a[3]), (a[0], a[1], a[0], a[3])):
                bresenham(px, x0, y0, x1, y1, c, size, size)
        elif op == 'fill':
            for y in range(max(0, a[1]), min(size, a[3] + 1)):
                for x in range(max(0, a[0]), min(size, a[2] + 1)): px[x, y] = c
        elif op == 'circle': ellipse(px, a[0], a[1], a[2], c, size, size)
        elif op == 'text':
            f = fonts['B' if a[0] == 'B' else 'R']
            s = a[1].encode('latin-1')
            w = sum(f['adv'][b] for b in s)
            x = (size - w) // 2
            y = (size - f['ysize']) // 2
            for b in s:
                gw, kern, sp, rows = f['glyph'](b)
                slant = a[0] == 'I'
                for yy, row in enumerate(rows):
                    for xx, on in enumerate(row):
                        sx = x + kern + xx + ((f['ysize'] - yy) // 4 if slant else 0)
                        if on and 0 <= sx < size and 0 <= y + yy < size: px[sx, y + yy] = c
                if a[0] == 'U':
                    for xx in range(sp + kern):
                        if 0 <= x + xx < size and 0 <= y + f['baseline'] + 2 < size: px[x + xx, y + f['baseline'] + 2] = c
                x += sp + kern
    return im

def main():
    if len(sys.argv) < 2: sys.exit('usage: iconsheet.py OUT.png [--root DIR]')
    out = sys.argv[1]
    root = sys.argv[sys.argv.index('--root') + 1] if '--root' in sys.argv else None
    light, dark = theme_colours()
    fdir = os.path.join(KIT, 'amiga', 'fonts')
    fonts = {'R': amifont.load(os.path.join(fdir, 'QuillSans', '11')), 'B': amifont.load(os.path.join(fdir, 'QuillSansB', '11'))}
    icons = ops(root)
    names = []
    for ic in icons:
        if ic['name'] not in names: names.append(ic['name'])
    byname = {}
    for ic in icons: byname[(ic['name'], ic['size'], ic['mirror'])] = ic
    label = fonts['R']
    cols = 3
    cellw, cellh = 230, 60
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new('RGB', (cols * cellw + 16, rows * cellh + 40), light['RIBBON'])
    d = ImageDraw.Draw(sheet)
    def text(x, y, s, colour):
        px = sheet.load()
        for b in s.encode('latin-1', 'replace'):
            gw, kern, sp, gr = label['glyph'](b)
            for yy, row in enumerate(gr):
                for xx, on in enumerate(row):
                    if on: px[x + kern + xx, y + yy] = colour
            x += sp + kern
    text(8, 8, 'Cornerstone interface icons: 16 px light, dark, dimmed, mirrored (right to left); 24 px; 48 px', light['TEXT'])
    bad = 0
    for i, n in enumerate(names):
        x0, y0 = 8 + (i % cols) * cellw, 32 + (i // cols) * cellh
        text(x0, y0, n, light['DIM'])
        y = y0 + 14
        x = x0
        for colours, dim in ((light, 0), (dark, 0), (light, 1)):
            ic = byname[(n, 16, 0)]
            bad += 'bad' in ic
            bg = colours['RIBBON']
            sheet.paste(draw(ic, 16, colours, dim, fonts, bg).resize((32, 32), Image.NEAREST), (x, y))
            x += 36
        if (n, 16, 1) in byname:
            sheet.paste(draw(byname[(n, 16, 1)], 16, light, 0, fonts, light['RIBBON']).resize((32, 32), Image.NEAREST), (x, y))
        x += 40
        sheet.paste(draw(byname[(n, 24, 0)], 24, light, 0, fonts, light['RIBBON']), (x, y + 4))
        x += 30
        sheet.paste(draw(byname[(n, 48, 0)], 48, light, 0, fonts, light['RIBBON']).resize((32, 32), Image.LANCZOS), (x, y))
        d.rectangle((x0 - 2, y0 - 2, x0 + cellw - 8, y0 + cellh - 6), outline=light['BORDER'])
    sheet.save(out)
    print('iconsheet: %d icons drawn to %s%s' % (len(names), out, ', %d malformed' % bad if bad else ''))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main())

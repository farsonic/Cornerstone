"""wbicons.py - Jotter's Workbench icons, for kit/amiga/tools/mkicons.py (see its head):
    python3 ../kit/amiga/tools/mkicons.py PKGDIR tools/wbicons.py [PREVIEW.png]
The pictures are Jotter's own: a spiral-bound note pad with a pencil (the program), the same pad
small on the drawer, a page for its documents. SPDX-License-Identifier: GPL-2.0-only"""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'kit', 'amiga', 'tools'))
from mkicons import S, K, INK, PAPER, LINE, BLUE, page

def pad(d, x0=8, y0=6, x1=34, y1=42):
    """a note pad: a sheet with lines, rings along its top"""
    d.rectangle(S(x0, y0, x1, y1), fill=PAPER, outline=INK, width=K)
    for y in range(int(y0) + 9, int(y1) - 2, 5):
        d.line(S(x0 + 3, y, x1 - 3, y), fill=LINE, width=int(1.6 * K))
    for x in range(int(x0) + 3, int(x1) - 1, 5):
        d.ellipse(S(x - 1.5, y0 - 3, x + 1.5, y0 + 3), outline=INK, width=K)

def pencil(d):
    """a pencil lying across the pad's corner"""
    d.polygon(S(26, 40, 41, 14, 45, 16, 30, 42), fill=(240, 196, 64), outline=INK, width=K)
    d.polygon(S(26, 40, 30, 42, 26, 45), fill=(232, 210, 170), outline=INK, width=K)
    d.polygon(S(41, 14, 45, 16, 46, 13, 43, 11), fill=(214, 96, 96), outline=INK, width=K)

def tool(d):
    pad(d)
    pencil(d)

def emblem(d):
    """(the drawer's emblem) the pad, small"""
    pad(d, 15, 22, 31, 37)

ICONS = [
    {'file': 'Jotter.info', 'kind': 'drawer', 'picture': 'drawer', 'window': (40, 30, 420, 200)},
    {'file': 'Jotter/Jotter.info', 'kind': 'tool', 'picture': tool, 'stack': 32768, 'pos': (16, 8),
     'tooltypes': ('(PUBSCREEN=Workbench)', '(LANGUAGE=de)', '(DEBUG=RAM:Jotter.log)')},
    {'file': 'Jotter/Install-Jotter.info', 'kind': 'project', 'picture': 'install', 'deftool': 'Installer', 'pos': (96, 8),
     'stack': 16384, 'tooltypes': ('APPNAME=Jotter', 'MINUSER=NOVICE', 'DEFUSER=AVERAGE')},
    {'file': 'Jotter/README.md.info', 'kind': 'project', 'picture': 'readme', 'deftool': 'SYS:Utilities/MultiView', 'pos': (176, 8), 'stack': 16384},
    {'file': 'Jotter/Welcome.txt.info', 'kind': 'project', 'picture': 'document', 'deftool': 'Jotter', 'pos': (256, 8), 'stack': 32768},
    {'file': 'Jotter/LICENSE.info', 'kind': 'project', 'picture': 'readme', 'deftool': 'SYS:Utilities/MultiView', 'pos': (16, 84), 'stack': 16384},
    {'file': 'Jotter/Fonts.info', 'kind': 'drawer', 'picture': 'fonts', 'pos': (96, 84), 'window': (80, 60, 360, 160)},
    {'file': 'Jotter/Source.info', 'kind': 'drawer', 'picture': 'source', 'pos': (176, 84), 'window': (80, 60, 460, 260)},
]

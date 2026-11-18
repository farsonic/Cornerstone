#!/usr/bin/env python3
"""amifont.py - reads an Amiga bitmap font size file (a hunk file with a DiskFontHeader, as
mkamigafonts.py writes them) for the fit checks and the screenshots' tools: each character's
advance (CharSpace + CharKern), its height and baseline, and its glyphs as bitmaps.
SPDX-License-Identifier: GPL-2.0-only"""
import os, struct

def load(path):
    """the font at path: a dict with 'adv' {code: pixels}, 'ysize', 'baseline', 'lo', 'hi', and
    'glyph(code)' giving (width, rows of 0/1) for drawing"""
    b = open(path, 'rb').read()
    tag = struct.pack('>I', 0x3E9)
    i = b.find(struct.pack('>I', 0x3E9 | 0x40000000))
    if i < 0:
        i = b.index(tag)
    n = struct.unpack('>I', b[i + 4:i + 8])[0]
    d = b[i + 8:i + 8 + n * 4]
    # MOVEQ/RTS (4), the DiskFontHeader's Node (14), FileID, Revision, Segment (8), Name (32), then the TextFont
    tf = 4 + 14 + 2 + 2 + 4 + 32
    p = tf + 20                                   # past the TextFont's Message
    ys, st, fl, xs, bl, bs, acc, lo, hi = struct.unpack('>HBBHHHHBB', d[p:p + 14]); p += 14
    cdata, mod, cloc, cspace, ckern = struct.unpack('>IHIII', d[p:p + 18])
    adv, locs = {}, {}
    for c in range(lo, hi + 2):
        k = c - lo
        sp = struct.unpack('>h', d[cspace + 2 * k:cspace + 2 * k + 2])[0] if cspace else xs
        kn = struct.unpack('>h', d[ckern + 2 * k:ckern + 2 * k + 2])[0] if ckern else 0
        adv[c] = sp + kn
        off, w = struct.unpack('>HH', d[cloc + 4 * k:cloc + 4 * k + 4])
        locs[c] = (off, w, kn, sp)
    default = adv.get(hi + 1, xs)
    for c in range(256):
        adv.setdefault(c, default)
    def glyph(c):
        off, w, kn, sp = locs.get(c, locs[hi + 1])
        rows = []
        for y in range(ys):
            row = []
            for x in range(w):
                bit = off + x
                byte = d[cdata + y * mod + bit // 8]
                row.append((byte >> (7 - bit % 8)) & 1)
            rows.append(row)
        return w, kn, sp, rows
    return {'adv': adv, 'ysize': ys, 'baseline': bl, 'lo': lo, 'hi': hi, 'glyph': glyph}

def find(cfg, family='QuillSans', size=11):
    """the font file of family/size in the program's fonts drawer (cornerstone.json "fonts")"""
    d = cfg.path(cfg.get('fonts', 'kit/amiga/fonts'))
    p = os.path.join(d, family, str(size))
    return p if os.path.exists(p) else None

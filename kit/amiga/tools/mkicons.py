#!/usr/bin/env python3
"""mkicons.py - draws a program's Workbench icons and writes them as .info files.

    python3 kit/amiga/tools/mkicons.py PKGDIR SPEC.py [PREVIEW.png]

SPEC.py is the program's own list of icons (Jotter's: example/tools/wbicons.py): a module with
ICONS, a list of dicts, each
    file      the .info to write, relative to PKGDIR ("Jotter.info", "Jotter/Jotter.info")
    kind      'disk', 'drawer', 'tool', 'project'
    picture   a name of a picture here ('drawer', 'document', 'readme', 'install', 'source', 'fonts')
              or a function f(d) drawing on a 46x46-unit ImageDraw scaled by K (use the helpers below)
    deftool, tooltypes, stack, pos (x, y in its drawer's window), window (x, y, w, h: drawers)
and optionally a function emblem(d) that pictures may use (the program's mark on the drawer).

Each icon has two images, made from the same picture:
  - a GlowIcon (the OS 3.5 colour icon, also read by AROS and MorphOS): an IFF FORM ICON with a
    FACE chunk and an IMAG chunk per state, run-length packed, up to 256 colours;
  - the classic planar image for Workbench 2.x-3.1 (two bitplanes in the standard four colours:
    grey, black, white, blue), and its selected image.
Pictures are drawn with PIL at four times the size and scaled down, then each pixel made fully
opaque or transparent over Workbench grey. The selected image is the picture brightened with a
warm glow round it. A preview sheet of every icon goes to PREVIEW.png.
Rules (docs/platforms/amiga/13-workbench-icons.md): original pictures, no copied logos, a one-unit
margin, recognisable in the four classic colours. NewIcons (icons kept in tool types) are not made.
SPDX-License-Identifier: GPL-2.0-only"""
import importlib.util, os, struct, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W = H = 46
K = 4
BG = (170, 170, 170)                       # Workbench grey
INK = (30, 34, 48)
PAPER = (252, 252, 248)
LINE = (120, 150, 200)
BLUE = (58, 102, 176)
FOLDER = (236, 196, 92)
FOLDER_DARK = (196, 150, 52)

# ---------------------------------------------------------------- drawing helpers (46-unit coordinates)
def canvas():
    im = Image.new('RGBA', (W * K, H * K), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)

def S(*v):
    return [x * K for x in v]

def font(size):
    here = os.path.dirname(os.path.abspath(__file__))
    for f in (os.path.join(here, '..', 'fonts', 'Carlito-Bold.ttf'), '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',
              '/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf'):
        if os.path.exists(f):
            return ImageFont.truetype(f, int(size * K))
    return ImageFont.load_default()

def page(d, x0, y0, x1, y1, fold=7, lines=True):
    """a sheet of paper with a folded corner and lines of text"""
    d.polygon(S(x0, y0, x1 - fold, y0, x1, y0 + fold, x1, y1, x0, y1), fill=PAPER, outline=INK, width=K)
    d.polygon(S(x1 - fold, y0, x1 - fold, y0 + fold, x1, y0 + fold), fill=(214, 214, 206), outline=INK, width=K)
    if lines:
        y = y0 + fold + 4
        while y < y1 - 4:
            d.line(S(x0 + 4, y, x1 - 5, y), fill=LINE, width=int(1.6 * K))
            y += 4.5

def drawer_shape(d, emblem=None):
    """a drawer: a folder front with a tab, an optional emblem on it"""
    d.polygon(S(3, 10, 17, 10, 21, 14, 43, 14, 43, 40, 3, 40), fill=FOLDER_DARK, outline=INK, width=K)
    d.polygon(S(3, 18, 43, 18, 43, 40, 3, 40), fill=FOLDER, outline=INK, width=K)
    if emblem:
        emblem(d)

def text_emblem(d, t, x=23, y=29, size=11, colour=INK):
    f = font(size)
    w = d.textlength(t, font=f)
    d.text((x * K - w / 2, y * K - size * K * 0.62), t, font=f, fill=colour)

def box(d):
    """an installer's box with an arrow into it"""
    d.polygon(S(6, 20, 23, 14, 40, 20, 40, 38, 23, 44, 6, 38), fill=(204, 160, 98), outline=INK, width=K)
    d.line(S(6, 20, 23, 26, 40, 20), fill=INK, width=K)
    d.line(S(23, 26, 23, 44), fill=INK, width=K)
    d.polygon(S(19, 2, 27, 2, 27, 10, 32, 10, 23, 19, 14, 10, 19, 10), fill=(64, 168, 80), outline=INK, width=K)

PICTURES = {
    'drawer': lambda d, spec: drawer_shape(d, getattr(spec, 'emblem', None)),
    'fonts': lambda d, spec: drawer_shape(d, lambda dd: text_emblem(dd, 'Aa')),
    'source': lambda d, spec: drawer_shape(d, lambda dd: text_emblem(dd, '{ }')),
    'document': lambda d, spec: page(d, 9, 3, 37, 43),
    'readme': lambda d, spec: (page(d, 9, 3, 37, 43, lines=True), text_emblem(d, 'i', 23, 30, 16, BLUE)),
    'install': lambda d, spec: box(d),
}

# ---------------------------------------------------------------- images
def finish(im):
    small = im.resize((W, H), Image.LANCZOS)
    out = Image.new('RGBA', (W, H))
    px, po = small.load(), out.load()
    for y in range(H):
        for x in range(W):
            r, g, b, a = px[x, y]
            if a < 100: po[x, y] = (0, 0, 0, 0)
            else:
                f = a / 255
                po[x, y] = (int(r * f + BG[0] * (1 - f)), int(g * f + BG[1] * (1 - f)), int(b * f + BG[2] * (1 - f)), 255)
    return out

def selected(im):
    mask = im.split()[3]
    glow = mask.filter(ImageFilter.MaxFilter(5)).load()
    near = mask.filter(ImageFilter.MaxFilter(3)).load()
    out = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    mp, op, ip = mask.load(), out.load(), im.load()
    for y in range(H):
        for x in range(W):
            if mp[x, y]:
                r, g, b, a = ip[x, y]
                op[x, y] = (min(255, r + 18), min(255, g + 18), min(255, b + 10), 255)
            elif near[x, y]: op[x, y] = (255, 214, 90, 255)
            elif glow[x, y]: op[x, y] = (255, 178, 64, 255)
    return out

def quantize(images, n=200):
    w = sum(i.size[0] for i in images)
    strip = Image.new('RGB', (w, H), BG)
    x = 0
    for i in images:
        strip.paste(i.convert('RGB'), (x, 0)); x += i.size[0]
    q = strip.quantize(colors=n, method=Image.MEDIANCUT)
    pal = q.getpalette()[:3 * n]
    palette = [(0, 0, 0)] + [tuple(pal[i * 3:i * 3 + 3]) for i in range(n)]
    idx, x = [], 0
    qp = q.load()
    for i in images:
        a = i.split()[3].load()
        idx.append([0 if a[xx, yy] < 128 else 1 + qp[x + xx, yy] for yy in range(H) for xx in range(W)])
        x += i.size[0]
    used = sorted(set(v for l in idx for v in l) | {0})
    remap = {v: k for k, v in enumerate(used)}
    return [palette[v] for v in used], [[remap[v] for v in l] for l in idx]

# ---------------------------------------------------------------- .info writing
WB_COLOURS = [(0xAA, 0xAA, 0xAA), (0x00, 0x00, 0x00), (0xFF, 0xFF, 0xFF), (0x66, 0x88, 0xBB)]
def nearest(c):
    return min(range(4), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, WB_COLOURS[i])))

def planar(im):
    words = (W + 15) // 16
    planes = [bytearray(words * 2 * H) for _ in range(2)]
    p = im.load()
    for y in range(H):
        for x in range(W):
            r, g, b, a = p[x, y]
            c = nearest((r, g, b)) if a >= 128 else 0
            for k in range(2):
                if c >> k & 1: planes[k][y * words * 2 + x // 8] |= 0x80 >> (x & 7)
    return struct.pack('>hhhhhIBBI', 0, 0, W, H, 2, 1, 3, 0, 0) + bytes(planes[0]) + bytes(planes[1])

class Bits:
    def __init__(self): self.out, self.acc, self.n = bytearray(), 0, 0
    def put(self, v, bits):
        for i in range(bits - 1, -1, -1):
            self.acc = (self.acc << 1) | ((v >> i) & 1); self.n += 1
            if self.n == 8: self.out.append(self.acc); self.acc, self.n = 0, 0
    def data(self): return bytes(self.out) + (bytes([self.acc << (8 - self.n)]) if self.n else b'')

def rle(values, depth):
    bw, i, n = Bits(), 0, len(values)
    while i < n:
        run = 1
        while i + run < n and run < 128 and values[i + run] == values[i]: run += 1
        if run >= 3:
            bw.put(257 - run, 8); bw.put(values[i], depth); i += run; continue
        j = i
        while j < n and j - i < 128 and not (j + 2 < n and values[j] == values[j + 1] == values[j + 2]): j += 1
        bw.put(j - i - 1, 8)
        for v in values[i:j]: bw.put(v, depth)
        i = j
    return bw.data()

def chunk(cid, body):
    return cid + struct.pack('>I', len(body)) + body + (b'\0' if len(body) & 1 else b'')

def glowicon(images):
    palette, idxs = quantize(images)
    depth = max(1, (len(palette) - 1).bit_length())
    form = b'ICON' + chunk(b'FACE', struct.pack('>BBBBH', W - 1, H - 1, 0, 0x11, len(palette) * 3 - 1))
    pal = bytes(c for rgb in palette for c in rgb)
    for idx in idxs:
        img = rle(idx, depth)
        form += chunk(b'IMAG', struct.pack('>BBBBBBHH', 0, len(palette) - 1, 3, 1, 0, depth, len(img) - 1, len(pal) - 1) + img + pal)
    return b'FORM' + struct.pack('>I', len(form)) + form

KINDS = {'disk': 1, 'drawer': 2, 'tool': 3, 'project': 4, 'garbage': 5}
NOPOS = -2147483648
def write_info(path, kind, normal, deftool=None, tooltypes=(), stack=0, pos=None, window=None):
    sel = selected(normal)
    gadget = struct.pack('>IhhhhHHHIIIIIHI', 0, 0, 0, W, H, 0x0006, 0x0003, 0x0001, 1, 1, 0, 0, 0, 0, 1)
    x, y = pos if pos else (NOPOS, NOPOS)
    isdrawer = kind in (1, 2, 5)
    body = struct.pack('>HH', 0xE310, 1) + gadget + struct.pack('>BBIIiiIII', kind, 0, 1 if deftool else 0, 1 if tooltypes else 0,
                                                                x, y, 1 if isdrawer else 0, 0, stack)
    if isdrawer:
        wl, wt, ww, wh = window or (60, 40, 420, 220)
        body += struct.pack('>hhhhBBIIIIIIIhhHHH', wl, wt, ww, wh, 255, 255, 0, 0, 0, 0, 0, 0, 0, 90, 40, 0xFFFF, 0xFFFF, 1) + struct.pack('>ii', 0, 0)
    body += planar(normal) + planar(sel)
    if deftool:
        s = deftool.encode('latin-1') + b'\0'
        body += struct.pack('>I', len(s)) + s
    if tooltypes:
        body += struct.pack('>I', (len(tooltypes) + 1) * 4)
        for t in tooltypes:
            s = t.encode('latin-1') + b'\0'
            body += struct.pack('>I', len(s)) + s
    if isdrawer: body += struct.pack('>IH', 1, 1)
    body += glowicon([normal, sel])
    with open(path, 'wb') as f: f.write(body)
    return sel

# ---------------------------------------------------------------- reading one back (the check)
def read_info(path):
    """the magic, kind, default tool and tool types of a .info we wrote (the tests' check)"""
    b = open(path, 'rb').read()
    magic, ver = struct.unpack('>HH', b[:4])
    kind = b[48]
    has_tool, has_tt = struct.unpack('>II', b[50:58])
    stack = struct.unpack('>I', b[74:78])[0]
    p = 78 + (56 if kind in (1, 2, 5) else 0)
    for _ in range(2):
        w, h, depth = struct.unpack('>hhh', b[p + 4:p + 10])
        p += 20 + ((w + 15) // 16) * 2 * h * depth
    tool, tts = None, []
    if has_tool:
        n = struct.unpack('>I', b[p:p + 4])[0]; tool = b[p + 4:p + 4 + n - 1].decode('latin-1'); p += 4 + n
    if has_tt:
        n = struct.unpack('>I', b[p:p + 4])[0] // 4 - 1; p += 4
        for _ in range(n):
            m = struct.unpack('>I', b[p:p + 4])[0]; tts.append(b[p + 4:p + 4 + m - 1].decode('latin-1')); p += 4 + m
    return {'magic': magic, 'kind': kind, 'tool': tool, 'tooltypes': tts, 'stack': stack, 'glow': b'FORM' in b[p:p + 16] or b.find(b'ICON', p) > 0}

def main():
    if len(sys.argv) < 3: sys.exit('usage: mkicons.py PKGDIR SPEC.py [PREVIEW.png]')
    pkg, specpath = sys.argv[1], sys.argv[2]
    s = importlib.util.spec_from_file_location('wbicons', specpath)
    spec = importlib.util.module_from_spec(s); s.loader.exec_module(spec)
    pics = []
    for ic in spec.ICONS:
        im, d = canvas()
        pic = ic['picture']
        (PICTURES[pic](d, spec) if isinstance(pic, str) else pic(d))
        normal = finish(im)
        path = os.path.join(pkg, ic['file'])
        if not os.path.isdir(os.path.dirname(path) or '.'): continue
        sel = write_info(path, KINDS[ic['kind']], normal, ic.get('deftool'), ic.get('tooltypes', ()), ic.get('stack', 0), ic.get('pos'), ic.get('window'))
        pics.append((ic['file'], normal, sel))
    if len(sys.argv) > 3:
        sheet = Image.new('RGB', (len(pics) * (W * 2 + 12) * 2 + 8, H * 2 + 8), BG)
        for i, (n, a, b) in enumerate(pics):
            x = 4 + i * (W * 2 + 12) * 2
            sheet.paste(a.resize((W * 2, H * 2), Image.NEAREST), (x, 4), a.resize((W * 2, H * 2), Image.NEAREST))
            sheet.paste(b.resize((W * 2, H * 2), Image.NEAREST), (x + W * 2 + 4, 4), b.resize((W * 2, H * 2), Image.NEAREST))
        sheet.save(sys.argv[3])
    print('mkicons: %d icons written' % len(pics))

if __name__ == '__main__':
    main()

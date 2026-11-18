#!/usr/bin/env python3
"""mklang.py - makes the program's table of interface words (cornerstone.json "table", a C header of
rows for cs_lang_init) from its strings files ("strings": the program's own, then the kit's).

    python3 kit/shared/tools/mklang.py [--root DIR]           write the table
    python3 kit/shared/tools/mklang.py [--root DIR] --check   only say whether it is up to date

A strings file has one row per English string, five columns split by a TAB: the English exactly as
the C source writes it, then German, French, Spanish, Italian (each in the upstream program's own
terms for that language). UTF-8 in the file, but only Latin-1 letters (the Amiga's fonts: no OE
ligature, no curly quotes, no euro sign); C escapes \\n \\t \\\\ \\" \\33 and octal allowed. Lines
starting "# " are notes; "## title" starts a section (a comment in the table). A column may be "-"
for "as English" (a word that is the same, or not translated yet: langcheck reports those).
The other platforms' tools read the same files (docs/shared/08-translation.md).
SPDX-License-Identifier: GPL-2.0-only"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import csconfig

NLANG = 5
LANGS = ['en', 'de', 'fr', 'es', 'it']
HEAD = '/* made by kit/shared/tools/mklang.py from the strings files: do not edit (English, German, French, Spanish, Italian) */\n'

def unescape(s):
    """the text a C-escaped string stands for"""
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c != '\\':
            out.append(c); i += 1; continue
        i += 1
        c = s[i] if i < len(s) else '\\'
        if c in '01234567':
            j = i
            while j < len(s) and j < i + 3 and s[j] in '01234567':
                j += 1
            out.append(chr(int(s[i:j], 8))); i = j; continue
        out.append({'n': '\n', 't': '\t', '"': '"', '\\': '\\', 'r': '\r', 'a': '\a', 'e': '\x1b'}.get(c, c)); i += 1
    return ''.join(out)

def c_literal(t):
    """text (Latin-1) as a C string literal: ASCII as it is, the rest in octal"""
    out, prev_oct = '', False
    for ch in t:
        x = ord(ch)
        if x == 10: out += '\\n'; prev_oct = False
        elif x == 9: out += '\\t'; prev_oct = False
        elif ch in '"\\': out += '\\' + ch; prev_oct = False
        elif 32 <= x < 127:
            if prev_oct and ch in '01234567': out += '""'
            out += ch; prev_oct = False
        else: out += '\\%03o' % x; prev_oct = True
    return '"' + out + '"'

def read_file(path, items, bad, seen):
    """one strings file's rows appended to items: ('row', file, line, [5 texts]) or ('section', ...)"""
    rel = os.path.basename(os.path.dirname(path)) + '/' + os.path.basename(path)
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip('\n')
            if not line.strip() or line.startswith('# ') or line == '#':
                continue
            if line.startswith('## '):
                items.append(('section', path, n, line[3:].strip())); continue
            cols = line.split('\t')
            if len(cols) != NLANG:
                bad.append('%s:%d: %d columns, not %d: %s' % (rel, n, len(cols), NLANG, line[:60])); continue
            texts = [unescape(c) for c in cols]
            texts = [texts[0] if t == '-' else t for t in texts]
            if texts[0] in seen:
                bad.append('%s:%d: "%s" is already at %s' % (rel, n, cols[0], seen[texts[0]])); continue
            seen[texts[0]] = '%s:%d' % (rel, n)
            for k, t in enumerate(texts):
                try:
                    t.encode('latin-1')
                except UnicodeEncodeError:
                    bad.append('%s:%d: not Latin-1 (%s): %s' % (rel, n, LANGS[k], cols[k]))
            items.append(('row', path, n, texts))

def read_rows(cfg):
    items, bad, seen = [], [], {}
    for p in cfg.paths('strings'):
        if os.path.exists(p):
            read_file(p, items, bad, seen)
    return items, bad

def table_text(items):
    lines = [HEAD]
    for it in items:
        if it[0] == 'section': lines.append('  /* %s */\n' % it[3])
        else: lines.append('  { { ' + ', '.join(c_literal(t) for t in it[3]) + ' } },\n')
    return ''.join(lines)

def main():
    cfg = csconfig.Config()
    items, bad = read_rows(cfg)
    for b in bad: print(b)
    if bad: return 1
    table = cfg.path(cfg.get('table'))
    text = table_text(items)
    old = open(table, encoding='latin-1').read() if os.path.exists(table) else ''
    rows = sum(1 for i in items if i[0] == 'row')
    if '--check' in sys.argv:
        if old != text:
            print('%s is out of date: run kit/shared/tools/mklang.py' % cfg.rel(table)); return 1
        print('%s is up to date (%d rows)' % (cfg.rel(table), rows)); return 0
    if old != text:
        with open(table, 'w', encoding='latin-1', newline='\n') as f:
            f.write(text)
    print('%s: %d rows%s' % (cfg.rel(table), rows, '' if old != text else ' (unchanged)'))
    return 0

if __name__ == '__main__':
    sys.exit(main())

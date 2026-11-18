#!/usr/bin/env python3
"""langcheck.py - checks that every interface string in the program's source has its translations
in a strings file, that every row of the program's own strings file is still used, and that each
row's translations keep what the code relies on.

    python3 kit/shared/tools/langcheck.py [--root DIR]            the report (exit 1 on any problem)
    python3 kit/shared/tools/langcheck.py [--root DIR] --rows     the missing strings as rows to paste
                                                                  (English in every column: translate)
    python3 kit/shared/tools/langcheck.py [--root DIR] --unused   only the rows no longer used

Every string literal in the sources (cornerstone.json "sources"; adjacent literals joined as C does)
must be one of: a row of a strings file; in an ignore file (words that aren't the interface's: file
names, format codes, log lines); or plainly not words (see not_words: icon drawings, numbers,
ALL-CAPS identifiers, paths, the arguments of calls that never show their text). It checks that a
row's translations keep the English's printf conversions in order, its '|' requester buttons, its
leading MUI escape (\\33c...) and its '_' shortcut marker, and that no translation is another row's
English (tr() run twice on it would change it). It flags a literal handed straight to a gadget or
a window title without tr() (unless its line says "lang: as is").
Rows of the kit's strings file that the program doesn't use are fine (the kit has more than one
program needs). SPDX-License-Identifier: GPL-2.0-only"""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import csconfig, mklang

QUIET_CALLS = {'CS_LOG', 'cs_logf', 'printf', 'fprintf', 'sscanf', 'strcmp', 'strncmp', 'strstr', 'strchr', 'strrchr',
               'strcspn', 'strspn', 'Stricmp', 'Strnicmp', 'fopen', 'Open', 'Lock', 'OpenLibrary', 'OpenDevice', 'FindPort',
               'MAKE_ID', 'GetVar', 'SetVar', 'DeleteFile', 'Rename', 'CreateDir', 'MatchPattern', 'ParsePattern',
               'AddPart', 'fputs', 'cs_prefs_get', 'cs_prefs_set', 'cs_prefs_int', 'cs_prefs_set_int', 'cs_prefs_path',
               'cs_open_font', 'cs_theme_set_family', 'strftime', 'keyarg', 'snprintf_log', 'FindToolType', 'cs_lang_by_code',
               'cs_icon_draw', 'cs_file_request_pattern'}
SHOWN = re.compile(r'(?:\b(?:Label1|Label2|LLabel1|LLabel2|Label|LLabel|CsLabel|CsButton|CsTitle|SimpleButton|KeyButton|cs_bubble)\s*\(\s*'
                   r'|MUIA_(?:ShortHelp|Text_Contents|Window_Title|Frame_Title)\s*,\s*\(ULONG\)\s*)"([^"]+)"')
FMT = re.compile(r'%(?:-?[0-9]*(?:\.[0-9]+)?|\.\*|-?\*)(?:hh|h|ll|l)?[diouxXeEfgGcs%]')
LANGS = ['de', 'fr', 'es', 'it']

def literals(path):
    """the string literals of a C file: (line, offset, raw text), adjacent ones joined"""
    src = open(path, encoding='latin-1').read()
    out, i, n, line, cur = [], 0, len(src), 1, None
    while i < n:
        c = src[i]
        if c == '\n': line += 1; i += 1; continue
        if src.startswith('/*', i):
            j = src.find('*/', i + 2); line += src.count('\n', i, j); i = j + 2; continue
        if src.startswith('//', i): i = src.find('\n', i); continue
        if c == "'":
            j = i + 1
            while src[j] != "'": j += 2 if src[j] == '\\' else 1
            i = j + 1
            if cur: out.append(cur); cur = None
            continue
        if c == '"':
            j = i + 1
            while src[j] != '"': j += 2 if src[j] == '\\' else 1
            if cur: cur = (cur[0], cur[1], cur[2] + src[i + 1:j])
            else: cur = (line, i, src[i + 1:j])
            i = j + 1; continue
        if c in ' \t\r\n': i += 1; continue
        if cur: out.append(cur); cur = None
        if src.startswith('#include', i): i = src.find('\n', i); continue
        if src.startswith('#define', i) or src.startswith('#if', i): pass
        i += 1
    if cur: out.append(cur)
    return src, out

def enclosing_call(src, pos):
    depth, i, lim = 0, pos - 1, max(0, pos - 3000)
    while i > lim:
        c = src[i]
        if c == ')': depth += 1
        elif c == '(':
            if depth == 0:
                j = i - 1
                while j >= 0 and src[j] in ' \t': j -= 1
                k = j
                while k >= 0 and (src[k].isalnum() or src[k] == '_'): k -= 1
                return src[k + 1:j + 1]
            depth -= 1
        elif c in ';{}' and depth == 0: return ''
        i -= 1
    return ''

ICONTOK = re.compile(r'^>?(?:[A-Za-z]{1,3}|-?\d+)(?: (?:[A-Za-z]{1,3}|-?\d+|[A-Za-z0-9]{1,7}))*$')
def not_words(t):
    """true for text that isn't words for people to read"""
    s = FMT.sub(' ', t)
    if re.match(r'\t[a-z]+\t', t): return True                                  # vbcc inline assembler
    if not re.search(r'[A-Za-z\300-\377]{2}', s): return True                 # numbers, signs, conversions
    if t.startswith('$VER:'): return True
    if ICONTOK.match(t) and (re.search(r'\d', t) or re.match(r'>?c[iargoydwlts] ', t) or re.fullmatch(r'[BIU] \S+', t)): return True   # icon drawings
    if re.fullmatch(r'>?[A-Z][A-Z0-9_.]*[A-Z0-9]?(\(\)?)?', t) and len(t) >= 3: return True   # IDENTIFIERS, icon names
    if ' ' not in t and re.fullmatch(r'[A-Za-z0-9_]+:\S*', t): return True        # PROGDIR:file, FONTS:
    if ' ' not in t and re.search(r'\.[a-z]{2,9}$', t) and not t.endswith('...'): return True   # file names
    if ' ' not in t and re.fullmatch(r'[a-z0-9_/.\-]+', t) and ('_' in t or '/' in t): return True
    if re.fullmatch(r'[LR]\d+(?: [LR]\d+)*', t): return True                      # a status bar's fields
    return False

def read_ignore(cfg):
    ign = set()
    for p in cfg.paths('ignore'):
        if not os.path.exists(p): continue
        for line in open(p, encoding='utf-8'):
            line = line.rstrip('\n')
            if not line or line.startswith('# ') or line == '#': continue
            ign.add(mklang.unescape(line))
    return ign

def check_row(texts):
    en, probs = texts[0], []
    fe = FMT.findall(en)
    for k, t in enumerate(texts[1:]):
        lang = LANGS[k]
        if FMT.findall(t) != fe: probs.append('%s: conversions %s, not %s' % (lang, FMT.findall(t), fe))
        if en.count('|') != t.count('|'): probs.append("%s: %d '|', not %d" % (lang, t.count('|'), en.count('|')))
        if en.startswith('\x1b') and not t.startswith(en[:2]): probs.append('%s: should start with the same \\33 escape' % lang)
        if ('_' in en) != ('_' in t) and re.search(r'(^|\|)[^|]*_', en): probs.append("%s: '_' shortcut marker" % lang)
        if not t.strip(): probs.append('%s: empty' % lang)
    return probs

def main():
    cfg = csconfig.Config()
    items, bad = mklang.read_rows(cfg)
    for b in bad: print(b)
    own = cfg.paths('strings')[0]
    rows = {it[3][0]: it for it in items if it[0] == 'row'}
    ign = read_ignore(cfg)
    missing, used, unwrapped = {}, set(), []
    for path in cfg.sources():
        rel = cfg.rel(path)
        src, lits = literals(path)
        for line, pos, raw in lits:
            t = mklang.unescape(raw)
            if t in rows: used.add(t); continue
            if t in ign or not_words(t): continue
            if enclosing_call(src, pos) in QUIET_CALLS: continue
            before = src[max(0, pos - 80):pos]
            if re.search(r'MUIA_(?:FixWidthTxt|FixHeightTxt|String_Accept|String_Reject|List_Format|Window_InputEvent)\s*,\s*(?:\(ULONG\)\s*)?$', before): continue
            if re.search(r'#\s*define\s+\w+\s+$', before) or re.search(r'#\s*include\s*$', before): continue
            missing.setdefault(t, []).append('%s:%d' % (rel, line))
        for m in SHOWN.finditer(src):
            t = mklang.unescape(m.group(1))
            if 'lang: as is' in src[m.start():src.find('\n', m.start())]: continue
            if t in rows or (t not in ign and not not_words(t)):
                unwrapped.append('%s:%d: "%s" is shown without tr()' % (rel, src.count('\n', 0, m.start()) + 1, m.group(1)))
    probs = []
    for en, it in rows.items():
        loc = '%s:%d' % (cfg.rel(it[1]), it[2])
        for p in check_row(it[3]): probs.append('%s: %s' % (loc, p))
        for k in range(1, mklang.NLANG):
            t = it[3][k]
            if t != en and t in rows and rows[t][3][k] != t:
                probs.append('%s: %s "%s" is also the English of %s:%d (translated twice it becomes "%s")'
                             % (loc, LANGS[k - 1], t, cfg.rel(rows[t][1]), rows[t][2], rows[t][3][k]))
    unused = [(it[2], en) for en, it in sorted(rows.items(), key=lambda x: x[1][2]) if it[1] == own and en not in used]
    orel = cfg.rel(own)
    if '--unused' in sys.argv:
        for n, en in unused: print('%s:%d: unused: "%s"' % (orel, n, en.replace('\n', '\\n')))
        return 1 if unused else 0
    if '--rows' in sys.argv:
        for t in missing: print('\t'.join([t.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t').replace('\x1b', '\\33')] * 5))
        return 1 if missing else 0
    for t, locs in sorted(missing.items(), key=lambda x: x[1][0]):
        print('%s: not translated: "%s"' % (locs[0], t.replace('\n', '\\n')))
    for u in unwrapped: print(u)
    for p in probs: print(p)
    for n, en in unused: print('%s:%d: unused (the source no longer has it): "%s"' % (orel, n, en.replace('\n', '\\n')))
    print('langcheck: %d rows, %d used, %d strings missing, %d shown without tr(), %d unused rows, %d row problems'
          % (len(rows), len(used), len(missing), len(unwrapped), len(unused), len(probs)))
    return 1 if missing or unwrapped or probs or bad or unused else 0

if __name__ == '__main__':
    sys.exit(main())

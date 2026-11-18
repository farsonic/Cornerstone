#!/usr/bin/env python3
"""csconfig.py - reads a Cornerstone program's cornerstone.json, the one file that tells the kit's
tools where the program keeps things. Every tool takes --root DIR (default: the current directory,
or the nearest parent with a cornerstone.json).

    {
      "program": "Jotter",
      "strings": ["lang/strings.txt", "kit/shared/lang/kit-strings.txt"],   the program's rows first
      "ignore":  ["lang/ignore.txt", "kit/shared/lang/kit-ignore.txt"],     literals that aren't words
      "table":   "amiga/lang_table.h",                                       what mklang.py writes
      "sources": ["amiga/*.c", "amiga/*.h", "ui/*.c", "kit/amiga/*.c"],     where langcheck.py looks
      "exclude": ["amiga/lang_table.h"],
      "fonts":   "kit/amiga/fonts",                                          QuillSans/11 for the fit checks
      "fit":     { "sources": ["ui/commands.c"], "init": "jotter_commands_init",
                   "ribbon": "jotter_ribbon", "menus": "jotter_menus", "width": 618 },
      "gap":     "docs/upstream-gap.md"
    }

Paths are relative to the root. SPDX-License-Identifier: GPL-2.0-only"""
import glob, json, os, sys

def find_root(argv):
    for i, a in enumerate(argv):
        if a == '--root' and i + 1 < len(argv):
            return os.path.abspath(argv[i + 1])
        if a.startswith('--root='):
            return os.path.abspath(a[7:])
    d = os.getcwd()
    while True:
        if os.path.exists(os.path.join(d, 'cornerstone.json')):
            return d
        p = os.path.dirname(d)
        if p == d:
            sys.exit('no cornerstone.json here or above: give --root DIR')
        d = p

class Config:
    def __init__(self, argv=None):
        self.root = find_root(argv if argv is not None else sys.argv)
        with open(os.path.join(self.root, 'cornerstone.json'), encoding='utf-8') as f:
            self.data = json.load(f)
    def get(self, key, default=None):
        return self.data.get(key, default)
    def path(self, rel):
        return os.path.normpath(os.path.join(self.root, rel))
    def paths(self, key):
        v = self.data.get(key, [])
        return [self.path(p) for p in ([v] if isinstance(v, str) else v)]
    def sources(self):
        out, excl = [], set(self.paths('exclude'))
        for pat in self.data.get('sources', []):
            out += [os.path.normpath(p) for p in sorted(glob.glob(self.path(pat)))]
        return [p for p in out if p not in excl]
    def rel(self, path):
        return os.path.relpath(path, self.root)

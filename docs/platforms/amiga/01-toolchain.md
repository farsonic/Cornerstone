# 1. Toolchain and project layout

## What is needed

| Part | Where (default) | From |
|---|---|---|
| **vbcc 0.9h** with vasm and vlink, the m68k-amigaos target | `/opt/amiga/vbcc` (`VBCC`) | http://www.compilers.de/vbcc.html: build from source, install the `vbcc_target_m68k-amigaos` and `vbcc_unix_config` archives |
| **NDK 3.2** C headers | `/opt/amiga/ndk/Include_H` (`NDK`) | Aminet `dev/misc/NDK3.2.lha` |
| **MUI 3.8 developer headers** | `/opt/amiga/mui/include` (`MUI`) | Aminet `dev/mui/mui38dev.lha`, its `C/Include`. **Not in any of our repositories**: it may only be redistributed whole. |
| A C compiler and Python 3 with Pillow | the host | for the native tests and the tools |
| FS-UAE, an AROS ROM, Xvfb, xdotool, ImageMagick's `import` | the host | for the emulator checks (chapter 12) |
| `jlha` or `lha` | the host | for packages |

## Compiling

```sh
export VBCC=/opt/amiga/vbcc PATH=/opt/amiga/vbcc/bin:$PATH
vc +aos68k -c99 -O1 -cpu=68020 -Ikit/shared -Ikit/amiga -I$MUI -I$NDK \
   amiga/*.c ui/*.c core/*.c kit/amiga/*.c kit/shared/cs_*.c -o Jotter -lamiga
```

That is Jotter's `build.sh`. Two kinds of program:

- **No floating point** (Jotter, Quill): `+aos68k`, `-cpu=68020` (or 68000), no maths library.
  Runs on any Amiga with OS 3.0. Where doubles are needed after all, Quill opens
  mathieeedoubbas.library itself (`wp_chartkeep.c`) rather than link a soft-float library.
- **68040/68060 with FPU** (Ledger, Easel, Folio): the config `kit/amiga/tools/vbcc-aos68k-fpu`
  (copy of Easel's), `+kit/amiga/tools/vbcc-aos68k-fpu -cpu=68040 -fpu=68040` and `-lm040`. The
  kit itself uses no floating point, so it serves both.

`-O1` is the kit's default. `-O2` is faster but has miscompiled loops in Quill and Easel (chapter
17); if you use it, test new core code in the emulator, not only natively.

vbcc's C99 has limits you will meet:

- no anonymous structs or unions;
- a cast of an expression in a macro needs its own brackets: `(APTR)(id)`, not `(APTR)id`;
- the target is big-endian: never read a file's integers through a pointer cast, read bytes;
- `long` is 32 bits: watch products of sizes (`cs_muldiv()` does `a * b / c` without overflow);
- names that are also graphics.library calls collide: a type called `Text` breaks every file that
  includes `proto/graphics.h` (Jotter's document type is `TextDoc` for that reason).

## A program's tree

```
build.sh           native tests (tools/test.sh) first, then vbcc; "sh build.sh fast" skips the tests
cornerstone.json   where the kit's tools find things (docs/shared/02-architecture.md)
core/              portable C99, tested natively with ASan/UBSan
ui/                the command table, menus, ribbon, context menus: portable data
amiga/             the MUI front end; lang_table.h generated from lang/
kit/               Cornerstone's kit (kit/shared, kit/amiga), copied in
lang/              strings.txt, ignore.txt
tests/             native tests
tools/             test.sh, package.sh, Install-Program, wbicons.py
```

**Big stacks**: a program started from the Shell gets 4 KB of stack unless the user's Stack command
says more. vbcc honours `long __stack = 32768;` in the program (Jotter has it). Decoders and
recursive code that need more run on a stack of their own: Easel's `amiga/bigstack.s` calls a
function on a new stack with exec's `StackSwap`.

**Hooks on Zune**: Zune can call a hook with the stack pointer two bytes off a long-word boundary,
and vbcc's varargs (`printf`, `sprintf`) then read garbage. Quill's `amiga/hook.s` is an entry that
realigns the stack before calling C: use it for any hook that formats text.

## The tests and CI

`tools/test.sh` (Jotter's: `example/tools/test.sh`) runs, in order: `mklang.py --check` (the
table is as the strings files make it), `langcheck.py` (every string translated and used), the
kit's tests, the program's tests (ASan and UBSan), `ribbonfit.py` and `langfit.py` (640x480 in
every language). CI runs exactly that (`.github/workflows/tests.yml`, from
`kit/shared/templates/workflows/`). The vbcc build needs the toolchain above and runs on the build
machine, under the shared lock when the machine is busy:

```sh
flock /tmp/claude-0/easel-test.lock sh build.sh
```

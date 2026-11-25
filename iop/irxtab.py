#!/usr/bin/env python3
"""The import and export tables of an IOP module, for ld.lld --irx.

  irxtab.py import <out.ilb> <cc> <flags...>    irx_imports.h and imports.lst
  irxtab.py export <out.S> <out.c> <cc> <flags...>    exports.tab

imports.lst and exports.tab are ps2sdk's: C, with the macros of irx.h. gcc
compiles them into tables, which needs the order of toplevel asm and variables;
clang does not keep that order. So the preprocessor alone reads them here.

import: an ILB file (Sony's import library format) with just the libraries, the
versions and the entries that imports.lst names. ld.lld makes the import tables
of the entries that the module uses, and nothing else is imported: a name that
several libraries have (AddDrv: ioman, iomanX) comes from the one listed.
export: the tables, in assembly (any number of them), and the C functions that
exports.tab defines itself (_retonly and so on).
"""
import re
import subprocess
import sys


def preprocess(cc, text):
    r = subprocess.run(cc + ['-E', '-P', '-x', 'c', '-'], input=text, stdout=subprocess.PIPE, text=True)
    if r.returncode:
        sys.exit(r.returncode)
    return r.stdout


def make_import(out, cc):
    text = '''#include "irx_imports.h"
#undef DECLARE_IMPORT_TABLE
#undef DECLARE_IMPORT
#undef END_IMPORT_TABLE
#define DECLARE_IMPORT_TABLE(lib, major, minor) @T #lib major minor;
#define DECLARE_IMPORT(n, name) @E n #name;
#define END_IMPORT_TABLE @Z;
#include "imports.lst"
'''
    # an entry without a macro for it stays as I_<name>: an error
    res = preprocess(cc, text)
    res = res[res.index('@'):] if '@' in res else ''
    ilb, rest = [], re.sub(r'@(T "\w+" \d+ \d+|E \d+ "\w+"|Z);', '', res).replace(';', '').strip()
    if rest:
        sys.exit(f'imports.lst: not understood: {rest}')
    for m in re.finditer(r'@(\w)(?: "?(\w+)"?)?(?: "?(\w+)"?)?(?: (\d+))?;', res):
        kind, a, b, c = m.groups()
        if kind == 'T':
            ilb += ['#IOP-ILB#', f'L {a}', f'V {int(b):02x}{int(c):02x}', 'F 0000']
        elif kind == 'E':
            ilb.append(f'E {int(a):03d} {b}')
    open(out, 'w').write('\n'.join(ilb) + '\n')


def make_export(out_s, out_c, cc):
    asm, name = ['\t.section .text,"ax",@progbits'], None
    pattern = re.compile(r'DECLARE_EXPORT_TABLE\(\s*(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)'
                         r'|DECLARE_EXPORT\(\s*(\w+)\s*\)|END_EXPORT_TABLE')
    text = preprocess(cc, '#include "exports.tab"\n')
    for m in pattern.finditer(text):
        lib, major, minor, entry = m.groups()
        if lib:
            asm += ['\t.balign 4', f'\t.globl _exp_{lib}', f'\t.type _exp_{lib}, @object', f'_exp_{lib}:',
                    '\t.word 0x41c00000, 0', f'\t.2byte {(int(major) << 8) | int(minor)}, 0',
                    f'\t.ascii "{lib[:8]}"', f'\t.space {8 - len(lib[:8])}']
        elif entry:
            asm.append(f'\t.word {entry}')
        else:
            asm.append('\t.word 0')
            asm.append(f'\t.size _exp_{name}, . - _exp_{name}')
        name = lib or name
    open(out_s, 'w').write('\n'.join(asm) + '\n')
    open(out_c, 'w').write(pattern.sub('', text))


if sys.argv[1] == 'import':
    make_import(sys.argv[2], sys.argv[3:])
else:
    make_export(sys.argv[2], sys.argv[3], sys.argv[4:])

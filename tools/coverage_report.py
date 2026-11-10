#!/usr/bin/env python3
"""Informe de cobertura: funciones por modulo y por tamano, a partir de la symtab del ELF. Escribe docs/COVERAGE.md.
Uso: coverage_report.py   (sin rutas absolutas)"""
import collections, struct
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
d = (ROOT / "baserom/SCES_504.11").read_bytes()
shoff = struct.unpack_from("<I", d, 32)[0]; se, sn, ss = struct.unpack_from("<HHH", d, 46)
sh = [struct.unpack_from("<10I", d, shoff + i * se) for i in range(sn)]
so = sh[ss][4]; names = [d[so + s[0]: d.index(b"\0", so + s[0])].decode() for s in sh]
sym = next(s for s in sh if s[1] == 2); strs = sh[sym[6]]
funcs = []
for k in range(sym[5] // 16):
    n, val, sz, info, oth, shndx = struct.unpack_from("<IIIBBH", d, sym[4] + k * 16)
    if info & 0xf == 2 and 0 < shndx < len(sh):
        funcs.append((names[shndx], val, sz, d[strs[4] + n: d.index(b"\0", strs[4] + n)].decode()))
BK = [(4, "<=1 instr"), (16, "2-4"), (40, "5-10"), (80, "11-20"), (160, "21-40"), (400, "41-100"), (1200, "101-300"), (10**9, ">300")]
def bucket(sz):
    for lim, nm in BK:
        if sz <= lim: return nm
mods = collections.OrderedDict()
for m, v, sz, nm in funcs: mods.setdefault(m, []).append((v, sz, nm))
def row(label, fs):
    c = collections.Counter(bucket(sz) for _, sz, _ in fs)
    return f"| {label} | {len(fs)} | {sum(sz for _, sz, _ in fs):,} | " + " | ".join(str(c.get(n, 0)) for _, n in BK) + " |"
hdr = "| Módulo | Funciones | Bytes | " + " | ".join(n for _, n in BK) + " |\n|---|---|---|" + "---|" * len(BK)
out = ["# Cobertura (generado por tools/coverage_report.py)\n",
       "Tamaños de `st_size` de la symtab; instr = bytes/4. `main` tiene su código en el ELF; "
       "el resto son **overlays con filesz 0**: su código vive en ficheros del disco.\n", hdr]
out.append(row("**main (en el ELF)**", mods["main"]))
ov = [f for m, fs in mods.items() if m != "main" for f in fs]
out.append(row("**overlays (en el disco)**", ov))
for m, fs in sorted(mods.items(), key=lambda x: -len(x[1])):
    if m != "main": out.append(row(f"&nbsp;&nbsp;{m}", fs))
cls = collections.Counter()
for v, sz, nm in mods["main"]:
    import re
    mm = re.match(r"^.+?__(\d+)(.*)$", nm)
    cls[mm.group(2)[:int(mm.group(1))] if mm else "(libre/C)"] += 1
out += ["\n## Clases con más funciones en `main`\n", ", ".join(f"{c} ({n})" for c, n in cls.most_common(25))]
(ROOT / "docs/COVERAGE.md").write_text("\n".join(out) + "\n")
print("\n".join(out))

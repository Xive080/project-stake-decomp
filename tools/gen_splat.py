#!/usr/bin/env python3
"""Genera config/splat.yaml y config/symbol_addrs.txt para el modulo `main` del ELF.
Agrupa funciones contiguas de la misma clase en un mismo fichero (unidad de compilacion probable).
Los overlays no se pueden dividir: su codigo no esta en el ELF (filesz 0)."""
import re, struct
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
d = (ROOT / "baserom/SCES_504.11").read_bytes()
shoff = struct.unpack_from("<I", d, 32)[0]; se, sn, ss = struct.unpack_from("<HHH", d, 46)
sh = [struct.unpack_from("<10I", d, shoff + i * se) for i in range(sn)]
so = sh[ss][4]; names = [d[so + s[0]: d.index(b"\0", so + s[0])].decode() for s in sh]
sym = next(s for s in sh if s[1] == 2); strs = sh[sym[6]]
main = next(s for s in sh if names[sh.index(s)] == "main")
VRAM, FOFF, FSZ = main[3], main[4], main[5]
funcs = []
for k in range(sym[5] // 16):
    n, val, sz, info, oth, shndx = struct.unpack_from("<IIIBBH", d, sym[4] + k * 16)
    if info & 0xf == 2 and names[shndx] == "main":
        funcs.append((val, sz, d[strs[4] + n: d.index(b"\0", strs[4] + n)].decode()))
funcs.sort()
_seen = {}
for i, (v, sz, nm) in enumerate(funcs):   # nombres repetidos (static en distintas unidades): sufijo con la direccion
    _seen[nm] = _seen.get(nm, 0) + 1
    if _seen[nm] > 1: funcs[i] = (v, sz, f"{nm}_dup_{v:X}")
def cls(nm):
    m = re.match(r"^.+?__(\d+)(.*)$", nm)
    return re.sub(r"\W", "_", m.group(2)[:int(m.group(1))]) if m else "lib"
# symbol_addrs
(ROOT / "config/symbol_addrs.txt").write_text("".join(f"{nm} = 0x{v:X}; // type:func size:0x{sz:X}\n" for v, sz, nm in funcs))
# runs
runs = []
for v, sz, nm in funcs:
    c = cls(nm)
    if runs and runs[-1][0] == c: runs[-1][2] = v + sz
    else: runs.append([c, v, v + sz])
text_start, text_end = funcs[0][0], max(v + sz for v, sz, _ in funcs)
y = ["name: Vampire Night (SCES-50411)", "sha1: 7bf30bab9a30b7ece6e5357ffffded97519dbabc", "options:",
     "  basename: vn", "  target_path: baserom/SCES_504.11", "  base_path: ..", "  platform: ps2", "  compiler: MWCCPS2",
     "  asm_path: asm", "  src_path: src", "  build_path: build", "  symbol_addrs_path: config/symbol_addrs.txt",
     "  ld_script_path: build/vn.ld", "  find_file_boundaries: False", "  segment_end_before_align: True",
     "segments:", f"  - [0x0, bin, elf_header]",
     "  - name: main", "    type: code", f"    start: 0x{FOFF:X}", f"    vram: 0x{VRAM:X}", "    subsegments:"]
if text_start > VRAM: y.append(f"      - [0x{FOFF:X}, asm, head]")
seen = {}
for c, a, b in runs:
    seen[c] = seen.get(c, 0) + 1
    y.append(f"      - [0x{FOFF + a - VRAM:X}, asm, {c}_{a:X}]")
y.append(f"      - [0x{FOFF + text_end - VRAM:X}, data, data]")
y += [f"  - [0x{FOFF + FSZ:X}, bin, tail]", f"  - [0x{len(d):X}]"]
(ROOT / "config/splat.yaml").write_text("\n".join(y) + "\n")
print(f"funciones main: {len(funcs)}; ficheros asm: {len(runs)}; texto 0x{text_start:X}-0x{text_end:X}")

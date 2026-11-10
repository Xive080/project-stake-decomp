#!/usr/bin/env python3
"""Valida el modelo de carga de los overlays (disc/TBL/*MOD.BIN, formato 'MWo3') contra los
simbolos del ELF y genera config/ov_<mod>.yaml + config/ov_<mod>_syms.txt para splat.
Modelo: el fichero se carga entero en `load` (la cabecera ocupa el inicio), sin reubicar."""
import re, struct
from pathlib import Path
from elftools.elf.elffile import ELFFile
ROOT = Path(__file__).resolve().parent.parent
e = ELFFile(open(ROOT / "baserom/SCES_504.11", "rb"))
names = {i: s.name for i, s in enumerate(e.iter_sections())}
st = e.get_section_by_name(".symtab")
by = {}
for s in st.iter_symbols():
    n = names.get(s["st_shndx"])
    if n and n.endswith("mod") and s["st_info"]["type"] == "STT_FUNC":
        by.setdefault(n, []).append((s["st_value"], s["st_size"], s.name))
tot = 0
for mod, fs in sorted(by.items()):
    p = ROOT / f"disc/TBL/{mod.upper()}.BIN"
    d = p.read_bytes(); w = struct.unpack_from("<8I", d)
    load, tsz, dsz, bsz = w[2], w[3], w[4], w[5]
    fs.sort(); ok = tot_ = 0
    for a, sz, n in fs:
        o = a - load
        if sz >= 16 and 0 <= o and o + sz <= len(d):
            tot_ += 1; ok += 0x03e00008 in struct.unpack_from("<4I", d, o + sz - 16)
    inrange = sum(1 for a, sz, n in fs if load <= a < load + len(d))
    print(f"{mod:8s} load=0x{load:X} filelen=0x{len(d):X} text=0x{tsz:X} funcs={len(fs)} en_rango={inrange} jr_ok={ok}/{tot_}")
    seen = {}; lines = []
    for a, sz, n in fs:
        seen[n] = seen.get(n, 0) + 1
        if seen[n] > 1: n = f"{n}_dup_{a:X}"
        lines.append(f"{n} = 0x{a:X}; // type:func size:0x{sz:X}\n")
    (ROOT / f"config/ov_{mod}_syms.txt").write_text("".join(lines))
    y = [f"name: Vampire Night overlay {mod}", "options:", f"  basename: {mod}", f"  target_path: disc/TBL/{mod.upper()}.BIN",
         "  base_path: ..", "  platform: ps2", "  compiler: MWCCPS2", f"  asm_path: asm/{mod}", f"  src_path: src/{mod}",
         "  build_path: build", f"  symbol_addrs_path: config/ov_{mod}_syms.txt", f"  ld_script_path: build/{mod}.ld",
         "  find_file_boundaries: False", "segments:", "  - [0x0, bin, header]  ",
         f"  - name: {mod}", "    type: code", "    start: 0x80", f"    vram: 0x{load + 0x80:X}", "    subsegments:",
         f"      - [0x80, asm, {mod}_text]", f"      - [0x{0x80 + max(0, min(tsz, len(d)) - 0x80):X}, data, {mod}_data]",
         f"  - [0x{len(d):X}]"]
    (ROOT / f"config/ov_{mod}.yaml").write_text("\n".join(y) + "\n")

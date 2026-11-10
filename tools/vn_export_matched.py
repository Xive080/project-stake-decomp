#!/usr/bin/env python3
"""Reparte src/auto2/small*.cpp (byte-matches de vn_small2.py) en src/matched/<modulo>_<grupo>.cpp, un fichero por clase."""
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "src/matched"
for f in sorted((ROOT / "src/auto2").glob("small*.cpp")):
    mod = f.stem[6:] or "main"; mod = mod.lstrip("_")
    txt = f.read_text().split("\n")
    i = next(k for k, l in enumerate(txt) if "@ 0x" in l and l.startswith("// "))
    decl = "\n".join(txt[1:i]); body = txt[i:]
    groups = {}
    pairs = [(body[k], body[k + 1]) for k in range(len(body) - 1) if body[k].startswith("// ") and " @ 0x" in body[k] and not body[k + 1].startswith("//")]
    for c, fn in pairs:
        nm = c[3:].split(" @ ")[0]
        m = re.match(r".*?__(\d+)(\D\w*)", nm)
        g = "free"
        if m:
            n = int(m.group(1)); g = (m.group(1) + m.group(2))[len(m.group(1)):][:n] or "free"
        groups.setdefault(g, []).append((c, fn))
    for g, fs in groups.items():
        p = OUT / f"{mod}_{g}.cpp"
        p.write_text(f"// Byte-match verificado contra SCES_504.11 con mwcps2-2.4-001213, flags: -lang c++ -O4,p\n"
                     f"// Modulo {mod}, grupo {g}. Generado por tools/vn_small2.py --module {mod} --max-instr 60 --permute 40;\n"
                     f"// cada funcion lleva su nombre real y direccion en el comentario.\n{decl}\n\n" + "\n".join(f"{c}\n{fn}" for c, fn in fs) + "\n")
    print(mod, sum(len(v) for v in groups.values()), "funciones en", len(groups), "ficheros")

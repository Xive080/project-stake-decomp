#!/usr/bin/env python3
"""Extrae mallas de los bloques de objeto (ps2_model): paquetes `S-32@0, STCYCL 0x105, V3-32@4, V4-8@5, V4-8@6, V2-32@7`
= n vertices (posicion float3, V4-8 normal?, V4-8 color?, UV float2) en tira de triangulos (hipotesis a validar).
Uso: afs_mesh.py FILE.AFS ENTRADA BLOQUE out.obj | render out.png"""
import struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import ps2_model as M, afs
from afs_unpack import parse_container

def meshes(x):
    """Devuelve lista de paquetes: dict(n, flag, pos[], nrm[], col[], uv[])."""
    m = M.parse_block(x); out = []
    if not 0 < len(m["tags"]) < 2048 or 0x30 + 16 * len(m["tags"]) > len(x): return out
    for t in m["tags"]:
        if not t["qwc"] or t["addr"] == 0 or t["addr"] + t["qwc"] * 16 > len(x): continue
        cm = M.parse_vif(x, t["addr"], t["qwc"])
        if len(cm) >= 7 and [c[0] for c in cm[:6]] == ["UNPACK S-32", "STCYCL", "STMASK", "UNPACK V3-32", "UNPACK V4-8", "UNPACK V4-8"] and cm[1][2] == 0x105:
            n = cm[3][1]; hd = struct.unpack_from("<4I", x, cm[0][3]); p3, pn, pc, pu = cm[3][3], cm[4][3], cm[5][3], cm[6][3]
            out.append(dict(n=n, flag=hd[1], pos=[struct.unpack_from("<3f", x, p3 + 12 * i) for i in range(n)],
                            nrm=[struct.unpack_from("<4b", x, pn + 4 * i) for i in range(n)], col=[struct.unpack_from("<4B", x, pc + 4 * i) for i in range(n)],
                            uv=[struct.unpack_from("<2f", x, pu + 8 * i) for i in range(n)]))
    return out

def to_obj(ms, path):
    v = 0; lines = []
    for m in ms:
        for p, u in zip(m["pos"], m["uv"]): lines.append(f"v {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}"); lines.append(f"vt {u[0]:.5f} {1 - u[1]:.5f}")
        for i in range(2, m["n"]):
            a, b, c = (v + i - 2, v + i - 1, v + i) if i % 2 == 0 else (v + i - 1, v + i - 2, v + i)
            lines.append(f"f {a+1}/{a+1} {b+1}/{b+1} {c+1}/{c+1}")
        v += m["n"]
    Path(path).write_text("\n".join(lines) + "\n")

def render(ms, path, size=400):
    from PIL import Image, ImageDraw
    pts = [p for m in ms for p in m["pos"]]
    if not pts: return False
    import math
    mn = [min(p[k] for p in pts) for k in range(3)]; mx = [max(p[k] for p in pts) for k in range(3)]
    sc = max(mx[k] - mn[k] for k in range(3)) or 1; c = [(mn[k] + mx[k]) / 2 for k in range(3)]
    im = Image.new("RGB", (size * 2, size), (20, 20, 20)); d = ImageDraw.Draw(im)
    for view, ox in ((0, 0), (1, size)):     # frontal (x,y) y lateral (z,y)
        def P(p):
            a = (p[0] - c[0]) if view == 0 else (p[2] - c[2]); b = -(p[1] - c[1])
            return ox + size / 2 + a / sc * size * 0.9, size / 2 + b / sc * size * 0.9
        for m in ms:
            for i in range(2, m["n"]):
                t = [P(m["pos"][i - 2]), P(m["pos"][i - 1]), P(m["pos"][i])]
                d.line(t + [t[0]], fill=(120, 200, 255))
    im.save(path); return True

if __name__ == "__main__":
    f, ents, _, _ = afs.parse(sys.argv[1]); o, s = ents[int(sys.argv[2])]; f.seek(o); x = parse_container(f.read(s))[int(sys.argv[3])]
    ms = meshes(x); out = sys.argv[4]
    (render if out.endswith(".png") else to_obj)(ms, out); print(len(ms), "paquetes", sum(m["n"] for m in ms), "vertices")

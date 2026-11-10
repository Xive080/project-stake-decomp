#!/usr/bin/env python3
"""Mallas por lotes de material (cabecera GIF de estado + paquetes de vertices) de los bloques de objeto de los AFS.

Cada lote de dibujo = un paquete de CABECERA ("segunda familia", `V4-32@0 x4, S-8@4 x12, STMASK, V2-32@4 x12, V4-8@16 x6, MSCAL 2`)
seguido de uno o mas paquetes de VERTICES (familia dominante, ver afs_mesh.py, MSCAL 4). La cabecera NO es geometria: es el
estado de dibujo que el microcodigo VU1 copia al paquete GIF de salida (ver docs/RENDER.md):
  quad 0  = (flags_cabecera, flags_estado, slot de paleta CLUT global, indice de textura)   [slot/textura == registro de material +0xC/+0x10]
  quad 1  = esfera envolvente (cx, cy, cz, radio) en espacio de objeto (culling: MSCAL 0 / pc 77 del microcodigo)
  quad 2  = plantilla de etiqueta GIF de los vertices (NREG=3: ST, RGBAQ, XYZF2); el PRIM de cada paquete de vertices se le inyecta
  quad 3  = etiqueta GIF A+D (EOP=1, NLOOP=9) y quads 4..12 = 9 registros A+D {dato 64 bits (V2-32), direccion (S-8)}:
            TEX0_1, TEX1_1, ALPHA_1, TEXA, TEST_1, MIPTBP1_1, MIPTBP2_1, PABE, CLAMP_1 (TEX0 sin TBP/PSM/CBP: los pone el motor)
  quads 16..21 = 6 x V4-8 de parametros de material/luz (escalados por constantes de fotograma, sin decodificar del todo).
Uso:
  afs_mesh2.py FILE.AFS ENTRADA BLOQUE out.png|out.obj   (render plano con normales / OBJ con un material por lote)
  afs_mesh2.py --check FILE.AFS [FILE2.AFS ...]          (comprobaciones objetivas sobre todos los bloques)"""
import struct, sys, math
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import ps2_model as M, afs
from afs_unpack import parse_container

GS_REG = {0x06: "TEX0_1", 0x14: "TEX1_1", 0x42: "ALPHA_1", 0x3B: "TEXA", 0x47: "TEST_1", 0x34: "MIPTBP1_1", 0x36: "MIPTBP2_1", 0x49: "PABE", 0x08: "CLAMP_1"}
ALPHA_SRC = {0: "Cs", 1: "Cd", 2: "0"}; ALPHA_A = {0: "As", 1: "Ad", 2: "FIX"}

def _bits(v, lo, n): return (v >> lo) & ((1 << n) - 1)

def decode_gs(name, v):
    """Decodifica el dato de 64 bits de un registro GS (solo los campos que el disco trae)."""
    if name == "ALPHA_1":
        a, b, c, d, fix = _bits(v, 0, 2), _bits(v, 2, 2), _bits(v, 4, 2), _bits(v, 6, 2), _bits(v, 32, 8)
        kind = ("opaco" if (a, b, d) == (2, 2, 0) or (a == b and d == 0) else
                "alfa" if (a, b, c, d) == (0, 1, 0, 1) else "aditivo" if (a, b, c, d) == (0, 2, 0, 1) else
                "aditivo_fix" if (a, b, c, d) == (0, 2, 2, 1) else "otro")
        return dict(A=a, B=b, C=c, D=d, FIX=fix, kind=kind, formula=f"(({ALPHA_SRC[a]}-{ALPHA_SRC[b]})*{ALPHA_A.get(c, '?')}>>7)+{ALPHA_SRC[d]}")
    if name == "TEST_1":
        return dict(ATE=_bits(v, 0, 1), ATST=_bits(v, 1, 3), AREF=_bits(v, 4, 8), AFAIL=_bits(v, 12, 2), DATE=_bits(v, 14, 1), ZTE=_bits(v, 16, 1), ZTST=_bits(v, 17, 2))
    if name == "CLAMP_1": return dict(WMS=_bits(v, 0, 2), WMT=_bits(v, 2, 2))     # 0 repeat, 1 clamp
    if name == "TEX1_1": return dict(LCM=_bits(v, 0, 1), MXL=_bits(v, 2, 3), MMAG=_bits(v, 5, 1), MMIN=_bits(v, 6, 3), K=_bits(v, 32, 12))
    if name == "TEX0_1": return dict(TCC=_bits(v, 34, 1), TFX=_bits(v, 35, 2), TBP0=_bits(v, 0, 14), PSM=_bits(v, 20, 6))   # TBP/PSM/CBP = 0 en disco
    if name == "TEXA": return dict(TA0=_bits(v, 0, 8), AEM=_bits(v, 15, 1), TA1=_bits(v, 32, 8))
    return dict(raw=v)

def decode_prim(w):
    """`w` = palabra alta del primer qword de la etiqueta GIF (q2 de S-32 del paquete de vertices): PRIM en bits 47..57 de la etiqueta."""
    p = (w >> 15) & 0x7FF
    return dict(prim=p, type=("pt", "line", "linestrip", "tri", "tristrip", "trifan", "sprite", "?")[p & 7], IIP=(p >> 3) & 1, TME=(p >> 4) & 1,
                FGE=(p >> 5) & 1, ABE=(p >> 6) & 1, AA=(p >> 7) & 1, FST=(p >> 8) & 1, CTXT=(p >> 9) & 1, FIX=(p >> 10) & 1, pre=(w >> 14) & 1)

def parse_header(x, cm, j):
    """cm[j] = UNPACK V4-32 (4 quads). Devuelve dict de cabecera o None."""
    c = cm[j]
    if not (c[0] == "UNPACK V4-32" and c[1] == 4 and j + 4 < len(cm) and cm[j + 1][0] == "UNPACK S-8" and cm[j + 1][1] == 12 and cm[j + 3][0] == "UNPACK V2-32"): return None
    q = struct.unpack_from("<16I", x, c[3]); f = struct.unpack_from("<4f", x, c[3] + 16)
    addr = struct.unpack_from("<12b", x, cm[j + 1][3]); dat = struct.unpack_from("<24I", x, cm[j + 3][3]); regs = {}
    for k in range(9):
        nm = GS_REG.get(addr[k], f"reg{addr[k]:#x}"); v = dat[2 * k] | (dat[2 * k + 1] << 32); regs[nm] = dict(decode_gs(nm, v), raw=v)
    mat = [struct.unpack_from("<4b", x, cm[j + 4][3] + 4 * k) for k in range(min(6, cm[j + 4][1]))] if cm[j + 4][0] == "UNPACK V4-8" else []
    return dict(flags=q[0], state=q[1], clut=struct.unpack_from("<i", x, c[3] + 8)[0], tex=struct.unpack_from("<i", x, c[3] + 12)[0], sphere=f,
                gif_tpl=q[8:12], gif_ad=q[12:16], regs=regs, mat=mat, regaddr=addr)

def parse_vertices(x, cm, j):
    """cm[j] = UNPACK S-32 de la familia dominante (n, flag, PRIM-estrip, PRIM-fan) -> dict o None."""
    if not (j + 6 < len(cm) and [c[0] for c in cm[j:j + 3]] == ["UNPACK S-32", "STCYCL", "STMASK"] and cm[j + 1][2] == 0x105 and cm[j + 3][0] == "UNPACK V3-32"): return None
    n = cm[j + 3][1]; hd = struct.unpack_from("<4I", x, cm[j][3]); p3, pn, pu = cm[j + 3][3], cm[j + 4][3], cm[j + 6][3]
    return dict(n=n, count_hdr=hd[0], flag=hd[1], prim_strip=hd[2], prim_fan=hd[3], pos=[struct.unpack_from("<3f", x, p3 + 12 * i) for i in range(n)],
                nrm=[struct.unpack_from("<3b", x, pn + 4 * i) for i in range(n)], uv=[struct.unpack_from("<2f", x, pu + 8 * i) for i in range(n)])

def batches(x):
    """Lista de lotes dict(header=..., packets=[...]) de un bloque. Los paquetes de vertices heredan la ultima cabecera vista."""
    m = M.parse_block(x); out = []; cur = None
    if not 0 < len(m["tags"]) < 2048 or 0x30 + 16 * len(m["tags"]) > len(x): return out
    for t in m["tags"]:
        if not t["qwc"] or t["addr"] == 0 or t["addr"] + t["qwc"] * 16 > len(x): continue
        cm = M.parse_vif(x, t["addr"], t["qwc"])
        for j in range(len(cm)):
            h = parse_header(x, cm, j)
            if h: cur = dict(header=h, packets=[]); out.append(cur); continue
            v = parse_vertices(x, cm, j)
            if v:
                if cur is None: cur = dict(header=None, packets=[]); out.append(cur)
                cur["packets"].append(v)
    return out

def strip_tris(n):
    """Triangulos de una tira de n vertices con la paridad de orientacion del GS. El microcodigo pone ADC=1 (no dibujar) en los dos primeros
    vertices y en los que tengan algun vertice de la terna fuera del volumen de recorte; aqui se dibujan todos (el disco no trae ADC)."""
    return [((i - 2, i - 1, i) if i % 2 == 0 else (i - 1, i - 2, i)) for i in range(2, n)]

def blend_class(h):
    if h is None: return "none"
    a = h["regs"].get("ALPHA_1"); return a["kind"] if a else "none"

def to_obj(bs, path):
    lines = ["mtllib batches.mtl"]; v = 0; mats = {}
    for bi, b in enumerate(bs):
        h = b["header"]; nm = f"b{bi}_tex{h['tex'] if h else -1}_{blend_class(h)}"; mats[nm] = h
        lines.append(f"usemtl {nm}")
        for p in b["packets"]:
            for q, u in zip(p["pos"], p["uv"]): lines.append(f"v {q[0]:.5f} {q[1]:.5f} {q[2]:.5f}"); lines.append(f"vt {u[0]:.5f} {1 - u[1]:.5f}")
            for a, bb, c in strip_tris(p["n"]):
                if len({p["pos"][a], p["pos"][bb], p["pos"][c]}) == 3: lines.append(f"f {v+a+1}/{v+a+1} {v+bb+1}/{v+bb+1} {v+c+1}/{v+c+1}")
            v += p["n"]
    Path(path).write_text("\n".join(lines) + "\n")
    Path(path).with_name("batches.mtl").write_text("".join(f"newmtl {k}\nKd 1 1 1\n" + (f"d {0.5 if blend_class(h) in ('alfa', 'aditivo') else 1}\n") for k, h in mats.items()))

def render(bs, path, size=500):
    """Render plano z-buffer: color = normal (Lambert) y tinte por clase de mezcla (opaco gris, alfa azul, aditivo naranja)."""
    import numpy as np
    from PIL import Image
    tri = []
    for b in bs:
        tint = {"alfa": (0.6, 0.8, 1.0), "aditivo": (1.0, 0.8, 0.5), "aditivo_fix": (1.0, 0.8, 0.5)}.get(blend_class(b["header"]), (0.85, 0.85, 0.85))
        for p in b["packets"]:
            P = np.array(p["pos"], float); N = np.array(p["nrm"], float) / 127.0
            for a, bb, c in strip_tris(p["n"]): tri.append((P[[a, bb, c]], N[[a, bb, c]], tint))
    if not tri: return False
    allp = np.concatenate([t[0] for t in tri]); mn, mx = allp.min(0), allp.max(0); c0 = (mn + mx) / 2; sc = (mx - mn).max() or 1
    img = np.zeros((size, size * 2, 3), np.uint8) + 25; light = np.array([0.3, 0.5, 0.8]); light /= np.linalg.norm(light)
    for view in (0, 1):
        zb = np.full((size, size), -1e9); ox = 0 if view == 0 else size
        for P, N, tint in tri:
            X = (P[:, 0] - c0[0]) if view == 0 else (P[:, 2] - c0[2]); X = X / sc * size * 0.9 + size / 2; Y = -(P[:, 1] - c0[1]) / sc * size * 0.9 + size / 2
            Z = P[:, 2] if view == 0 else -P[:, 0]
            x0, x1, y0, y1 = int(max(0, X.min())), int(min(size - 1, X.max())), int(max(0, Y.min())), int(min(size - 1, Y.max()))
            den = (Y[1] - Y[2]) * (X[0] - X[2]) + (X[2] - X[1]) * (Y[0] - Y[2])
            if abs(den) < 1e-9 or x1 < x0 or y1 < y0: continue
            xs, ys = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
            l1 = ((Y[1] - Y[2]) * (xs - X[2]) + (X[2] - X[1]) * (ys - Y[2])) / den; l2 = ((Y[2] - Y[0]) * (xs - X[2]) + (X[0] - X[2]) * (ys - Y[2])) / den; l3 = 1 - l1 - l2
            ok = (l1 >= 0) & (l2 >= 0) & (l3 >= 0); z = l1 * Z[0] + l2 * Z[1] + l3 * Z[2]; ok &= z > zb[y0:y1 + 1, x0:x1 + 1]
            if not ok.any(): continue
            nrm = l1[..., None] * N[0] + l2[..., None] * N[1] + l3[..., None] * N[2]; ln = np.linalg.norm(nrm, axis=2, keepdims=True) + 1e-9
            lam = np.clip((nrm / ln) @ light, 0, 1) * 0.8 + 0.2; col = (lam[..., None] * np.array(tint) * 255).astype(np.uint8)
            sub = img[y0:y1 + 1, ox + x0:ox + x1 + 1]; sub[ok] = col[ok]; zb[y0:y1 + 1, x0:x1 + 1][ok] = z[ok]
    Image.fromarray(img).save(path); return True

def check(paths):
    """Comprobaciones objetivas: conteos y coherencia cabecera <-> vertices <-> registro de material."""
    import collections, numpy as np
    S = collections.Counter(); ratios = []
    for pth in paths:
        f, ents, _, _ = afs.parse(pth)
        for i, (o, s) in enumerate(ents):
            if not s: continue
            f.seek(o)
            try: bl = parse_container(f.read(s))
            except Exception: continue
            for b, x in enumerate(bl or []):
                if len(x) < 0x30 or struct.unpack_from("<I", x, 0)[0] != 0x1000: continue
                try: bs = batches(x); recs = M.materials(x)
                except Exception: continue
                tags = M.parse_block(x)["tags"]
                for bt in bs:
                    h = bt["header"]
                    if h is None: S["paquetes sin cabecera"] += len(bt["packets"]); continue
                    S["cabeceras"] += 1; S["aduana: 9 regs A+D con direcciones esperadas"] += (list(h["regaddr"][:9]) == [6, 0x14, 0x42, 0x3b, 0x47, 0x34, 0x36, 0x49, 8])
                    S["cabeceras con >=1 paquete de vertices"] += bool(bt["packets"])
                    c = h["sphere"][:3]; r = h["sphere"][3]
                    for p in bt["packets"]:
                        P = np.array(p["pos"]); S["paquetes de vertices"] += 1; S["vertices"] += p["n"]
                        S["n == cuenta de cabecera"] += (p["n"] == p["count_hdr"])
                        pr = decode_prim(p["prim_strip"]); S["prim=tristrip"] += pr["type"] == "tristrip"; S["prim fan (alta+1)"] += decode_prim(p["prim_fan"])["prim"] == pr["prim"] + 1
                        S["flag bit6 == PRIM.IIP"] += (((p["flag"] >> 6) & 1) == pr["IIP"])
                        if np.isfinite(P).all() and r == r:
                            d = np.linalg.norm(P - np.array(c), axis=1); S["vertices dentro de la esfera (x1.001+eps)"] += int((d <= r * 1.001 + 1e-3).sum())
                            if d.max() > 0 and r > 0: ratios.append(float(d.max() / r))
                        deg = sum(1 for a, bb, cc in strip_tris(p["n"]) if len({p["pos"][a], p["pos"][bb], p["pos"][cc]}) < 3); S["triangulos degenerados"] += deg; S["triangulos de tira"] += max(0, p["n"] - 2)
        # cabecera.tex/clut vs registro de material que contiene el paquete
    print(f"{'; '.join(Path(p).name for p in paths)}")
    for k, v in S.items(): print(f"  {k}: {v}")
    if ratios: a = np.array(ratios); print(f"  max(dist)/radio por paquete: mediana {np.median(a):.3f}, p10 {np.percentile(a, 10):.3f}, p90 {np.percentile(a, 90):.3f}, max {a.max():.3f}")

if __name__ == "__main__":
    a = sys.argv[1:]
    if a and a[0] == "--check": check(a[1:]); sys.exit()
    f, ents, _, _ = afs.parse(a[0]); o, s = ents[int(a[1])]; f.seek(o); x = parse_container(f.read(s))[int(a[2])]
    bs = batches(x); out = a[3]
    (render if out.endswith(".png") else to_obj)(bs, out)
    print(len(bs), "lotes", sum(len(b["packets"]) for b in bs), "paquetes", sum(p["n"] for b in bs for p in b["packets"]), "vertices")
    for b in bs[:12]:
        h = b["header"]
        if h: print(f"  tex={h['tex']} clut={h['clut']} blend={blend_class(h)} clamp={h['regs']['CLAMP_1']['WMS']}/{h['regs']['CLAMP_1']['WMT']} test={h['regs']['TEST_1']['ATST']} paquetes={len(b['packets'])} sphere={tuple(round(v, 2) for v in h['sphere'])}")

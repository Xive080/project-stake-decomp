#!/usr/bin/env python3
"""Exporta un banco de objetos completo (mallas + materiales + texturas) a OBJ/MTL/PNG.
Uso: afs_export.py AFS BANCO [OUTDIR]   (BANCO = indice de las tablas del ELF 0x26C490/0x26C6B0, ver afs_ids.py)
Cada bloque del contenedor de objetos pasa a `obj_NNN.obj`; todos comparten `bank.mtl` y `tex_NN.png`.
El material de cada paquete sale del registro de material que lo contiene (ps2_model.materials; indice de textura en +0x10).
Indices de textura >= nº de subidas del fichero de texturas (ocurre en algunos bancos) quedan sin textura."""
import struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import numpy as np
from PIL import Image
import afs, ps2_model as M, gs_stream as G, gs_vram as V, afs_mesh as A
from afs_unpack import parse_container

ELF = Path(__file__).resolve().parent.parent / "baserom" / "SCES_504.11"

def bank_ids(bank):
    d = ELF.read_bytes(); at = lambda v: v - 0x140000 + 0x800
    return (struct.unpack_from("<h", d, at(0x26C490) + 2 * bank)[0], struct.unpack_from("<h", d, at(0x26C6B0) + 2 * bank)[0])

def objtbl(afs_path):
    d = (Path(afs_path).resolve().parent.parent / "TBL" / "OBJTBL.BIN").read_bytes()
    out = []
    for i in range(len(d) // 32):
        r = struct.unpack_from("<8I", d, i * 32); out.append(dict(psm=r[0] >> 16, tw=r[2] & 0xffff, th=r[2] >> 16, tbp=r[4], kind=r[6]))
    return out

def _rough(img):
    c = img[..., :3].astype(int); return abs(np.diff(c, axis=1)).mean() + abs(np.diff(c, axis=0)).mean()

ELF_PAL_TABLE = 0x28BA10      # PolyMgr::init1st2: 154 paletas globales {ptr, ncolores u16, flag u16} (flag 1 = CT16, 8 = CT32)

def elf_palette(afs_path, slot):
    """Paleta global `slot` (el campo +0xC de un material indexa esta tabla, no hay paletas por banco). Devuelve RGBA (n,4) en orden de indice."""
    e = (Path(afs_path).resolve().parent.parent.parent / "baserom" / "SCES_504.11").read_bytes(); off = lambda v: v - 0x140000 + 0x800
    p, n, fl = struct.unpack_from("<IHH", e, off(ELF_PAL_TABLE + 8 * slot)); d = e[off(p):off(p) + n * (2 if fl == 1 else 4)]
    if fl == 1: c = np.frombuffer(d, "<u2").astype(np.int64); rgba = np.stack([(c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31, np.where(c & 0x8000, 255, 0)], 1)
    else: c = np.frombuffer(d, np.uint8).reshape(-1, 4).astype(np.int64); rgba = np.stack([c[:, 0], c[:, 1], c[:, 2], np.minimum(255, c[:, 3] * 2)], 1)
    if n == 256: i = np.arange(256); rgba = rgba[(i & 0xe7) | ((i & 8) << 1) | ((i & 0x10) >> 1)]      # CSM1: el dato va en orden de bloque GS
    return rgba.astype(np.uint8)

def texture_images(buf, table, modes=None, afs_path=None):
    """Texturas del banco: el fichero de texturas es una instantanea de VRAM; los descriptores (psm, tamano, tbp) salen de OBJTBL.BIN.
    `modes` = {indice de textura: slot de paleta global} (campo +0xC de los materiales). Devuelve (lista de PIL RGBA o None, base)."""
    ups, _, _ = G.parse_chain(buf); ups = [u for u in ups if "w" in u]
    base = next((i for i in range(len(table) - len(ups)) if all(table[i + 1 + k]["tbp"] == ups[k]["dbp"] for k in range(len(ups)))), None)
    if base is None: return [], None
    vr = V.Vram()
    for u in ups: vr.upload_ct32(u["data"], u["w"], u["h"], u["dbp"], u["dbw"], u["dpsm"])
    recs = [table[base + 1 + k] for k in range(len(ups))]; modes = modes or {}; out = []
    for k, r in enumerate(recs):
        w, h = 1 << r["tw"], 1 << r["th"]
        t = V.read_texture(vr, r["psm"], w, h, r["tbp"])
        if r["psm"] == 0: im = np.stack([(t >> s) & 255 for s in (0, 8, 16, 24)], 2).astype(np.uint8); im[..., 3] = np.minimum(255, im[..., 3].astype(int) * 2)
        elif r["psm"] == 2:
            e = t.astype(np.int64); im = np.stack([(e & 31) * 255 // 31, ((e >> 5) & 31) * 255 // 31, ((e >> 10) & 31) * 255 // 31, np.where(e & 0x8000, 255, 0)], 2).astype(np.uint8)
        elif r["psm"] == 19 and modes.get(k, -1) >= 0 and afs_path: im = elf_palette(afs_path, modes[k])[t]
        elif r["psm"] in (19, 20): n = 256 if r["psm"] == 19 else 16; im = np.stack([t * (255 // (n - 1))] * 3 + [np.full_like(t, 255)], 2)
        else: out.append(None); continue
        out.append(Image.fromarray(np.ascontiguousarray(im.astype(np.uint8))))
    return out, base

def packets_by_material(x):
    """[(material_idx, textura, paquete)] asignando cada paquete de malla al registro de material que contiene su carga."""
    recs = M.materials(x); res = []
    m = M.parse_block(x)
    for t in m["tags"]:
        if not t["qwc"] or t["addr"] == 0 or t["addr"] + t["qwc"] * 16 > len(x): continue
        cm = M.parse_vif(x, t["addr"], t["qwc"])
        if len(cm) >= 7 and [c[0] for c in cm[:6]] == ["UNPACK S-32", "STCYCL", "STMASK", "UNPACK V3-32", "UNPACK V4-8", "UNPACK V4-8"] and cm[1][2] == 0x105:
            k = next((i for i, r in enumerate(recs) if r["off"] <= t["addr"] < r["off"] + 0xE0 + r["size"]), -1)
            res.append((k, recs[k]["tex"] if k >= 0 else -1, cm))
    return res

def export(blocks, nimgs, outdir):
    n = 0; out = Path(outdir); used = set()
    for bi, x in enumerate(blocks):
        pk = packets_by_material(x)
        if not pk: continue
        lines = ["mtllib bank.mtl"]; v = 0
        for k, tex, cm in pk:
            nv = cm[3][1]; p3, pn, pc, pu = cm[3][3], cm[4][3], cm[5][3], cm[6][3]
            mat = f"tex_{tex:02d}" if 0 <= tex < nimgs else "none"; used.add(mat); lines.append(f"usemtl {mat}")
            for i in range(nv):
                px, py, pz = struct.unpack_from("<3f", x, p3 + 12 * i); u, w = struct.unpack_from("<2f", x, pu + 8 * i)
                lines.append(f"v {px:.5f} {py:.5f} {pz:.5f}"); lines.append(f"vt {u:.5f} {1 - w:.5f}")
            for i in range(2, nv):
                a, b, c = (v + i - 2, v + i - 1, v + i) if i % 2 == 0 else (v + i - 1, v + i - 2, v + i)
                if len({tuple(struct.unpack_from('<3f', x, p3 + 12 * j)) for j in (a - v, b - v, c - v)}) < 3: continue   # triangulo degenerado
                lines.append(f"f {a+1}/{a+1} {b+1}/{b+1} {c+1}/{c+1}")
            v += nv
        (out / f"obj_{bi:03d}.obj").write_text("\n".join(lines) + "\n"); n += 1
    mtl = [f"newmtl {m}\nKd 1 1 1\nmap_Kd {m}.png\nd 1\n" if m != "none" else "newmtl none\nKd 0.6 0.6 0.6\n" for m in sorted(used | {"none"})]
    (out / "bank.mtl").write_text("\n".join(mtl)); return n

if __name__ == "__main__":
    path, bank = sys.argv[1], int(sys.argv[2]); out = Path(sys.argv[3]) if len(sys.argv) > 3 else Path("build/export") / f"{Path(path).stem}_bank{bank}"
    out.mkdir(parents=True, exist_ok=True); oid, tid = bank_ids(bank); f, ents, _, _ = afs.parse(path)
    if not ents[oid][1]: sys.exit(f"{Path(path).stem}: el objeto {oid} esta vacio")
    f.seek(ents[oid][0]); blocks = parse_container(f.read(ents[oid][1])) or []
    imgs = []; modes = {}
    for x in blocks:
        try:
            for m in M.materials(x):
                if m["mode"] >= 0: modes.setdefault(m["tex"], m["mode"])
        except Exception: pass
    if ents[tid][1]:
        f.seek(ents[tid][0]); imgs, base = texture_images(f.read(ents[tid][1]), objtbl(path), modes, path); print("descriptores OBJTBL desde el registro", base)
        for k, im in enumerate(imgs):
            if im is not None: im.save(out / f"tex_{k:02d}.png")
    n = export(blocks, len(imgs), out); print(f"banco {bank}: objeto {oid} ({len(blocks)} bloques, {n} con malla), texturas {tid} ({len(imgs)}) -> {out}")

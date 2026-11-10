#!/usr/bin/env python3
"""Exporta un banco de objetos al formato binario del motor nativo (VNM1) + texturas PNG.
Uso: vnm_export.py AFS BANCO OUTDIR
VNM1 (little endian): "VNM1", u32 nmesh, u32 ntex; por malla: i32 bloque, i32 textura (-1 = ninguna), u32 nv, u32 ni, nv x {f32 x,y,z, f32 u,v, u8 r,g,b,a, i8 nx,ny,nz,0}, ni x u32.
Las tiras de triangulos ya vienen convertidas a lista de triangulos."""
import struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import afs, afs_export as X, ps2_model as M
from afs_unpack import parse_container

def mesh_of(x, pk):
    out = {}
    for k, tex, cm in pk:
        nv = cm[3][1]; p3, pn, pc, pu = cm[3][3], cm[4][3], cm[5][3], cm[6][3]
        V, I = out.setdefault(tex, ([], [])); base = len(V)
        for i in range(nv):
            px, py, pz = struct.unpack_from("<3f", x, p3 + 12 * i); u, w = struct.unpack_from("<2f", x, pu + 8 * i)
            nx, ny, nz = struct.unpack_from("<3b", x, pn + 4 * i); r, g, bb, a = x[pc + 4 * i: pc + 4 * i + 4]
            V.append(struct.pack("<5f4B4b", px, py, pz, u, 1 - w, r, g, bb, min(255, a * 2), nx, ny, nz, 0))
        for i in range(2, nv):
            j = base + i; tri = (j - 2, j - 1, j) if i % 2 == 0 else (j - 1, j - 2, j)
            I.append(tri)
    return out

def main(path, bank, out):
    out = Path(out); out.mkdir(parents=True, exist_ok=True); oid, tid = X.bank_ids(bank); f, ents, _, _ = afs.parse(path)
    f.seek(ents[oid][0]); blocks = parse_container(f.read(ents[oid][1])) or []
    modes = {}
    for x in blocks:
        try:
            for m in M.materials(x):
                if m["mode"] >= 0: modes.setdefault(m["tex"], m["mode"])
        except Exception: pass
    imgs = []
    if ents[tid][1]:
        f.seek(ents[tid][0]); imgs, _ = X.texture_images(f.read(ents[tid][1]), X.objtbl(path), modes, path)
        for k, im in enumerate(imgs):
            if im is not None: im.save(out / f"tex_{k:02d}.png")
    meshes = []
    for bi, x in enumerate(blocks):
        pk = X.packets_by_material(x)
        for tex, (V, I) in mesh_of(x, pk).items():
            if not I: continue
            t = tex if 0 <= tex < len(imgs) and imgs[tex] is not None else -1
            meshes.append(struct.pack("<iiII", bi, t, len(V), len(I) * 3) + b"".join(V) + b"".join(struct.pack("<3I", *tri) for tri in I))
    (out / "bank.vnm").write_bytes(b"VNM1" + struct.pack("<II", len(meshes), len(imgs)) + b"".join(meshes))
    print(f"banco {bank}: {len(meshes)} mallas, {len(imgs)} texturas -> {out}")

if __name__ == "__main__": main(sys.argv[1], int(sys.argv[2]), sys.argv[3])

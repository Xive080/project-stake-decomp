#!/usr/bin/env python3
"""Decodificador de FS/COLLIDAT.BIN (colision de etapa) de Vampire Night PAL.
Carga (ColliMgr::entryExec_Init/SeekEndWait/ReadEndWait, asm/ColliMgrClass_1D30F0.s): EventCtrlTask::ENTRYCOLLI(n) pide
n (0..7); ColliMgr lee de COLLIDAT.BIN el trozo n (tabla de saltos jtbl_002C0C50, tamanos y desplazamientos en CHUNKS) a RAM:
n=0 -> 0x1ECD000, n>=1 -> 0x1ECD800. Los scripts de EVTPAL de la etapa s (0..5) piden ENTRYCOLLI 0 y ENTRYCOLLI s+1.
Formato de un trozo: grupos consecutivos hasta relleno a cero.
  grupo   = u32 tipo (siempre 1), u32 npoly, 6 f32 AABB (xmax,ymax,zmax,xmin,ymin,zmin), npoly poligonos
  poligono (72 B) = f32[4] plano (nx,ny,nz,d con n.v + d = 0), u32 kind (0/1/2), 4 vertices f32[3], u32 mat (id de material/superficie)
  kind 1: 3 vertices distintos en 2285 de 2661 casos (4o repetido); kind 0/2 mezclan 3 y 4. Significado de kind y mat sin confirmar.
Uso: afs_colli.py [--obj SALIDA_DIR]"""
import struct, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
COLLI = ROOT / "disc" / "FS" / "COLLIDAT.BIN"
# (desplazamiento, tamano) por n, de la tabla de saltos y las constantes de entryExec_Init
CHUNKS = [(0, 0x800), (0x800, 0xA000), (0xA800, 0x2C000), (0x36800, 0x6000), (0x3C800, 0x1800), (0x3E000, 0x9000), (0x47000, 0x3800), (0x4A800, 0x2000)]

def parse_chunk(d, off, size):
    """-> (grupos, fin) ; grupo = (tipo, aabb, [(plano, kind, [4 vertices], mat)])."""
    groups = []; o = off; end = off + size
    while o + 32 <= end:
        t, n = struct.unpack_from("<II", d, o)
        if t == 0 and n == 0: break
        if t != 1 or o + 32 + 72 * n > end: raise ValueError("cabecera de grupo invalida en 0x%X" % o)
        bb = struct.unpack_from("<6f", d, o + 8); p = o + 32; polys = []
        for _ in range(n):
            pl = struct.unpack_from("<4f", d, p); k = struct.unpack_from("<I", d, p + 16)[0]
            v = [struct.unpack_from("<3f", d, p + 20 + 12 * j) for j in range(4)]
            polys.append((pl, k, v, struct.unpack_from("<I", d, p + 68)[0])); p += 72
        groups.append((t, bb, polys)); o = p
    return groups, o

def load(path=COLLI):
    d = Path(path).read_bytes(); return [parse_chunk(d, o, s) for o, s in CHUNKS], d

def check(d):
    """Comprobaciones objetivas por trozo; devuelve lista de dicts."""
    res = []
    for i, (o, s) in enumerate(CHUNKS):
        g, end = parse_chunk(d, o, s); tot = inb = cop = unit = 0; worst = 0.0
        for t, bb, polys in g:
            for pl, k, v, m in polys:
                tot += 1
                inb += all(bb[3 + a] - 1e-3 <= vv[a] <= bb[a] + 1e-3 for vv in v for a in range(3))
                n = (pl[0], pl[1], pl[2]); ln = sum(c * c for c in n) ** 0.5; unit += abs(ln - 1) < 1e-2
                e = max(abs(sum(n[a] * vv[a] for a in range(3)) + pl[3]) for vv in v[:3]); cop += e < 0.05; worst = max(worst, e)
        res.append(dict(chunk=i, groups=len(g), polys=tot, in_aabb=inb, unit_normal=unit, plane_ok_first3=cop, worst_plane_err=round(worst, 3),
                        tail_zero=not any(d[end:o + s]), bytes_used=end - o))
    return res

def export_obj(d, outdir):
    outdir = Path(outdir); outdir.mkdir(parents=True, exist_ok=True)
    for i, (o, s) in enumerate(CHUNKS):
        g, _ = parse_chunk(d, o, s); L = []; F = []; n = 0
        for t, bb, polys in g:
            for pl, k, v, m in polys:
                uniq = []
                for vv in v:
                    if vv not in uniq: uniq.append(vv)
                if len(uniq) < 3: continue
                for vv in uniq: L.append("v %f %f %f" % vv)
                F.append("f " + " ".join(str(n + j + 1) for j in range(len(uniq)))); n += len(uniq)
        (outdir / ("colli_%d.obj" % i)).write_text("\n".join(L + F) + "\n")

if __name__ == "__main__":
    chunks, d = load()
    for r in check(d): print(r)
    if len(sys.argv) > 2 and sys.argv[1] == "--obj": export_obj(d, sys.argv[2])

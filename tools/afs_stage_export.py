#!/usr/bin/env python3
"""Exporta el DECORADO de una fase (hoy: ST1, tabla BG 0) a VNM1 + tex_NN.png (compatible con engine/vn_viewer) y renderiza una captura desde una camara.

Donde esta el decorado (ver docs/LEVELS.md, "Decorado de la fase 1"):
  * El opcode BGTB (EVTPAL, opcode 49 = plantilla 45 + 4; EventCtrlTask::BGTB -> BGCtrlClass::appointBG) elige un estado de fondo `n` (u16 en pc+4).
  * BGCtrlClass::setBGCtrlTbl(fase) apunta a dos tablas del ELF: D_002665A0[fase] (listas de estado: 26 x s16, fin = -1, 52 B por estado)
    y D_002665D0[fase] (entradas de 28 B: s32 id_primero, s32 id_ultimo, s16 flags, s16 +0xA, f32 x,y,z, s32).
    PutForEntry_sub dibuja, para cada indice de la lista del estado, los objetos id_primero..id_ultimo con PutPolyMatF(id); id = (banco << 16) | bloque.
  * Los objetos del decorado de ST1 son los bancos 175..179 (entradas AFS 348..356), mas 180 (cielo, flags != 0) y 58/256 (ver docs).
  * Las mallas ya estan en coordenadas de mundo (la entrada no trae matriz de colocacion; flags = 0 -> sin transformacion). Eje Y ARRIBA (normales int8 de suelos con ny>0 en 58-65% de los vertices de los bancos de terreno).
  * Texturas: los indices de textura de los materiales de un banco N indexan los registros OBJTBL del fichero `tex` que PRECEDE a su objeto en el AFS
    (entrada obj-1), no el de la tabla 0x26C6B0. Registros consecutivos desde el puntero de banco (OBJTBL cola 0x83FA4 + 4*banco) + 1.
    Material -2 = T4/T8 con la paleta en el registro siguiente; -1 = color directo.
Uso (copia de trabajo; opciones nuevas --extra B:b0-b1,.. [--extra-flags F] [--only] [--exclude B:b0-b1,..] [--flipv B:TEX,..]):
  afs_stage_export.py OUTDIR [--stage 0] [--bg N|all] [--sky] [--scale S] [--skip-banks 58,256]
                      [--shot IMG.png --route P --cam K [--frame T] [--size 960x540]]
  --bg: estado de fondo 1..5 (BGTB) o `all` (union, por defecto).  --sky: incluye entradas con flags != 0 (cielo/animadas).
  --scale: escala uniforme de salida (vn_viewer recorta a 5000 unidades; con ~0.3 cabe la fase entera).
  --shot: renderiza (CPU, numpy) el decorado desde la camara K de la ruta P de ST1 (afs_campath) en el cuadro T (por defecto el primero)."""
import json, struct, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import numpy as np
from PIL import Image
import afs, afs_export as X, afs_mesh2 as M2, gs_stream as G, gs_vram as V
from afs_unpack import parse_container

ELF = ROOT / "baserom" / "SCES_504.11"
OBJTBL_BANKPTR = 0x83FA4            # tabla de u32 (RAM 0x1FADFA4) = puntero de registros por banco; el registro 0 de cada banco es una cabecera vacia
OBJTBL_RAM = 0x1F2A000
BG_LISTS, BG_ENTRIES = 0x2665A0, 0x2665D0   # D_002665A0 / D_002665D0 (u32 por fase)
off = lambda v: v - 0x140000 + 0x800

def bg_tables(stage):
    """-> (listas {estado: [indices de entrada]}, entradas [dict])."""
    d = ELF.read_bytes(); a = struct.unpack_from("<I", d, off(BG_LISTS) + 4 * stage)[0]; b = struct.unpack_from("<I", d, off(BG_ENTRIES) + 4 * stage)[0]
    b_next = struct.unpack_from("<I", d, off(BG_LISTS) + 4 * (stage + 1))[0]
    lists = {}
    for s in range((b - a) // 52):
        L = []
        for k in range(26):
            v = struct.unpack_from("<h", d, off(a) + 52 * s + 2 * k)[0]
            if v == -1: break
            L.append(v)
        lists[s] = L
    ents = []
    for e in range((b_next - b) // 28):
        r = struct.unpack_from("<iihhfffi", d, off(b) + 28 * e)
        ents.append(dict(first=r[0], last=r[1], flags=r[2], f2=r[3], xyz=r[4:7], bank=r[0] >> 16, b0=r[0] & 0xffff, b1=r[1] & 0xffff))
    return lists, ents

class Bank:
    """Un banco de objetos de un AFS de fase + las imagenes de su fichero de texturas (entrada obj-1)."""
    def __init__(self, path, f, ents, bank, objtbl):
        self.bank = bank; self.oid = X.bank_ids(bank)[0]
        f.seek(ents[self.oid][0]); self.blocks = parse_container(f.read(ents[self.oid][1])) or []
        self.tid = self.oid - 1; self.images = {}; self.base = None
        ptr = struct.unpack_from("<I", (ROOT / "disc" / "TBL" / "OBJTBL.BIN").read_bytes(), OBJTBL_BANKPTR + 4 * bank)[0]
        self.ptr_rec = (ptr - OBJTBL_RAM) // 32      # registro de cabecera propio del banco; se usa solo como referencia (no se confia en el)
        if ents[self.tid][1]: self._images(f, ents, objtbl)
    def _images(self, f, ents, tab):
        f.seek(ents[self.tid][0]); ups, _, _ = G.parse_chain(f.read(ents[self.tid][1])); ups = [u for u in ups if "w" in u]
        base = next((i for i in range(len(tab) - len(ups)) if all(tab[i + 1 + k]["tbp"] == ups[k]["dbp"] for k in range(len(ups)))), None)
        self.base = base
        if base is None: return
        vr = V.Vram()
        for u in ups: vr.upload_ct32(u["data"], u["w"], u["h"], u["dbp"], u["dbw"], u["dpsm"])
        for k in range(len(ups)):
            r = tab[base + 1 + k]; w, h = 1 << r["tw"], 1 << r["th"]; psm = r["psm"]
            if psm not in (0, 2, 19, 20): continue
            t = V.read_texture(vr, psm, w, h, r["tbp"])
            if psm == 0: im = np.stack([(t >> s) & 255 for s in (0, 8, 16, 24)], 2).astype(np.uint8); im[..., 3] = np.minimum(255, im[..., 3].astype(int) * 2)
            elif psm == 2:
                e = t.astype(np.int64); im = np.stack([(e & 31) * 255 // 31, ((e >> 5) & 31) * 255 // 31, ((e >> 10) & 31) * 255 // 31, np.where(e & 0x8000, 255, 0)], 2).astype(np.uint8)
            else:
                nxt = tab[base + 2 + k] if k + 1 < len(ups) else None
                if nxt is None or nxt["kind"] != 2: continue          # sin paleta en el registro siguiente: no se decodifica
                # paleta = imagen del registro siguiente leida en raster (entrada i = pixel i; medido: aspereza de la imagen resultante mucho menor que
                # leyendo palabras lineales con clut_rgba); T8 con la permutacion CSM1 (i&0xe7)|((i&8)<<1)|((i&0x10)>>1)
                n = 256 if psm == 19 else 16
                px = np.asarray(V.read_texture(vr, nxt["psm"], 1 << nxt["tw"], 1 << nxt["th"], nxt["tbp"]), np.int64).reshape(-1)[:n]
                if nxt["psm"] == 0: pal = np.stack([(px >> s) & 255 for s in (0, 8, 16, 24)], 1); pal[:, 3] = np.minimum(255, pal[:, 3] * 2)
                else: pal = np.stack([(px & 31) * 255 // 31, ((px >> 5) & 31) * 255 // 31, ((px >> 10) & 31) * 255 // 31, np.where(px & 0x8000, 255, 0)], 1)
                if n == 256: i = np.arange(256); pal = pal[(i & 0xe7) | ((i & 8) << 1) | ((i & 0x10) >> 1)]
                im = pal.astype(np.uint8)[np.asarray(t, np.int64)]
            self.images[k] = Image.fromarray(np.ascontiguousarray(im.astype(np.uint8)))

ST1_TABLE, ST1_N = 0x266660, 12      # PutForEntry_st1 (0x1C8880): tabla fija de 12 entradas de 28 B que dibuja el juego cuando gp-0x73D8 == 0 (verificado con RAM viva)

def st1_entries():
    d = ELF.read_bytes(); ents = []
    for e in range(ST1_N):
        r = struct.unpack_from("<iihhfffi", d, off(ST1_TABLE) + 28 * e)
        ents.append(dict(first=r[0], last=r[1], flags=r[2], f2=r[3], xyz=r[4:7], bank=r[0] >> 16, b0=r[0] & 0xffff, b1=r[1] & 0xffff))
    return ents

def parse_extra(spec, flags=0):
    """'58:35-36,178:20' -> entradas extra (banco:b0[-b1]) con flags dados (indices 300+); sirve para probar candidatos."""
    r = []
    for i, t in enumerate(x for x in spec.split(",") if x):
        bk, rng = t.split(":"); lo, _, hi = rng.partition("-")
        r.append((300 + i, dict(first=0, last=0, flags=flags, f2=0, xyz=(0, 0, 0), bank=int(bk), b0=int(lo), b1=int(hi or lo))))
    return r

ST1_S5_EXCLUDE = [];   # vacia (2026-11-10): niebla/bosque no eran opacos por la paleta sino por falta de FLIPV en 176:20 (--raw)
ST1_S5_FLIPV =[(179, 0), (176, 20)]   # por defecto con --bg 5 (sin --raw); verificado a ojo contra la captura
EXTRA = []; ONLY = False; EXCL = set(); FLIPV = set()   # --extra/--only, --exclude, --flipv (ver __main__)
def selected_objects(stage, bg):
    st, sel = _selected_objects(stage, bg)
    return st, (EXTRA if ONLY else sel + EXTRA)

def _selected_objects(stage, bg):
    if bg == "st1": ents = st1_entries(); return [-1], [(100 + i, e) for i, e in enumerate(ents)]
    lists, ents = bg_tables(stage)
    states = sorted(s for s in lists if s > 0) if bg == "all" else [int(bg)]
    seen = [];
    for s in states:
        for e in lists[s]:
            if e not in seen: seen.append(e)
    return states, [(e, ents[e]) for e in seen]

def build(path, stage, bg, sky, skip=()):
    f, ents, _, _ = afs.parse(path); tab = X.objtbl(path)
    states, sel = selected_objects(stage, bg); banks = {}; objs = []
    for ei, e in sel:
        if (e["flags"] != 0 and not sky) or e["bank"] in skip: continue
        for blk in range(e["b0"], e["b1"] + 1):
            if (e["bank"], blk) in EXCL: continue
            if (e["bank"], blk) not in {(o[0], o[1]) for o in objs}: objs.append((e["bank"], blk, ei))
    for b in sorted({o[0] for o in objs}):
        try: banks[b] = Bank(path, f, ents, b, tab)
        except Exception as ex: print("banco", b, "no cargado:", ex, file=sys.stderr)
    return states, objs, banks

def blend_code(h):
    """Clase de mezcla del lote para VNM2: bits 0-1 = 0 opaco / 1 alfa (Cs-Cd)*As+Cd / 2 aditivo Cs*As+Cd; bit 2 = As fijo (FIX); bits 8-15 = FIX (128 = 1.0)."""
    k = M2.blend_class(h)
    if k == "alfa": return 1
    if k == "aditivo": return 2
    if k == "aditivo_fix": return 2 | 4 | (h["regs"]["ALPHA_1"]["FIX"] << 8)
    return 0

def export(path, out, stage=0, bg="all", sky=False, scale=1.0, skip=()):
    out = Path(out); out.mkdir(parents=True, exist_ok=True)
    states, objs, banks = build(path, stage, bg, sky, skip); ents_flags = {ei: e["flags"] for ei, e in selected_objects(stage, bg)[1]}; texid = {}; meshes = []; info = []; notex = {}
    for bank, blk, ei in objs:
        B = banks.get(bank)
        if B is None or blk >= len(B.blocks): continue
        per = {}; follow = bool(ents_flags.get(ei, 0) & 2)
        for bt in M2.batches(B.blocks[blk]):
            h = bt["header"]; t = h["tex"] if h else -1; bl = blend_code(h)
            for p in bt["packets"]:
                V_, I_ = per.setdefault((t, bl), ([], [])); base = len(V_)
                for q, u, nn in zip(p["pos"], p["uv"], p["nrm"]):
                    y = q[1]
                    V_.append(struct.pack("<5f4B4b", q[0] * scale, y * scale, q[2] * scale, u[0], (u[1] if (bank, t) in FLIPV else 1 - u[1]), 255, 255, 255, 255, nn[0], nn[1], nn[2], 0))
                for a, b, c in M2.strip_tris(p["n"]):
                    if len({p["pos"][a], p["pos"][b], p["pos"][c]}) == 3: I_.append((base + a, base + b, base + c))
        for (t, bl), (V_, I_) in per.items():
            if not I_: continue
            g = -1
            if t >= 0 and t not in B.images: notex[(bank, t)] = notex.get((bank, t), 0) + 1
            if t in B.images:
                key = (bank, t)
                if key not in texid: texid[key] = len(texid)
                g = texid[key]
            meshes.append(struct.pack("<iiIII", len(info) | (0x1000000 if follow else 0), g, len(V_), len(I_) * 3, bl) + b"".join(V_) + b"".join(struct.pack("<3I", *tri) for tri in I_))
        info.append(dict(index=len(info), id=f"{bank << 16 | blk:#x}", bank=bank, block=blk, entry=ei))
    for (bank, t), g in texid.items(): banks[bank].images[t].save(out / f"tex_{g:02d}.png")
    (out / "bank.vnm").write_bytes(b"VNM2" + struct.pack("<II", len(meshes), len(texid)) + b"".join(meshes))
    (out / "stage.json").write_text(json.dumps(dict(stage=stage, bg_states=states, y_up=True, scale=scale, sky=sky, objects=info,
        textures={str(g): dict(bank=k[0], tex_index=k[1], file_entry=banks[k[0]].tid) for k, g in texid.items()}), indent=1))
    print(f"fase {stage} bg {bg}: {len(info)} bloques colocados, {len(meshes)} mallas, {len(texid)} texturas -> {out}")
    if notex: print("  mallas con indice de textura sin imagen (sin textura en la salida):", sum(notex.values()), "en", len(notex), "(banco, indice):", sorted(notex)[:12])
    return out

# ---------------------------------------------------------------- render de camara (CPU)
def load_vnm(out):
    d = (Path(out) / "bank.vnm").read_bytes(); nm, nt = struct.unpack_from("<II", d, 4); p = 12; res = []; v2 = d[:4] == b"VNM2"
    for _ in range(nm):
        blk, tex, nv, ni = struct.unpack_from("<iiII", d, p); p += 16 + (4 if v2 else 0)
        v = np.frombuffer(d, np.dtype([("p", "<3f4"), ("uv", "<2f4"), ("c", "u1", 4), ("n", "i1", 4)]), nv, p); p += 28 * nv
        ix = np.frombuffer(d, "<u4", ni, p).reshape(-1, 3); p += 4 * ni; res.append((tex, v, ix))
    return res, nt

def cam_state(route, cam, frame):
    import afs_campath as C
    cp = C.load_paths(str(ROOT / "disc" / "FS" / "ST1.AFS"))[route]; c = cp.camera(cam); t0, t1 = cp.frames(cam); t = t0 if frame is None else frame
    pos = np.array([k(t) for k in c["pos"]]); tgt = np.array([k(t) for k in c["tgt"]]); fov = c["fov"](t)
    return pos, tgt, fov, (t0, t1)

def render(out, shot, route, cam, frame=None, size=(960, 540), scale=1.0):
    meshes, nt = load_vnm(out); pos, tgt, fov, rng = cam_state(route, cam, frame)
    texs = []                                   # cadena de mipmaps (filtro de caja) por textura
    for i in range(nt):
        m = [np.asarray(Image.open(Path(out) / f"tex_{i:02d}.png").convert("RGBA"))]
        while min(m[-1].shape[:2]) > 1:
            q = m[-1].astype(np.float32); h2, w2 = q.shape[0] // 2 * 2, q.shape[1] // 2 * 2
            m.append(((q[0:h2:2, 0:w2:2] + q[1:h2:2, 0:w2:2] + q[0:h2:2, 1:w2:2] + q[1:h2:2, 1:w2:2]) / 4).astype(np.uint8))
        texs.append(m)
    pos = pos * scale; tgt = tgt * scale
    W, H = size; fz = tgt - pos; fz /= np.linalg.norm(fz); up = np.array([0, 1.0, 0])
    rx = np.cross(fz, up); rx /= np.linalg.norm(rx); ry = np.cross(rx, fz)
    f = (H / 2) / np.tan(np.radians(min(max(fov, 10), 120)) / 2)      # fov del registro tomado como vertical (sin confirmar)
    img = np.zeros((H, W, 3), np.uint8); img[:] = (70, 90, 120); zb = np.full((H, W), 1e30)
    tris = []
    for tex, v, ix in meshes:
        P = v["p"].astype(np.float64) - pos; cam_ = np.stack([P @ rx, P @ ry, P @ fz], 1)
        tris.append((tex, cam_, v["uv"].astype(np.float64), v["n"][:, :3].astype(np.float64), ix))
    drawn = 0
    for tex, Pc, UV, N, ix in tris:
        T = texs[tex] if tex >= 0 else None
        for a, b, c in ix:
            z = Pc[[a, b, c], 2]
            if (z < 1.0).any(): continue
            sx = W / 2 + f * Pc[[a, b, c], 0] / z; sy = H / 2 - f * Pc[[a, b, c], 1] / z
            x0, x1 = int(max(0, np.floor(sx.min()))), int(min(W - 1, np.ceil(sx.max()))); y0, y1 = int(max(0, np.floor(sy.min()))), int(min(H - 1, np.ceil(sy.max())))
            if x1 < x0 or y1 < y0 or (x1 - x0) * (y1 - y0) > W * H * 0.6: continue
            den = (sy[1] - sy[2]) * (sx[0] - sx[2]) + (sx[2] - sx[1]) * (sy[0] - sy[2])
            if abs(den) < 1e-9: continue
            xs, ys = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
            l1 = ((sy[1] - sy[2]) * (xs - sx[2]) + (sx[2] - sx[1]) * (ys - sy[2])) / den; l2 = ((sy[2] - sy[0]) * (xs - sx[2]) + (sx[0] - sx[2]) * (ys - sy[2])) / den; l3 = 1 - l1 - l2
            ok = (l1 >= 0) & (l2 >= 0) & (l3 >= 0)
            if not ok.any(): continue
            w = np.stack([l1 / z[0], l2 / z[1], l3 / z[2]]); s = w.sum(0); zz = 1 / np.where(s == 0, 1e-12, s); ok &= zz < zb[y0:y1 + 1, x0:x1 + 1]
            if not ok.any(): continue
            wn = w * zz; n = (wn[0, ..., None] * N[a] + wn[1, ..., None] * N[b] + wn[2, ..., None] * N[c]); n /= np.linalg.norm(n, axis=2, keepdims=True) + 1e-9
            lam = 0.55 + 0.45 * np.clip(n @ np.array([0.4, 0.8, 0.5]) / 1.0, 0, 1)
            if T is not None:
                u = (wn[0] * UV[a, 0] + wn[1] * UV[b, 0] + wn[2] * UV[c, 0]); vv = (wn[0] * UV[a, 1] + wn[1] * UV[b, 1] + wn[2] * UV[c, 1])
                area_uv = abs((UV[b, 0] - UV[a, 0]) * (UV[c, 1] - UV[a, 1]) - (UV[c, 0] - UV[a, 0]) * (UV[b, 1] - UV[a, 1])) * T[0].shape[0] * T[0].shape[1]
                lvl = int(np.clip(0.5 * np.log2(max(area_uv, 1e-6) / max(abs(den) / 2, 1e-6)), 0, len(T) - 1)); M = T[lvl]
                th, tw = M.shape[:2]; col = M[(np.floor(vv * th).astype(int)) % th, (np.floor(u * tw).astype(int)) % tw]
                ok &= col[..., 3] > 5; rgb = col[..., :3] * lam[..., None]
            else: rgb = np.full(ok.shape + (3,), 150.0) * lam[..., None]
            sub = img[y0:y1 + 1, x0:x1 + 1]; sub[ok] = np.clip(rgb[ok], 0, 255).astype(np.uint8); zb[y0:y1 + 1, x0:x1 + 1][ok] = zz[ok]; drawn += 1
    Image.fromarray(img).save(shot)
    print(f"captura {shot}: ruta {route} camara {cam} cuadro {rng[0] if frame is None else frame} pos {np.round(pos / scale, 1)} -> {np.round(tgt / scale, 1)} fov {fov:.1f}, {drawn} triangulos dibujados")

if __name__ == "__main__":
    a = sys.argv[1:]
    if not a: sys.exit(__doc__)
    opt = lambda n, d=None: a[a.index(n) + 1] if n in a else d
    if opt("--extra"): EXTRA[:] = parse_extra(opt("--extra"), int(opt("--extra-flags", 0)))
    ONLY = "--only" in a
    if "--raw" not in a and opt("--stage", "0") == "0" and opt("--bg") == "5":      # ST1 estado 5: lo que el juego no deja ver (niebla/bosque opacos por falta de mezcla) y texturas con V invertida
        EXCL.update((b, x) for b, lo, hi in ST1_S5_EXCLUDE for x in range(lo, hi + 1)); FLIPV.update(ST1_S5_FLIPV)
    FLIPV.update(tuple(int(x) for x in t.split(":")) for t in (opt("--flipv") or "").split(",") if t)     # BANCO:TEXTURA cuya V se invierte (176:20,179:0 = bosque de abetos)
    for t in (opt("--exclude") or "").split(","):
        if t:
            bk, rng = t.split(":"); lo, _, hi = rng.partition("-"); EXCL.update((int(bk), x) for x in range(int(lo), int(hi or lo) + 1))
    out = a[0]; scale = float(opt("--scale", 1.0))
    export(str(ROOT / "disc" / "FS" / "ST1.AFS"), out, int(opt("--stage", 0)), opt("--bg", "all"), "--sky" in a, scale, tuple(int(x) for x in opt("--skip-banks", "").split(",") if x))
    if "--shot" in a:
        w, h = map(int, opt("--size", "960x540").split("x"))
        render(out, opt("--shot"), int(opt("--route", 1)), int(opt("--cam", 0)), float(opt("--frame")) if opt("--frame") else None, (w, h), scale)

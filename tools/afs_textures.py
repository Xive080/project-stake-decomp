#!/usr/bin/env python3
"""Convierte los ficheros de textura (cadena DMA con subidas GS) de un .AFS a PNG.
Uso: afs_textures.py FILE.AFS INDEX [OUTDIR]   |   afs_textures.py FILE.AFS all [OUTDIR]
Sin TEX0 no se sabe que paleta usa cada imagen indexada: se usa la ultima paleta CT32 anterior con el numero de
entradas adecuado (256 para T8, 16 para T4) y, si no hay, escala de grises."""
import struct, sys
from pathlib import Path
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parent))
import gs_stream as G
import afs

def to_rgba(data, w, h, psm):
    if psm == 0: return Image.frombytes("RGBA", (w, h), bytes(c for i in range(0, w * h * 4, 4) for c in (data[i], data[i + 1], data[i + 2], min(255, data[i + 3] * 2))))
    if psm == 2:
        px = bytearray()
        for i in range(0, w * h * 2, 2):
            v = data[i] | data[i + 1] << 8; px += bytes(((v & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, ((v >> 10) & 31) * 255 // 31, 255 if v & 0x8000 else 0))
        return Image.frombytes("RGBA", (w, h), bytes(px))
    if psm == 1: return Image.frombytes("RGB", (w, h), bytes(c for i in range(0, w * h * 4, 4) for c in data[i:i + 3]))
    return None

def clut(data, n):
    ent = [tuple(data[i * 4: i * 4 + 3]) + (min(255, data[i * 4 + 3] * 2),) for i in range(n)]
    if n == 256: ent = [ent[(i & 0xe7) | ((i & 8) << 1) | ((i & 0x10) >> 1)] for i in range(256)]   # CSM1: se intercambian bloques 8-15 y 16-23
    return ent

def indexed(data, w, h, psm, pal):
    if psm == 20: idx = bytes(v for b in data[: w * h // 2] for v in (b & 15, b >> 4))
    else: idx = data[: w * h]
    im = Image.frombytes("P", (w, h), idx)
    if pal:
        flat = []; [flat.extend(c[:3]) for c in pal]; im.putpalette(flat + [0] * (768 - len(flat)))
    else: im.putpalette([v for i in range(256) for v in (min(255, i * 17) if psm == 20 else i,) * 3])
    return im

def pal16(data, psm):
    """Paleta de 16 entradas desde una subida CT32 o CT16 de 8x2."""
    if psm == 0: return [tuple(data[i * 4: i * 4 + 3]) + (min(255, data[i * 4 + 3] * 2),) for i in range(16)]
    out = []
    for i in range(16):
        v = data[i * 2] | data[i * 2 + 1] << 8
        out.append(((v & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, ((v >> 10) & 31) * 255 // 31, 255 if v & 0x8000 else 0))
    return out

def roughness(idx, w, h, pal):
    """Variacion media de color entre pixeles vecinos: la paleta correcta da la imagen mas suave."""
    step = max(1, h // 32); tot = 0; n = 0
    for y in range(0, h - 1, step):
        r0 = idx[y * w:(y + 1) * w]; r1 = idx[(y + 1) * w:(y + 2) * w]
        for x in range(w - 1):
            a = pal[r0[x]]
            for c in (pal[r0[x + 1]], pal[r1[x]]): tot += abs(a[0] - c[0]) + abs(a[1] - c[1]) + abs(a[2] - c[2]); n += 1
    return tot / max(1, n)

def dump(buf, out):
    ups, used, dstart = G.parse_chain(buf); out.mkdir(parents=True, exist_ok=True); n = 0
    pals = {256: [], 16: []}
    for k, u in enumerate(ups):
        if "w" not in u: continue
        w, h, psm, data = u["w"], u["h"], u["dpsm"], u["data"]
        if psm == 0 and w * h == 256: pals[256].append((k, clut(data, 256)))
        if psm in (0, 2) and w * h == 16: pals[16].append((k, pal16(data, psm)))
    pairs = {}
    for k, u in enumerate(ups):
        if "w" not in u: continue
        w, h, psm, data = u["w"], u["h"], u["dpsm"], u["data"]
        if psm in (0, 1, 2): im = to_rgba(data, w, h, psm); tag = ""
        elif psm in (19, 20):
            idx = bytes(v for b in data[: w * h // 2] for v in (b & 15, b >> 4)) if psm == 20 else data[: w * h]
            cands = pals[256 if psm == 19 else 16]
            best = min(((roughness(idx, w, h, p), kp) for kp, p in cands), default=None)
            pal = dict(cands)[best[1]] if best else None
            im = indexed(data, w, h, psm, pal); tag = f"_pal{best[1]:03d}" if best else "_gris"
            if best: pairs[k] = best[1]
        else: continue
        if im is None: continue
        im.save(out / f"{k:03d}_{G.PSM.get(psm, psm)}_{w}x{h}{tag}.png"); n += 1
    return n, len(ups), pairs

def main():
    path, which = sys.argv[1], sys.argv[2]; out = Path(sys.argv[3]) if len(sys.argv) > 3 else Path("build/tex") / Path(path).stem
    f, ents, names, _ = afs.parse(path)
    for i, (o, s) in enumerate(ents):
        if not s or (which != "all" and i != int(which)): continue
        f.seek(o); b = f.read(s)
        try:
            if struct.unpack_from("<I", b, 0)[0] < s and struct.unpack_from("<Q", b, 0x10)[0] >> 28 & 7 == 1:
                n, t, pr = dump(b, out / f"{i:04d}"); print(f"entrada {i}: {n}/{t} imagenes, {len(pr)} indexadas emparejadas")
        except Exception as e: print(f"entrada {i}: no es cadena de texturas ({type(e).__name__})")
if __name__ == "__main__": main()

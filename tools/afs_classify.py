#!/usr/bin/env python3
"""Clasifica las entradas de los .AFS: tex (cadena DMA GS), lz (contenedor LZSS de bloques 0x1000), adx (audio ADX),
otbl (tabla de offsets crecientes), vacia, otro. Uso: afs_classify.py [FILE.AFS ...] (por defecto todos los de disc/FS)."""
import struct, sys, collections
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import afs
from afs_unpack import parse_container
from afs_lz import lz_decode

def classify(b):
    if not b: return "vacia", None
    u = struct.unpack_from("<I", b, 0)[0]
    if b[0] == 0x80 and b[1] == 0 and len(b) > 0x20 and b[4] == 3 and b[6] == 4: return "adx", f"{b[7]}ch {struct.unpack_from('>I', b, 8)[0]}Hz"
    if len(b) > 0x18 and u < len(b) and (struct.unpack_from("<Q", b, 0x10)[0] >> 28 & 7) == 1: return "tex", None
    bl = parse_container(b)
    if bl is not None: return "lz", f"{len(bl)} bloques, {sum(map(len, bl))} B"
    if len(b) > 12:
        a, bb, ex = struct.unpack_from("<III", b, 8 - 8 + 0)[0], struct.unpack_from("<I", b, 4)[0], struct.unpack_from("<I", b, 8)[0]
        if bb == len(b) and ex == 0: return "raw", f"n={a}"
        if 0 < ex < 1 << 26:
            try:
                out, _ = lz_decode(b, 12)
                if 0 <= ex - len(out) < 0x20: return "lz1", f"{a},{bb} exp {ex}"
            except IndexError: pass
    if u % 4 == 0 and 8 <= u <= min(len(b), 1 << 16):   # tabla de offsets absolutos sin cuenta: el primero es el tamano de la tabla
        t = struct.unpack_from("<%dI" % (u // 4), b, 0)
        if all(t[i] <= t[i + 1] <= len(b) for i in range(len(t) - 1)) and t[-1] <= len(b): return "otbl", f"{len(t)} subficheros"
    return "otro", b[:8].hex()

if __name__ == "__main__":
    files = sys.argv[1:] or sorted(str(p) for p in Path("disc/FS").glob("*.AFS"))
    tot = collections.Counter(); size = collections.Counter()
    for p in files:
        f, ents, _, _ = afs.parse(p); c = collections.Counter()
        for i, (o, s) in enumerate(ents):
            if not s: continue
            f.seek(o); k, d = classify(f.read(s)); c[k] += 1; size[k] += s
            if len(sys.argv) > 1: print(f"{Path(p).stem} {i:4d} {s:9d} {k} {d or ''}")
        tot.update(c); print(Path(p).stem, dict(c), file=sys.stderr)
    print("TOTAL", dict(tot), {k: f"{v/1e6:.1f} MB" for k, v in size.items()}, file=sys.stderr)

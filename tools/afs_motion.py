#!/usr/bin/env python3
"""Movimientos (bancos 'mot' = otbl de subficheros). Cada movimiento = N cuadros de stride (6n+15)&~3 bytes:
12 B de traslacion de la raiz (3 floats) + n articulaciones x 3 s16 (angulos, 65536 = 360 grados) + relleno; el resto del subfichero es cola.
n depende del tipo de cuerpo (tabla s16 en DATA00.BIN +0xF860, indexada por tipo). Uso: afs_motion.py AFS ENTRADA SUBFICHERO [N]"""
import struct, sys
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parent))
import afs

def subfiles(b):
    t = struct.unpack_from("<%dI" % (struct.unpack_from("<I", b, 0)[0] // 4), b, 0); return [b[t[i]:t[i + 1]] for i in range(len(t) - 1)]

def stride(n): return (6 * n + 15) & ~3

def frames(s, n):
    """(traslacion nf x 3 float, angulos nf x n x 3 en radianes)"""
    st = stride(n); nf = len(s) // st; a = np.frombuffer(s[:nf * st], np.uint8).reshape(nf, st)
    return a[:, :12].copy().view("<f4").reshape(nf, 3), a[:, 12:12 + 6 * n].copy().view("<i2").reshape(nf, n, 3) * (2 * np.pi / 65536)

def guess_n(s):
    """n cuyo cuadro da angulos mas suaves entre cuadros consecutivos (None si ninguno encaja)."""
    best = None
    for n in range(3, 32):
        st = stride(n); nf = len(s) // st
        if nf < 4: continue
        a = np.frombuffer(s[:nf * st], np.uint8).reshape(nf, st)[:, 12:12 + 6 * n].copy().view("<i2").astype(int)
        d = np.abs(((np.diff(a, axis=0) + 32768) % 65536) - 32768).mean()
        if best is None or d < best[0]: best = (d, n)
    return best[1] if best and best[0] < 500 else None

if __name__ == "__main__":
    f, ents, _, _ = afs.parse(sys.argv[1]); f.seek(ents[int(sys.argv[2])][0]); s = subfiles(f.read(ents[int(sys.argv[2])][1]))[int(sys.argv[3])]
    n = int(sys.argv[4]) if len(sys.argv) > 4 else guess_n(s); print("subfichero", len(s), "B, n =", n)
    if n:
        t, a = frames(s, n); print(len(t), "cuadros; raiz[0] =", t[0], "; ang[0] (grados) =", np.degrees(a[0]).round(0).tolist())

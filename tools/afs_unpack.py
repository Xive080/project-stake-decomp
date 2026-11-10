#!/usr/bin/env python3
"""Contenedores comprimidos de los .AFS: u32 tamano total expandido, u32 n, n pares (offset, tamano expandido) y un
par final (fin de datos, 0xffffffff); cada bloque va comprimido con afs_lz. `parse_container(b)` devuelve la lista de
bloques descomprimidos o None si la entrada no tiene esa forma."""
import struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from afs_lz import lz_decode

def parse_container(b):
    if len(b) < 16: return None
    u0, n = struct.unpack_from("<II", b, 0)
    if not 0 < n < 4096 or 8 + 8 * n > len(b): return None
    pairs = struct.unpack_from("<%dI" % (2 * n), b, 8)
    offs = list(pairs[0::2]); exps = pairs[1::2]
    ends = offs[1:] + [len(b)]
    if ends[-1] and n and offs[-1] > len(b): return None
    # variante con par final (fin de datos, 0xffffffff): el ultimo "par" real se lee como terminador
    blocks = []
    for i in range(n):
        if exps[i] == 0xffffffff: break
        if not offs[i] < len(b) or ends[i] < offs[i]: return None
        try: out, _ = lz_decode(b, offs[i], ends[i])
        except IndexError: return None
        if not 0 <= exps[i] - len(out) < 0x20: return None   # tamano declarado redondeado a 32 bytes
        out = out + bytes(exps[i] - len(out))
        blocks.append(out)
    return blocks or None

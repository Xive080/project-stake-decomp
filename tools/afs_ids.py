#!/usr/bin/env python3
"""Mapa semantico de IDs de entrada de los .AFS, leido de las tablas s16 del ELF (baserom/SCES_504.11):
las pasan a ADXF_OpenAfs los gestores del juego. Los IDs son indices de slot y se reutilizan entre ST1..ST6.
  0x26C490 obj (PolyMgr, 279 bancos) -> contenedores lz   | 0x26C6B0 tex del banco de objeto
  0x26CA60 mot (MotMgr) -> otbl | 0x26CB20 campath | 0x26CB50 osk (OneSkin) | 0x26E880 pares (cuerpo, 168) | 0x26F6C0 body2
Sin etiqueta: 547..737 texturas pedidas directamente por el codigo (sprites), 738..769 audio ADX.
Uso: afs_ids.py [ID ...]  (sin argumentos, resumen)"""
import struct, sys
from pathlib import Path
ELF = Path(__file__).resolve().parent.parent / "baserom" / "SCES_504.11"
TABLES = [("obj", 0x26C490, 280, 1), ("obj_tex", 0x26C6B0, 280, 1), ("mot", 0x26CA60, 96, 1), ("campath", 0x26CB20, 24, 1),
          ("osk", 0x26CB50, 100, 0), ("body_a", 0x26E880, 200, 0), ("body2", 0x26F6C0, 100, 1)]

def labels():
    d = ELF.read_bytes(); lab = {}
    for name, addr, n, first in TABLES:
        off = addr - 0x140000 + 0x800; t = struct.unpack_from("<%dh" % n, d, off)
        for k, v in enumerate(t):
            if k < first or v == 0: continue
            if name == "body_a": name2 = "body" if k % 2 == 0 else "body_b"
            else: name2 = name
            lab.setdefault(v, {})[name2] = k
    for v in range(738, 770): lab.setdefault(v, {})["adx"] = v
    for v in range(547, 738): lab.setdefault(v, {}).setdefault("sprite_tex", v)
    return lab

if __name__ == "__main__":
    lab = labels()
    if len(sys.argv) > 1:
        for a in sys.argv[1:]: print(a, lab.get(int(a), "?"))
    else:
        import collections; print(collections.Counter(k for v in lab.values() for k in v))

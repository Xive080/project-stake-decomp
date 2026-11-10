#!/usr/bin/env python3
"""Extrae ficheros de un estado guardado de PCSX2 (.p2s = zip con metodos stored(0) y zstd(93)). Uso: p2s_extract.py FILE.p2s OUTDIR [NOMBRE ...]"""
import struct, sys, zstandard
from pathlib import Path
d = Path(sys.argv[1]).read_bytes(); out = Path(sys.argv[2]); out.mkdir(parents=True, exist_ok=True); want = set(sys.argv[3:])
# directorio central (los tamanos de la cabecera local pueden ir a 0 con data descriptor)
eocd = d.rfind(b"PK\x05\x06"); n, cdsz, cdoff = struct.unpack_from("<HII", d, eocd + 10); p = cdoff
for _ in range(n):
    (_, ver, vn, fl, meth, _t, _dt, crc, csz, usz, nl, el, cl, _d, _ia, _ea, lho) = struct.unpack_from("<IHHHHHHIIIHHHHHII", d, p)
    name = d[p + 46:p + 46 + nl].decode(); p += 46 + nl + el + cl
    if want and name not in want: continue
    lnl, lel = struct.unpack_from("<HH", d, lho + 26); s = lho + 30 + lnl + lel; raw = d[s:s + csz]
    data = raw if meth == 0 else zstandard.ZstdDecompressor().decompress(raw, max_output_size=usz) if meth == 93 else None
    if data is None: print("metodo", meth, "no soportado:", name); continue
    (out / name).write_bytes(data); print(name, len(data))

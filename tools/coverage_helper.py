"""Utilidades comunes: lista de funciones del modulo `main` con su tamano (symtab del ELF)."""
import struct
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
def main_funcs():
    d = (ROOT / "baserom/SCES_504.11").read_bytes()
    shoff = struct.unpack_from("<I", d, 32)[0]; se, sn, ss = struct.unpack_from("<HHH", d, 46)
    sh = [struct.unpack_from("<10I", d, shoff + i * se) for i in range(sn)]
    so = sh[ss][4]; names = [d[so + s[0]: d.index(b"\0", so + s[0])].decode() for s in sh]
    sym = next(s for s in sh if s[1] == 2); strs = sh[sym[6]]; out = []
    for k in range(sym[5] // 16):
        n, val, sz, info, oth, shndx = struct.unpack_from("<IIIBBH", d, sym[4] + k * 16)
        if info & 0xf == 2 and names[shndx] == "main":
            out.append((val, sz, d[strs[4] + n: d.index(b"\0", strs[4] + n)].decode()))
    return sorted(out)

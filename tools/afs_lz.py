#!/usr/bin/env python3
"""Descompresor LZSS de los bloques de los .AFS (reproduce func_001E5490 del juego, que lo hace byte a byte).
Bits de control LSB primero: 1 = literal; 0,1 = copia larga (u16: bits15..3 = offset-0x2000, bits2..0 = len; 0 = fin;
len 0 -> un tercer byte n = longitud-1 en total, si no total = len+2); 0,0 = copia corta (2 bits = total-2, 1 byte = offset-0x100)."""
def lz_decode(src, pos=0, end=None):
    end = len(src) if end is None else end; out = bytearray(); ctl = 0; nb = 0
    def bit():
        nonlocal ctl, nb, pos
        if nb == 0: ctl = src[pos]; pos += 1; nb = 8
        b = ctl & 1; ctl >>= 1; nb -= 1; return b
    while pos < end:
        if bit(): out.append(src[pos]); pos += 1; continue
        if bit():
            v = src[pos] | src[pos + 1] << 8; pos += 2
            if v == 0: break
            off = (v >> 3) - 0x2000; lo = v & 7
            if lo: n = lo + 2
            else: n = src[pos] + 1; pos += 1
        else:
            n = (bit() << 1 | bit()) + 2; off = src[pos] - 0x100; pos += 1
        for _ in range(n): out.append(out[off + len(out)])
    return bytes(out), pos

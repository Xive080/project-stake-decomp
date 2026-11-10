#!/usr/bin/env python3
"""Parser de flujos GIF/GS de PS2 (paquetes de subida de texturas) tal como aparecen en los .AFS de Vampire Night.
`parse(buf, start)` devuelve la lista de subidas: dict(dbp, dbw, dpsm, w, h, data, tex0...) y de registros A+D vistos."""
import struct
PSM = {0: "CT32", 1: "CT24", 2: "CT16", 10: "CT16S", 19: "T8", 20: "T4", 27: "T8H", 36: "T4HL", 44: "T4HH", 48: "Z32", 49: "Z24", 50: "Z16", 58: "Z16S"}

def parse(b, start=0, maxq=1 << 20):
    pos = start; ups = []; regs = {}; cur = None; n = 0
    while pos + 16 <= len(b) and n < maxq:
        lo, hi = struct.unpack_from("<QQ", b, pos); n += 1
        nloop = lo & 0x7fff; eop = (lo >> 15) & 1; flg = (lo >> 58) & 3; nreg = (lo >> 60) & 15 or 16
        if hi == 0 and (lo >> 32) == 0 and (lo >> 28) in (0x1, 0x7, 0x0) and lo != 0: pos += 16; continue   # etiqueta DMA (cnt/end)
        if nloop == 0 and lo == 0 and hi == 0: pos += 16; continue
        if (lo >> 28) & 7 in (1, 2, 3, 5, 6, 7) and (lo >> 32) == 0 and flg == 0 and nreg != 16 and False: pass
        if flg == 2:
            data = b[pos + 16: pos + 16 + nloop * 16]
            if cur is not None: cur["data"] = data; ups.append(cur); cur = None
            else: ups.append({"data": data, "orphan": True})
            pos += 16 + nloop * 16; continue
        if flg == 0 and nreg == 1 and (hi & 0xf) == 0xe:
            for q in range(nloop):
                v, a = struct.unpack_from("<QQ", b, pos + 16 + q * 16); a &= 0xff; regs[a] = v
                if a == 0x52: cur = dict(w=v & 0xfff, h=(v >> 32) & 0xfff, dbp=(regs.get(0x50, 0) >> 32) & 0x3fff,
                                         dbw=(regs.get(0x50, 0) >> 48) & 63, dpsm=(regs.get(0x50, 0) >> 56) & 63, off=pos)
            pos += 16 + nloop * 16; continue
        if flg == 0:
            pos += 16 + nloop * nreg * 16; continue
        pos += 16 + nloop * 16
        if eop and False: break
    return ups, regs

if __name__ == "__main__":
    import sys
    f, off = sys.argv[1], int(sys.argv[2], 0); start = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0
    ups, regs = parse(open(f, "rb").read()[off:], start)
    for u in ups: print({k: (PSM.get(v, v) if k == "dpsm" else (len(v) if k == "data" else v)) for k, v in u.items()})

def parse_chain(b):
    """Formato de los ficheros de textura de los AFS: u32 inicio de datos, cadena DMA desde 0x10:
    `cnt`(id 1) con un paquete GIF A+D (BITBLTBUF/TRXPOS/TRXREG/TRXDIR + etiqueta IMAGE) seguido de `ref`(id 3) cuyos
    datos salen consecutivos de la region de datos. Devuelve lista de subidas con 'data'."""
    dstart = struct.unpack_from("<I", b, 0)[0]; pos = 0x10; dpos = dstart; ups = []; regs = {}; cur = None
    while pos + 16 <= dstart:
        lo, hi = struct.unpack_from("<QQ", b, pos); idt = (lo >> 28) & 7; qwc = lo & 0xffff; pos += 16
        if idt == 1:
            q = 0
            while q < qwc:
                glo, ghi = struct.unpack_from("<QQ", b, pos + q * 16); nloop = glo & 0x7fff; flg = (glo >> 58) & 3; nreg = (glo >> 60) & 15 or 16
                if flg == 0 and nreg == 1 and (ghi & 0xf) == 0xe:
                    for k in range(nloop):
                        v, a = struct.unpack_from("<QQ", b, pos + (q + 1 + k) * 16); a &= 0xff; regs[a] = v
                        if a == 0x52:
                            bb = regs.get(0x50, 0)
                            cur = dict(w=v & 0xfff, h=(v >> 32) & 0xfff, dbp=(bb >> 32) & 0x3fff, dbw=(bb >> 48) & 63, dpsm=(bb >> 56) & 63)
                    q += 1 + nloop
                elif flg == 2: q += 1 + nloop  # IMAGE: sus datos vienen por `ref`
                else: q += 1 + nloop * nreg
            pos += qwc * 16
        elif idt == 3:
            if cur is None: ups.append({"orphan": True, "data": b[dpos: dpos + qwc * 16]})
            else: cur["data"] = b[dpos: dpos + qwc * 16]; ups.append(cur); cur = None
            dpos += qwc * 16
        elif idt == 7: break
        else: pos += qwc * 16
    return ups, dpos - dstart, dstart

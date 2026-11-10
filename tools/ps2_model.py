#!/usr/bin/env python3
"""Bloques de objeto (`0x1000`) de los AFS: lista de display de PS2.
Cabecera: u32 0x1000, u32 tamano, u32 cuenta, ..., u32 ntags en 0x2c; desde 0x30 `ntags` tags DMA de 16 B
(w0 = 0x3000_0000 | qwc  => `ref`, w1 = direccion RELATIVA al bloque, w2/w3 = codigos VIF).
Cada tag apunta a un flujo VIF (STROW, UNPACK a VU1, MSCAL, DIRECT con paquetes GIF...)."""
import struct
VIF = {0x00: "NOP", 0x01: "STCYCL", 0x02: "OFFSET", 0x03: "BASE", 0x04: "ITOP", 0x05: "STMOD", 0x06: "MSKPATH3", 0x07: "MARK",
       0x10: "FLUSHE", 0x11: "FLUSH", 0x13: "FLUSHA", 0x14: "MSCAL", 0x15: "MSCALF", 0x17: "MSCNT", 0x20: "STMASK",
       0x30: "STROW", 0x31: "STCOL", 0x4a: "MPG", 0x50: "DIRECT", 0x51: "DIRECTHL"}
UNPACK_FMT = {0: "S-32", 1: "S-16", 2: "S-8", 4: "V2-32", 5: "V2-16", 6: "V2-8", 8: "V3-32", 9: "V3-16", 10: "V3-8",
              12: "V4-32", 13: "V4-16", 14: "V4-8", 15: "V4-5"}
UNPACK_SZ = {0: 4, 1: 2, 2: 1, 4: 8, 5: 4, 6: 2, 8: 12, 9: 6, 10: 3, 12: 16, 13: 8, 14: 4, 15: 2}

def parse_vif(buf, start, n):
    """Interpreta n quadwords de flujo VIF; devuelve [(cmd, num, imm, datos_offset)]."""
    out = []; p = start; end = start + n * 16
    while p + 4 <= end:
        w = struct.unpack_from("<I", buf, p)[0]; cmd = (w >> 24) & 0x7f; num = (w >> 16) & 0xff; imm = w & 0xffff; p += 4
        if cmd >= 0x60:
            fmt = cmd & 15; sz = UNPACK_SZ.get(fmt, 4); cyc = 1
            nbytes = (num or 256) * sz; nbytes = (nbytes + 3) & ~3
            out.append((f"UNPACK {UNPACK_FMT.get(fmt, fmt)}", num or 256, imm & 0x3ff, p)); p += nbytes
        elif cmd == 0x30: out.append(("STROW", 0, 0, p)); p += 16
        elif cmd == 0x31: out.append(("STCOL", 0, 0, p)); p += 16
        elif cmd == 0x20: out.append(("STMASK", 0, 0, p)); p += 4
        elif cmd == 0x4a: out.append(("MPG", num or 256, imm, p)); p += (num or 256) * 8
        elif cmd in (0x50, 0x51): out.append(("DIRECT", imm, 0, p)); p += imm * 16
        else: out.append((VIF.get(cmd, f"?{cmd:#x}"), num, imm, p))
    return out

def parse_block(x):
    h, size, cnt = struct.unpack_from("<III", x, 0); nt = struct.unpack_from("<I", x, 0x2c)[0]; tags = []
    for t in range(nt):
        w0, w1, v0, v1 = struct.unpack_from("<4I", x, 0x30 + 16 * t); tags.append(dict(qwc=w0 & 0xffff, addr=w1, vif=(v0, v1)))
    return dict(count=cnt, tags=tags)

if __name__ == "__main__":
    import sys; sys.path.insert(0, __file__.rsplit("/", 1)[0])
    import afs; from afs_unpack import parse_container
    f, ents, _, _ = afs.parse(sys.argv[1]); o, s = ents[int(sys.argv[2])]; f.seek(o); bl = parse_container(f.read(s)); x = bl[int(sys.argv[3])]
    m = parse_block(x); print("bloque", hex(len(x)), "cuenta", m["count"], "tags", len(m["tags"]))
    for t in m["tags"]:
        print(f"  tag qwc={t['qwc']:<4} addr={t['addr']:#06x} vif={t['vif'][0]:08x},{t['vif'][1]:08x}")
        if t["qwc"] and t["addr"] + t["qwc"] * 16 <= len(x):
            for c in parse_vif(x, t["addr"], t["qwc"]): print("     ", c[0], c[1], hex(c[2]))

def unpack_data(buf, p, fmt, num, cl, wl, usn, row=(0, 0, 0, 0)):
    """Convierte los datos de un UNPACK en quads (listas de 4 numeros) segun el formato VIF; aplica STCYCL (wl<cl = omision)."""
    nf = {0: 1, 4: 2, 8: 3, 12: 4, 15: 4}[fmt & 12 if fmt != 15 else 12] if fmt != 15 else 4
    comp = (fmt >> 2) + 1; width = fmt & 3
    sz = {0: 4, 1: 2, 2: 1, 3: 2}[width]; quads = []
    for i in range(num):
        v = []
        for c in range(comp):
            o = p + (i * comp + c) * sz
            if fmt == 15: break
            if width == 0: v.append(struct.unpack_from("<f", buf, o)[0])
            elif width == 1: v.append(struct.unpack_from("<H" if usn else "<h", buf, o)[0])
            elif width == 2: v.append(buf[o] if usn else struct.unpack_from("<b", buf, o)[0])
        while len(v) < 4: v.append(0 if len(v) < 3 or comp == 4 else 1)
        quads.append(v[:4])
    return quads

def vu_memory(x, tags):
    """Simula los UNPACK de los tags y devuelve lista de lotes (memoria VU1 como dict addr->quad) cortados en cada MSCAL."""
    mem = {}; cl = wl = 1; batches = []; usn = 0
    for t in tags:
        if not t["qwc"] or t["addr"] == 0: continue
        p = t["addr"]; end = p + t["qwc"] * 16
        while p + 4 <= end:
            w = struct.unpack_from("<I", x, p)[0]; cmd = (w >> 24) & 0x7f; num = (w >> 16) & 0xff; imm = w & 0xffff; p += 4
            if cmd >= 0x60:
                fmt = cmd & 15; n = num or 256; usn = (imm >> 14) & 1; addr = imm & 0x3ff
                q = unpack_data(x, p, fmt, n, cl, wl, usn); sz = UNPACK_SZ.get(fmt, 4) * n; p += (sz + 3) & ~3
                for i, v in enumerate(q):
                    a = addr + (i if cl == wl or wl >= cl else (i // wl) * cl + i % wl); mem[a] = v
            elif cmd == 1: cl, wl = imm & 0xff, imm >> 8
            elif cmd == 0x30: p += 16
            elif cmd == 0x31: p += 16
            elif cmd == 0x20: p += 4
            elif cmd in (0x14, 0x15, 0x17): batches.append(dict(mem)); 
            elif cmd in (0x50, 0x51): p += imm * 16
    return batches

def materials(x):
    """Registros de material (cabecera 0xE0 + datos): lista de dict(off, tex, size, mode). Empiezan 0x30 B tras la lista de tags;
    el campo +0x10 es el indice de textura (nlObjPutUpdateTexInfoPoly2) y +0xD8 el tamano de los datos que siguen."""
    nt = struct.unpack_from("<I", x, 0x2c)[0]; p = 0x30 + 16 * nt + 0x30; out = []
    while p + 0xE0 <= len(x) and struct.unpack_from("<I", x, p)[0] != 0:
        sz = struct.unpack_from("<I", x, p + 0xD8)[0]
        out.append(dict(off=p, tex=struct.unpack_from("<i", x, p + 0x10)[0], mode=struct.unpack_from("<i", x, p + 0xC)[0], size=sz)); p += 0xE0 + sz
    return out

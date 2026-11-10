#!/usr/bin/env python3
"""Lector/extractor de ficheros AFS (CRI): cabecera 'AFS\\0', n, n x (offset,size) y tabla opcional de nombres.
Uso: afs.py list FILE.AFS | afs.py extract FILE.AFS OUTDIR | afs.py types FILE.AFS"""
import struct, sys
from collections import Counter
from pathlib import Path

def parse(path):
    d = open(path, "rb").read(0x100000) if False else None
    f = open(path, "rb"); h = f.read(8)
    if h[:4] != b"AFS\0": raise SystemExit(f"{path}: no es AFS")
    n = struct.unpack_from("<I", h, 4)[0]; toc = f.read(8 * n + 8)
    ents = [struct.unpack_from("<II", toc, 8 * i) for i in range(n)]
    nt_off, nt_size = struct.unpack_from("<II", toc, 8 * n)
    names = None
    if nt_off and nt_size and nt_size >= 48 * n:
        f.seek(nt_off); raw = f.read(48 * n)
        names = [raw[48 * i: 48 * i + 32].split(b"\0")[0].decode("latin1") for i in range(n)]
    return f, ents, names, (nt_off, nt_size)

def peek(f, off, size, k=16):
    f.seek(off); return f.read(min(k, size))

def kind(b):
    if b[:4] == b"AFS\0": return "AFS"
    if b[:4] in (b"RIFF",): return "RIFF"
    if b[:3] == b"\x00\x00\x01": return "MPEG"
    if all(32 <= c < 127 or c == 0 for c in b[:4]) and b[:4].strip(b"\0"): return "ascii:" + b[:4].decode("latin1").strip("\0")
    return "bin:" + b[:4].hex()

def main():
    cmd, path = sys.argv[1], sys.argv[2]
    f, ents, names, nt = parse(path)
    if cmd == "list":
        print(f"{path}: {len(ents)} entradas; tabla de nombres: {nt if names else 'no'}")
        for i, (o, s) in enumerate(ents): print(f"{i:5d} 0x{o:08X} {s:9d} {names[i] if names else ''} {kind(peek(f, o, s)) if s else '(vacia)'}")
    elif cmd == "types":
        c = Counter(); tot = Counter()
        for o, s in ents:
            if s: k = kind(peek(f, o, s)); c[k] += 1; tot[k] += s
            else: c["(vacia)"] += 1
        for k, v in c.most_common(): print(f"{k:20s} {v:5d} {tot[k]:12d} B")
    elif cmd == "extract":
        out = Path(sys.argv[3]); out.mkdir(parents=True, exist_ok=True)
        for i, (o, s) in enumerate(ents):
            if not s: continue
            f.seek(o); nm = names[i] if names and names[i] else f"{i:05d}.bin"
            (out / nm).write_bytes(f.read(s))
        print(f"extraidos en {out}")
if __name__ == "__main__": main()

#!/usr/bin/env python3
"""Compila un .cpp con cada ee-gcc y compara, funcion a funcion, con el ELF original.
Uso: probe_compilers.py FUENTE.cpp [flags de ee-gcc...]   (por defecto -O2 -G0)
Compiladores Windows (ee-gcc.exe) -> wibo; Linux i386 (bin/ee-gcc) -> docker/colima.
Todo relativo al repo, sin rutas absolutas."""
import re, struct, subprocess, sys, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
WIBO = ROOT / "tools/bin/wibo"
ELFP = ROOT / "baserom/SCES_504.11"
ELF = ELFP.read_bytes()
BASE, FOFF = 0x140000, 0x800

def norm(n):
    """Quita la 'F' tras el nombre de clase (el ELF la lleva; los ee-gcc probados no la emiten)."""
    m = re.match(r"^(.*?__)(\d+)(.*)$", n)
    if not m: return n
    k = int(m.group(2)); rest = m.group(3)
    tail = rest[k + 1:] if rest[k:k + 1] == "F" else rest[k:]
    return m.group(1) + m.group(2) + rest[:k] + ("" if tail == "v" else tail)

def nm_funcs(path):
    out = []
    for l in subprocess.check_output(["nm", "-n", str(path)], text=True).splitlines():
        p = l.split()
        if len(p) == 3 and p[1] in "Tt" and not p[2].startswith(("gcc2", "__gnu")): out.append((int(p[0], 16), p[2]))
    return out

def trim(b):
    while len(b) >= 4 and b[-4:] == b"\0\0\0\0": b = b[:-4]
    return b

def relocs_of(path):
    """offsets (en .text) -> mascara a poner a 0 (bits que rellena el enlazador)"""
    d = Path(path).read_bytes()
    shoff = struct.unpack_from("<I", d, 32)[0]; shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 46)
    sh = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
    so = sh[shstrndx][4]; out = {}
    for s in sh:
        if d[so + s[0]: d.index(b"\0", so + s[0])] == b".rel.text":
            for i in range(s[5] // 8):
                off, info = struct.unpack_from("<II", d, s[4] + i * 8)
                t = info & 0xff
                out[off] = 0xffffffff if t in (2, 3) else (0x03ffffff if t == 4 else 0x0000ffff)
    return out

def mask(b, base, rel):
    b = bytearray(b)
    for off, m in rel.items():
        if base <= off < base + len(b) - 3:
            w = struct.unpack_from("<I", b, off - base)[0] & ~m
            struct.pack_into("<I", b, off - base, w)
    return bytes(b)

def text_of(path):
    d = Path(path).read_bytes()
    shoff = struct.unpack_from("<I", d, 32)[0]; shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 46)
    sh = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
    so = sh[shstrndx][4]
    for s in sh:
        if d[so + s[0]: d.index(b"\0", so + s[0])] == b".text": return d[s[4]: s[4] + s[5]]


def funcs_of(path):
    """[(nombre, bytes)] de cada simbolo de funcion, en cualquier seccion .text* (CodeWarrior: una por funcion)."""
    d = Path(path).read_bytes()
    shoff = struct.unpack_from("<I", d, 32)[0]; shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 46)
    sh = [struct.unpack_from("<10I", d, shoff + i * shentsize) for i in range(shnum)]
    so = sh[shstrndx][4]
    nm = lambda s: d[so + s[0]: d.index(b"\0", so + s[0])].decode()
    names = [nm(s) for s in sh]
    rel = {}
    for i, s in enumerate(sh):
        if s[1] == 9 and names[i].startswith(".rel"):
            tgt = s[7]; m = rel.setdefault(tgt, {})
            for k in range(s[5] // 8):
                off, info = struct.unpack_from("<II", d, s[4] + k * 8); t = info & 0xff
                m[off] = 0xffffffff if t in (2, 3) else (0x03ffffff if t == 4 else 0x0000ffff)
    symtab = next(s for s in sh if s[1] == 2); strs = sh[symtab[6]]
    syms = []
    for k in range(symtab[5] // 16):
        n, val, size, info, other, shndx = struct.unpack_from("<IIIBBH", d, symtab[4] + k * 16)
        if 0 < shndx < len(sh) and names[shndx].startswith(".text") and (info & 0xf) in (0, 2):
            nme = d[strs[4] + n: d.index(b"\0", strs[4] + n)].decode()
            if nme and not nme.startswith(("gcc2", "__gnu", ".")): syms.append((shndx, val, size, nme))
    syms.sort()
    out = []
    for i, (sx, val, size, nme) in enumerate(syms):
        sec = sh[sx]
        nxt = next((v for x, v, _, _ in syms[i + 1:] if x == sx and v > val), sec[5])
        end = val + size if size else nxt
        rm = {o - val: m for o, m in rel.get(sx, {}).items() if val <= o < end}
        out.append((nme, mask(d[sec[4] + val: sec[4] + end], val, rel.get(sx, {})), rm))
    return out

ELF_F = {norm(n): a for a, n in nm_funcs(ELFP)}
ELF_ADDRS = sorted(set(a for a, _ in nm_funcs(ELFP)))

def elf_bytes(name, rel=None, base=0):
    a = ELF_F[name]; nxt = min(x for x in ELF_ADDRS if x > a)
    o = a - BASE + FOFF
    b = trim(ELF[o: o + (nxt - a)])
    return mask(b, base, rel) if rel else b

def compilers():
    import os
    only = os.environ.get("CC_ONLY")
    cs = [c for c in (ROOT / "tools/compilers").iterdir() if c.name.startswith(("ee-gcc", "mwcps2"))]
    seen = {c.name for c in cs}
    pc = ROOT / "tools/compilers/ps2_compilers"
    if pc.exists(): cs += [c for c in pc.iterdir() if c.name.startswith("ee-gcc") and c.name not in seen]
    cs = [c for c in cs if not only or only in c.name]
    return sorted(cs, key=lambda c: c.name)

def compile_obj(comp, src, flags, td):
    o = Path(td) / "o.o"
    exe = comp / "bin/ee-gcc.exe"
    mw = comp / "mwccps2.exe"
    if mw.exists():  # CodeWarrior: wibo, sin -x ni -G
        cmd = [str(WIBO), str(mw), "-lang", "c++", *[f for f in flags if not f.startswith(("-G", "-x"))], "-c", "-o", str(o), str(src)]
    elif exe.exists():
        cmd = [str(WIBO), str(exe), "-c", *flags, "-o", str(o), "-x", "c++", str(src)]
    elif (comp / "bin/ee-gcc").exists():
        cmd = ["docker", "run", "--rm", "--platform", "linux/386", "-v", f"{comp}:/cc:ro", "-v", f"{src.parent}:/src:ro",
               "-v", f"{td}:/out", "i386/debian:bullseye-slim", "/cc/bin/ee-gcc", "-c", *flags, "-o", "/out/o.o", "-x", "c++", f"/src/{src.name}"]
    else: return None, "sin ejecutable"
    r = subprocess.run(cmd, cwd=comp, capture_output=True, text=True)
    return (o if o.exists() else None), (r.stdout + r.stderr).strip()[:200]

VERBOSE = "-v" in sys.argv

def main():
    src = Path(sys.argv[1]).resolve(); flags = [a for a in sys.argv[2:] if a != "-v"] or ["-O2", "-G0"]
    for comp in compilers():
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as td:
            o, err = compile_obj(comp, src, flags, td)
            if not o: print(f"{comp.name:22} FALLO: {err}"); continue
            res = []; ok = 0; fl = funcs_of(o)
            for n, mine, rm in fl:
                mine = trim(mine); key = norm(n)
                if key not in ELF_F: res.append(f"{key.split('__')[0]}=?"); continue
                a = ELF_F[key]; nxt = min(x for x in ELF_ADDRS if x > a)
                eb = mask(trim(ELF[a - BASE + FOFF: a - BASE + FOFF + (nxt - a)]), 0, rm)
                same = mine == eb; ok += same
                if VERBOSE and not same: print(f"   {key}\n     elf : {eb.hex()}\n     mine: {mine.hex()}")
                mw_, ew_ = [mine[k:k + 4] for k in range(0, len(mine), 4)], [eb[k:k + 4] for k in range(0, len(eb), 4)]
                dist = sum(x != y for x, y in zip(mw_, ew_)) + abs(len(mw_) - len(ew_))
                res.append(f"{key.split('__')[0]}={'OK' if same else 'd%d' % dist}")
            print(f"{comp.name:22} {ok}/{len(fl)}  {' '.join(res)}")
if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Desensamblador minimo de microcodigo VU1 (PS2) para los programas del ELF (baserom/SCES_504.11, vaddr 0x1B1C88..0x1C5A20).
Cada instruccion son 8 B (LE): palabra baja = lower, palabra alta = upper. NOP inferior = 0x8000033C (MOVE vf0,vf0).
Los 5 programas (16 KB cada uno = 2048 instr, subidos en 8 trozos) salen de la tabla del ELF 0x1B1A40.
Uso: vu1_dis.py [--list] [--prog N] [--from VADDR --count N] [--raw]
No es un ensamblador de referencia: los opcodes de la unidad EFU (E*) se marcan como `efu?`."""
import struct, sys
from pathlib import Path

ELF = Path(__file__).resolve().parent.parent / "baserom" / "SCES_504.11"
RANGE = (0x1B1C80, 0x1C5C80)
LNOP = 0x8000033C
DST = "xyzw"

def off(v): return v - 0x140000 + 0x800

def dest(w):
    d = (w >> 21) & 15
    return "." + "".join(DST[i] for i in range(4) if d & (8 >> i)) if d != 15 else ""

def vf(n): return f"vf{n:02d}"
def vi(n): return f"vi{n:02d}"
BC = "xyzw"

UP = {0x04: "sub", 0x00: "add", 0x08: "madd", 0x0C: "msub", 0x10: "max", 0x14: "mini", 0x18: "mul"}
UP2 = {0x1C: ("mulq", "q"), 0x1D: ("maxi", "i"), 0x1E: ("muli", "i"), 0x1F: ("minii", "i"), 0x20: ("addq", "q"), 0x21: ("maddq", "q"),
       0x22: ("addi", "i"), 0x23: ("maddi", "i"), 0x24: ("subq", "q"), 0x25: ("msubq", "q"), 0x26: ("subi", "i"), 0x27: ("msubi", "i")}
UP3 = {0x28: "add", 0x29: "madd", 0x2A: "mul", 0x2B: "max", 0x2C: "sub", 0x2D: "msub", 0x2E: "opmsub", 0x2F: "mini"}
# special2 superiores: [op&3][fd]
SP = {0: {0: "adda", 1: "suba", 2: "madda", 3: "msuba", 4: "itof0", 5: "ftoi0", 6: "mula", 7: "mulaq", 8: "addaq", 9: "subaq", 10: "adda_", 11: "suba_"},
      1: {0: "adda", 1: "suba", 2: "madda", 3: "msuba", 4: "itof4", 5: "ftoi4", 6: "mula", 7: "abs", 8: "maddaq", 9: "msubaq", 10: "madda_", 11: "msuba_"},
      2: {0: "adda", 1: "suba", 2: "madda", 3: "msuba", 4: "itof12", 5: "ftoi12", 6: "mula", 7: "mulai", 8: "addai", 9: "subai", 10: "mula_", 11: "opmula"},
      3: {0: "adda", 1: "suba", 2: "madda", 3: "msuba", 4: "itof15", 5: "ftoi15", 6: "mula", 7: "clip", 8: "maddai", 9: "msubai", 10: "nop", 11: "nop"}}

def upper(w):
    op = w & 63; d = dest(w); ft = (w >> 16) & 31; fs = (w >> 11) & 31; fd = (w >> 6) & 31
    if op < 0x1C:
        base = op & 0x3C; bc = BC[op & 3]; n = UP[base]
        return f"{n}{bc}{d} {vf(fd)}, {vf(fs)}, {vf(ft)}{bc}"
    if op in UP2:
        n, r = UP2[op]; src = "Q" if r == "q" else "I"
        return f"{n[:-1]}{d} {vf(fd)}, {vf(fs)}, {src}" if True else ""
    if op in UP3: return f"{UP3[op]}{d} {vf(fd)}, {vf(fs)}, {vf(ft)}"
    if op >= 0x3C:
        k = op & 3; f = SP[k].get(fd)
        if f is None: return f"up?{w:08x}"
        if f == "nop": return "nop"
        if f.startswith("itof") or f.startswith("ftoi") or f == "abs": return f"{f}{d} {vf(ft)}, {vf(fs)}"
        if f == "clip": return f"clipw.xyz {vf(fs)}, {vf(ft)}w"
        if f in ("adda", "suba", "madda", "msuba", "mula"): return f"{f}{BC[k]}{d} ACC, {vf(fs)}, {vf(ft)}{BC[k]}"
        if f in ("adda_", "suba_"): return f"{f[:-1]}{d} ACC, {vf(fs)}, {vf(ft)}"
        if f in ("madda_", "msuba_", "mula_"): return f"{f[:-1]}{d} ACC, {vf(fs)}, {vf(ft)}"
        if f == "opmula": return f"opmula{d} ACC, {vf(fs)}, {vf(ft)}"
        if f.endswith("q"): return f"{f[:-1]}q{d} ACC, {vf(fs)}, Q"
        if f.endswith("ai") or f == "mulai": return f"{f[:-1]}{d} ACC, {vf(fs)}, I"
        return f"{f}?"
    return f"up?{w:08x}"

def imm11(w):
    v = w & 0x7ff; return v - 0x800 if v & 0x400 else v

def lower(w, pc):
    if w == LNOP: return "nop"
    op = w >> 25; d = dest(w); ft = (w >> 16) & 31; fs = (w >> 11) & 31; it = ft; is_ = fs; id_ = (w >> 6) & 31
    if op == 0x00: return f"lq{d} {vf(ft)}, {imm11(w)}({vi(is_)})"
    if op == 0x01: return f"sq{d} {vf(fs)}, {imm11(w)}({vi(it)})"
    if op == 0x04: return f"ilw{d} {vi(it)}, {imm11(w)}({vi(is_)})"
    if op == 0x05: return f"isw{d} {vi(it)}, {imm11(w)}({vi(is_)})"
    if op in (0x08, 0x09):
        v = (((w >> 21) & 15) << 11) | (w & 0x7ff); return f"{'iaddiu' if op == 8 else 'isubiu'} {vi(it)}, {vi(is_)}, {v:#x}"
    if op == 0x10: return f"fceq {w & 0xffffff:#x}"
    if op == 0x11: return f"fcset {w & 0xffffff:#x}"
    if op == 0x12: return f"fcand vi01, {w & 0xffffff:#x}"
    if op == 0x13: return f"fcor vi01, {w & 0xffffff:#x}"
    if op in (0x14, 0x15, 0x16, 0x17):
        v = ((w >> 21) & 1) << 11 | (w & 0x7ff); return f"{['fseq', 'fsset', 'fsand', 'fsor'][op - 0x14]} {vi(it) if op != 0x15 else ''} {v:#x}"
    if op == 0x18: return f"fmeq {vi(it)}, {vi(is_)}"
    if op == 0x1A: return f"fmand {vi(it)}, {vi(is_)}"
    if op == 0x1B: return f"fmor {vi(it)}, {vi(is_)}"
    if op == 0x1C: return f"fcget {vi(it)}"
    if op in (0x20, 0x21):
        t = pc + 1 + imm11(w); return (f"b {t}" if op == 0x20 else f"bal {vi(it)}, {t}")
    if op == 0x24: return f"jr {vi(is_)}"
    if op == 0x25: return f"jalr {vi(it)}, {vi(is_)}"
    if op in (0x28, 0x29, 0x2C, 0x2D, 0x2E, 0x2F):
        t = pc + 1 + imm11(w); n = {0x28: "ibeq", 0x29: "ibne", 0x2C: "ibltz", 0x2D: "ibgtz", 0x2E: "iblez", 0x2F: "ibgez"}[op]
        return f"{n} {vi(it)}, {vi(is_)}, {t}" if op in (0x28, 0x29) else f"{n} {vi(is_)}, {t}"
    if op == 0x40:
        o6 = w & 63
        if o6 == 0x30: return f"iadd {vi(id_)}, {vi(is_)}, {vi(it)}"
        if o6 == 0x31: return f"isub {vi(id_)}, {vi(is_)}, {vi(it)}"
        if o6 == 0x32:
            v = (w >> 6) & 31; v = v - 32 if v & 16 else v; return f"iaddi {vi(it)}, {vi(is_)}, {v}"
        if o6 == 0x34: return f"iand {vi(id_)}, {vi(is_)}, {vi(it)}"
        if o6 == 0x35: return f"ior {vi(id_)}, {vi(is_)}, {vi(it)}"
        if o6 >= 0x3C:
            k = o6 & 3; fd = id_; ftf = (w >> 23) & 3; fsf = (w >> 21) & 3
            fld = lambda r, f: f"{vf(r)}{'xyzw'[f]}"
            T = {(0, 0x0C): ("move", "m"), (1, 0x0C): ("mr32", "m"), (0, 0x0D): ("lqi", "lqi"), (1, 0x0D): ("sqi", "sqi"), (2, 0x0D): ("lqd", "lqd"), (3, 0x0D): ("sqd", "sqd"),
                 (0, 0x0E): ("div", "div"), (1, 0x0E): ("sqrt", "sqrt"), (2, 0x0E): ("rsqrt", "rsqrt"), (3, 0x0E): ("waitq", "w"),
                 (0, 0x0F): ("mtir", "mtir"), (1, 0x0F): ("mfir", "mfir"), (2, 0x0F): ("ilwr", "ilwr"), (3, 0x0F): ("iswr", "iswr"),
                 (0, 0x10): ("rnext", "rn"), (1, 0x10): ("rget", "rn"), (2, 0x10): ("rinit", "ri"), (3, 0x10): ("rxor", "ri"),
                 (0, 0x19): ("mfp", "mfp"), (0, 0x1A): ("xtop", "xt"), (1, 0x1A): ("xitop", "xt"), (0, 0x1B): ("xgkick", "xg")}
            e = T.get((k, fd))
            if not e: return f"efu?{w:08x}" if fd >= 0x1C or fd < 0xC else f"lw?{w:08x}"
            n, f = e
            if f == "m": return f"{n}{d} {vf(ft)}, {vf(fs)}"
            if f == "lqi": return f"lqi{d} {vf(ft)}, ({vi(is_)}++)"
            if f == "sqi": return f"sqi{d} {vf(fs)}, ({vi(it)}++)"
            if f == "lqd": return f"lqd{d} {vf(ft)}, (--{vi(is_)})"
            if f == "sqd": return f"sqd{d} {vf(fs)}, (--{vi(it)})"
            if f == "div": return f"div Q, {fld(fs, fsf)}, {fld(ft, ftf)}"
            if f == "sqrt": return f"sqrt Q, {fld(ft, ftf)}"
            if f == "rsqrt": return f"rsqrt Q, {fld(fs, fsf)}, {fld(ft, ftf)}"
            if f == "w": return "waitq"
            if f == "mtir": return f"mtir {vi(it)}, {fld(fs, fsf)}"
            if f == "mfir": return f"mfir{d} {vf(ft)}, {vi(is_)}"
            if f == "ilwr": return f"ilwr{d} {vi(it)}, ({vi(is_)})"
            if f == "iswr": return f"iswr{d} {vi(it)}, ({vi(is_)})"
            if f == "mfp": return f"mfp{d} {vf(ft)}, P"
            if f == "xt": return f"{n} {vi(it)}"
            if f == "xg": return f"xgkick {vi(is_)}"
            return f"{n} ?"
    return f"low?{w:08x}"

def disasm(buf, base, n=None, vbase=0):
    """Devuelve lista de (pc, vaddr, upper_text, lower_text, ibit, ebit, mbit)."""
    out = []
    for i in range((len(buf) // 8) if n is None else n):
        lo, hi = struct.unpack_from("<II", buf, 8 * i)
        ib, eb, mb, db, tb = (hi >> 31) & 1, (hi >> 30) & 1, (hi >> 29) & 1, (hi >> 28) & 1, (hi >> 27) & 1
        lt = f"loi {struct.unpack('<f', struct.pack('<I', lo))[0]:g}" if ib else lower(lo, i)
        out.append((i, vbase + 8 * i, upper(hi), lt, ib, eb, mb))
    return out

TABLE = 0x1B1A40          # 5 punteros a descriptores de programa; cada descriptor = 8 x {vaddr, destino en VU1 mem, 0x800} + 0xFFFFFFFF
VU1_BASE = 0x1B1C80       # la rutina de subida (MPG por trozos de 0x800 B = 256 instr) empieza en 0x1B1C80 (no 0x1B1C88)

def load_programs():
    """Devuelve lista de (vaddr_primer_trozo, bytes) con los 5 programas de 16 KB (8 trozos de 256 instr) segun la tabla del ELF."""
    d = ELF.read_bytes(); progs = []
    for k in range(5):
        desc = struct.unpack_from("<I", d, off(TABLE + 4 * k))[0]; code = b""; first = None
        for c in range(8):
            v, dst, sz = struct.unpack_from("<3I", d, off(desc + 12 * c))
            if first is None: first = v
            assert dst == 0x800 * c and sz == 0x800, (k, c, hex(v), hex(dst), hex(sz))
            code += d[off(v):off(v) + sz]
        progs.append((first, code))
    return progs

if __name__ == "__main__":
    a = sys.argv[1:]
    progs = load_programs()
    if "--list" in a or not a:
        for k, (v, b) in enumerate(progs): print(k, hex(v), len(b) // 8, "instr")
        sys.exit()
    if "--prog" in a:
        k = int(a[a.index("--prog") + 1]); v, b = progs[k]
        for pc, va, u, l, ib, eb, mb in disasm(b, 0, vbase=v):
            print(f"{pc:4d} {va:#08x}  {'[E]' if eb else '   '} {u:<34s} | {l}")
    elif "--from" in a:
        v = int(a[a.index("--from") + 1], 0); n = int(a[a.index("--count") + 1]); d = ELF.read_bytes()
        for pc, va, u, l, ib, eb, mb in disasm(d[off(v):off(v) + 8 * n], 0, vbase=v): print(f"{pc:4d} {va:#08x}  {'[E]' if eb else '   '} {u:<34s} | {l}")

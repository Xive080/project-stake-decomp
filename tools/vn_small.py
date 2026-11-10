#!/usr/bin/env python3
"""Auto-matcher de funciones pequenas SIN saltos ni llamadas (hoja, linea recta) de `main`.
Ejecuta simbolicamente el codigo del ELF, genera C equivalente, compila con mwccps2 (wibo) y
verifica byte a byte (relocaciones enmascaradas). Los que coinciden van a src/auto/small.cpp.
Uso: vn_small.py [--max-instr 10] [--limit N] [--flags "-O4,p"]   (sin rutas absolutas)"""
import argparse, json, re, struct, subprocess, sys, tempfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import capstone
import probe_compilers as P

ROOT = P.ROOT
CC = ROOT / "tools/compilers/mwcps2-2.4-001213"
D = P.ELF; BASE, FOFF = P.BASE, P.FOFF
md = capstone.Cs(capstone.CS_ARCH_MIPS, capstone.CS_MODE_MIPS64 + capstone.CS_MODE_LITTLE_ENDIAN)

LOAD = {"ld": ("long long", 8), "lw": ("int", 4), "lh": ("short", 2), "lhu": ("unsigned short", 2), "lb": ("signed char", 1), "lbu": ("unsigned char", 1)}
STORE = {"sd": "long long", "sw": "int", "sh": "short", "sb": "char"}
REG = re.compile(r"\$(\w+)")

class Skip(Exception): pass
SWAP = True

class K:
    """constante que viene de lui(+addiu/ori): es una direccion de simbolo en el original"""
    def __init__(self, v, sym=False): self.v = v & 0xffffffff; self.sym = sym
    def __add__(self, o): return K(self.v + o, True)

CALLEES = {}
GG = {}   # globales gp-relativas por nombre -> tipo
GL = {}   # nombre -> tipo C de las globales externas
def gsym(addr, ty):
    n = f"g_{addr:08X}_{ty.replace(' ', '')}"
    GL[n] = ty
    return n


def kv(e):
    if isinstance(e, K):
        if not e.sym: return str(e.v - (1 << 32) if e.v & 0x80000000 else e.v)
        return f"((int){gsym(e.v, 'char')})"
    return e

def sx(v): return v - 0x10000 if v & 0x8000 else v

def sym_exec(ins, retmode=True, hoist=None, gt=False):
    """ins: lista de instrucciones capstone ya ordenadas por ejecucion (delay slot movido tras jr).
    Devuelve (args_usados, stmts, retexpr|None)."""
    regs = {"zero": "0"}
    used_args, written = set(), set()
    S = [[]]; RV = []; cnt = {}; ptrs = {}
    def emit(s): S[-1].append(s)
    tmp = [0]
    def get(r):
        if r == "zero": return "0"
        if r in regs: return regs[r]
        if r in ("a0", "a1", "a2", "a3"):
            used_args.add(int(r[1])); return r
        if r == "gp": return "$gp"
        raise Skip(f"lee {r} sin definir")
    written_since = set()
    LV = {}
    def put(r, e):
        written.add(r); written_since.add(r)
        if r in LV:
            n = LV[r]; e2 = kv(e)
            if e2 != n: S[-1].append(f"{n} = {e2};")
            regs[r] = n
        else: regs[r] = e
    def addr(base, off):
        b = get(base)
        if b == "$gp": return ("gp", off)
        return b, off
    def op(i):
        m, ops = i.mnemonic, [o.strip() for o in i.op_str.split(",")] if i.op_str else []
        R = lambda s: REG.match(s).group(1)
        if m == "nop": return
        if m in ("addiu", "daddiu") and ops and ops[0] == "$sp": return
        if m in ("addiu", "daddiu") and len(ops) == 3:
            rt, rs, imm = R(ops[0]), R(ops[1]), int(ops[2], 0)
            if rs == "zero": put(rt, str(imm))
            elif isinstance(get(rs), K): put(rt, get(rs) + imm)
            else: put(rt, f"({get(rs)} + {imm})")
        elif m in ("addu", "daddu", "or") and len(ops) == 3:
            rd, rs, rt = R(ops[0]), R(ops[1]), R(ops[2])
            if rt == "zero": put(rd, get(rs))
            elif rs == "zero": put(rd, get(rt))
            else:
                op = "|" if m == "or" else "+"
                x, y = kv(get(rs)), kv(get(rt))
                simple = lambda e: re.fullmatch(r"a\d|t\d+", e)
                if SWAP and simple(y) and not simple(x): x, y = y, x
                put(rd, f"({x} {op} {y})")
        elif m == "subu": put(R(ops[0]), f"({kv(get(R(ops[1])))} - {kv(get(R(ops[2])))})")
        elif m == "lui": put(R(ops[0]), K(int(ops[1], 0) << 16))
        elif m == "move" and len(ops) == 2: put(R(ops[0]), get(R(ops[1])))
        elif m == "and" and len(ops) == 3: put(R(ops[0]), f"({kv(get(R(ops[1])))} & {kv(get(R(ops[2])))})")
        elif m in ("ori", "andi", "xori"):
            op = {"ori": "|", "andi": "&", "xori": "^"}[m]; rt, rs, imm = R(ops[0]), R(ops[1]), int(ops[2], 0)
            if rs == "zero" and m == "ori": put(rt, str(imm))
            elif isinstance(get(rs), K) and m == "ori": put(rt, str((get(rs).v | imm) - ((1 << 32) if (get(rs).v | imm) & 0x80000000 else 0)))
            else: put(rt, f"({kv(get(rs))} {op} {imm})")
        elif m in ("sll", "sra", "srl"):
            op = {"sll": "<<", "sra": ">>", "srl": ">>"}[m]; e = get(R(ops[1]))
            if m == "srl": e = f"(unsigned int){e}"
            put(R(ops[0]), f"({e} {op} {int(ops[2], 0)})")
        elif m in ("slt", "sltu", "slti", "sltiu"):
            a_, b_ = kv(get(R(ops[1]))), (kv(get(R(ops[2]))) if m in ("slt", "sltu") else str(int(ops[2], 0)))
            if m.endswith("u"): a_, b_ = f"(unsigned int){a_}", f"(unsigned int){b_}"
            put(R(ops[0]), f"({a_} < {b_})")
        elif m == "jal":
            tgt = int(ops[0], 0); n = max([int(r[1]) + 1 for r in written_since if r in ("a0", "a1", "a2", "a3")] + [0])
            args = []
            for k in range(n):
                r = f"a{k}"
                args.append(kv(get(r)) if (r in regs and r in written_since) else (get(r)))
            callee = f"c_{tgt:08X}"; CALLEES[callee] = max(CALLEES.get(callee, 0), n)
            t = f"t{tmp[0]}"; tmp[0] += 1
            emit(f"int {t} = {callee}({', '.join(args)});")
            for r in ("v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9"): regs.pop(r, None)
            regs["v0"] = t; written.add("v0"); written_since.clear()
        elif (m in LOAD or m in STORE) and re.search(r"\(\$sp\)", ops[1].replace(" ", "")):
            if not re.match(r"\$(ra|s\d|fp)$", ops[0]): raise Skip("local de pila")
        elif m in LOAD or m in STORE:
            mm = re.match(r"(-?(?:0x)?[0-9a-fA-F]+)?\((\$\w+)\)", ops[1].replace(" ", ""))
            off = int(mm.group(1) or "0", 0); base = REG.match(mm.group(2)).group(1)
            b, o = addr(base, off)
            if isinstance(b, K):
                ty = (LOAD[m][0] if m in LOAD else STORE[m]); lval = gsym((b.v + off) & 0xffffffff, ty) + "[0]"
            elif b == "gp":
                ty = (LOAD[m][0] if m in LOAD else STORE[m])
                a = f"g_gp_{o & 0xffff:04X}_{ty.replace(' ', '')}"  # simbolo global tipado
                GG[a] = ty; lval = a
            else:
                # base numerica (lui+lw absoluto) o expresion
                if re.fullmatch(r"-?\d+", b): lval = None; absaddr = (int(b) + off) & 0xffffffff
                ty = (LOAD[m][0] if m in LOAD else STORE[m])
                if re.fullmatch(r"-?\d+", b): lval = f"(*({ty} *)0x{absaddr:X})"
                else:
                    cnt[b] = cnt.get(b, 0) + 1
                    hc, hm = hoist if hoist is not None else ({}, 0)
                    if hoist is not None and not re.fullmatch(r"[at]\d+", b) and (hc.get(b, 0) >= 2 if hm == 1 else "g_" in b):
                        if b not in ptrs:
                            ptrs[b] = f"p{len(ptrs)}"; decl_ = f"char *{ptrs[b]} = (char *)({b});"
                            if hm == 1: emit(decl_)
                            else:
                                cur = S[-1]; pos = 0
                                for tn in re.findall(r"\bt\d+\b", b):
                                    for q_, l_ in enumerate(cur):
                                        if re.match(rf"[a-z ]+ {tn} = ", l_): pos = max(pos, q_ + 1)
                                while pos < len(cur) and cur[pos].startswith("char *p"): pos += 1
                                cur.insert(pos, decl_)
                        lval = f"(*({ty} *)({ptrs[b]} + {off}))"
                    else: lval = f"(*({ty} *)({b} + {off}))"
            if m in LOAD:
                t = f"t{tmp[0]}"; tmp[0] += 1
                emit(f"{LOAD[m][0]} {t} = {lval};"); put(R(ops[0]), t)
            else:
                emit(f"{lval} = {kv(get(R(ops[0])))};")
        else: raise Skip(f"op no soportada: {m}")
    BRN = ("beq", "bne", "beqz", "bnez", "bltz", "blez", "bgtz", "bgez")
    BRL = ("beql", "bnel", "beqzl", "bnezl", "bltzl", "blezl", "bgtzl", "bgezl")
    BR = BRN + BRL
    def rw(i):
        """(lee, escribe) aproximado de una instruccion, para vivacidad"""
        m = i.mnemonic; rs = re.findall(r"\$(\w+)", i.op_str)
        if m in STORE or m in BR or m in ("jr", "mult", "multu"): return set(rs), set()
        if m == "jal": return {"a0", "a1", "a2", "a3"}, {"v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9"}
        if m in ("nop", "b", "j"): return set(), set()
        return set(rs[1:]), set(rs[:1])
    def live_from(k):
        live, wr = set(), set()
        for i in ins[k:]:
            r, w = rw(i)
            if i.mnemonic == "jr" and retmode: r = r | {"v0"}
            live |= (r - wr); wr |= w
        return live
    mv = [0]; MVS = []
    def snap(): return (dict(regs), set(written), set(written_since))
    def load(s): regs.clear(); regs.update(s[0]); written.clear(); written.update(s[1]); written_since.clear(); written_since.update(s[2])
    idx_of = {i.address: k for k, i in enumerate(ins)}
    end_addr = ins[-1].address + 4
    CMP = {"beq": "==", "bne": "!="}
    def cond_of(i):
        m = i.mnemonic; ops = [o.strip() for o in i.op_str.split(",")]; R = lambda s: REG.match(s).group(1)
        if m in BRL: m = m[:-1]
        if m in CMP: return f"({kv(get(R(ops[0])))} {CMP[m]} {kv(get(R(ops[1])))})"
        z = {"beqz": "==", "bnez": "!=", "bltz": "<", "blez": "<=", "bgtz": ">", "bgez": ">="}[m]
        return f"({kv(get(R(ops[0])))} {z} 0)"
    def merge(sa, sb, k_after, bodya, bodyb):
        """fusiona dos estados; devuelve en regs el estado fusionado y anade asignaciones a las ramas"""
        live = live_from(k_after)
        diff = [r for r in set(sa[0]) | set(sb[0]) if sa[0].get(r) != sb[0].get(r) and r in live and r not in ("zero", "sp", "ra")]
        load(sa)
        for r in diff:
            if r not in sa[0] or r not in sb[0]: raise Skip("registro indefinido en una rama")
            n = f"m{mv[0]}"; mv[0] += 1; MVS.append(n)
            bodya.append(f"{n} = {kv(sa[0][r])};"); bodyb.append(f"{n} = {kv(sb[0][r])};"); regs[r] = n
        for r in list(regs):
            if r not in diff and regs.get(r) != sb[0].get(r) and r != "zero": regs.pop(r, None)
    loops = {}; entered = set()
    for kk, i_ in enumerate(ins):
        if i_.mnemonic in BRN:
            tg = int(i_.op_str.split(",")[-1].strip(), 0)
            if tg <= i_.address and tg in idx_of:
                if idx_of[tg] in loops: raise Skip("varios saltos atras")
                loops[idx_of[tg]] = kk
        elif i_.mnemonic in BRL and int(i_.op_str.split(",")[-1].strip(), 0) <= i_.address: raise Skip("bucle con branch-likely")
    def loop_vars(lo, hi, after, cond_ins):
        wr, rd = set(), set()
        for q in range(lo, hi):
            r_, w_ = rw(ins[q]); rd |= r_; wr |= w_
        live = live_from(after); cr = rw(cond_ins)[0]
        bad = {"zero", "sp", "ra", "gp"}
        return sorted(r for r in wr if r not in bad and (r in rd or r in live or r in cr)), wr
    def new_var(r, init_ok=True):
        n = f"l{mv[0]}"; mv[0] += 1; MVS.append(n); return n
    def start_loop(vs):
        names = {}
        for r in vs:
            if r in ("a0", "a1", "a2", "a3") and int(r[1]) in used_args:
                names[r] = r
                if r in regs and kv(regs[r]) != r: S[-1].append(f"{r} = {kv(regs[r])};")
                continue
            n = new_var(r); names[r] = n
            if r in regs: S[-1].append(f"{n} = {kv(regs[r])};")
            elif r in ("a0", "a1", "a2", "a3"): S[-1].append(f"{n} = {get(r)};")
        for r, n in names.items(): regs[r] = n; written.add(r); LV[r] = n
        return names
    def end_iter(names):
        for r, n in names.items():
            if regs.get(r) != n:
                if r not in regs: raise Skip("variable de bucle sin valor")
                emit(f"{n} = {kv(regs[r])};"); regs[r] = n
    def after_loop(names, wr):
        for r in names: LV.pop(r, None)
        for r in wr:
            if r not in names and r not in ("zero", "sp", "ra", "gp"): regs.pop(r, None)
    def do_dw(top, b):
        """bucle do-while: ins[top:b] cuerpo, ins[b] rama hacia atras, ins[b+1] delay"""
        entered.add(top); bi = ins[b]
        vs, wr = loop_vars(top, b + 2, b + 2, bi)
        if rw(ins[b + 1])[1] & rw(bi)[0]: raise Skip("el delay modifica la condicion")
        names = start_loop(vs); outer = S[-1]; S.append([])
        if run(top, b): raise Skip("retorno dentro del bucle")
        op(ins[b + 1]); end_iter(names)
        c = cond_of(bi); body = S.pop()
        outer.append("do {"); outer.extend(body); outer.append(f"}} while {c};")
        after_loop(names, wr); return b + 2
    def do_while(k, E, top, cb):
        """bucle while rotado: `b E; delay; top: cuerpo; E: condicion; bcc top`"""
        entered.add(top); bi = ins[cb]
        if k + 1 < len(ins): op(ins[k + 1])
        vs, wr = loop_vars(top, E, cb + 2, bi)
        vs = [r for r in vs if r in regs or r in ("a0", "a1", "a2", "a3") or True]
        names = start_loop(vs); outer = S[-1]
        S.append([])
        for q in range(E, cb):
            if ins[q].mnemonic in BR or ins[q].mnemonic in ("b", "jal"): raise Skip("llamada/salto en la condicion del bucle")
            op(ins[q])
        c = cond_of(bi)
        if rw(ins[cb + 1])[1] & rw(bi)[0]: raise Skip("el delay modifica la condicion")
        cap = S.pop(); ex = []
        for l_ in cap:
            m_ = re.match(r"^(?:[a-z ]+? )?([tl]\d+) = (.*);$", l_)
            if not m_: raise Skip("sentencia no expresable en la condicion")
            if m_.group(1)[0] == "t": MVS.append(m_.group(1))
            ex.append(f"{m_.group(1)} = {m_.group(2)}")
        st_cond = snap()
        S.append([])
        if run(top, E): raise Skip("retorno dentro del bucle")
        if ins[cb + 1].mnemonic != "nop": op(ins[cb + 1])
        end_iter(names); body = S.pop()
        if gt:
            gid = mv[0]; mv[0] += 1
            outer.append(f"goto Lc{gid};"); outer.append(f"Lt{gid}: ;"); outer.extend(body); outer.append(f"Lc{gid}: ;")
            outer.extend(x_ + ";" for x_ in ex); outer.append(f"if {c} goto Lt{gid};")
        else:
            outer.append(f"while ({', '.join(ex + [c])}) {{"); outer.extend(body); outer.append("}")
        sym_exec.has_w = True
        load(st_cond); after_loop(names, wr)
        return cb + 2
    def run(lo, hi):
        """ejecuta ins[lo:hi]; True si termina con jr"""
        k = lo
        while k < hi:
            i = ins[k]; m = i.mnemonic
            if k in loops and k not in entered:
                if loops[k] + 2 > hi: raise Skip("bucle fuera de region")
                k = do_dw(k, loops[k]); continue
            if m == "jr":
                if k + 1 < len(ins): op(ins[k + 1])
                v0 = regs.get("v0") if (retmode and "v0" in written) else None
                RV.append(v0 is not None)
                emit(f"return {kv(v0)};" if v0 is not None else "return;")
                return True
            if m == "jal":
                if k + 1 < len(ins): op(ins[k + 1])
                op(i); k += 2; continue
            if m == "b":
                tgt = int(i.op_str, 0); E = idx_of.get(tgt)
                if E is not None and E > k and (k + 2) in loops and loops[k + 2] >= E and (k + 2) not in entered:
                    k = do_while(k, E, k + 2, loops[k + 2]); continue
                raise Skip("salto incondicional/bucle")
            if m in BR:
                tgt = int(i.op_str.split(",")[-1].strip(), 0)
                if tgt <= i.address or (tgt not in idx_of and tgt != end_addr): raise Skip("bucle")
                T = idx_of.get(tgt, len(ins))
                if T > hi: raise Skip("salto fuera de region")
                c = cond_of(i); likely = m in BRL
                if likely:
                    fall = snap()
                    if k + 1 < len(ins): op(ins[k + 1])
                    taken = snap(); load(fall)
                else:
                    if k + 1 < len(ins): op(ins[k + 1])
                    taken = snap()
                body0 = k + 2; E = None
                if not likely and T - 2 >= body0 and ins[T - 2].mnemonic == "b":
                    e_ = int(ins[T - 2].op_str, 0)
                    if e_ > tgt:
                        E = idx_of.get(e_, len(ins))
                        if E > hi: raise Skip("else fuera de region")
                outer = S[-1]; S.append([])
                if E is None:
                    term = run(body0, T); thenb = S.pop()
                    if term:
                        load(taken); outer.append(f"if (!{c}) {{"); outer.extend(thenb); outer.append("}"); k = T; continue
                    st_then = snap(); elseb = []
                    merge(st_then, taken, T, thenb, elseb)
                    outer.append(f"if (!{c}) {{"); outer.extend(thenb); outer.append("}" if not elseb else "} else {")
                    if elseb: outer.extend(elseb); outer.append("}")
                    k = T; continue
                term_t = run(body0, T - 2)
                if not term_t and T - 1 < len(ins): op(ins[T - 1])
                thenb = S.pop(); st_then = snap(); load(taken); S.append([])
                term_e = run(T, E); elseb = S.pop(); st_else = snap()
                if term_t and term_e:
                    outer.append(f"if (!{c}) {{"); outer.extend(thenb); outer.append("} else {"); outer.extend(elseb); outer.append("}"); return True
                if term_t or term_e: raise Skip("if/else con un solo retorno")
                merge(st_then, st_else, E, thenb, elseb)
                outer.append(f"if (!{c}) {{"); outer.extend(thenb); outer.append("} else {"); outer.extend(elseb); outer.append("}")
                k = E; continue
            if m in ("j", "jalr") or m[0] == "b":
                raise Skip(f"op no soportada: {m}")
            op(i); k += 1
        return False
    if not run(0, len(ins)): raise Skip("sin retorno")
    if len(set(RV)) != 1: raise Skip("retornos inconsistentes")
    stmts = [f"int {n};" for n in sorted(set(MVS))] + S[0]
    last = stmts[-1]
    if not last.startswith("return"): raise Skip("no acaba en return")
    ret = last[len("return "):-1] if last.startswith("return ") else None
    sym_exec.cnt = cnt
    return sorted(used_args), stmts[:-1], ret

def collect(max_instr):
    out = []
    syms = [(v, nm) for v, nm in P.nm_funcs(P.ELFP)]
    syms.sort()
    from_main = {}
    # funciones main con tamano: re-leer symtab con tamanos
    import coverage_helper
    return coverage_helper.main_funcs()

def f_name_marker(v): return f"@ 0x{v:X}\n"

import random
def units_of(stmts):
    """agrupa sentencias de nivel superior: un bloque if/else completo es una unidad"""
    us, cur, depth = [], [], 0
    for l in stmts:
        cur.append(l); depth += l.count("{") - l.count("}")
        if depth == 0: us.append(cur); cur = []
    return us if depth == 0 else None
def unit_info(u):
    txt = "\n".join(u)
    defs = set(re.findall(r"^(?:[a-z ]+\*?) ?\*?([tpm]\d+)\b ?(?:=|;)", u[0])) if len(u) == 1 else set()
    uses = set(re.findall(r"\b[tpma]\d+\b", txt)) - defs
    blk = len(u) > 1 or "c_" in txt
    st = bool(re.match(r"\s*(\(\*|g_)", u[0])) or blk
    ld = "*(" in txt or "[0]" in txt or bool(re.search(r"= g_gp_\w+;", txt))
    if u[0].startswith("char *p"): ld = st = False
    return defs, uses, st, ld, blk
def random_orders(stmts, n, rng):
    us = units_of(stmts)
    if not us or len(us) < 3: return []
    info = [unit_info(u) for u in us]; m = len(us)
    ret = us[-1] if us[-1][0].startswith("return") else None
    k = m - 1 if ret else m
    deps = [set() for _ in range(k)]
    for j in range(k):
        for i in range(j):
            di, ui, si, li, bi = info[i]; dj, uj, sj, lj, bj = info[j]
            if (di & uj) or (dj & ui) or (si and (lj or sj)) or (sj and li) or bi or bj: deps[j].add(i)
    seen, out = set(), []
    for _ in range(n * 4):
        rem, order = set(range(k)), []
        while rem:
            ready = [x for x in rem if deps[x] <= set(order)]
            x = rng.choice(ready); order.append(x); rem.discard(x)
        t = tuple(order)
        if t == tuple(range(k)) or t in seen: continue
        seen.add(t); out.append([l for x in order for l in us[x]] + (ret or []))
        if len(out) >= n: break
    return out

def main():
    ap = argparse.ArgumentParser(); ap.add_argument("--max-instr", type=int, default=10)
    ap.add_argument("--limit", type=int, default=0); ap.add_argument("--show", type=int, default=0); ap.add_argument("--flags", default="-O4,p")
    ap.add_argument("--dump-near", action="store_true", help="escribe build/perm_src/<mod>/ con el C de los casi-aciertos")
    ap.add_argument("--permute", type=int, default=0, help="variantes de orden de sentencias por funcion casi igual")
    ap.add_argument("--module", default="main", help="main o un overlay (gamemod, orgmod...)")
    a = ap.parse_args()
    import coverage_helper
    global D, BASE, FOFF
    if a.module != "main":
        from elftools.elf.elffile import ELFFile
        ef = ELFFile(open(ROOT / "baserom/SCES_504.11", "rb")); nm_ = {i: x.name for i, x in enumerate(ef.iter_sections())}
        D = (ROOT / f"disc/TBL/{a.module.upper()}.BIN").read_bytes(); BASE, FOFF = struct.unpack_from("<I", D, 8)[0], 0
        allf = sorted((x["st_value"], x["st_size"], x.name) for x in ef.get_section_by_name(".symtab").iter_symbols()
                      if x["st_info"]["type"] == "STT_FUNC" and nm_.get(x["st_shndx"]) == a.module)
    else: allf = coverage_helper.main_funcs()
    suf = "" if a.module == "main" else "_" + a.module
    funcs = [f for f in allf if f[1] <= a.max_instr * 4 + 4 and f[1] >= 8]
    if a.limit: funcs = funcs[:a.limit]
    cands, skips = [], {}
    for v, sz, nm in funcs:
        off = v - BASE + FOFF
        raw = bytearray(D[off: off + sz])
        for q in range(0, len(raw) - 3, 4):   # sq/lq rt,off(sp) (R5900): guardado/restauro de ra/s*, capstone no los decodifica
            w = struct.unpack_from("<I", raw, q)[0]
            if (w >> 26) in (0x1f, 0x1e) and ((w >> 21) & 31) == 29:
                struct.pack_into("<I", raw, q, (0x2b << 26 if (w >> 26) == 0x1f else 0x23 << 26) | (w & 0x03ffffff))  # sw/lw (mismo rt/base/off bajo)
        ins = list(md.disasm(bytes(raw), v))
        if len(ins) * 4 < sz - 12: skips[nm] = "no decodificable (R5900)"; continue
        if any(x.mnemonic == "j" for x in ins): skips[nm] = "thunk j (no reproducible en C)"; continue
        if not any(x.mnemonic == "jr" for x in ins): skips[nm] = "sin jr/delay"; continue
        modes = [True, False] if any(x.mnemonic in ("beq", "bne", "beqz", "bnez", "bltz", "blez", "bgtz", "bgez") for x in ins) else [True]
        got = False; err = None
        for rm_, gt_ in [(m_, g_) for m_ in modes for g_ in (False, True)]:
            sym_exec.has_w = False
            if gt_:
                try: sym_exec(ins, rm_)
                except Exception: pass
                if not sym_exec.has_w: continue
            for hz in (0, 1, 2):
                try:
                    args, stmts, ret = sym_exec(ins, rm_, None, gt_)
                    if hz:
                        c1 = dict(sym_exec.cnt)
                        if hz == 1 and not any(n >= 2 and not re.fullmatch(r"[at]\d+", b_) for b_, n in c1.items()): continue
                        if hz == 2 and not any("g_" in b_ for b_ in c1): continue
                        args, stmts, ret = sym_exec(ins, rm_, (c1, hz), gt_)
                    if any('$' in x for x in stmts) or (ret and '$' in str(ret)): raise Skip('usa $gp como valor')
                except Skip as e: err = str(e); continue
                except Exception as e: err = f"error {type(e).__name__}: {e}"; continue
                cands.append((v, nm, args, stmts, ret, ("_r" if rm_ else "_v") + ("g" if gt_ else "") + ("h%d" % hz if hz else ""))); got = True
        if not got: skips[nm] = err
    lines = ["// generado por tools/vn_small.py; byte-match verificado contra SCES_504.11 (mwcps2-2.4-001213 -O4,p)"]
    decl = "".join(f"int {c}(...);\n" for c, n in sorted(CALLEES.items())) + "".join(f"extern {GL[g]} {g}[64];\n" for g in sorted(GL)) + "".join(f"extern {GG[g]} {g};\n" for g in sorted(GG))
    def render(c, one_line=False):
        v, nm, args, stmts, ret, tag = c
        params = ", ".join(f"int a{k}" for k in range(max(args) + 1)) if args else "void"
        rt = "int" if ret is not None else "void"
        sep = " " if one_line else "\n    "
        return f"// {nm} @ 0x{v:X}\n{rt} f_{v:08X}{tag}({params}) {{{sep}" + sep.join(stmts) + (f"{sep}return {ret};" if ret is not None else "") + ("\n}\n" if not one_line else " }\n")
    (ROOT / "build").mkdir(exist_ok=True)
    def compile_batch(cs):
        src = "\n".join(lines) + "\n" + decl + "\n" + "\n".join(render(c) for c in cs)
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as td:
            cf = Path(td) / "batch.cpp"; cf.write_text(src); o = Path(td) / "batch.o"
            subprocess.run([str(P.WIBO), str(CC / "mwccps2.exe"), "-lang", "c++", *a.flags.split(), "-c", "-o", str(o), str(cf)],
                           cwd=CC, capture_output=True, text=True)
            if o.exists(): return {n.split("__")[0]: (b, rm) for n, b, rm in P.funcs_of(o)}
        if len(cs) == 1: return {}
        h = len(cs) // 2; r = compile_batch(cs[:h]); r.update(compile_batch(cs[h:])); return r
    built = compile_batch(cands)
    ok, bad, okc = [], [], {}
    for c in cands:
        v, nm, args, stmts, ret, tag = c
        fn = f"f_{v:08X}{tag}"
        if v in okc: continue
        if fn not in built: bad.append((v, nm, "no compila")); continue
        mine, rm = built[fn]; mine = P.trim(mine)
        sz = next(s for vv, s, n in funcs if vv == v)
        eb = P.mask(P.trim(D[v - BASE + FOFF: v - BASE + FOFF + sz]), 0, rm)
        if mine == eb: ok.append((v, nm, "")); okc[v] = c
        else:
            bad.append((v, nm, "bytes distintos"))
            if a.show > 0:
                a.show -= 1
                print(f"--- {nm} @0x{v:X}\n{render(c)}")
                for t_, bb in (("elf ", eb), ("mine", mine)):
                    print(" ", t_, " ; ".join(f"{i.mnemonic} {i.op_str}" for i in md.disasm(bb, 0)))
    if a.dump_near:
        dd = ROOT / "build/perm_src" / a.module; dd.mkdir(parents=True, exist_ok=True); nn = 0
        dec_c = decl.replace("(...)", "()")
        for c in cands:
            v, nm, args, stmts, ret, tag = c
            fn = f"f_{v:08X}{tag}"
            if v in okc or fn not in built: continue
            sz = next(s_ for vv, s_, n_ in funcs if vv == v); eb_raw = D[v - BASE + FOFF: v - BASE + FOFF + sz]
            if len(P.trim(built[fn][0])) != len(P.trim(eb_raw)): continue
            hd, bd = [], []
            for l_ in render(c).splitlines():
                m_ = re.match(r"^(\s*)((?:unsigned |signed )?(?:long long|short|int|char))( \*)? ?\*?([tpm]\d+) = (.*);$", l_)
                if m_ and not l_.startswith("//"): hd.append(f"    {m_.group(2)}{m_.group(3) or ''} {m_.group(4)};".replace(" ;", ";")); bd.append(f"{m_.group(1)}{m_.group(4)} = {m_.group(5)};")
                else: bd.append(l_)
            body_ = []; done = False
            for l_ in bd:
                body_.append(l_)
                if not done and l_.rstrip().endswith("{") and not l_.startswith("//") and "f_" in l_: body_.extend(hd); done = True
            (dd / f"{fn}.c").write_text(dec_c + "\n" + "\n".join(body_) + "\n"); (dd / f"{fn}.json").write_text(json.dumps({"hex": P.trim(eb_raw).hex(), "name": nm, "addr": v}))
            nn += 1
        print(f"casi-aciertos volcados: {nn} en {dd.relative_to(ROOT)}")
    if a.permute:
        rng = random.Random(1); pv = []
        for c in cands:
            v, nm, args, stmts, ret, tag = c
            if v in okc: continue
            fn = f"f_{v:08X}{tag}"
            if fn not in built: continue
            sz = next(s_ for vv, s_, n_ in funcs if vv == v)
            if len(P.trim(built[fn][0])) != len(P.trim(D[v - BASE + FOFF: v - BASE + FOFF + sz])): continue
            for q, ns in enumerate(random_orders(stmts, a.permute, rng)):
                pv.append((v, nm, args, ns, ret, f"{tag}p{q}"))
        print(f"permutaciones: {len(pv)}")
        if pv:
            b2 = compile_batch(pv)
            for c in pv:
                v, nm, args, stmts, ret, tag = c
                if v in okc: continue
                fn = f"f_{v:08X}{tag}"
                if fn not in b2: continue
                mine, rm = b2[fn]; mine = P.trim(mine); sz = next(s_ for vv, s_, n_ in funcs if vv == v)
                if mine == P.mask(P.trim(D[v - BASE + FOFF: v - BASE + FOFF + sz]), 0, rm): ok.append((v, nm, "perm")); okc[v] = c
    bad = [b for b in bad if b[0] not in okc]
    print(f"candidatas: {len(funcs)}  ejecutadas: {len({c[0] for c in cands})}  MATCH: {len(ok)}  distintas: {len({b[0] for b in bad})}  descartadas: {len(skips)}")
    from collections import Counter
    print("descartes:", Counter(re.sub(r"\d+|\$\w+|r\w+", "", x)[:40] for x in skips.values()).most_common(10))
    (ROOT / f"build/small_report{suf}.json").write_text(json.dumps({"ok": [[hex(v), n] for v, n, _ in ok], "bad": [[str(x[0]), x[1]] for x in bad[:300]]}, indent=1))
    out = ["// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.", decl] + [render(c, True) for c in okc.values()]
    (ROOT / "src/auto").mkdir(parents=True, exist_ok=True); (ROOT / f"src/auto/small{suf}.cpp").write_text("\n".join(out))
    print(f"escrito src/auto/small{suf}.cpp")

main()

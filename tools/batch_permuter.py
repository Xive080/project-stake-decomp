#!/usr/bin/env python3
"""Driver batch: orquesta m2c + decomp-permuter sobre funciones NO triviales cortas de un
ELF PS2 (CodeWarrior). Por cada función: objdump del slice -> target.s (sintaxis GNU as) ->
m2c --valid-syntax -> base.c -> permuter busca el byte-match compilando con el mwccps2 real.

Pensado para correr DENTRO de la imagen airblade-decomp:latest (trae objdump mips, python,
cpp). El compilador (mwccps2) y wibo se pasan por ruta. Las triviales (getter/setter/const)
NO son el objetivo aquí: las cubre automatch.py; este driver ataca lo que necesita permuter.

Uso (dentro del contenedor):
  python3 batch_permuter.py --elf <elf> --m2c /tools/m2c --permuter /tools/decomp-permuter \
    --compiler /aw/compilers/mwcps2-2.4 --wibo /aw/wibo-i686 --out <dir_salida> \
    --n 25 --min-len 12 --max-len 60 --timeout 60 --jobs 4
"""
import argparse, re, struct, subprocess, os, shutil
from pathlib import Path

JR_RA = 0x03E00008
NOP   = 0x00000000
REGS = (r"zero|at|v[01]|a[0-3]|t[0-9]|s[0-7]|k[01]|gp|sp|fp|ra|"
        r"f[0-9]|f1[0-9]|f2[0-9]|f3[0-1]")
RE_REG = re.compile(r"(?<![\w$])(" + REGS + r")(?![\w])")

def u32(d, o): return struct.unpack_from("<I", d, o)[0]

def load_segments(p):
    d = Path(p).read_bytes(); assert d[:4] == b"\x7fELF", "no ELF"
    ph = u32(d, 0x1c); n = struct.unpack_from("<H", d, 0x2c)[0]; segs = []
    for i in range(n):
        o = ph + i*32; t, off, va, _, fs = struct.unpack_from("<5I", d, o)
        if t == 1 and fs: segs.append((va, off, fs))
    return d, segs

def find_functions(d, segs):
    """Devuelve [(vaddr, file_start, file_end)] por detección de entradas + jr ra."""
    out = []
    for va, off, fs in segs:
        lo, hi = off, off + fs
        starts = [fo for fo in range(off, hi-7, 4)
                  if (fo - lo) >= 8 and (fo & 7) == 0
                  and (u32(d, fo-4) == NOP or u32(d, fo-8) == JR_RA)]
        for i, s in enumerate(starts):
            nxt = starts[i+1] if i+1 < len(starts) else hi
            p = s; end = None
            while p < nxt:
                if u32(d, p) == JR_RA: end = p + 8; break
                p += 4
            if end and end <= nxt:
                out.append((va + (s - off), s, end))
    return out

def disasm(raw, objdump):
    """objdump de un slice crudo -> líneas de instrucción en sintaxis con $registros."""
    tmp = "/tmp/_slice.bin"; Path(tmp).write_bytes(raw)
    r = subprocess.run([objdump, "-b", "binary", "-m", "mips:4300", "-EL", "-D", tmp],
                       capture_output=True, text=True)
    lines = []
    for ln in r.stdout.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(\S+)\s*(.*)", ln)
        if not m: continue
        off, word, mn, ops = m.groups()
        ops = RE_REG.sub(r"$\1", ops.split("#")[0].strip())
        insn = mn + (" " + ops if ops else "")
        lines.append((int(off, 16), word, insn))
    return lines

def has_control_flow(lines):
    """Descarta branches y saltos absolutos (pierden destino en un slice crudo).
    Permite jr/jalr (retorno y llamada por registro), que m2c sí maneja."""
    for _, _, insn in lines:
        mn = insn.split()[0] if insn else ""
        if mn in ("j", "jal") or (mn.startswith("b") and mn != "break"):
            return True
    return False

def build_target_s(name, lines):
    body = "\n".join(f"/* {off:04X} {word} */\t{insn}" for off, word, insn in lines)
    return f".set noat\n.set noreorder\n\nglabel {name}\n{body}\n"

def run_m2c(m2c_dir, target_s_path):
    env = dict(os.environ, PYTHONPATH=m2c_dir)
    r = subprocess.run(["python3", f"{m2c_dir}/m2c.py", "--target", "mips-mwcc-c",
                        "--valid-syntax", str(target_s_path)],
                       capture_output=True, text=True, env=env)
    return r.stdout.strip()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--m2c", required=True)
    ap.add_argument("--permuter", required=True)
    ap.add_argument("--compiler", required=True)
    ap.add_argument("--wibo", required=True)
    ap.add_argument("--out", default="batch_out")
    ap.add_argument("--n", type=int, default=25)
    ap.add_argument("--min-len", type=int, default=12)
    ap.add_argument("--max-len", type=int, default=60)
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--jobs", type=int, default=4)
    args = ap.parse_args()

    out = Path(args.out); (out / "matched").mkdir(parents=True, exist_ok=True)
    # m2c_macros.h usa s8/s16/s32/s64/u.. pero NO los define: hay que precederlos.
    PRELUDE = (
        "typedef signed char s8; typedef unsigned char u8;\n"
        "typedef short s16; typedef unsigned short u16;\n"
        "typedef int s32; typedef unsigned int u32;\n"
        "typedef long long s64; typedef unsigned long long u64;\n"
        "typedef float f32; typedef double f64;\n")
    macros = PRELUDE + Path(f"{args.m2c}/m2c_macros.h").read_text()
    objdump = "mips-linux-gnu-objdump"
    as_bin  = "mips-linux-gnu-as"

    d, segs = load_segments(args.elf)
    funcs = find_functions(d, segs)
    # no triviales cortas: longitud en rango, sin control de flujo interno
    cands = []
    for va, s, e in funcs:
        n = e - s
        if n < args.min_len or n > args.max_len: continue
        lines = disasm(d[s:e], objdump)
        if not lines or has_control_flow(lines): continue
        cands.append((n, va, lines))
    cands.sort()
    cands = cands[:args.n]
    print(f"funciones no triviales candidatas (piloto): {len(cands)}")

    results = []
    def record(name, n, st, b):
        """Registra, imprime y reescribe el resumen incremental (robusto ante corte)."""
        results.append((name, n, st, b))
        print(f"  [{len(results)}/{len(cands)}] {name} len={n} base={b} -> {st}", flush=True)
        ok = sum(1 for _, _, s, _ in results if s == "MATCH_0")
        (out / "summary.txt").write_text(
            f"# progreso: {len(results)}/{len(cands)} procesadas, {ok} MATCH score 0\n" +
            "\n".join(f"{nm}\tlen={ln}\t{s}\tbase={bb}" for nm, ln, s, bb in results))

    compile_sh = (f"#!/bin/bash\nexport MWCIncludes={args.compiler}\nulimit -c 0 2>/dev/null\n"
                  f'{args.wibo} {args.compiler}/mwccps2.exe -O4,p -c "$1" -o "$3"\n')
    for n, va, lines in cands:
        name = f"func_{va:08X}"
        wd = out / name; wd.mkdir(exist_ok=True)
        tgt_s = wd / "target.s"; tgt_s.write_text(build_target_s(name, lines))
        c = run_m2c(args.m2c, tgt_s)
        if not c or "?" in c.split("{")[0]:
            record(name, n, "m2c_fail", None); continue
        # m2c marca con M2C_ERROR lo que no puede decompilar (lee registros 'unset',
        # p.ej. arg no-estándar o límite de función mal detectado): no es viable ->
        # saltar en vez de malgastar una ronda de permuter con ERR garantizado.
        if "M2C_ERROR" in c or "unset register" in c:
            record(name, n, "m2c_unviable", None); continue
        (wd / "base.c").write_text(macros + "\n" + c + "\n")
        (wd / "compile.sh").write_text(compile_sh); os.chmod(wd / "compile.sh", 0o755)
        # 'glabel' es una macro del mundo decomp que m2c entiende pero GNU as no:
        # la definimos solo para ensamblar target.o (m2c lee el target.s original).
        as_src = wd / "_target_as.s"
        as_src.write_text(".macro glabel label\n.global \\label\n\\label:\n.endm\n"
                          + tgt_s.read_text())
        ar = subprocess.run([as_bin, "-EL", "-march=mips3", "-mabi=32",
                             str(as_src), "-o", str(wd / "target.o")], capture_output=True, text=True)
        if not (wd / "target.o").exists():
            record(name, n, "as_fail", ar.stderr.strip()[:50]); continue
        # Gate general: si base.c no compila con mwccps2, permuter no tiene base ->
        # saltar ya (rápido), en vez de arrancar permuter para que falle igual.
        chk = wd / "_chk.o"
        subprocess.run(["bash", str(wd / "compile.sh"), str(wd / "base.c"),
                        "-o", str(chk)], capture_output=True)
        if not chk.exists():
            record(name, n, "base_no_compile", None); continue
        chk.unlink(missing_ok=True)
        r = subprocess.run(["timeout", str(args.timeout), "python3",
                            f"{args.permuter}/permuter.py", ".", "--stop-on-zero",
                            "-j", str(args.jobs)],
                           cwd=str(wd), capture_output=True, text=True,
                           env=dict(os.environ, PYTHONPATH=args.permuter))
        allout = r.stdout + "\n" + r.stderr
        m = re.search(r"base score = (\d+)", allout)
        if m:
            base = m.group(1)
        elif "Traceback" in allout or "Error" in allout:
            base = "ERR:" + allout.strip().splitlines()[-1][:60]
        else:
            base = "?"
        zero = list(wd.glob("output*"))
        if base == "0":
            # el C de m2c ya compila byte-exacto sin permutar: base.c ES el match
            shutil.copy(wd / "base.c", out / "matched" / f"{name}.c")
            record(name, n, "MATCH_0", base)
        elif zero:
            src = next(zero[0].glob("*source*"), None) or next(zero[0].glob("*.c"), None)
            if src: shutil.copy(src, out / "matched" / f"{name}.c")
            record(name, n, "MATCH_0", base)
        else:
            record(name, n, "best>0", base)

    ok = sum(1 for _, _, st, _ in results if st == "MATCH_0")
    print("=" * 40)
    print(f" MATCH score 0 : {ok} / {len(results)}")
    print(f" resumen en {out}/summary.txt ; C matcheado en {out}/matched/")
    print("=" * 40)

if __name__ == "__main__":
    main()

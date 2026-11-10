#!/usr/bin/env python3
"""Prepara y lanza decomp-permuter contra el ELF con ee-gcc dentro de Docker (imagen vn-permuter).
Uso: ee_permuter.py FUNC [FUNC...] [--cc ee-gcc2.9-990721] [--flags "-O2 -G8"] [--secs 120] [-j 4]
Necesita tools/perm_bases/FUNC.c (C a mano; el nombre de la funcion C = FUNC). Sin rutas absolutas."""
import argparse, re, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
ap = argparse.ArgumentParser(); ap.add_argument("funcs", nargs="+")
ap.add_argument("--cc", default="ee-gcc2.9-990721"); ap.add_argument("--flags", default="-O2 -G8")
ap.add_argument("--secs", type=int, default=120); ap.add_argument("-j", type=int, default=4)
a = ap.parse_args()
ccdir = ROOT / "tools/compilers" / a.cc
if not ccdir.exists(): ccdir = ROOT / "tools/compilers/ps2_compilers" / a.cc
def sh(cmd, **k): return subprocess.run(cmd, capture_output=True, text=True, **k)
def dock(args, extra=()):
    return ["docker", "run", "--rm", "--platform", "linux/386", "-v", f"{ROOT}:/repo", "-v", f"{ccdir}:/cc:ro", *extra, "vn-permuter", *args]
for fn in a.funcs:
    d = ROOT / "build/perm" / fn; d.mkdir(parents=True, exist_ok=True)
    asm = sh([sys.executable, str(ROOT / "tools/dis_func.py"), fn]); 
    if asm.returncode: print(fn, "dis_func:", asm.stderr.strip() or asm.stdout.strip()); continue
    syms = set(); lines = []
    for l in asm.stdout.splitlines():
        if l.startswith("glabel"): continue
        m = re.search(r"(-?)0x([0-9a-f]+)\(\$gp\)", l)
        if m:
            s = "g_" + m.group(2); syms.add(s); l = l.replace(m.group(0), f"%gp_rel({s})($gp)")
        lines.append(l)
    (d / "target.s").write_text(".set noreorder\n.set noat\n" + "".join(f".extern {s}, 4\n" for s in sorted(syms)) +
                                f".text\n.globl {fn}\n.ent {fn}\n{fn}:\n" + "\n".join(lines) + f"\n.end {fn}\n")
    (d / "base.c").write_text((ROOT / "tools/perm_bases" / f"{fn}.c").read_text())
    (d / "compile.sh").write_text(f'#!/bin/sh\n/cc/bin/ee-gcc -c {a.flags} -x c "$@"\n'); (d / "compile.sh").chmod(0o755)
    (d / "settings.toml").write_text(f'func_name = "{fn}"\ncompiler_type = "gcc"\nobjdump_command = "mipsel-linux-gnu-objdump -drz"\n')
    r = sh(dock(["mipsel-linux-gnu-as", "-march=r5900", "-EL", "-o", f"/repo/build/perm/{fn}/target.o", f"/repo/build/perm/{fn}/target.s"]))
    if r.returncode: print(fn, "as:", r.stderr.strip()[:300]); continue
    print(f"== {fn}")
    r = sh(dock(["sh", "-c", f"cd /opt/permuter && timeout {a.secs} python3 permuter.py /repo/build/perm/{fn} -j {a.j} --stop-on-zero > /repo/build/perm/{fn}/log.txt 2>&1; grep -aiE 'base score|new best|zero|wrote' /repo/build/perm/{fn}/log.txt | grep -av iteration | head -8; grep -a iteration /repo/build/perm/{fn}/log.txt | tail -1"]))
    print(r.stdout.strip() or r.stderr.strip())

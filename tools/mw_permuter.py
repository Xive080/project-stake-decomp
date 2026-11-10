#!/usr/bin/env python3
"""Lanza decomp-permuter (tools/decomp-permuter, en el host) contra una funcion de overlay/main compilada con
mwcps2-2.4-001213 (wibo). Entrada: C de un casi-acierto volcado por `vn_small.py --dump-near`
(build/perm_src/<mod>/f_XXXXXXXX_tag.c + .json con los bytes del ELF/disco).
Uso: mw_permuter.py build/perm_src/gamemod/f_0092C7F0_rh2.c [--secs 120] [-j 4]
El objdump es tools/mwobjdump.py (capstone), el compilador compile.sh -> wibo mwccps2 -lang c -O4,p."""
import argparse, json, os, shutil, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
ap = argparse.ArgumentParser(); ap.add_argument("src", nargs="+"); ap.add_argument("--secs", type=int, default=120)
ap.add_argument("-j", type=int, default=4); ap.add_argument("--flags", default="-O4,p"); a = ap.parse_args()
CC = ROOT / "tools/compilers/mwcps2-2.4-001213"
for sp in a.src:
    sp = Path(sp); name = sp.stem; d = ROOT / "build/perm" / name; d.mkdir(parents=True, exist_ok=True)
    (d / "base.c").write_text("\n".join(l for l in sp.read_text().splitlines() if not l.lstrip().startswith("//")) + "\n"); shutil.copy(sp.with_suffix(".json"), d / "target.json")
    (d / "compile.sh").write_text(f'''#!/bin/bash
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT/tools/compilers/mwcps2-2.4-001213" && "$ROOT/tools/bin/wibo" mwccps2.exe -lang c {a.flags} -c -o "$3" "$1" >/dev/null 2>&1
''')
    (d / "compile.sh").chmod(0o755)
    (d / "settings.toml").write_text(f'func_name = "{name}"\ncompiler_type = "gcc"\nobjdump_command = "{ROOT}/.venv/bin/python {ROOT}/tools/mwobjdump.py"\n')
    # target.o: un ELF .o valido cualquiera (el shim usa target.json)
    r = subprocess.run(["bash", str(d / "compile.sh"), str(d / "base.c"), "-o", str(d / "target.o")])
    if not (d / "target.o").exists(): print(name, "base.c no compila"); continue
    env = dict(os.environ, PYTHONPATH="")
    sc = subprocess.run([str(ROOT / ".venv/bin/python"), str(ROOT / "tools/mwobjdump.py"), str(d / "target.o")], capture_output=True, text=True)
    print(f"== {name} (base.c compila; objdump shim rc={sc.returncode})")
    import signal, time, struct
    sys.path.insert(0, str(ROOT / "tools")); import probe_compilers as P
    cmd = [str(ROOT / ".venv/bin/python"), "-u", "permuter.py", str(d), "-j", str(a.j), "--stop-on-zero"]
    lf = open(d / "log.txt", "w")
    pr = subprocess.Popen(cmd, cwd=ROOT / "tools/decomp-permuter", stdout=lf, stderr=subprocess.STDOUT, start_new_session=True)
    t0 = time.time()
    try:
        while time.time() - t0 < a.secs and pr.poll() is None: time.sleep(1)
    finally:
        try: os.killpg(pr.pid, signal.SIGTERM)
        except ProcessLookupError: pass
    lf.close()
    tgt = bytes.fromhex(json.loads((d / "target.json").read_text())["hex"])
    outs = sorted(d.glob("output-*"), key=lambda q: (int(q.name.split("-")[1]), q.name))
    best = outs[0].name if outs else "(sin mejoras)"
    verified = None
    for q in outs[:8]:
        src = q / "source.c"
        if not src.exists(): continue
        o = d / "verify.o"; o.unlink(missing_ok=True)
        subprocess.run(["bash", str(d / "compile.sh"), str(src), "-o", str(o)])
        if not o.exists(): continue
        fs = P.funcs_of(o)
        if fs and P.trim(fs[0][1]) == P.mask(P.trim(tgt), 0, fs[0][2]): verified = src; break
    print(f"   mejor: {best}; iteraciones: {sum(1 for _ in open(d / 'log.txt', errors='ignore').read().split('iteration')) - 1}")
    if verified:
        dst = ROOT / "src/matched_perm"; dst.mkdir(parents=True, exist_ok=True)
        shutil.copy(verified, dst / f"{name}.c"); print(f"   BYTE-MATCH verificado -> src/matched_perm/{name}.c")
    else: print("   sin byte-match")

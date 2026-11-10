#!/usr/bin/env python3
"""Desensambla una funcion del ELF a texto estilo GNU as (para m2c). Uso: dis_func.py NOMBRE|0xADDR [bytes]"""
import re, subprocess, sys
from pathlib import Path
import capstone
ROOT = Path(__file__).resolve().parent.parent
ELFP = ROOT / "baserom/SCES_504.11"; D = ELFP.read_bytes(); BASE, FOFF = 0x140000, 0x800
syms = []
for l in subprocess.check_output(["nm", "-n", str(ELFP)], text=True).splitlines():
    p = l.split()
    if len(p) == 3 and p[1] in "Tt": syms.append((int(p[0], 16), p[2]))
arg = sys.argv[1]
if arg.startswith("0x"): a = int(arg, 16); name = next((n for v, n in syms if v == a), f"func_{a:X}")
else:
    a, name = next((v, n) for v, n in syms if n == arg or n.split("__")[0] == arg)
nxt = min(v for v, _ in syms if v > a)
size = int(sys.argv[2], 0) if len(sys.argv) > 2 else nxt - a
off = a - BASE + FOFF
md = capstone.Cs(capstone.CS_ARCH_MIPS, capstone.CS_MODE_MIPS64 + capstone.CS_MODE_LITTLE_ENDIAN); md.detail = False
ins = list(md.disasm(D[off:off + size], a))
while ins and ins[-1].mnemonic == "nop": ins.pop()
# mantener el delay slot de un jr final
if ins and ins[-1].mnemonic == "jr": ins += list(md.disasm(D[off + (ins[-1].address - a) + 4:off + (ins[-1].address - a) + 8], ins[-1].address + 4))
if len(ins) * 4 < min(size, (ins[-1].address + 4 - a)) : sys.exit(f"[!] desensamblado incompleto en {name} ({len(ins)*4} de {size} bytes): instruccion R5900 no soportada")
tg = {int(m.group(0), 16) for i in ins for m in [re.search(r"0x[0-9a-f]+$", i.op_str)] if m and i.mnemonic.startswith(("b", "j")) and i.mnemonic not in ("jr", "jalr") and a <= int(m.group(0), 16) < nxt}
print(f"glabel {name}")
for i in ins:
    if i.address in tg: print(f".L{i.address:X}:")
    ops = re.sub(r"0x[0-9a-f]+$", lambda m: f".L{int(m.group(0),16):X}" if int(m.group(0), 16) in tg and i.mnemonic.startswith("b") else m.group(0), i.op_str) if i.mnemonic.startswith("b") else i.op_str
    print(f"    {i.mnemonic} {ops}".rstrip())

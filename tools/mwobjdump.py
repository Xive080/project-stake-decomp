#!/usr/bin/env python3
"""Sustituto de `objdump -drz` para decomp-permuter con codigo MIPS R5900 de mwcc.
- Fichero .o normal (candidato): lee la funcion con probe_compilers.funcs_of, enmascara sus relocaciones.
- `target.o`: usa el `target.json` de su carpeta ({"hex": bytes de la funcion en el ELF}) y enmascara
  por dataflow lo que seria relocacion (lui/addiu/lw con direccion de simbolo, gp-relativo, jal).
Imprime el mismo formato que GNU objdump (direccion, bytes, mnemonico, operandos) sin lineas de reloc."""
import json, re, struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import capstone
import probe_compilers as P

md = capstone.Cs(capstone.CS_ARCH_MIPS, capstone.CS_MODE_MIPS64 + capstone.CS_MODE_LITTLE_ENDIAN)
USERS_IMM = {0x09, 0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2b, 0x37, 0x3f, 0x31, 0x39, 0x1e, 0x1f}  # addiu, loads/stores

def mask_target(b):
    w = list(struct.unpack("<%dI" % (len(b) // 4), b[: len(b) // 4 * 4]))
    out = list(w)
    for i, x in enumerate(w):
        op = x >> 26
        if op in (0x02, 0x03): out[i] = x & 0xfc000000; continue            # j / jal
        if op in USERS_IMM and ((x >> 21) & 31) == 28: out[i] = x & 0xffff0000; continue   # base $gp
        if op != 0x0f: continue
        rt = (x >> 16) & 31; users = []; sym = False
        for j in range(i + 1, min(i + 40, len(w))):
            y = w[j]; oy = y >> 26
            base = (y >> 21) & 31
            if oy in USERS_IMM and base == rt: users.append(j); sym = True
            elif oy == 0x0d and base == rt and ((y >> 16) & 31) == rt: break          # ori sobre el mismo reg: constante
            elif oy == 0x0d and base == rt: users.append(("c", j))
            if oy in (0x09, 0x0d, 0x0f) and ((y >> 16) & 31) == rt and base != rt and j > i: break
            if oy in (0x20, 0x21, 0x23, 0x24, 0x25) and ((y >> 16) & 31) == rt and base != rt: break
        if sym and not any(isinstance(u, tuple) for u in users):
            out[i] = x & 0xffff0000
            for u in users: out[u] = w[u] & 0xffff0000
    return struct.pack("<%dI" % len(out), *out) + b[len(out) * 4:]

def main():
    f = Path(sys.argv[-1])
    if f.name == "target.o":
        b = bytes.fromhex(json.loads((f.parent / "target.json").read_text())["hex"]); b = P.trim(mask_target(b))
    else:
        fs = P.funcs_of(f)
        if not fs: sys.exit("sin funciones")
        n, b, rm = fs[0]; b = P.trim(P.mask(b, 0, rm)) if rm is not None else P.trim(b)
    print(f"{f}:\tfile format elf32-tradlittlemips\n\nDisassembly of section .text:\n\n00000000 <f>:")
    for k in range(0, len(b) - 3, 4):
        word = struct.unpack_from("<I", b, k)[0]
        ins = list(md.disasm(b[k:k + 4], k))
        if ins:
            i = ins[0]; ops = re.sub(r"\$", "", i.op_str).replace(", ", ",")
            ops = re.sub(r"(-?)0x([0-9a-f]+)\(", lambda m: f"{m.group(1)}{int(m.group(2), 16)}(", ops)
            ops = re.sub(r"(^|,)\(", r"\g<1>0(", ops)
            mn = i.mnemonic; text = f"{mn}\t{ops}" if ops else mn
        else: text = f".word\t0x{word:08x}"
        print(f"{k:4x}:\t{word:08x} \t{text}")
main()

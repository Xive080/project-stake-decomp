#!/usr/bin/env python3
"""Auto-matcher de PS2 (CodeWarrior): barre el ELF, reconoce funciones triviales por
patrón, genera el C equivalente y un manifiesto con los bytes originales esperados.
Luego tools/verify_batch.sh compila cada C con mwccps2 y confirma el byte-match.

Filosofía: generar candidatos con match GARANTIZADO o muy probable (getters/setters/
const), que son cientos en cualquier binario, y quedarnos SOLO con los que verifican al
100%. Lo no reconocido queda para m2c + revisión. Sin rutas absolutas: todo relativo o
por argumento. Multi-segmento LOAD: cada vaddr se resuelve a su segmento real.
"""
import argparse, json, struct
from pathlib import Path

JR_RA = 0x03E00008
NOP   = 0x00000000

# --- loads: jr ra ; <load> v0, off(a0)  ->  getter(p) ---
LOADS = {
    0x8C820000: ("int",            "int"),            # lw   v0,off(a0)
    0x96820000: ("unsigned short", "unsigned short"), # lhu  v0,off(a0)
    0x90820000: ("unsigned char",  "unsigned char"),  # lbu  v0,off(a0)
    0x86820000: ("short",          "short"),          # lh   v0,off(a0)
    0x80820000: ("signed char",    "signed char"),    # lb   v0,off(a0)
}
# --- stores: jr ra ; <store> a1, off(a0)  ->  setter(p,v) ---
STORES = {
    0xACA50000: "int",   # sw a1,off(a0)
    0xA6A50000: "short", # sh a1,off(a0)
    0xA2A50000: "char",  # sb a1,off(a0)
}

def u32(d, o): return struct.unpack_from("<I", d, o)[0]
def soff(w):
    o = w & 0xFFFF
    return o - 0x10000 if o & 0x8000 else o

def load_segments(elf_path):
    """Devuelve (datos, [(vaddr, off, filesz), ...]) de todos los LOAD con contenido."""
    d = elf_path.read_bytes()
    assert d[:4] == b"\x7fELF", "no es ELF"
    e_phoff = u32(d, 0x1c); e_phnum = struct.unpack_from("<H", d, 0x2c)[0]
    segs = []
    for i in range(e_phnum):
        o = e_phoff + i*32
        p_type, p_off, p_vaddr, _, p_filesz = struct.unpack_from("<5I", d, o)
        if p_type == 1 and p_filesz:
            segs.append((p_vaddr, p_off, p_filesz))
    if not segs:
        raise SystemExit("sin segmentos LOAD")
    return d, segs

def is_entry(d, fo, lo):
    """fo es inicio probable de función: alineado a 8, y precedido por padding nop o
    por el delay-slot de un jr ra anterior. lo = offset mínimo del segmento."""
    if fo - lo < 8 or (fo & 7): return False
    return u32(d, fo-4) == NOP or u32(d, fo-8) == JR_RA

def classify(d, fo):
    """Reconoce funciones triviales de 8 bytes: jr ra + 1 instrucción útil en delay slot."""
    w0, w1 = u32(d, fo), u32(d, fo+4)
    if w0 != JR_RA:
        return None
    base = w1 & 0xFFFF0000
    off  = soff(w1)
    # getter: jr ra ; <load> v0, off(a0)
    if base in LOADS:
        ret_t, cast_t = LOADS[base]
        return ("getter", f"{ret_t} func(int p)", f"return *({cast_t} *)(p + {off:#x});")
    # setter: jr ra ; <store> a1, off(a0)
    if base in STORES:
        ty = STORES[base]
        return ("setter", f"void func(int p, {ty} v)", f"*({ty} *)(p + {off:#x}) = v;")
    # return const con signo: jr ra ; addiu v0, zero, imm
    if base == 0x24020000:
        return ("const", "int func(void)", f"return {off};")
    # return const sin signo pequeño: jr ra ; ori v0, zero, imm
    if base == 0x34020000:
        return ("const", "unsigned int func(void)", f"return {w1 & 0xFFFF};")
    # return 0: jr ra ; move v0,zero  (addu/or v0,zero,zero)
    if w1 in (0x00001021, 0x00001025):
        return ("const", "int func(void)", "return 0;")
    return None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--out", default="auto", help="carpeta de salida (relativa)")
    ap.add_argument("--name", default="PS2", help="nombre del juego (solo comentario)")
    args = ap.parse_args()

    elf = Path(args.elf).expanduser().resolve()
    out = Path(args.out); out.mkdir(parents=True, exist_ok=True)
    d, segs = load_segments(elf)

    manifest = []
    for vaddr, foff, fsz in segs:
        lo, hi = foff, foff + fsz
        fo = foff
        while fo + 8 <= hi:
            if is_entry(d, fo, lo):
                r = classify(d, fo)
                if r:
                    kind, sig, body = r
                    va = vaddr + (fo - foff)
                    name = f"f_{va:08x}"
                    hexs = " ".join("%02x" % b for b in d[fo:fo+8])
                    (out / f"{name}.c").write_text(
                        f"/* {args.name} {name} @ {va:#010x}  [{kind}]  esperado: {hexs} */\n"
                        f"{sig.replace('func', name)}{{ {body} }}\n")
                    manifest.append({"name": name, "vaddr": va, "kind": kind,
                                     "bytes": hexs, "cfile": str(out / f"{name}.c")})
            fo += 4
    (out / "manifest.json").write_text(json.dumps(manifest, indent=1))
    by_kind = {}
    for m in manifest: by_kind[m["kind"]] = by_kind.get(m["kind"], 0) + 1
    print(f"candidatas reconocidas: {len(manifest)}  {by_kind}")
    print(f"C en {out}/ , manifiesto en {out}/manifest.json")

if __name__ == "__main__":
    main()

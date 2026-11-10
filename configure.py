#!/usr/bin/env python3
"""Configure the Vampire Night decomp build.

No absolute paths: everything is resolved relative to this file at runtime so the
project is portable across machines and OSes. The user supplies the original ELF
(from their own disc); we never ship it.
"""
import argparse
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent

# Verification target: the original Vampire Night PAL executable.
EXPECTED = {
    "name": "SCES_504.11",
    "size": 3198676,
    "sha1": "7bf30bab9a30b7ece6e5357ffffded97519dbabc",
    "sha256": "8ef08196c0abc19514144207e5acbea31768dfeceb5ddd12031ad7c1c8290e1c",
}


def sha(path: Path, algo: str) -> str:
    h = hashlib.new(algo)
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def verify_elf(elf: Path) -> bool:
    if not elf.is_file():
        print(f"[!] No existe: {elf}")
        return False
    size = elf.stat().st_size
    if size != EXPECTED["size"]:
        print(f"[!] Tamaño {size} != esperado {EXPECTED['size']}")
        return False
    got1 = sha(elf, "sha1")
    if got1 != EXPECTED["sha1"]:
        print(f"[!] SHA1 {got1}\n    != esperado {EXPECTED['sha1']}")
        print("    Este no es el ELF PAL SCES-50411 sin modificar.")
        return False
    print("[ok] ELF verificado: Vampire Night PAL SCES-50411 original.")
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description="Configure Vampire Night decomp build")
    ap.add_argument("--elf", required=True,
                    help="Ruta al SCES_504.11 extraído de tu disco")
    ap.add_argument("--check", action="store_true",
                    help="Solo verificar el ELF y salir")
    args = ap.parse_args()

    elf = Path(args.elf).expanduser().resolve()
    if not verify_elf(elf):
        return 1
    if args.check:
        return 0

    # Place/symlink the verified ELF where splat expects it (relative to ROOT).
    target = ROOT / "config" / "SCES_504.11"
    try:
        if target.exists() or target.is_symlink():
            target.unlink()
        target.symlink_to(elf)
        print(f"[ok] Enlazado {target.relative_to(ROOT)} -> {elf}")
    except OSError as e:
        print(f"[!] No se pudo enlazar ({e}); cópialo a {target} a mano.")
        return 1

    print("\nSiguiente paso:")
    print("  pip install splat64 spimdisasm   # desensamblado")
    print("  splat split config/splat.yaml    # trocea el ELF en asm/ y data/")
    return 0


if __name__ == "__main__":
    sys.exit(main())

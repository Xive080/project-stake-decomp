#!/usr/bin/env bash
# Reproduce el primer byte-match de AirBlade en un Linux x86_64 NATIVO.
# No necesita el ISO: trae el compilador (decomp.me) y wibo él solo, compila las
# funciones candidatas y compara con los bytes originales (hardcodeados).
# En x86_64 real wibo-x86_64 funciona sin el segfault de la emulación ARM->qemu.
set -euo pipefail

WORK="${1:-$HOME/airblade-match}"
mkdir -p "$WORK"/{comp,out}
cd "$WORK"

echo "== 1. wibo (loader PE nativo x86_64) =="
if [ ! -x wibo ]; then
  curl -sL "https://github.com/decompals/wibo/releases/download/1.2.0/wibo-x86_64" -o wibo
  chmod +x wibo
fi

echo "== 2. compilador mwccps2 2.4.1.01 (decomp.me) =="
if [ ! -f comp/mwccps2.exe ]; then
  curl -sL "https://github.com/decompme/compilers/releases/download/compilers/mwcps2-2.4-001213.tar.gz" -o comp/c.tgz
  tar -xzf comp/c.tgz -C comp
fi

echo "== 3. funciones candidatas =="
cat > fn.c <<'EOF'
int func(int param_1) { return **(int **)(param_1 + 0x40); }
EOF

# Bytes originales (ground truth, del ELF SCES_504.11):
#   FUN_00107f20: lw v1,0x40(a0) ; jr ra ; lw v0,0x0(v1)
ORIG="40 00 83 8c 08 00 e0 03 00 00 62 8c"

echo "== 4. extractor de .text =="
cat > extract_text.py <<'PY'
import sys, struct
f=open(sys.argv[1],"rb").read(); end="<"
e_shoff,=struct.unpack_from(end+"I",f,0x20)
e_shentsize,e_shnum,e_shstrndx=struct.unpack_from(end+"HHH",f,0x2e)
def sh(i):
    o=e_shoff+i*e_shentsize
    name,=struct.unpack_from(end+"I",f,o)
    off,=struct.unpack_from(end+"I",f,o+0x10); size,=struct.unpack_from(end+"I",f,o+0x14)
    return name,off,size
snoff=sh(e_shstrndx)[1]
def nm(n):
    e=f.index(b"\0",snoff+n); return f[snoff+n:e].decode("latin1")
for i in range(e_shnum):
    name,off,size=sh(i)
    if nm(name)==".text":
        print(" ".join("%02x"%b for b in f[off:off+size])); sys.exit(0)
sys.exit(1)
PY

echo "== 5. compilar y comparar (varios flags) =="
export MWCIncludes="$WORK/comp"
ulimit -c 0 || true
printf "ORIGINAL:  %s\n" "$ORIG"
for F in "-O4,p" "-O4" "-O4,s" "-O2"; do
  ./wibo comp/mwccps2.exe $F -c fn.c -o out/fn.o 2>/dev/null || true
  GOT=$(python3 extract_text.py out/fn.o 2>/dev/null || echo "(err)")
  MARK="DIFIERE"; [ "$GOT" = "$ORIG" ] && MARK="== MATCH EXACTO =="
  printf "%-8s %s   %s\n" "$F" "$GOT" "$MARK"
done
echo "Listo. (En Mac ARM usar wibo-i686 en vez de wibo-x86_64.)"

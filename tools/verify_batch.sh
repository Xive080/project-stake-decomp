#!/usr/bin/env bash
# Compila cada C de auto/ con mwccps2 y verifica el byte-match contra el manifiesto.
# Pensado para correr DENTRO del contenedor (ubuntu:24.04 + libc6:i386) con wibo-i686.
# Uso: verify_batch.sh <dir_auto> <dir_compilador> <wibo> <extract_text.py>
set -u
AUTO="${1:?dir auto}"; COMP="${2:?dir compilador}"; WIBO="${3:?wibo}"; EXTRACT="${4:?extract_text.py}"
export MWCIncludes="$COMP"
ulimit -c 0 2>/dev/null || true

ok=0; bad=0; failc=0
while IFS= read -r line; do
  name=$(echo "$line" | python3 -c 'import sys,json;print(json.loads(sys.stdin.read())["name"])' 2>/dev/null) || continue
  exp=$(echo "$line"  | python3 -c 'import sys,json;print(json.loads(sys.stdin.read())["bytes"])')
  cf="$AUTO/$name.c"
  "$WIBO" "$COMP/mwccps2.exe" -O4,p -c "$cf" -o "/tmp/$name.o" 2>/dev/null
  if [ ! -f "/tmp/$name.o" ]; then failc=$((failc+1)); continue; fi
  got=$(python3 "$EXTRACT" "/tmp/$name.o" 2>/dev/null)
  if [ "$got" = "$exp" ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "DIFIERE $name: exp[$exp] got[$got]"; fi
done < <(python3 -c 'import json,sys;[print(json.dumps(m)) for m in json.load(open(sys.argv[1]))]' "$AUTO/manifest.json")

rm -f core qemu_wibo_*.core 2>/dev/null
total=$((ok+bad+failc))
echo "========================================"
echo " MATCH EXACTO : $ok / $total"
echo " difieren     : $bad"
echo " no compilan  : $failc"
echo "========================================"

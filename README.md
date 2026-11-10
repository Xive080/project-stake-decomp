# Vampire Night Decomp

Decompilación del juego de PS2 **Vampire Night** (PAL, `SCES-50411`) con dos objetivos:
byte-match del código propio y, a partir de ahí, un port nativo.

## El binario

| Dato | Valor |
|---|---|
| ELF | `SCES_504.11`, 3 198 676 bytes |
| SHA1 | `7bf30bab9a30b7ece6e5357ffffded97519dbabc` |
| SHA256 | `8ef08196c0abc19514144207e5acbea31768dfeceb5ddd12031ad7c1c8290e1c` |
| Arch | MIPS-III LE, estático, **con símbolos** (22 429; ~7 500 métodos C++ mangleados) |
| Secciones | 49; overlays con `.rel*` (gamemod, hntmod, optmod, bs2mod…bs6mod, shpmod, aomod, acmod, orgmod, trnmod) y `.mwcats` (CodeWarrior) |
| Entry / carga | `0x00140008`; segmento principal en `0x00140000` (filesz `0x198a80`) más 14 segmentos de overlay |
| Compilador | **Metrowerks CodeWarrior MIPS 2.4.1.01** (`.comment` del ELF: `MW MIPS C Compiler (2.4.1.01)`), confirmado por byte-match con `mwcps2-2.4-001213`, flags `-lang c++ -O4,p` |
| Librerías | CRI ADX/SFA/CVFS/SRD (jul–sep 2001), libpad/libmc de Sony |

## Compilador

`mwcps2-2.4-001213` (de `decompme/compilers`) en `tools/compilers/`, ejecutado con `tools/bin/wibo`
(binario macOS de decompals/wibo; ignorados por git). Los `ee-gcc` también descargados NO son el
compilador del juego (el `gcc2_compiled.` del ELF es solo un símbolo marcador de CodeWarrior).

    CC_ONLY=mwcps2 python3 tools/probe_compilers.py src/matched/camera_bg.cpp -O4,p

## Uso

    python3 configure.py --elf baserom/SCES_504.11 --check

No se incluye el ELF ni datos del juego: hay que aportar una copia propia.

## Permuter con mwcc

`decomp-permuter` (clonado en `tools/decomp-permuter`, no versionado) funciona en el host con mwcps2 vía wibo:

```
.venv/bin/python tools/vn_small.py --module gamemod --max-instr 40 --dump-near   # vuelca build/perm_src/<mod>/
.venv/bin/python tools/mw_permuter.py build/perm_src/gamemod/f_XXXXXXXX_r.c --secs 120 -j 6
```

`tools/mwobjdump.py` sustituye a `objdump` (capstone; enmascara relocaciones del candidato y deduce las del objetivo por dataflow).
Los byte-matches verificados se copian a `src/matched_perm/`.

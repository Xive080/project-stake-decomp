# Emulador como referencia (PCSX2) - 2026-11-09

Objetivo: ver el comportamiento real y leer la RAM del juego para contrastar lo decodificado (sin esto todo lo de game/ solo se prueba contra mi lectura del asm).

## Montaje (macOS arm64)
- PCSX2 v2.9.168 macOS (solo x86_64, va por Rosetta) en `~/emu/pcsx2/`; BIOS PAL del usuario en `~/emu/bios/` copiada a `~/Library/Application Support/PCSX2/bios/`; `PCSX2.ini` con `SetupWizardIncomplete = false`, BIOS `PS2 Bios 30004R V6 Pal.bin`, renderer Metal.
- Lanzar: `arch -x86_64 ~/emu/pcsx2/PCSX2-v2.9.168.app/Contents/MacOS/PCSX2 -batch -fastboot -- "<ISO>"`. Arranca solo y entra en la demo de la fase 1 (~1 min). CRC del juego 91510857.
- Estado guardado: `osascript` con System Events, ventana al frente + `key code 122` (F1) => `~/Library/Application Support/PCSX2/sstates/SCES-50411 (91510857).01.p2s`.
- Captura SOLO de la ventana del emulador (no del escritorio): posicion/tamano con `osascript ... get {position, size} of window 1` y `screencapture -x -R x,y,w,h`.

## Leer la RAM
- `tools/p2s_extract.py FILE.p2s OUTDIR eeMemory.bin GS.bin` (el .p2s es un zip con metodos stored y zstd; `tar`/`unzip`/`zipfile` no sirven; hace falta el modulo `zstandard`, ya en `.venv`). `eeMemory.bin` = RAM EE de 32 MB, direccion virtual = offset. `GS.bin` = VRAM del GS.

## Comprobado contra el juego real
| Hecho | Resultado |
|---|---|
| Codigo de `main` en RAM == ELF | 64 552 de 65 536 bloques de 16 B iguales (el resto = datos/parches) |
| Modo de juego global 0x500F14 | `0xFFFF000A`: bajo `0xA` = arcade normal, como se deduce del asm |
| Jugador 0 (0x500730 + offsets de game/src/player.h) | modo 5, flags 1, puntos 830, vida 3/5, balas 8/8, 43 disparos, 18 aciertos: coherente con lo visto en pantalla |
| gp del ELF (.reginfo) = 0x2E00F0 | `gp-0x73D8` (byte) = 0 => `PutForEntry` usa `PutForEntry_st1` (tabla fija) en ST1, no `PutForEntry_sub` (resuelve la duda abierta de docs/LEVELS.md) |
| Rutas de camara residentes (buscando los ficheros de ST1 en RAM) | solo la ruta 4 (131 716 B, 0xA355C0) y la 13 (objetos, 0xAFFD00); la ruta 1 NO esta: la exportacion de camara con la ruta 1 era la equivocada para esta escena |
| Camara viva | objeto en 0xA20320 (puntero en gp-0x7338): matriz 3x3 por filas en +0x10/+0x20/+0x30 y posicion en +0x40 = (2.28, -266.1, 401.0) |

## Resuelto con la RAM viva (2026-11-09)
- **La matriz de 0xA20320 es la matriz de VISTA, no la posicion**: posicion de camara = `-R t` con R = filas +0x10/+0x20/+0x30 y t = fila +0x40. Con la captura de la demo da (-12.8, 21.5, -480.7), a 1.2 unidades de la **ruta 4, camara 23, cuadro ~94** de ST1 (pos (-12.5, 20.8, -481.6), destino (-14.1, -16.2, -544.2), fov 42). Las rutas y los ejes de `afs_campath` son, pues, los del juego (sin invertir nada). El `forward` de la ruta coincide con la fila 2 de la matriz salvo el signo de z (convencion de vista de la matriz).
- **Estado de fondo vivo**: objeto BGCtrl en 0xA265D0 (+8 = estado actual = 2, +0xA = anterior = 1, +0xC/+0x10 = punteros 0x261620/0x261760 de la fase 0).
- **Que dibuja el juego en ST1**: `PutForEntry_st1` con la tabla fija de 12 entradas de 0x1C B en ELF 0x266660 (no los estados BGTB): cielo (banco 180, bloques 0..79 y 180..219), banco 58 bloque 45, banco 178 (bloques 20..38, 40, 42..43, 46..52, 57, 59), banco 179 (0, 2) y banco 177 (15..23). Los bancos 175 y 176 y el resto de 177 NO se dibujan en esta ruta. Entradas con flags: 0x12 (cielo, gira con el contador +0x48 / sigue camara) y 0x2 (traslada con la camara). Las entradas sin flags se dibujan en coordenadas crudas.
- `tools/afs_stage_export.py --bg st1` exporta justo ese conjunto (43 bloques, 68 mallas, 28 texturas). Renderizado con el visor nativo desde esa camara (`build/stage_live_r4/cam.txt`): colina de nieve con el mismo relieve y las ruinas al fondo; comparado lado a lado con la captura del emulador (`build/export/live_st1only_cmp.png`) la estructura coincide.

## Sigue sin coincidir (siguiente trabajo)
- Faltan los **props dinamicos** (lapidas, arbustos, abeto pelado, enemigos): en el terreno exportado quedan huecos donde el juego pone objetos (Komono/colocados por script); no vienen de los bancos del decorado.
- El **cielo** (2026-11-10): banco 180 es una cupula achatada (radio ~5049, y -253..3176) con el atardecer; las entradas con flag 0x2 (0x12) se dibujan centradas en la camara. Con la camara de ruta (cabeceo -31 grados, fov 42) el horizonte NO entra en cuadro, asi que el cielo se pinta con otra vista: `vn_viewer --cam --skypitch GRADOS` lo dibuja primero con la misma orientacion horizontal y cabeceo fijo (0 por defecto; el cabeceo real del juego sin averiguar), borra el z y dibuja el mundo. El exportador marca esas mallas con el bit 24 del campo de bloque (`afs_stage_export.py --bg st1 --sky`). Los huecos del terreno dejan ver el cielo: son agujeros reales donde van las piedras. Falta: giro con el contador +0x48 (flag 0x10), fondo lejano (colinas verdes, abetos) y los islotes de ruinas flotando (posicion cruda del banco 58 bloque 45).
- **fov** (2026-11-10): con el visor a 4:3 (`vn_viewer --cam --size 1024x768`) y fov 42 VERTICAL, los huecos circulares del terreno caen donde el juego pone las piedras (mismo reparto en x/y); el desajuste anterior era la relacion de aspecto (el visor usaba 1024x720). fov horizontal 42 (32 vertical) encuadra peor. Props: NO estan en Komono (ids 0x3C..0x48 = bancos 60-72, solo ST2-ST6), ni en banco 59 (cascotes), ni en las entidades residentes de ST1 33/35/45/47/53/87/107/334/346 (enemigos, efectos, esqueletos). Pendiente: volcado de GS de un cuadro.
- Sin iluminacion VU1, niebla ni mezcla GS (el juego aplica bloom/niebla visibles en la captura).
- Solo verificado un cuadro de una demo (ruta 4, camara 23).

## Correccion importante (2026-11-10, agentes)
- `~/emu/ss1` es una PAUSA con otra camara; la escena de `~/emu/w2.png` es ruta 4, camara 23, **cuadro 0**: pos (89.8, 22, -337.4) -> (23, 11, -334.4), fov 42 vertical, 4:3. Con ella el render encaja (arbol, pilares, cuna, arbustos).
- **Props = banco 176 de ST1 (entrada AFS 350), bloques 39..48** (39-43 pilares/muros con arbustos, 44 cuna, 45/47 arbol seco, 46/48 ramas). Salen de las listas de estado de fondo BGTB (estado 5), no de la tabla fija de 12: `BGCtrlTask::Ctrl` recorre la lista del estado actual. Reproducir: `afs_stage_export.py OUT --bg 5 --sky`, cam.txt con la camara de arriba, `vn_viewer OUT --cam --size 1024x768`. Los "huecos" del terreno visto desde la pausa eran troncos de abetos 177:1..14.
- Islotes = suelo de un valle lejano del banco 178 (bloques 40,42,43,46..52,57,59; y=-253) sin su base; el telon de castillo/abetos es 58/45 (z~-4700; su reflejo 58/46 no esta en la tabla fija). Cortinas de niebla 178/20..38 necesitan GL_BLEND (pendiente).
- Luz/niebla en ST1 (RAM): niebla GS apagada (FOGCOL blanco, escala 0), DOF apagado, sin luz direccional; el shader aplica factor plano (0.65,0.64,0.66) a lo no-cielo (medido en suelo; error medio ~1 en 0-255... sobre el fotograma de pausa, el de w2 coincide en media de suelo 150/151/160 vs 159/155/164). Bruma por distancia del agente NO aplicada (ajustada a un solo cuadro).
- Pendiente: GL_BLEND y orden de translucidos, cielo con cabeceo real, colina verde y abetos lejanos, bloom.

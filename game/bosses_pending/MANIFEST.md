# Manifiesto de game/bosses_pending (tanda sobre filas PENDING de Boss 3 y 4)
Estados: reescrita+test / reescrita sin test / pendiente. Direcciones = overlay bs4mod (base 0x9C4800) o bs3mod.
"+test" significa que al menos una asercion de game/test/bosses_pending_test.cpp ejecuta la funcion con valores esperados tomados del asm/ELF/BIN. No se ha ejecutado el juego real.

Compilar y ejecutar (raiz del repo):
    mkdir -p build/verify_pending && c++ -std=c++17 -Wall game/test/bosses_pending_test.cpp game/bosses_pending/boss4_pending.cpp game/bosses_pending/boss3_motion.cpp game/bosses_pending/boss_pending_data.cpp game/bosses_b/boss4.cpp game/bosses_b/boss4_data.cpp game/bosses_a/boss3.cpp game/bosses_a/boss_data.cpp -o build/verify_pending/bosses_pending_test && build/verify_pending/bosses_pending_test
Resultado ultima ejecucion: 495 comprobaciones, 0 fallos. Las tablas de saltos se contrastan con disc/TBL/BS4MOD.BIN y BS3MOD.BIN (se omiten con aviso si faltan).

## Boss 4 (game/bosses_pending/boss4_pending.cpp) - 29 funciones
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| CtrlPre | 0x9C5450 | reescrita+test | despacho por tipo; CtrlPre_shoukan no reescrita (tipo 3 solo suena) |
| CtrlPost | 0x9C5B90 | reescrita+test | |
| CtrlPre_battle1/2/3 | 0x9C5BE0 / 0x9C5D60 / 0x9C5EE0 | reescrita+test | probado el orden de llamadas y la carga unica de la tabla de estados; las acciones act* quedan tras env.stateRun |
| CtrlPre_bunshin | 0x9C68D0 | reescrita+test | |
| UpdateMotionFrame | 0x9C7330 | reescrita+test | |
| ControlCamera | 0x9C95D0 | reescrita+test | |
| ControlPos | 0x9C8770 | reescrita+test | rutas de camara y mezcla probadas con env simulado; flotantes comprobados con tolerancia, no bit a bit |
| SetHeight | 0x9CD060 | reescrita+test | |
| CheckDamageEnd | 0x9C9930 | reescrita+test | |
| ClearWeakHit | 0x9CAE20 | reescrita+test | |
| TurnPos | 0x9CD6B0 | reescrita+test | delega en func_951200 (env.turnToward) |
| MotSupSet | 0x9CD230 | reescrita+test | |
| ControlAttack_battle3 | 0x9C7CF0 | reescrita+test | |
| StartAttackIwa | 0x9C7FD0 | reescrita+test | slot invalido (-1) no comprobado en el original; aqui no genera rocas |
| StartAttackKiriPre | 0x9C8330 | reescrita+test | |
| StartAttackKiri | 0x9C8490 | reescrita+test | doubles del soft-float emulados con double del host |
| StartAttackKiri_shoukan | 0x9C8660 | reescrita+test | |
| GetShoukanPos | 0x9C94B0 | reescrita+test | |
| DownDifficultyReq | 0x9CA330 | reescrita+test | igual a DownDifficulty |
| BnStartBunshin / BnStartShusoku / BnStartStandby / BnCheckCmd | 0x9C6A60 / 0x9C6B10 / 0x9C6B90 / 0x9C6BE0 | reescrita+test | 4 funciones |
| ControlBunshin_battle1 | 0x9C9660 | reescrita+test | |
| ControlBunshin_battle2 | 0x9C97E0 | reescrita+test | tabla jtbl_009DA760 contrastada con el BIN |
| CtrlPre_bunshin_battle1 | 0x9C6C60 | reescrita+test | |
| CtrlPre_bunshin_battle2 | 0x9C6E90 | reescrita+test | tabla jtbl_009DA690 contrastada con el BIN |

Datos: boss_pending_data.cpp (generado por tools/gen_tables.py desde ELF/BIN): D_009D3AE0/3B10, D_009D3D30/3DE0, D_009D3F10/3F30, D_009D2B00 (filas de estado, punteros a act*), D_009D4210/4230 y constantes gp.

## Boss 3 (game/bosses_pending/boss3_motion.cpp)
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| MotSupSet2 (correccion) | 0x9C94A0 | reescrita+test | tabla D_009D0880 completa (13 entradas, contrastada con BS3MOD.BIN); indices 10..12 incluidos; idx >= 13 fuera de la escena 0x14 no hace nada (el original indexa sin limite) |

Resolucion de kBs3Attack[].motion: en la escena 0x14 el valor es un id de animacion directo (p. ej. 324); en otras escenas es un indice de D_009D0880 (13 validos). Las entradas 0..8 de kBs3Attack (motion >= 300) solo son validas como id en la escena 0x14; las 9..19 usan indices 4..8. El codigo existente (bosses_a/boss3.cpp, kBs3Motion[10]) NO se modifico: la correccion vive aqui y queda por integrar. El test prueba que coincide con el codigo previo en los indices 0..9. Un CHECK sobre 0x9D0880+324*8 es aritmetica trivial, no prueba nada del juego.

## Pendiente (no abordado)
- Boss 4: el resto de filas pendientes de bosses_b/MANIFEST.md (aprox. 446 de 475: dibujo, efectos, SetFcv/InitFcv/SetCamPathAttr, DispLife/Disp*, HitMarkEff*, EntryCameraTarget/EntryCollision, ControlSound, CtrlPre_shoukan, new_Boss4, CtrlPut, act*, FinishTask*, SetBodyMove, etc.).
- Boss 2 y el resto de Boss 3: filas pendientes de bosses_a y bosses_rest sin tocar.
- Integrar boss3_motion en boss3.cpp y bosses_b en el flujo real (no se modifico codigo existente).
- Sin verificacion contra una ejecucion real del juego: los tests solo contrastan constantes y logica con el asm.

# game/enemies4 - MANIFEST

Reescritura a mano (C++17, sin dependencias de plataforma) de tareas de enemigo/objeto de GAMEMOD (carga 0x92B000) no cubiertas por enemies*, enemies_ov, enemies_rest.
Globales de bss -> `E4Env` inyectado. Llamadas a motor -> `E4Event` en orden.

Estado: RW+T = reescrita con test; RW = reescrita sin test; PEND = pendiente.
IMPORTANTE: "test" = test unitario escrito por mi a partir de la lectura del asm y de tablas/punteros leidos de GAMEMOD.BIN. El original NO se ejecuto ni se comparo contra emulacion. Es verificacion por lectura, no equivalencia probada.

| # | Funcion original | Direccion | Estado | Notas |
|---|---|---|---|---|
| 1 | __ct__15PetitZombieTask (reglas) | 0x951940 | RW+T | offsets de ENEMY_TABLE inferidos; solo nlRand >= 0; tabla typeHasEffect (RAM) inyectada |
| 2 | Ctrl__15PetitZombieTask | 0x951C40 | RW+T | |
| - | Dead__15PetitZombieTask | 0x951FD0 | no cuenta | el original es `break 0`+jr ra; sin logica |
| - | prtsDispFunc | 0x951F00 | PEND (solo dibujo) | no se reescribe |
| 3 | __ct__9ArmorTask (reglas) | 0x956D60 | RW+T | |
| 4 | PreDisp__9ArmorTask | 0x956F00 | RW+T | |
| 5 | NotDisp__9ArmorTask | 0x956FC0 | RW+T | |
| 6 | ComeOff__9ArmorTask | 0x957040 | RW+T | |
| - | Disp__9ArmorTask | 0x956F50 | solo dibujo | solo `shouldDraw` |
| 7 | __ct__9EnEtcTask (reglas) | 0x938B50 | RW+T | no incluye la asignacion new_* |
| 8 | DemoPauseOn | 0x9381D0 | RW+T | |
| 9 | DemoPauseOff | 0x938200 | RW+T | |
| 10 | EnEtc_ItemCheck | 0x938B30 | RW+T | |
| 11 | St4_02TimerBunki | 0x9387A0 | RW+T | |
| 12 | EnEtc_St4_Tama | 0x9385D0 | RW+T | constantes float de lui/ori |
| 13 | EnEtcST3_SC02_02FireBall | 0x938230 | RW+T | posicion del camino (func_1CC710) inyectada |
| 14 | EnEtc_BranchMark | 0x938860 | RW+T | |
| 15 | SnowCtrl | 0x939D70 | RW+T | |
| 16 | EnSndInit | 0x942690 | RW+T | |
| 17 | EnSndReq | 0x9426E0 | RW+T | |
| 18 | __ct__EnEtcSnd | 0x942800 | RW+T | |
| 19 | EnEtcSnd Ctrl | 0x9427B0 | RW+T | |
| 20 | Ctrl_Pre__11EnDamSphere | 0x942A20 | RW+T | |
| 21 | EnPartsChain | 0x93B1E0 | RW+T | |
| 22 | EnmIDInit | 0x9356F0 | RW+T | |
| 23 | EnmIDSet | 0x935740 | RW+T | |
| 24 | EnmIDClear | 0x9357A0 | RW+T | |
| 25 | PatnInit | 0x94FC20 | RW+T | |
| 26 | PatnForward | 0x94FC60 | RW+T | |
| 27 | PatnFrame | 0x94FDA0 | RW+T | |
| 28 | PartsChangeChk | 0x950570 | RW+T | |
| 29 | Set_ShootOrder | 0x950850 | RW+T | |
| 30 | ScLRChk | 0x951040 | RW+T | |
| 31 | ScChkP | 0x950FB0 | RW+T | |
| 32 | __ct__13TAttackObject | 0x944CF0 | RW+T | |
| 33 | SelectAttackTarget | 0x945040 | RW+T | |
| 34 | GetTargetDist | 0x944FF0 | RW+T | |
| 35 | SetTarget | 0x944EB0 | RW+T | |
| 36 | ManySeTask::loop | 0x9435B0 | RW+T | |
| 37 | BgmFadeTask::loop | 0x943780 | RW+T | |
| 38 | Ctrl_Pre__11EnEtcAttach | 0x93CA70 | RW+T | |
| 39 | __ct__11EnEtcAttach (reglas) | 0x93CE20 | RW+T | velocidades iniciales dependen de matrices del motor: no modeladas |
| 40 | new_EnEtc4 | 0x94F880 | RW+T | solo la condicion |
| 41 | EnEtcThrowObj1Init | 0x93DB60 | RW+T | |
| 42 | Ctrl_Pre__13EnEtcThrowObj | 0x93D2F0 | RW+T | |
| 43 | EnEtcThrowObj3Init | 0x93F160 | RW+T | |
| 44 | Ctrl3_Pre | 0x93EEC0 | RW+T | |
| 45 | Ctrl3_Pos | 0x93F080 | RW+T | |
| 46 | Init9 | 0x941FE0 | RW+T | vector final requiere matrices (se devuelve el offset local) |
| 47 | Ctrl9_Pre | 0x941690 | RW+T | movimiento en espiral del motor no modelado |
| 48 | Ctrl9_Pos | 0x941E20 | RW+T | |

Total reescritas: 48 (48 con test, 0 sin test).

Pendientes (no reescritas): EnEtcParts::Ctrl (0x93A0D0), EnEtcBone Ctrl_Pre/Put/Pos, EnEtcThrowObj Ctrl2/5/6/7 y sus Init, WaitPosCtrl, WeakTask, EnEtc4Obj, PartsChange, TaskCloseX0, CheckHitParts, la asignacion new_* de EnEtcTask, ctors de varias tareas misc.

No verificado: offsets de parametros de ENEMY_TABLE (inferidos del asm); nlRand negativo en PetitZombie; efectos de motor (eventos), no ejecutados; los tests comprueban lo leido del asm y las tablas de GAMEMOD.BIN (mapa de manejadores, D_009B56B0, D_009ACB40, lista EnSnd), no el comportamiento real del juego.

## Compilar y ejecutar (desde la raiz del repo)
    mkdir -p build/verify_enemies4 && c++ -std=c++17 -Wall game/test/enemies4_test.cpp game/enemies4/e4_petit_zombie.cpp game/enemies4/e4_armor.cpp game/enemies4/e4_etc.cpp game/enemies4/e4_misc.cpp game/enemies4/e4_throw.cpp -o build/verify_enemies4/enemies4_test && ./build/verify_enemies4/enemies4_test
Resultado: `OK: 338 comprobaciones, 0 fallos`

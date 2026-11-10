# game/flow_rest — MANIFEST (tanda 2)

Estado: T = reescrita + test, S = reescrita sin test, P = pendiente.
"Test" significa que `game/test/flow_rest_test.cpp` la ejecuta; salvo donde se indica, es contra casos sinteticos escritos por mi a partir del asm, NO contra una ejecucion del original.

Comando (desde la raiz del repo):
`mkdir -p build/verify_flow_rest && c++ -std=c++17 -Wall game/test/flow_rest_test.cpp game/flow_rest/*.cpp game/flow/*.cpp -o build/verify_flow_rest/flow_rest_test && build/verify_flow_rest/flow_rest_test`
Resultado de la ultima ejecucion: `flow_rest_test: OK (0 comprobaciones omitidas)`. `game/test/flow_test.cpp` (antiguo) sigue compilando y pasando.

## (a) Opcodes EVTPAL — evtpal_ops.h/.cpp

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| evOpFromOpcodeV2 / evTemplateOfOpcode (tabla de 116 slots) | `__sinit_eventctrl.cc` 0x2C5B80, plantillas 0x26B530 | T | Derivada del codigo de sinit; las 111 plantillas se comparan una a una con el ELF. Slots nulos: 6, 8, 19, 48, 111. Corrige la extrapolacion de game/flow (111..114 eran 112..115). |
| evInsnLength | handlers de EventCtrl_000 | T | Longitud de cada instruccion contrastada con la ejecucion de los handlers reales en un mini interprete MIPS (10401 instrucciones de EVTPAL.BIN, 0 discrepancias; el interprete es temporal en build/, no se entrega). El test usa un volcado dorado de esa ejecucion (build/verify_flow_rest/evtpal_golden.txt); si falta se omite con aviso. |
| evScanScript | — | T | Recorrido de alcanzabilidad (no es del original). |
| kEvHandlerAddr | ELF | T | Tabla de datos. |

Medicion sobre EVTPAL.BIN (402 scripts, fases 10/11 excluidas), alcanzan NEXTEV:
- tabla antigua (con las longitudes nuevas): 62 (326 paran en opcode sin resolver). El antiguo game/flow_test daba 32 con sus longitudes.
- tabla nueva: 344 directos, 0 paran en opcode sin resolver; 382 siguiendo saltos de fase (BRANCHEV/SETEVNO).
- 20 scripts acaban en un WAIT sin fin: PENDIENTE de entender.
Sin verificar: WAIT/SCEFFINOUT tratados como terminales; el ZMAT con 4 o mas elementos hace assert en el original (no modelado).

## (b) Camara — camera.h/.cpp

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| calcFCurve | 0x1CC410 | T | Orden de operaciones del asm; test contra Hermite en doble (error < 2e-3, no bit-exacto: el FPU del EE trunca). Fuera de rango (x < t0, n=3) el original lee memoria fuera de la curva; aqui se acota. NO probado con curvas reales de AFS. |
| FCurve::fromBytes | — | T | |
| getCameraPath | 0x1CC590 | T | Sintetico. |
| getObjectPath | 0x1CC590+ | T | Sintetico. |
| vecAng | 0x1C9490 | T | arctan2 del motor es un fake. |
| interDivPosI | 0x1C95A0 | T | |
| calcLookAt | 0x1C9D30 | T | Solo indirectamente, con getAngleYX falso. |
| setAfterSpeed | 0x1CB370 | T | Con tablas inventadas; las tablas reales D_00266A50 no estan cargadas en el test. |
| setFinishPos | 0x1CAC80 | T | |
| traceCamPath | 0x1CAD50 | T | Modos 1, 2 y recorrido. |
| traceCamPath_000 | 0x1CAF40 | T | Solo la rama de modo demo (el avance fuera de demo lo hace otra rutina no localizada). Lee ring[head] como el original. |
| game | 0x1C9FA0 | T | Solo el arranque; delega en game_000 (P). |
| entryObj | 0x1CB310 | T | |

Pendiente: game_000 (0x1CA190), calcGameCam, calcGameInt, getObjectPathS, takeOver, CamPathMgr entryExec_*/freeExec/entryStage (carga de datos), curvas reales de AFS/CAMNOTBL, bit-exactitud del FPU.

## (c) Continuar / Game Over / Nombre / Ranking — screens.h/.cpp

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| PlayerTask::changePlayerMode | 0x20F020 | T | |
| PlayerTask::checkCredSt | 0x20EED0 | T | |
| PlayerTask::checkCredAndStBtn | 0x20EE30 | T | |
| PlayerTask::contInit | 0x20E2F0 | T | |
| PlayerTask::contLoop | 0x20E3C0 | T | El dibujo con proyeccion va como evento. |
| PlayerTask::gmoverInit | 0x20E670 | T | Tabla de posiciones 0x274BF0 inyectada, no leida del ELF. |
| PlayerTask::gmoverLoop | 0x20E780 | T | |
| PlayerTask::nameentInit | 0x20E820 | T | |
| PlayerTask::nameentLoop | 0x20E890 | T | SPR_ARG reducido a evento drawNameCursor. |
| CalcClearRanking | 0x2266D0 | T | Con las dos tablas reales del ELF (0x2C4BD0 y 0x2C4BF0). |
| ClearRankingLoop | 0x226EF0 | T | |
| BookExit | 0x2262C0 | T | |
| IraiList_Loop | 0x226420 | T | La transparencia de la tapa (formula en doble precision) NO esta trasladada: bookCover(fade) recibe solo la fase 0/1. |

Sin confirmar (nombres por offset): significado de gp-0x73B4, gp-0x73F0, gp-0x72D0 (bits), 0x500DFC, gp-0x6F90, estado de CreditSlot (+4 == 2), la condicion de 0x500738 && 0x500A80 que elige la tabla de ranking, unidades de contCount (0x1000 por segundo es deduccion).
Pendiente: ClearRankingInit, ClearRankingTaskConst, ClearRankingTask::putHaikeiTask/putInfo/putTask, ClearRankingCameraTask, IraiList/IraiList_Init/IraiExit, PlayerTask::contGameInit, TrnMojiUchi.

## Cuenta
30 funciones reescritas con test (4 + 13 + 13); 0 sin test. Todas verificadas con tests sinteticos salvo los puntos de "contra el ELF / el asm" indicados arriba. Pendientes enumeradas arriba.

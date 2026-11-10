# game/flow3 — MANIFEST (tanda 3)

Estado: T = reescrita + test, S = reescrita sin test, P = pendiente.
"Test" significa que `game/test/flow3_test.cpp` la ejecuta. Los tests comprueban MI lectura del asm (casos sinteticos y recorridos de EVTPAL.BIN real); NO son una ejecucion diferencial del original. No hay bit-exactitud verificada.

Comando (desde la raiz del repo):
`mkdir -p build/verify_flow3 && c++ -std=c++17 -Wall game/test/flow3_test.cpp game/flow3/*.cpp game/flow_rest/*.cpp game/flow/*.cpp -o build/verify_flow3/flow3_test && build/verify_flow3/flow3_test`
Ultima ejecucion: `flow3_test: OK (208 comprobaciones, 0 omitidas)`.

## Funciones

| Funcion original | Direccion | Estado | Notas |
|---|---|---|---|
| EventCtrlTask::__ct (lookup EVTPAL[etapa][evento][fase]) | 0x1E29F0 | T | fase inicial = (gameState != 9 && evento != 0) ? 1 : 0 |
| EventCtrl_000 (bucle de instrucciones, espera, stopFlag) | 0x1E2B80 | T | guardia de 100000 pasos por fotograma (el original no la tiene) |
| Primer tramo de EventCtrl_000 (canAdvance) | 0x1E2B80 | T | |
| Despacho de los 116 opcodes (nombres/handlers comprobados contra glabels del asm) | 0x1E2E80..0x1E5010 | T | ZMAT, SCEFFINOUT y BRANCHNOSET: ver "No verificado" |
| NEXTEV | 0x1E3BF0 | T | tipo 1 lee a traves de filas |
| EventStageClear | 0x1E2CC0 | T | tabla de transiciones comparada con GAMEMOD.BIN (campos 0x9B83D0) |
| InitStage | 0x1E28C0 | T | solo contadores y 256 objetos; efectos de motor emitidos como eventos |
| EvCancelTask::ctrl | 0x1E5320 | T | devuelve la accion al motor |
| EvCancelTask::skip | 0x1E53D0 | T | |
| game_000 (camara de juego por fotograma) | 0x1CA190 | T | tests sinteticos; `game/flow3/flow3_camera.*`; calcGameCam/calcGameInt inyectados |
| game_calcGameCam | 0x1CA6C0 | P | solo punto de inyeccion (virtual vacio) |
| game_calcGameInt | ~0x1CA860 | P | idem |
| calcCamera, gameCameraCtrl, takeOver | (CameraMgrInternalClass) | P | no abordadas |
| Camaras de jefe: EntryCameraTarget, GetCamPos, GetCamRot, Allow/ForbidCameraStart, new_Boss6_2CameraCtrl, SetCamPathAttr | | P | no abordadas |
| trn_05 / trn_08 | | P | ya existen game/stages/training.* (tick05/tick08) y game/trnkomono; no se rehizo ni se comparo |

Conteo real: 10 funciones con estado T (las 9 filas de EVTPAL, contando EventCtrl_000 y su primer tramo por separado, mas game_000); 0 S; el resto P.
Nota: el despacho de opcodes se cuenta como una fila aunque son 116 handlers.

## Hallazgos (EVTPAL)

- Los 20 scripts que terminan en WAIT estan aparcados a proposito: otro modulo (TrainingTask, modo Original o la propia partida) avanza el flujo con objetos (0x20..0x23, inferido) y reescribiendo savedPc. Etapa 6: 18 de ellos (11 misiones de entrenamiento con espera 0x21); 4 comparten direccion de WAIT con otro.
- Etapas 10 y 11: estructura identica entre si (150 fotogramas, 16 cambios de fase hasta EventStageClear).
- 448 scripts (etapas 0..11) ejecutados: 428 salen por NEXTEV/BRANCHEV/SETEVNO, 20 aparcan, 0 fallos, 0 con pc fuera del barrido estatico. Nueve etapas llegan a EventStageClear.
- Discrepancias en el game/flow existente (documentadas, NO modificado): BRANCHEV por defecto = primera entrada; los objetos se indexan `& 0xFF` (game/flow usaba `& 63`). Objeto mayor usado por scripts reales: 254.
- 25 opcodes validos sin uso alcanzable en EVTPAL.BIN (lista en la salida del test).

## No verificado

- Significado de las etapas 7 y 10; significado de los objetos 0x20..0x23 (inferido por uso).
- BRANCHNOSET: llamada opaca a func_97ED80, solo se emite como evento.
- Efectos de motor (todos emitidos como eventos, no ejecutados).
- ZMAT desborda y SCEFFINOUT cuelga en el original: se modelan como fallos, ningun script real los alcanza.
- Tabla DefaultTrans: 484 campos comparados con GAMEMOD.BIN; no se probo cada fila contra el comportamiento del original.
- game_000: m2c es incompleto en mula/madd/msub/adda; lo reescribi leyendo el asm a mano. Las sumas con acumulador del EE (precision de redondeo) se hacen con float normal. Detalles fieles al asm que conviene conocer: tras actualizar look, f20/f22 siguen con los valores previos (como los registros); branchNo no esta acotado en el original (aqui se acota y se cuenta en `outOfRange`). nlTan, VecCos e InterDivCamAng3D son del motor y se inyectan: no revisados aqui.
- Las constantes 0x3DCCCCCD (0.1f) y 0x3F7FFF58 (0.99999f) se decodificaron a mano.

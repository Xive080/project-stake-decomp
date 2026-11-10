# game/stages — logica de fase (overlays orgmod, aomod, trnmod, acmod)

Test: `c++ -std=c++17 -Wall game/test/stages_test.cpp game/stages/org.cpp game/stages/org_tables.cpp game/stages/komono.cpp game/stages/training.cpp game/stages/training_tables.cpp game/stages/title.cpp -o build/stages_test && ./build/stages_test` (ultima ejecucion: `stages_test: TODO OK`).

Metodo: m2c como borrador, cada funcion contrastada con el asm (m2c erro en OrgShopLevel: el asm no excluye el nivel 4). Tablas leidas de los .BIN (`disc/TBL/*.BIN`, offset = addr - base del overlay) y del ELF. Los tests usan esas tablas reales; los valores esperados de las funciones salen de leer el asm (no hay ejecucion diferencial contra el original).
Direcciones: las de orgmod/acmod/aomod/trnmod no se citan una a una (los nombres son los del asm: `asm/<overlay>/<overlay>_text.s`).

Estados: R+T = reescrita + test; R = reescrita sin test; P = pendiente.

## orgmod (org.h/.cpp) — 11 funciones
| Original | Estado | Notas |
|---|---|---|
| OrgPlWeaponDamageGet_core | R+T | tabla D_00A12500; objeto 8 sin marca +0x346 = 0.5. Indexa sin acotar en el original; aqui ids >15 dan 1.0 |
| OrgEnemyLifeScale | R+T | minimo de 1 / 0.6666 (0x3F2AA64C) / 0.5 por objetos 38/39 en ranuras 2..3, jugadores en modo 0,1,3,4,5 |
| OrgAddItem | R+T | solo estado; devuelve si hay aviso HFileAddPageG |
| OrgShopLevel | R+T | corregido frente a m2c; tabla D_00A12B70 |
| OrgAddSilver | R+T | solo estado + eventos TDispSilver |
| new_mission_complete | R+T (parte de estado) | el mapeo "mision 14 -> limpia 0x502720..." es literal del asm; no se sabe el significado |
| SetOrgMissionOver | R+T | 30000 pts / 1000 plata / 140000 pts |
| checkGetAnotherPlayerEquip, unequipAnotherPlayerEquip | R+T | tabla de modos jtbl 0xA12BD0/0xA12C00 leida del .BIN |
| unequipJoinIn | R+T | tabla de objetos ELF 0x2C2A10 |
| setDispScoreJoinIn | R+T | |
| gunOrgExe (0xA10 B), gunOrgBulletSet, gunOrgSoundOff, ctrl/disp de McTask, TDispSilver, putNum/putSilver | P | gunOrgExe es la regla de disparo por arma (jitter con rand, balas por objeto 10/11/13...), larga y con jtbl; el resto dibuja |

## aomod (komono.h/.cpp) — 13 funciones
| Original | Estado | Notas |
|---|---|---|
| checkEquipItem | R+T | jtbl 0xA0B590: modo 2 y >= 7 se saltan |
| calcItemNo | R+T | botin ponderado; kind 1/2 hace `break 0` (devuelve -1) |
| CKomonoBase::checkEndCP | R+T | capacidad 5 supuesta por el layout de initEndCP (no verificado) |
| Komono3Task::checkCamPath, Komono2Task::checkCloseCamPath | R+T | |
| checkMissionEnd_setTrnsl, checkMissionCompleteMessageDispTiming, checkCloseTiming_clrBranchNo, checkGameMode | R+T | las llamadas func_1A2630 (cerrar tarea) = `closeTask`; la semantica "cerrar" es inferida |
| checkColli_setBranchNo (+ Hit) | R+T (solo estado) | sonidos 0xB012B / 0x20025 |
| setBranchNo, clrBranchNo | R+T | |
| ItemTask::ctrlPost | R+T (solo recogida) | eleccion de jugador, sonidos, vida vs inventario; movimiento posterior no |
| P: KomEtcMgr*, initEndCP, bornItem, SetKom*HaichiTbl* (generan tareas), Komono3Task::*Ctrl (nuts, goldenRat, secretDoor, zaisan, makimono, hitotsu, dog...), KomonoTumiage*, KomonoRigid*, Kusari*, Komono2Task::st*(dibujo/animacion), ItemGetDisp* | P | mezclan estado con motor; las Ctrl usan las mismas rutas de camara y se pueden hacer reusando K3 |

## trnmod (training.h/.cpp) — 37 entradas
Misiones (indice = NN-1): init01/02/03/04/05/06/07/09/11/12 (10) + tick01..09,11,12 (11) = 21; trn_boss (bossOver), DecideBatNum, time_change, Trn08KomonoMgrTask::moji2obj, checkBadWord (lista real de 50 palabras del ELF), trn_free1 = 6; TrainingBat SetFallParam, FallDown, RotPos, ChkMode (+ DecNowEnemyNumChk, DecNowEnemyAlvNumChk, ChgDead inlinadas) = 7; CTrn10Komono setMode, ctrl2, hitMove = 3. Todo R+T.
Notas: tick05/06/07/11 solo la parte de estado (dibujo y camara excluidos); init05/trn_05 no crea el gestor Trn05KomonoMgrTask; trn_07 usa la Y de camara como entrada; 50 fotogramas = 1 s (x0x32). Los sufijos de dibujo (time_put, norm_put) no se modelan.
P: trn_10/trn_10_init (gestor Trn10KomonoMgrTask), Trn05/Trn08/Trn10KomonoMgrTask::ctrl (escritura de nombre / bolas), CTrn05Komono, CTrn08Komono::ctrl, judge/success/failed/retry/exit/start/start_init/select*, trn_free0, TrnGeneralTask::ctrl (0x1434), TrainingObjTask, mcSave.

## acmod (title.h/.cpp) — 9 funciones
GetMainMenuNo, DemoMgrClass::setStageNo/getStartPtid/getDemoNo (tabla D_00A1AE20: solo hay 1 demo + terminador), Title2Task::decideSound, checkConnectGuncon, RankSampleTask::getSightPos (estado), TutdispBulletMoveOn (contador circular), DemoCtrlTask::loop. Todo R+T.
P: Title2Task::loop/Title2MviTask::loop (flujo del menu), Ranking*/RankSampleTask::loop/checkController, Logo*, Adver*, TutControlerTask::ctrl (guion del tutorial; en m2c es casi todo dibujo), Tutorial*Task.

## Resumen
Total: 70 entradas reescritas con test (algunas solo en su parte de estado, indicado arriba). Sin ejecucion diferencial contra el original ni contra el ELF: la verificacion es lectura de asm + tablas reales de los .BIN/ELF + tests unitarios.

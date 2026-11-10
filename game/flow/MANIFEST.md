# game/flow MANIFEST

Test: `c++ -std=c++17 -Wall game/test/flow_test.cpp game/flow/*.cpp -o build/flow_test && build/flow_test`
(desde la raiz del repo; ultima ejecucion: `flow_test: OK`).

Estados: T = reescrita + test; S = reescrita sin test; P = pendiente (no incluida).
"Test" significa que hay aserciones con valores sacados del ELF/GAMEMOD/asm; no se ha comparado
contra una ejecucion del juego original.

| Original | Direccion | Estado | Notas |
|---|---|---|---|
| MainModeMgrClass::init | ELF | T | |
| MainModeMgrClass::next | ELF | T | tabla kNextMode (ELF 0x2C2130) |
| MainModeMgrClass::synch | ELF | T | kSkipOnPress (ELF 0x2C0E92) |
| MainModeMgrClass::synch2 | ELF | T | |
| MainModeMgrClass::chgInit | ELF | T | los efectos del motor salen como ChgInitPlan |
| SysCfgMgr::setLang | ELF | T | mapa SCF 5->5, 4->4, 3->6, 2->3, resto 2 |
| SysCfgMgr::clrAll | ELF | T | defaults ELF 0x266BA0 |
| SysCfgMgr::init2 | ELF | T | |
| SysCfgMgr::synch | ELF | T | |
| SysCfgMgr::init1 / setBook* | ELF | P | vacias en el original, no incluidas |
| RankingMgr::setInitTbl | ELF | T | tablas A/B ELF 0x28C180/0x28C200 |
| RankingMgr::checkRankIn | ELF | T | modo 9 no entra |
| ClearRanking* | ELF | P | no leido |
| PauseMgr::init | 0x1F33xx | T | llamadas virtuales sustituidas por PauseHooks |
| PauseMgr::set / get | ELF | T | |
| PauseMgr::deleteWaitCauseElem | 0x1F33E0 | T | |
| PauseMgr::chkLoadWaitOvlMod | ELF | T | |
| PauseMgr::exec | ELF | T | |
| PauseMgr::synch | 0x1F3610 | T | |
| PauseMgr::pushStat / pushSetStat / popStat | ELF | T | |
| PauseMgr::reqPauseUntilAllDataEntry | ELF | T | |
| PauseMgr::reqPauseUntilOvlModEntry | ELF | T | |
| SystemTask::__ct / __dt (flags y contadores) | ELF | T | operator new / killFamilyLink: P |
| SystemTask2::newExec | ELF | T | |
| SystemTask2::changeExec | ELF | T | |
| SystemTask2::deleteExec | ELF | T | |
| SystemTask2::threadCtrl / threadCtrl2 | ELF | T | |
| SystemTaskLevelCheck::threadCtrlLevelCheck / 2 | ELF | T | solo nodos con prio >= PauseMgr::get() |
| CameraMgrInternal::flushCamJob | 0x1C98xx | T | |
| CameraMgrInternal::chkRemainCamJob | | T | |
| CameraMgrInternal::entryEvent | | T | longitud registro CCTB = 3 + (tipo>>4) palabras |
| CameraMgrInternal::chgToDirMode | | T | |
| CameraMgrInternal::getJob / finishJob | | T | |
| CameraMgrInternal::setFrameSpd | 0x1CAB20 | T | |
| CameraMgrInternal::setCamSplCnt | 0x1CAB90 | T | |
| CameraMgrInternal::setCafsIdx | 0x1CAC40 | T | |
| CameraMgrInternal::game / game_000 / setFinishPos / traceCamPath / setAfterSpeed | 0x1C9FA0, 0x1CAC80, 0x1CAD50 | P | matematica de camara en coma flotante, solo via CamHost |
| CamPathMgr::getStartFrame / getEndFrame | ELF | T | solo a nivel de curva (CurveKeys); CalcFCurve no |
| CamPathMgr::getCameraPath / getObjectPath(S) / entryExec_STAT_* / entryStage | ELF | P | |
| CameraTask::gameCameraCtrl / calcCamera / entryObj / init | ELF | P | |
| BGCtrl::appointBG / appointBGImm | ELF | T | solo a traves del handler BGTB de EventCtrl |
| BGCtrl PutForEntry* / setBGCtrlTbl / BGCtrlTask::Ctrl | ELF | P | dibujo |
| EventCtrlTask::EventCtrl_000 (frame) | 0x1E2D10 | T | |
| EventCtrl::__ct | ELF | S | solo estado; parcial |
| EventCtrl lookupPc (EVTPAL 3 niveles) | | T | |
| EventCtrl NEXTEV + tablas de transicion (12) | GAMEMOD 0x9B83D0 | T | numero de filas inferido por separacion de punteros: sin verificar |
| Handlers de salto: BRANCH, BRANCHNO, BRANCHEV, SETEVNO, SWITCH_MENM, BRANCHITEM, BRANCHADR, MISSENMCLR, SKIP, WLCTB, ZMAT | | T | BRANCHITEM/GAMEFLAGSET/WAIT_FLUSHDATA_NOPAUSE dependen de la extension de opcodes 111..114 |
| Handlers de espera: WAIT, WAIT_FRM, WAIT_ENNUM, WAIT_ENALV, WAIT_PEOPLE, WAIT_ITEM, WAIT_CAMJOB, WAIT_CAMFRM, WAIT_FLUSHDATA(_NOPAUSE), WAIT_BUFFERRAM, WAIT_LOAD_OVLMOD | | T | semantica de wait +4 de WAIT inferida |
| Handlers de estado: ITEMSET, GAMEFLAGSET, GETCPMODE, NOWLOADINGSET, SNOWDISP, BSFT, JOBFLAGSET, WAIT_INT_MODE, FORCE_CAMPARAM, CCTB, CAMDR, BGTB, VIBRATION, SNDONSK/SNDONMSK | | T | probados via exec(EvOp) directo cuando el opcode no resuelve por step() |
| Ordenes de longitud fija o terminada en lista (host.cmd) | | S | probadas en conjunto, no una a una |
| ENTB, ENTB2, ET1P, ET2P, ENTBSK, FOGCTRL, LIGHTCTRL, DOFCTRL, BLURCTRL | | S | longitud/efecto decididos por el host; solo se prueba skip y delegacion |
| Opcodes sin resolver: 6, 8, 19, 29..32, 36, 38, 39, 41..52 | | P | afs_evtpal.py no los nombra |
| Opcodes 111..114 | | S | EXTRAPOLADOS (plantilla +4), SIN VERIFICAR |
| Continue / game over / entrada de nombre | | P | no tocado |

## Barrido informativo de EVTPAL.BIN real
402 scripts: 32 llegan a NEXTEV, 151 paran en opcode sin resolver, 198 en operaciones de
longitud variable decididas por el motor, 12 en saltos. No es una prueba de exactitud.

## Cuenta
Funciones individuales con test (T): MainMode 5, SysCfg 4, Ranking 2, Pause 12, SystemTask 1 (ct/dt), SystemTask2 5,
LevelCheck 2, Camara 9, CamPathMgr 2, BGCtrl 2, EventCtrl_000 1, NEXTEV 1 = 46,
mas los handlers de EventCtrl (~63 casos en eventctrl.cpp, de ellos varios S).

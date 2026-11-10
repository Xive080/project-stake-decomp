# MANIFEST game/komono (clases Komono* del overlay aomod)

Verificacion: lectura del asm (asm/aomod/aomod_text.s) + borrador m2c (build/m2c/komono_aomod.s, build/m2c/komono_all.c; con `vsqi/vlqd` neutralizados con sed) y
test contra las tablas de disc/TBL/AOMOD.BIN (D_00A093F0, D_00A096D0, D_00A09DB4; offset = direccion - 0x9DF000). El original NUNCA se ejecuto: el resto de
valores de los tests (sonidos, constantes, temporizadores) son los del asm/m2c y comprueban que el C++ los reproduce, no que el asm este bien leido.
Los globales de sistema (0x500F14, gp-0x73B4, gp-0x7338 camara, D_500F20 pausa, D_502708 misiones, gp-0x72E0, gp-0x6F00, gp-0x7608..) viven en bss: se inyectan en `KomonoEnv`.
Las llamadas a motor van por la interfaz abstracta `KomonoHost` (sonido, puntuacion, efectos, colisionadores, animacion, creacion de tareas).
Se compila con `-ffp-contract=off` (y `#pragma STDC FP_CONTRACT OFF`): la PS2 no tiene FMA.

Compilar y ejecutar (desde la raiz del repo):

    mkdir -p build/verify_komono && c++ -std=c++17 -Wall -ffp-contract=off game/test/komono_test.cpp game/komono/komono.cpp game/komono/komono3.cpp game/komono/komono3b.cpp game/komono/komono_debris.cpp game/komono/komono_tumiage.cpp game/komono/komono_tables.cpp -o build/verify_komono/komono_test && ./build/verify_komono/komono_test

Ultimo resultado: OK: 627 comprobaciones, 0 fallos.

Estado: R+T = reescrita con test; PEND = pendiente.


## Funciones reescritas (56)

| Clase | Funcion original | Direccion | Reescrita como | Estado | Notas |
|---|---|---|---|---|---|
| CKomonoBase | checkEndCP__11CKomonoBaseFv | 0x9DF0F0 | CKomonoBase::checkEndCP | R+T |  |
| CKomonoBase | bornItem__11CKomonoBaseF11tagNLpoint3iiii | 0x9DF170 | CKomonoBase::bornItem | R+T | la creacion de ItemTask sale por host.spawnItem; 5o parametro (tipo de objeto) = t1 (inferido del llamador close__10KomonoTask, que lo carga de +0x180) |
| CKomonoBase | initEndCP__11CKomonoBaseFPii | 0x9DF2B0 | CKomonoBase::initEndCP | R+T | el bucle desenrollado del original se reduce a un bucle simple con el mismo resultado final (incl. solape de ids/limites con n > 5) |
| KomonoTumiageColliTask | __ct__22KomonoTumiageColliTask... | 0x9E73E0 | ColliLogic::init | R+T | solo la parte de estado (flags, radio, pos); registro de tablas/colision del motor omitido |
| KomonoTumiageColliTask | chkColli | 0x9E7600 | ColliLogic::chkColli | R+T |  |
| KomonoTumiageColliTask | getHitPl | 0x9E7630 | ColliLogic::getHitPl | R+T | si no hay bit 2 ni 4 el original devuelve el valor basura de a1; aqui 0 (no verificable) |
| KomonoTumiageColliTask | clrColli | 0x9E76F0 | ColliLogic::clrColli | R+T |  |
| KomonoTumiageColliTask | close | 0x9E7710 | ColliLogic::close | R+T |  |
| KomonoTumiageColliTask | setPos | 0x9E7720 | ColliLogic::setPos | R+T | m2c perdia argumentos; confirmado en el asm de la llamada (0x9E99D4) |
| KomonoTumiageColliTask | setRadius | 0x9E7730 | ColliLogic::setRadius | R+T |  |
| KomonoTumiageColliTask | ctrlPre | 0x9E7740 | ColliLogic::ctrlPre | R+T |  |
| KomonoTumiageTask | ctrl | 0x9E9400 | TumiageLogic::ctrl | R+T | parcial: centros, colisionador y gafas; NO el calculo de ejes/matriz de orientacion (+0x12E0, func_1AF9FC/1AF968) ni el dibujo |
| KomonoTumiageTask | ctrlPost | 0x9E9B10 | TumiageLogic::ctrlPost | R+T |  |
| KomonoTask | __ct__10KomonoTaskFP11ENEMY_TABLEiii | 0x9E9B60 | KomonoLogic::init | R+T | unido con init: copia de campos y initEndCP |
| KomonoTask | init__10KomonoTaskFP11ENEMY_TABLE | 0x9E9D40 | KomonoLogic::init / altType | R+T | tabla id->id alternativo y busqueda en D_00A093F0 (45 filas, comparada con AOMOD.BIN) |
| KomonoTask | ctrl__10KomonoTaskFv | 0x9E9F80 | KomonoLogic::ctrl | R+T | dibujo omitido |
| KomonoTask | ctrlPost__10KomonoTaskFv | 0x9EA090 | KomonoLogic::ctrlPost | R+T | golpe = 10 puntos al jugador que disparo |
| KomonoTask | close__10KomonoTaskFv | 0x9EA1E0 | KomonoLogic::close | R+T | tabla D_00A096D0 (43 filas, comparada con AOMOD.BIN); el 2o argumento de func_1E12B0 (contador del bucle) no se emite: sin uso conocido |
| Komono3Task | setDynaParam__11Komono3TaskFii | 0x9EA4B0 | DynaLogic::setDynaParam | R+T | tabla s16 de 0x1FB73E4 fuera de ELF y de AOMOD.BIN: inyectada |
| Komono3Task | setBranchNo | 0x9ED560 | Komono3Logic::setBranchNo | R+T |  |
| Komono3Task | clrBranchNo | 0x9ED580 | Komono3Logic::clrBranchNo | R+T |  |
| Komono3Task | checkHit | 0x9ED2D0 | Komono3Logic::checkHit | R+T |  |
| Komono3Task | calcEffPos | 0x9ED3D0 | Komono3Logic::calcEffPos | R+T | la transformacion de camara (func_1B00E0 + func_1AFC00) es del motor |
| Komono3Task | setColli | 0x9ED510 | (estatica en komono3.cpp / komono3b.cpp) | R+T | probada a traves de las rutinas que la usan |
| Komono3Task | checkCamPath | 0x9ED590 | Komono3Logic::checkCamPath | R+T |  |
| Komono3Task | checkColli_setBranchNo | 0x9ED5F0 | Komono3Logic::checkColli_setBranchNo | R+T |  |
| Komono3Task | checkMissionEnd_setTrnsl | 0x9ED700 | Komono3Logic::checkMissionEnd_setTrnsl | R+T |  |
| Komono3Task | checkGameMode | 0x9ED7A0 | Komono3Logic::checkGameMode | R+T | D_502708 (banderas de mision) inyectado |
| Komono3Task | checkMissionCompleteMessageDispTiming | 0x9ED8C0 | Komono3Logic::checkMissionCompleteMessageDispTiming | R+T |  |
| Komono3Task | checkCloseTiming_clrBranchNo | 0x9ED920 | Komono3Logic::checkCloseTiming_clrBranchNo | R+T |  |
| Komono3Task | lightCtrl | 0x9ED130 | Komono3Logic::lightCtrl | R+T | solo el avance del angulo de luz; la copia de parametros de luz al motor se omite |
| Komono3Task | orgCtrl | 0x9ED970 | Komono3Logic::orgCtrl | R+T | dibujo (orgPut) omitido |
| Komono3Task | broochCtrl | 0x9ED9E0 | Komono3Logic::broochCtrl | R+T | = orgCtrl; los 3 dibujos fijos (D_00A09AE0) omitidos |
| Komono3Task | silverGrassCtrl | 0x9EDAE0 | Komono3Logic::silverGrassCtrl | R+T |  |
| Komono3Task | goldenBornCtrl | 0x9EDCB0 | Komono3Logic::goldenBornCtrl | R+T | 3 rafagas de efectos con desplazamientos del asm |
| Komono3Task | goldenRatCtrl | 0x9EE040 | Komono3Logic::goldenRatCtrl | R+T | la muestra de animacion (func_1CC710) es del motor |
| Komono3Task | goldenFishCtrl | 0x9EE390 | Komono3Logic::goldenFishCtrl | R+T | la tabla de modelos D_00A09240 solo afecta al dibujo (omitida) |
| Komono3Task | nutsCtrl | 0x9EE710 | Komono3Logic::nutsCtrl | R+T | parcial: posiciones animadas +0x2E8/+0x300 las escribe el motor (animSample); dibujo omitido |
| Komono3Task | silverKinokoCtrl | 0x9EF120 | Komono3Logic::silverKinokoCtrl | R+T | escala del pulso (seno de +0x5DC) solo para dibujo |
| Komono3Task | makimonoCtrl | 0x9EFA00 | Komono3Logic::makimonoCtrl | R+T |  |
| Komono3Task | urokoCtrlA | 0x9EFBC0 | Komono3Logic::urokoCtrlA | R+T | campos +0x1CC/+0x1D0 reutilizados como camara/frame de cierre |
| Komono3Task | urokoCtrlB | 0x9EFD90 | Komono3Logic::urokoCtrlB | R+T |  |
| Komono3Task | hitotsuCtrl | 0x9EFF10 | Komono3Logic::hitotsuCtrl | R+T | limites 561.2/261.2/461.2 deducidos como A + 1.2f (no son constantes del asm); muestreo func_1E6DE0 omitido |
| Komono3Task | kinokoCtrl | 0x9F0460 | Komono3Logic::kinokoCtrl | R+T | 3 dibujos de D_00A09DB0 omitidos; posicion de gafas D_00A09DB4 comparada con AOMOD.BIN |
| Komono3Task | maboroshiCtrl | 0x9F1CA0 | Komono3Logic::maboroshiCtrl | R+T | = cadena orgCtrl; 2 dibujos fijos omitidos |
| Komono3Task | st2NakaDoorCtrl | 0x9EBD30 | StageObjLogic::nakaDoorCtrl | R+T | solo el cierre por camara; el resto es dibujo |
| Komono3Task | st2NakaDoorCtrlPost | 0x9EBEB0 | StageObjLogic::nakaDoorCtrlPost | R+T |  |
| Komono3Task | st3bs01KnightBunkiCtrlPre | 0x9EC0B0 | StageObjLogic::knightCtrlPre | R+T |  |
| Komono3Task | st3bs01KnightBunkiCtrl | 0x9EC150 | StageObjLogic::knightCtrl | R+T | dibujo omitido |
| Komono2Task | checkCloseCamPath__11Komono2TaskFif | 0x9F6CE0 | DynaLogic::checkCloseCamPath | R+T | argumentos (int path, float frame) leidos de m2c: this no se usa |
| Komono2Task | setDynaParam__11Komono2TaskFii | 0x9F6D40 | DynaLogic::setDynaParam | R+T | misma logica, offsets 0x1C/0x2C |
| KomonoDishKowareTask | ctrl__20KomonoDishKowareTaskFv | 0x9E43F0 | DebrisTasks::dishCtrl | R+T |  |
| KomonoBoxKowareTask | ctrl__19KomonoBoxKowareTaskFv | 0x9E4E80 | DebrisTasks::boxCtrl | R+T |  |
| KomonoCrosetKowareTask | ctrl__22KomonoCrosetKowareTaskFv | 0x9E3A40 | DebrisTasks::crosetCtrl | R+T |  |
| KomonoKusariHahenTask | __ct__21KomonoKusariHahenTaskFffff | 0x9F1E20 | DebrisTasks::kusariInit | R+T | funciones func_1B0938/func_1B09B4 (angulo) del motor; rotVel[2] y rot[2] a 0 (el original no los escribe: asume memoria a cero) |
| KomonoKusariHahenTask | loop__21KomonoKusariHahenTaskFv | 0x9F2110 | DebrisTasks::kusariLoop | R+T |  |

## Vacias en el original (sin logica, no contadas)
`KomonoTumiageColliTask::ctrlPost`, `::ctrl`; `KomonoTask::ctrlPre`; `Komono3Task::st2NakaDoorCtrlPre`, `st3bs01KnightBunkiCtrlPost`.

## Pendiente (no incluido; no se ha inventado nada)
- `Komono3Task::__ct__` (0x9EA600: usa tabla de saltos que m2c no resuelve; lee la tabla del overlay), `hasiraCtrl` (tabla de saltos), `secretDoorCtrl`, `zaisanCtrl`, `goldenKubiwaCtrl`, `agroomCtrl`, `taruBoyCtrl`, `dogCtrl`, `st3ElvGrassCtrl`, `st1NuigurumiCtrl`, `st3bs01KnightBunkiCtrlPost` (vacia), `orgPut` (solo dibujo).
- `KomonoRigidTask` entera (`__ct__`, `ctrlPre`, `ctrl`, `ctrlPost`, `indoorBoxCtrl`, `indoorBoxCtrlPost`): fisica de cuerpo rigido, ~800 lineas de m2c sin revisar.
- `KomonoTumiageTask::__ct__`, `setKomParam` (inicializacion de puntos CBoxPoint), `ctrlPre` (~600 lineas, fisica de los puntos) y el calculo de orientacion de `ctrl`.
- Constructores y `newPtcl` de `KomonoDish/Box/CrosetKowareTask` (reparto aleatorio de los trozos); `KomonoTask`/`Komono3Task`/`KomonoTumiageTask` destructores; `new_Komono*Task` (reserva de memoria).
- `Komono2Task` (~85 funciones `st2*`/`st3*`/`adv*`/`door*Put`...): objetos de decorado casi todo dibujo/animacion; solo se han reescrito `checkCloseCamPath` y `setDynaParam`.
- Dibujo omitido deliberadamente en todas las `ctrl` (func_1B0808/1B055C/1B03EC/1B027C/214A00/214AE0, OrgDisp*): salta por no ser regla de juego.


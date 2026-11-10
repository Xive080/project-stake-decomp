# game/player2 MANIFEST

Resto de PlayerTask / PrePlayerTask / PostPlayerTask (ELF main SCES_504.11, 0x20D990..0x2133A0) que no cubren `game/src/player.*`, `game/flow_rest` (cont/gmover/nameent/checkCred/changePlayerMode) ni `game/extras` (GunMgr). Los ensamblados son `asm/PlayerTask_*.s`, `asm/PrePlayerTask_213200.s`, `asm/PostPlayerTask_2133A0.s`.
Estado: T = reescrita+test, S = reescrita sin test, P = pendiente.
Archivos: `player2_bullet.*` (fogonazo/estela), `player2_hud.*` (iconos de balas, cartuchos, particulas de vida, dano, guarda de disparo), `player2_cycle.*` (arranque, gameLoop, espera/union/demo, efectos de disparo). Test: `game/test/player2_test.cpp`.
Comando (desde la raiz del proyecto):
`c++ -std=c++17 -Wall game/test/player2_test.cpp game/player2/player2_bullet.cpp game/player2/player2_hud.cpp game/player2/player2_cycle.cpp game/src/player.cpp -o build/player2_test && ./build/player2_test`
(se ejecuta desde la raiz porque lee `baserom/SCES_504.11` para comparar tablas; si falta, esas comparaciones se omiten y lo dice la salida).

| Original | Direccion | Estado | Notas |
|---|---|---|---|
| PlayerTask::bulletConst | 0x20FA90 | T | crea fogonazo (anillo A) y estela (anillo B) con indice circular de 6. GetAngleYX y la transformacion de camara (func_001B00E0 + func_001AFC00) son interfaz `BulletHost`; la normalizacion (func_001AFA3C) la hago yo con sqrt, no verificada contra la funcion del motor |
| PlayerTask::bullet | 0x20FDF0 | T | vida (fogonazo 10, estela 50), movimiento (x30 con edad 1), giro 0x1000, anulacion por gp-0x738C / gp-0x72F0. El dibujo sale como evento `drawMuzzle/drawTracer`; los parametros de dibujo de cada bloque no se reescriben |
| jtbl_002C2E20 (parte de bullet) | ELF 0x2C2E20 | T | `muzzleStyleFor`: arma 8..15 -> bloque; el test compara las 8 entradas con el ELF. Armas fuera de 8..15 usan el bloque por defecto (el original compara sin signo) |
| PlayerTask::dispBulletInit | 0x210C60 | T | |
| PlayerTask::dispBulletMoveOn | 0x210CE0 | T | el original lee D_51F270[i+desplazamiento] sin acotar (hasta 16); yo repito la tabla cada 9 (los 9 valores de y son iguales entre jugadores); jugador 1 lee fuera de la tabla, no verificado |
| PlayerTask::dispBulletLoop | 0x210DB0 | T | solo logica: deslizamiento de iconos (4 fotogramas), numero cuando bullet >= 10, aviso de recarga (flag58 % 60 < 45, >= 100 capa fuerte), sonido 0x20017 (param 0x40) y contador +0x58. La parte de SprPutCE y las coordenadas de pantalla no se reescriben. GetPlCartType (D_00274CB0) solo elige el sprite del icono: no modelado |
| PlayerTask::dispCartridgeInit | 0x210680 | T | |
| PlayerTask::dispCartridgeBorn | 0x2106D0 | T | orden de llamadas a nlRand verificado contra el asm (angulo, vel. angular, vy, vx); `rand` inyectado. Armas 11 y 13 en tienda: vx x6 y edad inicial 90 |
| PlayerTask::dispCartridgeLoop | 0x210990 | T | gravedad -2e-5, vida 0x79 fotogramas, anillo de 18 |
| PlayerTask::dispLifePtclInit | 0x211C50 | T | |
| PlayerTask::dispLifePtclBorn | 0x211DE0 | T | m2c tenia mal el orden de llamadas a nlRand y varias constantes; esta version sale del asm (6 llamadas) |
| PlayerTask::dispLifePtclLoop | 0x211FE0 | T | muere con edad >= 0x51 o escala <= 0 |
| PostPlayerTask::dmgEftPlayer | 0x2134D0 | T | temblor: 0.025*(48*t)*sin(t*0x1800); el seno (func_001B09B4, angulo en 1/65536 de vuelta con mascara 0xFFFF) es `Sin16Fn` inyectado; en el test uso std::sin, el seno real del motor es un polinomio de VU0 sin comparar |
| PostPlayerTask::exec | 0x213450 | T | fija gp-0x73F4 segun s16 0x500DF8 (2 -> 2, 1 -> 0/1 segun 0x500734 en {5,7}, 0 -> -1, otro: no toca) |
| PrePlayerTask::exec | 0x2132B0 | T | pone/quita el bit 0 de gp-0x72D0 (camara en estado 1, gp-0x73D8 != 7, 0x500F14 != 9, bits 0x24 apagados) |
| PlayerTask::dispSight | 0x20F4C0 | T | solo la condicion de dibujo (`sightVisible`); dispSight_core es dibujo |
| PlayerTask::startInit (completa) | 0x20EBE0 | T | tabla ELF 0x274AF0 de 6 filas utiles (flags, modo, valor), comparada con el ELF. Hace ademas lo que el `startInit` de game/src/player.cpp omite: vigencia de dmgEff, invulnerabilidad gp-0x7410[pl], +0xB4/+0x1C/+0x20/+0x50/+0x5C, 7 bytes +0x340 si gameKind == 1, dispBulletInit, +0x54/+0x58, 12 flags de fogonazo/estela y el indice. **Observacion**: ese `startInit` hace `kind & 3`, pero `demoInit` usa kind 4 (fila {60, 9, 75}, modo 9); no lo he tocado |
| PlayerTask::init | 0x20DCC0 | T | despacha por modo con `__ptmf_scall(D_274960 + 0x1C*modo)`; esa tabla esta a 0 en el ELF (se rellena al ejecutar), asi que el destino de cada modo NO esta recuperado: `CycleHost::dispatchModeInit` |
| PlayerTask::firstGameInit | 0x20DD00 | T | |
| PlayerTask::contGameInit | 0x20DD70 | T | gp-0x73C6 -3 con piso 0 (s16 con signo); AddPlayerScore(pl, 1) via `PlayerLogic::addScore` |
| PlayerTask::stageGameInit | 0x20DE40 | T | |
| PlayerTask::joinGameInit | 0x20DE80 | T | func_A12120 / func_A12270 (overlay ORGMOD) quedan como `orgJoinIn`: A12270 es `setDispScoreJoinIn` de extras; A12120 no analizada |
| PlayerTask::gameLoop | 0x20DF60 | T | todo el flujo de decision (setMaxBullet, mira, vida, arma org vs gunExe, gp-0x72A0, muerte -> modo 4 con 0x500DF8--, saltos de dibujo en 0xA, lifePrev solo al dibujar, demo). gunExe/func_A11490/func_A11430/dibujo entran por `CycleHost` |
| PlayerTask::joinWaitInit | 0x20E9A0 | T | |
| PlayerTask::joinWaitLoop | 0x20EA10 | T | envoltorio de joinWaitLoop_main (cubierto por `joinWaitInit`/`joinWaitLoopMain`) |
| PlayerTask::joinWaitLoop_main | 0x20EA30 | T | usa checkCredAndStBtn de flow_rest como callback |
| PlayerTask::waitInit | 0x20EB00 | T | |
| PlayerTask::waitLoop | 0x20EB30 | T | |
| PlayerTask::demoInit | 0x20EBA0 | T | |
| PlayerTask::commonLoop | 0x2121F0 | T | |
| GunMgrClass::read_VB | ELF (extras) | - | cuerpo vacio en el asm (`jr ra`), no hay nada que reescribir |

Ayudas que no son funciones originales (no se cuentan arriba): `PlayerCycle::onShot` reune lo que gunExe (0x20F770) llama al disparar (bulletConst, sonido 0x2000C/0x2000D con parametro 0x40, cartucho en el icono min(bala,8) solo con pantalla modo 2 y sin balas infinitas, dispBulletMoveOn siempre); `PlayerCycle::onReload` = el dispBulletInit de reloadExe (0x20F5F8); `HudLogic::layoutInit` = parte del constructor que rellena D_51F270 (x = -/+0.0145, y = (-0.0075 + 0.001 i) - 0.0001, z = -0.02, constantes leidas del asm).

## Pendiente (no incluidas)
| Original | Direccion | Motivo |
|---|---|---|
| PlayerTask::dispLife | 0x2114A0 | dibujo de corazones; solo el nacimiento de particulas (nlRand()%10 == 9) es logica y depende de coordenadas de dibujo |
| PlayerTask::dispLifePtclCalcVelX | 0x211CA0 | depende de la matriz de la camara (func_001B00E0/func_001AFC00); `LifeParticlePool::velX` entra desde fuera |
| PlayerTask::dispLifePtclPut / dispCartridgePut / PutForEntry | 0x2120C0 / 0x210AF0 / 0x212230 | solo dibujo |
| PlayerTask::__ct__ / __dt__ y Pre/PostPlayerTask::__ct__/__dt__ | 0x20D990 / 0x213870 / 0x213200 / 0x2133A0 | construccion de tareas del planificador (ptmf, newExec); solo se reescribio la parte del layout de iconos |
| PlayerTask::getInput / gunExe / reloadExe | 0x20F590..0x20FA90 | ya en game/src/player.cpp (no repetidas). Falta alli lo que hace gunExe con el motor: ver `onShot`/`onReload` aqui, y la escritura de `1 << (6 - jugador)` en el byte 0x50064C |
| tabla ptmf D_274960 (modo -> rutina Init) | 0x274960 | a 0 en el ELF; hay que leerla de la memoria en ejecucion |

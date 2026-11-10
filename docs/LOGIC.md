# Mapa de la logica del juego

Direcciones y simbolos leidos de la symtab del ELF (`baserom/SCES_504.11`) y del asm de `asm/`.
`main` = segmento principal (0x140000..); el resto son overlays de `disc/TBL/<MOD>.BIN` (direcciones 0x92xxxx..0xA2xxxx,
se solapan entre overlays: cada boss `bsNmod` se carga en 0x9c4xxx). Lo marcado **(inferido)** es deduccion por nombres/llamadas, no lectura completa.

## 1. Capa de datos / recursos (main)

| Clase | Direcciones | Que hace |
|---|---|---|
| `EntryDatClass` | init `0x2188F0`, exec `0x218900`, execCore `0x218D20`, entry*/free* `0x218FB0..0x2197B0` | Cola circular de peticiones `DATAENT_REQ`: indice de lectura en `+0x1002` (halfword) y de escritura en `+0x1000` (`init` los pone a 0; `exec` mira si `wr != rd + 0x100`; `incReqRdPtr/WrPtr` `0x218F70/0x218F90`). `execCore` despacha por tipo (`sltiu <0x1F`, tabla `jtbl_002C2F10`) a los demas gestores. Pares `entryX/freeX` por tipo de dato: BankObj, BankTex, BankMot, BankSpr, BankCamPath, EnVtx, OneSkin, OskName, OskBank, FaniData, FaMotData, ClVtx, Colli, SndPack, Bufram, OvlMod (`0x2197B0`, carga de overlays: gamemod, bossN...). |
| `PolyMgrClass` | init1st `0x213930`, init `0x213B70`, entryBankObj `0x214390`, freeBankObj `0x214680`, freeAllObj `0x2146D0` | Gestor de modelos/poligonos. `entryBankObj_STAT_00..05` (`0x213CC0..0x214200`) son los estados de una maquina de carga asincrona (lectura del banco, relocacion, registro). Llamado desde `EntryDatClass::execCore` (objeto en `gp-0x7334`, `jal entryBankObj__12PolyMgrClassFP11DATAENT_REQ`). |
| `MotMgrClass` | init `0x1FBA50`, entryExec `0x1FBEF0`, freeExec `0x1FBF70`, entryBankImm `0x1FC070`, getMotAnmAdr `0x1FC2B0`, getMotAnmAdrFcv `0x1FC370`, entryStage `0x1FC3E0` | Gestor de bancos de movimiento (animaciones). Misma carga por estados (`entryExec_STAT_00/005/01/02`). Objeto en `gp-0x7234`. `getMotAnmAdr*` devuelve la direccion de una animacion por id. |
| `SprMgrClass`, `TexMgrClass`, `TexBufMgrClass`, `SoundMgrClass` | ver `asm/` | Mismo patron entryExec/freeExec para sprites (`gp-0x7438`), texturas, buffers de textura y paquetes de sonido. |
| `OneSkinMgrClass` | init1st `0x1FC7A0`, entryBodyNo `0x1FD070`, entryOskName `0x1FD5D0`, entryOskBank `0x1FDAF0`, ctrl `0x1FE650`, attach `0x1FE890`, partsChange `0x1FECE0`, changeLowModel `0x1FED60`, changeNormalModel `0x1FF370`, changeEndingModel `0x1FF880`, changeShadowModel `0x1FF9D0` | "One skin" (osk): malla unica con pesos por hueso para personajes. Carga por cuerpo/nombre/banco con STAT_xx; `data_set/tbl_set/p_mat_set/vertex_set` (`0x1FE010..0x1FE5C0`) construyen los buffers; `changeXxxModel` cambia el LOD (bajo/normal/final/sombra). |
| `ColliMgrClass` | init `0x1D30F0`, entryExec `0x1D3430` | Carga de datos de colision (estados Init/SeekEndWait/ReadEndWait/End). La logica de impactos esta en `ColliCtrlTask` (gamemod). |

## 2. Cuerpo / movimiento (main)

* `BodyMgrClass` (`0x1F89D0..`): `ctrl` `0x1F8D50` y `put` `0x1F9D60` recorren los cuerpos; `setMotPtr` `0x1F9FE0`, `motChange` `0x1FA540`
  (cambio brusco), `motChangeDivKeep` `0x1FA5D0`, `motSupplement*` `0x1FA640..0x1FA8F0` (mezcla/suplemento de movimiento),
  `motDivOnce/setDivNum` `0x1FAA60/0x1FAB80` (interpolacion dividida en N frames), `calcFrameNum` `0x1FAC90`, `putMat_rcv` `0x1FAD60`
  (matrices de huesos), `clrBodyMove` `0x1FB410`.
* `MotObjTask` (`colliFunc` `0x1F9D50`, `prtsDispFunc` `0x1FB820`, `oskDispFunc` `0x1FFE70`, `oskAttachFunc` `0x1FFEA0`) son los callbacks
  que cada entidad registra: colision, dibujo por partes, dibujo del OneSkin y enganche de objetos. Todos los enemigos replican estos
  cuatro nombres (`EnZombieTask::oskAttachFunc` `0x959DD0`, `colliFunc` `0x95ABD0`, `EnVampireTask::colliFunc` `0x955770`, `PeopleTask::colliFunc` `0x20C990`...).
* `FaceAnimMgrClass` (`0x1E5700`): animacion facial (FaMotData).
* `CFcvTRS`, `FcvEffect`, `CFcvTRSBase`: curvas de animacion (traslacion/rotacion/escala) y efectos asociados.

## 3. Estados del juego

* `MainModeMgrClass`: `next` `0x1F2FF0` (si el siguiente modo en `+6` es `-1`, lo lee de una tabla de bytes indexada por el actual en `+4`,
  puntero en `D_0026E18C`), `synch` `0x1F3020` (llama `is_PressStart(0/1)` e `is_PressDecide(0/1)`; si hay pulsacion limpia `0x5004D8` y reinicia
  el render env: salto de pantallas/demos), `chgInit` `0x1F3210`, `synch2` `0x1F32F0`, `init` `0x1F3C70`.
* `PauseMgrClass` (`0x1F3330..`): `set` `0x1F3390`, `get` `0x1F33B0` (candidatas triviales ya con match auto), pila de estados
  `pushStat/pushSetStat/popStat` `0x1F36B0/0x1F3700/0x1F3750`, `reqPauseUntilAllDataEntry` `0x1F3790` y `reqPauseUntilOvlModEntry` `0x1F37D0`
  (pausa hasta que `EntryDatClass` termine de cargar: une carga asincrona y estados).
* `StatSynchClass` (`chgInit` `0x1E25F0`, `set` `0x1EA9A0`) sincroniza cambios de estado entre tareas.
* `PlayerTask` (main `0x20D990..`): maquina de estados por pares Init/Loop: `firstGameInit` `0x20DD00`, `contGameInit` `0x20DD70`,
  `stageGameInit` `0x20DE40`, `joinGameInit` `0x20DE80`, `gameLoop` `0x20DF60`, `contInit/contLoop` `0x20E2F0/0x20E3C0` (continuar),
  `gmoverInit/Loop` `0x20E670/0x20E780`, `nameentInit/Loop` `0x20E820/0x20E890` (alta de nombre/ranking), `joinWaitInit/Loop`
  `0x20E9A0/0x20EA10` (entrada del 2P), `demoInit` `0x20EBA0`, `startInit` `0x20EBE0`, `checkCredAndStBtn` `0x20EE30` (credito + boton start).
* `EventCtrlTask` (114 metodos, `0x1E29F0..`): interprete del script de eventos; cada opcode es un metodo (`WLCTB` `0x1E2E80`, `ITEMSET` `0x1E3B00`, ya con match a mano en
  `src/matched/event_ctrl.cpp`). Su `pc` esta en `+0xC`. Dirige la secuencia de fase: spawns (`EnemyMgrClass::entryEvent*` `0x935380..0x9356C0`,
  `entryEvent2_MENM/RAND`), camara, dialogos (`NpcTask::*_Job`), cambios de modo.

## 4. Enemigos y personajes

* `EnemyMgrClass` (gamemod, `entryEvent` `0x935380`, `entryEvent2` `0x935420`, `skipEvent` `0x9356C0`): crea enemigos a partir de eventos.
* `EnZombieTask` (gamemod, 114 metodos, `__ct` `0x9572A0`): ciclo `Ctrl_Pre` `0x958260` / `Ctrl_Put` `0x958360` / `Ctrl_Pos` `0x959200`;
  dano `DamageCheck` `0x95AFD0`, `Damage` `0x95B640`, `DamageMotSet` `0x95BB60`; muerte `Dead_Sub/Dead/DeadMotSet` `0x95BF40/0x95C230/0x95C8A0`; IA
  `CheckCom` `0x95DA60`, `AttackPlayerSelect` `0x95E0C0` (elige a que jugador ataca), `DamageToPlayer` `0x95E500`, `AttackPosSet` `0x95E630`;
  contadores `DecNowEnemyAlvNumChk/DecNowEnemyNumChk` `0x95E4A0/0x95E4D0`.
* `EnVampireTask` (gamemod, `__ct` `0x952170`): `checkDamage` `0x953410`, `sufferDamage` `0x953D90`, `decLife` `0x9542B0`, `changeDeadMot` `0x954370`,
  `getParam` `0x954500` (parametros por tipo), `turnTarget` `0x954BD0`, `damagePlayer` `0x955410`, `Attack_Eknd` `0x955E00`, `Dead` `0x956810`,
  luces locales `LocalLightOn/Off` `0x956B80/0x956D40`.
* `PeopleTask` (main, 50 metodos, `__ct` `0x204F20`): los civiles. Estados parasito/vampiro/superviviente (`PreDispParasite` `0x2062A0`,
  `PreDispVamp` `0x2074A0`, `PreDispSurvive` `0x208230`), conversion `changeSurvive` `0x20B000` / `changeVampire` `0x20B150`,
  `checkParaDamage/checkVampDamage` `0x20A530/0x20A7A0`, `damagePlayer` `0x20A250`, y contadores de rescate:
  `addPeopleNum` `0x20AF60`, `addRescuePeopleNum` `0x20AF90`, `lifeUp` `0x20BCA0`, `helpMe` `0x20C180`. `ErsPplTask` `0x20CCE0` los borra.
* `NpcTask` (gamemod, 69 metodos): personajes de cinematicas, funciones `*_Job` por escena (`st3bs01_00b3c_Job` `0x974110`, `b6_bs01_00_mae_Job` `0x974C00`,
  `lnks_st6sc03b6_Job` `0x974DF0`...), `faceAnim` `0x973540`, `footEffect` `0x973930`.
* Jefes: un overlay por jefe cargado por `EntryDatClass::entryOvlMod`: `Boss2Task` (bs2mod, 101 metodos, `0x9c4b50`), `Boss3Task` (bs3mod, 86, `0x9c4880`),
  `TBoss4` (bs4mod, 145, `0x9c4880`), `TBoss5` (bs5mod, 106, `0x9c4900`), `Boss6Task` (bs6mod, 85, `0x9c4880`), mas sus clases de efectos/proyectiles
  (`TBoss3Bomb`, `TBoss3Laser`, `TBoss6XBat`, `TBoss6XBullet`, `TBoss5Ice`, `TSwordFire`...). Las direcciones coinciden entre overlays porque se cargan en el mismo hueco.
* `Komono2Task`/`Komono3Task`/`TrnKomono*` (aomod / main): enemigos menores de las fases ("komono"), con `KomonoTumiageColliTask` para apilables.

## 5. Armas, colision y puntuacion

* `GunMgrClass` (main, `0x1EDA10..`): `read_SYNCH_pad` `0x1EDBA0` (mando), `read_SYNCH_gun1/gun2` `0x1EDEA0/0x1EDFF0` (pistolas GunCon2, jugadores 1 y 2),
  `read_SYNCH` `0x1EE0D0`, `reset_SightPos*` `0x1EDA90/0x1EDB40` (mira), `orgAddTrig/orgAddTrig0` `0x1EE280/0x1EE210`, `adjustVal_cz` `0x1EE4B0`.
* `PlayerTask` arma y vida: `gunExe` `0x20F770`, `reloadExe` `0x20F590`, `bulletConst` `0x20FA90`, `bullet` `0x20FDF0`, `setMaxBullet` `0x20F280`,
  `setMaxLife/setInitLife/setLifeStart` `0x20F060/0x20F0E0/0x20F260`, `dispSight` `0x20F4C0`, HUD de cargador `dispCartridge*` `0x210680..0x210AF0` y vida `dispLife*` `0x2114A0..`.
* `ColliCtrlTask` (gamemod, `Ctrl` `0x92B150`): resuelve disparo contra objetos: `Judge` `0x92B490`, `Shoot_VS_Ball` `0x92B6D0`, `Shoot_VS_FixBox/MoveBox`
  `0x92BDB0/0x92BF70`, `Parts_Check*` `0x92BA70..0x92BBD0`, `Shoot_Hit*` `0x92BE90..0x92C530`, `WeakSearch` `0x92CC20` (punto debil),
  `HitMarkConst` `0x92CF70`. Los enemigos reciben el resultado en su `colliFunc` y lo traducen a `DamageCheck/checkDamage`.
* Puntuacion y resultado: `ResultTask` (gamemod, `__ct` `0x97CBB0`, `loop` `0x97D0F0`): `GetNumRescuedPeople` `0x97DC70`, `GetNumTotalPeople` `0x97DCA0`,
  `GetNumAccuracy` `0x97DCD0`, `GetNumScore` `0x97DD90`, `GetRateRescued` `0x97DE20`, `GetRateAccuracy` `0x97DEC0`, `GetNumLifeUp` `0x97DF70`
  (rescates + precision + score -> vidas extra). Ranking: `ClearRankingTask` (main `0x226730`), `IraiNote` (peticiones/misiones, `0x224A20`), `MojiUchi` (texto, `0x223840`).
* `HuntFileTask` (hntmod, `0x9C5330..`): libreta de caza/expedientes (selector, paginas, fotos).

## 6. Relaciones (resumen)

```
MainModeMgrClass --(modo)--> PlayerTask (gameLoop/contLoop/gmoverLoop/nameentLoop)
EventCtrlTask --(script)--> EnemyMgrClass::entryEvent* --> En*Task / BossNTask / PeopleTask / NpcTask
EntryDatClass (cola DATAENT_REQ) --execCore--> PolyMgr / MotMgr / SprMgr / TexMgr / OneSkinMgr / ColliMgr / SoundMgr / entryOvlMod
GunMgrClass.read_SYNCH --> PlayerTask.getInput/gunExe/bullet --> ColliCtrlTask.Shoot_* --> colliFunc -> DamageCheck / Dead
PeopleTask.addRescuePeopleNum + PlayerTask (vida/balas) --> ResultTask.GetNum* --> ClearRankingTask
BodyMgrClass.motChange/motSupplement + MotMgr.getMotAnmAdr --> OneSkinMgr.ctrl/attach (dibujo)
```

## 7. Matches generados en esta pasada

`tools/vn_small2.py` (copia de `vn_small.py` que escribe en `src/auto2/` y `build/small2_report*.json`, sin pisar `src/auto/`) con `--max-instr 60 --permute 40`
verifica contra el ELF y `tools/vn_export_matched.py` reparte cada resultado en `src/matched/<modulo>_<clase>.cpp`
(un fichero por clase; cada funcion con su nombre real y direccion en el comentario). Nota: `probe_compilers.py` no sirve para esos ficheros
porque las funciones se llaman `f_<DIR>_<modo>`, no por simbolo; la verificacion autoritativa es la comparacion byte a byte (relocaciones enmascaradas) de `vn_small2.py`.
Para reproducir: `.venv/bin/python tools/vn_small2.py --module <mod> --max-instr 60 --permute 40` y despues `.venv/bin/python tools/vn_export_matched.py`.

## Reescritura a mano por sistemas (game/, 2026-11-09)
Decision del usuario: reescribir la logica a mano por sistemas (no recompilacion estatica ni m2c en bloque). Flujo: m2c (`tools/m2c/m2c.py -t mips-mwcc-c`) da un primer borrador en build/m2c/ -> se reescribe en C++ limpio en `game/src/` con nombres inferidos, sin dependencias de hardware, citando la funcion original -> test unitario en `game/test/` con valores sacados del ELF/asm.
Limites: los globales de sistema (0x500xxx, 0x502xxx, gp-0x7xxx) viven en bss y no estan en el ELF; se modelan como configuracion (`PlayerConfig`). m2c no decompila los ficheros con MMI/VU (p. ej. PlayerTask 0x20F590 reloadExe/gunExe/bullet, 0x210AF0 HUD).
Hecho: `game/src/player.*` = setMaxLife, setInitLife, setMaxBullet, setLifeStart, changePlayerMode, startInit (parte de estado), tick de vida de gameLoop; test `build/player_test` (c++ -std=c++17 game/test/player_test.cpp game/src/player.cpp). Tabla ELF 0x274AF0 (flags, modo, valor): {61,5,75} {25,5,150} {8,5,75} {61,5,150}.
Pendiente de PlayerTask: reloadExe, gunExe, getInput, bullet, bulletConst (no decompilan con m2c; leer asm), daño (PostPlayerTask::dmgEftPlayer), puntuacion (AddPlayerScore), continuar/gameover/nameent.
Tambien hecho: `PlayerLogic::gunExe` (+getInput): el disparo descuenta bala, cuenta disparos, marca cargador vacio, pide recarga al soltar el gatillo con balas gastadas; modo 8 o gp-0x72D0&0x10 = balas infinitas. Consejo tecnico: m2c falla en los ficheros con opcodes VU falsos (`vsqi/vlqd`): se neutralizan con sed (`build/m2c/pt_*.s`) y falla en funciones con tabla de saltos (reloadExe, bullet): hay que leerlas del asm.
Ampliado (jugador, `game/src/player.*`): `reloadExe` (recarga; sonido 0x20027 o, en modo tienda por objeto 8..15: 0xB0113,0xB0113,0xB010F,0xB0115,0xB0117,0xB0119,0xB011B,0xB011D), `addScore` (objetos 47 = x2, 48 = x4; no baja de 0), `addHitNum`, `hitBonus` (10*aciertos/disparos -> tabla ELF 0x274F00 = 0,500,1000,...,4000,5000,8000), `canDecLife`, `decLife` (invulnerabilidad 75 fotogramas, -100 puntos, objeto 0x1B salva con prob 1/min(vida+1,5), contador gp-0x73BC baja 1 o 2), `incLife` (al maximo da +1000 puntos). Tests en game/test/player_test.cpp. Faltan del jugador: bullet/HUD (dispLife, cartuchos: solo dibujo), continuar/game over/nombre (flujo de pantallas).

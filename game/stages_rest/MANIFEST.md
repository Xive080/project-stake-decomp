# game/stages_rest: resto de escenarios (tanda 2)

Verificacion: lectura del asm + `game/test/stages_rest_test.cpp` (88 comprobaciones). NO hay ejecucion diferencial contra el original:
los tests comprueban que el codigo hace lo que el asm dice (segun mi lectura) y que las tablas coinciden con ORGMOD.BIN / TRNMOD.BIN.

Compilar y ejecutar (desde la raiz del repo):
`c++ -std=c++17 -Wall game/test/stages_rest_test.cpp game/stages_rest/*.cpp game/stages/org.cpp game/stages/org_tables.cpp game/stages/komono.cpp game/stages/training.cpp game/stages/training_tables.cpp -o build/verify_stages_rest/stages_rest_test && build/verify_stages_rest/stages_rest_test`

## Reescritas (34 filas)

| Funcion | Direccion | Estado | Notas / sin verificar |
|---|---|---|---|
| PlayerTask::gunOrgBulletSet | 0xA11440 | reescrita+test | gunOrgSoundOff (0xA11430) vacia, no se modela |
| PlayerTask::gunOrgExe (todas las armas, sonidos por jtbl 0xA12BB0, tabla D_00A12300) | 0xA11490 | reescrita+test | tabla comparada con ORGMOD.BIN. Armas fuera de 0..15 usan la fila 0 (el original indexa sin acotar). Efectos de motor (func_20FA30, 210C60, 1D39F0...) son la interfaz GunOrgHost; su efecto interno no se reescribe. Rafagas 13/11 y dispersion 10 probadas solo en casos sueltos |
| OrgPlWeaponDamageGet | 0x942F00 | reescrita+test | delega en OrgLogic::weaponDamage ya existente |
| DecLife | 0x9501C0 | reescrita+test | marca 0x346 => dano = vida entera; invulnerable 0x1000 |
| Ray_VS_BallColli (solo la decision de impacto) | 0x1D3B0C | reescrita+test | los senos/cosenos del rayo (Ent_ShotRay) los calcula el motor: NO reescrito |
| Komono3Task::checkHit | 0x9ED2F0 | reescrita+test | |
| KomonoTumiageColliTask::chkColli / getHitPl / clrColli / close / ctrlPre | 0x9E75F0 / 0x9E7630 / 0x9E76F0 / 0x9E7710 / 0x9E7740 | reescrita+test | 5 filas (cinco funciones) |
| KomonoTumiageColliTask::setPos / setRadius | 0x9E7720 / 0x9E7730 | reescrita sin test | |
| secuencia comun Komono3 (checkGameMode, checkMissionEnd_setTrnsl, checkMissionCompleteMessageDispTiming) | ver stages/komono | reescrita+test (common3) | usa KomonoLogic existente |
| Komono3 comun completa (+ checkCloseTiming_clrBranchNo + checkColli_setBranchNo) | 0x9ED5E0 | reescrita sin test | se ejercita solo via goldenRat/secretDoor/nuts |
| Komono3Task::nutsCtrl | 0x9EE6D0 | reescrita+test | el paso de fotograma +1 y las ventanas de sonido 0xC000F: con +1.0 caen dos fotogramas por ventana (56,57 y 61,62); sale del asm, no probado en juego |
| Komono3Task::goldenRatCtrl | 0x9EE0D0 | reescrita+test | rehecha tras ver los nop del delay slot |
| Komono3Task::secretDoorCtrl | 0x9EEAF0 | reescrita+test | rotacion de puerta por camara (0x17A) solo en codigo, no en test |
| Komono3Task::silverKinokoCtrl (parte logica) | 0x9EF0F0 | reescrita sin test | |
| KomonoKusariHahenTask::loop | 0x9F20F0 | reescrita+test | |
| KusariTask::ctrl (decision) | 0x9F2680 | reescrita+test | generacion de escombros = evento |
| CKomonoBase::bornItem (construccion de la tabla) | 0x9DF140 | reescrita+test | peso float truncado a int |
| TrainingTask::judge | 0x9E4A20 | reescrita+test | 17 misiones por jtbl_009F4FF0. DISCREPANCIA: el MANIFEST de stages dice que trn_09 usa enemyAlive; en el asm que lei, el indice 6 usa enemyAlive y el 8 usa norm. No reconciliado |
| TrainingTask::failed | 0x9E69B0 | reescrita+test | |
| TrainingTask::retry | 0x9E6AD0 | reescrita+test | |
| TrainingTask::exit | 0x9E6BE0 | reescrita+test | |
| TrainingTask::mc_save | 0x9E6E00 | reescrita+test | |
| TrainingTask::start_init (cabecera) | 0x9E3650 | reescrita+test | solo el cambio de mision; la carga de recursos no |
| TrainingTask::success, subestado 0 (progreso de rango) | 0x9E5A90 | reescrita+test | |
| TrainingTask::success, subestado 1 (calificacion) | 0x9E5A90 | reescrita+test | |
| trn_10_init | 0x9F2200 | reescrita+test | |
| trn_10 | 0x9F22E0 | reescrita+test | |
| OriginalTask::shop_buy | 0x9C7390 | reescrita+test | sale solo la decision; ShopScriptDisp/shop_put_icon son eventos |
| OriginalTask::mode_select | shpmod_text.s lineas 1396-1612 | reescrita+test | leida una vez; NO releida tras el test |

## Hallazgos
- Base de TRNMOD: offset = direccion - 0x9DF000 (no 0x9DF080). Con esa base, `kTrnSeconds` (existente) coincide: en el fichero real son 48 entradas de 4 bytes
  `{segundos, -1, 3, -1}` (indice mision*3+nivel); el existente guarda solo el campo 0. Los otros tres campos (constantes -1, 3, -1) tienen uso desconocido.
- gunOrgExe: el consumo de municion y el cooldown (+0x340) salen de la tabla; ver comentarios en gunorg.cpp.

## Pendiente (no reescrito)
- KomonoTumiageTask::ctrl*, KomonoRigidTask::ctrl*.
- Controles de Komono3 no cubiertos: zaisan, makimono, hitotsu, dog, goldenFish, goldenKubiwa, agroom, hasira, taruBoy, uroko A/B, brooch, kinoko,
  maboroshi, silverGrass, goldenBorn, nuigurumi y los de st2/st3.
- Trn05/08/10KomonoMgrTask::ctrl, CTrn05/CTrn08Komono::ctrl.
- success: subestados 2-4, grados 4/5/6 y guardado por jtbl_009F50D0; select / select2; trn_free0.
- shop_buy_open_book/open_book2, shop_melt y sus libros, equipment*; mode_select_colli y shotsw_numcheck (se reciben ya resueltos).
- Ent_ShotRay (calculo de senos/cosenos del rayo) y la geometria de Ray_VS_*.
- OrgAddSilver/OrgAddItem/gunOrgBulletSet: ya existian (stages/org, OrgLogic::addSilver/addItem); no se duplican.

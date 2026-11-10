# game/extras MANIFEST

Direcciones: ELF main (SCES_504.11) o overlay (shpmod base 0x9C4800, optmod 0x92B000, orgmod 0xA0F800, hntmod).
Estado: T = reescrita+test, S = reescrita sin test, P = pendiente.
Todas las reescritas T tienen test unitario en game/test/extras_test.cpp; los valores salen del asm, y las tablas
(precios, atributos, dificultad) se comparan ademas byte a byte con disc/TBL/*.BIN y el ELF (se omite si faltan).

| Original | Direccion | Estado | Notas |
|---|---|---|---|
| GunMgr init/start | ELF 0x500580 (obj) | T | fisica de campos segun asm |
| resetSightPosPl / resetSightPos | ELF | T | |
| adjustVal_cz | ELF | T | para dispositivos distintos de 2/3 el original usa registros sin inicializar; yo devuelvo 0 |
| readSynchPad | ELF | T | velocidad 1+0.8*((cfg%11)+1), zona muerta 0x61..0x9F |
| readSynchGun1 / readSynchGun2 | ELF | T | gatillo GunCon con retardo de 4 frames; calibracion |
| orgAddTrig0 / orgAddTrig | ELF | T | rafaga de armas 11/13, cooldown 0..24 |
| readSynch | ELF | T | |
| silverSub / silverAdd | shpmod 0x9C6880 / 0x9C6900 | T | tope 999999 |
| calcPrice | shpmod 0x9CC0C0 | T | el original no comprueba limites; yo devuelvo 0 fuera de 0..51 |
| checkGetAnotherPlayerEquip (otherEquipCount) | orgmod 0xA11FE0 | T | |
| unequipAnotherPlayerEquip (otherUnequip) | orgmod 0xA12080 | T | |
| checkExistItem | shpmod 0x9C7F40 | T | |
| trnsIconToEquipType | shpmod 0x9C8020 | T | |
| checkItemNum | shpmod 0x9CAB90 | T | |
| decideItemNum | shpmod 0x9CAD50 | T | guarda de 400 iteraciones que no existe en el original |
| unEquipTool | shpmod 0x9CAF50 | T | |
| setEquipmentItem | shpmod 0x9CAFC0 | T | |
| OrgAddItemG | ELF 0x1D3820 | T | |
| OrgAddShopItemG | ELF 0x1D3880 | T | |
| setCredit | shpmod 0x9CC4F0 | T | |
| setDifficulty | shpmod 0x9CC600 | T | |
| setDispScore | shpmod 0x9CC700 | T | |
| countUnsolvedReq | shpmod 0x9CDD80 | T | significado de bit0/bit1 de reqFlags sin validar |
| countRemainReq | shpmod 0x9CDDC0 | T | idem |
| shotswPlCheck | shpmod 0x9CB9E0 | T | |
| dirswCheck | shpmod 0x9CBAE0 | T | siempre 0 |
| HFileAddPageG (addPage) | ELF 0x1D38B0 | T | yo compruebo limites; el original no |
| addOtherGallery | ELF 0x1D3950 | T | |
| checkBookComp | hntmod 0x9C55D0 | T | |
| checkNoData | hntmod 0x9C7710 | T | |
| calcLinePos | hntmod 0x9C5AD0 | T | |
| getSightPos | optmod 0x933260 | T | |
| calcBgmBalance | optmod 0x92B5D0 | T | |
| calcVoiceBalance | optmod 0x92B6F0 | T | |
| chgConfigBitR | optmod 0x9384A0 | T | |
| setVibrationSysCfg | optmod 0x9367B0 | T | |
| Set_SysCfg_To_CsOptData (sysCfgToMenu) | optmod 0x92C960 | T | nombres de SysCfg +9/+0x15 sin validar |
| gamesystemDefault | optmod 0x935490 | T | |
| lifeDefault | optmod 0x935E70 | T | |
| difficultyChoose | optmod 0x935520 | T | |
| lifeAdjust | optmod 0x935B80 | T | |
| creditsAdjust | optmod 0x935EB0 | T | |
| balanceAdjust | optmod 0x939450 | T | |
| sightSpeedAdjust | optmod 0x936D70 | T | |
| checkSysCfgBak | optmod 0x93B310 | T | |
| copySysCfgBak | optmod 0x93B2E0 | T | unidad (0xBD palabras) sin validar |
| SetOrgMissionOver | orgmod 0xA11EA0 | T | umbrales 30000/1000/140000 leidos del asm; significado de los bytes de flags no validado |
| setDispScoreJoinIn | orgmod 0xA12270 | T | |
| OrgPlWeaponDamageGet(_core), DecLife, OrgEnemyLifeScale, OrgAddSilver, OrgAddItem, OrgShopLevel | orgmod 0xA10BF0..0xA113D0 | P | leidas pero no reescritas ni testeadas |
| gunOrgBulletSet / gunOrgExe | orgmod 0xA11440 / 0xA11490 | P | solo analizada en parte |
| unequipJoinIn, nlaInitOrgGameCredit, GetGunDevice | orgmod | P | |
| shop_buy, shop_buy_open_book(2), shop_melt_open_book(2) | shpmod | P | flujo de transaccion mezclado con interfaz |
| CheckInput, loop, Put* | hntmod | P | dibujo/interfaz |
| loop, configurationPutSub y demas flujo de menu | optmod | P | dibujo/interfaz |
| OriginalTask::mode_select y flujo | orgmod | P | |
| mode_select_colli, put_icon, putnum, equipIconFade | varios | P | dibujo |
| Ent_ShotRay, Ray_VS_BallColli, CheckOnSc | ELF | P | colision/motor |
| ShopBook, OrgIntroMviTask, OriginalMotTask, DogKeyTask | varios | P | no abordadas |

Recuento: 48 filas reescritas con test (la fila GunMgr init/start agrupa dos funciones; 49 funciones).
Sin test especifico de la logica: ninguna reescrita sin test.

Compilar y ejecutar (desde la raiz del proyecto):
c++ -std=c++17 -Wall game/test/extras_test.cpp game/extras/gunmgr.cpp game/extras/shop.cpp game/extras/options.cpp game/extras/hunt.cpp game/extras/orgrules.cpp -o build/extras_test && build/extras_test

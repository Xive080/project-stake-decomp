# MANIFEST game/enemies (enemigos comunes)

Verificacion: tests unitarios (game/test/enemies_test.cpp) contra valores extraidos de los binarios
(ELF baserom/SCES_504.11, disc/TBL/GAMEMOD.BIN, ORGMOD.BIN) y leyendo el asm. El original NUNCA se ejecuto:
no hay comparacion de comportamiento en vivo.

Compilar y ejecutar (desde la raiz del repo):

    c++ -std=c++17 -Wall game/test/enemies_test.cpp game/enemies/enemy_common.cpp game/enemies/vampire.cpp game/enemies/vampire_tables.cpp game/enemies/people.cpp -o build/enemies_test && ./build/enemies_test

Estado: R+T = reescrita con test; PEND = pendiente.

## Reglas comunes (enemy_common)
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| OrgPlWeaponDamageGet | 0x942F00 / core 0xA10BF0 (ORGMOD) | R+T | tabla D_A12500 leida del binario; indice fuera de 0..15 no acotado en el original |
| OrgEnemyLifeScale | 0xA10C60 (ORGMOD) | R+T | siempre 1.0 (deducido del asm) |

## EnVampireTask (vampire.cpp)
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| charInit (solo vida) | 0x9530D0 | R+T | tablas de vida ByType/ByLevel/Diff1P/Diff2P extraidas; resto del init es motor |
| decLife | 0x9542B0 | R+T | |
| sufferDamage | 0x953D90 | R+T | kVampDamage 18x18x6 extraida |
| checkDamage | 0x953410 | R+T | |
| damagePlayer | 0x955410 | R+T | |
| chkParam | 0x954440 | R+T | |
| setAtcPlID | 0x954F90 | R+T | |
| checkDamageMotEnd | 0x9541A0 | R+T | necesita motLen inyectado (tabla runtime 0x1FB73E4, no esta en el ELF) |
| changeDeadMot | 0x954370 | R+T | |
| Dead | 0x956810 | R+T | |
| Attack_Eknd_Sub (attackJump) | 0x955C30 | R+T | |
| getParam (VM 21 ops) | 0x954500 | R+T | decodificada del asm (m2c falla con jtbl); tabla 0x9B56C0 |
| Attack_Eknd | 0x955E00 | R+T | 7 estados; copia de parte posExtra (marca 1DD8) en estado 5 NO hecha |
| checkDamage_Eknd | 0x953870 | PEND | variante con armadura y eleccion aleatoria |
| Wait | 0x955A40 | PEND | necesita tablas D_9A22C0 |
| changeDamageMot | 0x954000 | PEND | registro de umbral sin resolver |
| turnTarget/turnTargetSub, adjChest/adjPos, getHight | - | PEND | matematica de motor |
| FirstCtrl/LastCtrl/KneadEffect, weakInit | - | PEND | |
| display / colli | - | PEND | dibujo, fuera de alcance |

## PeopleTask (people.cpp)
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| addPeopleNum | 0x20AF60 | R+T | |
| addRescuePeopleNum | 0x20AF90 | R+T | |
| checkVampDamage | 0x20A7A0 | R+T | |
| sufferVampDamage | 0x20A8A0 | R+T | tablas kPeopleBaseDamage/PartClass/DamageMot extraidas |
| checkParaDamage | 0x20A530 | R+T | pide sonido cada frame con bit 2 activo (fiel al asm, no confirmado en juego) |
| checkVampDamageMotEnd | 0x20AEB0 | R+T | |
| damagePlayer | 0x20A250 | R+T | |
| motionSelect | 0x20A200 | R+T | |
| setAtcPlID | 0x20A030 | R+T | |
| changeSurvive | 0x20B000 | R+T | |
| changeVampire | 0x20B150 | R+T | |
| lifeUp | 0x20BCA0 | R+T | |
| groanSound | 0x20BFA0 | R+T | |
| helpMeSound | 0x20C180 | R+T | stage 1 consume un rand extra; stage 5 sin sonidos; stage 6 como 0 (fiel al asm) |
| LastCtrl (parte de espera) | 0x205D70 | R+T | solo la parte de espera |
| getParam (VM 43 ops) | 0x208C90 | R+T | op 39 con flags de item inyectados; el 0x500F04++ se aplica en VmEnv; tabla 0x2C2BC0 |
| FirstCtrl, resto de LastCtrl | - | PEND | avance de frames |
| turnTarget, backWaitPos, keepAwaySpeed | - | PEND | matematica vectorial |
| ChangeEffect, WalkNoisily, shapeAnim* | - | PEND | |
| display / colli | - | PEND | |

## Sin hacer
- ErsPplTask: el constructor es de motor/reserva; FirstCtrl/LastCtrl no revisados. PEND.
- Komono*/TrnKomono*: no examinadas. PEND.
- EnZombie y otras clases En*: no examinadas. PEND.

## Total
Reescritas con test: 31 (2 reglas comunes + 13 EnVampire + 16 People). Reescritas sin test: 0.
Cada "R+T" tiene al menos una asercion con valor sacado del binario o del asm; no se garantiza cobertura de todas las ramas.

## Sin verificar (offsets/semantica inferidos)
- Varios offsets de campos y su significado son inferidos del asm.
- La matematica de motor (geometria, matrices) no se implementa: se delega en eventos o entradas.
- Algunos comentarios de direccion en el codigo fuente son aproximados; las direcciones correctas estan en esta tabla.

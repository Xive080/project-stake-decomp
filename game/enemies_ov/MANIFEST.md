# MANIFEST game/enemies_ov (EnZombie, GAMEMOD)

Verificacion: lectura del asm (asm/gamemod) + borrador m2c, y test contra tablas extraidas de
GAMEMOD.BIN (base 0x92B000), baserom/SCES_504.11 y disc/TBL/AOMOD.BIN. El original NUNCA se ejecuto.
El test se salta con aviso si faltan los archivos.

Compilar y ejecutar (desde la raiz del repo):

    c++ -std=c++17 -Wall game/test/enemies_ov_test.cpp game/enemies_ov/zombie.cpp game/enemies/enemy_common.cpp game/enemies/vampire.cpp game/enemies/vampire_tables.cpp -o build/verify_enemies_ov/enemies_ov_test && ./build/verify_enemies_ov/enemies_ov_test

Ultimo resultado: OK: 201 comprobaciones, 0 fallos.

Estado: R+T = reescrita con test; PEND = pendiente.

## EnZombieTask (zombie.h / zombie.cpp, struct ZombieLogic)
| Funcion (zombie.cpp) | Original | Direccion | Estado | Notas |
|---|---|---|---|---|
| isAttackPlayer | is_AttackPlayer | 0x951080 | R+T | |
| angChk | AngChk | 0x95E920 | R+T | |
| eTurn | ETurn | 0x95E870 | R+T | sin wrap, puede devolver 0x10000 (fiel al asm) |
| decNowEnemyAlvNumChk | DecNowEnemyAlvNumChk | 0x95E4A0 | R+T | |
| decNowEnemyNumChk | DecNowEnemyNumChk | 0x95E4D0 | R+T | |
| atcPlFlgClear | AtcPlFlgClear | 0x95E440 | R+T | |
| deleteTask | Delete | 0x95D320 | R+T | |
| attackPlayerSelect | AttackPlayerSelect | 0x95E0C0 | R+T | |
| attackPlayerSelectAsd | AttackPlayerSelect_Asd | 0x95E2B0 | R+T | |
| damageToPlayer | DamageToPlayer | 0x95E500 | R+T | |
| damageToPlayer2 | DamageToPlayer2 | 0x961FA0 | R+T | |
| motSupSet | MotSupSet | 0x95DF20 | R+T | |
| motSupSet2 | MotSupSet2 | 0x95E000 | R+T | |
| damageMotEnd | DamageMotEnd | 0x95BCD0 | R+T | |
| weakHitClear | WeakHitClear | 0x95AF90 | R+T | |
| damage | Damage | 0x95B640 | R+T | switch por clase de arma (jtbl_009B5880) y tablas de vida leidas del binario; ZomCheckHitParts inyectado, no reescrito |
| damageCheck | DamageCheck | 0x95AFD0 | R+T | |
| fallCheck | FallCheck | 0x95D540 | R+T | |
| deadBounce | Dead_Sub (solo fisica) | 0x95BF40 | R+T parcial | la parte de efectos NO hecha |
| checkCom | CheckCom | 0x95DA60 | R+T | interprete del script s16 (ops -1, 60..66); op 60 carga PTMF de D_009A1B70 (tabla rellenada en runtime, en el fichero es 0); op desconocido: el original cuelga, el mio aborta y devuelve -1 |
| posSet | PosSet | 0x9648D0 | R+T | |
| missEnemyNumAdd | MissEnemyNumAdd | 0x9641C0 | R+T | |
| lifeSet | LifeSet | 0x9649D0 | R+T | tablas D_009B6B50/80/B0 |
| advDead | Adv_Dead | 0x964550 | R+T | |
| tutDead | Tut_Dead | 0x964740 | R+T | asm revisado |
| st2Sc03_03 | St2_Sc03_03 | 0x9643E0 | R+T | |
| st3Sc03_02 | St3_Sc03_02 | 0x9641F0 | R+T | compara curMot con sA4 (fiel al asm) |
| st1Sc01_00 | St1_Sc01_00 | 0x962D90 | R+T | NO comparada linea a linea con el asm (m2c + coherencia) |
| frmWait | FrmWait | 0x9631E0 | R+T | idem |

Total: 29 funciones reescritas con test (deadBounce es parcial; MotSupSet/MotSupSet2 contadas como 2).
Reescritas sin test: 0.

## Pendiente
- DamageMotSet 0x95.... : usa t0 sin inicializar cuando type != 0x12; no se reescribe sin decidir ese caso.
- Dead, DeadMotSet (jtbl), DamageVoice, Wait*, ApproachPlayer*, AttackPlayer*, FuncCTrace*, CharInit,
  Ctrl_Pre/Put/Pos, ZM_Sound, ZomCheckHitParts, CamWait, KeepMove, DistSet, AttachSet/AttachOff,
  ZM_HighSet, Dead_Sub (efectos), PetitZombieTask, ArmorTask, EnEtc*.
- Komono*/TrnKomono* (aomod/trnmod: KomonoTumiage*, KomonoRigid*, Kusari*, Komono2/3Task, CTrn05/08/10Komono,
  Trn*KomonoMgrTask): inventariadas (~215 funciones), NINGUNA reescrita.
- Otras clases En* de GAMEMOD no examinadas.

## Sin verificar (inferido)
- Nombres y significado de campos inferidos del uso en el asm; offsets anotados en comentarios.
- Entorno global (gp-0x7xxx, 0x500xxx) modelado como ZEnv inyectado; sonido/dibujo/colisiones como ZEvent.
- Los tests comprueban tablas contra los binarios y casos construidos a mano; no comparan con ejecucion real.

// Logica de EnZombieTask (gamemod, 0x95xxxx..0x96xxxx) reescrita a mano. Cada funcion cita la original con su direccion.
// Campos nombrados por uso con su offset en comentario. Los globales de sistema (gp-0x7xxx, 0x500xxx) no estan en el ELF:
// se modelan como estructura inyectada `ZEnv`. Las llamadas a motor (sonido, efectos, movimiento, tareas) salen como eventos `ZEvent`.
// Sin rutas absolutas ni dependencias de plataforma. Los nombres de las funciones de motor (func_xxx) son los del asm.
#pragma once
#include "../enemies/enemy_common.h"
#include <cstdint>
#include <cstring>
#include <vector>
namespace vn { namespace enov {

// ---------------------------------------------------------------------------------------------------------------------
// Eventos de motor (el motor los ejecuta en orden).
enum ZEvType : int {
    ZE_SOUND_EMITTER,     // seqRequest sobre el emisor del enemigo (+0x24): a = id
    ZE_SOUND_GLOBAL,      // func_217C30(&D_515F30, a = id, 0, 0, [0x40]): sonido global (sin emisor)
    ZE_SCORE,             // func_212FA0(a = jugador, b = puntos)  AddPlayerScore
    ZE_HITNUM,            // func_213070(a = jugador)               AddPlayerHitNum
    ZE_HITMARK,           // new_EnemyHitMark(a = parte, b = jugador, c = 0x3E8 golpe / 0x3E9 muerte)
    ZE_PARTS_CHAIN,       // EnPartsChain(a = parte, b = jugador, c = byte tabla D_009A2470[tipo]+0, d = byte +4)
    ZE_DAMAGE_PLAYER,     // DecPlayerLife_Chk(a = jugador, b = 1, c = tipo de dano)
    ZE_DAMAGE_MOT,        // DamageMotSet(a = jugador)
    ZE_DAMAGE_VOICE,      // DamageVoice()
    ZE_ATTACH_OFF,        // AttachOff(a = parte, b = 0)
    ZE_EFFECT,            // new_EnEff(a = codigo de efecto, b = parametro de nivel, pos f[3])
    ZE_EFFECT_PLAYER,     // func_1E1500(a = 0x26/0x27 segun jugador, escala 0.5, pos f[3])
    ZE_EFFECT_LAUNCHER,   // TyakudanLauncherMgrBorn(escala 0.8 = 0x3F4CCCCD, pos f[3]): arma 0xE en entrenamiento
    ZE_MOTION_SET,        // func_1FA640 (cambio de animacion): a = animacion, b = mezcla/parametro
    ZE_MOTION_SET_NOBLEND,// func_1FA540: cambio de animacion cuando la actual es la 0x43
    ZE_MOTION_LAYER,      // func_1FAA60 (capa de dano): a = animacion
    ZE_TASK_CLOSE,        // TaskCloseX(): cierra la tarea del enemigo
    ZE_CHECKCOM,          // (informativo) se invoco CheckCom: lo resuelve el llamador con `checkCom`
    ZE_MISS_ADD,          // (informativo) gp-0x7358 += 1
    ZE_CAM_EFFECT,        // func_1E1620(1.0, 0x1D, pos): efecto de camara
    ZE_DAMAGE_CHECK       // (informativo) el original llama a DamageCheck(); lo ejecuta el llamador
};
struct ZEvent { ZEvType type; int a, b, c, d; float f[3]; };
typedef std::vector<ZEvent> ZEvents;
inline void zemit(ZEvents& v, ZEvType t, int a = 0, int b = 0, int c = 0, int d = 0, float x = 0, float y = 0, float z = 0) {
    ZEvent e = {t, a, b, c, d, {x, y, z}}; v.push_back(e);
}

// ---------------------------------------------------------------------------------------------------------------------
// Entorno global inyectado (gp y 0x500xxx; viven en bss, NO estan en el ELF).
struct ZEnv {
    en::EnemyEnv base;            // gameState 0x500F14, gameKind gp-0x73B4, playerCount 0x500DF8, coop 0x500DFC, solo gp-0x73F4,
                                  // levelA gp-0x73C8, levelB gp-0x73C6, attackSlot gp-0x7420/-0x741C, pl[] (modo en D_500734+0x348*i), cam (gp-0x7338), rand
    int  mode7218 = 2;            // gp-0x7218 -> +4 (byte): modo de camara; si != 2 el enemigo es invulnerable (DamageCheck)
    int8_t attackGate = 0;        // gp-0x7360: hay un ataque en curso que bloquea elegir jugador
    int8_t flag500F07 = 0;        // 0x500F07: se activa cuando un ataque marcado (0x02000000) hiere al jugador
    int16_t missCount = 0;        // gp-0x7358: ataques que ya no se contaran (se suma en cada intento de dano)
    int16_t alive = 0;            // gp-0x7370: enemigos vivos
    int16_t onScreen = 0;         // gp-0x7374: enemigos en pantalla
    float   groundNone = 0;       // gp-0x73A8: valor "sin suelo" que devuelve EN_HighGet
};

// Tabla de dano al jugador: 10 B por registro (D_009A2370[tipo][patron] -> registros): +6 = tipo de dano (s8), +7 = mascara de proteccion (s8).
struct ZAtkRec { int8_t dmgType = 0; int8_t mask = 0; };

// Bits de +0xC (flags).
enum ZFlags : uint32_t {
    ZF_BIT2 = 0x2, ZF_HITCHECK = 0x8, ZF_INVULN = 0x100, ZF_BIT2000 = 0x2000, ZF_BIT4000 = 0x4000,
    ZF_BIT100000 = 0x100000, ZF_BIT8000000 = 0x08000000, ZF_ATTACKING = 0x02000000, ZF_DEAD = 0x04000000,
    ZF_BIT10000000 = 0x10000000, ZF_DAMAGE_MOT = 0x40000000
};
// Bits de +0x10 (flags2).
enum ZFlags2 : uint32_t { ZF2_ALIVE_DEC = 0x2, ZF2_ONSCR_DEC = 0x4, ZF2_BIT20 = 0x20, ZF2_FALLING = 0x40, ZF2_BIT200 = 0x200, ZF2_BIT400 = 0x400 };
// Bits de +0x1D84 (flags3).
enum ZFlags3 : uint32_t {
    Z3_BIT8 = 0x8, Z3_SOUND_DONE = 0x10000, Z3_BIT800 = 0x800, Z3_BIT80000 = 0x80000, Z3_SLOT_TAKEN = 0x04000000,
    Z3_LAYER_ANIM = 0x01000000, Z3_MAIN_ANIM = 0x02000000, Z3_ALT_ANIM = 0x10000000
};

struct ZPart {                 // 0xF0 B por parte (la parte 0 empieza en +0; se usan 1..17)
    uint32_t flag150 = 0;      // +0x150: bit 0x01000000 = parte ya desprendida
    uint32_t flag154 = 0;      // +0x154: bit 0x100 = marcada (AttachSet)
    uint32_t w18C = 0;         // +0x18C: Damage con arma clase 2 escribe 0x29
};

struct ZombieState {
    uint32_t flags = 0;        // +0x0C
    uint32_t flags2 = 0;       // +0x10
    int life = 0;              // +0x18
    int lifeMax = 0;           // +0x1C
    int lifePrev = 0;          // +0x20
    int emitter = 0;           // +0x24
    int curMot = 0;            // +0x34 animacion actual
    int layerMot = 0;          // +0x38 animacion de la capa de dano
    float layerEndFrame = 0;   // +0x40
    float curFrame = 0;        // +0x44
    float layerFrame = 0;      // +0x48
    int type = 0;              // +0x5C tipo de zombi (0..17)
    float posX = 0, posY = 0, posZ = 0;                // +0x30 -> +0/+4/+8
    float velX = 0, velY = 0, velZ = 0, gravity = 0;   // +0x30 -> +0xC/+0x10/+0x14/+0x1C
    int posExtra[3] = {0, 0, 0};                       // +0x30 -> +0x24/+0x28/+0x2C (PosSet)
    float reach = 0;           // +0x30 -> +0x38 (alcance al jugador: <0 = delante)
    int8_t guardBits = 0;      // +0x1D52 (solo bits 0..2: proteccion)
    int8_t killer = 0;         // +0x1D54 jugador que lo mato
    uint32_t flags3 = 0;       // +0x1D84
    int stepCounter = 0;       // +0x1D8C (KeepMove: >= 0x7E fuerza el paso 2)
    // "script" de comportamiento: flujo de s16 en +0x1D98 (aqui bytes + indice en bytes).
    const uint8_t* script = nullptr; int scriptSize = 0; int scriptPos = 0;
    int sv9C = 0, sA0 = 0, sA4 = 0, sA8 = 0, sAC = 0, sB0 = 0;     // +0x1D9C.. temporales del script
    uint32_t handler[3] = {0, 0, 0};   // +0x1DC4/+0x1DC8/+0x1DCC: "funcion de estado" actual (palabras opacas copiadas de D_009A1BD0/D_009A1B70)
    int8_t handlerArg = 0;     // +0x1E4C
    float frameOff1DE8 = 0;    // +0x1DE8 (se suma al fotograma 0x44)
    float frameOff1DEC = 0;    // +0x1DEC (se suma al fotograma 0x48)
    int8_t atkPlayer = -1;     // +0x1DF0 jugador al que ataca (-1 = ninguno)
    uint8_t pattern = 0;       // +0x1DF1 patron de comportamiento (indice de tablas por tipo)
    uint8_t step = 0;          // +0x1DF4 paso de la maquina de estados de la funcion actual
    int8_t atkRec = 0;         // +0x1DF6 registro de ataque actual
    int8_t weaponClass = 0;    // +0x1DF7 clase de arma del ultimo impacto (2 = bala?; 0,1,3 = ..., 4/5 = ver Damage)
    int8_t hitPart = 0;        // +0x1DF9 ultima parte golpeada
    int motObjId = 0;          // +0x8 -> +4: animacion que tiene realmente el objeto de movimiento (0x43 = caso especial sin mezcla)
    int8_t partOfPlayer0 = 0;  // +0x0 -> +0x70: parte golpeada por el jugador 0 (Tut_Dead la escribe)
    uint8_t fallStep = 0;      // +0x1E4D
    float groundY = 0;         // +0x1E74 altura del suelo
    float reach354 = 0;        // +0x354 (<= 0: puede ser elegido como atacante)
    // punteros a las WeakTask (+0x1D58..+0x1D64): solo importa el bit 0x40000000 de su +0xC
    uint32_t* weak[4] = {nullptr, nullptr, nullptr, nullptr};
    uint32_t weakPartId = 0;   // +0x1D58 -> +0x14C (parte que, al golpearla, vale el doble)
    ZPart parts[18];
};

// Entradas de impacto del fotograma (iguales que en EnVampire).
using en::HitInput;

// Lo que el motor devuelve a DamageCheck (ZomCheckHitParts y tablas por tipo).
struct ZombieState;
struct ZDamageIn {
    int8_t chainA[2] = {0, 0}; // D_009A2470[tipo] +0 / +4 (bytes) para EnPartsChain
    uint32_t deathHandler[3] = {0, 0, 0}; // D_009A1BD0 (3 palabras): se copian a +0x1DC4 al morir
    void (*zomCheckHitParts)(ZombieState&) = nullptr;  // ZomCheckHitParts (0x?): NO reescrita; es la que descuenta la vida. Si es null se omite.
    struct PerPlayer { int8_t weaponItem = 0; } pl[2];   // D_502690[pl*4]: objeto de arma equipado
    struct PartGeom { float selfPos[3] = {0, 0, 0}; /* +0x16C/170/174 de la parte propia */
                      float bodyPos[3] = {0, 0, 0}; /* +0x13C/140/144 de la parte del cuerpo (+8) */
                      float bodyF154 = 0;           /* +0x154 */ } part[18];
    float bodyScale = 0;       // +0x8 -> +0x1CA0
};

struct ComTables { const uint32_t* tbl1B70 = nullptr; int count = 0; };

struct ZombieLogic {
    // ---- reglas comunes -------------------------------------------------------------------------------------------
    // is_AttackPlayer(Fi) (0x951080): arcade modo 9 => siempre; si no, el jugador 0/1 debe estar en modo 5.
    static bool isAttackPlayer(const ZEnv& e, int player);
    // AngChk (0x95E920): el angulo (16 bit) `ang` esta en [centro-rango, centro+rango] (aritmetica sin signo con +0x10000).
    static int  angChk(int ang, int center, int range);
    // ETurn (0x95E870): gira `cur` hacia `target` como mucho `step` (16 bit); si queda a menos de un paso devuelve `target`.
    static int  eTurn(int cur, int target, int step);

    // ---- contadores y reservas ------------------------------------------------------------------------------------
    // DecNowEnemyAlvNumChk (0x95E4A0) / DecNowEnemyNumChk (0x95E4D0): descuentan una sola vez (bit 2 / bit 4 de +0x10).
    static void decNowEnemyAlvNumChk(ZombieState& z, ZEnv& e);
    static void decNowEnemyNumChk(ZombieState& z, ZEnv& e);
    // AtcPlFlgClear (0x95E440): libera la reserva de jugador y el bloqueo de ataque.
    static void atcPlFlgClear(ZombieState& z, ZEnv& e);
    // Delete (0x95D320): paso 0 marca muerto, descuenta contadores, libera reservas, vida 0; paso 1 cierra la tarea.
    static void deleteTask(ZombieState& z, ZEnv& e, ZEvents& ev);

    // ---- eleccion de jugador y dano al jugador ----------------------------------------------------------------------
    // AttackPlayerSelect (0x95E0C0): `scLR` = ScLRChk (-1 izquierda / 1 derecha / 0 ninguno). Devuelve true si reservo jugador.
    static bool attackPlayerSelect(ZombieState& z, ZEnv& e, int scLR);
    // AttackPlayerSelect_Asd (0x95E2B0): variante sin reserva; `scChkP` = ScChkP(310.0, +0x280) == 1.
    static bool attackPlayerSelectAsd(ZombieState& z, ZEnv& e, int scLR, bool scChkP);
    // DamageToPlayer (0x95E500): usa el registro `rec` (D_009A2370[tipo][patron][atkRec]). No hace nada si atkPlayer no es 0/1.
    static void damageToPlayer(ZombieState& z, ZEnv& e, const ZAtkRec& rec, ZEvents& ev);
    // DamageToPlayer2 (0x961FA0): ataque por fotograma `frame`; `scLR` = ScLRChk. Devuelve 1 si el jugador era valido.
    static int  damageToPlayer2(ZombieState& z, ZEnv& e, int frame, int dmgType, int mask, int scLR, ZEvents& ev);

    // ---- animaciones ------------------------------------------------------------------------------------------------
    // MotSupSet (0x95DF20): cambio de animacion que respeta las animaciones en curso (flags3 0x02000000 la bloquea).
    static void motSupSet(ZombieState& z, int mot, int p2, int p3, ZEvents& ev);
    // MotSupSet2 (0x95E000): como la anterior pero sin bloqueo; limpia ZF_DAMAGE_MOT y los bits 0x02000000 / 0x01000000.
    static void motSupSet2(ZombieState& z, int mot, int p2, int p3, ZEvents& ev);
    // DamageMotEnd (0x95BCD0): `motLen` = tabla de duraciones por animacion (halfwords en 0x1FB73E4, RAM de ejecucion: inyectada).
    static void damageMotEnd(ZombieState& z, const int16_t* motLen);

    // ---- vida y dano recibido -------------------------------------------------------------------------------------
    // MissEnemyNumAdd (0x9641C0): missCount++ y CheckCom.
    static void missEnemyNumAdd(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev);
    // LifeSet (0x9649D0): vida = s16 del script + tablas por nivel (0x9B6B50) y dificultad (0x9B6B80 / 0x9B6BB0 si solo==2); luego CheckCom.
    static void lifeSet(ZombieState& z, ZEnv& e, const struct ComTables& t, ZEvents& ev);
    // WeakHitClear (0x95AF90): quita el bit 0x40000000 de las 4 WeakTask.
    static void weakHitClear(ZombieState& z);
    // Damage (0x95B640): reaccion al impacto de `player` segun clase de arma (la parte y la clase salen de `h`). Termina con ZF_BIT8000000.
    static void damage(ZombieState& z, const ZEnv& e, int player, const HitInput& h, const ZDamageIn& in, ZEvents& ev);
    // DamageCheck (0x95AFD0): impactos del fotograma, +100 (+200 si la parte es la debil) al morir y muerte.
    // NO descuenta vida: lo hace ZomCheckHitParts (inyectada en `in.zomCheckHitParts`, no reescrita).
    static void damageCheck(ZombieState& z, ZEnv& e, const HitInput& h, const ZDamageIn& in, ZEvents& ev);
    // FallCheck (0x95D540): caida con gravedad -0.0392 hasta `groundY`; al aterrizar limpia el bit 0x40 de flags2 (un solo fotograma).
    static void fallCheck(ZombieState& z);
    // Dead_Sub (0x95BF40), parte fisica: rebote contra el suelo. `ground` = EN_HighGet(pos). El resto (sonidos/efectos por fase) NO.
    static void deadBounce(ZombieState& z, float ground);

    // ---- maquina de ordenes (script) ------------------------------------------------------------------------------
    // CheckCom (0x95DA60): interprete de ordenes del flujo (-1 fin, 60..66). `tbl1B70` = tabla D_009A1B70 (12 B por entrada, 3 palabras).
    //  -1: handler = tbl[0], handlerArg = 0, scriptPos += 2, step = 0, Delete(); si ZF_ATTACKING: missCount++. Termina el bucle.
    //  60: idx = s16 en +2; handler = tbl[idx]; handlerArg = byte en +2; scriptPos += 4; step = 0. Termina el bucle.
    //  61/62: flags |= / &= ~u32 en +4; 63/64: flags2; 65/66: flags3; consumen 8 bytes y el bucle sigue.
    //  Otro codigo: el original no avanza y repite para siempre; aqui se aborta (devuelve -1).
    // Devuelve el numero de ordenes ejecutadas.
    static int checkCom(ZombieState& z, ZEnv& e, const struct ComTables& t, ZEvents& ev, int maxOps = 4096);
    // PosSet (0x9648D0): posExtra[3] = 3 s16; salta un s16; 3 floats (x,y,z): todo 0 -> nada; x==0 y z==0 -> solo y; si no pos = (x,y,z); consume 12 B; CheckCom.
    static void posSet(ZombieState& z, ZEnv& e, const struct ComTables& t, ZEvents& ev);
    // LifeSet/PosSet/Adv_Dead/Tut_Dead/St*: funciones de estado guionizadas (1DF4 = paso; el paso N+1 entra por fallthrough del N).
    // Adv_Dead (0x964550): muerte guionizada de la aventura. Usa e.base.cam.pathId (gp-0x7338->+0xD0).
    static void advDead(ZombieState& z, ZEnv& e, const ZDamageIn& in, ZEvents& ev);
    // Tut_Dead (0x964740): muerte del tutorial (vida del script, golpe forzado a una parte, esperas de N*50/60 fotogramas).
    static void tutDead(ZombieState& z, ZEnv& e, const struct ComTables& t, ZEvents& ev);
    // St2_Sc03_03 (0x9643E0): espera hasta el fotograma 250; marca invulnerable al pasar de sA8 y bit 0x200 a partir de 250.
    static void st2Sc03_03(ZombieState& z, ZEnv& e, const struct ComTables& t, ZEvents& ev);
    // St3_Sc03_02 (0x9641F0): animacion con un golpe (DamageToPlayer2) en el fotograma sA4. `scLR` = ScLRChk. `motLen` = 0x1FB73E4.
    static void st3Sc03_02(ZombieState& z, ZEnv& e, const struct ComTables& t, const int16_t* motLen, int scLR, ZEvents& ev);
    // St1_Sc01_00 (0x962D90): animacion hasta su ultimo fotograma - 1. `motLen` = 0x1FB73E4.
    static void st1Sc01_00(ZombieState& z, ZEnv& e, const struct ComTables& t, const int16_t* motLen, ZEvents& ev);
    // FrmWait (0x9631E0): espera de N*50/60 fotogramas con una animacion. `seqMot[2]` = D_009A22C0[tipo][patron] (s16; indice = flags3 & Z3_ALT_ANIM).
    static void frmWait(ZombieState& z, ZEnv& e, const struct ComTables& t, const int16_t seqMot[2], ZEvents& ev);
};

// Lectura de un s16 / u32 / float del flujo (little endian).
inline int16_t zs16(const uint8_t* p) { return (int16_t)(p[0] | (p[1] << 8)); }
inline uint32_t zu32(const uint8_t* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
inline float zf32(const uint8_t* p) { uint32_t u = zu32(p); float f; std::memcpy(&f, &u, 4); return f; }
}}  // namespace vn::enov

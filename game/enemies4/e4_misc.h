// Tareas y utilidades pequenas de enemigos/objetos de GAMEMOD (0x935xxx..0x95xxxx) reescritas a mano:
// sonidos de enemigos (EnSnd*), EnDamSphere, EnPartsChain, tabla de IDs de enemigo (EnmID*), patrones de animacion (Patn*),
// PartsChangeChk, Set_ShootOrder, ScLRChk/ScChkP, TAttackObject, ManySeTask/BgmFadeTask y trozos de armadura (EnEtcAttach).
#pragma once
#include "e4_common.h"
namespace vn { namespace en4 {

// --------------------------------------------------------------------------------------------------------------------
// EnSnd: limitador de repeticion de sonidos de enemigo. Lista D_009857A0 (ids, terminada en -1) -> registros {id, ultimoFotograma, -1} en 0x9C39A0.
struct EnSndRec { int id; int lastFrame; int unused; };
struct EnSndTable {
    static const int kMax = 16;
    EnSndRec rec[kMax]; int count = 0;
    // EnSndInit__Fv (0x942690): rec[i] = {lista[i], 0, -1}.
    void init(const int* idList /* terminada en -1 */);
    // EnSndReq__Fii (0x9426E0): si el id esta en la lista y han pasado >= 5 fotogramas desde el ultimo, lo reproduce
    // (solo si stage != 0). Con |frame - last| < 5 no hace nada. Solo atiende al primer registro con ese id. Devuelve true si lo encontro.
    bool request(int id, E4Env& e, E4Events& ev);
};

// EnEtcSnd (0x942800 ctor, 0x9427B0 Ctrl): sonido retardado.
struct EnEtcSndState { int delay = 0; int soundId = 0; bool closed = false; };
struct EnEtcSndLogic {
    static void init(EnEtcSndState& s, int soundId, int delayNtsc);        // retardo = palFrames(delay)
    static void ctrl(EnEtcSndState& s, E4Events& ev);                      // Ctrl__8EnEtcSndFv
};

// --------------------------------------------------------------------------------------------------------------------
// EnDamSphere (0x942A20..): esfera de dano que reenvia los impactos al enemigo dueno. Campos: +0xC flags propios, dueno +0xC / +0x10.
struct DamOwner { uint32_t flags = 0; uint32_t flags2 = 0; };
struct DamSphereState { uint32_t flags = 0x10001; /* +0xC: ctor = 0x10001 */ bool closed = false; };
struct DamSphereLogic {
    // Ctrl_Pre__11EnDamSphereFv (0x942A20), parte de reglas. dueno.flags: 0x04000000 = muerto (cierra), 0x100 = invulnerable, 0x8 = golpeado,
    // 0x2/0x4 = tipo de impacto. La parte de posiciones (matrices) es del motor.
    static void ctrlPre(DamSphereState& s, DamOwner& owner, E4Events& ev);
};

// EnPartsChain (0x93B1E0): crea dos EnEtcParts (tipos 2 y 3) con los mismos parametros.
struct PartsChainArgs { int parent, a, b, c, d; };   // (task, Uc a, Uc b, Sc c, Sc d, Sc e) -> el original recibe 6 argumentos
struct PartsChainLogic {
    static void run(int parentTask, int a, int b, int c, int d, int e, E4Events& ev);   // emite dos E4_SPAWN_PARTS (d=2 y d=3)
};

// --------------------------------------------------------------------------------------------------------------------
// EnmID: 32 huecos (D_501310) con el puntero del enemigo; cada enemigo guarda su indice (byte +0x24, -1 = ninguno) y el bit 0 de +0x10.
struct EnmIdTable {
    uintptr_t slot[32];
    void init();                                       // EnmIDInit__Fv (0x9356F0)
    // EnmIDSet__FP9EnemyTask (0x935740): primer hueco libre -> guarda al enemigo, activa el bit 0 de flags2 y su indice.
    // Devuelve el indice, o -1 si estan los 32 ocupados (el indice queda en -1).
    int set(uintptr_t enemy, uint32_t& flags2, int8_t& idByte);
    // EnmIDClear__FP9EnemyTask (0x9357A0): si 0 <= id < 32 y flags2 bit 0 -> libera el hueco.
    void clear(uint32_t flags2, int8_t idByte);
};

// Patn: reproductor de patrones de animacion. PATNTBL: +0 lista de valores (o null: el valor es el propio indice), +4 cantidad, +8 velocidad (float),
// +0xC bucle (!= 0). PATNSTAT: +0 tabla, +4 valor actual, +8 posicion (float), +0xC velocidad, +0x10 terminado.
struct PatnTbl { const int* list = nullptr; int count = 0; float speed = 0; int loop = 0; };
struct PatnStat { const PatnTbl* tbl = nullptr; int value = 0; float pos = 0; float speed = 0; int ended = 0; };
struct PatnLogic {
    static int init(PatnStat& s, const PatnTbl* t);    // PatnInit__FP8PATNSTATP7PATNTBL (0x94FC20)
    static int forward(PatnStat& s);                   // PatnForward__FP8PATNSTAT (0x94FC60)
    static int frame(const PatnStat& s) { return (int)s.pos; }   // PatnFrame (0x94FDA0): trunca la posicion
};

// PartsChangeChk__Fi (0x950570): `ownerFlags` = flags (+0xC) del enemigo apuntado por gp-0x6FD8. Fuera de arcade (0xA) activa 0x200|0x400|0x800;
// pone gp-0x7348[player] = 0 y, si 0x800 sigue activo, de nuevo 0 (el segundo store es redundante en el original).
void partsChangeChk(int16_t gameState, uint32_t& ownerFlags, int player, int32_t weaponClass[2]);

// Set_ShootOrder__Fv (0x950850): orden de disparo (gp-0x7350 / gp-0x734C). Modo 0xE: ambos 0 (la ultima escritura). Si no, por `solo` (gp-0x73F4):
// 0 -> (0,-1); 1 -> (1,-1); 2 -> aleatorio: rand paridad con signo C: 0 => (1,0), distinto de 0 => (0,1); -1 -> (-1,-1).
struct ShootOrder { int first = -1, second = -1; };
ShootOrder setShootOrder(int16_t gameState, int solo, int (*rand)());

// ScLRChk__FP9MvObjTask (0x951040): lado de pantalla (1 derecha / -1 izquierda) por x*k/z >= 0, k = gp-0x7330.
int scLRChk(float x, float z, float k);
// ScChkP__FP6MOPRTSff (0x950FB0): true si (px*k/pz, py*k/pz) cae dentro de [-w, w] x [-h, h] (px,py,pz = parte +0xCC/+0xD0/+0xD4).
bool scChkP(float px, float py, float pz, float k, float w, float h);

// --------------------------------------------------------------------------------------------------------------------
// TAttackObject (0x944CF0..): marcador de objetivo de ataque.
struct AttackObjState {
    uint32_t flags = 0;      // +0xC
    int      type = 0;       // +0x14
    int      id = 0;         // +0x24 (byte)
    int      life = 0;       // +0x18 (s16 de ENEMY_TABLE+0x1C)
    float    scale0 = 0, scale1 = 0;   // +0xB0/+0xB4 = ENEMY_TABLE+0x18
    int8_t   player = -1;    // +0x150 jugador objetivo
    int8_t   fixed = 0;      // +0x151 (== 1: apunta siempre a la camara)
    int      w14C = 0;       // +0x14C
    float    target[3] = {0, 0, 0};    // +0x140/+0x144/+0x148
};
struct AttackObjLogic {
    // __ct__13TAttackObjectFP11ENEMY_TABLE (0x944CF0): copia de campos; suma 1 a enemigos en pantalla y vivos (gp-0x7374 / gp-0x7370).
    static void init(AttackObjState& s, int tblB0, int tblB1, int tblLife, float tblScale, uint32_t tblW14, int8_t b1E, int8_t b1F, E4Env& e, int16_t& alive);
    // SelectAttackTarget__13TAttackObjectFv (0x945040). playerCount = 0x500DF8; attackSlot = gp-0x7420/-0x741C; modes = modo de cada jugador.
    // true si reservo jugador. Con 2 jugadores elige por nlRand()&1 y, si ambos estan ocupados, los libera.
    static bool selectTarget(AttackObjState& s, int playerCount, int solo, int gameState, const int modes[2], int attackSlot[2], int (*rand)());
    // GetTargetDist__13TAttackObjectFv (0x944FF0): distancia 3D entre +0x2C y +0x140 (sqrt).
    static float targetDist(const float pos[3], const float target[3]);
    // SetTarget__13TAttackObjectFi (0x944EB0): sin arg, con +0x151 == 1 o sin jugador -> posicion de la camara; si no -> camara + desplazamiento local
    // (-1.2*(1-2*jugador), 0, -1.5) transformado por la matriz de la camara (func_1AFC00): lo calcula el motor y entra como `worldOffset`.
    static void setTarget(AttackObjState& s, int arg, const float camPos[3], const float worldOffset[3]);
    static void localOffset(int player, float out[3]);
};

// --------------------------------------------------------------------------------------------------------------------
// ManySeTask::loop (0x9435B0): repite un sonido `count` veces cada `period` fotogramas.
struct ManySeState { int count = 0; int timer = 0; int period = 0; int soundId = 0; bool closed = false; };
// gp7298 / gp7294 = gp-0x7298 / gp-0x7294 (cualquiera != 0 corta la repeticion).
void manySeLoop(ManySeState& s, int gp7298, int gp7294, E4Events& ev);

// BgmFadeTask::loop (0x943780): rampa de volumen. vol/target/step en dB*10 (float). step > 0 sube (tope 0.0), step <= 0 baja (suelo -960.0).
struct BgmFadeState { float vol = 0, target = 0, step = 0; bool closed = false; int outVolume = 0; };
void bgmFadeLoop(BgmFadeState& s);

// --------------------------------------------------------------------------------------------------------------------
// EnEtcAttach (0x93CA70 Ctrl_Pre): trozo de armadura que vuela con gravedad, rebota y se encoge/desvanece.
struct AttachState {
    uint32_t flags = 0;                 // +0xC: 0x40000000 = ya rebote, 0x04000000 = terminado
    float pos[3] = {0, 0, 0};           // +0x2C..
    float vel[3] = {0, 0, 0};           // +0x38..
    float acc[3] = {0, 0, 0};           // +0x44..
    int   rot[3] = {0, 0, 0};           // +0x50..
    int   rotVel[3] = {0, 0, 0};        // +0xBC..
    int   timer = 0;                    // +0xAC
    float scale[3] = {2, 2, 2};         // +0xC8..
    float scaleDecay[3] = {0.04f, 0.08f, 0.12f};   // +0xD4.. = 1.2 * 2.0 / {60, 30, 20}
    float alpha = 1.0f;                 // +0xB4
    float groundSaved = 0;              // gp-0x760C <- EN_HighGet
    bool  closed = false;
};
struct AttachLogic {
    // Ctrl_Pre__11EnEtcAttachFv (0x93CA70). `ground` = EN_HighGet(pos). El primer rebote pide el sonido 0x2001B via EnSndReq.
    static void ctrlPre(AttachState& s, float ground, EnSndTable& snd, E4Env& e, E4Events& ev);
    // Parte de reglas de __ct__11EnEtcAttachFP4ANPOUi (0x93CE20): giros aleatorios con nlRand (3 llamadas en este orden) y constantes.
    static void init(AttachState& s, int (*rand)());
};

// new_EnEtc4__FP11ENEMY_TABLE (0x94F880): solo crea el objeto si el byte 1 de ENEMY_TABLE es 0.
inline bool etc4Spawns(int8_t tblByte1) { return tblByte1 == 0; }
}}

// Logica de PeopleTask (ELF main, 0x204F20..0x20D690): los civiles (parasito / vampiro / superviviente). Reescrita a mano.
// Cada funcion cita la original. Campos nombrados por uso con su offset. Las llamadas a motor salen como eventos.
#pragma once
#include "enemy_common.h"
namespace vn { namespace en {

// Tablas del ELF (ver MANIFEST.md).
extern const uint8_t kPeopleBaseDamage[3][8];      // D_274368 -> 0x2D8538/40/48: dano base por [tipo de civil][clase de parte]
extern const int16_t kPeoplePartClass[18];         // D_9ACB10 (GAMEMOD, 0x9ACB10): clase (0..7) por parte golpeada
extern const int16_t kPeopleDamageMot[3][8];       // D_2743B0 -> 0x274380/90/A0: animacion de dano por [tipo][clase]

enum PeopleFlags : uint32_t {
    PF_HITCHECK = 0x8, PF_INVULN = 0x100, PF_BIT2000 = 0x2000, PF_FINISHED = 0x20000, PF_BIT80000 = 0x80000,
    PF_DEAD = 0x04000000, PF_BIT10000000 = 0x10000000, PF_DAMAGE_MOT = 0x40000000
};
enum PeopleState : uint32_t { PS_RESCUED_MARK = 0x1, PS_PARA_HIT = 0x2, PS_DAMMOT_KIND = 0x4, PS_RESCUED = 0x10 };

struct PeopleTaskState {
    uint32_t flags = 0;          // +0xC
    int life = 0;                // +0x18
    int lifePrev = 0;            // +0x20
    int emitter = 0;             // +0x24
    int curMot = 0;              // +0x34 animacion actual
    float frame3C = 0;           // +0x3C
    float curFrame = 0;          // +0x44
    int8_t layerBusy = 0;        // +0x6C
    float screenX = 0;           // +0x2E0 (signo: jugador izquierdo)
    float screenLimit = 0;       // +0x2E8 (si <= -10 no ataca)
    uint32_t stateFlags = 0;     // +0x1D74
    int type = 0;                // +0x1DB0 tipo de civil (0..2)
    int waitKind = -1;           // +0x1DA4 condicion de espera del "script" (-1 = ninguna; 0..7, 37, 39)
    float speed = 0;             // +0x1DA8 (multiplicador de velocidad de animacion)
    int loop = 0;                // +0x1DAC (repeticiones restantes de la animacion)
    int p1DC0 = 0, p1DC4 = 0;    // parametros de la espera
    int p1D78 = 0, p1DB8 = 0, p1DBC = 0, p1DD0 = 0, p1E28 = 0;
    int8_t atkPlayer = 0;        // +0x1E00 (-1 = ninguno)
    int8_t hitPart = 0;          // +0x1E01
    int8_t rescuer = 0;          // +0x1E03 jugador que lo rescato
    int counter1E64 = 0;         // +0x1E64
    // --- "script" (getParam): flujo de ordenes de 16 bits en +0x1DA0 (aqui: indice en bytes)
    int scriptPos = 0;           // +0x1DA0
    int p1DC8 = 0;               // +0x1DC8
    float f1DF0 = 0;             // +0x1DF0
    int mark1DF4 = 0;            // +0x1DF4 (orden 22: posicion del flujo + 2)
    float posX = 0, posY = 0, posZ = 0;   // +0x30->+0/+4/+8
    int posExtra[3] = {0, 0, 0}; // +0x30->+0x24/+0x28/+0x2C
    float f88 = 0, f90 = 0;      // +0x88, +0x90
    uint32_t flags1CCC = 0;      // +0x1CCC (bits 1 y 2)
    float f1E30 = 0; int16_t h1E34 = 0; int p1E68 = 0, p1E60 = 0;
};

// Entradas del interprete del script de civiles (getParam, 0x208C90).
struct VmEnv {
    int     abortAll = 0;              // gp-0x7294
    int     skipFx = 0;                // gp-0x7298
    uint8_t* eventFlags = nullptr;     // D_500E10 (orden 25 escribe 1)
    const int8_t* itemFlags = nullptr; // D_502690 + 0x78 + indice (orden 39)
    int8_t  g500F04 = 0;               // 0x500F04 (orden 39 y 40)
    float   dist1DF8 = 0;              // +0x1DF8 (orden 6)
    float   distToTarget = 0;          // distancia (x,z) a la posicion marcada por la orden 22 (orden 7: func_1B0AA0)
};

// Contadores globales de civiles (gp-0x7208 total, gp-0x7204 rescatados, D_515EC0[fase] / D_515EE0[fase] por fase, gp-0x7214, gp-0x720C, 0x500E34, gp-0x7200).
struct PeopleCounters {
    int16_t total = 0;                 // gp-0x7208
    int16_t rescued = 0;               // gp-0x7204
    int16_t totalByStage[7] = {0};     // D_515EC0 + fase*2
    int16_t rescuedByStage[7] = {0};   // D_515EE0 + fase*2
    int16_t stage6Alive = 0;           // gp-0x7214 (solo fase 6)
    int16_t walking = 0;               // gp-0x720C
    int8_t  flag500E34 = 0;            // 0x500E34
    int     flag7200 = 0;              // gp-0x7200
};

// Entradas del evaluador de esperas (LastCtrl): resultados de funciones de motor/estado global.
struct WaitIn {
    bool    backWaitDone = false;      // backWaitPos() != 0
    bool    onScreen = true;           // CheckOnSc(...) != 0 (caso 4)
    float   angleDeg = 0;              // |nlArcTan2 * 0.005493164| (caso 5)
    float   viewScale = 1;             // D_502510[camara]
    float   dist1DF8 = 0;              // +0x1DF8 (caso 6)
    bool    baseMatch = false;         // +0x1DA0 == +0x28->+0x8 (caso 0)
    const uint8_t* eventFlags = nullptr;   // D_500E10: banderas de evento (casos 3 y 7)
    bool    abortAll = false;          // gp-0x7294 != 0
};

struct PeopleLogic {
    static void addPeopleNum(PeopleCounters& c, int stage);                                            // 0x20AF60
    static void addRescuePeopleNum(PeopleCounters& c, const EnemyEnv& e, EventList& ev);              // 0x20AF90
    // 0x20A7A0 checkVampDamage (civil-vampiro): +10 por golpe, +100 al matarlo.
    static void checkVampDamage(PeopleTaskState& p, EnemyEnv& e, const HitInput& h, EventList& ev);
    // 0x20A8..: sufferVampDamage(parte). Devuelve el sonido pedido (0 = ninguno).
    static int  sufferVampDamage(PeopleTaskState& p, EnemyEnv& e, EventList& ev, int part, int weaponClass);
    // 0x20A530 checkParaDamage (civil con parasito): rescate (+400) y sonido; partner = tarea del parasito (flags +0xC).
    static void checkParaDamage(PeopleTaskState& p, uint32_t& partnerFlags, EnemyEnv& e, const HitInput& h, PeopleCounters& c, EventList& ev);
    static void checkVampDamageMotEnd(PeopleTaskState& p, const int16_t* motLen);                      // 0x20AF.. checkVampDamageMotEnd
    static bool damagePlayer(const PeopleTaskState& p, const EnemyEnv& e, EventList& ev);              // 0x20A250
    static int  motionSelect(const int16_t* list, int count, int (*rand)());                           // 0x20A..: motionSelect(list,count)
    // 0x20A...: setAtcPlID(listaDePosiciones count). Devuelve la posicion de espera elegida (indice) o -1 si no hay hueco.
    static int  setAtcPlID(PeopleTaskState& p, EnemyEnv& e, int posCount);
    static void changeSurvive(PeopleTaskState& p, EnemyEnv& e, PeopleCounters& c, uint32_t& partnerFlags, EventList& ev);   // 0x20B000
    static void changeVampire(PeopleTaskState& p, EnemyEnv& e, PeopleCounters& c, EventList& ev);      // 0x20B150
    static void lifeUp(const PeopleTaskState& p, const EnemyEnv& e, EventList& ev);                    // 0x20BCA0
    static int  groanSound(const PeopleTaskState& p, const EnemyEnv& e);                               // 0x20BF60 groan (devuelve id; 0 = ninguno)
    static int  helpMeSound(const PeopleTaskState& p, const EnemyEnv& e);                              // 0x20C180 helpMe
    // PeopleTask::getParam (0x208C90): interprete del script. 43 ordenes (tabla 0x2C2BC0). Ordenes 1..7, 19, 37, 38, 0 fijan la espera (waitKind = numero de orden);
    // el resto ejecuta y sigue. `script` = flujo de bytes. Devuelve el numero de ordenes ejecutadas.
    // Eventos: EV_SCRIPT con a = orden: 8 motSupplement, 9 motSupplementMove, 10 motChange, 11 adjChest+motSupplement, 12 adjChest+motSupplementMove,
    // 28/29 sonido (b = id, c = 1 si es la forma 29), 30 newExec, 31 func_94EE60, 32 ChgMode, 36 (b = h1E34), 39 func_A10B00.
    static int getParam(PeopleTaskState& p, EnemyEnv& e, PeopleCounters& c, VmEnv& vm, const uint8_t* script, int scriptSize, EventList& ev, int maxOps = 4096);
    // 0x206.. LastCtrl (parte de espera/script): evalua la condicion de espera; devuelve true si el "script" debe leer la siguiente orden (getParam).
    static bool lastCtrlWait(PeopleTaskState& p, const EnemyEnv& e, PeopleCounters& c, const WaitIn& in, const int16_t* motLen);
};
}}  // namespace vn::en

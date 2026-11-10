// Resto de la logica de enemigos comunes (EnVampireTask, PeopleTask, ErsPplTask), SEGUNDA tanda. Reescrita a mano desde el asm.
// Reutiliza las estructuras de game/enemies (VampireState, PeopleTaskState, EnemyEnv, EventList) sin modificarlas.
// Lo que falta en esas estructuras (campos solo usados aqui) vive en VampExt / PeopleExt. Las llamadas a motor (matrices, camara,
// cierre de tareas, dibujo) salen como eventos EV_SCRIPT con codigos RE_* o como entradas inyectadas. Sin rutas absolutas.
#pragma once
#include "../enemies/vampire.h"
#include "../enemies/people.h"
namespace vn { namespace en { namespace rest {

// Codigos de EV_SCRIPT.a propios de esta tanda (no chocan con los opcodes 8..39 de getParam).
enum RestEvent : int {
    RE_WAITPOS_BUFF = 0x7001,   // Enemy_Set_WaitPosBuff(this, this+0x1D70)            (solo arcade, gameState 0xA)
    RE_CAM_ENTRY = 0x7002,      // CameraTask::entryObj(cam, 0, &punto, escala): b = 1 si flag 0x10000000; la escala va en CtrlOut
    RE_TASK_CLOSE = 0x7003,     // TaskCloseX / TaskClose(this)
    RE_PRE_CLOSE = 0x7004,      // func_1A2A10() previo a TaskCloseX (EnVampire)
    RE_SPARK = 0x7005,          // new_EnEff(0, &pos, 0, 0x1F): chispa de armadura (a = parte); posicion en EkndOut
    RE_DAMAGE_LAYER = 0x7006,   // func_1FAA60(tabla, obj, a2, a3, 5, 5, 8): arranque de la capa de dano (a=a2, b=a3)
    RE_CHILD_ALL_CLOSE = 0x7007,// ChildAllClose() (PeopleTask)
    RE_FUNC_9357A0 = 0x7008,    // func_9357A0() (PeopleTask, al cerrar)
    RE_TURN_TARGET = 0x7009,    // turnTarget() de PeopleTask (pendiente, ver MANIFEST)
    RE_KNEAD_DRAW = 0x700A      // func_214AE0(id, escala): sombra/amasado; a = id, escala en KneadOut
};

// ---- Tablas extraidas de los binarios (rest_tables.cpp, generado) ----
extern const uint8_t kWaitMotBase[18];          // D_009A22C0[tipo] -> indice base en la tabla ELF 0x2D8860
extern const int16_t kWaitMotPairs[25][2];      // ELF 0x2D8860 + 4*i -> lista s16 en GAMEMOD: 2 primeras animaciones
extern const int8_t  kEkndChainParam[18][2];    // D_009A2470[tipo]: bytes +0 y +4 (EnPartsChain al morir)
extern const int16_t kDamageMotByClass[16];     // D_009A23D0 -> D_009A23C0 -> D_009B5D50: animacion de dano por clase de parte

// ---- Campos de EnVampireTask que no estan en VampireState ----
struct VampExt {
    int   closeCountdown = 0;      // +0x1D98 (0 = nada; 1 = cerrar en el siguiente fotograma tras +0x10 |= 0x20)
    float accelX = 0, accelZ = 0;  // (+0x30)->+0x18 / +0x20 (la aceleracion Y es VampireState::gravity, +0x1C)
    int   spawnPathId = 0;         // (+0x28)->+4  camino de camara en el que aparece
    int   spawnFrame = 0;          // (+0x28)->+6  fotograma de camara tras el que se retira
    float target[3] = {0, 0, 0};   // +0x1DB0/B4/B8 punto al que mira (turnTarget)
    int   turnStep = 0;            // +0x1DBC giro maximo por fotograma
    int   rotY = 0;                // (+0x30)->+0x28 giro Y (entero, angulo de 16 bits)
    float vec34C[3] = {0, 0, 0};   // +0x34C/350/354 (LastCtrl: |v|^2 da la escala de entrada en camara)
    float mark1DC4[3] = {0, 0, 0}; // *(+0x1DC4) posicion marcada (turnTarget modo -4)
};

// ---- changeDamageMot (0x954000) ----
struct DamageMotIn {
    int motTypeFlag = 0x12;     // halfword 0x1F2F860[tipo] (tabla de RAM de ejecucion, NO esta en el ELF): 0x12 = usa la tabla de animaciones
    // Si motTypeFlag != 0x12 el original usa registros SIN inicializar (a2, a3, t3) heredados del llamador; no se pueden resolver desde el asm.
    int staleA2 = 1, staleA3 = 0, staleThreshold = 11;
};

// ---- checkDamage_Eknd (0x953870) ----
struct EkndIn {
    int armorCount = 0;         // halfword 0x1F29860[tipo] (RAM de ejecucion): se miran las partes 1..armorCount-1
    float body[kVampParts][3] = {};  // *(this+8) + parte*0xF0 + 0x13C/0x140/0x144: posicion de la parte (chispa)
    float body154[kVampParts] = {};  // *(this+8) + parte*0xF0 + 0x154 (float)
    float body1CA0 = 0;              // *(this+8) + 0x1CA0
    float partLocal[kVampParts][3] = {};  // this + parte*0xF0 + 0x16C/0x170/0x174 (trazadora)
};
struct EkndOut {
    bool  spark = false;  float sparkPos[3] = {0, 0, 0};
    bool  tracer = false; float tracerLocal[3] = {0, 0, 0}; float tracerScale = 0;   // z ya con +1.0; el motor lo lleva a camara (func_1AFC00)
};

// ---- turnTarget (0x954BD0) / turnTargetSub (0x954AA0) ----
struct TurnIn {
    float camPos[3] = {0, 0, 0};        // camara actual +0x90/0x94/0x98
    int   coop = 0;                     // 0x500DFC (s16)
    int   gp7374 = 0;                   // gp-0x7374 (s16)
    float sideTarget[3] = {0, 0, 0};    // modo -1 con jugador: traslacion de la matriz de camara por (turnSideOffsetX, 0, 0) (motor)
    float mirrorSrc[3] = {0, 0, 0};     // modos -2/-3: traslacion de la matriz de camara por (x, 0, -30 + alcance) (motor); x=0 en -2, (+0x30)->+0x30 en -3
    float explicitTarget[3] = {0, 0, 0};// modo puntero (el valor de +0x1DAC es un puntero a 3 floats)
    int   angleToTarget = 0;            // s16: nlArcTan2 del objetivo en el sistema local (motor)
};

// ---- LastCtrl / FirstCtrl ----
struct CtrlIn {
    const int16_t* motLen = nullptr;    // tabla de duraciones 0x1FB73E4 (RAM de ejecucion)
    const uint8_t* script = nullptr; int scriptSize = 0;
};
struct CtrlOut { bool camEntry = false; float camEntryScale = 0; };

// ---- KneadEffect (0x952DA0) ----
struct KneadIn { float height = 0;                        // getHight()
                 float viewY[kVampParts] = {};            // Y en camara del punto local de cada parte (motor, func_1AFC00)
                 float radius[kVampParts] = {}; };        // this + parte*0xF0 + 0x184
struct KneadOut { bool draw = false; int effectId = 0; float scale = 0; };

// ---- PeopleTask FirstCtrl / LastCtrl ----
struct PeopleExt {
    int   spawnPathId = 0, spawnFrame = 0;   // (+0x28)->+4 / +6
    float target[3] = {0, 0, 0};             // +0x1DD4/D8/DC (si +0x1DD0 == -1)
    float dist1DF8 = 0, dist1DFC = 0;        // salida de FirstCtrl: distancia XZ y 3D al objetivo
    float layerEndFrame = 0;                 // +0x40
    float vec34C[3] = {0, 0, 0};             // +0x34C
};
struct PeopleFirstIn {
    float camPos[3] = {0, 0, 0};             // camara +0x90/0x94/0x98
    bool  offScreen = false;                 // !CheckOnSc(punto en camara de +0xBBC, +0x1CE0) (solo si stateFlags & 0x100)
};
struct PeopleLastIn {
    WaitIn wait;
    const int16_t* motLen = nullptr;
    int16_t gp7210 = 0;                      // gp-0x7210 (s16)
    const uint8_t* script = nullptr; int scriptSize = 0;
};

struct RestLogic {
    // EnVampireTask::changeDamageMot (0x954000). `part` = byte +0x70 del jugador; `weaponClass` = gp-0x7348[player].
    static void changeDamageMot(VampireState& v, EventList& ev, int part, int weaponClass, const DamageMotIn& in);
    // EnVampireTask::checkDamage_Eknd (0x953870): variante con armadura (flag154 bit 0x100) y eleccion aleatoria de la parte que se rompe.
    static void checkDamageEknd(VampireState& v, EnemyEnv& e, const HitInput& h, const EkndIn& in, EkndOut& out, EventList& ev);
    // EnVampireTask::Wait (0x955A40): espera aleatoria; `motLen` = 0x1FB73E4.
    static void wait(VampireState& v, const EnemyEnv& e, EventList& ev, const int16_t* motLen);
    static int  pickWaitMot(const VampireState& v, const EnemyEnv& e);      // trozo de Wait: animacion al azar entre las 2 primeras de la lista
    // EnVampireTask::turnTargetSub (0x954AA0), parte de reglas: giro acotado. Devuelve el nuevo rotY.
    static int  turnClamp(int rotY, int angle, int maxStep);
    static float turnSideOffsetX(int atkPlayer);                             // modo -1: desplazamiento lateral de la camara (jugador 1 = +3.5, 0 = -3.5, otro = 0)
    // EnVampireTask::turnTarget (0x954BD0): elige el punto (segun +0x1DAC) y gira.
    static void turnTarget(VampireState& v, VampExt& x, const TurnIn& in);
    // EnVampireTask::FirstCtrl (0x9526D0). Devuelve true si cerro la tarea.
    static bool firstCtrl(VampireState& v, VampExt& x, EnemyEnv& e, const TurnIn& turn, EventList& ev);
    // EnVampireTask::LastCtrl (0x952B40): usa chkParam y getParam existentes.
    static void lastCtrl(VampireState& v, VampExt& x, EnemyEnv& e, const CtrlIn& in, CtrlOut& out, EventList& ev);
    // EnVampireTask::KneadEffect (0x952DA0): sombra/amasado al caer. Devuelve true si hay que dibujar.
    static bool kneadEffect(const VampireState& v, const KneadIn& in, KneadOut& out);

    // PeopleTask::FirstCtrl (0x205880). Devuelve true si cerro la tarea.
    static bool peopleFirstCtrl(PeopleTaskState& p, PeopleExt& x, EnemyEnv& e, const PeopleFirstIn& in, EventList& ev);
    // PeopleTask::LastCtrl completo (0x205D70): entrada en camara + espera (PeopleLogic::lastCtrlWait) + getParam + avance de frames.
    static void peopleLastCtrl(PeopleTaskState& p, PeopleExt& x, EnemyEnv& e, PeopleCounters& c, VmEnv& vm, const PeopleLastIn& in, CtrlOut& out, EventList& ev);

    // ErsPplTask::FirstCtrl (0x20D100): `parentProgress` = PeopleTask+0x1D78 (float). Devuelve el alfa de SetAlpha; closed = TaskClose pedido (progress == 0).
    static float ersFirstCtrl(float parentProgress, bool& closed, EventList& ev);
    // ErsPplTask::LastCtrl (0x20D230): avanza frame3C y +0x40 con +0x1D78 propio.
    static void ersLastCtrl(float step, float& frame3C, float& layerEndFrame);
};
}}}  // namespace vn::en::rest

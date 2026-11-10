// Logica del jefe 3 de Vampire Night (Boss3Task, overlay bs3mod, ventana de carga 0x9C4800), reescrita a mano.
// Mismas convenciones que boss2.h: cada funcion cita el original y su direccion; campos nombrados por uso con su offset del Boss3Task.
#pragma once
#include <cstdint>
#include "boss_data.h"
namespace vn { namespace boss {

// Forma (+0x1DAC): 0 y 1 (peleas); fase (+0x1DAD): 0, 1, 2 (umbrales de vida en kBs3PhaseParam).
// Flags del enemigo (+0x0C)
enum Bs3EnemyFlags : uint32_t {
    B3_EF_HIT_THIS_FRAME = 0x8, B3_EF_ATTACK_DAMAGING = 0x10000000, B3_EF_DAMAGE_MOTION = 0x40000000,
    B3_EF_ALWAYS_WEAK = 0x80000000u, B3_EF_NO_HIT_MASK = 0x40008100, B3_EF_NO_CHECK_MASK = 0x8100,
};
// Flags de estado (+0x1DA4)
enum Bs3StateFlags : uint32_t {
    B3_SF_ATTACKING = 0x8,        // ataque en curso
    B3_SF_UPPER_HIT = 0x80,       // reaccion de golpe parcial en curso (CheckDamageEnd la cierra al pasar el fotograma 55)
    B3_SF_APPROACH_ON = 0x1000,   // interpolacion hacia un punto (SetApproach)
    B3_SF_APPROACH_SNAP = 0x200,  // ApproachANPO: coloca directamente en el punto
    B3_SF_DAMAGE = 0x4000,        // reaccion de dano completa en curso
    B3_SF_BIT15 = 0x8000,         // marca de golpe de melee (blur activo)
    B3_SF_LIFE_SHOW = 0x20000,    // barra de aturdimiento visible
    B3_SF_WEAK_HIDE = 0x40000,    // fase de dano: barra de aturdimiento a 0 (y WeakHit solo hace el rebote)
    B3_SF_NO_LIFE_LOSS = 0x01000000, // WeakHit no resta vida
    B3_SF_NO_HIT_MASK = 0x4C080,  // con alguno de estos WeakHit solo hace el rebote
};

struct Bs3Pos { float x = 0, y = 0, z = 0; int32_t ax = 0, ay = 0, az = 0; };  // posicion y angulos (+0x30 -> +0..8, +0x24..+0x2C)
struct Bs3Anpo { float x, y, z; int32_t ax, ay, az; };                          // punto de ruta ANPO (+0..+0x14)

struct Bs3State {
    uint32_t enemyFlags = 0;    // +0x0C
    uint32_t flags2 = 0;        // +0x10 (bit 2/4: contadores de enemigos ya descontados)
    int      motion = 0;        // +0x34
    float    frame = 0;         // +0x3C
    float    frameB = 0;        // +0x40
    int16_t  scene = 0;         // +0x5C (0x14 y 0x15 tienen reglas propias)
    Bs3Pos   pos;
    uint32_t st = 0;            // +0x1DA4
    uint8_t  form = 0;          // +0x1DAC
    uint8_t  phase = 0;         // +0x1DAD
    const int16_t* seqPc = nullptr;            // +0x1DB8
    const int16_t* const* seqTable = nullptr;  // +0x1DBC
    int16_t  seqTableIdx = 0;   // +0x1DC0
    int8_t   seqWait = 0;       // +0x1DC5
    float    speed = 1;         // +0x1DC8 velocidad efectiva
    float    baseSpeed = 1;     // +0x1DD0
    float    speedMul = 1;      // +0x1DD8
    int8_t   attackId = 0;      // +0x1E14
    int8_t   target = -1;       // +0x1E15
    int8_t   playersAtStart = 0;// +0x1E16
    int16_t  attackWork = 0;    // +0x1E30
    int8_t   attackSub = 0;     // +0x1E32
    float    lifeRatio = 1;     // +0x1E98
    float    weakRatio = 1;     // +0x1E9C
    int16_t  weakShowTimer = 0; // +0x1EAA
    int8_t   lastHitParam = 0;  // +0x1EAC
    int8_t   hitCtr = 0;        // +0x1EAF
    int8_t   staggerHits = 0;   // +0x1EB0
    int8_t   hitReload = 0;     // +0x1EB1
    int8_t   staggerMax = 1;    // +0x1EB2
    int16_t  attackAux = 0;     // +0x1ED0
    int8_t   fireCount = 0;     // +0x1ED2 numero de disparos de la ventana de ataque (SetBulletNum)
    int8_t   recovered = 0;     // +0x1ED3
    int32_t  fireStep = 0;      // +0x1ED4
    int16_t  postFrame = 0;     // +0x1ED8
    int32_t  motionIdx = 0;     // +0x1EE8 ultimo indice de movimiento (MotSupSet2)
    int32_t  motionLen = 0;     // +0x1EEC
    int32_t  motionLenPrev = 0; // +0x1EF0
    int32_t  motionBlend = 0;   // +0x1EF4
    int32_t  motionMode = 0;    // +0x1EF8
    // interpolacion (SetApproach / ApproachANPO)
    float    apT = 0, apRate = 0, apDur = 0;     // +0x1E5C,+0x1E60,+0x1E64
    float    apX = 0, apY = 0, apZ = 0;          // +0x1E68.. posicion inicial
    int32_t  apAx = 0, apAy = 0, apAz = 0;       // +0x1E74.. angulos iniciales
};

struct Bs3Env {
    int bossLife = 0;          // gp-0x7384
    int bossLifeMax = 0;       // gp-0x7388
    int lifeInitPending = 0;   // gp-0x7074 (InitLife solo fija la vida si esta a 1)
    int gameState = 0;         // 0x500F14
    int gameKind = 0;          // gp-0x73B4
    int gp73F8 = 0, gp73FC = 0;
    int activePlayer = 0;      // gp-0x73F4
    int nPlayers = 1;          // 0x500DF8 (1 o 2)
    int nPlayersB = 1;         // 0x500DFC
    int attackerFlag[2] = {0, 0};   // gp-0x7420
    int diff[2] = {0, 0};      // gp-0x7078, gp-0x7076 dificultad por jugador (0..16)
    int difficultyShown = 0;   // gp-0x73B8
    int phaseParamIdx = 0;     // gp-0x7070 = &kBs3PhaseParam[idx] (idx = func_96CED0)
    int nowEnemyAlive = 0, nowEnemyNum = 0;   // gp-0x7370, gp-0x7374
};

struct Boss3Engine {
    virtual ~Boss3Engine() {}
    virtual void  sound(int id, int a2 = 0, int a3 = 0, int vol = -1) = 0;
    virtual void  addScore(int player, int pts) = 0;                  // func_212FA0
    virtual int   damagePlayer(int player, int kind, int amount) = 0; // func_212D00
    virtual float playerDamageMul(int player) = 0;                    // func_942F00
    virtual void  addHitNum(int player) = 0;                          // func_213070
    virtual int   startDifficulty() = 0;                              // func_96CEC0
    virtual int   rand() = 0;                                         // func_1A7EE0
    virtual bool  playerValid(int player) = 0;                        // func_951080
    virtual int   motionLength(int motionId) = 0;                     // tabla s16 en 0x1FB73E4 (fuera del overlay)
    virtual void  setMotionNow(int animId) = 0;                       // func_1FA540
    virtual void  setMotionBlend(int animId, int blend) = 0;          // func_1FA640
    virtual void  overlayMotion(int kind, int animId) = 0;            // func_1FAA60
    virtual void  damageEffect(int kind) = 0;                         // new_Boss3DamageEff
    virtual void  hitMark(int which, int player, int hitPart) = 0;    // HitMarkEff0 (0) / HitMarkEff (1)
    virtual void  spawnBomb() = 0;                                    // new_Boss3Bomb
    virtual void  spawnLaser() = 0;                                   // TBoss3Laser
    virtual void  cameraEvent(int a, int b) = 0;                      // func_1C9810(camara, a, b)
    virtual void  blur(int op, int n) = 0;                            // 0 = setBlurStart, 1 = setBlurOn, 2 = setBlurOff
    virtual int   attackPathAux(int attackId) = 0;                    // trayectoria del ataque: +0x18 de su registro (ControlAttack con puntero)
    virtual void  unlockFlag(int a, int b) = 0;                       // func_1D38B0 escritura
};
struct Bs3Query { bool flag22Set = false; };    // resultado de func_1D38B0(2,2)

struct Bs3Hit { int player; int hitPart; int param; };    // como Bs2Hit

struct Boss3Logic {
    // dificultad
    static void initDifficulty(Bs3Env& e, Boss3Engine& g, int settingIdx);        // InitBS3Difficulty (settingIdx = func_96CED0)
    static void incDifficulty(Bs3Env& e, int normal, int twoPlayer);              // IncBS3Difficulty
    static void decDifficulty(Bs3Env& e, int normal, int twoPlayer);              // DecBS3Difficulty
    static int  getDifficulty(Bs3Env& e);                                         // GetBS3Difficulty
    // vida y golpes
    static void initLife(Bs3State& s, Bs3Env& e, int life);                       // InitLife (parte de estado)
    static void controlDispLife(Bs3State& s, const Bs3Env& e);                    // ControlDispLife (proporciones y temporizador)
    static bool checkWeakHit(const Bs3State& s);                                  // CheckWeakHit
    static void checkDamage(Bs3State& s, Bs3Env& e, Boss3Engine& g, const Bs3Query& q, const Bs3Hit hits[2]); // CheckDamage
    static void weakHit(Bs3State& s, Bs3Env& e, Boss3Engine& g, const Bs3Query& q, int hitPart, int player);  // WeakHit
    static void damageUpperBody(Bs3State& s, Boss3Engine& g, int player);         // DamageUpperBody
    static void damage(Bs3State& s, Boss3Engine& g);                              // Damage
    static int  damageMotionId(const Bs3State& s);                                // SetDamageMotion (indice; -1 forma no prevista)
    static void checkDamageEnd(Bs3State& s, Boss3Engine& g);                      // CheckDamageEnd
    // ataques
    static bool selectAttackTarget(Bs3State& s, Bs3Env& e, Boss3Engine& g);       // SelectAttackTarget
    static void setBulletNum(Bs3State& s, Bs3Env& e, Boss3Engine& g, int8_t attack); // SetBulletNum
    static void attackDamageFrame(Bs3State& s, Bs3Env& e, Boss3Engine& g);        // AttackDamageFrame
    static void startAttack(Bs3State& s, Bs3Env& e, Boss3Engine& g, int8_t id);   // StartAttack
    static void controlAttack(Bs3State& s, Bs3Env& e, Boss3Engine& g);            // ControlAttack
    static void atcPlFlgClear(Bs3State& s, Bs3Env& e);                            // AtcPlFlgClear
    static void decNowEnemyAlvNumChk(Bs3State& s, Bs3Env& e);                     // DecNowEnemyAlvNumChk
    static void decNowEnemyNumChk(Bs3State& s, Bs3Env& e);                        // DecNowEnemyNumChk
    // movimiento
    static void motSupSet2(Bs3State& s, Boss3Engine& g, int idx, int blend, int mode); // MotSupSet2
    static int  interDivAng(float t, int a0, int a1);                             // B3InterDivAngF
    static void setApproach(Bs3State& s, int frames);                             // SetApproach
    static void approachAnpo(Bs3State& s, const Bs3Anpo& target);                 // ApproachANPO
    // secuencias
    static void seqCmdInit(Bs3State& s, const int16_t* const* table);             // SeqCmdInit
    static void seqCmdNextTable(Bs3State& s);                                     // SeqCmdNextTable
    static void seqCmdSetTable(Bs3State& s, int16_t idx);                         // SeqCmdSetTable
};

}} // namespace

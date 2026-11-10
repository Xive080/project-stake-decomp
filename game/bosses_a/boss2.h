// Logica del jefe 2 de Vampire Night (Boss2Task, overlay bs2mod, ventana de carga 0x9C4800), reescrita a mano.
// Cada funcion cita la original (simbolo mangled sin sufijo) y su direccion. Los nombres de campo son inferidos del uso;
// el offset del Boss2Task va en comentario. Sin dependencias de plataforma: el motor se inyecta con Boss2Engine/Boss2Env.
#pragma once
#include <cstdint>
#include "boss_data.h"
namespace vn { namespace boss {

// Formas del jefe (+0x201C). 0 y 1 = peleas de espada 1/3 (CheckWeakHit siempre valido), 2 = pelea final (solo parte 2 es punto debil).
enum Bs2Form : int { BS2_FORM_BATTLE1 = 0, BS2_FORM_BATTLE3 = 1, BS2_FORM_BATTLE4 = 2 };

// Flags del enemigo (+0x0C)
enum Bs2EnemyFlags : uint32_t {
    EF_HIT_THIS_FRAME = 0x8,         // CheckDamage solo actua si esta activo
    EF_ATTACK_DAMAGING = 0x10000000, // ventana de golpe activa (StartAttack lo pone; ControlAttack lo quita)
    EF_ATTACK_BIT15 = 0x8000,        // ataque 0x17 lo pone tras el primer golpe
    EF_DEAD = 0x04000000,
    EF_DAMAGE_MOTION = 0x40000000,   // Damage lo pone; MotSupSet2 lo quita al cambiar de movimiento
    EF_ALWAYS_WEAK = 0x80000000,
    EF_NO_HIT_MASK = 0x40008100,     // WeakHit: con alguno de estos solo hace el efecto de rebote
    EF_NO_CHECK_MASK = 0x8100,       // CheckDamage: ni mira golpes
};
// Flags de estado del jefe (+0x2014)
enum Bs2StateFlags : uint32_t {
    SF_WARP_ALPHA_BUSY = 0x4,   // rampa de alfa activa (ControlWarp)
    SF_WARP_TIMER = 0x2,        // temporizador de teletransporte activo
    SF_WARP_PRE = 0x1,
    SF_ATTACKING = 0x200,       // ataque en curso (StartAttack/ControlAttack)
    SF_AFTERIMAGE = 0x400,
    SF_FACE_PLAYER = 0x800,     // ControlPos gira hacia el jugador (forma 0)
    SF_STAGGER = 0x4000,        // aturdido (CheckDamageEnd cuenta el temporizador)
    SF_OUTLINE = 0x40000,
    SF_HIDDEN = 0x40,           // alfa 0 tras desaparecer
};
// Flags2 (+0x10)
enum Bs2Flags2 : uint32_t { F2_ALV_DEC = 0x2, F2_NUM_DEC = 0x4, F2_FALLING = 0x40 };

struct Bs2Branch {            // BS2BRANCHPRM (0x14 B)
    int32_t first;            // +0  valor para +0x2114 la primera vez
    int32_t repeat;           // +4  idem a partir de la segunda visita
    const int16_t* const* seqFirst;    // +8  tabla de secuencias
    const int16_t* const* seqRepeat;   // +0xC
    int8_t next[4];           // +0x10 siguiente rama segun +0x2069 (solo se ve el indice 0..3)
};

// Entrada de teletransporte (ENEMY de 0x28 B en +0x2114 + paso*0x28): tiempos en fotogramas.
struct Bs2WarpPoint {
    int16_t outPre = 0, outMid = 0, outPost = 0;   // +0x12,+0x14,+0x16 salida: espera, rampa de alfa, espera final
    int16_t inPre = 0, inMid = 0, inPost = 0;      // +0x18,+0x1A,+0x1C entrada
    int32_t outMotion = -1;                        // +0x20 movimiento al salir (<0 = ninguno)
};

struct Bs2State {
    uint32_t enemyFlags = 0;        // +0x0C
    uint32_t flags2 = 0;            // +0x10
    int      motion = 0;            // +0x34 movimiento actual
    float    frame = 0;             // +0x3C fotograma actual
    float    frameB = 0;            // +0x44
    float    px = 0, py = 0, pz = 0;// posicion (+0x30 -> +0,+4,+8)
    float    vy = 0, ay = 0;        // velocidad/aceleracion vertical (+0x10, +0x1C de la posicion)
    int32_t  angleY = 0;            // +0x28 de la posicion: angulo de giro (16 bits)
    float    swordFireAlpha = 0;    // +0x22F4
    uint32_t st = 0;                // +0x2014 flags de estado
    uint8_t  form = 0;              // +0x201C
    uint8_t  branchStep = 0;        // +0x201E paso dentro de la rama (los warps y la secuencia se indexan con el)
    int32_t  time = 0;              // +0x2020 contador de fotogramas
    const int16_t* seqPc = nullptr; // +0x2054 puntero de comandos de secuencia
    const int16_t* const* seqTable = nullptr; // +0x2058
    int16_t  seqTableIdx = 0;       // +0x205C
    int8_t   seqWait = 0;           // +0x2060 contador/espera del comando actual (se pone a 0 al cambiar de tabla)
    int8_t   seqSp = 0;             // +0x2061 profundidad de la pila de llamadas
    const int16_t* seqStack[10] = {};// +0x202C
    const Bs2Branch* branches = nullptr; // +0x2064
    int8_t   branchIdx = 0;         // +0x2068
    int8_t   branchSel = 0;         // +0x2069
    int32_t  branchParam = 0;       // +0x2114
    float    speed = 1;             // +0x206C velocidad efectiva de movimiento (baseSpeed*speedMul)
    float    baseSpeed = 1;         // +0x2074
    float    speedMul = 1;          // +0x207C
    int8_t   attackId = 0;          // +0x20BC
    int8_t   target = -1;           // +0x20BD jugador atacado (-1 ninguno)
    int8_t   playersAtStart = 0;    // +0x20BE copia de 0x500DFC al empezar el ataque
    int16_t  attackTimer = 0;       // +0x20D8
    int8_t   attackSub = 0;         // +0x20DA subestado del ataque
    float    lifeRatio = 1;         // +0x20DC
    float    weakRatio = 1;         // +0x20E0
    int16_t  staggerTimer = 0;      // +0x20EC
    int8_t   hitCtr = 0;            // +0x20F1 golpes que faltan para contar un impacto
    int8_t   staggerHits = 0;       // +0x20F2 impactos que faltan para aturdir
    int8_t   hitReload = 0;         // +0x20F3 valor de recarga de hitCtr
    int8_t   staggerMax = 0;        // +0x20F4
    int32_t  dmgPerHit = 0;         // +0x20F8
    int32_t  difficulty = 0;        // +0x20FC indice 0..16
    int16_t  diffUp = 0, diffDown = 0; // +0x2100, +0x2102
    int8_t   lastHitParam = 0;      // +0x20EE
    int16_t  diffTimer = 0;         // +0x2104 (s16 segun m2c; solo se pone a 0 en lo visto)
    uint8_t  fallState = 0;         // +0x2105
    float    alpha = 0;             // +0x2118
    float    alphaRate = 0;         // +0x211C
    float    alphaOutThr = 0;       // +0x2120 (colision se desactiva al bajar de aqui)
    float    alphaInThr = 0;        // +0x2124 (colision se activa al subir de aqui)
    int16_t  warpTimer = 0;         // +0x2128
    int8_t   warpKind = 0;          // +0x212A 1 = salir, 2 = entrar
    int8_t   warpPhase = 0;         // +0x212B
    float    outlineAlpha = 0;      // +0x2130
    uint8_t  deadStage = 0;         // +0x20B4 (Dead pone 3), +0x20B5, +0x20B6 a 0
    uint8_t  deadB5 = 0, deadB6 = 0;
};

// Globales de sistema que lee/escribe el jefe (viven en bss; el motor las inyecta).
struct Bs2Env {
    int bossLife = 0;        // gp-0x7384 vida actual del jefe
    int bossLifeMax = 0;     // gp-0x7388
    int gameState = 0;       // 0x500F14 (0xA = arcade normal)
    int gameKind = 0;        // gp-0x73B4 (2 = modo especial)
    int gp73F8 = 0;          // gp-0x73F8
    int gp73FC = 0;          // gp-0x73FC
    int activePlayer = 0;    // gp-0x73F4 jugador activo (partida de uno)
    int attackerFlag[2] = {0, 0};   // gp-0x7420 / gp-0x741C: jugador ya elegido como objetivo
    int difficultyShown = 0; // gp-0x73B8
    int nPlayersDFC = 1;     // 0x500DFC (numero de jugadores, copiado a playersAtStart)
    int nowEnemyAlive = 0;   // gp-0x7370
    int nowEnemyNum = 0;     // gp-0x7374
    int8_t branchVisits[8] = {0};            // gp-0x7080 veces que se ha entrado en cada rama
    const int16_t* const* seqCallTable = nullptr; // gp-0x7A90 destinos de SeqCmdCall
};

// Hechos del motor. Todo lo que dibuja, suena o consulta colisiones sale por aqui (no se implementa).
struct Boss2Engine {
    virtual ~Boss2Engine() {}
    virtual void  sound(int id, int a2 = 0, int a3 = 0, int vol = -1) = 0; // func_217C30(&D_515F30, id, a2, a3, vol) (vol -1 = no se pasa)
    virtual void  addScore(int player, int pts) = 0;       // func_212FA0 (AddPlayerScore)
    virtual int   damagePlayer(int player, int kind, int amount) = 0; // func_212D00; 1 = se aplico el danio
    virtual float playerDamageMul(int player) = 0;         // func_942F00(jugador): multiplicador de danio
    virtual void  addHitNum(int player) = 0;               // func_213070 (AddPlayerHitNum)
    virtual void  attackPosSet() = 0;                      // AttackPosSet (geometria del objetivo)
    virtual void  setAttackDist(float dist) = 0;           // SetAttackDist
    virtual void  resetSwordFire() = 0;                    // obj +0x22F0 ->+0x2570 = 0
    virtual int   twoPlayerLevel() = 0;                    // func_943AC0 (0 = un jugador, 1 = dos)
    virtual int   startDifficulty() = 0;                   // func_96CEC0
    virtual int   rand() = 0;                              // func_1A7EE0 (nlRand)
    virtual bool  playerValid(int player) = 0;             // func_951080
    virtual float groundHeight(float x, float y, float z) = 0; // func_951300
    virtual int   motionAngle(int motionId) = 0;           // GetMotAngle
    virtual void  setMotion(int motionId, int a, int b) = 0;  // func_1FA640(mgr, cuerpo, motion, a, b)
    virtual void  playOverlayMotion(int kind, int motionId) = 0; // func_1FAA60
    virtual void  spawnSonicWave() = 0;                    // new_SonicWave
    virtual void  pillarDynamics(int a, int b, int c, int d, int e) = 0; // StartPillarDynamics
    virtual void  damageEffect(int kind) = 0;              // new_Boss2DamageEff
    virtual void  hitMark(int kind, int player, int hitPart) = 0;  // HitMarkEff (kind 1) / HitMarkEff2 (kind 2) (efectos)
    virtual void  warpEffect(int pre, int mid, int post) = 0;      // new_WarpEff
    virtual void  unlockFlag(int a, int b) = 0;            // func_1D38B0 con escritura (resultado inyectado en Bs2Query)
    virtual void  enableCollision() = 0;
    virtual void  disableCollision() = 0;
    virtual void  setAlpha(float a) = 0;                   // SetAlpha
    virtual void  noWeakHitFeedback() = 0;                 // func_946FD0(0)
};
struct Bs2Query { bool flag20Set = false; };   // resultado de func_1D38B0(2,0) (lectura); solo Dead lo usa

// Una ranura de CheckDamage (2 por fotograma): gp-0x7350[ranura] = jugador que acerto (-1 ninguno); hitPart = byte +0x70 de la tabla de
// impactos (parte golpeada); param = gp-0x7348[jugador] (-> +0x20EE).
struct Bs2Hit { int player; int hitPart; int param; };

struct Boss2Logic {
    // Vida y dificultad
    static void initLife(Bs2State& s, Bs2Env& e, int16_t life);                          // InitLife
    static void controlDispLife(Bs2State& s, const Bs2Env& e);                           // ControlDispLife (solo las dos proporciones)
    static void setDiffParam(Bs2State& s, Bs2Env& e, Boss2Engine& g);                    // SetDiffParam
    static void initDifficulty(Bs2State& s, Boss2Engine& g);                             // InitDifficulty
    static void upDifficulty(Bs2State& s);                                               // UpDifficulty
    static void downDifficulty(Bs2State& s);                                             // DownDifficulty (= DownDifficultyReq)
    static void noChgDifficultyReq(Bs2State& s);                                         // NoChgDifficultyReq
    // Golpes, danio y muerte
    static bool checkWeakHit(const Bs2State& s, int hitPart);                            // CheckWeakHit
    static void checkDamage(Bs2State& s, Bs2Env& e, Boss2Engine& g, const Bs2Query& q, const Bs2Hit hits[2]); // CheckDamage (ClearWeakHit lo hace el motor)
    static void weakHit(Bs2State& s, Bs2Env& e, Boss2Engine& g, const Bs2Query& q, int hitPart, int player); // WeakHit
    static void noWeakHit(const Bs2State& s, Boss2Engine& g);                            // NoWeakHit
    static void damageUpperBody(Bs2State& s, Boss2Engine& g, int player);                            // DamageUpperBody
    static void damage(Bs2State& s, Bs2Env& e, Boss2Engine& g);                          // Damage (+SetDamageMotion)
    static int  damageMotionId(const Bs2State& s);                                       // SetDamageMotion (id de movimiento)
    static void dead(Bs2State& s, Bs2Env& e, Boss2Engine& g, const Bs2Query& q, int player); // Dead (+SetDeadMotion)
    static void checkDamageEnd(Bs2State& s);                                             // CheckDamageEnd
    // Ataques
    static void startAttack(Bs2State& s, Bs2Env& e, Boss2Engine& g, int8_t id);          // StartAttack (sin efectos de espada)
    static void controlAttack(Bs2State& s, Bs2Env& e, Boss2Engine& g);                   // ControlAttack
    static void attackDamageFrame(Bs2State& s, Bs2Env& e, Boss2Engine& g);              // AttackDamageFrame (clave de escena = rama<<16 | tabla)
    static void damageToPlayer(Bs2State& s, Boss2Engine& g);                             // DamageToPlayer
    static bool selectAttackTarget(Bs2State& s, Bs2Env& e, Boss2Engine& g);              // SelectAttackTarget
    static void atcPlFlgClear(Bs2State& s, Bs2Env& e);                                   // AtcPlFlgClear
    static void decNowEnemyAlvNumChk(Bs2State& s, Bs2Env& e);                            // DecNowEnemyAlvNumChk
    static void decNowEnemyNumChk(Bs2State& s, Bs2Env& e);                               // DecNowEnemyNumChk
    // Movimiento
    static void motSupSet2(Bs2State& s, Boss2Engine& g, int motionId, int a, int b);     // MotSupSet2
    static void setHeight(Bs2State& s, Boss2Engine& g);                                  // SetHeight
    static void fallCheck(Bs2State& s, Boss2Engine& g);                                  // FallCheck
    static void controlWarp(Bs2State& s, Boss2Engine& g, const Bs2WarpPoint& w, bool outlineOn);  // ControlWarp (temporizadores + rampa de alfa; sin la eleccion de movimiento de entrada)
    static void controlSound(const Bs2State& s, Boss2Engine& g);                         // ControlSound
    static void controlOutline(Bs2State& s);                                             // ControlOutline
    static void startWarpOut(Bs2State& s, Boss2Engine& g, const Bs2WarpPoint& w);        // StartWarpOut
    static void seqCmdCall(Bs2State& s, const Bs2Env& e, int16_t idx);                   // SeqCmdCall
    static int  eTurn(int cur, int target, int step);                                    // ETurn
    static bool angChk(int ang, int center, int range);                                  // AngChk
    // Secuencias (interprete de comandos: solo la pila y las ramas)
    static void seqCmdInit(Bs2State& s, const int16_t* const* table);                    // SeqCmdInit
    static void seqCmdNextTable(Bs2State& s, Bs2Env& e);                                            // SeqCmdNextTable (si la tabla siguiente es nula llama a Branch, como el original)
    static void seqCmdPush(Bs2State& s);                                                 // SeqCmdPush
    static void seqCmdPop(Bs2State& s);                                                  // SeqCmdPop
    static void seqCmdCancelWait(Bs2State& s);                                           // SeqCmdCancelWait
    static void branchInit(Bs2State& s, Bs2Env& e, const Bs2Branch* b);                  // BranchInit
    static void branch(Bs2State& s, Bs2Env& e);                                          // Branch
};

}} // namespace

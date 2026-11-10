// Logica del jefe 5 (TBoss5, overlay bs5mod, carga 0x9C4800), reescrita a mano a partir de asm/bs5mod/bs5mod_text.s.
// Cada funcion cita la original. Los campos se nombran por uso, con el offset de la estructura TBoss5 en comentario.
// Sin dependencias de plataforma: todo lo que el original pide al motor (sonido, movimiento, camara, colisiones, rand,
// globales gp-0x7xxx) pasa por `Boss5Env` y `Boss5Globals`. Ver MANIFEST_boss5.md para el estado de cada funcion.
// El jefe 5 es el jefe de agua/hielo: tipo 0 = "battle1" (la forma inicial) y tipo 1 = "battle2" (el combate principal, tiene
// barra de vida compartida, pilares de agua y ataques de hielo); nombres tomados de CtrlPre_battle1/CtrlPre_battle2.
#pragma once
#include <cstdint>
namespace vn { namespace boss5 {

enum Kind : uint8_t { K_BATTLE1 = 0, K_BATTLE2 = 1 };   // +0x1E48

// Estado de la tarea (+0x1F24): valores vistos en el codigo (CharInit/Dead/actDead/op 1).
enum TaskState : uint8_t { TS_RUN = 0, TS_FINISHED = 1, TS_DEAD = 2 };

// Banderas de +0x0C (tarea base de enemigos; solo las que usa este overlay).
enum Flags0C : uint32_t {
    F0C_HIT = 0x8,               // CheckDamage: impacto este fotograma
    F0C_COLLISION_OFF = 0x8000,  // Enable/DisableCollision
    F0C_DELETE = 0x4000,         // actDelete
    F0C_DEAD = 0x04000000,       // Dead / actDelete
    F0C_ATTACKING = 0x10000000,  // StartAttack / ControlAttack / op 8
    F0C_IN_DAMAGE = 0x40000000,  // Damage; tambien en la mascara de rechazo de WeakHit
    F0C_WEAK = 0x80000000u,      // CheckDamage: el punto golpeado es debil
};
// Banderas de +0x1E40 (nombradas por uso; ver comentarios).
enum Flags1E40 : uint32_t {
    FB_ATTACK = 0x1,             // StartAttack lo pone; ControlAttack lo quita al acabar
    FB_HIT_STUN = 0x10,          // DamageUpperBody: aturdido (cuenta atras 1F5C)
    FB_NO_PART_COLLISION = 0x20, // Enable/DisableCollision no tocan las partes
    FB_LIFEBAR = 0x200,          // op 44 / CtrlPre: ControlDispLife
    FB_ATK_WINDOW = 0x100000,    // ventana de dano del ataque abierta
    FB_ALPHA_OUT = 0x8000,       // ControlAlpha: desvanecer
    FB_PILLARS = 0x10000,        // ControlWaterPillar activo
    FB_SWIM = 0x20000,           // CheckSwimMotion: movimiento de nado (0x1D8/0x1DA/0x1DC)
};

// ---- Tablas (generadas en boss5_data.cpp desde disc/TBL/BS5MOD.BIN y el ELF) ----
extern const int8_t kCmdLen[47];            // D_009CF8D0
extern const int16_t kScript[];             // scripts (int16)
extern const unsigned kScriptLen;
extern const uint16_t kSeqTab0[], kSeqTab1[];   // tablas de scripts por tipo (0xFFFF = fin)
extern const uint16_t kCallScript[2];       // gp-0x7930
struct KindConfig { uint32_t orC, andC, or1E40, and1E40; const uint16_t* seqTab; uint8_t initState; };
extern const KindConfig kKindConfig[2];     // gp-0x7900 -> 0x9CF410 / 0x9CF430
struct AttackEntry { int16_t motion; int8_t dmg; int16_t f[6]; int16_t endFrame; };   // f[] = +6,+8,+A,+C,+E,+0x10; endFrame = +0x12
extern const AttackEntry kAttackTable[4];   // D_009DBD00
extern const float kStrength[2][17][3];     // D_009CF4E0 [nivel][dificultad]
extern const int8_t kPartLife[5][2][17];    // D_009CF680 [sel][nivel][dificultad]
extern const int32_t kDmgMul[2][17];        // D_009CF730
extern const int16_t kDiffStep[2][2];       // ELF 0x2D87F8 [nivel][subida,bajada]
extern const float kPillarSpeed[2][17];     // D_009CF450
extern const float kPose[2][6];             // D_009CE850

constexpr int kStackDepth = 11;
constexpr int kTimers = 10;

// Estado de un TBoss5 (solo campos usados por la logica reescrita). Offsets de la tarea en comentario.
struct Boss5State {
    uint32_t f0C = 0;            // +0x0C
    uint32_t f10 = 0;            // +0x10 (bit1 = ya descontado de vivos, bit2 = ya descontado de enemigos, bit5 = borrar, bit6 = cayendo)
    float pos[3] = {0, 0, 0};    // objeto de posicion (+0x30 -> +0,+4,+8)
    float rot[3] = {0, 0, 0};    // idem +0x24,+0x28,+0x2C
    float velY = 0, accelY = 0;  // idem +0x10, +0x1C
    float f18 = 0;               // +0x18
    int motion = 0;              // +0x34 movimiento actual
    float motionFrame = 0;       // +0x3C
    int16_t camTrackId = 0, camTrackFrame = 0;   // *(+0x1D78): {id de ruta de camara, fotograma} (op 16)
    int8_t kind74 = 0;           // +0x1D74 (CharInit: 0 = tipo 0, 1 = tipo 1)
    float heightOffset = 0;      // +0x1D80
    int8_t pathMode = 0;         // +0x1DB8 (op 32/35/36)
    float pathPos = 0, pathLimitA = 0, pathLimitB = 0;   // +0x1DE0, +0x1DDC, +0x1DE8 (ops 11 y 13)
    float pathSpeed = 0;         // +0x1DE4 (op 34)
    float f1DF4 = 0, f1DF8 = 0;  // +0x1DF4/+0x1DF8 (op 35)
    float camRotSave[3] = {0, 0, 0};   // +0x1E08..+0x1E10 (op 35: copia de la rotacion)
    uint32_t f1E40 = 0;          // +0x1E40
    uint8_t kind = 0;            // +0x1E48
    float alpha = 1.0f, alphaStep = 0;   // +0x1E50, +0x1E54
    int32_t phaseCount = 0;      // +0x1E58 (fases/golpes consumidos)
    int16_t branch[3] = {0, 0, 0};   // +0x1E38,+0x1E3A,+0x1E3C (op 21: seleccion + dos aleatorios)
    int8_t pillarState = 0;      // +0x1E60 (ControlWaterPillar: 0 parado, 1 subiendo, 2 acabando)
    int16_t pillarPhase = 0, pillarRise = 0;   // +0x1E62, +0x1E64
    float pillarBase = 0, pillarTarget = 0, pillarVel = 0, pillarAcc = 0;   // +0x1E68, +0x1E6C, +0x1E70, +0x1E74
    const int16_t* pc = nullptr; // +0x1EBC
    const uint16_t* seqTab = nullptr; // +0x1EC0 (indices en kScript)
    int16_t tabIdx = 0;          // +0x1EC4
    const int16_t* stack[kStackDepth] = {};  // +0x1E94
    int16_t timer[kTimers] = {}; // +0x1EC6 (un contador por nivel de pila; -1 = sin iniciar)
    int8_t timerSel = 0;         // +0x1EDA
    int8_t sp = 0;               // +0x1EDB
    float rate = 1.0f;           // +0x1EDC (velocidad de reproduccion efectiva)
    float rateBase = 1.0f;       // +0x1EE4
    float rateMul = 1.0f;        // +0x1EEC
    uint8_t state = 0, stateB = 0, stateC = 0;   // +0x1F24,+0x1F25,+0x1F26
    int8_t attack = 0;           // +0x1F2C (id de ataque)
    int8_t target = -1;          // +0x1F2D (jugador objetivo; -1 = ninguno)
    int8_t numPlayers = 0;       // +0x1F2E (copia de 0x500DFC al empezar el ataque)
    int8_t atkStep = 0;          // +0x1F4A
    int16_t s1F48 = 0;           // +0x1F48
    float lifeRatio = 1.0f, hitRatio = 1.0f;     // +0x1F4C, +0x1F50
    int16_t stunTimer = 0;       // +0x1F5C (aturdido tras golpe, cuenta atras)
    int8_t weakIdx = 0;          // +0x1F5E
    int8_t hitCounter = 1;       // +0x1F61 (cuenta atras de impactos hasta contar dano)
    int8_t partLife = 0;         // +0x1F62 (golpes que quedan para la fase)
    int8_t hitsPerCount = 1;     // +0x1F63
    int8_t partLifeMax = 0;      // +0x1F64
    int32_t dmgMul = 0;          // +0x1F68
    int32_t difficulty = 0;      // +0x1F6C (0..16)
    int16_t diffUp = 0, diffDown = 0;   // +0x1F70,+0x1F72
    int8_t diffArmed = 0;        // +0x1F74
    int8_t sel = 0;              // +0x1F75 (selector de tabla de vida, op 46)
    uint8_t fallStep = 0;        // +0x1F76
};

// Globales del juego que leen estas funciones (gp-relativos, bss: no estan en el ELF).
struct Boss5Globals {
    int bossLife = 0;            // gp-0x7384 (vida actual compartida del jefe)
    int bossLifeMax = 0;         // gp-0x7388
    int16_t enemiesLeft = 0;     // gp-0x7374
    int16_t enemiesAlive = 0;    // gp-0x7370
    int targetBusy[2] = {0, 0};  // gp-0x7420, gp-0x741C
    int8_t lastHitTarget = 0;    // gp-0x73F4
    int diffMirror = 0;          // gp-0x73B8
    int8_t deathCount73FC = 0;   // gp-0x73FC
    int8_t deathCount73F8 = 0;   // gp-0x73F8
    int gameState = 0;           // 0x500F14 (0xA = juego normal)
    int gameKind = 0;            // gp-0x73B4 (2 = modo especial)
    int gameLevel = 0;           // func_943AC0(): nivel de dificultad de la partida (0/1)
    int8_t numPlayersB = 0;      // 0x500DFC
    uint8_t cameraStartAllowed = 0;   // 0x500E10
    int8_t g73CC = 0, g72A0 = 0;      // gp-0x73CC, gp-0x72A0
    int g729C = 0;                    // gp-0x729C
};

// Llamadas al motor / a funciones no reescritas, identificadas para `Boss5Env::hook`.
enum Hook {
    H_CLEAR_BODY_MOVE = 1,   // ClearBodyMove(a)        (ops 25 y 27: a = 0x12 / 0x10)
    H_SET_BODY_MOVE,         // SetBodyMove(a)          (ops 26 y 28)
    H_SET_PATH,              // SetPath(a)              (op 33)
    H_INIT_WAIT_PATH,        // InitWaitPath()          (op 38, bit 0x13)
    H_INIT_SIPPO_POS,        // InitSippoPos()          (op 38, bit 0x12)
    H_BIT_0F_SOUND,          // sonido 0x1A010E al tocar el bit 0xF (op 38)
    H_PATH_CAM_OFFSET,       // op 35: calculo de +0x1DFC con matrices (func_1B0834/1B0808/1B06CC/1AFC00)
    H_POSE_ROTATE,           // op 45 con c1 == -1: rota el vector D_009CE868 y lo escribe en la posicion (func_1AFC00)
    H_ENABLE_PART_COLLISION, // EnableCollision: quita 0x40 de +0x1A8 de las 4 partes (+0x1D58)
    H_DISABLE_PART_COLLISION,
    H_START_WEAK_DISP,       // StartWeakDisp: quita 0x80 de +0x1A8 de las partes
    H_CLEAR_WEAK_HIT,        // ClearWeakHit: quita 0x40000000 de +0xC de las 4 partes
    H_HIT_PREPARE,           // func_950910 al empezar CheckDamage
    H_HIT_CONSUMED,          // func_213070(slot) tras WeakHit
    H_HIT_MARK1,             // HitMarkEff(slot)
    H_HIT_MARK2,             // HitMarkEff2(slot, partType)
    H_CHAR_INIT_CLEAR,       // func_939280 al inicio de CharInit
    H_OWNER_CLEAR_FLAGS,     // Dead: *(owner+0x1C9C) &= ~3
    H_UPPER_HIT_FX,          // DamageUpperBody: inchiki_dammot_set(arg) + func_1FAA60 (a = 0/1 segun nado, b = valor de +0x194)
    H_FINISH_TASK,           // FinishTask -> func_950F80()
    H_PILLAR_START,          // Start__17TBoss5WaterPillar del pilar a
    H_SPAWN_SPHERE,          // new_Boss5Sphere(this)
    H_SPAWN_XICE,            // new_Boss5XIce(this)
};

// Todo lo que el original delega en el motor.
struct Boss5Env {
    virtual ~Boss5Env() {}
    virtual float camFrame() = 0;                          // *(float*)(G + G[1]*0xD0 + 0xC0), G = *(gp-0x7338)
    virtual int camTrackId() = 0;                          // *(int16*)(G + G[1]*0xD0 + 0xD0)
    virtual int camRot() = 0;                              // GetCamRot()
    virtual void setCamSpeed(float v) = 0;                 // op 31: *(float*)(G + G[1]*0xD0 + 0xC4) = v
    virtual int motionLen(int motion) = 0;                 // *(int16_t*)(0x1FB73E4 + motion*2)
    virtual int rand() = 0;                                // func_1A7EE0
    virtual float randFloat() = 0;                         // func_1A7F10
    virtual int damagePlayer(int player, int a, int dmg) = 0;  // func_212D00; 1 = el jugador recibio dano
    virtual void playMotion(int motion, int startFrame, int blend) = 0;   // func_1FA640
    virtual void sound(int id, int extra) = 0;             // func_217C30(&D_515F30, id, 0, 0, extra)
    virtual int statFunc(int a, int b) = 0;                // func_1D38B0(a, b)
    virtual float damageScale(int part) = 0;               // func_942F00(part)
    virtual void registerHit(int part, int pts) = 0;       // func_212FA0(part, pts)
    virtual bool playerAlive(int p) = 0;                   // func_951080(p) != 0
    virtual float groundY(const float pos[3]) = 0;         // func_951300
    virtual void hook(int what, int a, int b, int c) = 0;
};

struct Boss5Logic {
    // ---- VM de comandos ----
    static void seqInit(Boss5State& b, const uint16_t* table);              // SeqCmdInit
    static void seqNextTable(Boss5State& b);                                 // SeqCmdNextTable
    static void seqSetTable(Boss5State& b, int idx);                         // SeqCmdSetTable
    static void seqPush(Boss5State& b);                                      // SeqCmdPush
    static void seqPop(Boss5State& b);                                       // SeqCmdPop
    static void seqCall(Boss5State& b, int callScript);                      // SeqCmdCall
    static void seqCancelWait(Boss5State& b);                                // SeqCmdCancelWait
    static void execSeqCmd(Boss5State& b, Boss5Globals& g, Boss5Env& env);   // ExecSeqCmd
    static int actExecSeqCmd(Boss5State& b, Boss5Globals& g, Boss5Env& env); // actExecSeqCmd
    static void initBranchParam(Boss5State& b);                              // InitBranchParam
    static void motSupSet2(Boss5State& b, Boss5Env& env, int motion, int start, int blend);  // MotSupSet2
    static void checkSwimMotion(Boss5State& b);                              // CheckSwimMotion
    // ---- dificultad ----
    static void initDifficulty(Boss5State& b, int initial);                  // InitDifficulty (initial = func_96CEC0())
    static void setDiffParam(Boss5State& b, Boss5Globals& g);                // SetDiffParam
    static void upDifficulty(Boss5State& b);                                 // UpDifficulty
    static void downDifficulty(Boss5State& b);                               // DownDifficulty
    static void downDifficultyReq(Boss5State& b);                            // DownDifficultyReq
    static void noChgDifficultyReq(Boss5State& b);                           // NoChgDifficultyReq
    static int getStrength(const Boss5State& b, const Boss5Globals& g, Boss5Env& env);   // GetStrength (0, 1 o 2)
    // ---- vida / dano / muerte ----
    static void initLife(Boss5State& b, Boss5Globals& g, int life);          // InitLife
    static void controlDispLife(Boss5State& b, const Boss5Globals& g);       // ControlDispLife (parte numerica)
    static bool checkWeakHit(const Boss5State& b, int partType);             // CheckWeakHit
    struct HitInfo { int slot; int8_t partType; int8_t weakIdx; };   // por jugador: slot = *(gp-0x7350+4i) (-1 = sin golpe), partType = byte +0x70 de la zona, weakIdx = *(gp-0x7348+slot*4)
    static void checkDamage(Boss5State& b, Boss5Globals& g, Boss5Env& env, const HitInfo hit[2]);  // CheckDamage
    static void weakHit(Boss5State& b, Boss5Globals& g, Boss5Env& env, int partType, int hitIdx);   // WeakHit
    static void noWeakHit();                                                 // NoWeakHit (vacia)
    static void damageUpperBody(Boss5State& b, Boss5Env& env);               // DamageUpperBody
    static void damage(Boss5State& b, Boss5Globals& g, Boss5Env& env);       // Damage
    static void dead(Boss5State& b, Boss5Globals& g, Boss5Env& env, int hitIdx);   // Dead
    static void setDamageMotion(Boss5State& b, Boss5Env& env);               // SetDamageMotion
    static int actDelete(Boss5State& b, Boss5Globals& g, Boss5Env& env);     // actDelete
    static int actDead(Boss5State& b, Boss5Globals& g, Boss5Env& env);       // actDead
    static void checkDamageEndTimer(Boss5State& b);                          // CheckDamageEnd (solo el bloque del aturdimiento)
    // ---- ataques ----
    static bool selectAttackTarget(Boss5State& b, Boss5Globals& g, Boss5Env& env);   // SelectAttackTarget
    static void startAttack(Boss5State& b, Boss5Globals& g, Boss5Env& env, int id);  // StartAttack
    static void controlAttack(Boss5State& b, Boss5Globals& g, Boss5Env& env);  // ControlAttack
    static void attackDamageFrame(Boss5State& b, Boss5Globals& g, Boss5Env& env, int which);   // AttackDamageFrame
    static void damageToPlayer(Boss5State& b, Boss5Globals& g, Boss5Env& env);       // DamageToPlayer
    static void decNowEnemyAlvNumChk(Boss5State& b, Boss5Globals& g);        // DecNowEnemyAlvNumChk
    static void decNowEnemyNumChk(Boss5State& b, Boss5Globals& g);           // DecNowEnemyNumChk
    // ---- varios ----
    static void controlAlpha(Boss5State& b);                                 // ControlAlpha
    static void enableCollision(Boss5State& b, Boss5Env& env);               // EnableCollision
    static void disableCollision(Boss5State& b, Boss5Env& env);              // DisableCollision
    static void setHeight(Boss5State& b, Boss5Env& env);                     // SetHeight
    static void fallCheck(Boss5State& b, float groundY);                     // FallCheck
    static void charInit(Boss5State& b, Boss5Globals& g, Boss5Env& env, int scaledLife);   // CharInit (scaledLife = func_14C518(2500.0 * D_985830[func_96CED0(1)]))
    static void controlWaterPillar(Boss5State& b, const Boss5Globals& g, Boss5Env& env);    // ControlWaterPillar
    static int eTurn(int cur, int target, int step);                         // ETurn
    static bool angChk(int ang, int center, int range);                      // AngChk
    static void allowCameraStart(Boss5Globals& g) { g.cameraStartAllowed = 1; }   // AllowCameraStart
    static void forbidCameraStart(Boss5Globals& g) { g.cameraStartAllowed = 0; }  // ForbidCameraStart
};

// Proyectiles de hielo (TBoss5Ice, objeto aparte; posicion en +0x2C/+0x30/+0x34, velocidad en +0x38/+0x3C/+0x40).
struct IceState {
    float pos[3] = {0, 0, 0};    // +0x2C,+0x30,+0x34
    float vel[3] = {0, 0, 0};    // +0x38,+0x3C,+0x40
    float acc[3] = {0, 0, 0};    // +0x44,+0x48,+0x4C (solo Attack_chaku)
    float baseY = 0;             // +0x1B4 (altura final de UpShape)
    int shapeState = 0;          // +0x18C (4 = listo)
    int morphTimer = 0;          // +0x190
    float morph[4] = {0, 0, 0, 0};   // +0x1C0 (=1-t), +0x1C4 (=t), +0x1C8 (=1-t), +0x1CC (=t): ver Morph
    float homeDist = 0;          // +0xB0 (radio del objetivo)
    int8_t target = -1;          // +0x150 (jugador)
    int finished = 0;            // veces que se ha llamado func_950F80(this) (fin de la tarea)
};
struct IceLogic {
    // dist = func_944FF0(this) (distancia al objetivo)
    static void checkAttackEnd(IceState& s, Boss5State& owner, Boss5Env& env, float dist);   // CheckAttackEnd
    static void attackSphere(IceState& s, Boss5State& owner, Boss5Env& env, float dist);     // Attack_sphere
    static void attackTagger(IceState& s, Boss5State& owner, Boss5Env& env, float dist);     // Attack_tagger
    static void attackChaku(IceState& s, Boss5State& owner, Boss5Env& env, float dist);      // Attack_chaku
    static void upShape(IceState& s);             // UpShape
    static void morph(IceState& s);               // Morph
};

}}  // namespace vn::boss5

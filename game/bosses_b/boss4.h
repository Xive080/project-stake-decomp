// Logica del jefe 4 (TBoss4, overlay bs4mod, carga 0x9C4800), reescrita a mano a partir de asm/bs4mod/bs4mod_text.s.
// Cada funcion cita la original. Los campos se nombran por uso, con el offset de la estructura TBoss4 en comentario.
// Sin dependencias de plataforma: todo lo que el original pide al motor (sonido, movimiento, camara, colisiones, rand,
// globales gp-0x7xxx) pasa por `Boss4Env`. Ver MANIFEST.md para el estado de cada funcion.
#pragma once
#include <cstdint>
namespace vn { namespace boss4 {

// Tipo de combatiente (+0x1E1C). Nombres tomados de las funciones CtrlPre_battle1/2/3, CtrlPre_shoukan, CtrlPre_bunshin.
enum Kind : uint8_t { K_BATTLE1 = 0, K_BATTLE2 = 1, K_BATTLE3 = 2, K_SHOUKAN = 3, K_BUNSHIN = 4 };

// Estado de la tarea (+0x1F1C): valores vistos en el codigo.
enum TaskState : uint8_t { TS_DEAD = 4, TS_FINISHED = 3 };

// Banderas de +0x0C (heredadas de la tarea base de enemigos; solo las que usa el codigo de este overlay).
enum Flags0C : uint32_t {
    F0C_HIT = 0x8,               // CheckDamage: la tarea ha recibido un impacto este fotograma
    F0C_ATTACK_WINDOW = 0x100,   // ControlAttack: ventana de dano del ataque abierta
    F0C_COLLISION_OFF = 0x8000,  // Enable/DisableCollision
    F0C_DELETE = 0x4000,         // actDelete
    F0C_NO_HEIGHT = 0x40000,     // colliFunc: no ajustar altura
    F0C_DEAD = 0x04000000,       // Dead / actDelete
    F0C_ATTACKING = 0x10000000,  // StartAttack / ControlAttack / op 6
    F0C_IN_DAMAGE = 0x40000000,  // Damage
    F0C_WEAK = 0x80000000u,      // CheckDamage: el punto golpeado es debil
};
// Banderas de +0x1E14.
enum Flags1E14 : uint32_t {
    FB_ATTACK = 0x10, FB_VISIBLE = 0x20, FB_HIT_STUN = 0x100, FB_LIFEBAR = 0x2000, FB_BUNSHIN_ASK = 0x8000,
    FB_SHUSOKU_ASK = 0x10000, FB_BN_HOLD = 0x20000, FB_BN_CANCEL = 0x4000, FB_ALPHA_RUN = 0x100000,
    FB_DEAD_MASK = 0x200000,     // Dead lo limpia (no se ha podido nombrar mejor)
};

// ---- Tablas (generadas en boss4_data.cpp desde disc/TBL/BS4MOD.BIN y el ELF) ----
extern const int8_t kCmdLen[67];            // D_009D4250
extern const int16_t kScript[];             // scripts de comandos (int16)
extern const unsigned kScriptLen;
extern const uint16_t kSeqTab0[], kSeqTab1[], kSeqTab2[], kSeqTab3[];   // tablas de scripts por tipo
extern const uint16_t kCallScript[5];       // D_009D2CA0
struct KindConfig { uint32_t orC, andC, or1E14, and1E14; const uint16_t* seqTab; uint8_t initState; };
extern const KindConfig kKindConfig[5];     // D_009D3390
struct AttackEntry { int16_t motion; int8_t dmg; int16_t startFrame; int16_t f[6]; int16_t endFrame; float range; };   // f[] = +6,+8,+0xA,+0xC,+0xE,+0x10; endFrame = +0x12; range = +0x14
extern const AttackEntry kAttackTable[15];  // D_009DA500
struct RunRowA { int16_t mot[5]; int8_t atk[3]; };
struct RunRowB { int16_t mot[2]; int8_t atk[3]; };
extern const RunRowA kRunRowA[2];           // D_009D2BB0
extern const RunRowB kRunRowB[3];           // D_009D2BD0
extern const float kPose[3][6];             // D_009D2B60
extern const float kReappearPos[3][3];      // D_009D2BF0
extern const int16_t kSchedFrame[8];        // ELF 0x2D86F8 = gp-0x79F8 (gp = 0x2E00F0, .reginfo): fotogramas de camara de las reapariciones; solo [0..3] son datos de esta tabla
struct DiffRow { int8_t hp[17]; int32_t dmgMul[17]; float speed16[17]; int16_t stepUp, stepDown; };
extern const DiffRow kDiffRows[10];         // D_009D3C90: [tipo*2 + nivelJuego]
extern const int8_t kHpKind2[3][2][17];     // D_009D3470
extern const int16_t kLoopKind1[2][17];     // D_009D36C0
extern const int16_t kLoopKind2Sub0[2][17]; // D_009D3A90
extern const int16_t kLoopKind2Sub2[2][17]; // D_009D3BA0

constexpr int kStackDepth = 11;

// Estado de un TBoss4 (solo campos usados por la logica reescrita). Offsets de la tarea en comentario.
struct Boss4State {
    uint32_t f0C = 0;            // +0x0C
    uint32_t f10 = 0;            // +0x10 (bit1 = ya descontado de enemigos vivos, bit2 = ya descontado de enemigos, bit6 = cayendo)
    float pos[3] = {0, 0, 0};    // objeto de posicion (+0x30 -> +0,+4,+8)
    float rot[3] = {0, 0, 0};    // idem +0x24,+0x28,+0x2C
    float velY = 0, accelY = 0;  // idem +0x10 (velocidad vertical), +0x1C (aceleracion vertical)
    int typeId = 0;              // +0x5C (0x1C = tipo con manos separadas)
    int32_t f18 = 0;             // +0x18
    int32_t f2E38 = 0;           // +0x2E38 (puesto a 0 en Dead)
    int32_t f1E44 = 0;           // +0x1E44 (0x10 = clones en marcha, 0x33 = reunion)
    int8_t b1E11 = 0;            // +0x1E11 (subfase elegida por el flujo de camara)
    int motion = 0;              // +0x34 movimiento actual
    float motionFrame = 0;       // +0x3C fotograma del movimiento
    float camSpeed1E00 = 0;      // +0x1E00
    float f1E04 = 0, f1E08 = 0;  // +0x1E04/+0x1E08
    float f1DF8 = 0, f1DFC = 0;  // +0x1DF8/+0x1DFC
    int8_t b1DB8 = 0;            // +0x1DB8
    uint32_t f1E14 = 0;          // +0x1E14
    uint8_t kind = 0;            // +0x1E1C
    float alpha = 1.0f;          // +0x1E24
    float alphaStep = 0;         // +0x1E28
    int32_t loopCount = 0;       // +0x1E30 (repeticiones por dificultad)
    int32_t phaseCount = 0;      // +0x1E34 (golpes recibidos / fase)
    int8_t runA = 0, runB = 0, runC = 0, runD = 0;   // +0x1E38..+0x1E3B (SetRunKindRand)
    int8_t bunshinCmd = 0;       // +0x1E3C
    float speedBase = 0;         // +0x1E40
    int16_t motReqMode = 0, motReqA = 0, motReqB = 0;  // +0x1E46,+0x1E48,+0x1E4A (peticion de movimiento)
    int8_t b1E4C = 0;            // +0x1E4C (fase de la logica externa: 4 = herido (tipo 2), 5 = muerto)
    int8_t b1E4D = 0, b1E4E = 0; // +0x1E4D/+0x1E4E (decision aleatoria de fase 3)
    int8_t subPhase = 0;         // +0x1E50 (subfase del tipo 2: 0,1,2)
    int8_t b1E51 = 0;            // +0x1E51
    const int16_t* pc = nullptr; // +0x1E7C
    const uint16_t* seqTab = nullptr; // +0x1E80 (indices en kScript)
    int16_t tabIdx = 0;          // +0x1E84
    const int16_t* stack[kStackDepth] = {};  // +0x1E54
    int16_t timer[kStackDepth + 1] = {};     // +0x1E86 (un contador por nivel de pila; -1 = sin iniciar)
    int8_t timerSel = 0;         // +0x1E9A
    int8_t sp = 0;               // +0x1E9B
    float rate = 1.0f;           // +0x1E9C (velocidad de reproduccion efectiva)
    float rateBase = 1.0f;       // +0x1EA4
    float rateMul = 1.0f;        // +0x1EAC
    uint8_t state = 0, stateB = 0, stateC = 0;   // +0x1F1C,+0x1F1D,+0x1F1E
    int8_t attack = 0;           // +0x1F24
    int8_t target = -1;          // +0x1F25 (jugador objetivo; -1 = ninguno)
    int8_t b1F26 = 0;            // +0x1F26
    int16_t s1F40 = 0;           // +0x1F40
    int8_t atkStep = 0;          // +0x1F42
    float lifeRatio = 1.0f, hitRatio = 1.0f;     // +0x1F44, +0x1F48
    int16_t stunTimer = 0;       // +0x1F54 (aturdido tras golpe, cuenta atras)
    int8_t weakIdx = 0;          // +0x1F56
    int8_t hitCounter = 1;       // +0x1F59 (cuenta atras de impactos hasta contar dano)
    int8_t partLife = 0;         // +0x1F5A (golpes que quedan para la fase)
    int8_t hitsPerCount = 1;     // +0x1F5B
    int8_t partLifeMax = 0;      // +0x1F5C
    int32_t dmgMul = 0;          // +0x1F60
    int32_t difficulty = 0;      // +0x1F64 (0..16)
    int16_t diffUp = 0, diffDown = 0;   // +0x1F68,+0x1F6A
    int8_t diffArmed = 0;        // +0x1F6C
    uint8_t fallStep = 0;        // +0x1F6D
    // Para K_BUNSHIN/K_SHOUKAN: puntero al jefe padre (+0x2100)
    Boss4State* owner = nullptr;
};

// Globales del juego que leen estas funciones (gp-relativos, bss: no estan en el ELF).
struct Boss4Globals {
    int bossLife = 0;            // gp-0x7384 (vida actual compartida del jefe)
    int bossLifeMax = 0;         // gp-0x7388
    int16_t enemiesLeft = 0;     // gp-0x7374 (contador de enemigos)
    int16_t enemiesAlive = 0;    // gp-0x7370
    int targetBusy[2] = {0, 0};  // gp-0x7420, gp-0x741C (jugador ya elegido como objetivo)
    int lastHitTarget = 0;       // gp-0x73F4
    int diffMirror = 0;          // gp-0x73B8 (copia de la dificultad)
    int deathCount73FC = 0;      // gp-0x73FC
    int deathCount73F8 = 0;      // gp-0x73F8
    int gameState = 0;           // 0x500F14 (0xA = juego normal)
    int gameKind = 0;            // gp-0x73B4 (2 = modo especial)
    int gameLevel = 0;           // func_943AC0(): nivel de dificultad de la partida (0/1)
    int numPlayersB = 0;         // 0x500DFC
    uint8_t cameraStartAllowed = 0;   // 0x500E10 (AllowCameraStart = 1, ForbidCameraStart = 0)
    int8_t g73CC = 0, g72A0 = 0;      // gp-0x73CC, gp-0x72A0 (usados en actDead)
    int g729C = 0;                    // gp-0x729C
};

// Todo lo que el original delega en el motor. Los nombres son los del uso; los comentarios dan la funcion original.
struct Boss4Env {
    virtual ~Boss4Env() {}
    virtual float camFrame() = 0;                          // *(float*)(G + G[1]*0xD0 + 0xC0), G = *(gp-0x7338) (fotograma de la ruta de camara)
    virtual int camTrackId() = 0;                          // *(int*)(G + G[1]*0xD0 + 0xD0) (solo actDead)
    virtual void setCamSpeed(float v) = 0;                 // op 36: *(float*)(G + G[1]*0xD0 + 0xC4) = v
    virtual int motionLen(int motion) = 0;                 // *(int16_t*)(0x1FB73E4 + motion*2)
    virtual int rand() = 0;                                // func_1A7EE0
    virtual int damagePlayer(int player, int a, int dmg) = 0;  // func_212D00; 1 = el jugador recibio dano
    virtual void playMotion(int motion, int startFrame) = 0;   // func_1FA640(&D_4A13E0, objeto, motion, startFrame)
    virtual void sound(int id, int extra) = 0;             // func_217C30(&D_515F30, id, 0, 0[, extra])
    virtual void projectToScreen(const float pos[3], float& x, float& y) = 0;   // func_1AC3A0
    virtual int statFunc(int a, int b) = 0;                // func_1D38B0(a, b)
    virtual float damageScale(int part) = 0;               // func_942F00(part) (multiplicador de dano por zona)
    virtual void registerHit(int part, int pts) = 0;       // func_212FA0(part, pts) (puntuacion/estadisticas del impacto)
    virtual bool playerAlive(int p) = 0;                   // func_951080(p) != 0
    virtual void hook(int what, int a, int b, int c) = 0;  // funciones del motor/no reescritas: ver enum Hook
};
// Llamadas a funciones no reescritas, identificadas para `Boss4Env::hook`.
enum Hook {
    H_SET_FCV = 1,           // SetFcv(a,b,c)
    H_CLEAR_BODY_MOVE,       // ClearBodyMove(a)
    H_SET_BODY_MOVE,         // SetBodyMove(a)
    H_RAISE_GAREKI,          // RaiseGareki(a)
    H_RAISE_GAREKI2,         // RaiseGareki2(a)
    H_RAISE_GAREKI3,         // RaiseGareki3(a)
    H_FALL_GAREKI,           // FallGareki(a)
    H_SET_CAM_PATH_ATTR,     // SetCamPathAttr()
    H_ENABLE_PART_COLLISION, // EnableCollision: bits 0x40 de +0x1A8 de las 4 partes (+0x1D58)
    H_DISABLE_PART_COLLISION,
    H_START_WEAK_DISP,       // StartWeakDisp: bit 0x80 de +0x1A8 de las partes
    H_START_ATTACK_IWA,      // StartAttackIwa()
    H_START_ATTACK_KIRI_PRE, // StartAttackKiriPre(a)
    H_START_ATTACK_KIRI,     // StartAttackKiri()
    H_START_ATTACK_KIRI_SHOUKAN,
    H_SET_SCREEN_OFFSET,     // func_1ABD40(0,0) en op 60
    H_FORBID_CAMERA_ATTR,
    H_FINISH_TASK,           // FinishTask -> func_950F80()
    H_HIT_MARK1,             // HitMarkEff(slot)
    H_HIT_MARK2,             // HitMarkEff2(slot, partType)
    H_SPAWN_EFFECT,          // func_1FAA60(&D_4A13E0, pos, 1, a): efecto de golpe
    H_NO_WEAK_FX,            // func_946FD0(0) en NoWeakHit
    H_PLAY_HAND_MOTIONS,     // movimientos de las manos (GetHandMot) si typeId == 0x1C
    H_HIT_PREPARE,           // func_950910 al empezar CheckDamage
    H_HIT_CONSUMED,          // func_213070(slot) tras WeakHit
    H_CLEAR_WEAK_HIT,        // ClearWeakHit: quita 0x40000000 de +0x0C en las 4 partes (+0x1D58)
    H_CHAR_INIT_CLEAR,       // func_939280 al inicio de CharInit
    H_INIT_GAREKI,           // InitGareki (pendiente)
    H_INIT_KIRIKABE,         // InitKiriKabe (pendiente)
    H_BUNSHIN_FX,            // func_9443A0(1,5,0x1E) en BnStartBunshin
};

struct Boss4Logic {
    // ---- VM de comandos (ExecSeqCmd y auxiliares SeqCmd*) ----
    static void seqInit(Boss4State& b, const uint16_t* table);              // SeqCmdInit
    static void seqNextTable(Boss4State& b);                                 // SeqCmdNextTable
    static void seqSetTable(Boss4State& b, int idx);                         // SeqCmdSetTable
    static void seqPush(Boss4State& b);                                      // SeqCmdPush
    static void seqPop(Boss4State& b);                                       // SeqCmdPop
    static void seqCall(Boss4State& b, int callScript);                      // SeqCmdCall
    static void seqCancelWait(Boss4State& b);                                // SeqCmdCancelWait
    static void execSeqCmd(Boss4State& b, Boss4Globals& g, Boss4Env& env);   // ExecSeqCmd
    // ---- movimiento ----
    static void motSupSet2(Boss4State& b, Boss4Env& env, int motion, int a, int bb);  // MotSupSet2
    static void motSupSet3(Boss4State& b, Boss4Env& env, int motion, int a, int bb);  // MotSupSet3
    // ---- dificultad ----
    static void initDifficulty(Boss4State& b, int initial);                  // InitDifficulty (initial = func_96CEC0())
    static void setDiffParam(Boss4State& b, Boss4Globals& g);                // SetDiffParam
    static void upDifficulty(Boss4State& b);                                 // UpDifficulty
    static void downDifficulty(Boss4State& b);                               // DownDifficulty
    static void noChgDifficultyReq(Boss4State& b);                           // NoChgDifficultyReq
    // ---- vida / dano / muerte ----
    static void initLife(Boss4State& b, Boss4Globals& g, int life);          // InitLife
    static void controlDispLife(Boss4State& b, const Boss4Globals& g);       // ControlDispLife (parte numerica)
    static bool checkWeakHit(const Boss4State& b);                           // CheckWeakHit
    struct HitInfo { int slot; int8_t partType; int8_t weakIdx; };   // por jugador: slot = *(gp-0x7350+4i) (-1 = sin golpe), partType = byte +0x70 de la zona, weakIdx = *(gp-0x7348+slot*4)
    static void checkDamage(Boss4State& b, Boss4Globals& g, Boss4Env& env, const HitInfo hit[2]);  // CheckDamage
    static void weakHit(Boss4State& b, Boss4Globals& g, Boss4Env& env, int partType, int hitIdx);   // WeakHit
    static void noWeakHit(const Boss4State& b, Boss4Env& env, int partType);   // NoWeakHit
    static void damageUpperBody(Boss4State& b, Boss4Env& env);               // DamageUpperBody
    static void damage(Boss4State& b, Boss4Globals& g, Boss4Env& env);       // Damage
    static void dead(Boss4State& b, Boss4Globals& g, Boss4Env& env, int hitIdx);   // Dead
    static void setDamageMotion(Boss4State& b, Boss4Env& env);               // SetDamageMotion
    static void setDeadMotion(Boss4State& b, Boss4Env& env);                 // SetDeadMotion
    static int actDelete(Boss4State& b, Boss4Globals& g, Boss4Env& env);     // actDelete
    static int actDead(Boss4State& b, Boss4Globals& g, Boss4Env& env);       // actDead
    static int actBunshin(Boss4State& b);                                    // actBunshin
    static int actShoukan(Boss4State& b, Boss4Globals& g, Boss4Env& env);    // actShoukan
    // ---- ataques ----
    static bool selectAttackTarget(Boss4State& b, Boss4Globals& g, Boss4Env& env);   // SelectAttackTarget
    static void startAttack(Boss4State& b, Boss4Globals& g, Boss4Env& env, int id);  // StartAttack
    static void controlAttack(Boss4State& b, Boss4Globals& g, Boss4Env& env);        // ControlAttack
    static void attackDamageFrame(Boss4State& b, Boss4Globals& g, Boss4Env& env);    // AttackDamageFrame
    static void damageToPlayer(Boss4State& b, Boss4Globals& g, Boss4Env& env);       // DamageToPlayer
    static void setRunKindRand(Boss4State& b, Boss4Env& env);                // SetRunKindRand
    static void decNowEnemyAlvNumChk(Boss4State& b, Boss4Globals& g);        // DecNowEnemyAlvNumChk
    static void decNowEnemyNumChk(Boss4State& b, Boss4Globals& g);           // DecNowEnemyNumChk
    // ---- alfa, clones, colisiones ----
    static void controlAlpha(Boss4State& b);                                 // ControlAlpha
    static void startAlphaAdd(Boss4State& b);                                // StartAlphaAdd
    static void startAlphaSub(Boss4State& b);                                // StartAlphaSub
    static void startBunshin(Boss4State& b, Boss4Env& env);                  // StartBunshin
    static void startShusoku(Boss4State& b, Boss4Env& env);                  // StartShusoku
    static void cancelBunshin(Boss4State& b);                                // CancelBunshin
    static void enableCollision(Boss4State& b, Boss4Env& env);               // EnableCollision
    static void disableCollision(Boss4State& b, Boss4Env& env);              // DisableCollision
    static void fallCheck(Boss4State& b, float groundY);                     // FallCheck (groundY = func_951300(boss, pos))
    static void charInit(Boss4State& b, Boss4Globals& g, Boss4Env& env, int scaledLife);   // CharInit (scaledLife = func_14C518(10000*D_985830[func_96CED0()]))
    // ---- utilidades puras ----
    static int eTurn(int cur, int target, int step);                         // ETurn (gira un angulo de 16 bits hacia el objetivo)
    static bool angChk(int ang, int center, int range);                      // AngChk
};

}}  // namespace vn::boss4

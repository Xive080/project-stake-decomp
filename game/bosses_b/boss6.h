// Logica del jefe 6 (Boss6Task, overlay bs6mod, carga 0x9C4800), reescrita a mano a partir de asm/bs6mod/bs6mod_text.s.
// Cada funcion cita la original. Los campos de la tarea se nombran por uso con su offset de Boss6Task en comentario.
// Sin dependencias de plataforma: todo lo que el original pide al motor (sonido, movimiento, camara, colisiones, rand,
// globales gp-relativos) pasa por `Boss6Env` / `Boss6Globals`. Ver MANIFEST_boss6.md para el estado de cada funcion.
// Las direcciones de pc/tablas de scripts son direcciones de VM del original (overlay 0x9D....; ELF 0x2D88..) y se leen
// de las imagenes de boss6_data.cpp.
#pragma once
#include <cstdint>
namespace vn { namespace boss6 {

// Tipo de combatiente (+0x1DAC), usado en WeakHit/SetSpeedBaseRate/ExecSeqCmd (0..2).
enum Kind : uint8_t { K_PHASE0 = 0, K_PHASE1 = 1, K_PHASE2 = 2 };

// Banderas de +0x0C (tarea base de enemigos; solo las usadas por este overlay).
enum Flags0C : uint32_t {
    F0C_HIT = 0x8,                 // CheckDamage: la tarea recibio un impacto
    F0C_ATTACK_WINDOW = 0x100,     // CheckDamage: junto con 0x8000 anula el procesado de impactos
    F0C_COLLISION_OFF = 0x8000,    // Enable/DisableCollision
    F0C_ATTACKING = 0x10000000,    // StartAttack / ControlAttack / op 6
    F0C_DEAD = 0x04000000,         // actDelete
    F0C_IN_DAMAGE = 0x40000000,    // Damage / MotSupSet2 lo limpia
    F0C_WEAK = 0x80000000u,        // CheckDamage: el punto golpeado es debil
};
// Banderas de +0x1DA4 (nombres por uso).
enum Flags1DA4 : uint32_t {
    FA_ATTACK = 0x1,               // ataque en curso (StartAttack / ControlAttack / batKillAdd)
    FA_FORM2 = 0x2,                // segunda forma del cuerpo (typeId 0x25)
    FA_HITFLASH = 0x20,            // destello por impacto (DamageUpperBody / CheckDamageEnd / BodyAlphaControl)
    FA_NO_PART_COLLISION = 0x40,   // Enable/DisableCollision no tocan las partes
    FA_PATH = 0x80,                // sigue una ruta (ops 28,29,30,31)
    FA_CAMPATH = 0x100,            // ruta de camara (op 30)
    FA_PATH_DONE = 0x200,          // fin de ruta (op 35)
    FA_PATH_800 = 0x800,           // op 28: guarda posicion al entrar en ruta
    FA_PATH_1000 = 0x1000,         // solo se limpia (SetDamageMotion / op 31)
    FA_APPROACH = 0x2000,          // SetApproach / op 37
    FA_IN_DAMAGE_ANIM = 0x4000,    // Damage..CheckDamageEnd
    FA_ATK_8000 = 0x8000,          // ControlAttack: fase de dano abierta
    FA_LIFE_BLINK = 0x20000,       // ControlDispLife / op 53
    FA_DEAD = 0x40000,             // la fase de muerte esta decidida (WeakHit)
    FA_ALPHA_RUN = 0x100000,       // op 53
    FA_ATK_200000 = 0x200000,      // batKillAdd / ControlAttack
    FA_ATK_400000 = 0x400000,      // ControlAttack lo limpia
    FA_ATK_80000 = 0x80000,        // AttackDamageFrame (caso 2)
    FA_CIRCLE = 0x01000000,        // SetCircleMove / CircleMove / op 67
};

// ---- Tablas (generadas en boss6_data.cpp desde disc/TBL/BS6MOD.BIN y el ELF) ----
struct MotionRow { int32_t id, a, b; };          // D_009D2E40, filas de 12 bytes
struct AttackRow { int16_t motion, endFrame, startFrame, dmgFrame, f8, kind; };   // D_009D5000: +0 indice de movimiento (>0 = hay), +2 fin, +4 inicio, +6 fotograma de dano, +8, +0xA tipo de ataque
struct Pose { float p[3]; int32_t r[3]; };        // fila de 24 bytes: posicion (float) y rotacion (int32)
extern const int32_t kMotionTab_flat[54 * 3];
extern const int16_t kAttackTab_flat[28 * 6];
extern const int8_t kBulletNum[36];               // D_009D30D0
extern const float kSpeedMul[36];                 // D_009D3100
extern const int16_t kDmgPerHit[40];              // D_009D3440
extern const int8_t kHitCount0_flat[3 * 68];      // D_009D3190 [3][34][2]
extern const int8_t kHitCount1_flat[4 * 68];      // D_009D3260 [4][34][2]
extern const int8_t kHitCount2_flat[3 * 68];      // D_009D3370 [3][34][2]
extern const float kLifeRows_flat[17 * 6];        // D_009D3490 [17][6]
extern const Pose kPoseA_flat[12];                // D_009D3630 [4][3]
extern const Pose kPoseB_flat[16];                // D_009D3750 [4][4]
extern const Pose kPoseC_flat[20];                // D_009D38D0 [4][5]
extern const Pose kPoseD_flat[4];                 // D_009D3AB0 [4]
extern const int32_t kSoundIds[6];                // D_009D3B10
extern const float kLight_flat[90];               // D_009D430C [10][9]
extern const uint32_t kOvlImageBase;              // 0x9D3B20
extern const uint32_t kOvlImageLen;               // numero de int16
extern const int16_t kOvlImage[];                 // imagen int16 de D_009D3B20..D_009D4480
extern const uint32_t kElfImageBase;              // 0x2D8800
extern const int16_t kElfImage[16];
extern const uint32_t kSeqTabAddr[3];             // D_009D4350: direccion de la tabla de scripts de cada tipo

inline MotionRow motionRow(int i) { return {kMotionTab_flat[i * 3], kMotionTab_flat[i * 3 + 1], kMotionTab_flat[i * 3 + 2]}; }
inline AttackRow attackRow(int i) { const int16_t* p = kAttackTab_flat + i * 6; return {p[0], p[1], p[2], p[3], p[4], p[5]}; }
// Lectura de la imagen de VM (direcciones del original).
int16_t vmRead16(uint32_t addr);
uint32_t vmRead32(uint32_t addr);

// Estado de una Boss6Task (solo campos usados por la logica reescrita). Offsets en comentario.
struct Boss6State {
    uint32_t f0C = 0;            // +0x0C
    uint32_t f10 = 0;            // +0x10 (bit1 = ya descontado de vivos, bit2 = ya descontado de enemigos, 0x20 = fin)
    int32_t f18 = 0;             // +0x18
    float pos[3] = {0, 0, 0};    // objeto de posicion (+0x30 -> +0,+4,+8)
    int32_t rot[3] = {0, 0, 0};  // idem +0x24,+0x28,+0x2C (angulos enteros)
    int32_t motion = 0;          // +0x34 id de movimiento en curso
    float motionFrame = 0;       // +0x3C
    float f40 = 0;               // +0x40
    int typeId = 0;              // +0x5C (0x25 = cuerpo principal, 0x26 = otro)
    bool partPresent[4] = {false, false, false, false};   // +0x1D58[i] != NULL
    uint32_t partF0C[4] = {0, 0, 0, 0};                   // (+0x1D58[i])->+0x0C
    uint32_t partF1A8[4] = {0, 0, 0, 0};                  // (+0x1D58[i])->+0x1A8
    uint32_t parentF1C9C = 0;    // (this->+0x08)->+0x1C9C
    uint32_t f1DA4 = 0;          // +0x1DA4
    uint8_t kind = 0;            // +0x1DAC
    uint8_t pad1DAD = 0;
    float rate = 1.0f;           // +0x1E18 velocidad de reproduccion
    float f1E1C = 0, f1E24 = 0, f1E2C = 1.0f;   // +0x1E1C, +0x1E24, +0x1E2C (DamageUpperBody)
    float rate2 = 1.0f;          // +0x1E90
    int8_t sel1DC4 = 0;          // +0x1DC4 (indice de fila A)
    int8_t sel1DC5 = 0;          // +0x1DC5 (indice de fila B)
    uint32_t pc = 0;             // +0x1DB8 direccion de VM del comando actual
    uint32_t seqTab = 0;         // +0x1DBC direccion de VM de la tabla de scripts
    int16_t tabIdx = 0;          // +0x1DC0
    int16_t timer = 0;           // +0x1DC2 (cuenta atras de op 7)
    float branchRate[18];        // +0x1DC8..+0x1E0C (pesos de GetRandomBranchRate)
    int8_t b1E10 = 0, b1E11 = 0; // +0x1E10 / +0x1E11 (ultima y penultima rama elegida)
    int32_t f1E14 = 0;           // +0x1E14
    uint8_t state = 0, stateB = 0;   // +0x1E60, +0x1E61
    int8_t attack = 0;           // +0x1E64 (fila de la tabla de ataques)
    int8_t target = -1;          // +0x1E65 (jugador objetivo, -1 = ninguno)
    int8_t gameMode = 0;         // +0x1E66 (copia de 0x500DFC al empezar un ataque)
    int8_t atkStep = 0;          // +0x1E82 (paso de ControlAttack: 0, 100, 10, 11, -1)
    int8_t atkDiff = 0;          // +0x1E83 (dificultad al empezar el ataque)
    int16_t s1E80 = 0;           // +0x1E80
    int32_t pathId = 0;          // +0x1E84
    float pathFrame = 0;         // +0x1E88
    int32_t pathEnd = 0;         // +0x1E8C
    float savedPos[3] = {0, 0, 0};   // +0x1E94..+0x1E9C (posicion guardada en op 28)
    int32_t savedRot[3] = {0, 0, 0}; // +0x1EA0..+0x1EA8
    float apprPos[3] = {0, 0, 0};    // +0x1ED0..+0x1ED8 (SetApproach)
    int32_t apprRot[3] = {0, 0, 0};  // +0x1EDC..+0x1EE4
    int32_t f1EC4 = 0; float f1EC8 = 0, f1ECC = 0;   // +0x1EC4, +0x1EC8, +0x1ECC
    float alphaCur = 1.0f;       // +0x1F00
    float alphaTarget = 1.0f;    // +0x1F04
    float spawnPos[3] = {0, 0, 0};   // +0x1F54..+0x1F5C (posicion de aparicion de mascaras)
    float lifeRatio = 1.0f, hitRatio = 1.0f;   // +0x1F34, +0x1F38
    int16_t blinkTimer = 0;      // +0x1F46
    int8_t hitCounter = 1;       // +0x1F4B (cuenta atras de golpes de la barra pequena)
    int8_t phaseHits = 0;        // +0x1F4C (golpes de fase que quedan)
    int8_t hitCounterMax = 1;    // +0x1F4D
    int8_t phaseHitsMax = 1;     // +0x1F4E
    int8_t weakIdx = 0;          // +0x1F48 (byte; en CheckDamage)
    int32_t curMotionIdx = -1;   // +0x1F68 (indice de la tabla de movimientos en curso)
    struct MotRec { int32_t cur, prev, a, b; int32_t pad[2]; } rec[2];   // +0x1F6C y +0x1F84 (stride 0x18)
    int32_t bulletNum = 0;       // +0x1F65 (s8)
    int8_t atkKind = 0;          // +0x1F64
    int8_t atkFlag = 0;          // +0x1F66
    int8_t atkCount = 0;         // +0x1F67
    int32_t dmgMul = 0;          // +0x1F60 (ControlAttack)
    int32_t f1F7C = 0, f1F94 = 0;   // +0x1F7C / +0x1F94 (op 15)
    float alphaOut1 = 1.0f;      // +0x1F80
    float alphaOut2 = 1.0f;      // +0x1F98
    int32_t hitFlags = 0;        // +0x1F9C (bits de golpes de murcielagos: batKillAdd)
    int32_t light = 0;           // +0x1FA0 (indice de luz actual)
    int32_t lightCount = 0;      // +0x1FA4
    float lightColA[3] = {0}, lightColB[3] = {0}, lightColC[3] = {0};   // +0x1FAC.., +0x1FBC.., +0x1FCC..+0x1FD4
    float speedBase = 1.0f;      // +0x1FD8 (SetSpeedBaseRate)
    int32_t lightRes[3] = {0, 0, 0};   // +0x1FA8, +0x1FB8, +0x1FC8 (puestos a 0 en setLightColor)
    bool gauge[2] = {false, false};    // +0x1F3C, +0x1F40 (barras de vida creadas)
};

// Globales del juego que leen estas funciones (gp-relativos o direcciones fijas; bss: no estan en el ELF).
struct Boss6Globals {
    int16_t diff[2] = {0, 0};    // gp-0x7030 (jugador 0), gp-0x702E (jugador 1): dificultad 0..16
    int lifeRowIdx = 0;          // indice de fila de kLifeRows_flat (nivel de partida)
    int lifeInitFlag = 0;        // gp-0x702C (1 = vida compartida aun sin inicializar)
    int bossLife = 0;            // gp-0x7384
    int bossLifeMax = 0;         // gp-0x7388
    int16_t enemiesLeft = 0;     // gp-0x7374
    int16_t enemiesAlive = 0;    // gp-0x7370
    int targetBusy[2] = {0, 0};  // gp-0x7420, gp-0x741C
    int lastHitTarget = 0;       // gp-0x73F4 (jugador de la partida de 1 jugador)
    int diffMirror = 0;          // gp-0x73B8
    int8_t deathCount73FC = 0;   // gp-0x73FC (lb)
    int8_t deathCount73F8 = 0;   // gp-0x73F8 (lb)
    int g72D0 = 0;               // gp-0x72D0
    int16_t gameState = 0;       // 0x500F14 (0xA = juego normal)
    int gameKind = 0;            // gp-0x73B4 (2 = modo especial)
    int16_t playersMode = 0;     // 0x500DF8 (1 = un jugador, 2 = dos jugadores)
    int16_t gameMode = 0;        // 0x500DFC (2 = modo dos jugadores para Inc/DecBS6Difficulty y StartAttack)
    uint8_t cameraStartAllowed = 0;   // 0x500F07 (AllowCameraStart = 1, ForbidCameraStart = 0)
};

// Todo lo que el original delega en el motor. Los comentarios dan la funcion original.
struct Boss6Env {
    virtual ~Boss6Env() {}
    virtual float camFrame() = 0;                     // *(float*)(G + G[1]*0xD0 + 0xC0), G = *(gp-0x7338)
    virtual int camTrackId() = 0;                     // *(int*)(G + G[1]*0xD0 + 0xD0)
    virtual void setCamMode(int v) = 0;               // func_1C9810(G, 0, v)
    virtual int motionLen(int motion) = 0;            // *(int16_t*)(0x1FB73E4 + motion*2)
    virtual int rand() = 0;                           // func_1A7EE0
    virtual float randFloat() = 0;                    // func_1A7F10 (0..1)
    virtual void startMotion(int id) = 0;             // func_1FA5D0(&D_4A13E0, objeto, id)
    virtual void startMotionBlend(int id, int a, int mode) = 0;   // func_1FA7A0(&D_4A13E0, objeto, id, a, mode)
    virtual void spawnEffect(const float pos[3], int id) = 0;   // func_1FAA60(&D_4A13E0, pos, 1, id)
    virtual void sound(int id, int id2, int flag) = 0;      // func_217C30(&D_515F30, id, id2, flag[, 0x40])
    virtual int statFunc(int a, int b) = 0;           // func_1D38B0(a, b)
    virtual float damageScale(int slot) = 0;          // func_942F00(slot)
    virtual void registerHit(int slot, int pts) = 0;  // func_212FA0(slot, pts)
    virtual bool playerAlive(int p) = 0;              // func_951080(p) != 0
    virtual int damagePlayer(int p, int a, int dmg) = 0;    // func_212D00; 1 = el jugador recibio dano
    virtual int pathEndFrame(int pathId) = 0;         // func_1CCCB0(gp-0x7310, pathId)
    virtual void spawnMask(const float pos[3], int mode) = 0;   // new TBoss6XB4Mask (ops 43/44)
    virtual void hook(int what, int a, int b, int c) = 0;   // funciones del motor/no reescritas: ver enum Hook
};
enum Hook {
    H_HIT_PREPARE = 1,       // func_950910 al empezar CheckDamage
    H_HIT_CONSUMED,          // func_213070(slot) tras WeakHit
    H_HIT_MARK,              // HitMarkEff(slot)
    H_HIT_MARK0,             // HitMarkEff0(slot, partType)
    H_NO_WEAK_FX,            // func_946FD0(0) en NoWeakHit
    H_FINISH_TASK,           // FinishTask -> func_950F80()
    H_RESTORE_POSE_AFTER_DAMAGE,   // CheckDamageEnd: recolocacion con matrices (typeId 0x25)
    H_DAMAGE_MOTION_GEOMETRY,      // SetDamageMotion: orienta la pose hacia la camara (typeId 0x25)
    H_CAM_TRANSFORM_A,       // op 19: pos desde camara (signo -)
    H_CAM_TRANSFORM_B,       // op 20: pos desde camara
    H_CAM_POS_TO_SPAWN,      // op 43 con P == 0: spawnPos <- posicion de camara
    H_SPAWN_ALLBAT,          // op 26: new_Boss6AllBat
    H_INIT_BLUR, H_END_BLUR, // ops 64
    H_SET_CIRCLE_MOVE,       // op 66: SetCircleMove(P)
    H_SET_BAT_SPIRAL,        // AttackDamageFrame caso 1
    H_ADD_BAT_SPIRAL_COUNT,  // ControlAttack
    H_NEW_BULLET,            // new_TBoss6XBullet(this, a, b)
    H_NEW_WATER,             // new_BS6B5Water
    H_CLEAR_BULLET_DAMAGE,   // ClearTBoss6XBulletDamage
    H_DEC_DIFFICULTY_NOTE,   // reservado
    H_FREE_GAUGE,            // FinishTask0: func_1A1F00(gauge[i])
    H_ALLOC_GAUGE,           // InitLife: gauge(a)
    H_DRAW_GAUGE,            // ControlDispLife: func_948FB0(ratio, gauge)
};

struct Boss6Logic {
    // ---- dificultad (0..16 por jugador; 17..33 = combinada de dos jugadores) ----
    static void initBS6Difficulty(Boss6Globals& g, int level, int initial);        // InitBS6Difficulty (level = func_96CED0(), initial = func_96CEC0())
    static int incBS6Difficulty(Boss6Globals& g, int slot, int d1, int d2);        // IncBS6Difficulty
    static int decBS6Difficulty(Boss6Globals& g, int slot, int d1, int d2);        // DecBS6Difficulty
    static int getBS6Difficulty(Boss6Globals& g, int slot);                        // GetBS6Difficulty
    // ---- vida ----
    static void initLife(Boss6State& b, Boss6Globals& g, Boss6Env& env, int life);   // InitLife
    static void controlDispLife(Boss6State& b, const Boss6Globals& g, Boss6Env& env);// ControlDispLife
    // ---- impactos y dano ----
    struct HitInfo { int slot; int8_t partType; int8_t weakIdx; };   // por jugador: slot = *(gp-0x7350+4i) (-1 = sin golpe), partType = byte +0x70 de la zona, weakIdx = *(gp-0x7348+slot*4)
    static bool checkWeakHit(const Boss6State& b);                                   // CheckWeakHit
    static void clearWeakHit(Boss6State& b);                                         // ClearWeakHit
    static void checkDamage(Boss6State& b, Boss6Globals& g, Boss6Env& env, const HitInfo hit[2]);   // CheckDamage
    static void weakHit(Boss6State& b, Boss6Globals& g, Boss6Env& env, int partType, int slot);     // WeakHit
    static void noWeakHit(const Boss6State& b, Boss6Env& env, int slot);             // NoWeakHit
    static void damageUpperBody(Boss6State& b, Boss6Env& env, int slot);             // DamageUpperBody
    static void damage(Boss6State& b, Boss6Globals& g, Boss6Env& env, int slot);     // Damage
    static void setDamageMotion(Boss6State& b, Boss6Env& env);                       // SetDamageMotion
    static void checkDamageEnd(Boss6State& b, Boss6Env& env);                        // CheckDamageEnd
    static void batKillAdd(Boss6State& b, int bits);                                 // batKillAdd
    // ---- movimiento ----
    static void motSupSet2(Boss6State& b, Boss6Env& env, int index, int blend, int mode);   // MotSupSet2
    // ---- VM de comandos ----
    static void seqInit(Boss6State& b, uint32_t table);                              // SeqCmdInit
    static void seqNextTable(Boss6State& b);                                         // SeqCmdNextTable
    static void seqSetTable(Boss6State& b, int idx);                                 // SeqCmdSetTable
    static void execSeqCmd(Boss6State& b, Boss6Globals& g, Boss6Env& env);           // ExecSeqCmd
    static int actExecSeqCmd(Boss6State& b, Boss6Globals& g, Boss6Env& env);         // actExecSeqCmd
    // ---- ataques ----
    static bool selectAttackTarget(Boss6State& b, Boss6Globals& g, Boss6Env& env);   // SelectAttackTarget
    static void setBulletNum(Boss6State& b, Boss6Globals& g, int attack);            // SetBulletNum
    static void atcPlFlgClear(Boss6State& b, Boss6Globals& g);                       // AtcPlFlgClear
    static void startAttack(Boss6State& b, Boss6Globals& g, Boss6Env& env, int attack);   // StartAttack
    static void controlAttack(Boss6State& b, Boss6Globals& g, Boss6Env& env);        // ControlAttack
    static void attackDamageFrame(Boss6State& b, Boss6Globals& g, Boss6Env& env);    // AttackDamageFrame
    static void decNowEnemyAlvNumChk(Boss6State& b, Boss6Globals& g);               // DecNowEnemyAlvNumChk
    static void decNowEnemyNumChk(Boss6State& b, Boss6Globals& g);                  // DecNowEnemyNumChk
    // ---- estados de tarea ----
    static int actNop();                                                             // actNop
    static int actDelete(Boss6State& b, Boss6Globals& g, Boss6Env& env);             // actDelete
    static int actDead();                                                            // actDead
    static void finishTask(Boss6Env& env);                                           // FinishTask
    static void finishTask0(Boss6State& b, Boss6Globals& g, Boss6Env& env);          // FinishTask0
    static void allowCameraStart(Boss6Globals& g);                                   // AllowCameraStart
    static void forbidCameraStart(Boss6Globals& g);                                  // ForbidCameraStart
    static void enableCollision(Boss6State& b);                                      // EnableCollision
    static void disableCollision(Boss6State& b);                                     // DisableCollision
    static void loopSoundOff(Boss6Env& env);                                         // LoopSoundOff
    // ---- ruta, alfa, ramas, luz, velocidad ----
    static int getPathEndFrame(Boss6State& b, Boss6Env& env);                        // GetPathEndFrame
    static void setApproach(Boss6State& b, int param);                               // SetApproach
    static void bodyAlphaControl(Boss6State& b);                                     // BodyAlphaControl
    static void bodyAlphaDirectSet(Boss6State& b, float a);                          // BodyAlphaDirectSet
    static void bodyAlphaSupSet(Boss6State& b, float a);                             // BodyAlphaSupSet
    static void initBranchRate(Boss6State& b);                                       // InitBranchRate
    static int getRandomBranchRate(Boss6State& b, Boss6Env& env, int n);             // GetRandomBranchRate
    static void setSpeedBaseRate(Boss6State& b);                                     // SetSpeedBaseRate
    static void setLightColor(Boss6State& b, int idx);                               // setLightColor
    static void ctrlBossLight(Boss6State& b);                                        // CtrlBossLight
    static int b6InterDivAngF(int a, int b, float t);                                // B6InterDivAngF
};

}}  // namespace vn::boss6

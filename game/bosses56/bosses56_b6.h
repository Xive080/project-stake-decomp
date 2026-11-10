// Jefe 6 (Boss6Task y clases auxiliares, overlay bs6mod, carga 0x9C4800): movimiento por rutas y orquestacion por fotograma
// de la tarea principal, murcielagos (TBoss6XBat) y pequenas clases de efecto (SkyFade, MaskFeather, AfterImage).
// Reescrito a mano a partir de asm/bs6mod/bs6mod_text.s. Complementa game/bosses_b/boss6.* (que no se modifica y se reutiliza
// con #include: Boss6State, Boss6Logic, tablas de poses). Los campos nuevos viven en `B6MoveState` / `B6BatState` con su offset.
// Todo lo que el original pide al motor pasa por `B6MoveEnv` / `B6BatEnv`. Sin rutas absolutas ni dependencias de plataforma.
#pragma once
#include <cstdint>
#include "../bosses_b/boss6.h"
namespace vn { namespace boss56 {

// Banderas de +0x1DA4 (mismos nombres que boss6.h; aqui solo las usadas).
constexpr uint32_t B6A_PATH = 0x80;           // sigue una ruta (TracePathParam)
constexpr uint32_t B6A_CAMPATH = 0x100;       // ruta de camara (GetPathParamCT)
constexpr uint32_t B6A_PATH_DONE = 0x200;     // fin de ruta
constexpr uint32_t B6A_PATH_400 = 0x400;      // giro en cuartos de vuelta con sel1DC4
constexpr uint32_t B6A_PATH_800 = 0x800;      // ruta relativa a la posicion/rotacion guardadas (ops 28..)
constexpr uint32_t B6A_PATH_LOOP = 0x1000;    // la ruta da la vuelta
constexpr uint32_t B6A_APPROACH = 0x2000;     // acercamiento en curso
constexpr uint32_t B6A_LIFEBAR = 0x100000;    // CtrlPre: ControlDispLife
constexpr uint32_t B6A_CIRCLE = 0x01000000;   // movimiento circular
constexpr uint32_t B6F0C_FROZEN = 0x4000;     // +0x0C: movimiento detenido (AddMotionFrame)

// ANPO: posicion + rotacion (6 palabras).
struct B6Anpo { float pos[3]; int32_t rot[3]; };

struct B6MoveState {
    uint32_t f0C = 0;                // +0x0C
    uint32_t f1DA4 = 0;              // +0x1DA4
    uint8_t kind = 0;                // +0x1DAC (0..2)
    int8_t sel1DC4 = 0;              // +0x1DC4
    int8_t sel1DC5 = 0;              // +0x1DC5
    float pos[3] = {0, 0, 0};        // objeto +0x30 -> +0,+4,+8
    int32_t rot[3] = {0, 0, 0};      // idem +0x24,+0x28,+0x2C
    float frame3C = 0, frame40 = 0;  // +0x3C / +0x40 (AddMotionFrame)
    float rate = 1.0f, rate1E1C = 0; // +0x1E18 / +0x1E1C
    float rate2 = 1.0f;              // +0x1E90 (avance de la ruta)
    int32_t rotOrder = 0;            // +0x1CC8
    int32_t pathId = 0;              // +0x1E84
    float pathFrame = 0;             // +0x1E88
    int32_t pathEnd = 0;             // +0x1E8C
    float savedPos[3] = {0, 0, 0};   // +0x1E94..+0x1E9C
    int32_t savedRot[3] = {0, 0, 0}; // +0x1EA0..+0x1EA8
    B6Anpo pathOut{};                // +0x1EAC..+0x1EC0 (resultado de TracePathParam)
    float apprFrame = 0, apprStep = 0, apprTotal = 0;   // +0x1EC4, +0x1EC8, +0x1ECC (SetApproach)
    float apprPos[3] = {0, 0, 0};    // +0x1ED0..+0x1ED8
    int32_t apprRot[3] = {0, 0, 0};  // +0x1EDC..+0x1EE4
    B6Anpo lastPath{};               // +0x1EE8..+0x1EFC (copia del resultado de ruta)
    float alphaCur = 1.0f;           // +0x1F00
    // movimiento circular (SetCircleMove / CircleMove)
    int32_t circAng0 = 0;            // +0x1F08
    float circY0 = 0, circR0 = 0;    // +0x1F0C, +0x1F10
    int32_t circAng1 = 0;            // +0x1F14
    float circY1 = 0, circR1 = 0;    // +0x1F18, +0x1F1C
    int32_t circAngDelta = 0;        // +0x1F20
    float circDY = 0, circDR = 0;    // +0x1F24, +0x1F28
    float circCnt = 0, circTotal = 0;// +0x1F2C, +0x1F30
    int32_t effectTick = 0;          // +0x1FE0 (contador que usa EffectCtrl)
    // copia del fotograma anterior (CtrlPre) +0x1E30..+0x1E5C
    float savedF[9] = {0};           // +0x1E30..+0x1E50 (pos y los 6 floats del objeto)
    int32_t savedI[3] = {0, 0, 0};   // +0x1E54..+0x1E5C
    float obj0C[3] = {0}, objScale[3] = {0};   // objeto +0x0C..+0x14 / +0x18..+0x20
};

struct B6MoveEnv {
    virtual ~B6MoveEnv() {}
    virtual void camPos(float out[3]) = 0;                              // G + G[1]*0xD0 + 0x90
    virtual void pathEval(float frame, int id, B6Anpo& out) = 0;        // func_1CC710(frame, gp-0x7310, id, out)
    virtual void pathEvalCT(float frame, int id, B6Anpo& out, float target[3]) = 0;   // func_1CC590: muestra + punto de mirada
    virtual void lookAt(float dx, float dy, float dz, int32_t& rot0, int32_t& rot1) = 0;   // func_1C8BD0
    virtual int atan2Ang(float dx, float dz) = 0;                       // func_1A79A0
    virtual void transformSaved(const B6MoveState& b, B6Anpo& out) = 0; // rama 0x800: matriz de camara + 1E94/1E9C + ejes en orden 1CC8 -> pos de `out`
    virtual void rotYawVec(int ang, float v[3]) = 0;                    // func_1B0834 + func_1B03EC(ang) + func_1AFC00(v, v)
    virtual void rotCircle(int ang, float radius, float y, float out[3]) = 0;   // CircleMove: (0, y, -radius) girado `ang` en Y
    virtual float sqrtf_(float v) = 0;                                  // func_1B0AA0
    virtual void spawnCircleCamera(int dir) = 0;                        // new_Boss6_2CameraCtrl(this, dir)
    virtual void spawnAfterImage() = 0;                                 // new_Boss6AfterImage(this)
};

// Pasos de CtrlPre.
enum B6Step {
    B6S_CHECK_DAMAGE_END = 1,   // CheckDamageEnd (boss6.cpp)
    B6S_DISPATCH_STATE,         // func_1C6C80(this, estado 1E60) -> 1E62
    B6S_UPDATE_PATH,            // UpdatePath (aqui)
    B6S_CHECK_DAMAGE,           // CheckDamage
    B6S_CONTROL_ATTACK,         // ControlAttack
    B6S_BODY_ALPHA,             // BodyAlphaControl (solo tipos 1 y 2)
    B6S_SET_ALPHA,              // SetAlpha(1F00) (solo tipos 1 y 2)
    B6S_LIFE_DISPLAY,           // ControlDispLife (flag 0x100000)
};
struct B6StepEnv { virtual ~B6StepEnv() {} virtual void step(int what) = 0; };

struct B6Move {
    static void addMotionFrame(B6MoveState& b);                                  // AddMotionFrame (0x9C5460)
    static void addPathFrame(B6MoveState& b);                                    // AddPathFrame (0x9C5570)
    static void approachANPO(B6MoveState& b, const B6Anpo& tgt);                 // ApproachANPO (0x9C5700)
    static void getPathParamPoint(B6MoveState& b, B6MoveEnv& env, float frame, int id, B6Anpo& out);   // GetPathParam(float,int,ANPO*) (0x9CA100)
    static void getPathParamCT(B6MoveEnv& env, float frame, int id, B6Anpo& out);                     // GetPathParamCT (0x9CA250)
    static void getPathParam(B6MoveState& b, B6MoveEnv& env, B6Anpo& out);       // GetPathParam(ANPO*) (0x9C9DC0)
    static void tracePathParam(B6MoveState& b, B6MoveEnv& env);                  // TracePathParam (0x9C9D00)
    static void updatePath(B6MoveState& b, B6MoveEnv& env);                      // UpdatePath (0x9C54A0)
    static void setCircleMove(B6MoveState& b, B6MoveEnv& env, int dir);          // SetCircleMove (0x9CACF0)
    static void circleMove(B6MoveState& b, B6MoveEnv& env);                      // CircleMove (0x9CAEF0)
    static void effectCtrl(B6MoveState& b, B6MoveEnv& env);                      // EffectCtrl (0x9CB100)
    static void ctrlPre(B6MoveState& b, B6StepEnv& st);                          // CtrlPre (0x9C4F30)
};

// ---------- Murcielagos (TBoss6XBat) ----------
// Modos de +0x1BE.
enum B6BatMode : int16_t { BAT_IDLE = 0, BAT_WAIT = 1, BAT_SHOT = 2, BAT_SPIRAL = 3, BAT_FALL = 4, BAT_DEAD = 5 };
// Banderas de +0x1BC.
constexpr int16_t BATF_ACTIVE = 0x1, BATF_PATH = 0x2, BATF_SPIRAL = 0x4, BATF_APPROACH = 0x8, BATF_SHOT = 0x10;

struct B6BatState {
    int32_t hp = 1;                  // +0x18
    float pos[3] = {0, 0, 0};        // +0x2C,+0x30,+0x34
    float radius = 0;                // +0xB0 (radio del murcielago, usado por PlHitChk)
    int8_t target = -1;              // +0x150
    int8_t gameMode = 0;             // +0x151
    int32_t killBits = 0;            // +0x194 (bits que suma batKillAdd)
    float fall[3] = {0, 0, 0};       // +0x1AC,+0x1B0,+0x1B4 (velocidad de caida)
    int16_t index = 0;               // +0x1B8
    int16_t flags = 0;               // +0x1BC
    int16_t mode = 0;                // +0x1BE
    int16_t alertSound = 0;          // +0x1C6 (distinto de 0: avisa con func_A10B00(0x1A) al morir)
    int32_t rotA = 0, rotB = 0, rotC = 0, rotSpeed = 0;   // +0x1CC,+0x1D0,+0x1D4,+0x1D8
    float rotRadius = 0, rotRadiusTarget = 0;             // +0x1DC, +0x1E0
    float dest[3] = {0, 0, 0};       // +0x1E4,+0x1E8,+0x1EC (destino al que se acerca)
    float apprT = 0, apprStep = 0;   // +0x1F0, +0x1F4
    int32_t pathId = 0;              // +0x258
    float pathT = 0, pathStep = 0, pathLen = 0;   // +0x25C, +0x260, +0x264
    float shotLen = 0, shotRatio = 0;             // +0x24C, +0x250 (longitud del disparo y su razon 1/273.368)
    float sample[3] = {0, 0, 0};     // +0x268,+0x26C,+0x270
    bool deleted = false;            // se ha llamado a func_1A2630 (borrar la tarea)
};

// Globales del murcielago (gp-0x7xxx, bss: inyectadas). Se rellenan por SetBatSpiral / AddBatSpiralCount.
struct B6BatGlobals {
    float spiralStart = 0;           // gp-0x7014
    float spiralRate = 1.0f;         // gp-0x7010
    float spiralLen = 0;             // gp-0x7018
    float spiralSlope = 0;           // gp-0x7020
    float spiralDist = 0;            // gp-0x7024 (longitud del vector de la espiral; la calcula el motor)
    int spiralHitFlag = 0;           // gp-0x701C
};

struct B6BatEnv {
    virtual ~B6BatEnv() {}
    virtual int rand() = 0;                                          // func_1A7EE0
    virtual float randFloat() = 0;                                   // func_1A7F10
    virtual int pathLen(int id) = 0;                                 // func_1CCCB0
    virtual void pathEval(float t, int id, float out[3]) = 0;        // func_1CC710 (solo la posicion)
    virtual void rotMatrixAdd(const float v[3], float dest[3]) = 0;  // func_1B00E0(matriz) + func_1AFC00(v, dest)
    virtual float sinAng(int a) = 0;                                 // func_1B09B4 (nombre asumido; no verificado cual es seno)
    virtual float cosAng(int a) = 0;                                 // func_1B0938
    virtual void sound(int id, int a, int b, int c) = 0;             // func_217C30(&D_515F30, id, a, b, c)
    virtual void alert(int v) = 0;                                   // func_A10B00(v)
    virtual void spawnBat(int index) = 0;                            // new_Boss6Bat(owner, index)
    virtual int damagePlayer(int player, int a, int dmg) = 0;        // func_212D00; 1 = recibio dano
    virtual float distToCam(const float pos[3]) = 0;                 // func_1B0AA0(|pos - camara|)
    virtual void setShotParam(B6BatState& s) = 0;                    // SetShotParam (0x9CCA80: geometria con matrices; no reescrita)
    virtual void rotPos(B6BatState& s) = 0;                          // RotPos (0x9CC870: giro orbital con matrices; no reescrita)
};

struct B6Bat {
    static void setApproach(B6BatState& s, float step);                                   // SetApproach (0x9CD480)
    static void approach(B6BatState& s);                                                   // Approach (0x9CD4A0)
    static void setFallParam(B6BatState& s, B6BatEnv& env);                                // SetFallParam (0x9CD570)
    static void fallDown(B6BatState& s);                                                   // FallDown (0x9CD620)
    static void chgDead(B6BatState& s);                                                    // ChgDead (0x9CD660)
    static void setPathParam(B6BatState& s, const boss6::Boss6State& o, B6BatEnv& env);    // SetPathParam (0x9CCB90)
    static void traceShotPath(B6BatState& s, B6BatEnv& env);                               // TraceShotPath (0x9CCC50)
    static void setSpiralParam(B6BatState& s, const B6BatGlobals& g, const boss6::Boss6State& o);   // SetSpiralParam (0x9CCEC0)
    static void traceSpiralPath(B6BatState& s, B6BatGlobals& g, B6BatEnv& env);            // TraceSpiralPath (0x9CCF20)
    static void rotInit(B6BatState& s, B6BatEnv& env);                                     // RotInit (0x9CC7E0)
    static void setBatSpiral(B6BatGlobals& g, const boss6::Boss6State& o, int diff, float dist);   // SetBatSpiral (0x9CCD00), parte escalar (dist = longitud del vector que calcula el motor)
    static void addBatSpiralCount(B6BatGlobals& g);                                        // AddBatSpiralCount (0x9CCEA0)
    static void chkMode(B6BatState& s, B6BatGlobals& g, boss6::Boss6State& o, B6BatEnv& env);   // ChkMode (0x9CD070)
    static void execMode(B6BatState& s, B6BatGlobals& g, const boss6::Boss6State& o, B6BatEnv& env);   // ExecMode (0x9CD3A0)
    static bool plHitChk(B6BatState& s, B6BatGlobals& g, boss6::Boss6State& o, boss6::Boss6Globals& og, B6BatEnv& env);   // PlHitChk (0x9CC680)
    static void setTargetID(int8_t& target, int8_t& mode, const boss6::Boss6Globals& og, B6BatEnv& env);   // SetTargetID (0x9CC070)
};

// ---------- Clases de efecto pequenas ----------
struct B6SkyFade {          // TBS6SkyFade (Ctrl 0x9D2240)
    int32_t cnt = 0;        // +4
    int32_t total = 0;      // +8
    int32_t mode = 0;       // +0xC (0 = aparece, otro = desaparece)
    float ratio = 0;        // +0x10
    bool deleted = false;
};
struct B6SkyFadeEnv {
    virtual ~B6SkyFadeEnv() {}
    virtual int fadeBusy() = 0;          // func_1F33B0(&D_500F20) (<= 0 = libre)
    virtual int skipFlagA() = 0;         // gp-0x7294
    virtual int skipFlagB() = 0;         // gp-0x7298
    virtual void setSkyAlpha(float a) = 0;   // *(float*)(gp-0x7440 + 0x20) = a
};
void b6SkyFadeCtrl(B6SkyFade& s, B6SkyFadeEnv& env);                // Ctrl (0x9D2240)

struct B6MaskFeather {      // TBS6MaskFeather
    int32_t cnt = 0, total = 13;   // +4, +8 (el ctor pone 0xD)
    float dy = 0, dz = 0;          // +0xC, +0x10
    bool deleted = false;
};
void b6MaskFeatherPre(B6MaskFeather& s);                            // CtrlPre (0x9D25E0)
void b6MaskFeatherPost(B6MaskFeather& s);                           // CtrlPost (0x9D26A0)

struct B6AfterImage { float alpha = 1.0f; bool deleted = false; };  // TBoss6AfterImage: alfa en +0x1C
void b6AfterImagePre(B6AfterImage& s);                              // CtrlPre (0x9D1780)

}}  // namespace vn::boss56

namespace vn { namespace boss56 {
// Tablas generadas (bosses56_data.cpp) desde asm/bs6mod/data: D_009D4720 / D_009D47A8 (17 floats cada una, por dificultad) y D_009D4830.
extern const uint32_t kBatSpeedA_bits[17];
extern const uint32_t kBatSpeedB_bits[17];
extern const int32_t kBatPathIds[17];
float b6BitsToFloat(uint32_t u);
}}

// Jefe 5 (TBoss5, overlay bs5mod, carga 0x9C4800): movimiento por rutas, orquestacion por fotograma y avisos de sonido por
// fotograma de movimiento. Reescrito a mano a partir de asm/bs5mod/bs5mod_text.s (m2c como borrador, verificado contra el asm).
// Complementa game/bosses_b/boss5.* (que no se modifica). Esta unidad tiene su propia estructura `B5MoveState` con SOLO los campos
// que tocan estas funciones (offset de TBoss5 en comentario); todo lo que el original pide al motor (rutas, matrices, sonido,
// efectos, rand, camara) pasa por `B5MoveEnv`. Sin dependencias de plataforma ni rutas absolutas.
#pragma once
#include <cstdint>
namespace vn { namespace boss56 {

// Banderas de +0x1E40 usadas aqui (nombres por uso).
enum B5Flags1E40 : uint32_t {
    B5F_TURN_TO_CAM = 0x2,       // ControlPos: girar hacia la camara
    B5F_NO_CUES = 0x400,         // ControlByMotionX: sin avisos de sonido
    B5F_SND_2000 = 0x2000,       // ControlByMotionX: efecto/sonido periodico 0x31 / 0x1C000F
    B5F_PILLARS = 0x10000,       // ControlByMotionX: sonido continuo 0x1B001C
    B5F_LIFEBAR = 0x200,         // CtrlPre: ControlDispLife
    B5F_GAUGE_UP = 0x4000,       // CtrlPre: sube el nivel 43E4 (si no, baja)
    B5F_OFFSET = 0x80000,        // ControlPos/ControlPath: suma el desplazamiento 1E14
    B5F_CLEAR_AT_PRE = 0x40,     // CtrlPre lo quita al empezar el fotograma
};
constexpr uint32_t B5F0C_ATTACKING = 0x10000000;   // +0x0C (ControlPos)
constexpr uint32_t B5F0C_FROZEN = 0x4000;          // +0x0C (UpdateMotionFrame: movimiento detenido)

// Muestra de una ruta (funcion del motor func_1CC710): posicion y rotacion en 6 palabras (+0x1DBC.. del TBoss5).
struct B5PathSample { float pos[3]; int32_t rot[3]; };

struct B5MoveState {
    uint32_t f0C = 0;                 // +0x0C
    float pos[3] = {0, 0, 0};         // objeto de posicion (+0x30 -> +0,+4,+8)
    float objF0C[3] = {0, 0, 0};      // +0x30 -> +0x0C..+0x14 (copiado por CtrlPre a 1F00..1F08)
    float objScale[3] = {0, 0, 0};    // +0x30 -> +0x18..+0x20 (copiado a 1F0C..1F14)
    int32_t rot[3] = {0, 0, 0};       // +0x30 -> +0x24,+0x28,+0x2C (angulos enteros)
    int32_t motion = 0;               // +0x34 movimiento actual
    float frame = 0;                  // +0x44 fotograma actual (leido por ControlByMotion/X; el significado exacto es inferido)
    float frame3C = 0, frame40 = 0;   // +0x3C / +0x40 (UpdateMotionFrame los avanza con 1EDC / 1EE0)
    int32_t rotOrder = 0;             // +0x1CC8 orden de aplicacion de ejes (GetWaitPos)
    int8_t kind74 = 0;                // +0x1D74 (!=0: esperando el aviso de camara 0x272)
    int8_t pathMode = 0;              // +0x1DB8 (1 = sigue ruta -> ControlPath)
    // ruta (InitPath / ControlPath)
    B5PathSample path{};              // +0x1DBC..+0x1DD0 (muestra actual sin girar)
    int32_t pathId = 0;               // +0x1DD4
    int32_t f1DD8 = 0;                // +0x1DD8 (solo se pone a 0)
    float pathLimit = 0;              // +0x1DDC (fin de ruta)
    float pathT = 0;                  // +0x1DE0 (posicion en la ruta)
    float pathSpeed = 1.2f;           // +0x1DE4
    int32_t f1DE8 = 0;                // +0x1DE8
    int8_t pathKind = 0;              // +0x1DED (0/1 = mira a la camara; 2/3 = mira hacia delante)
    int32_t pathYaw = 0;              // +0x1DF0 (ultimo giro calculado)
    float pathBlend = 1.0f;           // +0x1DF4
    float pathBlendStep = 0;          // +0x1DF8
    // ruta de espera (InitWaitPath / ControlWaitPath)
    float off[3] = {0, 0, 0};         // +0x1E14,+0x1E18,+0x1E1C (desplazamiento actual)
    float offPrev[3] = {0, 0, 0};     // +0x1E20..+0x1E28 (el del fotograma anterior)
    int32_t waitSel = 0;              // +0x1E2C (ruta de espera: 0xC0..0xC3)
    float waitT = 0;                  // +0x1E30
    float waitSpeed = 0;              // +0x1E34
    // varios
    uint32_t f1E40 = 0;               // +0x1E40
    uint32_t counter = 0;             // +0x1E4C (contador de fotogramas, u32)
    uint8_t cueBits = 0;              // +0x1E5D (bit0: sonido del movimiento 0x1DA, bit1: pilares, bit2: 0x2000)
    float rate = 1.0f, rate2 = 1.0f;  // +0x1EDC / +0x1EE0 (velocidad de reproduccion de los dos canales)
    // copia del fotograma anterior que hace CtrlPre
    float savedPos[3] = {0, 0, 0};    // +0x1EF4..+0x1EFC
    float savedF0C[3] = {0, 0, 0};    // +0x1F00..+0x1F08
    float savedScale[3] = {0, 0, 0};  // +0x1F0C..+0x1F14
    int32_t savedRot[3] = {0, 0, 0};  // +0x1F18..+0x1F20
    uint8_t kind = 0;                 // +0x1E48 (0 = battle1, 1 = battle2)
    // indicador sonoro del agua (CtrlPre)
    int32_t gaugeObj = 0;             // +0x43E0 (distinto de 0: hay objeto)
    float gaugeLevel = 0;             // +0x43E4 (0..5, paso 1/12)
};

// Llamadas al motor. Los comentarios dan la funcion original.
struct B5MoveEnv {
    virtual ~B5MoveEnv() {}
    virtual int camTrackId() = 0;                                   // *(s16*)(G + G[1]*0xD0 + 0xD0), G = *(gp-0x7338)
    virtual void camPos(float out[3]) = 0;                          // G + G[1]*0xD0 + 0x90
    virtual void pathEval(float t, int id, B5PathSample& out) = 0;  // func_1CC710(t, gp-0x7310, id, out)
    virtual int pathLen(int id) = 0;                                // func_1CCCB0(gp-0x7310, id)
    virtual void rotYaw(int ang, const float in[3], float out[3]) = 0;   // func_1B0834 + func_1B03EC(ang) + func_1AFC00(in, out)
    virtual void rotOrdered(int order, const int32_t rot[3], const float in[3], float out[3]) = 0;   // GetWaitPos: ejes en el orden de 1CC8
    virtual int atan2Ang(float dx, float dz) = 0;                   // func_1A79A0(dx, dz)
    virtual int rand() = 0;                                         // func_1A7EE0
    virtual float randFloat() = 0;                                  // func_1A7F10
    virtual void sound(int id, int id2, int stop) = 0;              // func_217C30(&D_515F30, id, id2, stop, 0x40)
    virtual void effect(float scale, int id, const float pos[3]) = 0;   // func_1E1500(scale, id, pos)
    virtual void effectBig(float scale, float scale2, int id, const float pos[3]) = 0;   // func_1E1770
    virtual void posRotY(float a, float b, float c, float out[3]) = 0;  // GetPosRotY (punto delante del jefe)
    virtual void turnToCam(const float camPos[3], int speed) = 0;   // TurnPos -> func_951200
    virtual void setGaugeLevel(int obj, float level) = 0;           // func_943E10(level, obj)
};

// Pasos por fotograma que CtrlPre_battle1/2 delegan en funciones no reescritas aqui (o en boss5.cpp).
enum B5Step {
    B5S_DISPATCH_STATE = 1,   // func_1C6C80(this, estado 1F24, tabla): despacho por estado, resultado -> 1F2A
    B5S_FALL_CHECK,           // FallCheck (boss5.cpp)
    B5S_CHECK_DAMAGE_END,     // CheckDamageEnd
    B5S_CHECK_DAMAGE,         // CheckDamage
    B5S_CONTROL_ATTACK,       // ControlAttack
    B5S_CONTROL_POS,          // ControlPos (aqui)
    B5S_CONTROL_ALPHA,        // ControlAlpha
    B5S_BY_MOTION,            // ControlByMotion (aqui)
    B5S_WAIT_PATH,            // ControlWaitPath (aqui)
    B5S_WATER_PILLAR,         // ControlWaterPillar
    B5S_BY_MOTION_X,          // ControlByMotionX (aqui)
    B5S_LIFE_DISPLAY,         // ControlDispLife
};
struct B5StepEnv {
    virtual ~B5StepEnv() {}
    virtual void step(int what) = 0;
};

struct B5Move {
    static int getCamRot(B5MoveEnv& env);                                    // GetCamRot (0x9C7160)
    static void getCamPos(B5MoveEnv& env, const float in[3], float out[3]);  // GetCamPos (0x9C71C0)
    static void updateMotionFrame(B5MoveState& b);                           // UpdateMotionFrame (0x9C5970)
    static void controlByMotion(B5MoveState& b, B5MoveEnv& env);             // ControlByMotion (0x9C59B0)
    static void controlByMotionX(B5MoveState& b, B5MoveEnv& env);            // ControlByMotionX (0x9C5A70)
    static void initPath(B5MoveState& b);                                    // InitPath (0x9C6B30)
    static void controlPath(B5MoveState& b, B5MoveEnv& env);                 // ControlPath (0x9C6B80)
    static void getWaitPos(B5MoveState& b, B5MoveEnv& env, float out[3]);    // GetWaitPos (0x9C6F90)
    static void initWaitPath(B5MoveState& b, B5MoveEnv& env);                // InitWaitPath (0x9C6E60)
    static void controlWaitPath(B5MoveState& b, B5MoveEnv& env);             // ControlWaitPath (0x9C6EE0)
    static void controlPos(B5MoveState& b, B5MoveEnv& env);                  // ControlPos (0x9C6560)
    static void ctrlPre(B5MoveState& b, B5MoveEnv& env, B5StepEnv& st);      // CtrlPre (0x9C5010)
    static void ctrlPreBattle1(B5StepEnv& st);                               // CtrlPre_battle1 (0x9C5560)
    static void ctrlPreBattle2(B5StepEnv& st);                               // CtrlPre_battle2 (0x9C5690)
};

}}  // namespace vn::boss56

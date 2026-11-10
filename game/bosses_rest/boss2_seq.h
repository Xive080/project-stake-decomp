// Boss2Task (overlay bs2mod, carga 0x9C4800): interprete de secuencias ExecSeqCmd, ExecCmd, ControlPos, AttackPosSet, SetAttackDist,
// CharInit (configuracion + despacho por forma), FinishTask0/FinishTask, actDelete/actDead. Segunda tanda (pendiente de game/bosses_a).
// Reutiliza Bs2State/Bs2Env/Boss2Engine/Boss2Logic de ../bosses_a/boss2.h sin modificarlos; lo nuevo va en Bs2Ext.
#pragma once
#include <cstdint>
#include "../bosses_a/boss2.h"
#include "boss_rest_data.h"
#include "boss3_seq.h"     // Bs3MatrixOps (pila de matrices del motor, compartida)
namespace vn { namespace boss {

struct Bs2Ext {
    int32_t  cmdRun = -1;                 // +0x2024 (ExecCmd: -1 = buscando el comando 1)
    const int16_t* cmdPc = nullptr;       // +0x2028
    const int16_t* cmdEnd = nullptr;      // guarda propia (el original no comprueba limite)
    int16_t  bunshinArgA = 0;             // +0x22E2 (ExecCmd comando 0: 1er argumento s16)
    int32_t  bunshinArgB = 0;             // +0x22D0 (ExecCmd comando 0: 2o argumento, 32 bits tomados de 2 s16 contiguos)
    float    anchor[3] = {0, 0, 0};       // +0x20CC,+0x20D0,+0x20D4 (punto de referencia de SetAttackDist)
    int16_t  scene = 0;                   // +0x5C
    int32_t  warpTrack = -1;              // +0x2108
    int32_t  warpTrackAux = 0;            // +0x210C
    int      swordFirePresent[2] = {0, 0};     // +0x22EC,+0x22F0 != NULL
    int      swordFireDetached[2] = {0, 0};    // FinishTask0 pone *(p+0x2550) = 0: se anota que se ha hecho
    int      warpEffPresent = 0;          // +0x212C != NULL
    void*    fx20E4[2] = {nullptr, nullptr};   // +0x20E4,+0x20E8 (objetos liberados en FinishTask0)
    uint32_t bodyFlags = 0;               // *(this+8)->+0x1C9C
    float    sf22F4 = 0;                  // +0x22F4
    int32_t  sf22F8 = 0;                  // +0x22F8
    const Bs2State* owner = nullptr;      // +0x22D0 como puntero (forma 3: copia st del padre)
};
struct Bs2SysGlobals {
    uint8_t cameraStartAllowed = 0;   // 0x500E17 (AllowCameraStart = 1, Forbid = 0)
    int8_t  g73CC = 0;                // gp-0x73CC
    int     g729C = 0;                // gp-0x729C
    int8_t  g72A0 = 0;                // gp-0x72A0
};
// Configuracion por forma (D_009D5D20[forma] apunta a esta estructura en el overlay; solo los campos escalares se leen del BIN en el test).
struct Bs2FormConfig {
    uint32_t orC, andC;           // +0x00,+0x04 sobre enemyFlags (+0x0C): |= orC; &= andC
    uint32_t orSt, andSt;         // +0x08,+0x0C sobre st (+0x2014)
    float    alphaOutThr, alphaInThr;   // +0x10,+0x14 -> +0x2120,+0x2124
    const Bs2Branch* branch;      // +0x18 (si no es nulo: BranchInit)
    int32_t  branchParam;         // +0x1C (-> +0x2114 si no es 0)
    const int16_t* const* seqTable;     // +0x20 (si no es nulo: SeqCmdInit)
    uint8_t  deadStage;           // +0x24 (-> +0x20B4)
    int8_t   hitReload;           // +0x25 (-> +0x20F1 y +0x20F3)
};
struct Bs2SeqHost : Bs3MatrixOps {
    virtual float camFrame() = 0;                  // *(float*)(G + G[1]*0xD0 + 0xC0), G = *(gp-0x7338)
    virtual int   camTrackId() = 0;                // *(int16_t*)(G + G[1]*0xD0 + 0xD0)
    virtual void  camPoint(float out[3]) = 0;      // G + G[1]*0xD0 + 0x90..0x98
    virtual int   camAngle() = 0;                  // G + G[1]*0xD0 + 0xA0
    virtual int   motionLength(int motion) = 0;    // tabla s16 en 0x1FB73E4
    virtual const Bs2WarpPoint& warpPoint(int step) = 0;   // entrada de teletransporte
    virtual void  startWarpIn() = 0;               // StartWarpIn (0x9C6C20, pendiente)
    virtual void  startWeakDisp() = 0;             // StartWeakDisp (pendiente)
    virtual void  initBunshin(Bs2Ext& x) = 0;      // InitBunshin (0x9C96C0, pendiente)
    virtual void  turnToward(const float target[3], int step) = 0;   // TurnPos -> func_951200(pos, target, step)
    virtual void  getAngleYX(float dx, float dy, float dz, int& pitch, int& yaw) = 0;   // GetAngleYX (0x1C8BD0)
    virtual void  scaleToLength(const float in[3], float len, float out[3]) = 0;        // func_1AFA7C (v * len/|v|)
    virtual void  releaseTrack(int track, int aux) = 0;   // func_1E5E50 / func_1E6240 en FinishTask0
    virtual void  releaseTrail() = 0;              // func_1CE4B0(this+0x2140)
    virtual void  freeObject(void* p) = 0;         // func_1A1F00
    virtual void  deleteWarpEff() = 0;             // ~CBoss2WarpEff(delete)
    virtual void  taskEnd() = 0;                   // FinishTask -> func_950F80
    // CharInit
    virtual void  charInitClear() = 0;             // func_939280
    virtual void  setWarpPos() = 0;                // set_warp_pos
    virtual void  newSwordFire(int idx, int hand) = 0;   // new_SwordFire(this, -1)
    virtual void  initSwordFire(int idx, int a, int b, int c) = 0;   // campos +0x255C,+0x2560,+0x2564 de la espada
    virtual void  setTakion(int idx) = 0;          // SetTakion
    virtual void  finalizeSwordFire(int idx) = 0;  // +0x2570 = 3
    virtual int   scaledLife() = 0;                // func_14C518(5000 * D_985830[func_96CED0()])
    virtual void  form2Setup(Bs2Ext& x) = 0;       // func_939C80(&info) + func_9499C0(this+0x1D70,1,1,1) + new CBoss2WarpEff
};

struct Boss2SeqLogic {
    enum { RUN_OK = 0, RUN_HUNG = 1 };   // HUNG: operacion >= 30 (el original lee la tabla de longitudes fuera de rango)
    static void allowCameraStart(Bs2SysGlobals& sg);                       // AllowCameraStart 0x9CA240
    static void forbidCameraStart(Bs2SysGlobals& sg);                      // ForbidCameraStart 0x9CA250
    static void execCmd(Bs2Ext& x, Bs2SeqHost& h);                         // ExecCmd 0x9C7890
    static int  execSeqCmd(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SysGlobals& sg, Boss2Engine& g, Bs2SeqHost& h);   // ExecSeqCmd 0x9C98C0
    static int  actExecSeqCmd(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SysGlobals& sg, Boss2Engine& g, Bs2SeqHost& h);   // actExecSeqCmd 0x9CA090
    static void controlPos(Bs2State& s, Bs2SeqHost& h);                    // ControlPos 0x9C7550
    static void attackPosSet(Bs2State& s, Bs2SeqHost& h, float out[3]);    // AttackPosSet 0x9CB3E0
    static void setAttackDist(Bs2State& s, const Bs2Ext& x, Bs2SeqHost& h, float dist);   // SetAttackDist 0x9CADC0
    static void finishTask0(Bs2State& s, Bs2Ext& x, Bs2Env& e, Boss2Engine& g, Bs2SeqHost& h);   // FinishTask0 0x9CA0B0
    static void finishTask(Bs2SeqHost& h);                                 // FinishTask 0x9CA220
    static int  actDelete(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SeqHost& h);   // actDelete 0x9C9410
    static int  actDead(Bs2State& s, Bs2Ext& x, Bs2SysGlobals& sg, Bs2SeqHost& h);   // actDead 0x9C97A0
    static int  actTest();                                                 // actTest 0x9C98B0 (devuelve 0)
    // CharInit__9Boss2TaskFv (0x9C7930): aplica la configuracion de la forma y crea sus objetos.
    static void charInit(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SysGlobals& sg, Boss2Engine& g, Bs2SeqHost& h, const Bs2FormConfig* cfg);
};

}} // namespace

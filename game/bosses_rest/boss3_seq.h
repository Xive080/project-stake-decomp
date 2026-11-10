// Boss3Task (overlay bs3mod, carga 0x9C4800): interprete de secuencias ExecSeqCmd, ExecCmd y auxiliares de tarea.
// Segunda tanda (pendiente de game/bosses_a). Cada funcion cita la original y su direccion; campos nombrados por uso con offset.
// Reutiliza Bs3State/Bs3Env/Boss3Engine/Boss3Logic de ../bosses_a/boss3.h sin modificarlos; lo nuevo va en Bs3Ext.
#pragma once
#include <cstdint>
#include "../bosses_a/boss3.h"
#include "boss_rest_data.h"
namespace vn { namespace boss {

// Campos de Boss3Task que usan estas funciones y que Bs3State no tiene.
struct Bs3Ext {
    int32_t  cmdRun = -1;                 // +0x1DB0 (ExecCmd: -1 = buscando el comando 1)
    const int16_t* cmdPc = nullptr;       // +0x1DB4
    const int16_t* cmdEnd = nullptr;      // guarda propia (el original no comprueba limite): si cmdPc llega aqui, ExecCmd sale
    int32_t  pathId = -1;                 // +0x1E34 (id de ruta de camara; -1 = ninguna)
    float    pathFrame = 0;               // +0x1E38
    int32_t  pathEnd = 0;                 // +0x1E3C (ultimo fotograma de la ruta)
    int8_t   counter = 0;                 // +0x1DC4 (ops 37/38)
    int32_t  lastTurn = 0;                // +0x1EFC (op 15)
    uint8_t  b1E10 = 0, b1E11 = 0;        // +0x1E10 (op 42), +0x1E11 (paso de actDelete)
    int32_t  eulerOrder = 0;              // +0x1CC8 (0..5: orden de las rotaciones X,Y,Z en ops 18/19)
    float    scale = 1;                   // +0x1CD0
    float    off[3] = {0, 0, 0};          // +0x70,+0x74,+0x78 (desplazamiento local)
    uint32_t bodyFlags = 0;               // *(this+8)->+0x1C9C (ops 18..21 los tocan: bits 0x13 / 0x12)
    int32_t  f1F00 = 0;                   // +0x1F00 (op 56)
    uint32_t f1F04 = 0;                   // +0x1F04 (bit 0 = sonido en bucle activo)
    float    fireTarget = 0;              // +0x1EDC
    float    fireCur = 0;                 // +0x1EE0
    float    fireOut = 0;                 // +0x1EE4
    int      partPresent[4] = {0, 0, 0, 0};   // +0x1D58[i] != NULL
    uint32_t partFlags[4] = {0, 0, 0, 0};     // +0x1A8 de cada parte (bit 0x40 = colision apagada)
    void*    fx1EA0[2] = {nullptr, nullptr};  // +0x1EA0,+0x1EA4 (objetos liberados en FinishTask0)
    uint8_t  b1EB6 = 0;                   // +0x1EB6 (SetHeight)
};
// Estructuras de sistema que tocan las operaciones (globales de bss y fijas del ELF).
struct Bs3SysGlobals {
    uint8_t cameraStartAllowed = 0;   // 0x500F07 (AllowCameraStart = 1, Forbid = 0; op 7 la pone a 1, op 8 espera a que sea 0)
    uint8_t g500E12 = 0;              // 0x500E12 (op 26 con ruta -1)
    int8_t  g73CC = 0;                // gp-0x73CC (op 52 la copia a seqWait)
};
// Pila de matrices del motor (func_1B0834 push, func_1B0808 translate, func_1B027C/1B03EC/1B055C rotX/Y/Z, func_1B06CC scale, func_1AFC00 transform).
struct Bs3MatrixOps {
    virtual ~Bs3MatrixOps() {}
    virtual void push() = 0;
    virtual void pop() = 0;
    virtual void translate(float x, float y, float z) = 0;
    virtual void rotX(int ang) = 0;
    virtual void rotY(int ang) = 0;
    virtual void rotZ(int ang) = 0;
    virtual void scale(float x, float y, float z) = 0;
    virtual void transformPoint(const float in[3], float out[3]) = 0;
};
// Llamadas del interprete al resto del motor (no reescritas).
struct Bs3SeqHost : Bs3MatrixOps {
    virtual float camFrame() = 0;                  // *(float*)(G + G[1]*0xD0 + 0xC0), G = *(gp-0x7338)
    virtual int   camTrackId() = 0;                // *(int16_t*)(G + G[1]*0xD0 + 0xD0)
    virtual int   pathEndFrame(int pathId) = 0;    // func_1CCCB0(gp-0x7310, pathId)
    virtual void  spawnXWing() = 0;                // op 56: func_1A4100(0x1E40,...) + __ct__Boss3XWingTask
    virtual void  blurInit() = 0;                  // initBlur
    virtual void  blurEnd() = 0;                   // endBlur
    virtual void  releaseTrail() = 0;              // func_1CE4B0(this+0x1F40) (FinishTask0)
    virtual void  freeObject(void* p) = 0;         // func_1A1F00
    virtual void  loopSoundOffEvent() = 0;         // func_217C30(&D_515F30, 0x17000C, 0x17000C, 1) en LoopSoundOff
    virtual float groundHeight(float x, float y, float z) = 0;   // func_951300
    virtual void  taskEnd() = 0;                   // FinishTask: func_1A2A10() + func_950F80(this)
};

struct Boss3SeqLogic {
    enum { RUN_OK = 0, RUN_HUNG = 1 };   // HUNG: op sin implementar (6, 49..51, 53, 55 o >= 59): el original no avanza pc y vuelve a leer el mismo op = bucle infinito
    static void allowCameraStart(Bs3SysGlobals& sg);                  // AllowCameraStart 0x9C9310
    static void forbidCameraStart(Bs3SysGlobals& sg);                 // ForbidCameraStart 0x9C9320
    static void execCmd(Bs3Ext& x);                                   // ExecCmd 0x9C6B50
    static int  execSeqCmd(Bs3State& s, Bs3Ext& x, Bs3Env& e, Bs3SysGlobals& sg, Boss3Engine& g, Bs3SeqHost& h);   // ExecSeqCmd 0x9C8050
    static int  getPathEndFrame(Bs3Ext& x, Bs3SeqHost& h);            // GetPathEndFrame 0x9C9800
    static void fireAlphaDirectSet(Bs3Ext& x, float v);               // FireAlphaDirectSet 0x9C9EB0
    static void fireAlphaSupSet(Bs3Ext& x, float v);                  // FireAlphaSupSet 0x9C9EC0
    static void fireAlphaControl(const Bs3State& s, Bs3Ext& x);       // FireAlphaControl 0x9C9CF0
    static void enableCollision(Bs3State& s, Bs3Ext& x);              // EnableCollision 0x9C9330
    static void disableCollision(Bs3State& s, Bs3Ext& x);             // DisableCollision 0x9C9390
    static void loopSoundOff(Bs3Ext& x, Bs3SeqHost& h);               // LoopSoundOff 0x9CA020
    static void finishTask0(Bs3State& s, Bs3Ext& x, Bs3SeqHost& h, Boss3Engine& g);   // FinishTask0 0x9C9220
    static void finishTask(Bs3SeqHost& h);                            // FinishTask 0x9C92E0
    static void setHeight(Bs3State& s, Bs3Ext& x, Bs3SeqHost& h);     // SetHeight 0x9C93F0
    static int  actExecSeqCmd(Bs3State& s, Bs3Ext& x, Bs3Env& e, Bs3SysGlobals& sg, Boss3Engine& g, Bs3SeqHost& h);  // actExecSeqCmd 0x9C91F0
    static int  actNop();                                             // actNop 0x9C7FA0 (devuelve 1)
    static int  actDead();                                            // actDead 0x9C9210 (devuelve 0)
    static int  actDelete(Bs3State& s, Bs3Ext& x, Bs3Env& e, Bs3SeqHost& h);   // actDelete 0x9C7FB0
};

}} // namespace

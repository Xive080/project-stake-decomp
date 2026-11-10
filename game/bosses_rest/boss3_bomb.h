// TBoss3Bomb (overlay bs3mod): solo las funciones pequenas de movimiento cuya logica se pudo leer completa del asm.
// El resto de la clase (CtrlPre_Target2D/Target3D/Ground/Linear/Bound0/ModeChg, CtrlPre, CtrlPut, Laser, XWing, Subchr) sigue pendiente: ver MANIFEST.md.
#pragma once
#include <cstdint>
namespace vn { namespace boss {

struct Bs3BombState {
    float pos[3] = {0, 0, 0};        // +0x2C,+0x30,+0x34 posicion mundo
    float base[3] = {0, 0, 0};       // +0x140,+0x144,+0x148 origen del sistema local (SetTargetWorld lo calcula en el original: aqui se inyecta)
    float local[3] = {0, 0, 0};      // +0x164,+0x168,+0x16C posicion local
    float vel[3] = {0, 0, 0};        // +0x170,+0x174,+0x178 velocidad
    float target[3] = {0, 0, 0};     // +0x17C,+0x180,+0x184 punto objetivo local
    float acc[3] = {0, 0, 0};        // +0x218,+0x21C,+0x220 aceleracion
    int8_t kindA = 0, kindB = 0;     // +0x150, +0x151 (nombres por uso: SetTargetLocal los lee; kindB == 1 o kindA == -1 = rama simetrica)
};
struct Bs3BombHost {
    virtual ~Bs3BombHost() {}
    virtual float rand01() = 0;                         // func_1A7F10: float en [0,1)
    virtual void setTargetLocalFloat(Bs3BombState&) = 0; // SetTargetLocalFloat (0x9CBEB0, usa la pila de matrices: no reescrita)
    virtual void setTargetWorld(Bs3BombState&) = 0;      // SetTargetWorld (0x9CC070: T(camara)*RotY, transforma +0x140 en +0x17C)
    virtual void ctrlPreTarget2D(Bs3BombState&) = 0;     // CtrlPre_Target2D (no reescrita)
    virtual void ctrlPreBound0(Bs3BombState&) = 0;       // CtrlPre_Bound0 (no reescrita)
};
struct Boss3BombLogic {
    static void resetAcc(Bs3BombState& b);                       // ResetAcc 0x9CCBC0
    static void adjustLocalPos(Bs3BombState& b);                 // AdjustLocalPos 0x9CC110
    static void setTargetLocal(Bs3BombState& b, Bs3BombHost& h); // SetTargetLocal 0x9CBF80
    static void ctrlPreFloat(Bs3BombState& b, Bs3BombHost& h);   // CtrlPre_Float 0x9CCAE0
    static void ctrlPreFall(Bs3BombState& b, Bs3BombHost& h);    // CtrlPre_Fall 0x9CC970
    static void ctrlPreBound(Bs3BombState& b, Bs3BombHost& h);   // CtrlPre_Bound 0x9CC990
    static void ctrlPreFly(Bs3BombState& b, Bs3BombHost& h);     // CtrlPre_Fly 0x9CCB80
    static void ctrlPreAppear(Bs3BombState&);                    // CtrlPre_Appear 0x9CCAD0 (vacia)
    static void ctrlPreExplosion(Bs3BombState&);                 // CtrlPre_Explosion 0x9CCBB0 (vacia)
};

}} // namespace

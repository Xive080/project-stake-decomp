// Selector de modo principal (MainModeMgrClass, ELF main 0x1F2FF0..0x1F3C70), reescrito a mano.
// El objeto original tiene: +4 modo actual (s16), +6 modo pedido (s16, -1 = ninguno), +8 cuenta atras de pantalla apagada (s32).
// Las llamadas a motor (render env, sonido, Urawaza, descarga de overlays) salen como valores/eventos en `ModeEvents`.
#pragma once
#include <cstdint>
namespace vn {

// Tabla ELF 0x2C2130 (bytes con signo, via puntero en 0x26E18C): modo siguiente por defecto de cada modo (0..23).
extern const int8_t kNextMode[24];
// Tabla ELF 0x2C0E90 (4 B por entrada, indexada por el estado de juego 0x500F14; se lee el s16 en +2): 1 = en ese estado
// una pulsacion de start/decidir salta la pantalla.
extern const int16_t kSkipOnPress[24];

// Entradas que synch lee del sistema.
struct ModeInput {
    bool pressStart[2];      // is_PressStart(0/1)
    bool pressDecide[2];     // is_PressDecide(0/1)
    int  gameState;          // s16 en 0x500F14
    int  gp73A4;             // gp-0x73A4 (peticion externa de salto a modo 4; se limpia al consumirla)
};

// Efectos de motor que synch pide en este fotograma.
struct ModeEvents {
    bool clearAbortFlag = false;   // 0x5004D8 = 0
    bool screenBitOff = false;     // nlGetDefaultRendEnv: bit1 de +0x10 y +0x38 a 0
    bool screenBitOn = false;      // idem a 1 (al acabar la cuenta atras)
    bool playPressSound = false;   // seqRequest(0x400001, 0, -10)
    bool urawazaCheck = false;     // UrawazaCheck() (modo pedido == 0xA)
    bool urawazaInit = false;      // UrawazaInit() (cualquier otro modo pedido)
    bool modeChanged = false;      // se llamo a la funcion virtual +0x14 (cambio de modo; el planificador hace chgInit)
};

// Que hace chgInit (resumen de las llamadas de motor, en orden). El orden y condiciones salen del asm.
struct ChgInitPlan {
    bool setFlag503584Around = true;  // 0x503584 = 1; EntryDat.flushCur(); 0x503584 = 0
    bool ovlChangeMainModule = true;  // OvlChangeMainModule(modo, gp-0x73B4)
    bool fullReset = false;           // si modo != 0: fuente 8x8, EntryDat.init, FileMgr.waitIdleAll, Sofdec.destroyHandleAll, MallocInit,
                                      //   gp-0x73D8 = 0, gp-0x73F0 = 0, [gameState == 9 -> func_A10560(0x502D10)], FileMgr.closeAll(-1,-1), SprMgr.init, PolyMgr.init
    bool callA10560 = false;          // dentro de fullReset, solo si gameState == 9
    bool powerOnInitComm = true;      // PowerOnInitComm() siempre al final
};

struct MainModeMgr {
    int16_t mode = 0;        // +4
    int16_t reqMode = -1;    // +6
    int32_t blank = 0;       // +8 (cuenta atras: >=2 baja, ==1 baja a 0 y reactiva el render)
    // init (0x1F3C70): +8 = 0, +6 = modo inicial, luego virtual +0x10.
    void init(int16_t firstMode);
    // next (0x1F2FF0): si no hay modo pedido, toma el siguiente de kNextMode[modo].
    void next();
    // synch (0x1F3020). Devuelve los efectos pedidos.
    ModeEvents synch(ModeInput& in);          // limpia in.gp73A4 al consumirlo, como el original
    // synch2 (0x1F32F0): aplica el modo pedido sin comprobaciones (modo = pedido; pedido = -1). Devuelve el modo aplicado.
    int16_t synch2();
    // chgInit (0x1F3210): plan de llamadas de motor para el modo actual.
    ChgInitPlan chgInit(int gameState) const;
};
}

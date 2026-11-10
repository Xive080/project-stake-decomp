// Logica de titulo/demo/ranking/tutorial (overlay acmod, 0xA0F880). Solo reglas: menu principal por posicion de mira, lista de demos, deteccion de pistola.
#pragma once
#include "stage_common.h"
namespace vn {

// Fila de la tabla de demos D_00A1AE20 de ACMOD.BIN (0x14 B): +0 demo, +2 etapa, +4 valor gp-0x73D4, +6 valor DemoMgr+0, +8 ptid inicial; +2 == -1 termina la lista.
struct DemoRow { int16_t demoNo, stage, gp73D4, mgr0, startPtid; };
extern const DemoRow kDemoTable[2];

struct DemoMgr {
    int v0 = 0;       // +0
    int index = 0;    // +4 (fila actual)
    int started = 0;  // +8
};
struct DemoGlobals { int gameState = 0; int stageGp73D8 = 0; int valGp73D4 = 0; int gp73E4 = 0; };

struct TitleLogic {
    // GetMainMenuNo (0xA0xxxx): menu de 6 casillas segun la mira (x, y, en coordenadas centradas de la pistola). -1 = ninguna.
    //  fila superior (y >= 0x41): x < -0x6E -> 0, x >= 0x6F -> 2, -0x63 <= x < 0x64 -> 1; fila inferior (-0x80 <= y < 0x10): 3, 5, 4.
    static int getMainMenuNo(int x, int y);
    // DemoMgrClass::setStageNo: solo gameState 9 (atraccion). Si la fila actual esta marcada (-1) vuelve a 0, carga etapa/valores y deja index = 0.
    static void setStageNo(DemoMgr& m, DemoGlobals& g);
    static int  getStartPtid(const DemoMgr& m) { return kDemoTable[m.index].startPtid; }
    static int  getDemoNo(const DemoMgr& m) { return kDemoTable[m.index].demoNo; }
    // Title2Task::decideSound: 0x2000C para jugador 0, 0x2000D para el otro.
    static int  decideSound(int pl) { return pl == 0 ? 0x2000C : 0x2000D; }
    // checkConnectGuncon: alguno de los 2 puertos con pistola (estado 2 o 3 de func_1EFC90).
    static bool checkConnectGuncon(const int state[2]) { for (int i = 0; i < 2; i++) if (state[i] == 3 || state[i] == 2) return true; return false; }
    // RankSampleTask::getSightPos: posicion en pantalla de la mira (640x448): x = sx + 0x140, y = 0xE0 - sy. mode (func_1EFC90): 1 siempre, 2/3 solo con gatillo activo.
    static bool getSightPos(int mode, int sightX, int sightY, bool active, int& outX, int& outY);
    // TutdispBulletMoveOn: contador circular de cartuchos de pantalla (gp-0x6F0C): baja 1 y salta a 8 al pasar de 0.
    static int  bulletRingStep(int cur) { cur--; return cur < 0 ? 8 : cur; }
    // DemoCtrlTask::loop (0x502D10 baja 1; si < 4 pone 0x502D18 = 0).
    static void demoCtrlLoop(int& timer502D10, int& flag502D18) { timer502D10--; if (timer502D10 < 4) flag502D18 = 0; }
};
}

#include "title.h"
namespace vn {
const DemoRow kDemoTable[2] = {{2, 1, 20, 3584, 11}, {-1, -1, 0, 0, 0}};   // D_00A1AE20 (ACMOD.BIN)

int TitleLogic::getMainMenuNo(int x, int y) {
    int16_t sx = (int16_t)x, sy = (int16_t)y;
    if (sy >= -0x80) {
        if (sy >= 0x41) {
            if (sx < -0x6E) return 0;
            if (sx >= 0x6F) return 2;
            if (sx >= -0x63 && sx < 0x64) return 1;
            return -1;
        }
        if (sy < 0x10) {
            if (sx < -0x6E) return 3;
            if (sx >= 0x6F) return 5;
            if (sx >= -0x63 && sx < 0x64) return 4;
            return -1;
        }
    }
    return -1;
}

void TitleLogic::setStageNo(DemoMgr& m, DemoGlobals& g) {
    if (g.gameState != 9) return;
    if (kDemoTable[m.index].stage == -1) m.index = 0;
    const DemoRow& r = kDemoTable[m.index];
    g.stageGp73D8 = (int8_t)r.stage; g.valGp73D4 = (int8_t)r.gp73D4;
    m.v0 = r.mgr0; m.started = 1; g.gp73E4 = 0; m.index = 0;
}

bool TitleLogic::getSightPos(int mode, int sx, int sy, bool active, int& ox, int& oy) {
    ox = 0; oy = 0;
    if (mode == 1 || ((mode == 2 || mode == 3) && active)) { ox = sx + 0x140; oy = 0xE0 - sy; return true; }
    return false;
}
}

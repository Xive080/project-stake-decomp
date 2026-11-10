#include "mainmode.h"
namespace vn {
// ELF 0x2C2130
const int8_t kNextMode[24] = {17, 0, 0, 0, 15, 16, 0, 6, 6, 15, 18, 18, 6, 18, 4, 14, 4, 19, 4, 20, 21, 4, 0, 18};
// ELF 0x2C0E92 + 4*estado (s16)
const int16_t kSkipOnPress[24] = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0};

void MainModeMgr::init(int16_t firstMode) { blank = 0; reqMode = firstMode; }   // init__16MainModeMgrClassFi (la virtual +0x10 la hace el motor)

void MainModeMgr::next() {                                                       // next__16MainModeMgrClassFv
    if (reqMode == -1) reqMode = kNextMode[mode];
}

ModeEvents MainModeMgr::synch(ModeInput& in) {                             // synch__16MainModeMgrClassFv
    ModeEvents ev;
    if (in.pressStart[0] || in.pressStart[1] || in.pressDecide[0] || in.pressDecide[1]) {
        ev.clearAbortFlag = true;
        if (in.gameState >= 0 && in.gameState < 24 && kSkipOnPress[in.gameState] != 0) {
            ev.screenBitOff = true; blank = 4; ev.playPressSound = true; reqMode = 4;
        }
    }
    if (in.gp73A4 != 0) { ev.screenBitOff = true; blank = 4; in.gp73A4 = 0; reqMode = 4; }
    if (reqMode >= 0) {
        if (reqMode == 0xA) ev.urawazaCheck = true; else ev.urawazaInit = true;
        mode = reqMode; ev.modeChanged = true;
    }
    reqMode = -1;
    if (blank >= 2) blank--;
    else if (blank == 1) { blank = 0; ev.screenBitOn = true; }
    return ev;
}

int16_t MainModeMgr::synch2() {                                                  // synch2__16MainModeMgrClassFv
    mode = reqMode; reqMode = -1; return mode;
}

ChgInitPlan MainModeMgr::chgInit(int gameState) const {                          // chgInit__16MainModeMgrClassFv
    ChgInitPlan p; p.fullReset = (mode != 0); p.callA10560 = p.fullReset && gameState == 9; return p;
}
}

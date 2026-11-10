#include "screens.h"
namespace vn {

void PlayerScreens::changePlayerMode(int mode, bool exec) {          // changePlayerMode__10PlayerTaskFii
    pl.mode = mode;
    if (exec) eng.changeExec(PlExec::ModeDispatch, pl.playerNo);
}

bool PlayerScreens::checkCredSt(int mode) {                          // checkCredSt__10PlayerTaskFi
    bool ret = false;
    if (eng.creditEnough(pl.playerNo, pl.mode == 4)) {
        ret = true;
        if (sys.gameState500F14 != 0xA) { sys.nextState500F16 = 0xA; sys.gp72D0 |= 1; eng.maskScreen(); }
        uint32_t f = sys.gp72D0;
        if (f & 1) changePlayerMode(mode, true);
        else if (f & 0x10) changePlayerMode(mode, true);
        else if ((f & 8) && mode == 1) changePlayerMode(mode, true);
        else { pl.reqMode94 = mode; changePlayerMode(0xA, true); }
    }
    if (ret && sys.gameState500F14 != 6) eng.soundSeq(0x1000E);
    return ret;
}

bool PlayerScreens::checkCredAndStBtn(int mode) {                    // checkCredAndStBtn__10PlayerTaskFi
    bool ret = false;
    bool startPressed = eng.pressStart(pl.playerNo) && sys.gameState500F14 == 0xA && (sys.gp72D0 & 2);
    if (startPressed || pl.flag18 != 0) ret = checkCredSt(mode);
    pl.flag18 = 0;
    return ret;
}

void PlayerScreens::contInit() {                                     // contInit__10PlayerTaskFv
    CreditSlot a = eng.creditData(0), b = eng.creditData(1);
    pl.contCount = 0x9FFF;
    pl.credSnap0 = a.value; pl.credSnap1 = b.value;
    if (sys.gameState500F14 == 0xA && sys.gp73B4 == 2) { changePlayerMode(1, true); return; }
    eng.changeExec(PlExec::ContLoop, pl.playerNo);
}

void PlayerScreens::contLoop() {                                     // contLoop__10PlayerTaskFv
    bool keepCountdown = true;   // var_s1: sigue la pantalla de continuar
    bool expired = false;        // var_s2
    bool notDone = true;         // var_s0
    eng.drawDamageEffect(pl.playerNo);
    CreditSlot c0 = eng.creditData(0), c1 = eng.creditData(1);
    if (pl.credSnap0 != c0.value || pl.credSnap1 != c1.value) { pl.credSnap0 = c0.value; pl.credSnap1 = c1.value; pl.contCount = 0x9FFF; }
    if (sys.gp73F0 != 4 && eng.pauseLevel() <= 0 && (sys.gp72D0 & 2)) {
        if (eng.pullTrigger(pl.playerNo) || eng.pressReload(pl.playerNo)) {
            if (pl.contCount < 0x8000) { pl.contCount &= ~0xFFF; pl.contCount += 1; }   // acelera: salta al principio del digito actual + 1
        }
        pl.contCount -= 0x1E;
        if (pl.contCount <= 0 || (c0.state != 2 && c1.state != 2 && !eng.isFreePlay())) expired = true;
    }
    eng.scoreDispCheck(pl.playerNo);
    if (checkCredAndStBtn(1)) { notDone = false; keepCountdown = false; }
    else if (expired) {
        pl.contCount = 0; sys.word500DFC = (int16_t)(sys.word500DFC - 1);
        changePlayerMode(6, true); notDone = false; keepCountdown = false;
    }
    if (sys.gp73F0 != 4 && (sys.gp72D0 & 2) && notDone) pl.flags27FC |= 2;
    pl.contActive = 0;
    if (keepCountdown && (sys.gp72D0 & 2)) { pl.contActive = 1; pl.flags27FC |= 0x10; }
}

void PlayerScreens::gmoverInit() {                                   // gmoverInit__10PlayerTaskFv
    if (sys.gp73B4 == 1) eng.clearPlayerBytes(pl.playerNo);
    if (sys.gameState500F14 != 0xB) pl.flags27FC |= 4;
    if (sys.word500E00 == 1) { pl.pos[0] = pl.pos[1] = pl.pos[2] = 0; }
    else { int p = pl.playerNo & 1; for (int i = 0; i < 3; i++) pl.pos[i] = gmoverInitPos[p][i]; }
    pl.vel[0] = pl.vel[1] = pl.vel[2] = 0;
    pl.gmoverCount = 100;
    eng.changeExec(PlExec::GmoverLoop, pl.playerNo);
}

void PlayerScreens::gmoverLoop() {                                   // gmoverLoop__10PlayerTaskFv
    switch (sys.gameState500F14) {
        case 10:
            if (--pl.gmoverCount == 0) { eng.setBookPlGameEnd(pl.playerNo); changePlayerMode(9, true); }
            else pl.flags27FC |= 4;
            break;
        case 11: break;
        default: changePlayerMode(9, true); break;
    }
}

void PlayerScreens::nameentInit() {                                  // nameentInit__10PlayerTaskFv
    for (int& f : pl.nameFields) f = 0;
    pl.unk1CC0 = 0;
    eng.changeExec(PlExec::NameentLoop, pl.playerNo);
}

void PlayerScreens::nameentLoop(const NameCursor& cur) {             // nameentLoop__10PlayerTaskFv
    if (sys.gp6F90[pl.playerNo & 1] != 0) return;
    if (cur.active != 0) eng.drawNameCursor(pl.playerNo, 320.0f + (float)cur.x, 224.0f - (float)cur.y);
    pl.unk4C = 8;
    eng.gunExe(pl.playerNo);
    eng.bullet(pl.playerNo);
}

int calcClearRanking(int32_t score, const int32_t table[8]) {        // CalcClearRanking (0x2266D0)
    int i = 7;
    while (i > 0 && score < table[i]) i--;
    return i;
}

void RankingScreen::loop() {                                         // ClearRankingLoop (0x226EF0)
    eng.taskLoop();
    if (eng.pauseLevel() < 5) {
        sys.gp73E4 += 1;
        if (sys.gp73E4 >= kFrames) { eng.soundSeq(0x400001); eng.maskScreen(); sys.nextState500F16 = 0xB; }
    }
}

void BookScreen::bookExit() { eng.soundSeq(0x70013); sys.gp7118 = 1; }   // BookExit

void BookScreen::loop() {                                            // IraiList_Loop
    switch (sys.gp7114) {
        case 0:
            eng.bookCover(0);
            if (sys.gp73E4 >= 0x65u || eng.pressDecide(0) || eng.pressDecide(1)) sys.gp7114 += 1;
            break;
        case 1:
            eng.bookCover(1);
            if (eng.pauseLevel() < 5 && eng.bookAnimDone()) sys.gp7114 = 2;
            break;
        case 2: eng.bookPutPages(); break;
        case 3: if (eng.pauseLevel() < 5 && eng.bookAnimBackDone()) sys.gp7114 = 0; break;
    }
    eng.bookPut();
    if (sys.gp7118 != 0) sys.gp7114 = 3;
    if (eng.pauseLevel() < 5) sys.gp73E4 += 1;
    eng.pressDecide(0);
}
}

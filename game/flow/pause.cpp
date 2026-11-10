#include "pause.h"
namespace vn {

void PauseMgr::init() {                                         // init__13PauseMgrClassFi
    stackCount = 0; queueCount = 0; waitCount = 0; loadPause = 0; loadPausePrev = 0; state = 0;
    // +0xC/+0xE tambien se ponen a 0 (campos sin uso conocido)
}

void PauseMgr::deleteWaitCauseElem(int kind) {                  // deleteWaitCauseElem__13PauseMgrClassFi
    int keep = 0;
    for (int i = 0; i < waitCount; i++) {
        if (waits[i].kind != kind) {
            if (keep != i) waits[keep] = waits[i];
            keep++;
        }
    }
    waitCount = keep;
}

int PauseMgr::chkLoadWaitOvlMod(PauseHooks& h) {                // chkLoadWaitOvlMod__13PauseMgrClassFv
    for (int i = 0; i < waitCount; i++)
        if (waits[i].kind == 5) h.onAssert("chkLoadWaitOvlMod: entrada de tipo 5");   // `break 0`
    deleteWaitCauseElem(5);
    return 0;
}

void PauseMgr::exec(PauseHooks& h) {                            // exec__13PauseMgrClassFv
    if (loadPause != 0) {
        int done = 1;                                           // s3
        if (chkLoadWaitOvlMod(h) != 0) {
            done = 0;
        } else {
            int again;                                          // s2
            do {
                again = 0;
                if (waitCount == 0) {
                    done = 1;
                } else {
                    for (int i = 0; i < waitCount; i++) {
                        if (waits[i].kind == 0) {
                            if (h.dataRemaining()) { done = 0; continue; }
                            deleteWaitCauseElem(0);
                            again = 1;
                            break;
                        } else {
                            h.onBadWaitEntry(i, waits[i].kind);   // printf + break 0; el original sigue con la siguiente entrada
                        }
                    }
                }
                gp7168 = 0;
            } while (again);
        }
        if (done) loadPause = 0;
    } else {
        for (int i = 0; i < waitCount; i++)
            if (waits[i].kind == 5 || waits[i].kind == 0) loadPause = 1;
    }
}

void PauseMgr::synch(PauseHooks& h) {                           // synch__13PauseMgrClassFv
    loadPausePrev = loadPause;
    exec(h);
    if (queueCount != 0) {
        wanted = queue[queueCount - 1];
        if (loadPausePrev != loadPause) { state = wanted; h.onStateChange(state); }
        else if (state != wanted) { state = wanted; h.onStateChange(state); }
    }
    queueCount = 0;
}

void PauseMgr::pushStat() {                                     // pushStat__13PauseMgrClassFv
    int8_t v = (queueCount != 0) ? queue[queueCount - 1] : (int8_t)state;
    stack[stackCount++] = v;
}

void PauseMgr::popStat() {                                      // popStat__13PauseMgrClassFv
    stackCount--;
    queue[queueCount++] = stack[stackCount];
}

bool PauseMgr::reqPauseUntilAllDataEntry(PauseHooks& h) {       // reqPauseUntilAllDataEntry__13PauseMgrClassFv
    if (waitCount >= kPauseWaitMax) { h.onAssert("reqPauseUntilAllDataEntry: lista llena"); return false; }
    waits[waitCount].kind = 0;                                  // (arg no se escribe)
    waitCount++;
    return true;
}

bool PauseMgr::reqPauseUntilOvlModEntry(PauseHooks& h, int mod) {   // reqPauseUntilOvlModEntry__13PauseMgrClassFi
    if (waitCount >= kPauseWaitMax) { h.onAssert("reqPauseUntilOvlModEntry: lista llena"); return false; }
    waits[waitCount].kind = 0; waits[waitCount].arg = mod;
    waitCount++;
    return true;
}
}

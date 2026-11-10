#include "camjob.h"
namespace vn {

void CamJobQueue::flushCamJob() {                              // flushCamJob__22CameraMgrInternalClassFv
    head = tail;
    for (int i = 0; i < kSlots; i++) {
        slot[i].job = CJ_NONE; slot[i].handler = CH_NOP;
        for (int k = 0; k < 4; k++) slot[i].sub[k] = -2;
        cam[i].flagD5 = 1;
    }
    for (int i = 0; i < kRing; i++) ringState[i] = 0;
}

uint32_t CamJobQueue::entryEvent(uint32_t rec, int skip) {     // entryEvent__22CameraMgrInternalClassFPUis
    if (skip == 0) {
        ringRec[tail] = rec; ringState[tail] = 1;
        tail = (int16_t)((tail + 1) & 7);
    }
    return rec + 0xC + 4 * ((int32_t)mem->rd(rec + 8) >> 4);
}

uint32_t CamJobQueue::chgToDirMode(uint32_t pc) {              // chgToDirMode__22CameraMgrInternalClassFPUi
    cam[mem->rd(pc + 4) & 3].mode = 1;
    return pc + 8;
}

void CamJobQueue::getJob(int s) {                              // getJob__22CameraMgrInternalClassFi
    if (slot[s].job != CJ_NONE) return;
    int h = head;
    int span = (tail >= h) ? 0 : 8;
    int end = tail + span;
    for (int i = h; i < end; i++) {
        int idx = i & 7;
        if (ringState[idx] == 1 && (int32_t)mem->rd(ringRec[idx] + 4) == s) {
            int type = (int32_t)mem->rd(ringRec[idx] + 8);
            switch (type) {
                case CJ_GAME:      slot[s].job = type; slot[s].handler = CH_GAME; break;
                case CJ_FRAMESPD:  slot[s].job = type; slot[s].handler = CH_FRAMESPD; break;
                case CJ_SPLCNT:    slot[s].job = type; slot[s].handler = CH_SPLCNT; break;
                case CJ_CAFSIDX:   slot[s].job = type; slot[s].handler = CH_CAFSIDX; break;
                case CJ_FINISHPOS: slot[s].job = type; slot[s].handler = CH_FINISHPOS; break;
                case CJ_TRACE:     slot[s].job = type; slot[s].handler = CH_TRACE; break;
                default: break;                                // tipo desconocido: el original no asigna rutina
            }
            ringState[idx] = 2; slot[s].ring = idx;
            return;
        }
    }
}

void CamJobQueue::finishJob(int s, CamHost& host) {            // finishJob__22CameraMgrInternalClassFi
    ringState[slot[s].ring] = 0;
    slot[s].job = CJ_NONE; slot[s].handler = CH_NOP;
    int h = head, t = tail;
    int end = t + ((t >= h) ? 0 : 8);
    bool found = false;
    for (int i = h; i < end; i++) {
        if (ringState[i & 7] != 0) { head = (int16_t)(i & 7); found = true; break; }
    }
    if (!found) head = (int16_t)t;
    getJob(s);
    runSlot(s, host);
}

void CamJobQueue::runSlot(int s, CamHost& host) {              // __ptmf_scall(this, s)
    uint32_t rec = ringRec[slot[s].ring];
    switch (slot[s].handler) {
        case CH_FRAMESPD: {                                    // setFrameSpd (0x1CAB20): velocidad = 1.2f * parametro (float)
            uint32_t bits = mem->rd(rec + 0xC); float p; std::memcpy(&p, &bits, 4);
            cam[s].speed = 1.2f * p; finishJob(s, host); break; }
        case CH_SPLCNT: {                                      // setCamSplCnt (0x1CAB90): 1 si p == 1, si no p*50/60 (en 50 Hz)
            int32_t p = (int32_t)mem->rd(rec + 0xC);
            cam[s].splCnt = (p == 1) ? 1 : (p * 50) / 60; finishJob(s, host); break; }
        case CH_CAFSIDX:                                       // setCafsIdx (0x1CAC40)
            slot[s].cafsIdx = (int8_t)mem->rd(rec + 0xC); finishJob(s, host); break;
        case CH_NOP: break;
        default: host.runHandler(slot[s].handler, s); break;
    }
}
}

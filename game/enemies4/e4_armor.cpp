#include "e4_armor.h"
namespace vn { namespace en4 {

void ArmorLogic::init(ArmorState& s, const ArmorDt& dt, E4Events& ev) {
    s.flags = 0x10001;
    s.dt = dt;
    if (dt.shown != 0) {
        e4emit(ev, E4_SET_HANDLER, AH_PREDISP, 0);
        e4emit(ev, E4_SET_HANDLER, AH_DISP, 0x10005);
    } else {
        e4emit(ev, E4_SET_HANDLER, AH_NOTDISP, 0);
    }
}

void ArmorLogic::comeOff(ArmorState& s, E4Events& ev) {
    e4emit(ev, E4_SPAWN_ATTACH, s.dt.attachModel, 0, 0, 0, s.dt.localPos[0], s.dt.localPos[1], s.dt.localPos[2]);
    s.part.stage = (int16_t)(s.part.stage + 1);
    e4emit(ev, E4_TASK_CLOSE);
    s.closed = true;
}

void ArmorLogic::preDisp(ArmorState& s, E4Events& ev) {
    if ((s.part.flag154 & 0x100) == 0) comeOff(s, ev);
}

void ArmorLogic::notDisp(ArmorState& s, const uint32_t* modelTable, E4Events& ev) {
    if ((s.part.flag154 & 0x100) == 0) {
        s.part.modelRef = modelTable[s.dt.partIdx * 6 + s.part.stage];   // v1 = tabla + idx*24; + stage*4
        comeOff(s, ev);
    }
}
}}

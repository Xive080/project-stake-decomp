#include "e4_throw.h"
namespace vn { namespace en4 {

void ThrowLogic::endLocalOffset(int coop, bool rightSide, float k, float out[3]) {
    if (coop == 1) { out[0] = 0; out[1] = 0; out[2] = -1.5f; return; }
    float side = rightSide ? 160.0f : -160.0f;                 // a2 == 0 -> +160 (0x43200000), si no -160
    out[0] = (-1.5f * side) / k;
    out[1] = (-0.0f) / k;
    out[2] = -1.5f;
}

void ThrowLogic::init1(ThrowObjState& s, ThrowOwner& owner, int flightFrames, bool holdsAttackGate, E4Env& e, E4Events& ev) {
    s.coop = e.coop;
    s.timer = flightFrames; s.flightFrames = s.timer;
    s.scale = 0.2f;                                            // 0x3E4CCCCD
    s.frameStart = 0x1B008D; s.frameEnd = 0x1B0096;
    e4emit(ev, E4_SOUND_GLOBAL, 0xE040F);
    owner.flags2 |= 0x20000000;
    s.frame = s.frameStart;
    s.flags = 0x10001;
    if (holdsAttackGate) { s.flags |= 0x20000000; e.attackGate = 1; }
    s.alpha = 1.0f; s.state = TS_FLY;
}

void ThrowLogic::ctrlPre(ThrowObjState& s, ThrowOwner& o, E4Env& e, E4Events& ev) {
    if (!(s.flags & 0x04000000)) {
        if (o.flags2 & 0x20) s.flags |= 0x04000000;            // la madre entra en su fase final -> este objeto tambien muere
    }
    if (!(s.flags & 0x8000) && (s.flags & 0x8)) {              // derribado a tiros
        e4emit(ev, E4_SOUND_GLOBAL, 0xE0411);
        s.flags |= 0x8000; s.flags |= 0x4000;
        if (s.flags & 0x2) e4emit(ev, E4_HITNUM, 0);
        if (s.flags & 0x4) e4emit(ev, E4_HITNUM, 1);
        s.alpha = 0.5f;
        s.state = TS_SHOT_DOWN;
    }
    auto release = [&]() {                                     // comun a los finales de los estados 2 y 5
        e.attackSlot[s.player & 1] = 0;
        if (s.flags & 0x20000000) e.attackGate = 0;
        if (s.hasGroup) e4emit(ev, E4_CLOSE_GROUP);
        else            e4emit(ev, E4_TASK_CLOSE);
        s.closed = true;
    };
    switch (s.state) {
    case TS_SHOT_DOWN:
        if (!(s.flags & 0x04000000)) o.flags2 &= ~0x20000000u;
        s.alpha -= fbits(0x3D088889);                               // 0x3D088889
        s.scale += fbits(0x3D888889);                                // 0x3D888889
        if (s.alpha <= 0.0f) release();
        break;
    case TS_FLY:
    case TS_HIT:
    case TS_FADE: {
        if (s.state == TS_FLY) {
            for (int i = 0; i < 3; i++) s.pos[i] += (s.end[i] - s.start[i]) / (float)s.flightFrames;
            s.timer -= 1;
            if (s.timer >= 0) break;                           // sigue volando
            s.state++;
        }
        if (s.state == TS_HIT) {
            e4emit(ev, E4_DAMAGE_PLAYER, s.player, 1, 7);      // DecPlayerLife_Chk(jugador, 1, 7)
            s.flags |= 0x01000000; s.flags |= 0x8000;
            s.timer = 15;
            s.state++;
        }
        // TS_FADE
        s.alpha -= fbits(0x3D23D70A);
        s.timer -= 1;
        if (!(s.timer > 0)) { s.flags |= 0x4000; s.timer = 0; }
        if (s.alpha <= 0.0f) {
            e.attackSlot[s.player & 1] = 0;
            if (!(s.flags & 0x04000000)) o.flags2 &= ~0x20000000u;
            if (s.flags & 0x20000000) e.attackGate = 0;
            if (s.hasGroup) e4emit(ev, E4_CLOSE_GROUP); else e4emit(ev, E4_TASK_CLOSE);
            s.closed = true;
        }
        break;
    }
    default: break;
    }
}

void ThrowLogic::init3(ThrowObjState& s) {
    s.frameStart = 0x260067; s.frameEnd = 0x260086; s.timer = 0;
    s.frame = s.frameStart; s.flags = 0x10001;
    s.alpha = 0; s.scale2 = 0; s.scale = 0;                    // +0x160 / +0x16C / +0x15C = 0
}
void ThrowLogic::ctrl3Pre(ThrowObjState& s, const ThrowOwner& owner, E4Events& ev) {
    if (owner.flags & 0x04000000) { e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
}
void ThrowLogic::ctrl3Pos(ThrowObjState& s, E4Events& ev) {
    s.frame += 1;
    if (!((uint32_t)s.frame < (uint32_t)s.frameEnd)) s.frame = s.frameStart;     // sltu: sin signo
    if (s.timer < 25) { s.alpha += fbits(0x3D23D70B); s.scale2 += fbits(0x3D23D70B); }                 // 0x3D23D70B
    if (!(s.timer < 101)) { s.alpha -= fbits(0x3CA12773); s.scale2 -= fbits(0x3CA12773); }     // 0x3CA12773
    if (!(s.timer < 150)) { e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
    s.timer += 1;
}
}}

namespace vn { namespace en4 {
void ThrowLogic::init9(ThrowObjState& s, ThrowOwner& owner, int kind, int flightFrames, float k, int (*rand)(), float endLocal[3]) {
    (void)owner;
    float rx = (float)(rand() % 0xF0);
    float ry = (float)(rand() % 0xA8);
    int r = rand(); int m = r & 1; if (r < 0 && m != 0) m -= 2;
    if (m == 0) rx *= -1.0f;
    r = rand(); m = r & 1; if (r < 0 && m != 0) m -= 2;
    if (m == 0) ry *= -1.0f;
    endLocal[0] = (-1.5f * rx) / k; endLocal[1] = (-1.5f * ry) / k; endLocal[2] = -1.5f;
    s.kind = kind; s.timer = flightFrames;
    s.scale = 0.5f; s.frameStart = 0x2900F1; s.frameEnd = 0x2900F1; s.frame = s.frameStart;
    s.flags = 0x10001; s.alpha = 1.0f; s.state = TS_FLY;
}

void ThrowLogic::ctrl9Pre(ThrowObjState& s, ThrowOwner& o, int solo, float k, E4Env& e, E4Events& ev) {
    if (!(s.flags & 0x04000000)) { if (o.flags2 & 0x20) s.flags |= 0x04000000; }
    if (!(s.flags & 0x8000) && (s.flags & 0x8)) {             // derribado
        e4emit(ev, E4_SOUND_GLOBAL, 0x1B0017);
        s.flags |= 0x8000; s.flags |= 0x4000;
        if (s.flags & 0x2) e4emit(ev, E4_HITNUM, 0);
        if (s.flags & 0x4) e4emit(ev, E4_HITNUM, 1);
        if (!(s.flags & 0x04000000)) o.pending -= 1;
        e4emit(ev, E4_TASK_CLOSE); s.closed = true;           // el original sigue ejecutando el switch de estado
    }
    if (s.state == TS_FLY) {
        if (s.view[2] < -1.0f) return;                        // aun lejos de la camara
        s.state++;
    }
    if (s.state == TS_HIT) {
        float sx = (s.view[0] * k) / s.view[2];
        int player;
        if (e.coop == 1) player = solo;
        else player = (sx < 0.0f) ? 1 : 0;
        e4emit(ev, E4_DAMAGE_PLAYER, player, 1, 7);
        s.flags |= 0x01000000; s.flags |= 0x8000;
        s.timer = 15;
        s.state++;
    }
    if (s.state == TS_FADE) {
        s.alpha -= fbits(0x3D23D70A);
        if (s.alpha <= 0.0f) {
            if (!(s.flags & 0x04000000)) o.pending -= 1;
            e4emit(ev, E4_TASK_CLOSE); s.closed = true;
        }
    }
}

void ThrowLogic::ctrl9Pos(ThrowObjState& s) {
    if (!(s.flags & 0x4000)) {
        s.frame += 1;
        if (!((uint32_t)s.frame < (uint32_t)s.frameEnd)) s.frame = s.frameStart;
    }
}
}}

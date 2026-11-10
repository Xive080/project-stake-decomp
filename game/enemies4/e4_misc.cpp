#include "e4_misc.h"
#include <cmath>
namespace vn { namespace en4 {

void EnSndTable::init(const int* ids) {
    count = 0;
    for (; ids[count] != -1 && count < kMax; count++) { rec[count].id = ids[count]; rec[count].lastFrame = 0; rec[count].unused = -1; }
}
bool EnSndTable::request(int id, E4Env& e, E4Events& ev) {
    for (int i = 0; i < count; i++) {
        if (rec[i].id != id) continue;
        int d = rec[i].lastFrame - e.frameCounter; if (d < 0) d = -d;
        if (d >= 5) {
            rec[i].lastFrame = e.frameCounter;
            if (e.stage != 0) e4emit(ev, E4_SOUND_GLOBAL, id);
        }
        return true;
    }
    return false;
}

void EnEtcSndLogic::init(EnEtcSndState& s, int soundId, int delayNtsc) { s.soundId = soundId; s.delay = palFrames(delayNtsc); }
void EnEtcSndLogic::ctrl(EnEtcSndState& s, E4Events& ev) {
    s.delay -= 1;
    if (!(s.delay > 0)) { e4emit(ev, E4_SOUND_GLOBAL, s.soundId); e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
}

void DamSphereLogic::ctrlPre(DamSphereState& s, DamOwner& o, E4Events& ev) {
    if (o.flags & 0x04000000) { e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
    if (!(s.flags & 0x8000) && (s.flags & 0x8)) {              // esfera sin bloqueo y golpeada
        if (!(o.flags & 0x100)) {                              // el dueno no es invulnerable: recibe el impacto
            o.flags |= 0x8;
            if (s.flags & 0x2) { o.flags2 &= ~0x20000000u; o.flags |= 0x2; }
            else               { o.flags2 |= 0x20000000u;  o.flags |= 0x4; }
            o.flags2 |= 0x40000000u;
        }
        s.flags &= ~0x8u; s.flags &= ~0x2u; s.flags &= ~0x4u;
        s.flags |= 0x8000;                                     // se bloquea hasta el siguiente fotograma
        if (o.flags & 0x100) s.flags &= ~0x8000u;              // invulnerable: sigue activa
    }
}

void PartsChainLogic::run(int parentTask, int a, int b, int c, int d, int e, E4Events& ev) {
    // new_EnEtcParts(parent, a, b, 2, c, d, e) y luego (..., 3, ...): el tipo va en el cuarto argumento
    e4emit(ev, E4_SPAWN_PARTS, parentTask, a, b, 2);
    e4emit(ev, E4_SPAWN_PARTS, parentTask, a, b, 3);
    (void)c; (void)d; (void)e;
}

void EnmIdTable::init() { for (int i = 0; i < 32; i++) slot[i] = 0; }
int EnmIdTable::set(uintptr_t enemy, uint32_t& flags2, int8_t& idByte) {
    idByte = -1;
    for (int i = 0; i < 32; i++) {
        if (slot[i] == 0) { flags2 |= 1; slot[i] = enemy; idByte = (int8_t)i; return i; }
    }
    return -1;
}
void EnmIdTable::clear(uint32_t flags2, int8_t idByte) {
    if (idByte >= 0 && idByte < 32 && (flags2 & 1)) slot[idByte] = 0;
}

int PatnLogic::init(PatnStat& s, const PatnTbl* t) {
    s.tbl = t; s.pos = 0; s.speed = t->speed; s.ended = 0;
    s.value = t->list ? t->list[0] : 0;
    return s.value;
}
int PatnLogic::forward(PatnStat& s) {
    const PatnTbl* t = s.tbl;
    int idx = (int)s.pos;
    s.value = t->list ? t->list[idx] : idx;          // lee ANTES de avanzar
    s.pos += s.speed;
    s.ended = 0;
    float fi = (float)(int)s.pos;                    // cvt.w.s -> cvt.s.w
    if (t->loop) {
        if (fi < 0.0f) s.pos = s.pos + (float)t->count;
        else if (!(fi < (float)t->count)) s.pos = s.pos - (float)t->count;
        s.ended = 0;
    } else {
        if (fi < 0.0f) { s.pos = 0.0f; s.ended = 1; }
        else if (!(fi < (float)t->count)) { s.pos = (float)(t->count - 1); s.ended = 1; }
    }
    return s.value;
}

void partsChangeChk(int16_t gameState, uint32_t& ownerFlags, int player, int32_t weaponClass[2]) {
    if (gameState != 0xA) ownerFlags |= (0x200 | 0x400 | 0x800);
    weaponClass[player] = 0;
    if (ownerFlags & 0x800) weaponClass[player] = 0;
}

ShootOrder setShootOrder(int16_t gameState, int solo, int (*rand)()) {
    ShootOrder o; o.first = -1; o.second = -1;
    if (gameState == 0xE) { o.first = 0; return o; }   // gp-0x7350 = 0 (gp-0x734C se queda en -1)
    if (solo == 0) o.first = 0;
    else if (solo == 1) o.first = 1;
    else if (solo == 2) {
        int r = rand(); int m = r & 1; if (r < 0 && m != 0) m -= 2;
        if (m != 0) { o.first = 0; o.second = 1; } else { o.first = 1; o.second = 0; }
    }
    return o;
}

int scLRChk(float x, float z, float k) { return ((x * k) / z < 0.0f) ? 1 : -1; }
bool scChkP(float px, float py, float pz, float k, float w, float h) {
    float sx = (px * k) / pz, sy = (py * k) / pz;
    if (sx < -w) return false;
    if (!(sx <= w)) return false;
    if (sy < -h) return false;
    if (!(sy <= h)) return false;
    return true;
}

void AttackObjLogic::init(AttackObjState& s, int tblB0, int tblB1, int tblLife, float tblScale, uint32_t tblW14, int8_t b1E, int8_t b1F, E4Env& e, int16_t& alive) {
    s.flags = 1; s.type = tblB1; s.id = tblB0 & 0xFF; s.life = tblLife;
    s.scale0 = tblScale; s.scale1 = tblScale; s.w14C = (int)tblW14; s.player = b1E; s.fixed = b1F;
    e.gp7374 += 1; alive += 1;
}
bool AttackObjLogic::selectTarget(AttackObjState& s, int playerCount, int solo, int gameState, const int modes[2], int attackSlot[2], int (*rand)()) {
    if (playerCount == 2) {
        int a = rand() & 1;
        if (attackSlot[a] != 0) {
            a = (a + 1) & 1;
            if (attackSlot[a] != 0) { attackSlot[1] = 0; attackSlot[0] = 0; }
        }
        s.player = (int8_t)a;
    } else if (playerCount == 1) {
        s.player = (int8_t)solo;
    }
    int p = s.player;
    bool ok = (gameState == 9) ? true : ((p == 0 || p == 1) ? modes[p] == 5 : false);   // is_AttackPlayer
    if (!ok) s.player = -1;
    if (s.player == -1) return false;
    attackSlot[s.player] = 1;
    return true;
}
float AttackObjLogic::targetDist(const float p[3], const float t[3]) {
    float dx = p[0] - t[0], dy = p[1] - t[1], dz = p[2] - t[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
void AttackObjLogic::localOffset(int player, float out[3]) { out[0] = -1.2f * (float)(1 - 2 * player); out[1] = 0; out[2] = -1.5f; }
void AttackObjLogic::setTarget(AttackObjState& s, int arg, const float cam[3], const float worldOffset[3]) {
    if (arg == 0 || s.fixed == 1 || s.player == -1) { for (int i = 0; i < 3; i++) s.target[i] = cam[i]; }
    else                                            { for (int i = 0; i < 3; i++) s.target[i] = worldOffset[i]; }
}

void manySeLoop(ManySeState& s, int gp7298, int gp7294, E4Events& ev) {
    if (s.count == 0 || gp7298 != 0 || gp7294 != 0) { e4emit(ev, E4_TASK_CLOSE_MOT); s.closed = true; }   // el original sigue ejecutando
    if (s.timer == s.period) { s.timer = 0; e4emit(ev, E4_SOUND_GLOBAL, s.soundId); s.count -= 1; }
    s.timer += 1;
}

void bgmFadeLoop(BgmFadeState& s) {
    bool done = false;
    if (!(s.step <= 0.0f)) {
        s.vol += s.step;
        if (!(s.vol < 0.0f)) { s.vol = 0.0f; done = true; }
        if (!(s.vol < s.target)) { s.vol = s.target; done = true; }
    } else {
        s.vol += s.step;
        if (s.vol <= -960.0f) { s.vol = -960.0f; done = true; }
        if (s.vol <= s.target) { s.vol = s.target; done = true; }
    }
    s.outVolume = (int)s.vol;                                  // func_1E9920(gp-0x7280, (int)vol)
    if (done) s.closed = true;                                 // func_1A3F80
}

void AttachLogic::ctrlPre(AttachState& s, float ground, EnSndTable& snd, E4Env& e, E4Events& ev) {
    s.groundSaved = ground;
    float g = ground + 1.0f;
    for (int i = 0; i < 3; i++) s.rot[i] += s.rotVel[i];
    for (int i = 0; i < 3; i++) s.vel[i] += s.acc[i];
    if (s.pos[1] + s.vel[1] < g) {                              // toca el suelo
        s.pos[1] = g;
        s.vel[0] *= 0.7f; s.vel[1] *= -0.3f; s.vel[2] *= 0.7f;
        for (int i = 0; i < 3; i++) s.rotVel[i] = (int)(0.5f * (float)s.rotVel[i]);
        if (!(s.flags & 0x40000000)) { snd.request(0x2001B, e, ev); s.flags |= 0x40000000; }
    }
    for (int i = 0; i < 3; i++) s.pos[i] += s.vel[i];
    s.timer -= 1;
    if (!(s.timer > 0)) {
        s.timer = 0;
        for (int i = 0; i < 3; i++) { s.scale[i] -= s.scaleDecay[i]; if (s.scale[i] < 0.0f) s.scale[i] = 0.0f; }
    }
    if (s.flags & 0x40000000) {
        s.alpha -= 0.04f;                                        // 0x3D23D70A
        if (s.alpha <= 0.0f) { s.flags |= 0x04000000; e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
    }
}

void AttachLogic::init(AttachState& s, int (*rand)()) {
    s.rotVel[0] = (rand() & 7) << 10;
    int r = rand(); int m = r & 1; if (r < 0) { if (m != 0) m -= 2; } m <<= 1;
    int sign = 1 - m;
    s.rotVel[1] = sign * ((rand() & 7) << 10);
    { uint32_t u = 0xBD20902E; float f; __builtin_memcpy(&f, &u, 4); s.acc[1] = f; }   // gravedad (+0x48)
    s.alpha = 1.0f; s.flags = 0;
    s.scale[0] = s.scale[1] = s.scale[2] = 2.0f;
    s.scaleDecay[0] = 1.2f * (2.0f / 60.0f); s.scaleDecay[1] = 1.2f * (2.0f / 30.0f); s.scaleDecay[2] = 1.2f * (2.0f / 20.0f);
    s.timer = 0;
}
}}

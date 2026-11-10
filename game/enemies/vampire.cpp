#include "vampire.h"
#include <cmath>
namespace vn { namespace en {

void VampireLogic::initLife(VampireState& v, const EnemyEnv& e) {           // EnVampireTask::charInit (parte de vida)
    int life = kVampLifeByType[v.type % kVampTypes];
    life += kVampLifeByLevel[e.levelA];
    life += (e.solo == 2) ? kVampLifeByDiff2P[e.levelB] : kVampLifeByDiff1P[e.levelB];
    if (e.gameState == 0xA && e.gameKind == 1)
        life = static_cast<int>(static_cast<float>(life) * rules::enemyLifeScale(e));
    v.life = life; v.lifeMax = life; v.lifePrev = life;
    if (v.type == 8) { v.subType = 0; v.reachLimit = -20.0f; }               // tipo 8: +0x1D7D = 0, +0x1DC8 = -20.0
}

void VampireLogic::decLife(VampireState& v, const EnemyEnv& e, int baseDamage) {   // EnVampireTask::decLife 0x9542B0
    int dmg = rules::scaledDamage(e, baseDamage, e.lastHitPlayer);
    if (e.gameState == 0xA && e.gameKind == 1 && e.pl[e.lastHitPlayer & 1].oneHit != 0) dmg = v.life;
    v.lifePrev = v.life;
    v.life -= dmg;
}

int VampireLogic::sufferDamage(VampireState& v, const EnemyEnv& e, EventList& ev, int player, int part, int weaponClass) {   // 0x953D90
    int snd = 0;
    switch (weaponClass) {                    // gp-0x7348[player]
        case 4: break;
        case 0: case 1: case 3: snd = 0x180010; break;
        case 2:
            snd = 0x20025;
            if (e.gameState == 0xA && e.gameKind == 1 && e.pl[player & 1].weapon == 0xE) emit(ev, EV_EFFECT_TRACER, part, player);
            else emit(ev, EV_EFFECT_HIT, part, player);
            break;
        default: break;                       // el original no hace nada fuera de 0..4
    }
    (void)v;
    emit(ev, EV_DAMAGE_MOT, player);          // changeDamageMot(player)
    return snd;
}

void VampireLogic::checkDamage(VampireState& v, EnemyEnv& e, const HitInput& h, EventList& ev) {   // 0x953410
    int sound = 0;
    if (v.flags & VF_HITCHECK) {                                  // (CheckHitParts: lo hace el motor y rellena HitInput)
        for (int slot = 0; slot < 2; slot++) {
            int pl = h.shooter[slot];
            if (pl == -1) continue;
            int part = h.partOfPlayer[pl & 1];
            e.lastHitPlayer = pl;
            if (part <= 0) continue;
            if (v.flags & VF_DEAD) continue;
            VampPart& pt = v.parts[part % kVampParts];
            if (!(v.flags & VF_INVULN)) {
                int base = kVampDamage[v.type % kVampTypes][part % kVampParts][pt.hitLevel % 6];
                decLife(v, e, base);
                if (v.life <= 0) {
                    v.flags |= VF_DEAD;
                    if (!(pt.flag150 & 0x01000000)) emit(ev, EV_PARTS_CHAIN, part, pl);
                    v.stateFlags |= (VS_HIT | VS_DEADHIT);
                    emit(ev, EV_SCORE, pl, 100);
                    emit(ev, EV_HITMARK, part, pl, 0x3E9);
                } else {
                    v.stateFlags |= VS_HIT;
                    sound = sufferDamage(v, e, ev, pl, part, h.weaponClass[pl & 1]);
                    emit(ev, EV_HITMARK, part, pl, 0x3E8);
                }
                if (pt.flag154 & 0x100) { pt.flag154 &= ~0x100u; emit(ev, EV_SOUND, 0x2002C, 0); }
                emit(ev, EV_SCORE, pl, 10);
            }
            emit(ev, EV_HITNUM, pl);
        }
    }
    if (v.flags & VF_DEAD) {
        sound = 0x180011;
        for (int i = 1; i < kVampParts; i++) v.parts[i].flag154 &= ~0x100u;   // el original desenrolla partes 1..17
    }
    if (sound != 0) emit(ev, EV_SOUND, sound, 0);
}

bool VampireLogic::damagePlayer(const VampireState& v, const EnemyEnv& e, EventList& ev, float reach, int kind) {   // 0x955410
    if (e.cam.state != 2) return false;
    if (reach <= v.reachLimit) return false;
    int target = 1;
    if (e.playerCount == 1) target = e.solo;
    else if (v.screenX < 0.0f) target = 0;
    if (target < 0 || target >= 2) return false;
    if (!rules::playerCanBeHurt(e.pl[target])) return false;
    emit(ev, EV_DAMAGE_PLAYER, target, 1, kind);
    return true;
}

bool VampireLogic::chkParam(VampireState& v, const EnemyEnv& e) {   // 0x954...: chkParam
    bool reset = false;
    switch (v.paramKind) {
        case 0: reset = (v.motActive == 0); break;
        case 1: reset = !(v.frame3C < static_cast<float>(v.paramA)); break;
        case 2: reset = (e.cam.pathId == v.paramA) && !(e.cam.frame < static_cast<float>(v.paramB)); break;
        default: break;
    }
    if (reset) v.paramKind = -1;
    return reset;
}

int VampireLogic::setAtcPlID(VampireState& v, EnemyEnv& e) {         // 0x954...: setAtcPlID
    int id = 0;
    if (e.attackSlot[0] != 0) id = 1;
    else if (e.attackSlot[1] != 0) id = 0;
    else {
        switch (e.playerCount) {
            case 1: id = (e.coop == 1) ? (e.rand() % 2) : e.solo; break;
            case 2: id = rules::weightedPlayerPick(e); break;
            default: break;
        }
    }
    if (id < 0 || id > 1 || e.attackSlot[id] != 0) { v.atkPlayer = -1; return -1; }
    e.attackSlot[id] = 1;
    v.atkPlayer = static_cast<int8_t>(id);
    return id;
}

static inline int halfTrunc(int16_t x) { return x / 2; }      // el original: (x + (x<0)) >> 1 == division truncada
static inline int quarterTrunc(int16_t x) { return x / 4; }

void VampireLogic::checkDamageMotEnd(VampireState& v, const int16_t* motLen) {   // 0x954...: checkDamageMotEnd
    if (v.stateFlags & VS_DAMMOT_KIND) {
        if (v.layerBusy == 0) { v.flags &= ~VF_DAMAGE_MOT; v.stateFlags &= ~VS_DAMMOT_KIND; return; }
        if (!(v.layerFlag6B & 4)) {
            int16_t len = motLen[v.layerMot];
            if (!(v.layerFrame < static_cast<float>(quarterTrunc(len)))) v.layerEndFrame = static_cast<float>(len);
        }
    } else {
        int16_t len = motLen[v.curMot];
        if (!(v.curFrame < static_cast<float>(halfTrunc(len)))) { v.flags &= ~VF_DAMAGE_MOT; v.motActive = -1; }
    }
}

void VampireLogic::changeDeadMot(VampireState& v, EventList& ev) {   // 0x954370
    if (v.stateFlags & VS_DEADSND) {
        v.stateFlags &= ~VS_DEADSND;
        emit(ev, EV_SOUND, 0x17030D, (v.emitter << 12) + 0x17030D);
        v.speedScale = 1.2f;                                              // 0x3F99999A
    } else if (v.flags & VF_BIT2000) {
        emit(ev, EV_ADJ_CHEST, 0x49, 10);
    }
    emit(ev, EV_MOTION, 0x49, 10, 0);                                     // motSupplement(anim 0x49, mezcla 10)
    v.motActive = 1; v.paramA = 0x19; v.paramKind = 0x14;
}

void VampireLogic::dead(VampireState& v, EnemyEnv& e, Counters& c, const DeadIn& in, EventList& ev) {   // 0x956810
    switch (v.step) {
    case 0:
        if (v.atkPlayer != -1) { e.attackSlot[v.atkPlayer & 1] = 0; v.atkPlayer = -1; }
        c.alive--;
        v.flags2 |= 2;
        changeDeadMot(v, ev);
        v.velY = 0; v.velX = 0; v.velZ = 0; v.gravity = 0;
        if (in.aboveGround) { v.motLoop = -1; v.flags |= VF_AIRBORNE_DEAD; v.stateFlags |= VS_AIRBORNE; v.step = 1; }
        else { v.flags &= ~VF_AIRBORNE_DEAD; v.motLoop = 0; v.stateFlags &= ~VS_AIRBORNE; v.step = 3; }
        if (v.type == 8) emit(ev, EV_BEAT_RECORD, v.type);
        break;
    case 1:
        if (!(v.frame3C < static_cast<float>(v.paramA))) {
            v.flags &= ~VF_AIRBORNE_DEAD; v.motActive = 0;
            v.velX = in.kickX; v.velY = -1.5f; v.velZ = in.kickZ; v.gravity = -0.0392f;   // 0xBD20902E
            v.step = 2;
        }
        break;
    case 2:
        if (in.landed) { v.motActive = 1; v.motLoop = 0; v.step = 3; }
        break;
    case 3:
        if (v.stateFlags & VS_AIRBORNE) {
            v.velX *= 0.8f; v.velZ *= 0.8f;
            if (std::fabs(v.velX) < 0.01f && std::fabs(v.velZ) < 0.01f) {
                v.velZ = 0; v.velY = 0; v.velX = 0; v.gravity = 0;
                v.stateFlags &= ~VS_AIRBORNE;
            }
        } else if (v.motActive == 0) {
            v.flags |= VF_FINISHED; c.onScreen--; v.flags2 |= 4;
        }
        break;
    default: break;
    }
}

void VampireLogic::attackJump(VampireState& v, float jumpFrames, float tx, float ty, float tz, EventList& ev) {   // Attack_Eknd_Sub 0x955...
    float T = jumpFrames / 1.44f;
    float g = -0.0392f;
    v.speedScale = 1.2f;                      // 0x3F99999A
    v.step = 3;
    v.gravity = g;
    v.velX = (tx - v.posX) / T;
    v.velZ = (tz - v.posZ) / T;
    v.velY = ((ty - (5.0f + v.posY)) / T) - ((T * g) / 2.0f);
    v.stateFlags |= VS_AIRBORNE;
    emit(ev, EV_SOUND, 0x2001F, 0);
}

void VampireLogic::attackEknd(VampireState& v, EnemyEnv& e, const AttackIn& in, EventList& ev) {   // 0x955E00
    const int snd = v.emitter << 12;
    switch (v.step) {
    case 0:
        setAtcPlID(v, e);
        if (v.stateFlags & VS_DEADSND) { v.motLoop = -1; v.step = 2; return; }
        v.motId = 0x312; v.step = 1;
        /* sigue en el paso 1 */
    case 1:
        if (v.flags & VF_DAMAGE_MOT) return;
        if (v.curMot != v.motId) { emit(ev, EV_MOTION, v.motId, 10, 0); return; }
        if (v.frame3C < 45.0f) {
            if (!(v.curFrame <= 25.0f - v.speedScale) && v.curFrame < 25.0f + v.speedScale) emit(ev, EV_SOUND, 0x17030C, 0);
            return;
        }
        if (v.h1DD4 == -1) { emit(ev, EV_MOTION, 0x325, 5, 0); }
        else {
            emit(ev, EV_MOTION, v.h1DD4, 5, 0);
            if (v.h1DD4 == 0x314 || v.h1DD4 == 0x313) v.speedScale = 0.96f;          // 0x3F75C290
        }
        v.motLoop = -1;
        v.stateFlags |= VS_DEADSND;                                                 // bit 0x10
        emit(ev, EV_SOUND, 0x17030D, snd + 0x17030D);
        v.step = 2;
        return;
    case 2: {
        bool cue = false;
        static const float w325[2] = {20.0f, 47.0f};
        static const float w30x[4] = {20.0f, 47.0f, 78.0f, 134.0f};
        const float* w = nullptr; int n = 0;
        if (v.curMot == 0x30B || v.curMot == 0x30A) { w = w30x; n = 4; }
        else if (v.curMot == 0x325) { w = w325; n = 2; }
        for (int i = 0; i < n; i++) if (!(v.curFrame <= w[i] - v.speedScale) && v.curFrame < w[i] + v.speedScale) { cue = true; break; }
        if (cue) emit(ev, EV_SOUND, 0x2001F, 0);
        if (v.dist1DD0 < 50.0f) { emit(ev, EV_SCRIPT, 200, 0); return; }              // Attack_Eknd_Sub: el motor aporta el objetivo y llama a attackJump
        if (v.atkPlayer == -1) { setAtcPlID(v, e); return; }
        if (!(v.dist1DD0 < 80.0f)) return;
        if (v.curMot == 0x314 || v.curMot == 0x313) {
            if (v.curFrame <= 3.0f) return;
            if (!(v.curFrame < 11.0f)) return;
        }
        emit(ev, EV_SCRIPT, 200, 0);                                               // Attack_Eknd_Sub (el motor aporta el objetivo y llama a attackJump)
        return; }
    case 3:
        if (v.dist1DD0 < 16.0f) {
            v.stateFlags &= ~VS_AIRBORNE;
            damagePlayer(v, e, ev, in.reach, 7);
            v.aiTimer = 0; v.step = 4;
        }
        return;
    case 4:
        if (v.aiTimer < 10.0f) return;
        v.stateFlags |= VS_AIRBORNE;
        v.velX *= -1.2f; v.velY *= -1.2f; v.velZ *= -1.2f;
        if (v.atkPlayer != -1) { e.attackSlot[v.atkPlayer & 1] = 0; v.atkPlayer = -1; }
        if (v.hasMark1DC4) {
            float dx = in.markX - v.posX, dz = in.markZ - v.posZ;
            float d = std::sqrt(dx * dx + dz * dz);
            v.velX = in.speedXZ * dx / d;
            v.velZ = in.speedXZ * dz / d;
            v.dist1DC0 = in.camToMark;
        }
        v.step = 5;
        return;
    case 5: {
        if (!(v.posY < in.height + 5.0f)) return;
        bool land = !(v.dist1DD0 <= v.dist1DC0) ? true : (in.speedXZ < 0.5f);
        // original: si dist1DD0 <= dist1DC0 y velocidad >= 0.5 -> rebote; si dist1DD0 > dist1DC0 -> aterriza
        if (land) {
            if (v.h1DD4 == 0x314 || v.h1DD4 == 0x313) { if (v.hasMark1DD8) v.motLoop = 0; /* + copia posExtra desde la marca 1DD8 (motor) */ }
            else v.motLoop = -1;
            v.stateFlags &= ~0x18u;
            emit(ev, EV_SOUND, 0x17030D, snd + 0x17030D, 1);
            v.gravity = 0; v.velY = 0;
            emit(ev, EV_MOTION, 0x30D, 1, 1);
            emit(ev, EV_SOUND, 0x2001F, 0);
            v.step = 6;
        } else if (v.posY < in.height) {
            v.velX *= 0.75f; v.velY *= -0.75f; v.velZ *= 0.75f;
            emit(ev, EV_SOUND, 0x2001F, 0);
        }
        return; }
    case 6:
        if (v.frame3C + v.speedScale < static_cast<float>(in.motLen ? in.motLen[v.curMot] : 0)) {
            if (!(v.frame3C <= 25.0f - v.speedScale) && v.frame3C < 25.0f + v.speedScale) emit(ev, EV_SOUND, 0x20022, 0);
            return;
        }
        if ((v.h1DD4 == 0x314 || v.h1DD4 == 0x313) && v.hasMark1DC4) emit(ev, EV_SCRIPT, 201, 0);   // reposiciona respecto a la marca (matriz)
        v.aiMode = 1; v.step = 0; v.aiTimer = 0;
        return;
    default: return;
    }
}

namespace {
struct Rd {
    const uint8_t* p; int size; int& pos; bool ok = true;
    int16_t s16() { if (pos + 2 > size) { ok = false; return 0; } int16_t r = static_cast<int16_t>(p[pos] | (p[pos + 1] << 8)); pos += 2; return r; }
    uint32_t u32() { if (pos + 4 > size) { ok = false; return 0; } uint32_t r = p[pos] | (p[pos + 1] << 8) | (p[pos + 2] << 16) | (uint32_t(p[pos + 3]) << 24); pos += 4; return r; }
    float f32() { uint32_t u = u32(); float f; static_assert(sizeof(f) == 4, ""); __builtin_memcpy(&f, &u, 4); return f; }
    void skip(int n) { pos += n; if (pos > size) ok = false; }
};
}

int VampireLogic::getParam(VampireState& v, EnemyEnv& e, const uint8_t* script, int scriptSize, EventList& ev, int maxOps) {   // 0x954500
    (void)e;
    Rd r{script, scriptSize, v.scriptPos};
    int ops = 0;
    while (v.paramKind == -1 && r.ok && ops < maxOps) {
        int op = r.s16(); if (!r.ok) break; ops++;
        switch (op) {
        case 0: v.motActive = r.s16(); if (v.motActive > 0) v.paramKind = 0; break;
        case 1: v.paramA = r.s16(); v.paramKind = 1; break;
        case 2: v.paramA = r.s16(); v.paramB = r.s16(); r.skip(2); v.paramKind = 2; break;
        case 3: { int a = r.s16(), b = r.s16(), c = r.s16(); emit(ev, EV_SCRIPT, 3, a, b, c); break; }
        case 4: { int a = r.s16(), b = r.s16(), c = r.s16(); emit(ev, EV_SCRIPT, 4, a, b, c); v.f88 = r.f32(); v.f90 = r.f32(); break; }
        case 5: { int a = r.s16(); emit(ev, EV_SCRIPT, 5, a); break; }
        case 6: { int a = r.s16(), b = r.s16(), c = r.s16(); emit(ev, EV_ADJ_CHEST, a, b); emit(ev, EV_SCRIPT, 3, a, b, c); break; }
        case 7: { int a = r.s16(), b = r.s16(), c = r.s16(); emit(ev, EV_SCRIPT, 7, a, b, c); emit(ev, EV_SCRIPT, 3, a, b, c); break; }   // adjPos(a,b) + motSupplement
        case 8: r.skip(2); v.speedScale = 1.2f * r.f32(); break;
        case 9: r.skip(2); v.flags |= r.u32(); break;
        case 10: r.skip(2); v.flags &= ~r.u32(); break;
        case 11: { r.skip(2); uint32_t m = r.u32();
            if ((m & VS_DEADSND) && v.curMot == 0x312) emit(ev, EV_SOUND, 0x17030D, (v.emitter << 12) + 0x17030D);
            v.stateFlags |= m; break; }
        case 12: r.skip(2); v.stateFlags &= ~r.u32(); break;
        case 13: { int a = r.s16(); v.aiMode = static_cast<int8_t>(a & 0xFF); v.step = 0; v.aiTimer = 0; v.motActive = -1; break; }
        case 14: r.skip(2); v.posX = r.f32(); v.posY = r.f32(); v.posZ = r.f32(); break;
        case 15: v.scriptMark1DD8 = v.scriptPos; v.posExtra[0] = r.s16(); v.posExtra[1] = r.s16(); v.posExtra[2] = r.s16(); break;
        case 16: v.h1DD4 = r.s16(); break;
        case 17: v.scriptMark1DC4 = v.scriptPos + 2; r.skip(0xE); break;
        case 18: if (r.s16() != 0) v.flags1CCC |= 2; else v.flags1CCC &= ~2u; break;
        case 19: r.skip(2); emit(ev, EV_SOUND, static_cast<int>(r.u32()), 0); break;
        case 20: v.paramKind = 20; break;
        default: break;                                    // >20: se ignora (sltiu 0x15)
        }
    }
    return ops;
}
}}  // namespace vn::en

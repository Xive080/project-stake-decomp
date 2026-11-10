#include "people.h"
namespace vn { namespace en {

// ELF main (direccion virtual v -> offset v-0x140000+0x800): punteros D_274368 = {0x2D8538, 0x2D8540, 0x2D8548}, 8 bytes cada uno.
const uint8_t kPeopleBaseDamage[3][8] = {{100, 110, 80, 35, 35, 60, 45, 45}, {100, 90, 60, 25, 25, 40, 35, 35}, {100, 180, 70, 35, 35, 50, 45, 45}};
// GAMEMOD.BIN 0x9ACB10 (offset v-0x92B000): clase de parte (0..7) para las partes 0..17.
const int16_t kPeoplePartClass[18] = {0, 2, 1, 3, 3, 3, 3, 4, 4, 4, 4, 5, 6, 6, 6, 7, 7, 7};
// ELF: D_2743B0 = {0x274380, 0x274390, 0x2743A0}.
const int16_t kPeopleDamageMot[3][8] = {{6097, 6097, 6097, 6100, 6098, 6096, 6101, 6099}, {6278, 6278, 6278, 6282, 6280, 6277, 6281, 6279}, {132, 132, 132, 132, 132, 132, 132, 132}};

void PeopleLogic::addPeopleNum(PeopleCounters& c, int stage) {            // 0x20AF60
    c.total++;
    c.totalByStage[stage % 7]++;
}

void PeopleLogic::addRescuePeopleNum(PeopleCounters& c, const EnemyEnv& e, EventList& ev) {   // 0x20AF90
    c.rescued++;
    c.rescuedByStage[e.stage % 7]++;
    if (e.gameState != 0xA || e.gameKind != 2) emit(ev, EV_GALLERY, e.gameState, c.rescued);
}

// Sonido de impacto (muerto / vivo) por fase: jtbl 0x2C2CB0 (muerto) y 0x2C2C90 (vivo, par = even / impar = odd). Fase 5 = ninguno; 6 = como 0.
static int stageIdx(int stage) { return stage == 6 ? 0 : stage; }
static const int kDeadSnd[5] = {0x150018, 0x0F0010, 0x080011, 0x090011, 0x0A0012};
static const int kHurtOdd[5] = {0x15000C, 0x0D000D, 0x19000E, 0x11010E, 0x0A0013};
static const int kHurtEven[5] = {0x15000D, 0x0D000E, 0x19000F, 0x11010F, 0x0A0014};
static const int kParaSnd[5] = {0x15000E, 0x0D0013, 0x19000C, 0x11010C, 0x0A000E};

static bool validStage(int stage) { return stage >= 0 && stage <= 6 && stage != 5; }

int PeopleLogic::sufferVampDamage(PeopleTaskState& p, EnemyEnv& e, EventList& ev, int part, int weaponClass) {   // 0x20A8A0
    (void)weaponClass;
    int cls = kPeoplePartClass[part % 18];
    int base = kPeopleBaseDamage[p.type % 3][cls];
    int dmg = rules::scaledDamage(e, base, e.lastHitPlayer);
    if (e.gameState == 0xA && e.gameKind == 1 && e.pl[e.lastHitPlayer & 1].oneHit != 0) dmg = p.life;
    p.lifePrev = p.life;
    p.life -= dmg;
    if (p.life <= 0) {
        p.flags |= PF_DEAD;
    } else if (!(p.flags & PF_BIT2000)) {
        int mot = kPeopleDamageMot[p.type % 3][cls];
        if (part <= 10) {                                                     // slti s1,0xB
            if (!(p.flags & PF_DAMAGE_MOT)) {
                p.flags |= PF_DAMAGE_MOT; p.stateFlags |= PS_DAMMOT_KIND;
                emit(ev, EV_MOTION, mot, 5, 1);                               // motDivOnce(mgr, obj, 1, mot, 5, 5, 8)
            }
        } else if (!(p.flags & PF_BIT10000000) && (!(p.flags & PF_DAMAGE_MOT) || (p.stateFlags & PS_DAMMOT_KIND))) {
            p.flags |= PF_DAMAGE_MOT;
            emit(ev, EV_MOTION, mot, 10, 1);                                  // motSupplement(mot, 10)
            p.loop = 1; p.stateFlags &= ~PS_DAMMOT_KIND; p.p1DD0 = 0;
        }
        p.flags |= PF_BIT80000;
    }
    if (e.gameState == 0xA && e.gameKind == 1 && e.pl[e.lastHitPlayer & 1].weapon == 0xE) emit(ev, EV_EFFECT_TRACER, part, e.lastHitPlayer);
    else emit(ev, EV_EFFECT_HIT, part, e.lastHitPlayer);
    int snd = 0;
    int st = e.stage;
    if (validStage(st)) {
        int i = stageIdx(st);
        if (p.flags & PF_DEAD) snd = kDeadSnd[i];
        else snd = rules::pickByParity(e.rand(), kHurtOdd[i], kHurtEven[i]);
    }
    if (snd) emit(ev, EV_SOUND, snd, p.emitter << 12);
    return snd;
}

void PeopleLogic::checkVampDamage(PeopleTaskState& p, EnemyEnv& e, const HitInput& h, EventList& ev) {   // 0x20A7A0
    if (!(p.flags & PF_HITCHECK)) return;
    for (int slot = 0; slot < 2; slot++) {
        int pl = h.shooter[slot];
        if (pl == -1) continue;
        int part = h.partOfPlayer[pl & 1];
        e.lastHitPlayer = pl;
        if (part <= 0) continue;
        if (p.flags & PF_DEAD) continue;
        if (!(p.flags & PF_INVULN)) {
            sufferVampDamage(p, e, ev, part, h.weaponClass[pl & 1]);
            if (p.flags & PF_DEAD) emit(ev, EV_SCORE, pl, 100);
            emit(ev, EV_SCORE, pl, 10);
        }
        emit(ev, EV_HITNUM, pl);
    }
}

void PeopleLogic::checkParaDamage(PeopleTaskState& p, uint32_t& partnerFlags, EnemyEnv& e, const HitInput& h, PeopleCounters& c, EventList& ev) {   // 0x20A530
    if (partnerFlags & 8) {
        int pl;
        switch (partnerFlags & 6) { case 4: pl = 1; break; case 2: pl = 0; break; default: pl = e.rand() & 1; break; }
        emit(ev, EV_HITNUM, pl);
        if (p.flags & PF_INVULN) {
            partnerFlags &= ~0xEu;                                           // and con 0xFFFFFFF1
        } else {
            emit(ev, EV_SCORE, pl, 400);
            p.rescuer = static_cast<int8_t>(pl);
            addRescuePeopleNum(c, e, ev);
            emit(ev, EV_CAMERA_CHG, 0, 1);
            p.stateFlags |= (PS_RESCUED | PS_RESCUED_MARK);                  // |= 0x11
            emit(ev, EV_SOUND, 0x2002D, p.emitter << 12);
        }
    } else if (p.flags & PF_HITCHECK) {
        for (int slot = 0; slot < 2; slot++) {
            int pl = h.shooter[slot];
            if (pl == -1) continue;
            int part = h.partOfPlayer[pl & 1];
            if (part <= 0) continue;
            p.hitPart = static_cast<int8_t>(part);
            if (!(p.flags & PF_INVULN)) {
                if (!(p.stateFlags & PS_PARA_HIT)) {
                    emit(ev, EV_SCORE, pl, -200);                            // golpear a un civil con parasito resta 200
                    emit(ev, EV_CAMERA_CHG, 0, 1);
                    p.stateFlags |= PS_PARA_HIT;
                }
            }
            emit(ev, EV_HITNUM, pl);
        }
    }
    if (p.stateFlags & PS_PARA_HIT) {
        if (validStage(e.stage)) emit(ev, EV_SOUND, kParaSnd[stageIdx(e.stage)], p.emitter << 12);
    }
}

void PeopleLogic::checkVampDamageMotEnd(PeopleTaskState& p, const int16_t* motLen) {   // 0x20AF.. (checkVampDamageMotEnd)
    if (p.stateFlags & PS_DAMMOT_KIND) {
        if (p.layerBusy == 0) { p.flags &= ~PF_DAMAGE_MOT; p.stateFlags &= ~PS_DAMMOT_KIND; }
    } else if (!((p.curFrame + p.speed) < static_cast<float>(motLen[p.curMot]))) {
        p.flags &= ~PF_DAMAGE_MOT;
        p.loop = -1;
    }
}

bool PeopleLogic::damagePlayer(const PeopleTaskState& p, const EnemyEnv& e, EventList& ev) {   // 0x20A250
    if (e.cam.state != 2) return false;
    if (p.screenLimit <= -10.0f) return false;
    int target = 1;
    if (e.playerCount == 1) target = e.solo;
    else if (p.screenX < 0.0f) target = 0;
    if (target < 0 || target >= 2) return false;
    if (!rules::playerCanBeHurt(e.pl[target])) return false;
    emit(ev, EV_DAMAGE_PLAYER, target, 1, 9);                                // DecPlayerLife(target, 1, 9)
    return true;
}

int PeopleLogic::motionSelect(const int16_t* list, int count, int (*rand)()) {   // motionSelect
    return list[rand() % count];
}

int PeopleLogic::setAtcPlID(PeopleTaskState& p, EnemyEnv& e, int posCount) {   // setAtcPlID
    int id = 0;
    switch (e.playerCount) {
        case 1: id = e.solo; break;
        case 2:
            if (e.pl[1].invuln) id = 0;
            else if (e.pl[0].invuln) id = 1;
            else id = rules::weightedPlayerPick(e);
            break;
        default: break;
    }
    p.atkPlayer = static_cast<int8_t>(id);
    if (e.attackSlot[0] != 0 || e.attackSlot[1] != 0 || id < 0 || id > 1) { p.atkPlayer = -1; return -1; }
    e.attackSlot[id] = 1;
    return e.rand() % posCount;                                               // indice de la posicion de espera (+0x1DEC = lista + idx*0xC)
}

void PeopleLogic::changeSurvive(PeopleTaskState& p, EnemyEnv& e, PeopleCounters& c, uint32_t& partnerFlags, EventList& ev) {   // 0x20B000
    p.flags &= ~PF_BIT10000000;
    p.flags |= PF_BIT80000;
    p.p1D78 = 0;
    partnerFlags |= PF_DEAD;                                                 // +0x1E6C->+0xC |= 0x04000000
    p.p1DD0 = -1;
    emit(ev, EV_MOTION, 0, 0x14, 1);                                         // motSupplement(anim de superviviente, 0x14)
    p.loop = 1;
    p.waitKind = 0;
    if (p.atkPlayer != -1) { e.attackSlot[p.atkPlayer & 1] = 0; p.atkPlayer = -1; }
    p.p1DBC = 0;
    if (e.stage == 6) c.stage6Alive--;
}

void PeopleLogic::changeVampire(PeopleTaskState& p, EnemyEnv& e, PeopleCounters& c, EventList& ev) {   // 0x20B150
    (void)e;
    p.p1DB8 = 0;
    p.flags |= (PF_BIT80000 | PF_INVULN);                                    // |= 0x80100
    p.flags &= ~PF_BIT10000000;
    p.p1DD0 = -1;
    c.flag7200 = 1;
    emit(ev, EV_MOTION, 0, 0xA, 1);
    p.loop = 1;
    p.waitKind = 0x2A;
    p.p1E28 = 0;
    if (e.stage == 6) c.flag500E34 = 1;
}

void PeopleLogic::lifeUp(const PeopleTaskState& p, const EnemyEnv& e, EventList& ev) {   // 0x20BCA0
    int pl = p.rescuer & 1;
    emit(ev, EV_SOUND, 0x2002A, p.emitter << 12);
    if (e.pl[pl].life >= e.pl[pl].maxLife) emit(ev, EV_SCORE, pl, 1000);
    else emit(ev, EV_LIFEUP_ITEM, pl);
}

int PeopleLogic::groanSound(const PeopleTaskState& p, const EnemyEnv& e) {   // 0x20BF60
    if (!validStage(e.stage)) return 0;
    int i = stageIdx(e.stage);
    static const int solo[5] = {0x15001D, 0x0F0015, 0x080014, 0x090014, 0x0A0015};
    static const int odd[5]  = {0x150015, 0x0F000C, 0x08000E, 0x09000E, 0x0A000C};
    static const int even[5] = {0x150016, 0x0F000D, 0x08000F, 0x09000F, 0x0A0016};
    if (p.type == 1) return solo[i];
    return rules::pickByParity(e.rand(), odd[i], even[i]);
}

int PeopleLogic::helpMeSound(const PeopleTaskState& p, const EnemyEnv& e) {   // 0x20C180
    if (!validStage(e.stage)) return 0;
    int i = stageIdx(e.stage);
    static const int t1[5]   = {0x150020, 0x080017, 0x080017, 0x090017, 0x0A0018};
    static const int t0[5]   = {0x150017, 0, 0, 0, 0};                        // fase 0: tipo != 1 -> 0x150017 sin nlRand
    static const int odd[5]  = {0, 0x080019, 0x080019, 0x090019, 0x0A0019};
    static const int even[5] = {0, 0x080010, 0x080010, 0x090010, 0x0A0017};
    if (i == 0) return p.type == 1 ? t1[0] : t0[0];
    if (i == 1 && p.type != 1) (void)e.rand();                               // la fase 1 cae en el codigo de la fase 2 tras una llamada extra a nlRand
    if (p.type == 1) return t1[i];
    return rules::pickByParity(e.rand(), odd[i], even[i]);
}

bool PeopleLogic::lastCtrlWait(PeopleTaskState& p, const EnemyEnv& e, PeopleCounters& c, const WaitIn& in, const int16_t* motLen) {   // LastCtrl 0x206..
    if (p.loop > 0 && !((p.curFrame + p.speed) < static_cast<float>(motLen[p.curMot]))) p.loop--;
    bool done = false;
    switch (p.waitKind) {
    case 7:
        if (in.backWaitDone) done = true;
        else if (p.p1DC0 >= 0 && in.eventFlags && in.eventFlags[p.p1DC0] != 0) done = true;
        break;
    case 0:
        if (p.loop == 0) { done = true; if (in.baseMatch) c.walking--; }
        break;
    case 1: if (!(p.frame3C < static_cast<float>(p.p1DC0))) done = true; break;
    case 2: if (e.cam.pathId == p.p1DC0 && !(e.cam.frame < static_cast<float>(p.p1DC4))) done = true; break;
    case 3: if (in.eventFlags && in.eventFlags[p.p1DC0] != 0) done = true; break;
    case 4: if (!in.onScreen) done = true; break;
    case 5: if (!(in.angleDeg <= ((640.0f * in.viewScale) / 448.0f) / 2.0f)) done = true; break;
    case 6: if (in.dist1DF8 <= static_cast<float>(p.p1DC0)) done = true; break;
    case 37: done = true; break;
    case 39: p.counter1E64++; if (p.counter1E64 >= 2) done = true; break;
    default: break;
    }
    if (done) p.waitKind = -1;
    if (in.abortAll) p.waitKind = -1;
    bool readNext = (p.waitKind == -1);                                       // getParam()
    return readNext;
}

namespace {
struct Rd {
    const uint8_t* p; int size; int& pos; bool ok = true;
    int16_t s16() { if (pos + 2 > size) { ok = false; return 0; } int16_t r = static_cast<int16_t>(p[pos] | (p[pos + 1] << 8)); pos += 2; return r; }
    uint32_t u32() { if (pos + 4 > size) { ok = false; return 0; } uint32_t r = p[pos] | (p[pos + 1] << 8) | (p[pos + 2] << 16) | (uint32_t(p[pos + 3]) << 24); pos += 4; return r; }
    float f32() { uint32_t u = u32(); float f; __builtin_memcpy(&f, &u, 4); return f; }
    void skip(int n) { pos += n; if (pos > size) ok = false; }
};
}

int PeopleLogic::getParam(PeopleTaskState& p, EnemyEnv& e, PeopleCounters& c, VmEnv& vm, const uint8_t* script, int scriptSize, EventList& ev, int maxOps) {   // 0x208C90
    Rd r{script, scriptSize, p.scriptPos};
    bool keep = true;                                                         // v1 del original: resultado del post-proceso anterior
    int ops = 0;
    while (p.waitKind == -1 && r.ok && ops < maxOps) {
        int op = r.s16(); if (!r.ok) break; ops++;
        switch (op) {
        case 0: p.loop = r.s16(); if (p.loop > 0) p.waitKind = 0; break;
        case 1: p.p1DC0 = r.s16(); p.waitKind = 1; break;
        case 2: p.p1DC0 = r.s16(); p.p1DC4 = r.s16(); r.skip(2); p.waitKind = 2; break;
        case 3: p.p1DC0 = r.s16(); p.waitKind = 3; break;
        case 4: case 5: r.skip(2); p.waitKind = op; break;
        case 6: p.p1DC0 = r.s16(); p.p1DD0 = -1; if (!(vm.dist1DF8 <= 5.0f + static_cast<float>(p.p1DC0))) p.waitKind = 6; break;
        case 7: p.p1DC0 = r.s16(); if (!(vm.distToTarget <= 8.0f)) p.waitKind = 7; break;
        case 8: { int a = r.s16(), b = r.s16(), d = r.s16(); emit(ev, EV_SCRIPT, 8, a, b, d); break; }
        case 9: { int a = r.s16(), b = r.s16(), d = r.s16(); emit(ev, EV_SCRIPT, 9, a, b, d); p.f88 = r.f32(); p.f90 = r.f32(); break; }
        case 10: { int a = r.s16(); emit(ev, EV_SCRIPT, 10, a); break; }
        case 11: { int a = r.s16(), b = r.s16(), d = r.s16(); emit(ev, EV_ADJ_CHEST, a, b); emit(ev, EV_SCRIPT, 8, a, b, d); break; }
        case 12: { int a = r.s16(), b = r.s16(), d = r.s16(); emit(ev, EV_ADJ_CHEST, a, b); emit(ev, EV_SCRIPT, 9, a, b, d); p.f88 = r.f32(); p.f90 = r.f32(); break; }
        case 13: r.skip(2); p.flags |= r.u32(); break;
        case 14: r.skip(2); p.flags &= ~r.u32(); break;
        case 15: r.skip(2); p.stateFlags |= r.u32(); break;
        case 16: { r.skip(2); uint32_t m = r.u32();
            if ((m & 0x200) && !(p.stateFlags & 0x200)) c.walking--;
            p.stateFlags &= ~m; break; }
        case 17: c.stage6Alive--; r.skip(2); break;
        case 18: { int a = r.s16(); p.p1DC8 = (a == 1) ? 1 : (50 * a) / 60; break; }   // 50 -> 60 Hz (x*50/60, division truncada)
        case 19: r.skip(2); p.p1DB8 = 0; p.loop = -1; p.p1DD0 = -1; p.waitKind = 19; break;
        case 20: if (p.atkPlayer != -1) { e.attackSlot[p.atkPlayer & 1] = 0; p.atkPlayer = -1; } r.skip(2); break;
        case 21: p.f1DF0 = static_cast<float>(r.s16()); break;
        case 22: p.mark1DF4 = p.scriptPos + 2; r.skip(0xE); break;
        case 23: r.skip(2); p.posX = r.f32(); p.posY = r.f32(); p.posZ = r.f32(); break;
        case 24: p.posExtra[0] = r.s16(); p.posExtra[1] = r.s16(); p.posExtra[2] = r.s16(); break;
        case 25: { int i = r.s16(); if (vm.eventFlags) vm.eventFlags[i] = 1; break; }
        case 26: p.p1DD0 = r.s16(); break;
        case 27: p.p1DD0 = p.scriptPos + 2; r.skip(0xE); break;
        case 28: r.skip(2); { uint32_t id = r.u32(); if (keep) emit(ev, EV_SCRIPT, 28, static_cast<int>(id), 0, p.emitter << 12); } break;
        case 29: r.skip(2); { uint32_t id = r.u32(); emit(ev, EV_SCRIPT, 29, static_cast<int>(id), 1, p.emitter << 12); } break;
        case 30: r.skip(2); p.p1E60 = static_cast<int>(r.u32()); emit(ev, EV_SCRIPT, 30, p.p1E60); break;
        case 31: { int a = r.s16(); if (keep) emit(ev, EV_SCRIPT, 31, a); break; }
        case 32: { int a = r.s16(); if (keep) emit(ev, EV_SCRIPT, 32, a, (a == 1 && !(p.stateFlags & 1)) ? 1 : 0); break; }   // c = 1: ademas func_A06120
        case 33: if (r.s16() != 0) p.flags1CCC |= 2; else p.flags1CCC &= ~2u; break;
        case 34: if (r.s16() != 0) p.flags1CCC |= 1; else p.flags1CCC &= ~1u; break;
        case 35: if (keep) lifeUp(p, e, ev); r.skip(2); break;
        case 36: if (keep) { p.h1E34 = r.s16(); p.f1E30 = 1.2f * r.f32(); emit(ev, EV_SCRIPT, 36, p.h1E34); } else { r.skip(2); r.skip(4); } break;
        case 37: r.skip(2); p.waitKind = 37; break;
        case 38: r.skip(2); if (keep) { c.walking--; p.waitKind = 38; } break;
        case 39: {
            bool training = (e.gameState == 0xA && e.gameKind == 1);
            int peek = (r.pos + 2 <= r.size) ? static_cast<int16_t>(script[r.pos] | (script[r.pos + 1] << 8)) : 0;
            int8_t it = (vm.itemFlags ? vm.itemFlags[peek] : 0);
            bool a3 = training && (it & 1);
            bool a0 = a3 && (it & 2);
            if (a0) {
                if (p.counter1E64 != 0) break;                                // el original no avanza el flujo
                int a = r.s16();
                if (a == 15 || a == 7) { if (vm.g500F04 < 2) break; }
                emit(ev, EV_SCRIPT, 39, a); p.waitKind = 39;
            } else r.skip(2);
            break; }
        case 40: vm.g500F04++; r.skip(2); break;                                  // 0x500F04++
        case 41: p.p1E68 = r.s16(); break;
        case 42: p.waitKind = 42; break;
        default: break;
        }
        // Post-proceso tras cada orden (0x209778..0x209830)
        bool k;
        if (vm.abortAll != 0 || vm.skipFx != 0) {
            if (p.waitKind != 37) k = false;
            else k = true;                                                   // sigue a 0x2097D4
        } else {
            if (p.waitKind == 37) k = false;
            else k = true;
        }
        if (k) {                                                             // 0x2097D4
            if (!(p.stateFlags & 0x200)) k = true;
            else if (c.walking > 0) k = false;
            else { c.walking++; p.stateFlags &= ~0x200u; k = true; }
        }
        if (!k) p.waitKind = -1;
        keep = k;
    }
    return ops;
}
}}  // namespace vn::en

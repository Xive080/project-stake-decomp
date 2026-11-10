// Logica de EnZombieTask reescrita a mano desde el asm (asm/gamemod/gamemod_text.s). Ver zombie.h y MANIFEST.md.
#include "zombie.h"
#include "../enemies/vampire.h"   // tablas de vida compartidas (kVampLifeByLevel/Diff1P/Diff2P = D_009B6B50/80/B0)

namespace vn { namespace enov {

static inline bool has(uint32_t v, uint32_t m) { return (v & m) != 0; }

// ---------------------------------------------------------------------------------------------------------------------
bool ZombieLogic::isAttackPlayer(const ZEnv& e, int p) {                      // is_AttackPlayer 0x951080
    if (e.base.gameState == 9) return true;
    if (p == 0 || p == 1) return e.base.pl[p].mode == 5;
    return false;
}

int ZombieLogic::angChk(int ang, int center, int range) {                      // AngChk 0x95E920
    uint32_t c = (uint32_t)center & 0xFFFF;
    uint32_t lo = (c - (uint32_t)range) + 0x10000u, hi = c + (uint32_t)range + 0x10000u;
    if (hi < lo) { uint32_t t = lo; lo = hi; hi = t; }
    uint32_t a = (uint32_t)ang & 0xFFFF;
    while (a < lo) a += 0x10000u;                    // do { a += 0x10000 } while (a < lo)
    return (a <= hi) ? 1 : 0;
}

int ZombieLogic::eTurn(int cur, int target, int step) {                        // ETurn 0x95E870
    int t = target & 0xFFFF, a = cur & 0xFFFF;
    int d1 = a - t; if (d1 < 0) d1 = -d1;
    int d2 = (t + 0x10000) - a; if (d2 < 0) d2 = -d2;
    int d3 = (a + 0x10000) - t; if (d3 < 0) d3 = -d3;
    if (step >= d1 || step >= d2 || step >= d3) return t;
    if (a < t) { if (d3 < d1) step = -step; }
    else       { if (d2 >= d1) step = -step; }
    return a + step;
}

// ---------------------------------------------------------------------------------------------------------------------
void ZombieLogic::decNowEnemyAlvNumChk(ZombieState& z, ZEnv& e) {              // 0x95E4A0
    if (!has(z.flags2, ZF2_ALIVE_DEC)) { z.flags2 |= ZF2_ALIVE_DEC; e.alive--; }
}
void ZombieLogic::decNowEnemyNumChk(ZombieState& z, ZEnv& e) {                 // 0x95E4D0
    if (!has(z.flags2, ZF2_ONSCR_DEC)) { z.flags2 |= ZF2_ONSCR_DEC; e.onScreen--; }
}
void ZombieLogic::atcPlFlgClear(ZombieState& z, ZEnv& e) {                     // 0x95E440
    if (z.atkPlayer != -1) {
        if (z.atkPlayer == 0 || z.atkPlayer == 1) e.base.attackSlot[z.atkPlayer] = 0;   // el original no acota el indice
        z.atkPlayer = -1;
    }
    if (has(z.flags3, Z3_SLOT_TAKEN)) { z.flags3 &= ~Z3_SLOT_TAKEN; e.attackGate = 0; }
}
void ZombieLogic::deleteTask(ZombieState& z, ZEnv& e, ZEvents& ev) {           // Delete 0x95D320
    if (z.step == 0) {
        z.flags |= ZF_DEAD;
        decNowEnemyAlvNumChk(z, e);
        decNowEnemyNumChk(z, e);
        atcPlFlgClear(z, e);
        z.flags |= ZF_BIT4000;
        z.flags2 |= ZF2_BIT20;
        z.life = 0;
        z.step++;
    } else if (z.step == 1) {
        zemit(ev, ZE_TASK_CLOSE);                  // TaskCloseX
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Division con signo del original (resultado de sll/mult 0x88888889): (v * 50) / 60 truncada hacia cero.
static int framesOf(int v) { return (v * 50) / 60; }

bool ZombieLogic::attackPlayerSelect(ZombieState& z, ZEnv& e, int scLR) {       // 0x95E0C0
    en::EnemyEnv& b = e.base;
    z.atkPlayer = -1;
    if (e.attackGate == 0 && e.flag500F07 == 0) {
        if (b.coop == 1) {
            if (b.solo == 0 && b.attackSlot[0] == 0) z.atkPlayer = 0;
            if (b.solo == 1 && b.attackSlot[1] == 0) z.atkPlayer = 1;
        } else if (b.coop == 2) {
            if (b.playerCount == 1) {
                int sel = b.rand() % 2;                                  // % con signo de C
                if (b.attackSlot[sel & 1] == 0) z.atkPlayer = (int8_t)sel;
            } else if (e.onScreen == 1) {
                int sel = b.rand() % 2;
                if (b.attackSlot[sel & 1] != 0) sel = ~sel;
                z.atkPlayer = (int8_t)sel;
            } else {
                if (scLR == -1) { if (b.attackSlot[0] == 0) z.atkPlayer = 0; }
                if (scLR == 1 && b.attackSlot[1] == 0) z.atkPlayer = 1;
            }
        }
        if (!isAttackPlayer(e, z.atkPlayer)) z.atkPlayer = -1;
    }
    if (z.atkPlayer == -1) return false;
    if (!(z.reach354 <= 0.0f)) return false;
    if (z.atkPlayer == 0 || z.atkPlayer == 1) b.attackSlot[z.atkPlayer] = 1;     // PEND: el original escribe gp-0x7420 + 4*atkPlayer sin acotar (p.ej. -2 con modo 9)
    return true;
}

bool ZombieLogic::attackPlayerSelectAsd(ZombieState& z, ZEnv& e, int scLR, bool scChkP) {   // 0x95E2B0
    en::EnemyEnv& b = e.base;
    z.atkPlayer = -1;
    if (e.attackGate == 0) {
        if (b.coop == 1) {
            if (b.solo == 0) z.atkPlayer = 0;
            if (b.solo == 1) z.atkPlayer = 1;
        } else if (b.coop == 2) {
            if (b.playerCount == 1 || e.onScreen == 1) z.atkPlayer = (int8_t)(b.rand() % 2);
            else {
                if (scLR == -1) z.atkPlayer = 0;
                if (scLR == 1) z.atkPlayer = 1;
            }
        }
        if (!isAttackPlayer(e, z.atkPlayer)) z.atkPlayer = -1;
    }
    if (z.atkPlayer == -1) return false;
    if (!scChkP) return false;
    return z.reach354 <= 0.0f;
}

void ZombieLogic::damageToPlayer(ZombieState& z, ZEnv& e, const ZAtkRec& rec, ZEvents& ev) {   // 0x95E500
    int p = z.atkPlayer;
    if (p != 0 && p != 1) return;
    int mask = rec.mask;                                              // lb: con signo
    if (mask == (mask & (z.guardBits & 7))) return;                   // mascara ya cubierta por la proteccion
    if (has(z.flags, ZF_ATTACKING)) {
        if (e.base.cam.state == 2) { zemit(ev, ZE_DAMAGE_PLAYER, p, 1, rec.dmgType); e.flag500F07 = 1; }
        z.flags &= ~ZF_ATTACKING;
    } else {
        if (e.base.cam.state == 2) zemit(ev, ZE_DAMAGE_PLAYER, p, 1, rec.dmgType);
    }
    e.missCount++;
}

int ZombieLogic::damageToPlayer2(ZombieState& z, ZEnv& e, int frame, int dmgType, int mask, int scLR, ZEvents& ev) {   // 0x961FA0
    if (z.curFrame < (float)(uint32_t)frame) return 0;                // unsigned -> float
    if (!(z.reach < 0.0f)) return 0;
    int p = (scLR == -1) ? 0 : scLR;
    if (e.base.coop == 1) p = e.base.solo;
    if (p != 0 && p != 1) return 0;
    uint32_t m = (uint32_t)mask & 0xFF;
    if (m == (m & ((uint32_t)z.guardBits & 7))) return 1;             // cubierto: devuelve 1 sin dano
    if (has(z.flags, ZF_ATTACKING)) {
        if (e.base.cam.state == 2) { zemit(ev, ZE_DAMAGE_PLAYER, p, 1, dmgType & 0xFF); e.flag500F07 = 1; }
        z.flags &= ~ZF_ATTACKING;
    } else {
        if (e.base.cam.state == 2) zemit(ev, ZE_DAMAGE_PLAYER, p, 1, dmgType & 0xFF);
    }
    e.missCount++;
    return 1;
}

// ---------------------------------------------------------------------------------------------------------------------
void ZombieLogic::motSupSet(ZombieState& z, int mot, int p2, int p3, ZEvents& ev) {         // 0x95DF20
    if (has(z.flags3, Z3_MAIN_ANIM)) return;
    if (z.motObjId == mot) return;
    if (has(z.flags3, Z3_LAYER_ANIM)) { z.flags3 &= ~Z3_LAYER_ANIM; zemit(ev, ZE_MOTION_SET, mot, p2, p3); return; }
    if (z.motObjId != 0x43) zemit(ev, ZE_MOTION_SET, mot, p2, p3);
    else                    zemit(ev, ZE_MOTION_SET_NOBLEND, mot);
}
void ZombieLogic::motSupSet2(ZombieState& z, int mot, int p2, int p3, ZEvents& ev) {        // 0x95E000
    if (z.motObjId == mot) return;
    z.flags &= ~ZF_DAMAGE_MOT;
    z.flags3 &= ~Z3_MAIN_ANIM;
    z.flags3 &= ~Z3_LAYER_ANIM;
    if (z.motObjId != 0x43) zemit(ev, ZE_MOTION_SET, mot, p2, p3);
    else                    zemit(ev, ZE_MOTION_SET_NOBLEND, mot);
}

void ZombieLogic::damageMotEnd(ZombieState& z, const int16_t* motLen) {                     // 0x95BCD0
    if (!has(z.flags, ZF_DAMAGE_MOT)) return;
    float f2 = z.curFrame + z.frameOff1DE8;
    float f1 = z.layerFrame + z.frameOff1DEC;
    int cls = z.weaponClass;
    if (cls == 4) {
        if (has(z.flags3, Z3_LAYER_ANIM)) {
            float L = (float)motLen[z.layerMot];
            if (!(f1 < L)) { z.flags3 &= ~Z3_LAYER_ANIM; z.layerEndFrame = L; }
        }
        if (has(z.flags3, Z3_MAIN_ANIM)) {
            float L = (float)motLen[z.curMot];
            if (!(f2 < L)) z.flags3 &= ~Z3_MAIN_ANIM;
        }
    } else if (cls == 3 || cls == 1 || cls == 0) {
        if (has(z.flags3, Z3_LAYER_ANIM)) {
            float L = (float)motLen[z.layerMot];
            if (!(f1 < L) || !(f1 < L / 4.0f)) { z.flags3 &= ~Z3_LAYER_ANIM; z.layerEndFrame = L; }
        }
        if (has(z.flags3, Z3_MAIN_ANIM)) {
            float L = (float)motLen[z.curMot];
            if (!(f2 < L) || !(f2 < L / 4.0f)) z.flags3 &= ~Z3_MAIN_ANIM;
        }
    }
    if (!has(z.flags3, Z3_LAYER_ANIM) && !has(z.flags3, Z3_MAIN_ANIM)) z.flags &= ~ZF_DAMAGE_MOT;
}

// ---------------------------------------------------------------------------------------------------------------------
void ZombieLogic::weakHitClear(ZombieState& z) {                                // 0x95AF90
    for (int i = 0; i < 4; i++) if (z.weak[i]) *z.weak[i] &= 0xBFFFFFFFu;
}

void ZombieLogic::damage(ZombieState& z, const ZEnv& e, int player, const HitInput& h, const ZDamageIn& in, ZEvents& ev) {   // 0x95B640
    const int part = h.partOfPlayer[player];
    const int cls = h.weaponClass[player];
    const bool arcade = e.base.gameState == 0xA;
    const ZDamageIn::PartGeom& g = in.part[(part >= 0 && part < 18) ? part : 0];
    if (cls == 2) {
        if (arcade && e.base.gameKind == 1 && in.pl[player].weaponItem == 0xE)
            zemit(ev, ZE_EFFECT_LAUNCHER, part, player, 0, 0, g.selfPos[0], g.selfPos[1], g.selfPos[2] + 1.0f);   // la posicion pasa por la matriz de camara (motor)
        else {
            int code = (z.type == 0xF || z.type == 0xE || z.type == 0xC) ? 2 : 0;
            zemit(ev, ZE_EFFECT, 0x1F, code, 0, 0, g.bodyPos[0], g.bodyPos[1], g.bodyPos[2] + g.bodyF154 * in.bodyScale);
        }
        if (e.base.gameState != 0xE) zemit(ev, ZE_HITMARK, part, player, 0x3E8);
    } else {
        zemit(ev, ZE_EFFECT_PLAYER, player == 0 ? 0x26 : 0x27, 0, 0, 0, g.selfPos[0], g.selfPos[1], g.selfPos[2] + 1.0f);   // escala 0.5, matriz de camara (motor)
    }
    switch (cls) {
    case 0: case 1: case 3:
        zemit(ev, ZE_DAMAGE_MOT, player); zemit(ev, ZE_DAMAGE_VOICE); zemit(ev, ZE_SCORE, player, 10);
        break;
    case 2:
        if (part >= 0 && part < 18) z.parts[part].w18C = 0x29;
        zemit(ev, ZE_SOUND_GLOBAL, 0x20025, 0x40);
        zemit(ev, ZE_SCORE, player, 10);
        break;
    case 4:
        if (part > 0 && part < 0xB) zemit(ev, ZE_DAMAGE_MOT, player);
        zemit(ev, ZE_SCORE, player, 0x1E);
        zemit(ev, ZE_DAMAGE_VOICE);
        zemit(ev, ZE_ATTACH_OFF, part, 0);
        break;
    case 5:
        if (part > 0 && part < 0xB) zemit(ev, ZE_DAMAGE_MOT, player);
        zemit(ev, ZE_SCORE, player, 0xC8);
        zemit(ev, ZE_DAMAGE_VOICE);
        zemit(ev, ZE_EFFECT, 0x31, 0, 0, 0, g.bodyPos[0], g.bodyPos[1], g.bodyPos[2] - 2.0f);
        zemit(ev, ZE_SOUND_GLOBAL, 0x20025, 0x40);
        zemit(ev, ZE_SOUND_GLOBAL, 0x17000F, 0x40);
        break;
    default: break;
    }
    z.flags |= ZF_BIT8000000;
}

void ZombieLogic::damageCheck(ZombieState& z, ZEnv& e, const HitInput& h, const ZDamageIn& in, ZEvents& ev) {   // 0x95AFD0
    const bool wasInvuln = has(z.flags, ZF_INVULN);
    if (e.mode7218 != 2) z.flags |= ZF_INVULN;
    if (has(z.flags, ZF_HITCHECK)) {
        if (in.zomCheckHitParts) in.zomCheckHitParts(z);
        if (!has(z.flags, ZF_INVULN)) {
            for (int i = 0; i < 2; i++) {
                int s = h.shooter[i];
                if (s == -1) continue;
                int part = h.partOfPlayer[s];
                if (part <= 0) continue;
                zemit(ev, ZE_HITNUM, s);
                if (h.weaponClass[s] != 2) z.weaponClass = (int8_t)h.weaponClass[s];
                if (has(z.flags, ZF_DEAD)) continue;
                z.hitPart = (int8_t)part;
                if (z.life <= 0) {
                    z.flags3 |= Z3_BIT800;
                    z.flags |= ZF_DEAD;
                    z.killer = (int8_t)s;
                    std::memcpy(z.handler, in.deathHandler, 12);
                    z.handlerArg = 8;
                    z.step = 0;
                    if ((int)z.hitPart == (int)z.weakPartId) zemit(ev, ZE_SCORE, s, 0xC8);
                    zemit(ev, ZE_SCORE, s, 0x64);
                    damage(z, e, s, h, in, ev);
                    z.flags3 |= Z3_BIT80000;
                    if (!has(z.parts[part].flag150, 0x01000000))
                        zemit(ev, ZE_PARTS_CHAIN, part, s, in.chainA[0], in.chainA[1]);
                    if (e.base.gameState != 0xE) zemit(ev, ZE_HITMARK, part, s, 0x3E9);
                } else {
                    z.flags |= ZF_DAMAGE_MOT;
                    damage(z, e, s, h, in, ev);
                }
            }
        }
        weakHitClear(z);
    }
    if (!wasInvuln) z.flags &= ~ZF_INVULN;
}

void ZombieLogic::fallCheck(ZombieState& z) {                                    // 0x95D540
    if (!has(z.flags2, ZF2_FALLING)) return;
    switch (z.fallStep) {
    case 0:
        z.gravity = -0.0392f; z.fallStep++;                       // 0xBD20902E
        /* fallthrough */
    case 1: {
        float ground = z.groundY;
        z.velY += z.gravity;
        z.posY += z.velY;
        if (ground < z.posY + z.velY) return;                      // sigue cayendo
        z.velY = 0.0f; z.gravity = 0.0f; z.posY = ground; z.fallStep++;
    }   /* fallthrough */
    case 2:
        z.velY = 0.0f; z.gravity = 0.0f;
        z.flags2 &= ~ZF2_FALLING;
        z.fallStep = 0;
        break;
    default: break;
    }
}

void ZombieLogic::deadBounce(ZombieState& z, float ground) {                      // Dead_Sub 0x95BF40 (parte fisica)
    z.velY += z.gravity;
    z.posY += z.velY;
    if (!(ground <= z.posY + z.velY)) {
        z.posY = ground;
        z.velX *= 0.7f; z.velY *= 0.5f; z.velZ *= 0.7f;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Script. En el original el flujo es un puntero a s16 que avanza de 2 en 2 bytes.
static inline bool inRange(const ZombieState& z, int n) { return z.script && z.scriptPos >= 0 && z.scriptPos + n <= z.scriptSize; }
static inline int  rdS16(ZombieState& z) { int v = inRange(z, 2) ? zs16(z.script + z.scriptPos) : 0; z.scriptPos += 2; return v; }

int ZombieLogic::checkCom(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev, int maxOps) {   // 0x95DA60
    int n = 0;
    while (n < maxOps) {
        if (!inRange(z, 2)) return -1;
        int op = zs16(z.script + z.scriptPos);
        n++;
        if (op == -1) {
            if (t.tbl1B70) std::memcpy(z.handler, t.tbl1B70, 12);
            z.handlerArg = 0;
            z.scriptPos += 2;
            z.step = 0;
            deleteTask(z, e, ev);
            if (has(z.flags, ZF_ATTACKING)) e.missCount++;
            return n;
        } else if (op == 60) {
            if (!inRange(z, 4)) return -1;
            int idx = zs16(z.script + z.scriptPos + 2);
            if (t.tbl1B70 && idx >= 0 && (t.count == 0 || idx < t.count)) std::memcpy(z.handler, t.tbl1B70 + 3 * idx, 12);
            z.handlerArg = (int8_t)z.script[z.scriptPos + 2];
            z.scriptPos += 4;
            z.step = 0;
            return n;
        } else if (op >= 61 && op <= 66) {
            if (!inRange(z, 8)) return -1;
            uint32_t m = zu32(z.script + z.scriptPos + 4);
            z.scriptPos += 8;
            switch (op) {
            case 61: z.flags |= m; break;
            case 62: z.flags &= ~m; break;
            case 63: z.flags2 |= m; break;
            case 64: z.flags2 &= ~m; break;
            case 65: z.flags3 |= m; break;
            case 66: z.flags3 &= ~m; break;
            }
        } else {
            return -1;                                  // el original repetiria indefinidamente
        }
    }
    return -1;
}

void ZombieLogic::posSet(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev) {   // 0x9648D0
    z.posExtra[0] = rdS16(z); z.posExtra[1] = rdS16(z); z.posExtra[2] = rdS16(z);
    z.scriptPos += 2;
    if (inRange(z, 12)) {
        float x = zf32(z.script + z.scriptPos), y = zf32(z.script + z.scriptPos + 4), zz = zf32(z.script + z.scriptPos + 8);
        if (x != 0.0f || y != 0.0f || zz != 0.0f) {
            if (x == 0.0f && zz == 0.0f) z.posY = y;
            else { z.posX = x; z.posY = y; z.posZ = zz; }
        }
    }
    z.scriptPos += 12;
    checkCom(z, e, t, ev);
}

void ZombieLogic::missEnemyNumAdd(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev) {   // 0x9641C0
    e.missCount++;
    checkCom(z, e, t, ev);
}

void ZombieLogic::lifeSet(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev) {   // 0x9649D0
    z.life = rdS16(z);
    z.scriptPos += 2;
    z.life += en::kVampLifeByLevel[e.base.levelA];
    z.life += (e.base.solo == 2) ? en::kVampLifeByDiff2P[e.base.levelB] : en::kVampLifeByDiff1P[e.base.levelB];
    z.lifeMax = z.life;
    z.lifePrev = z.life;
    checkCom(z, e, t, ev);
}

void ZombieLogic::advDead(ZombieState& z, ZEnv& e, const ZDamageIn& in, ZEvents& ev) {   // 0x964550
    switch (z.step) {
    case 0:
        z.sv9C = rdS16(z); z.sA0 = rdS16(z); z.sA4 = rdS16(z); z.sA8 = rdS16(z);
        z.sAC = framesOf(rdS16(z));
        z.scriptPos += 2;
        motSupSet2(z, z.sv9C, 0, 0, ev);
        z.step++;
        /* fallthrough */
    case 1:
        z.sAC -= 1;
        if (z.sAC < 0) {
            z.flags |= ZF_DEAD;
            if (z.sA0 >= 0 && z.sA0 < 18 && !has(z.parts[z.sA0].flag150, 0x01000000))
                zemit(ev, ZE_PARTS_CHAIN, z.sA0 & 0xFF, (int8_t)z.sA4, in.chainA[0], in.chainA[1]);
            z.step++;
            /* fallthrough */
    case 2:
            if (e.base.cam.pathId == z.sA8) {
                z.flags2 |= ZF2_BIT20;
                z.flags |= ZF_BIT100000;
                z.step++;
                return;
            }
        } else {
            return;
        }
        break;
    case 3:
        zemit(ev, ZE_TASK_CLOSE);
        break;
    default: break;
    }
}

void ZombieLogic::tutDead(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev) {   // 0x964740
    switch (z.step) {
    case 0:
        z.life = rdS16(z); z.lifeMax = z.life; z.lifePrev = z.life;
        z.step++;
        /* fallthrough */
    case 1:
        z.sv9C = rdS16(z);
        z.sA0 = framesOf(rdS16(z));
        z.partOfPlayer0 = (int8_t)z.sv9C;
        z.flags |= ZF_HITCHECK;
        z.flags |= ZF_BIT2;
        if (z.sv9C >= 0 && z.sv9C < 18) { z.parts[z.sv9C].flag150 |= 8; z.parts[z.sv9C].flag150 |= 2; }
        zemit(ev, ZE_DAMAGE_CHECK);
        z.step++;
        /* fallthrough */
    case 2:
        z.sA0 -= 1;
        if (z.sA0 <= 0) {
            if (inRange(z, 2) && zs16(z.script + z.scriptPos) == -1) {
                z.scriptPos += 2;
                checkCom(z, e, t, ev);
            } else {
                z.step = 1;
            }
        }
        break;
    default: break;
    }
}

void ZombieLogic::st2Sc03_03(ZombieState& z, ZEnv& e, const ComTables& t, ZEvents& ev) {   // 0x9643E0
    if (z.step == 0) { z.sA4 = rdS16(z); z.sA8 = rdS16(z); z.step++; }
    if (z.step == 1) {
        if (!((z.curFrame + z.frameOff1DE8) < 250.0f)) { z.step++; checkCom(z, e, t, ev); }
    } else if (z.step == 2) {
        checkCom(z, e, t, ev);
    }
    if (z.step == 1 && !has(z.flags, ZF_DAMAGE_MOT) && z.curMot != z.sA4) motSupSet2(z, z.sA4, 0, 0, ev);
    if (z.curMot == z.sA4) {
        if (!(z.curFrame <= (float)z.sA8)) z.flags |= ZF_INVULN;
        if (!(z.curFrame < 250.0f)) z.flags2 |= ZF2_BIT200;
    }
}

void ZombieLogic::st3Sc03_02(ZombieState& z, ZEnv& e, const ComTables& t, const int16_t* motLen, int scLR, ZEvents& ev) {   // 0x9641F0
    if (z.step == 0) {
        z.sv9C = rdS16(z); z.sA0 = rdS16(z); z.sA4 = rdS16(z); z.sA8 = rdS16(z); z.sAC = rdS16(z); z.sB0 = rdS16(z);
        z.posExtra[1] = z.sB0;
        motSupSet2(z, z.sv9C, z.sAC, z.sA0, ev);
        z.flags3 &= ~Z3_BIT8;
        z.step++;
    }
    if (z.step == 1) {
        if (!has(z.flags3, Z3_BIT8)) {
            if (!(z.curFrame < (float)z.sA4)) {
                damageToPlayer2(z, e, z.sA4 & 0xFFFF, z.sA8 & 0xFF, 0xFF, scLR, ev);
                z.flags3 |= Z3_BIT8;
            }
        }
        if (!((z.curFrame + z.frameOff1DE8) < (float)motLen[z.sv9C])) { z.step++; checkCom(z, e, t, ev); }
    } else if (z.step == 2) {
        checkCom(z, e, t, ev);
    }
    if (z.step == 1 && !has(z.flags, ZF_DAMAGE_MOT) && z.curMot != z.sA4)     // sic: el original compara con +0x1DA4 (el fotograma del golpe)
        motSupSet2(z, z.sv9C, z.sAC, z.sA0, ev);
}

void ZombieLogic::st1Sc01_00(ZombieState& z, ZEnv& e, const ComTables& t, const int16_t* motLen, ZEvents& ev) {   // 0x962D90
    if (z.step == 0) { z.sA4 = rdS16(z); z.sA8 = rdS16(z); z.sAC = rdS16(z); z.scriptPos += 2; z.step++; }
    if (z.step == 1) {
        if (!((z.curFrame + z.frameOff1DE8) < (float)(motLen[z.sA4] - 1))) { z.step++; checkCom(z, e, t, ev); }
    } else if (z.step == 2) {
        checkCom(z, e, t, ev);
    }
    if (z.step == 1 && !has(z.flags, ZF_DAMAGE_MOT) && z.curMot != z.sA4) {
        z.flags &= ~ZF_DAMAGE_MOT;
        z.flags3 &= ~Z3_MAIN_ANIM;
        z.flags3 &= ~Z3_LAYER_ANIM;
        zemit(ev, ZE_MOTION_SET, z.sA4, z.sAC);
    }
}

void ZombieLogic::frmWait(ZombieState& z, ZEnv& e, const ComTables& t, const int16_t seqMot[2], ZEvents& ev) {   // 0x9631E0
    if (z.step == 0) {
        z.sA0 = rdS16(z); z.sA4 = rdS16(z); z.sA8 = rdS16(z);
        z.scriptPos += 2;
        z.sv9C = 0;
        if (z.sA4 == -1) z.sA4 = z.curMot;
        else if (z.sA4 == -2) z.sA4 = seqMot[has(z.flags3, Z3_ALT_ANIM) ? 1 : 0];
        z.step++;
    }
    if (z.step == 1) {
        motSupSet(z, z.sA4, 0, z.sA8, ev);
        z.sv9C++;
        if (z.sv9C >= framesOf(z.sA0)) { z.step++; checkCom(z, e, t, ev); }
    } else if (z.step == 2) {
        checkCom(z, e, t, ev);
    }
}

}}  // namespace vn::enov

#include "komono_rest.h"
namespace vn {

// Eleccion de jugador compartida por checkHit y getHitPl: ambos bits -> rand()%2 (semantica C: -1 si rand negativo e impar); solo uno -> se limpia.
static int pickPlayer(uint32_t& flags, RandFn rand) {
    bool p1 = (flags & 2) != 0, p2 = (flags & 4) != 0;
    if (p1 && p2) return rand() % 2;
    if (p1) { flags &= ~2u; return 0; }
    if (p2) { flags &= ~4u; return 1; }
    return 0;
}

bool k3CheckHit(uint32_t& flags, RandFn rand, int& pl) {
    if (!(flags & 8)) return false;
    pl = pickPlayer(flags, rand);
    flags &= ~8u;
    return true;
}

int TumiageColli::getHitPl(RandFn rand) { return pickPlayer(flags, rand); }

// ---------------------------------------------------------------- Komono3Task
void K3Objects::common3(K3Anim& a, K3Ctx& c, K3Out& o) {
    KomonoLogic::K3Event e = KomonoLogic::checkGameMode(a.k, *c.sys, *c.g);
    if (e.closeTask) o.closeTask = true;
    KomonoLogic::checkMissionEnd_setTrnsl(a.k, *c.g, c.paused);
    e = KomonoLogic::checkMissionCompleteMessageDispTiming(a.k, c.cam);
    if (e.showMissionComplete) { o.showMissionComplete = true; o.missionNo = e.missionNo; }
}

// checkColli_setBranchNo (0x9ED5E0): primer golpe -> efecto + setBranchNo + sonido 0xB012B; golpes siguientes (rama 1) -> efecto + 0x20025/0x1000.
static void hitBlock(K3Anim& a, K3Ctx& c, K3Out& o, std::initializer_list<int> firstSounds) {
    int pl = 0;
    if (a.k.branchState == 0) {
        if (k3CheckHit(a.hitFlags, c.rand, pl)) {
            o.hitEffectPl = pl;
            KomonoLogic::setBranchNo(*c.g);
            a.k.branchState = 1;
            for (int s : firstSounds) o.snd(s);
        }
    } else if (a.k.branchState == 1) {
        if (k3CheckHit(a.hitFlags, c.rand, pl)) { o.hitEffectPl = pl; o.snd(0x20025, 0x1000); }
    }
}

void K3Objects::commonFull(K3Anim& a, K3Ctx& c, K3Out& o) {
    common3(a, c, o);
    KomonoLogic::K3Event e = KomonoLogic::checkCloseTiming_clrBranchNo(a.k, *c.g, c.cam);
    if (e.closeTask) o.closeTask = true;
    hitBlock(a, c, o, {0xB012B});
}

void K3Objects::nuts(K3Anim& a, K3Ctx& c, K3Out& o) {
    common3(a, c, o);
    if (KomonoLogic::checkCamPath(c.cam, a.k.closePath, a.k.closeFrame)) { KomonoLogic::clrBranchNo(*c.g); o.closeTask = true; }
    if (a.k.branchState == 0) {
        int pl = 0;
        if (k3CheckHit(a.hitFlags, c.rand, pl)) {
            o.hitEffectPl = pl;
            KomonoLogic::setBranchNo(*c.g);
            a.k.branchState = 1;
            o.snd(0xC000E); o.snd(0xB012B);
        }
    }
    if (!c.paused && a.k.branchState == 1) {
        a.frame += 1.0f;
        float t = a.frame;
        if (t >= 56.0f && t < 57.2f) o.snd(0xC000F);          // 0x42600000 / 0x4264CCCD
        if (t >= 61.0f && t < 62.2f) o.snd(0xC000F);          // 0x42740000 / 0x4278CCCD
        if (!(t <= 90.0f)) { a.frame = 90.0f; a.animState = 1; }   // 0x42B40000
    }
    if (a.k.branchState == 0) o.goggleEffect = true;
}

void K3Objects::goldenRat(K3Anim& a, K3Ctx& c, K3Out& o) {
    commonFull(a, c, o);
    switch (a.animState) {
        case 0:
            if (!c.paused) {
                a.frame += 1.0f;
                int fi = (int)a.frame;
                if (fi % 90 == 60) o.snd(0x190110);
                if (!(a.frame <= 340.0f)) a.frame = 0;          // 0x43AA0000
                a.cnt59C = (a.cnt59C + 1) % 15;
            }
            if (a.k.branchState == 1) a.animState = 1;
            break;
        case 1:
            a.frame += 1.0f;
            a.cnt59C = (a.cnt59C + 1) % 15;
            if (!(a.frame <= 340.0f)) { a.frame = 0; a.animState = 2; a.cnt59C = 0; }
            if (c.cam.pathId == 0x23F && !(c.cam.frame < 65.0f)) { a.frame = 0; a.animState = 2; a.cnt59C = 0; }   // 0x42820000
            break;
        default: break;          // estado 2: reposo
    }
    if (a.k.branchState == 0) o.goggleEffect = true;
}

void K3Objects::secretDoor(K3Anim& a, K3Ctx& c, K3Out& o) {
    common3(a, c, o);
    if (KomonoLogic::checkCamPath(c.cam, a.k.closePath, a.k.closeFrame)) { KomonoLogic::clrBranchNo(*c.g); o.closeTask = true; }
    hitBlock(a, c, o, {0xB012B, 0xC0010, 0xC000D});
    if (a.k.branchState == 0) o.goggleEffect = true;
    if (!c.paused && a.k.branchState == 1) {
        if (a.frame > 10.0f && a.debrisCount < 7 && ((int)a.frame % 9) == 8) { o.debris.push_back(a.debrisCount); a.debrisCount++; }
        float f = a.frame + 1.2f;                                // 0x3F99999A
        a.frame = f;
        if (!(f <= 100.0f)) { a.frame = 100.0f; a.animState = 1; }   // 0x42C80000
        else o.quake++;                                          // func_21AD80(1, 0x10)
    }
    if (!c.paused && c.cam.pathId == 0x17A) {
        if (!(c.cam.frame < 203.0f) && c.cam.frame <= 263.0f) a.doorAngle += 0x222;    // 0x434B0000 / 0x43838000
        if (c.cam.frame >= 203.0f && c.cam.frame < 204.2f) o.snd(0xC0012);               // 0x434C3333
    }
}

void K3Objects::silverKinoko(K3Anim& a, K3Ctx& c, K3Out& o) {
    KomonoLogic::K3Event e = KomonoLogic::checkGameMode(a.k, *c.sys, *c.g);
    if (e.closeTask) o.closeTask = true;
    if (a.k.missionState == 2) {
        if (!c.paused) {
            a.k.trnsl -= 0.033333335f;                           // 0x3D088889
            if (a.k.trnsl < 0.0f) { a.k.trnsl = 0.0f; KomonoLogic::clrBranchNo(*c.g); o.closeTask = true; }
        }
    } else if (a.k.missionState == 1) {
        if (c.g->nowSettingGp72E0 == 0) a.k.missionState = 2;
    }
    e = KomonoLogic::checkMissionCompleteMessageDispTiming(a.k, c.cam);
    if (e.showMissionComplete) { o.showMissionComplete = true; o.missionNo = e.missionNo; }
    e = KomonoLogic::checkCloseTiming_clrBranchNo(a.k, *c.g, c.cam);
    if (e.closeTask) o.closeTask = true;
    hitBlock(a, c, o, {0xB012B});
    if (a.k.branchState == 0) o.goggleEffect = true;
}

// ---------------------------------------------------------------- Kusari
bool kusariHahenLoop(KusariHahen& h, bool paused) {
    for (int i = 0; i < 3; i++) {
        KusariPiece& p = h.p[i];
        if (paused) continue;
        if (p.pos[1] < h.floorY) {                // rebote en el suelo
            p.vel[1] = p.vel[1] - 0.5f * p.vel[1];
            p.vel[0] = 0.5f * p.vel[0];
            p.vel[2] = 0.5f * p.vel[2];
            p.pos[1] = h.floorY;
            p.rotSpdA = 0; p.rotSpdB = 0;
        }
        p.rotA = (p.rotA + p.rotSpdA) & 0xFFFF;
        p.rotB = (p.rotB + p.rotSpdB) & 0xFFFF;
        p.vel[1] += p.grav;
        p.pos[0] += p.vel[0];
        p.pos[1] += p.vel[1];
        p.pos[2] += p.vel[2];
    }
    if (!paused) {
        h.age++;
        if (h.age >= 0x15) h.alpha -= 0.033333335f;
    }
    return h.age >= 0x33;
}

KusariTick kusariCtrl(TumiageColli* colli[], int n, bool endCP, bool paused, RandFn rand) {
    KusariTick t;
    if (endCP) {
        for (int i = 0; i < n; i++) if (colli[i]) colli[i]->close();
        t.closeAll = true;
    }
    if (paused) return t;
    for (int i = 0; i < n; i++) {
        if (!colli[i] || !colli[i]->chkColli()) continue;
        KusariLinkEvent e;
        e.link = i; e.pl = colli[i]->getHitPl(rand); e.spawnDebris = true; e.sound = 0xD0018;
        t.broken.push_back(e);
        colli[i]->close(); colli[i] = nullptr;
    }
    return t;
}

// ---------------------------------------------------------------- bornItem
bool bornItem(const BornSrc& src, const float pos[3], int a2, int a3, int t0, int arg3, bool allocOk, BornTable& out) {
    out.kindA = (int16_t)a2; out.kindB = (int16_t)a3; out.kindC = (int16_t)t0;
    for (int i = 0; i < 3; i++) out.pos[i] = pos[i];
    out.e.clear();
    for (int i = 0; i < src.n && i < 16; i++) {
        BornEntry e; e.item = src.item[i]; e.weight = (int)src.weight[i];     // func_14C518 (trunc)
        out.e.push_back(e);
    }
    out.tail = arg3;
    return allocOk;
}
}

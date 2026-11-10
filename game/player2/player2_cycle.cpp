#include "player2_cycle.h"
namespace vn {

// ELF 0x274AF0: {flags, modo, valor}; filas 0..5 (la fila 6 {0,12,0} y 7 {0,0,-1} no las usa startInit con kind <= 5).
const StartRow kStartRows6[kStartRows] = {
    {0x3D, 5, 75}, {0x19, 5, 150}, {0x08, 5, 75}, {0x3D, 5, 150}, {0x3C, 9, 75}, {0x3D, 9, 75}
};

bool PlayerCycle::sightVisible(const CycleSys& s, const PlayerRound& r) {
    return s.gunDevice != 2 && s.gunDevice != 3 && s.g738C == 0 && r.p.life > 0 && s.g7418 != 0 && s.gunSightValid;
}

BulletEnv PlayerCycle::bulletEnv(const PlayerRound& r, const CycleSys& s) {
    BulletEnv e;
    e.playerNo = r.playerNo; e.g739C = s.g739C; e.gameState = s.cfg.gameState; e.gameKind = s.cfg.gameKind;
    e.flags72D0 = s.flags72D0; e.itemSlot0 = s.cfg.itemSlot[0]; e.word502704 = s.word502704; e.g738C = s.g738C;
    e.g72F0[0] = s.g72F0[0]; e.g72F0[1] = s.g72F0[1];
    e.sightX = r.p.sightX; e.sightY = r.p.sightY; e.sightScale = s.sightScale;
    return e;
}

bool PlayerCycle::startInit(PlayerRound& r, CycleSys& s, CycleHost& h, int kind) {
    (void)h;
    if (kind < 0 || kind >= kStartRows) return false;
    PlayerState& p = r.p;
    PlayerLogic::setMaxLife(p, s.cfg);
    PlayerLogic::setInitLife(p, s.cfg, r.playerNo);
    PlayerLogic::setMaxBullet(p, s.cfg, r.playerNo);
    const StartRow& row = kStartRows6[kind];
    if (row.flags & 1) { p.life = p.initLife; p.lifePrev = p.initLife; }
    if (row.flags & 4) p.score = 0;
    if (row.flags & 0x10) s.credits500DF8++;
    if (row.flags & 0x20) s.credits500DFC++;
    r.dmgEff[0] = r.dmgEff[1] = r.dmgEff[2] = 0;
    s.invuln[r.playerNo & 1] = row.value;
    r.fieldB4 = 0;
    p.sightY = 0;                       // +0x20
    p.sightX = 0;                       // +0x1C
    p.bullet = p.maxBullet;             // +0x4C = +0x48
    p.emptyFlag = 0;                    // +0x50
    p.reloadReq = 0;                    // +0x5C
    if (s.cfg.gameKind == 1) for (int i = 0; i < 7; i++) r.shopFlags[i] = 0;
    HudLogic::bulletInit(r.hud);        // dispBulletInit
    p.flag58 = 0;                       // +0x58
    p.bulletFlag54 = 0;                 // +0x54
    BulletLogic::clear(r.bullets);      // 12 flags de fogonazo/estela a 0 y +0x1CC0 = 0
    p.mode = row.mode;                  // changePlayerMode(modo, 0)
    return true;
}

void PlayerCycle::init(PlayerRound& r, CycleHost& h) { h.dispatchModeInit(r.p.mode, r.playerNo); }

void PlayerCycle::firstGameInit(PlayerRound& r, CycleSys& s, CycleHost& h) {
    r.p.flags |= 1;
    h.setBookPlGameStart(r.playerNo);
    startInit(r, s, h, SK_FIRST);
    h.changeExec(P2Exec::GameLoop, r.playerNo);
}

void PlayerCycle::contGameInit(PlayerRound& r, CycleSys& s, CycleHost& h) {
    if (r.dmgEff[0] != 0) h.clrDamEff(r.playerNo);
    h.setBookPlGameEnd(r.playerNo);
    h.setBookPlGameCont(r.playerNo);
    PlayerLogic::addScore(r.p, s.cfg, r.playerNo, 1);        // AddPlayerScore(pl, 1)
    s.g73C6 = (int16_t)(s.g73C6 - 3);
    if (s.g73C6 < 0) s.g73C6 = 0;
    h.resetSightPosPl(r.playerNo);
    startInit(r, s, h, SK_CONT);
    h.changeExec(P2Exec::GameLoop, r.playerNo);
}

void PlayerCycle::stageGameInit(PlayerRound& r, CycleSys& s, CycleHost& h) {
    startInit(r, s, h, SK_STAGE);
    h.changeExec(P2Exec::GameLoop, r.playerNo);
}

void PlayerCycle::joinGameInit(PlayerRound& r, CycleSys& s, CycleHost& h) {
    r.p.flags |= 1;
    r.field60 = 0; r.p.shotsFired = 0; r.p.hits = 0; r.p.shotsTotal = 0; r.p.hitsTotal = 0;   // +0x60..+0x70
    h.setBookPlGameStart(r.playerNo);
    h.resetSightPosPl(r.playerNo);
    if (s.cfg.gameState == 0xA && s.cfg.gameKind == 1) { h.orgJoinIn(r.playerNo, 0); h.orgJoinIn(r.playerNo, 1); }
    startInit(r, s, h, SK_JOIN);
    h.changeExec(P2Exec::GameLoop, r.playerNo);
}

void PlayerCycle::onReload(PlayerRound& r) { HudLogic::bulletInit(r.hud); }

void PlayerCycle::onShot(PlayerRound& r, CycleSys& s, CycleHost& h, bool refill, RandFn rand) {
    BulletLogic::bulletConst(r.bullets, bulletEnv(r, s), h.bulletHost());
    h.sound(r.playerNo == 0 ? 0x2000C : 0x2000D, 0x40);
    if (s.screenMode == 2 && !refill) {
        int idx = r.p.bullet < 8 ? r.p.bullet : 8;            // icono de la bala recien gastada
        const BulletIcon& ic = r.hud.icon[idx];
        CartridgeIn ci; ci.playerNo = r.playerNo; ci.gameState = s.cfg.gameState; ci.gameKind = s.cfg.gameKind;
        ci.itemSlot0 = s.cfg.itemSlot[0]; ci.pauseLevel = s.pauseLevel;
        CartridgeLogic::born(r.cartridges, ci, ic.x, ic.y, ic.z, rand);
    }
    HudLogic::bulletMoveOn(r.hud, r.p.maxBullet, r.p.bullet);
}

void PlayerCycle::gameLoop(PlayerRound& r, CycleSys& s, CycleHost& h) {
    PlayerState& p = r.p;
    const int pl = r.playerNo;
    PlayerLogic::setMaxBullet(p, s.cfg, pl);
    if (sightVisible(s, r)) h.dispSight(pl);
    if (s.cfg.gameState == 9) p.life = 1;
    if (p.life <= 0) p.life = 0;
    if (!(r.flagsC8 & 1) && p.damageFlag != 0) h.dispDamEffReq(pl);
    if (s.pauseLevel < 4) {
        if (p.life != 0) {
            if (s.cfg.gameKind == 1 && !(s.flags72D0 & 0x10) && p.mode != 8) h.gunOrgExe(pl);
            else h.gunExe(pl);
            if (s.g7418 == 0 && s.g729C != 0 && s.gunTrigger && s.gunSightValid) s.g72A0 = 1;
        }
    } else if (s.cfg.gameState == 0xA && s.cfg.gameKind == 1) {
        h.gunOrgSoundOff(pl);
    }
    BulletLogic::bullet(r.bullets, bulletEnv(r, s), h.bulletHost());
    int st = s.cfg.gameState;
    if (st != 0xE && st != 9) {
        if (p.life == 0) {
            s.g7400 = 1; s.g73FC = 1;
            s.credits500DF8 = (int16_t)(s.credits500DF8 - 1);
            if (st == 0xA && s.cfg.gameKind == 1) h.gunOrgSoundOff(pl);
            p.mode = 4;                                        // changePlayerMode(4, 1)
            h.changeExec(P2Exec::Init, pl);
        } else {
            bool skipDraw = false;
            if (st == 0xA && s.g73D8 == 5 &&
                (s.g73D4 == 3 || s.g73F0 == 5 || s.g73F0 == 6 || (uint32_t)((int)s.g73F0 - 0xF) < 2u)) skipDraw = true;
            if (!skipDraw) {
                r.taskFlags27FC |= 8;
                h.calcLifeVelX(r);
                HudBulletIn in;
                in.playerNo = pl; in.maxBullet = p.maxBullet; in.bullet = p.bullet; in.emptyFlag = p.emptyFlag; in.flag58 = p.flag58;
                in.screenMode = s.screenMode; in.active7418 = s.g7418; in.gunTrigger = s.gunTrigger; in.gunSightValid = s.gunSightValid;
                in.pauseLevel = s.pauseLevel;
                HudBulletOut o = HudLogic::bulletLoop(r.hud, in);
                p.flag58 = in.flag58;
                if (o.sound) h.sound(o.sound, o.soundParam);
                h.drawBullets(pl, o);
                CartridgeLogic::loop(r.cartridges, s.pauseLevel);
                h.drawCartridges(pl, r.cartridges);
                h.drawLife(r);
                LifeParticleLogic::loop(r.lifeParticles, s.pauseLevel);
                h.drawLifeParticles(pl, r.lifeParticles);
                if (r.flagsC8 & 1) { if (r.dmgEff[0] != 0) h.clrDamEff(pl); }
                else h.dispDamEff(pl);
                p.lifePrev = p.life;
            }
        }
    }
    if (s.demoPlay) joinWaitLoopMain(r, s, h);
    h.scoreDispCheck(pl);
}

void PlayerCycle::joinWaitInit(PlayerRound& r, CycleSys& s, CycleHost& h) {
    if (s.cfg.gameState != 9) {
        h.changeExec(P2Exec::JoinWaitLoop, r.playerNo);
        joinWaitLoopMain(r, s, h);                         // joinWaitLoop llama a joinWaitLoop_main
    } else {
        demoInit(r, s, h);
    }
}

void PlayerCycle::joinWaitLoopMain(PlayerRound& r, CycleSys& s, CycleHost& h) {
    h.checkCredAndStBtn(r.playerNo, s.cfg.gameState == 0xA ? 3 : 0);
    bool a1 = (s.flags72D0 & 2) && s.cfg.gameState == 0xA;
    if (r.p.mode != 9) a1 = false;
    r.joinReqC9 = 0;
    if (a1 && (s.flags72D0 & 2)) { r.joinReqC9 = 1; r.taskFlags27FC |= 0x10; }
}

void PlayerCycle::waitInit(PlayerRound& r, CycleHost& h) { h.changeExec(P2Exec::WaitLoop, r.playerNo); }

void PlayerCycle::waitLoop(PlayerRound& r, CycleSys& s, CycleHost& h) {
    if (s.flags72D0 & 1) {
        r.p.mode = r.reqMode94;
        init(r, h);
        return;
    }
    if ((s.g739C & 0x20) && s.g738C == 0) r.taskFlags27FC |= 1;
}

void PlayerCycle::demoInit(PlayerRound& r, CycleSys& s, CycleHost& h) {
    startInit(r, s, h, SK_DEMO);
    h.changeExec(P2Exec::GameLoop, r.playerNo);
}

void PlayerCycle::commonLoop(PlayerRound& r, CycleHost& h) {
    r.taskFlags27FC = 0;
    h.entryPut(r.playerNo);
}
}

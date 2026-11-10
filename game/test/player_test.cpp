#include "../src/player.h"
#include <cstdio>
using namespace vn;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)
int main() {
    PlayerConfig c{}; c.gameState = 1; c.cfgInitLife = 5; c.cfgMaxLife = 7; c.bulletTable[2] = 12;
    PlayerState p; int16_t ca = 0, cb = 0;
    PlayerLogic::setMaxLife(p, c); CHECK(p.maxLife == 7);
    c.gameState = 0xA; c.gameKind = 1; PlayerLogic::setMaxLife(p, c); CHECK(p.maxLife == 9);
    c.itemSlot[0] = 2; c.itemSlot[2] = 31; c.itemSlot[3] = 32;           // 3 + 2 + 5 = 10, recortado a maxLife 9
    PlayerLogic::setInitLife(p, c, 0); CHECK(p.initLife == 9);
    c.itemSlot[2] = 30; c.itemSlot[3] = 0; PlayerLogic::setInitLife(p, c, 0); CHECK(p.initLife == 4);
    PlayerLogic::setMaxBullet(p, c, 0); CHECK(p.maxBullet == 12);
    c.bulletTable[2] = -5; PlayerLogic::setMaxBullet(p, c, 0); CHECK(p.maxBullet == 1);
    c.gameKind = 2; PlayerLogic::setMaxBullet(p, c, 0); CHECK(p.maxBullet == 8);
    c.gameState = 1; c.gameKind = 0;
    int next = PlayerLogic::startInit(p, c, 0, 0, ca, cb);
    CHECK(next == 5); CHECK(p.life == p.initLife); CHECK(ca == 1 && cb == 1);   // flags 61: vida, 0x10 y 0x20
    next = PlayerLogic::startInit(p, c, 0, 2, ca, cb); CHECK(ca == 1 && cb == 1); // fase nueva (flags 8): sin creditos
    p.life = 0; CHECK(PlayerLogic::lifeTick(p, c) == true);
    c.gameState = 9; CHECK(PlayerLogic::lifeTick(p, c) == false && p.life == 1);
    // gunExe: 8 balas, dispara hasta vaciar, luego soltar pide recarga
    PlayerState q; q.maxBullet = 8; q.bullet = 2; GunInput gi{100, 200, true, true}; GunGlobals gg{false, true, false};
    ShotEvent e = PlayerLogic::gunExe(q, gi, gg, 5); CHECK(e.fired && q.bullet == 1 && q.shotsFired == 1 && q.shotNow == 1 && q.sightX == 100);
    e = PlayerLogic::gunExe(q, gi, gg, 5); CHECK(e.fired && q.bullet == 0 && q.emptyFlag == 1);
    e = PlayerLogic::gunExe(q, gi, gg, 5); CHECK(!e.fired && q.bullet == 0);
    gi.trigger = false; PlayerLogic::gunExe(q, gi, gg, 5); CHECK(q.reloadReq == 1);
    gg.reloadCancel = true; PlayerLogic::gunExe(q, gi, gg, 5); CHECK(q.reloadReq == 0);
    gg.reloadCancel = false; PlayerLogic::gunExe(q, gi, gg, 8); CHECK(q.bullet == 8);
    // reload / puntuacion / dano
    PlayerConfig c2{}; c2.gameState = 0xA; c2.gameKind = 1; c2.itemSlot[0] = 10; c2.bulletTable[10] = 6;
    PlayerState r; r.maxBullet = 3; GunGlobals g2{false, true, false};
    ReloadEvent re = PlayerLogic::reloadExe(r, c2, g2, 0, 0); CHECK(r.bullet == 6 && re.sound == 0xB010F);
    c2.itemSlot[0] = 3; re = PlayerLogic::reloadExe(r, c2, g2, 0, 0); CHECK(re.sound == 0x20027);
    c2.itemSlot[0] = 10; re = PlayerLogic::reloadExe(r, c2, g2, 0, 1); CHECK(re.sound == 0);
    PlayerConfig c3{}; c3.gameState = 0xA; c3.gameKind = 1; c3.itemSlot[2] = 47; c3.itemSlot[3] = 48;
    PlayerState sc; PlayerLogic::addScore(sc, c3, 0, 100); CHECK(sc.score == 800);
    PlayerLogic::addScore(sc, c3, 0, -10000); CHECK(sc.score == 0);
    PlayerState h; h.shotsFired = 10; h.hits = 10; CHECK(PlayerLogic::hitBonus(h) == 8000); h.hits = 5; CHECK(PlayerLogic::hitBonus(h) == 2500); h.shotsFired = 0; CHECK(PlayerLogic::hitBonus(h) == 0);
    PlayerConfig c4{}; c4.gameState = 1; PlayerState d; d.mode = 5; d.life = 3; DamageGlobals dg;
    CHECK(PlayerLogic::decLife(d, c4, dg, 0, true, 7, nullptr) == 1); CHECK(d.life == 2 && dg.invuln == 0x4B && d.damageKind == 7 && dg.counter73BC == -2);
    CHECK(PlayerLogic::decLife(d, c4, dg, 0, true, 7, nullptr) == 0 && d.life == 2);          // invulnerable
    dg.camState = 1; dg.invuln = 0; CHECK(PlayerLogic::decLife(d, c4, dg, 0, false, 0, nullptr) == 0);   // camara no 2: no se puede
    d.maxLife = 3; d.life = 3; CHECK(PlayerLogic::incLife(d, c4, 0) == 0 && d.score == 1000);
    printf(fails ? "%d fallos\n" : "OK\n", fails); return fails != 0;
}

#include "player.h"
namespace vn {
// ELF 0x274AF0: {61,5,75} {25,5,150} {8,5,75} {61,5,150} (flags, modo, valor)
const StartInfo kStartInfo[4] = {{61, 5, 75}, {25, 5, 150}, {8, 5, 75}, {61, 5, 150}};

void PlayerLogic::setMaxLife(PlayerState& p, const PlayerConfig& c) {
    int v = c.cfgMaxLife;
    if (c.gameState == 0xA && (c.gameKind == 1 || c.gameKind == 2)) v = 9;
    p.maxLife = v;
}

void PlayerLogic::setInitLife(PlayerState& p, const PlayerConfig& c, int pl) {
    if (c.gameState == 0xA) {
        if (c.gameKind == 2) p.initLife = 3;
        else if (c.gameKind == 1) {
            p.initLife = 3;
            // objetos equipados 2..3 del jugador: 30 = +1, 31 = +2, 32 = +5 (el original acumula en cadena: case 31 suma 1 y luego 1 mas; case 32 suma 4 y luego 1)
            for (unsigned i = 2; i < 4; i++) {
                int8_t it = c.itemSlot[i]; (void)pl;
                if (it == 30) p.initLife += 1; else if (it == 31) p.initLife += 2; else if (it == 32) p.initLife += 5;
            }
        } else p.initLife = c.cfgInitLife;
    } else p.initLife = c.cfgInitLife;
    if (p.maxLife < p.initLife) p.initLife = p.maxLife;
}

void PlayerLogic::setMaxBullet(PlayerState& p, const PlayerConfig& c, int pl) {
    (void)pl;
    if (c.gameState == 0xA && c.gameKind != 2) {
        if (c.gameKind == 1) {
            p.maxBullet = c.bulletTable[(uint8_t)c.itemSlot[0]];
            if (p.maxBullet < 0) p.maxBullet = 1;
        }                                    // otros gameKind en estado 0xA: el original no toca maxBullet
    } else p.maxBullet = 8;
    if (p.maxBullet < p.bullet) p.bullet = p.maxBullet;
}

void PlayerLogic::setLifeStart(PlayerState& p) { p.life = p.initLife; p.lifePrev = p.initLife; }
void PlayerLogic::changePlayerMode(PlayerState& p, int mode) { p.mode = mode; }

int PlayerLogic::startInit(PlayerState& p, const PlayerConfig& c, int pl, int kind, int16_t& ca, int16_t& cb) {
    setMaxLife(p, c); setInitLife(p, c, pl); setMaxBullet(p, c, pl);
    const StartInfo& s = kStartInfo[kind & 3];
    if (s.flags & SF_SET_LIFE) { p.life = p.initLife; p.lifePrev = p.initLife; }
    if (s.flags & SF_CREDIT_A) ca++;
    if (s.flags & SF_CREDIT_B) cb++;
    p.bullet = p.maxBullet;                 // +0x4C = +0x48
    return s.mode;
}

ShotEvent PlayerLogic::gunExe(PlayerState& p, const GunInput& g, const GunGlobals& gl, int mode) {
    ShotEvent ev; bool refill = false;
    p.shotNow = 0; p.sightX = g.sightX; p.sightY = g.sightY;                 // getInput
    if (p.bulletFlag54 == 1) p.bulletFlag54 = 0;                              // (dispBulletInit lo hace el motor)
    if (gl.noAmmoCost || mode == 8) {                                         // balas infinitas: recarga completa cada fotograma
        refill = true; p.bullet = p.maxBullet; p.emptyFlag = 0; p.flag58 = 0; p.bulletFlag54 = 0;
    }
    if (g.active) {
        if (g.trigger && p.bullet != 0 && gl.fireEnabled) {
            if (!refill) { p.bullet--; p.shotsFired++; p.shotsTotal++; }
            if (p.bullet == 0) { p.emptyFlag = 1; p.flag58 = 0; }
            p.shotNow = 1; ev.fired = true;
        }
        if (g.trigger && p.bullet == 0 && !gl.noAmmoCost) ev.dryFire = true;  // gatillo con cargador vacio (el original no hace nada mas aqui)
        if (!g.trigger && p.bullet != p.maxBullet) p.reloadReq = 1;           // soltar el gatillo con balas gastadas pide recarga
    }
    if (gl.reloadCancel) p.reloadReq = 0;
    return ev;
}

ReloadEvent PlayerLogic::reloadExe(PlayerState& p, const PlayerConfig& c, const GunGlobals& gl, int pl, int pauseLevel) {
    (void)pl; ReloadEvent ev;
    p.reloadReq = 0; p.bullet = p.maxBullet;
    if (c.gameKind == 1) p.bullet = (int8_t)(c.bulletTable[(uint8_t)c.itemSlot[0]] & 0xFF);   // el original lee solo el byte bajo (lb)
    p.emptyFlag = 0; p.flag58 = 0; p.bulletFlag54 = 0;
    if (!gl.fireEnabled || gl.noAmmoCost || gl.reloadCancel || pauseLevel > 0) return ev;
    if (c.gameKind != 1) { ev.sound = 0x20027; return ev; }
    int idx = (int8_t)c.itemSlot[0] - 8;
    static const int snd[8] = {0xB0113, 0xB0113, 0xB010F, 0xB0115, 0xB0117, 0xB0119, 0xB011B, 0xB011D};
    ev.sound = ((unsigned)idx < 8) ? snd[idx] : 0x20027;
    return ev;
}

void PlayerLogic::addScore(PlayerState& p, const PlayerConfig& c, int pl, int16_t d) {
    (void)pl; int16_t a = d;
    if (c.gameState == 0xA && c.gameKind == 1)
        for (int i = 2; i < 4; i++) { if (c.itemSlot[i] == 47) a = (int16_t)(a * 2); else if (c.itemSlot[i] == 48) a = (int16_t)(a * 4); }
    p.score += a; if (p.score < 0) p.score = 0;
}

void PlayerLogic::addHitNum(PlayerState& p) { p.hits++; p.hitsTotal++; }

int PlayerLogic::hitBonus(const PlayerState& p) {
    static const int16_t tbl[11] = {0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000, 8000};
    if (p.shotsFired <= 0) return 0;
    unsigned v = (unsigned)(10.0f * ((float)p.hits / (float)p.shotsFired));
    return tbl[v > 10 ? 10 : v];
}

int PlayerLogic::canDecLife(const PlayerConfig& c, const PlayerState& p, const DamageGlobals& g, int pl) {
    if (g.camState != 2) return -2;
    if (pl != 0 && pl != 1) return -1;
    if (c.gameState != 0xA) return 0;
    return (p.mode == 4 || p.mode == 1 || p.mode == 5) ? 0 : -3;
}

int PlayerLogic::decLife(PlayerState& p, const PlayerConfig& c, DamageGlobals& g, int pl, bool hasKind, int kind, int (*rand)()) {
    if (canDecLife(c, p, g, pl) != 0 || g.invuln != 0 || c.gameState == 9) return 0;
    int range = p.life + 1; if (range >= 5) range = 5;
    bool saved = false;
    if (c.gameState == 0xA && c.gameKind == 1 && c.itemSlot[1] == 0x1B) saved = (rand() % range) == 0;   // objeto 0x1B: probabilidad de evitar el dano
    if (!saved) { p.life--; g.lifeLostFlag = 1; }
    if (g.mode73F4 != 2) g.counter73BC--;
    g.counter73BC--;
    addScore(p, c, pl, -100);
    if (saved) { p.damageFlag = 1; p.damageKind = 0xB; }
    else if (hasKind) { p.damageFlag = 1; p.damageKind = kind; }
    g.invuln = 0x4B;                                    // 75 fotogramas de invulnerabilidad
    if (g.camState != 2 && p.life <= 0) p.life = 1;     // fuera de la camara de juego no se puede morir
    return 1;
}

int PlayerLogic::incLife(PlayerState& p, const PlayerConfig& c, int pl) {
    if (p.life >= p.maxLife) { addScore(p, c, pl, 1000); return 0; }
    p.life++; return 1;
}

bool PlayerLogic::lifeTick(PlayerState& p, const PlayerConfig& c) {
    if (c.gameState == 9) p.life = 1;       // modo 9: vida fija
    if (p.life <= 0) { p.life = 0; return c.gameState != 0xE && c.gameState != 9; }
    return false;
}
}

#include "player2_hud.h"
#include <cstring>
namespace vn {

static float fbits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// ---------------- Iconos de balas ----------------
void HudLogic::layoutInit(BulletHud& h, int playerNo) {
    // PlayerTask::__ct__: x = -0.0145 (jugador 0) o +0.0145; y = (-0.0075 + 0.001*i) - 0.0001; z = -0.02
    for (int i = 0; i < kBulletIcons; i++) {
        h.layout[i].x = playerNo == 0 ? fbits(0xBC6D9168) : fbits(0x3C6D9168);
        h.layout[i].y = (fbits(0xBBF5C28F) + fbits(0x3A83126F) * (float)i) - fbits(0x38D1B717);
        h.layout[i].z = fbits(0xBCA3D70A);
    }
}

void HudLogic::bulletInit(BulletHud& h) {
    for (int i = 0; i < kBulletIcons; i++) {
        h.icon[i].x = h.layout[i].x; h.icon[i].y = h.layout[i].y; h.icon[i].z = h.layout[i].z;
        h.icon[i].moving = 0; h.icon[i].step = 0;
    }
}

void HudLogic::bulletMoveOn(BulletHud& h, int maxBullet, int bullet) {
    int mx = maxBullet < 9 ? maxBullet : 9;
    int shift = (bullet < mx) ? mx - bullet : 0;
    for (int i = 0; i < kBulletIcons; i++) {
        // El original indexa D_51F270[i + shift] sin acotar (hasta 16); mas alla de 8 lee memoria contigua. Aqui la fila se repite
        // cada 9 (los 9 valores de y son iguales para ambos jugadores); no verificado para el jugador 1 (lee fuera de la tabla).
        float ty = h.layout[(i + shift) % kBulletIcons].y;
        h.icon[i].vy = (ty - h.icon[i].y) / 4.0f;
        h.icon[i].moving = 1;
        h.icon[i].step = 0;
    }
}

HudBulletOut HudLogic::bulletLoop(BulletHud& h, HudBulletIn& in) {
    HudBulletOut o;
    int n = in.maxBullet < 9 ? in.maxBullet : 9;
    for (int i = 0; i < n; i++) {
        BulletIcon& ic = h.icon[i];
        if (ic.moving == 0) continue;
        ic.y += ic.vy;
        ic.step++;
        if (ic.step == 4) { ic.step = 0; ic.moving = 0; }
    }
    int cnt = in.bullet < 9 ? in.bullet : 9;
    if (in.screenMode == 2) {
        o.drawIcons = true; o.iconCount = cnt;
        if (in.bullet >= 10) {
            o.drawCount = true;
            o.tens = in.bullet / 10; o.ones = in.bullet % 10;
        }
    }
    if ((in.screenMode == 2 || in.screenMode == 1) && in.emptyFlag != 0 && in.active7418 != 0) {
        if (in.flag58 % 60 < 45) {
            o.reloadBlink = true;
            if (in.flag58 >= 100) o.reloadStrong = true;
        }
        if (in.flag58 != 0 && in.gunTrigger && in.gunSightValid && in.flag58 < 100) { o.sound = 0x20017; o.soundParam = 0x40; }
        if (in.pauseLevel <= 0) in.flag58++;
    }
    return o;
}

// ---------------- Cartuchos ----------------
void CartridgeLogic::init(CartridgePool& p) { p = CartridgePool(); }

void CartridgeLogic::born(CartridgePool& p, const CartridgeIn& in, float x, float y, float z, RandFn rand) {
    if (in.pauseLevel > 0) return;
    Cartridge& c = p.c[p.next];
    p.next++;
    if (p.next == kCartridges) p.next = 0;
    c.x = x; c.y = y; c.z = z;
    const float k = fbits(0x3727C5AC);              // 1.0e-5
    c.angle = rand() & 0x3FF;
    c.angleVel = (rand() + 0x100) & 0x3FF;
    c.vy = fbits(0x3951B717) + k * (float)(rand() % 20);   // 2.0e-4 + 1.0e-5*(0..19)
    c.vz = 0;
    int shotBurst = 0;
    bool shop = in.gameState == 0xA && in.gameKind == 1;
    if (shop) {
        int w = in.itemSlot0;
        shotBurst = (w == 13 || w == 11) ? 1 : 0;
    }
    float r = k * (float)(rand() % 20);
    if (in.playerNo == 0) c.vx = fbits(0xB727C5AC) - r;    // -1e-5 - 1e-5*r
    else                  c.vx = k + r;
    c.age = 0;
    if (shop && shotBurst == 1) { c.vx = c.vx * 6.0f; c.age = 0x5A; }
    c.active = 1;
    c.kind = 9;
}

void CartridgeLogic::loop(CartridgePool& p, int pauseLevel) {
    if (pauseLevel > 0) return;
    const float g = fbits(0xB7A7C5AC);              // -2.0e-5
    for (int i = 0; i < kCartridges; i++) {
        Cartridge& c = p.c[i];
        if (c.active == 0) continue;
        c.x += c.vx;
        c.y += c.vy;
        c.angle += c.angleVel;
        c.vy += g;
        c.age++;
        if (c.age >= 0x79) { c.age = 0; c.active = 0; }
    }
}

// ---------------- Particulas de vida ----------------
void LifeParticleLogic::init(LifeParticlePool& p) {
    for (int i = 0; i < kLifeParticles; i++) {
        LifeParticle& q = p.p[i];
        q.x = q.y = q.z = 0; q.active = 0; q.age = 0; q.spinA = 0; q.spinB = 0;   // el original solo borra +0,+4,+8,+0x18,+0x1C,+0x20,+0x24,+0x28 (+0x24 sin uso visto)
    }
    p.velX = 0;
}

bool LifeParticleLogic::born(LifeParticlePool& p, int pauseLevel, float x, float y, float z, RandFn rand) {
    if (pauseLevel > 0) return false;
    for (int i = 0; i < kLifeParticles; i++) {
        LifeParticle& q = p.p[i];
        if (q.active != 0) continue;
        q.x = x; q.y = y;
        const float j = fbits(0x3951B717);          // 2.0e-4
        if ((rand() % 10) < 5) q.x = q.x + j * (float)((rand() % 5) - 2);
        else                   q.y = q.y + j * (float)((rand() % 5) - 2);
        q.z = z;
        q.vx = p.velX;
        q.vy = fbits(0x3727C5AC) * (float)(-((rand() % 3) + 2));
        q.vz = 0;
        q.active = 1; q.age = 0; q.spinA = 0; q.spinB = 0;
        q.spinAVel = rand() & 0xFFF;
        q.spinBVel = rand() & 0xFFF;
        q.scale = fbits(0x3AAD03DA);                // 0.00132
        int r = rand();
        int m = r % 4;                              // resto de C (con correccion de signo en el original)
        q.scaleVel = fbits(0x37A7C5AC) * (float)(-(m + 1));
        return true;
    }
    return false;
}

void LifeParticleLogic::loop(LifeParticlePool& p, int pauseLevel) {
    if (pauseLevel > 0) return;
    for (int i = 0; i < kLifeParticles; i++) {
        LifeParticle& q = p.p[i];
        if (q.active == 0) continue;
        q.x += q.vx;
        q.y += q.vy;
        q.spinA += q.spinAVel;
        q.spinB += q.spinBVel;
        q.scale += q.scaleVel;
        q.age++;
        if (q.age >= 0x51) q.active = 0;
        if (q.scale <= 0.0f) q.active = 0;
    }
}

// ---------------- Dano y guardas ----------------
void DmgLogic::dmgEftPlayer(DmgShakeSys& s, Sin16Fn sin16) {
    if (s.hitP0 == 1) { s.hitP0 = 0; s.timer = 0x28; }
    if (s.hitP1 == 1) { s.hitP1 = 0; s.timer = 0x28; }
    s.timer -= 1;
    if (s.timer <= 0) s.timer = 0;
    if (s.timer != 0 && (s.flags72D0 & 0x10) == 0) {
        s.shakeY = 0;
        float f2 = 48.0f * (float)s.timer;
        float f1 = fbits(0x3CCCCCCD) * f2;          // 0.025
        s.shakeX = (int32_t)(f1 * sin16(s.timer * 0x1800));
    } else {
        s.shakeX = 0; s.shakeY = 0;
    }
}

bool DmgLogic::postExec(DmgShakeSys& s, Sin16Fn sin16, int16_t w, int word500734, int32_t& g73F4) {
    dmgEftPlayer(s, sin16);
    if (w == 2) { g73F4 = 2; return true; }
    if (w == 1) { g73F4 = (word500734 == 5 || word500734 == 7) ? 0 : 1; return true; }
    if (w == 0) { g73F4 = -1; return true; }
    return false;
}

void DmgLogic::preExec(uint32_t& f, int8_t camState, int8_t g73D8, int16_t gameState) {
    if (camState == 1 && g73D8 != 7 && gameState != 9 && (f & 0x24) == 0) f &= ~1u;
    else f |= 1u;
}
}

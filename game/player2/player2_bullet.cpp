#include "player2_bullet.h"
#include <cmath>
#include <cstring>
namespace vn {

static float bitsToFloat(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

MuzzleStyle muzzleStyleFor(int weapon) {
    // jtbl_002C2E20 (ELF): 0x20FF68,0x20FF68,0x20FF0C,0x20FEB0,0x210148,0x20FEB0,0x210214,0x210374
    static const MuzzleStyle tbl[8] = {MS_W8_9, MS_W8_9, MS_W10, MS_DEFAULT, MS_W12, MS_DEFAULT, MS_W14, MS_W15};
    unsigned i = (unsigned)(weapon - 8);
    return i < 8 ? tbl[i] : MS_DEFAULT;
}

int BulletLogic::effectiveWeapon(const BulletEnv& e) {
    int s3 = 0;
    if (e.gameState == 0xA && e.gameKind == 1 && (e.flags72D0 & 0x10) == 0) {
        s3 = e.itemSlot0;
        if (s3 == 8 && e.word502704 < 5) s3 = 0;
    }
    return s3;
}

void BulletLogic::bulletConst(BulletPool& b, const BulletEnv& e, BulletHost& h) {
    int s2 = e.g739C % 4;                    // resto de C (el original corrige el signo a mano)
    int s3 = effectiveWeapon(e);
    int idx = b.ring;
    MuzzleSlot& a = b.muzzle[idx];
    a.active = 1; a.age = 0;
    a.x = e.sightX / e.sightScale;
    a.y = e.sightY / e.sightScale;
    a.z = kMuzzleDepth;
    const float c = bitsToFloat(kMuzzleDepthScaleBits);
    float f3 = a.z;
    float f2 = a.x / f3, f1 = a.y / f3;      // dividir por -1: cambia el signo
    a.z = c;
    a.x = f2 * c;
    a.y = f1 * c;
    h.getAngleYX(a.x, a.y, a.z, a.angA, a.angB);
    a.angB += 0x8000;
    a.angA = -a.angA;
    a.weapon = s3; a.phase = s2;

    TracerSlot& t = b.tracer[idx];
    t.active = 1; t.age = 0;
    float in[3] = {e.sightX / e.sightScale, e.sightY / e.sightScale, kMuzzleDepth};
    float world[3], cam[3];
    h.cameraRay(in, world, cam);
    t.x = world[0]; t.y = world[1]; t.z = world[2];
    float d[3] = {t.x - cam[0], t.y - cam[1], t.z - cam[2]};
    float len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);     // func_001AFA3C: normaliza
    float inv = 1.0f / len;
    t.vx = d[0] * inv * kTracerSpeed;
    t.vy = d[1] * inv * kTracerSpeed;
    t.vz = d[2] * inv * kTracerSpeed;
    t.weapon = s3; t.phase = s2;

    b.ring = (int16_t)(b.ring + 1);
    if (b.ring >= kBulletRing) b.ring = 0;
}

void BulletLogic::bullet(BulletPool& b, const BulletEnv& e, BulletHost& h) {
    const int pl = e.playerNo & 1;
    for (int i = 0; i < kBulletRing; i++) {
        MuzzleSlot& a = b.muzzle[i];
        if (!(a.age < kMuzzleLife && e.g738C == 0)) a.active = 0;
        if (a.active) h.drawMuzzle(muzzleStyleFor(a.weapon), e.playerNo, i, a);

        TracerSlot& t = b.tracer[i];
        bool kill = t.age >= kTracerLife || (t.age == 1 && e.g72F0[pl] != 0) || e.g738C != 0;
        if (kill) t.active = 0;
        if (t.active) {
            if (t.age == 1) { t.x += kTracerFirstFrameBoost * t.vx; t.y += kTracerFirstFrameBoost * t.vy; t.z += kTracerFirstFrameBoost * t.vz; }
            else { t.x += t.vx; t.y += t.vy; t.z += t.vz; }
            t.spin += kTracerSpin;
            if (t.age > 0) h.drawTracer(e.playerNo, i, t);
        }
        a.age++;
        t.age++;
    }
}

void BulletLogic::clear(BulletPool& b) {
    for (int i = 0; i < kBulletRing; i++) { b.muzzle[i].active = 0; b.tracer[i].active = 0; }
    b.ring = 0;
}
}

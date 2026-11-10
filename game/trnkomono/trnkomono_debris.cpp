#include "trnkomono_debris.h"
#include <cmath>
#include <cstring>
namespace vn {
static float bfd(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

void debrisStep(DebrisPiece& p, bool paused) {
    if (paused) return;
    if (p.pos[1] < p.floorY) {
        float vy = p.vel[1];
        p.vel[1] = vy - bfd(0x3F666666u) * vy;            // 0.9
        p.vel[0] = bfd(0x3F19999Au) * p.vel[0];            // 0.6
        p.vel[2] = bfd(0x3F19999Au) * p.vel[2];
        p.pos[1] = p.floorY;
        p.rotVel[0] = p.rotVel[1] = p.rotVel[2] = 0;
    }
    p.vel[1] = p.vel[1] + p.grav;
    p.pos[0] += p.vel[0];
    p.pos[1] += p.vel[1];
    p.pos[2] += p.vel[2];
    p.rot[0] = (p.rot[0] + p.rotVel[0]) & 0xFFFF;
    p.rot[1] = (p.rot[1] + p.rotVel[1]) & 0xFFFF;
    p.rot[2] = (p.rot[2] + p.rotVel[2]) & 0xFFFF;
}

void CrosetKoware::initPiece(DebrisPiece& p, const float base[3], float floorY, const TrnRand& r) {
    float f01 = bfd(0x3DCCCCCDu);                         // 0.1
    p.vel[0] = base[0] + f01 * (float)((r.i() % 10) - 5);
    p.vel[1] = base[1] + f01 * (float)(r.i() % 10);
    p.vel[2] = base[2] + f01 * (float)((r.i() % 10) - 5);
    p.grav = bfd(0xBD23D70Au);
    p.rotVel[0] = (r.i() & 0x3FF) - 0x1FF;
    p.rotVel[1] = (r.i() & 0x3FF) - 0x1FF;
    p.rotVel[2] = (r.i() & 0x3FF) - 0x1FF;
    p.floorY = floorY;
}
bool CrosetKoware::ctrl(DebrisPiece* pieces, int n, bool paused) {
    for (int i = 0; i < n; i++) debrisStep(pieces[i], paused);
    if (!paused) frames++;
    if (frames >= 0x3D) alpha = alpha - bfd(0x3D4CCCCDu);   // 0.05
    return frames >= 0xE11;
}

const DishRow* DishKoware::findRow(uint32_t obj) {
    for (int i = 0; i < 16; i++) if (kDishRows[i].id == obj) return &kDishRows[i];
    return nullptr;
}
void DishKoware::setup(uint32_t objId) {
    obj = objId; alpha = 1.0f; scale84 = 1.0f; frames = 0;
    if (const DishRow* r = findRow(objId)) count = r->count;
    if (objId == 0x1B0079) { scale84 = bfd(0x3DCCCCCDu); mult = 3.0f; }
    else mult = 2.0f;
}
void DishKoware::initPiece(DebrisPiece& p, const TrnRand& r, const TrnTrig& t) const {
    float f21 = (obj == 0x1B0079) ? 0.3f : 0.5f;
    int a = (r.i() & 0xFFFF) * 2;
    int b = (r.i() & 0xFFFF) * 2;
    p.vel[0] = (f21 * t.axisY(a)) * t.axisX(b);
    float vy = mult * ((f21 * t.axisY(a)) * t.axisY(b));
    p.vel[1] = vy;
    if (vy < 0.0f) p.vel[1] = -vy;
    p.vel[2] = f21 * t.axisX(a);
    p.grav = bfd(0xBD4CCCCDu);                            // -0.05
    p.rotVel[0] = (r.i() & 0x3FF) - 0x1FF;
    p.rotVel[1] = (r.i() & 0x3FF) - 0x1FF;
    p.rotVel[2] = (r.i() & 0x3FF) - 0x1FF;
    p.floorY = 0.0f;
}
bool DishKoware::ctrl(DebrisPiece* pieces, int n, bool paused) {
    for (int i = 0; i < n; i++) debrisStep(pieces[i], paused);
    if (!paused) frames++;
    bool close = false;
    if (obj == 0x1B0079) { if (frames >= 0x29) alpha -= bfd(0x3D4CCCCDu); if (frames >= 0x3D) close = true; }
    else                 { if (frames >= 0x15) alpha -= bfd(0x3D4CCCCDu); if (frames >= 0x29) close = true; }
    return close;
}
}

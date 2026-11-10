// Fragmentos de rotura de las clases Komono*Koware* y KomonoKusariHahen: ver komono.h.
#pragma STDC FP_CONTRACT OFF   // la PS2 no tiene FMA: sin contraccion de a*b+c
#include "komono.h"
namespace vn { namespace komono {

// Cuerpo comun de ctrl__20KomonoDishKowareTask / ctrl__19KomonoBoxKowareTask / ctrl__22KomonoCrosetKowareTask (por trozo).
void DebrisLogic::stepKoware(Debris& d, float floorY) {
    if (d.pos[1] < floorY) {
        d.vel[1] = d.vel[1] - (0.9f * d.vel[1]);
        d.vel[0] = 0.6f * d.vel[0];
        d.vel[2] = 0.6f * d.vel[2];
        d.pos[1] = floorY;
        d.rotVel[0] = 0; d.rotVel[1] = 0; d.rotVel[2] = 0;
    }
    d.vel[1] = d.vel[1] + d.grav;
    d.pos[0] = d.pos[0] + d.vel[0];
    d.pos[1] = d.pos[1] + d.vel[1];
    d.pos[2] = d.pos[2] + d.vel[2];
    d.rot[0] = (d.rot[0] + d.rotVel[0]) & 0xFFFF;
    d.rot[1] = (d.rot[1] + d.rotVel[1]) & 0xFFFF;
    d.rot[2] = (d.rot[2] + d.rotVel[2]) & 0xFFFF;
}

// Cuerpo de loop__21KomonoKusariHahenTaskFv (por trozo).
void DebrisLogic::stepKusari(Debris& d, float floorY) {
    if (d.pos[1] < floorY) {
        d.vel[1] = d.vel[1] - (0.5f * d.vel[1]);
        d.vel[0] = 0.5f * d.vel[0];
        d.vel[2] = 0.5f * d.vel[2];
        d.pos[1] = floorY;
        d.rotVel[0] = 0; d.rotVel[1] = 0;
    }
    d.rot[0] = (d.rot[0] + d.rotVel[0]) & 0xFFFF;
    d.rot[1] = (d.rot[1] + d.rotVel[1]) & 0xFFFF;
    d.vel[1] = d.vel[1] + d.grav;
    d.pos[0] = d.pos[0] + d.vel[0];
    d.pos[1] = d.pos[1] + d.vel[1];
    d.pos[2] = d.pos[2] + d.vel[2];
}

// ctrl__20KomonoDishKowareTaskFv: suelo y = 0. Vida: 0x1B0079 -> desvanece desde el fotograma 41 y cierra en 61; resto -> 21 y 41.
void DebrisTasks::dishCtrl(DishState& s, const KomonoEnv& env, KomonoHost& h) {
    for (int k = 0; k < s.count; k++)
        if (env.pauseLevel <= 0) DebrisLogic::stepKoware(s.pieces[k], 0.0f);
    if (env.pauseLevel <= 0) s.frame++;
    if (s.typeId == 0x1B0079) {
        if (s.frame >= 0x29) s.alpha = s.alpha - 0.05f;
        if (s.frame >= 0x3D) h.closeTask();
    } else {
        if (s.frame >= 0x15) s.alpha = s.alpha - 0.05f;
        if (s.frame >= 0x29) h.closeTask();
    }
    // (si alpha > 0: dibujo de los trozos; omitido)
}

// ctrl__19KomonoBoxKowareTaskFv: suelo en +0xC4; desvanece desde el fotograma 31, cierra en 51.
void DebrisTasks::boxCtrl(BoxState& s, const KomonoEnv& env, KomonoHost& h) {
    if (env.pauseLevel <= 0)
        for (int k = 0; k < s.count; k++) DebrisLogic::stepKoware(s.pieces[k], s.floorY);
    if (env.pauseLevel <= 0) s.frame++;
    if (s.frame >= 0x1F) s.alpha = s.alpha - 0.05f;
    if (s.frame >= 0x33) h.closeTask();
}

// ctrl__22KomonoCrosetKowareTaskFv: suelo propio por trozo (+0x3C); desvanece desde el fotograma 61 y cierra cuando alpha < 0.
void DebrisTasks::crosetCtrl(CrosetState& s, const KomonoEnv& env, KomonoHost& h) {
    for (int k = 0; k < s.count; k++)
        if (env.pauseLevel <= 0) DebrisLogic::stepKoware(s.pieces[k], s.pieces[k].floorY);
    if (env.pauseLevel <= 0) s.frame++;
    if (s.frame >= 0x3D) s.alpha = s.alpha - 0.05f;
    if (s.alpha < 0.0f) h.closeTask();
}

// __ct__21KomonoKusariHahenTaskFffff (0x9F1E20): 3 trozos con direccion y giros aleatorios (orden de nlRand identico al original).
void DebrisTasks::kusariInit(KusariState& s, float x, float y, float z, float floorY, KomonoHost& h) {
    s.alpha = 1.0f;                                     // 0x3F800000
    s.floorY = floorY;                                  // +0x104
    s.frame = 0;                                        // +0x100
    for (int i = 0; i < 3; i++) {
        Debris& d = s.pieces[i];
        float speed = 0.4f + 0.06f * (float)(h.rand() % 10);   // 0x3ECCCCCD + 0x3D75C28F * (rand % 10)
        int a1 = (h.rand() & 0xFFFF) * 2;
        int a2 = (h.rand() & 0xFFFF) * 2;
        d.pos[0] = x; d.pos[1] = y; d.pos[2] = z;
        float t = speed * h.angA(a1);
        d.vel[0] = t * h.angB(a2);
        t = speed * h.angA(a1);
        d.vel[1] = t * h.angA(a2);
        if (d.vel[1] < 0.0f) d.vel[1] = -d.vel[1];
        d.vel[1] = d.vel[1] + 0.2f;                     // 0x3E4CCCCD
        d.vel[2] = speed * h.angB(a1);
        d.grav = -0.04f - 0.004f * (float)(h.rand() % 10);
        d.rot[0] = (h.rand() & 0xFFFF) * 2;
        d.rot[1] = (h.rand() & 0xFFFF) * 2;
        d.rot[2] = 0;
        d.rotVel[0] = (h.rand() & 0xFFF) * 2;
        d.rotVel[1] = (h.rand() & 0xFFF) * 2;
        d.rotVel[2] = 0;
    }
}

// loop__21KomonoKusariHahenTaskFv: 3 trozos; desvanece (1/30) desde el fotograma 21; cierra en 51.
void DebrisTasks::kusariLoop(KusariState& s, const KomonoEnv& env, KomonoHost& h) {
    for (int k = 0; k < 3; k++)
        if (env.pauseLevel <= 0) DebrisLogic::stepKusari(s.pieces[k], s.floorY);
    if (env.pauseLevel <= 0) {
        s.frame++;
        if (s.frame >= 0x15) s.alpha = s.alpha - 0.033333335f;
    }
    if (s.frame >= 0x33) h.closeTask();
}
}}  // namespace vn::komono

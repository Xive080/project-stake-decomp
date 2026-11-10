// Clases TrnKomono* del ELF principal: CTrnBoxPoint, CTrnKomonoBase, TrnKomonoTumiageColliTask, TrnKomonoTask.
#include "trnkomono.h"
#include "../stages_rest/komono_rest.h"
namespace vn {

// ---- CTrnBoxPoint --------------------------------------------------------------------------------------------------
// init__12CTrnBoxPointFv (0x21AE40): pone a cero contador de enlaces, v18 y pos/pos0.
void TrnBoxPoint::init() {
    linkCount = 0;
    for (int i = 0; i < 3; i++) { v18[i] = 0; pos0[i] = 0; pos[i] = 0; }
}
// initPos__12CTrnBoxPointFfff (0x21AE70): pos = pos0 = (x,y,z).
void TrnBoxPoint::initPos(float x, float y, float z) {
    pos[0] = x; pos[1] = y; pos[2] = z;
    pos0[0] = x; pos0[1] = y; pos0[2] = z;
}
// setLink__12CTrnBoxPointFP12CTrnBoxPoint (0x21AEA0): d = sqrt(dx^2+dy^2+dz^2); cada punto guarda al otro y la distancia en su siguiente ranura.
// El original no acota linkCount (el array tiene 0x20 ranuras: +0x24..+0xA3 distancias, +0xA4..+0x123 punteros).
void TrnBoxPoint::setLink(TrnBoxPoint& o, float (*sqrtFn)(float)) {
    float dy = pos[1] - o.pos[1];
    float dx = pos[0] - o.pos[0];
    float dz = pos[2] - o.pos[2];
    float d = sqrtFn(dx * dx + dy * dy + dz * dz);
    if (linkCount < 32)   { link[linkCount] = &o; linkDist[linkCount] = d; linkCount++; }
    if (o.linkCount < 32) { o.link[o.linkCount] = this; o.linkDist[o.linkCount] = d; o.linkCount++; }
}

// ---- CTrnKomonoBase ------------------------------------------------------------------------------------------------
// initEndCP__14CTrnKomonoBaseFPii (0x21AF60): n = count; para cada par: id[i] = src[2i]; frame[i] = (float)src[2i+1].
void TrnKomonoBase::initEndCP(const int* src, int count) {
    n = count;
    for (int i = 0; i < count && i < 5; i++) { id[i] = src[2 * i]; frame[i] = (float)src[2 * i + 1]; }
}
// checkEndCP__14CTrnKomonoBaseFv (0x21B170): recorre n entradas; la primera con ruta == id decide: fotograma de camara >= tabla -> true, si no se sigue.
bool TrnKomonoBase::checkEndCP(const CamPathNow& cam) const {
    for (int i = 0; i < n && i < 5; i++)
        if (cam.pathId == id[i] && !(cam.frame < frame[i])) return true;
    return false;
}

// ---- TrnKomonoTumiageColliTask -------------------------------------------------------------------------------------
void TrnTumiageColli::ctor() {
    *this = TrnTumiageColli();
    c.flags = 1u | 0x10000u;
    c.radius = 8.0f;           // 0x41000000
    c.closeFlag = 0;
    enClActive = 0;
}
void TrnTumiageColli::setEnClName(int name) {
    c.flags |= 1u; c.flags |= 0x10u; c.flags |= 0x40u; c.flags &= 0xFFFEFFFFu;
    enClName = name;
    enClActive = 1;
}

// ---- TrnKomonoTask -------------------------------------------------------------------------------------------------
int TrnKomonoLogic::secondObj(int id) {
    switch ((uint32_t)id) {
        case 0x4700F6: return 0x4700F7;
        case 0x4700D6: return 0x4700D7;
        case 0x410051: return 0x410052;
        case 0x4700B3: return 0x4700B4;
        case 0x40003A: return 0x40003B;
        case 0x430013: return 0x430014;
        case 0x3C003C: return 0x3C003D;
        case 0x3C001C: return 0x3C001D;
        default: return 0;
    }
}
const TrnKomonoRadius* TrnKomonoLogic::findRadius(int id) {
    for (int i = 0; i < kTrnKomonoRadiusN; i++) if ((int)kTrnKomonoRadius[i].id == id) return &kTrnKomonoRadius[i];
    return nullptr;
}
const TrnKomonoBreak* TrnKomonoLogic::findBreak(int id) {
    for (int i = 0; i < kTrnKomonoBreakN; i++) if ((int)kTrnKomonoBreak[i].id == id) return &kTrnKomonoBreak[i];
    return nullptr;
}
void TrnKomonoLogic::init(TrnKomonoTask& t) {
    t.objId2 = secondObj(t.objId);
    t.flags = 1u | 0x10000u;
    t.hasCollision = 0;
    t.radius = 8.0f;           // D_002C3264 (valor por defecto = primera fila de la tabla)
    t.yOffset = 4.0f;          // D_002C3268
    if (const TrnKomonoRadius* r = findRadius(t.objId)) { t.radius = r->radius; t.yOffset = r->yOff; t.hasCollision = r->collide; }
}
void TrnKomonoLogic::ctorCounter(const TrnKomonoSys& s, int8_t& norm) {
    if (s.gameKind == 2 && s.missionKind == 6 && s.mission == 5) norm = (int8_t)(norm + 1);
}
void TrnKomonoLogic::dtorCounter(const TrnKomonoSys& s, int8_t& norm) {
    if (s.gameKind == 2 && s.missionKind == 6 && s.mission == 5) { norm = (int8_t)(norm - 1); if (norm < 0) norm = 0; }
}
void TrnKomonoLogic::ctrl(const TrnKomonoTask& t, const CamPathNow& cam, TrnKomonoOut& o) {
    if (t.endCP.checkEndCP(cam)) o.add(TrnKomonoEvent::CLOSE_TASK);
}
void TrnKomonoLogic::ctrlPost(TrnKomonoTask& t, const TrnKomonoSys& s, RandFn rand, TrnKomonoOut& o) {
    int pl = 0;
    if (k3CheckHit(t.flags, rand, pl)) {
        o.add(TrnKomonoEvent::ADD_SCORE, pl, 10);       // AddPlayerScore(pl, 10)
        o.add(TrnKomonoEvent::ADD_HIT, pl);             // AddPlayerHitNum(pl)
        close(t, s, o);
    }
    if (t.hasCollision != 0) o.add(TrnKomonoEvent::COLLISION_UPDATE, 0, 0, t.pos[1] + t.yOffset);
}
void TrnKomonoLogic::close(const TrnKomonoTask& t, const TrnKomonoSys& s, TrnKomonoOut& o) {
    if (const TrnKomonoBreak* b = findBreak(t.objId)) {
        if (b->kind != 0) o.add(TrnKomonoEvent::SPAWN_DYNAMICS, b->model);
        else o.add(TrnKomonoEvent::SPAWN_DISH, t.objId, (s.gameState == 0xA && s.gameKind == 2) ? 1 : 0);
        if (b->fx != 0) o.add(TrnKomonoEvent::SPAWN_FCV, b->fx, 0, b->fxScale);
        if (b->sound != -1) o.add(TrnKomonoEvent::SOUND, b->sound, 0x40);
    }
    switch ((uint32_t)t.objId) {
        case 0x4700F6: o.add(TrnKomonoEvent::SPAWN_FCV, 8, 0, 3.0f); break;
        case 0x3C001C: o.add(TrnKomonoEvent::SPAWN_FCV_AT, 3); break;
        case 0x3C003C: o.add(TrnKomonoEvent::SPAWN_FCV_AT, 4); break;
        case 0x3C004F: o.add(TrnKomonoEvent::SPAWN_FCV_AT, 5); break;
        case 0x40003A: o.add(TrnKomonoEvent::SPAWN_FCV, 6, 0, 2.0f); break;
        case 0x4700D6: o.add(TrnKomonoEvent::SPAWN_FCV, 7, 0, 2.0f); break;
        default: break;
    }
    o.add(TrnKomonoEvent::CLOSE_TASK);
}
}

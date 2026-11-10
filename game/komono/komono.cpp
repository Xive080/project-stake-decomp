// Ver komono.h. Cada funcion cita la original de asm/aomod/aomod_text.s.
#pragma STDC FP_CONTRACT OFF   // la PS2 no tiene FMA: sin contraccion de a*b+c
#include "komono.h"
#include <cstring>
namespace vn { namespace komono {

static float bitsToFloat(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
static uint32_t floatToBits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }

float KomonoEndCP::limit(int k) const { return bitsToFloat(w[6 + k]); }

// ---------------------------------------------------------------------------------------------------------------------
// CKomonoBase
// initEndCP__11CKomonoBaseFPii (0x9DF2B0). El original desenrolla el bucle de 8 en 8; el resultado final es identico al bucle simple
// (id k -> palabra 1+k, limite (float)valor -> palabra 6+k; con n > 5 los ids pisan los primeros limites, igual que el original).
void CKomonoBase::initEndCP(KomonoEndCP& cp, const int32_t* pairs, int n) {
    cp.w[0] = (uint32_t)n;
    if (n > 18) n = 18;                       // proteccion: el original no acota; el hueco real del objeto es de 4 pares
    for (int k = 0; k < n; k++) {
        cp.w[1 + k] = (uint32_t)pairs[2 * k];
        cp.w[6 + k] = floatToBits((float)pairs[2 * k + 1]);
    }
}

// checkEndCP__11CKomonoBaseFv (0x9DF0F0)
int CKomonoBase::checkEndCP(const KomonoEndCP& cp, const KomonoEnv& env) {
    for (int k = 0; k < cp.count() && k < 18; k++) {
        if (env.camPathId == (int16_t)cp.id(k)) {               // lh 0xD0(camara) == id
            if (!(env.camFrame < cp.limit(k))) return 1;        // !(frame < limite)
        }
    }
    return 0;
}

// bornItem__11CKomonoBaseF11tagNLpoint3iiii (0x9DF170)
void CKomonoBase::bornItem(const KomonoEndCP& cp, const float pos[3], int rotX, int rotY, int rotZ, int itemKind, KomonoHost& h) {
    ItemSpawn s;
    std::memset(&s, 0, sizeof s);
    s.rot[0] = (int16_t)rotX; s.rot[1] = (int16_t)rotY; s.rot[2] = (int16_t)rotZ;   // sh a2,a3,t0 -> +2,+4,+6
    for (int k = 0; k < 3; k++) s.pos[k] = pos[k];
    int n = cp.count();
    if (n > 8) n = 8;
    for (int k = 0; k < n; k++) {
        s.cpId[k] = cp.id(k);                                   // +0x14 + 8k = lista[k].id
        s.cpLimit[k] = (int32_t)cp.limit(k);                    // func_14C518 = fptosi (trunca)
    }
    s.count = cp.count();
    s.itemKind = itemKind;
    h.spawnItem(s);
}

// Decodificacion del impacto (ver komono.h).
int komonoHitPlayer(uint32_t& flags, KomonoHost& h, int dflt, bool consume) {
    if ((flags & 2) && (flags & 4)) {
        int v = h.rand();
        int p = v & 1;
        if (v < 0 && p != 0) p -= 2;                            // % 2 con signo de C
        return p;
    }
    if (flags & 2) { if (consume) flags &= ~2u; return 0; }
    if (flags & 4) { if (consume) flags &= ~4u; return 1; }
    return dflt;
}

// ---------------------------------------------------------------------------------------------------------------------
// KomonoTumiageColliTask
void ColliLogic::init(ColliState& s, const float tablePos[3]) {
    s.flags = 1;                                                // sw 1 -> +0xC
    s.flags |= 0x10000;
    for (int k = 0; k < 3; k++) s.pos[k] = tablePos[k];
    s.radius = 8.0f;                                            // 0x41000000
    s.closeReq = 0;                                             // +0x144
    for (int k = 0; k < 6; k++) s.zeros[k] = 0;                 // +0x38..+0x4C
    for (int k = 0; k < 3; k++) s.rot[k] = 0;                   // +0x50..+0x58 (el constructor los pisa a 0)
}
int  ColliLogic::chkColli(const ColliState& s) { return (s.flags & 8) ? 1 : 0; }
int  ColliLogic::getHitPl(ColliState& s, KomonoHost& h) { return komonoHitPlayer(s.flags, h, 0, true); }
void ColliLogic::clrColli(ColliState& s) { s.flags &= ~8u; }
void ColliLogic::close(ColliState& s) { s.closeReq = 1; }
void ColliLogic::setPos(ColliState& s, float x, float y, float z) { s.pos[0] = x; s.pos[1] = y; s.pos[2] = z; }
void ColliLogic::setRadius(ColliState& s, float r) { s.radius = r; }
void ColliLogic::ctrlPre(const ColliState& s, KomonoHost& h) {
    if (s.closeReq != 0) h.closeTask();                          // func_1A2630
    float out[3];
    h.registerCollider(s.pos, out);                              // func_1AFC00(+0x2C, +0x5C) + func_92CD90
}

// ---------------------------------------------------------------------------------------------------------------------
// KomonoTask
// Tabla de init__10KomonoTaskFP11ENEMY_TABLE (0x9E9D40): id -> id alternativo.
int KomonoLogic::altType(int id) {
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

void KomonoLogic::init(KomonoState& s, const float tablePos[3], const uint16_t tableRot[3], int typeId, const int32_t* cpPairs, int cpCount, int itemKind) {
    s.typeId = typeId;                                           // +0x16C
    s.itemKind = itemKind;                                       // +0x180
    CKomonoBase::initEndCP(s.endCp, cpPairs, cpCount);           // +0x140
    s.typeAlt = altType(typeId);                                 // +0x170
    s.flags = 1 | 0x10000;                                       // +0xC
    s.collide = 0;                                               // +0x17C
    s.radius = kKomonoTypeTable[0].radius;                       // por defecto 8.0 (D_00A093F4)
    s.yOffset = kKomonoTypeTable[0].yOffset;                     // por defecto 4.0 (D_00A093F8)
    for (int k = 0; k < 45; k++) {                               // busca el id en D_00A093F0; sin coincidencia quedan los valores por defecto
        if (kKomonoTypeTable[k].id == typeId) {
            s.radius = kKomonoTypeTable[k].radius;
            s.yOffset = kKomonoTypeTable[k].yOffset;
            s.collide = kKomonoTypeTable[k].collide;
            break;
        }
    }
    for (int k = 0; k < 3; k++) s.pos[k] = tablePos[k];          // +0x2C..+0x34
    for (int k = 0; k < 3; k++) s.rot[k] = tableRot[k];          // +0x50..+0x58 (lhu)
}

// ctrl__10KomonoTaskFv (0x9E9F80): cierra al llegar a la camara de fin; en entrenamiento muestra el efecto de gafas.
void KomonoLogic::ctrl(KomonoState& s, const KomonoEnv& env, KomonoHost& h) {
    if (CKomonoBase::checkEndCP(s.endCp, env)) h.closeTask();
    // (func_1B0808/055C/03EC/027C + func_214A00: dibujo de typeId y typeAlt; omitido)
    if (env.gameState == 0xA && env.gameKind == 1 && s.itemKind >= 0x15) {
        float p[3] = {s.pos[0], s.pos[1] + 4.0f, s.pos[2]};
        h.goggleEffect(p);                                       // OrgDispSenriganGoggleEffect
    }
}

// ctrlPost__10KomonoTaskFv (0x9EA090)
void KomonoLogic::ctrlPost(KomonoState& s, KomonoHost& h) {
    if (s.flags & 8) {
        int p = komonoHitPlayer(s.flags, h, 0, true);
        h.addScore(p, 10);                                       // func_212FA0(jugador, 0xA)
        h.addHitNum(p);                                          // func_213070
        s.flags &= ~8u;
        close(s, h);
    }
    if (s.collide != 0) {
        float p[3] = {s.pos[0], s.pos[1] + s.yOffset, s.pos[2]};
        h.registerCollider(p, s.colliPos);
    }
}

// close__10KomonoTaskFv (0x9EA1E0): rotura. Busca typeId en D_00A096D0 (43 filas de 0x14 B).
void KomonoLogic::close(KomonoState& s, KomonoHost& h) {
    float pos[3] = {s.pos[0], s.pos[1], s.pos[2]};
    int16_t rot[3] = {(int16_t)s.rot[0], (int16_t)s.rot[1], (int16_t)s.rot[2]};  // sp34 = +0x54 (rotY)
    for (int k = 0; k < 43; k++) {
        const KomonoBreakRow& r = kKomonoBreakTable[k];
        if (r.id != s.typeId) continue;
        if (r.isItem != 0) {
            h.breakEffectA(pos, rot[1], r.piece);                // func_1E12B0(&{+1 = 5, +4 = rotY, +8 = pos, +0x14 = pieza})
        } else {
            h.spawnBreakTask(r.id, pos);                         // KomonoDishKowareTask(id, pos)
        }
        if (r.effect != 0) h.effect(r.scale, r.effect, pos);     // func_1E1620(escala, codigo, pos)
        if (r.sound != -1) h.sound(r.sound, 0, 0);
        break;
    }
    switch ((uint32_t)s.typeId) {                                // efectos extra por tipo
        case 0x4700F6: h.effect(3.0f, 8, pos); break;
        case 0x3C001C: h.groundEffect(3, s.pos[0], s.pos[2]); break;
        case 0x3C003C: h.groundEffect(4, s.pos[0], s.pos[2]); break;
        case 0x3C004F: h.groundEffect(5, s.pos[0], s.pos[2]); break;
        case 0x40003A: h.effect(2.0f, 6, pos); break;
        case 0x4700D6: h.effect(2.0f, 7, pos); break;
        default: break;
    }
    if (s.itemKind != 0) CKomonoBase::bornItem(s.endCp, s.pos, s.rot[0], s.rot[1], s.rot[2], s.itemKind, h);
    h.counterInc();                                              // KomEtcMgrInc
    h.closeTask();                                               // func_1A2630
}
}}  // namespace vn::komono

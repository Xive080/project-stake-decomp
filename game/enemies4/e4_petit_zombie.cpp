// Ver e4_petit_zombie.h.
#include "e4_petit_zombie.h"
namespace vn { namespace en4 {

void PetitZombieLogic::init(PetitZombieState& s, const PetitParam& p, int8_t tblByte1, const float pos0[3], int u16a, int u16b, int u16c,
                            const E4Env& e, const PetitCtorIn& in) {
    s.pos[0] = pos0[0]; s.pos[1] = pos0[1]; s.pos[2] = pos0[2];          // +0x1D7C..: ENEMY_TABLE +8,+0xC,+0x10
    s.slot1DA0 = u16a & 0xFFFF; s.slot1DA4 = u16b & 0xFFFF; s.slot1DA8 = u16c & 0xFFFF;   // lhu de +2,+4,+6
    s.flags = 1;                                                          // +0xC = 1 (vivo)
    s.life = tblByte1;                                                    // +0x14 (s16) y +0x18 = byte +1 de ENEMY_TABLE
    s.p = p;
    s.mode = p.mode;                                                      // +0x1D78 = byte +1 del bloque
    s.type = p.type;                                                      // +0x5C   = byte +0 del bloque
    if (p.anim == 0) {                                                    // lh +6 == 0 -> sorteo %4 (con signo C)
        int r = e.rand();
        int m = r & 3; if (r < 0 && m != 0) m -= 4;
        s.anim = in.initAnimTable[m];
    } else {
        s.anim = (int16_t)p.anim;
    }
    if (e.gameState != 0xF) {
        if (p.type >= 0 && p.type < in.scaleCount) s.scale = in.scaleByType[p.type];   // +0x1CE0 = D_009ACB40[tipo]
    } else {
        s.flags |= 0x8000;
    }
    for (int i = 0; i < 8; i++) s.partFlags[i] = 0;
    if (in.typeHasEffect) s.flags |= 0x80;
    s.angle = 0;
}

void PetitZombieLogic::ctrl(PetitZombieState& s, E4Env& e, E4Events& ev) {
    if (!e.paused) {                                                      // func_1F33B0(&D_500F20) <= 0
        if (s.flags & 0x8) {                                              // golpeado
            s.flags &= ~1u;                                               // deja de estar vivo
            e4emit(ev, E4_MOTION, 0x43, 0, 0, 5);                         // func_1FA640(..., 0x43, 0, 5)
            e4emit(ev, E4_SET_HANDLER, PH_DEAD, 0);                       // func_1A3F60(&base, D_009A1620[2], this)
        } else if (e.cam.pathId == s.p.pathId && !(e.cam.frame < (float)s.p.closeFrame)) {
            e4emit(ev, E4_TASK_CLOSE_MOT);                                // func_1A3F80: sale de camara
        }
        if (s.mode == 1) {                                                // gira: sentido segun la vida
            s.angle += (s.life != 0) ? 0x40 : -0x40;
        }
        if (s.mode == 2) {                                                // patrulla entre limites (X y Z)
            const PetitParam& p = s.p;
            bool turned = false;
            if (s.pos[0] < p.xMax) {
                if (s.pos[0] <= p.xMin) {
                    s.pos[0] = p.xMin;
                    s.angle += (s.angle >= 0x4000) ? 0x100 : -0x100;
                    turned = true;
                }
            } else {
                s.pos[0] = p.xMax;
                s.angle += (s.angle >= 0xC000) ? 0x100 : -0x100;
                turned = true;
            }
            if (s.pos[2] < p.zMax) {
                if (s.pos[2] <= p.zMin) {
                    s.pos[2] = p.zMin;                                    // el original escribe el limite aunque ya haya girado en X
                    if (!turned) s.angle += (s.angle < 0x8000) ? 0x100 : -0x100;
                }
            } else {
                s.pos[2] = p.zMax;
                if (!turned) s.angle += (s.angle >= 0x8000) ? 0x100 : -0x100;
            }
        }
    }
    e4emit(ev, E4_MOTION_STEP);                                           // func_1F8D50(D_4A13E0, ...)
    if (!e.paused) {
        s.frame += 1.0f;                                                  // +0x3C += 1.0
        s.flags &= ~0x8u;                                                 // se limpia el bit de golpe
    }
}
}}

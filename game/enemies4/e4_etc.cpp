#include "e4_etc.h"
#include <cmath>
namespace vn { namespace en4 {

bool EtcLogic::itemGate(const E4Env& e, int item) {
    if (!(e.gameState == 0xA && e.gameKind == 1)) return false;
    int8_t f = e.itemFlag[item & 63];
    return (f & 1) && (f & 2) && !(f & 0x20);
}

void EtcLogic::init(EtcState& s, E4Env& e, int type, const float pos0[3], int u2, int u4, int u6, const uint32_t param[4], E4Events& ev) {
    s.flags = 1;
    s.type = (int8_t)type;                        // lb +1 -> sh +0x14
    for (int i = 0; i < 3; i++) s.pos[i] = pos0[i];
    s.a50 = u2 & 0xFFFF; s.a54 = u4 & 0xFFFF; s.a58 = u6 & 0xFFFF;
    for (int i = 0; i < 4; i++) s.param[i] = param[i];
    s.step = 0;
    s.w168 = e.gp73D0; s.w164 = e.gp73D4;         // +0x168 = gp-0x73D0, +0x164 = gp-0x73D4
    int t = s.type;
    switch ((unsigned)t < 9u ? t : -1) {
    case 1:   // nieve
        s.w168 = -1; s.w164 = -1; s.branchSeen = 0;
        s.handler = EH_SNOW; s.handlerPrio = 0;
        e4emit(ev, E4_SET_HANDLER, EH_SNOW, 0);
        e.gp7340 = 1; s.w148 = 0;
        break;
    case 2: s.handler = EH_DEMO_PAUSE_ON;  s.handlerPrio = 3; e4emit(ev, E4_SET_HANDLER, EH_DEMO_PAUSE_ON, 3); break;
    case 3: s.handler = EH_DEMO_PAUSE_OFF; s.handlerPrio = 3; e4emit(ev, E4_SET_HANDLER, EH_DEMO_PAUSE_OFF, 3); break;
    case 4: s.handler = EH_TIMER_BUNKI;    s.handlerPrio = 5; e4emit(ev, E4_SET_HANDLER, EH_TIMER_BUNKI, 5); break;
    case 5: s.handler = EH_ST4_TAMA;       s.handlerPrio = 5; e4emit(ev, E4_SET_HANDLER, EH_ST4_TAMA, 5); break;
    case 6: case 7:
            s.handler = EH_FIREBALL;       s.handlerPrio = 5; e4emit(ev, E4_SET_HANDLER, EH_FIREBALL, 5); break;
    case 8:   // objetos 0x1B y 0x20: se aplican si estan poseidos+activos (0x20 solo si gp-0x7400 == 0)
        if (itemGate(e, 0x1B)) e4emit(ev, E4_ITEM_APPLY, 0x1B);
        if (itemGate(e, 0x20) && e.gp7400 == 0) e4emit(ev, E4_ITEM_APPLY, 0x20);
        s.handler = EH_ITEM_CHECK; s.handlerPrio = 5; e4emit(ev, E4_SET_HANDLER, EH_ITEM_CHECK, 5);
        break;
    default: break;   // tipo 0 y fuera de rango (>= 9): sin manejador propio aqui
    }
    if (s.type >= 10 && s.type < 60) {            // marca de rama
        s.fB0 = 10.0f; s.wC4 = 0;                 // +0xB0 = 10.0 ; +0xC4 = 0
        s.flags |= 0x1008A; s.flags2 = 0; s.branchSeen = 0;
        s.type -= 10;
        s.w148 = 0x1008A; s.w14C = (int)s.param[0]; s.w150 = (int)s.param[1]; s.w154 = (int)s.param[2]; s.w158 = (int)s.param[3];
        s.handler = EH_BRANCH_MARK; s.handlerPrio = 5; e4emit(ev, E4_SET_HANDLER, EH_BRANCH_MARK, 5);
    }
    if (s.type >= 70) {                           // comprobacion de objeto: deja 0x500F03 a 1 si procede
        s.type -= 70;
        if (itemGate(e, s.type)) {
            e.g500F03 = 1;
            if (s.type == 0x1E && e.gp7200 == 1) e.g500F03 = 0;
        } else {
            e.g500F03 = 0;
        }
        s.handler = EH_ITEM_CHECK; s.handlerPrio = 5; e4emit(ev, E4_SET_HANDLER, EH_ITEM_CHECK, 5);
    }
}

void EtcLogic::demoPauseOn(EtcState& s, E4Env& e, E4Events& ev) { (void)e; e4emit(ev, E4_DRAW, 1); /* func_1F3390(&D_500F20, 1) */ e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
void EtcLogic::demoPauseOff(EtcState& s, E4Env& e, E4Events& ev) { (void)e; e4emit(ev, E4_DRAW, 0); /* func_1F3390(&D_500F20, 0) */ e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
void EtcLogic::itemCheck(EtcState& s, E4Events& ev) { e4emit(ev, E4_TASK_CLOSE); s.closed = true; }

void EtcLogic::timerBunki(EtcState& s, E4Env& e, E4Events& ev) {
    if (s.step == 0) {
        s.w148 = palFrames(s.a50);              // (+0x50 * 50) / 60: fotogramas PAL
        e.gp7358 = 0;
        s.step++;
    }
    if (s.step == 1) {
        s.w148 -= 1;
        if (!(s.w148 > 0)) {                    // bgtz fallido: llego a 0
            if (e.gp7374 != 0) e.gp7358 += 1;   // si hay enemigos en pantalla cuenta un ataque perdido
            e4emit(ev, E4_TASK_CLOSE); s.closed = true;
        }
    }
}

void EtcLogic::st4Tama(EtcState& s, E4Env& e, E4Events& ev) {
    const float kMax = 0.9f, kRate = 1.2f;      // 0x3F666666, 0x3F99999A
    if (s.step == 0) {
        e.tintAlpha = 0; e.tintScrollX = 0; e.tintScrollY = 0;
        s.w148 = s.a50; s.w14C = s.a54;         // duraciones de subida y bajada
        s.step++;
    }
    if (s.step == 1) {
        if (!e.paused) e.tintAlpha += kRate * (kMax / (float)s.w148);
        if (!(e.tintAlpha < kMax)) { e.tintAlpha = kMax; s.step++; }
    }
    if (s.step == 2) {                          // (tras la subida el original encadena la bajada en el mismo fotograma)
        if (!e.paused) e.tintAlpha -= kRate * (kMax / (float)s.w14C);
        if (e.tintAlpha <= 0.0f) { e.tintAlpha = 0; e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
    }
    if (!e.paused) {                            // desplazamiento del patron (truncado a entero cada fotograma)
        e.tintScrollX = (int)((float)e.tintScrollX + (-153.6f));   // 0xC319999A
        e.tintScrollY = (int)((float)e.tintScrollY + 76.8f);       // 0x4299999A
    }
}

void EtcLogic::fireBall(EtcState& s, E4Env& e, const float pathPos[3], E4Events& ev) {
    if (!e.paused) {
        if (s.step == 0) {
            s.w148 = 0x260057; s.w14C = 0x260057; s.w150 = 0x260066;      // rango de modelos de la animacion de llama
            e4emit(ev, E4_SOUND_GLOBAL, 0x17010C);
            s.w154 = 0; s.w158 = 0; s.f170 = 1.0f;
            s.step++;
        }
        if (s.step == 1) {
            s.pos[0] = pathPos[0]; s.pos[1] = pathPos[1]; s.pos[2] = pathPos[2];   // camino 0x229 (tipo 6) / 0x22A (tipo 7) en el fotograma w158
            s.w158 += 1;
            if (s.w158 >= 0x2D) {                // 45 fotogramas de vuelo
                s.flags |= 0x40000000;           // fase de explosion
                s.w148 = 0x260040; s.w14C = 0x260040; s.w150 = 0x260056;
                s.w154 = s.w150 - s.w14C;        // 22 fotogramas de explosion
                s.step++;
            }
        }
        if (s.step == 2) {
            s.w154 -= 1;
            if (!(s.w154 > 0)) { e4emit(ev, E4_TASK_CLOSE); s.closed = true; }
        }
    }
    // animacion cíclica de la llama (solo sin pausa)
    if (!e.paused) {
        s.w14C += 1;
        if (!(s.w14C < s.w150)) s.w14C = s.w148;
    }
}

void EtcLogic::branchMark(EtcState& s, E4Env& e, E4Events& ev) {
    if (!itemGate(e, s.type)) {                  // la marca solo existe si el objeto esta poseido+activo en arcade
        e.g500F05 = 0; e.gp73CC = 0;
        e4emit(ev, E4_TASK_CLOSE); s.closed = true;       // el original sigue ejecutando
    }
    if (s.branchSeen == 0) {
        if (e.cam.pathId == s.w14C && !(e.cam.frame < (float)s.w150)) {
            e4emit(ev, E4_ITEM_APPLY, (int8_t)s.type);    // func_A10B00(lb +0x14)
            s.branchSeen = 1;
        }
    }
    if (e.cam.pathId == s.w154 && !(e.cam.frame < (float)s.w158)) {    // el tramo termino: la marca desaparece
        e.g500F05 = 0; e.gp73CC = 0;
        e4emit(ev, E4_TASK_CLOSE); s.closed = true;
    }
    if (s.flags & 0x8) {                         // impacto: la marca queda "tocada"
        s.flags &= ~0x8u;
        s.w148 = 0x10088;
        e.g500F05 = 1; e.gp73CC = 1;
        if (s.type == 0x10) e.g500E11 = 1;
    }
}

void EtcLogic::snowCtrl(EtcState& s, E4Env& e, E4Events& ev) {
    bool moved = false;
    if (s.step != 0) {
        moved = true;                              // s0 = 1
        if (s.step != 1) return;
        if (e.gp7340 == 0) return;                 // sigue esperando
        s.step = 0;
    }
    if (e.gp7340 == 0) { s.step = 1; return; }     // se apaga la nieve: espera
    const float* c = e.cam.pos;
    float dx = c[0] - s.lastCam[0], dy = c[1] - s.lastCam[1], dz = c[2] - s.lastCam[2];
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    s.lastCam[0] = c[0]; s.lastCam[1] = c[1]; s.lastCam[2] = c[2];
    if (!(dist < 50.0f)) moved = true;             // la camara salto > 50: se reinician los huecos
    if (moved) {
        for (int i = 0; i < 10; i++) e.snowFlags[i] = 0;
        e.gp6FDC += 1;
    }
    float yoff = -50.0f;
    for (int i = 0; i < 10; i++) if (e.snowFlags[i] == 1) yoff += 15.0f;
    for (int i = 0; i < 10; i++) {
        if (e.snowFlags[i] == 0) {
            yoff += 15.0f;
            e4emit(ev, E4_SNOW_PARTICLE, i, moved ? 1 : 0, 0, 0, c[0], c[1] + yoff, c[2]);
            e.snowFlags[i] = 1;
        }
    }
}
}}

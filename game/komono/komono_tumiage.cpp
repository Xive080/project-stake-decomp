#pragma STDC FP_CONTRACT OFF   // la PS2 no tiene FMA: sin contraccion de a*b+c
// KomonoTumiageTask: ver komono.h.
#include "komono.h"
namespace vn { namespace komono {

// ctrl__17KomonoTumiageTaskFv (0x9E9400): por caja activa recalcula los centros (suma en el orden del original, despues /4 y /8),
// coloca su colisionador en center8 y, en entrenamiento arcade, enseña el efecto de gafas sobre la primera caja.
// No se reescribe el calculo de ejes de orientacion (func_1AF9FC/func_1AF968 + matriz +0x12E0): solo afecta al dibujo.
void TumiageLogic::ctrl(TumiageState& s, const KomonoEnv& env, KomonoHost& h) {
    for (int i = 0; i < s.count; i++) {
        TumiageBox& b = s.boxes[i];
        if (!b.active) continue;
        for (int k = 0; k < 3; k++) {
            float a = 0.0f;
            for (int q = 4; q < 8; q++) a = a + b.p[q][k];
            b.center4[k] = a / 4.0f;
        }
    }
    for (int i = 0; i < s.count; i++) {
        TumiageBox& b = s.boxes[i];
        if (!b.active) continue;
        for (int k = 0; k < 3; k++) {
            float a = 0.0f;
            for (int q = 0; q < 8; q++) a = a + b.p[q][k];
            b.center8[k] = a / 8.0f;
        }
    }
    for (int i = 0; i < s.count; i++) {
        TumiageBox& b = s.boxes[i];
        if (!b.active) continue;
        if (b.colli) ColliLogic::setPos(*b.colli, b.center8[0], b.center8[1], b.center8[2]);
    }
    if (s.count > 0 && s.boxes[0].active && env.gameState == 0xA && env.gameKind == 1 && s.itemKind >= 0x15) {
        float p[3] = {s.boxes[0].center4[0], s.boxes[0].center4[1] + 4.0f, s.boxes[0].center4[2]};
        h.goggleEffect(p);
    }
}

// ctrlPost__17KomonoTumiageTaskFv
void TumiageLogic::ctrlPost(TumiageState& s) {
    for (int i = 0; i < s.count; i++)
        if (s.boxes[i].active && s.boxes[i].cooldown > 0) s.boxes[i].cooldown--;
}
}}  // namespace vn::komono

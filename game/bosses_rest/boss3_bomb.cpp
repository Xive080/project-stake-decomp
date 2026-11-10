#include "boss3_bomb.h"
#include <cstring>
namespace vn { namespace boss {
static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

void Boss3BombLogic::resetAcc(Bs3BombState& b) { b.acc[0] = 0; b.acc[1] = bitsF(0xBD20902E); b.acc[2] = 0; }   // aceleracion y = -0.0393 (gravedad)

void Boss3BombLogic::adjustLocalPos(Bs3BombState& b) { for (int i = 0; i < 3; i++) b.local[i] = b.pos[i] - b.base[i]; }

// SetTargetLocal: x = 8*(r-0.5) en la rama simetrica; si no, x = 4*(r-0.5) - 2*(1 - 2*kindA) (el original usa mula/msub); y = 4*(r-0.5); z = -1.5.
void Boss3BombLogic::setTargetLocal(Bs3BombState& b, Bs3BombHost& h) {
    if (b.kindB == 1 || b.kindA == -1) {
        float r = h.rand01();
        b.target[0] = 4.0f * (2.0f * (r - 0.5f));
    } else {
        float r = h.rand01();
        float t = 2.0f * (r - 0.5f);
        float acc = 2.0f * t;
        float k = (float)(1 - b.kindA * 2);
        b.target[0] = acc - 2.0f * k;
    }
    b.target[1] = 2.0f * (2.0f * (h.rand01() - 0.5f));
    b.target[2] = bitsF(0xBFC00000);
}

// CtrlPre_Float: SetTargetLocalFloat; velocidad = 0.05 * (objetivo - posicion); posicion += velocidad.
void Boss3BombLogic::ctrlPreFloat(Bs3BombState& b, Bs3BombHost& h) {
    h.setTargetLocalFloat(b);
    const float k = bitsF(0x3D4CCCCD);
    for (int i = 0; i < 3; i++) b.vel[i] = k * (b.target[i] - b.pos[i]);
    for (int i = 0; i < 3; i++) b.pos[i] = b.pos[i] + b.vel[i];
}
void Boss3BombLogic::ctrlPreFall(Bs3BombState& b, Bs3BombHost& h) { h.ctrlPreBound0(b); }
void Boss3BombLogic::ctrlPreBound(Bs3BombState& b, Bs3BombHost& h) { h.setTargetWorld(b); h.ctrlPreTarget2D(b); h.ctrlPreBound0(b); }
void Boss3BombLogic::ctrlPreFly(Bs3BombState& b, Bs3BombHost& h) { h.setTargetWorld(b); h.ctrlPreBound0(b); }
void Boss3BombLogic::ctrlPreAppear(Bs3BombState&) {}
void Boss3BombLogic::ctrlPreExplosion(Bs3BombState&) {}
}} // namespace

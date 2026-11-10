// game_000 (0x1CA190). Verificado contra el asm a mano (m2c incompleto en mula/madd/msub/adda).
// Offsets: cam = gp-0x7338 + 0xD0*s; el original usa s1 = cam + 0x10, asi que s1+0x80 = cam.pos (+0x90), s1+0x98 = cam.look (+0xA8),
// s1+0x94 = pos.flag, s1+0xC3 = d3, +0xC4 = d4, +0xC5 = flagD5, +0xBC = splCnt (+0xCC). slot = D_308740 + 0x240*s.
#include "flow3_camera.h"
#include <cmath>
namespace vn {
void CameraGeo3::runHandler(int h, int s) {
    if (h == CH_GAME000) game000(s); else CameraGeo::runHandler(h, s);
}

void CameraGeo3::game000(int s) {
    CamGeoState& c = cam[s]; CamSlotGeo& g = slot[s]; CamSlotState& ss = q.slot[s];
    int k = branchNoV;
    if (k < 0 || k > 2) { outOfRange++; k = k < 0 ? 0 : 2; }       // el original indexa sin acotar (+0x18C + 0x3C*k)
    calcGameCam(s);
    calcGameInt(s);
    if (c.d3 == 1 && ss.sub[1 + k] == -1) {                         // sub-registro listo: la camara se acerca a su posicion (1:15)
        c.pos.x = interDivPosI(c.pos.x, g.subPos[k].x, 1, 15);
        c.pos.y = interDivPosI(c.pos.y, g.subPos[k].y, 1, 15);
        c.pos.z = interDivPosI(c.pos.z, g.subPos[k].z, 1, 15);
    }
    float dx = c.pos.x - g.tgtPos[0], dy = c.pos.y - g.tgtPos[1], dz = c.pos.z - g.tgtPos[2];
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);            // adda/madd.s, func_001B0AA0
    float B[3] = {0, 0, 0};
    if (!(dist <= 0.1f)) {                                          // 0x3DCCCCCD
        B[0] = (c.look.x + c.pos.x) - g.tgtPos[0];
        B[1] = (c.look.y + c.pos.y) - g.tgtPos[1];
        B[2] = (c.look.z + c.pos.z) - g.tgtPos[2];
        float f20 = c.look.x - c.pos.x, f21 = c.look.y - c.pos.y, f22 = c.look.z - c.pos.z;   // direccion actual
        float f23 = B[0] - c.pos.x, f24 = B[1] - c.pos.y, f25 = B[2] - c.pos.z;               // direccion con el desplazamiento
        float f26 = g.tgtLook[0] - c.pos.x, f27 = g.tgtLook[1] - c.pos.y, f28 = g.tgtLook[2] - c.pos.z;   // hacia el destino
        float c1 = f20 * f25 - f22 * f23, c2 = f20 * f28 - f22 * f26;     // mula/msub: productos cruzados en el plano XZ
        if ((c1 > 0 && c2 > 0) || (c1 < 0 && c2 < 0)) {            // mismo signo estricto: el destino queda al mismo lado
            const float a[3] = {f20, 0, f22}, b1[3] = {f23, 0, f25}, b2[3] = {f26, 0, f28};
            int s4 = vecAng(eng, a, b1), v = vecAng(eng, a, b2);
            if (s4 < v) { c.look.x = B[0]; c.look.z = B[2]; }      // (B es la mirada desplazada)
            else        { c.look.x = g.tgtLook[0]; c.look.z = g.tgtLook[2]; }
        }
        // f20/f22 siguen siendo los de antes de actualizar look (registros): el original no los recalcula
        int A  = (int16_t)eng.arcTan2(f21, std::sqrt(f20 * f20 + f22 * f22));
        int Bn = (int16_t)eng.arcTan2(f24, std::sqrt(f23 * f23 + f25 * f25));
        int C  = (int16_t)eng.arcTan2(f27, std::sqrt(f26 * f26 + f28 * f28));
        int r;
        if (C < Bn) { r = A; if (!(C < A)) r = C; if (!(A < Bn)) r = Bn; }
        else        { r = A; if (!(A < C)) r = C; if (!(Bn < A)) r = Bn; }
        float dx2 = c.look.x - c.pos.x, dz2 = c.look.z - c.pos.z;
        float h = std::sqrt(dx2 * dx2 + dz2 * dz2);
        c.look.y = c.pos.y + h * nlTan(r);
    }
    float o[3];
    interDivCamAng3D(c.pos, g.tgtLook, c.look, o, 1, g.afterSpeed);
    c.look.x = o[0]; c.look.y = o[1]; c.look.z = o[2];
    calcLookAt(c.pos, c.look, c.pos.flag);
    const float a[3] = {g.tgtLook[0] - c.pos.x, g.tgtLook[1] - c.pos.y, g.tgtLook[2] - c.pos.z};
    const float b[3] = {c.look.x - c.pos.x, c.look.y - c.pos.y, c.look.z - c.pos.z};
    c.d4 = std::fabs(vecCos(a, b)) <= 0.99999f ? 0 : 1;            // 0x3F7FFF58
    if (c.d4 == 1 || g.flag47 == 0) {
        if (c.d3 == 1) {
            q.cam[s].flagD5 = 1; q.cam[s].splCnt = 0; ss.cafsIdx = 1;       // +0xC5, +0xBC, slot+0x48
            q.finishJob(s, *this);
            ss.sub[1] = ss.sub[2] = ss.sub[3] = -2;
        }
    }
    setAfterSpeed(s);
}
}

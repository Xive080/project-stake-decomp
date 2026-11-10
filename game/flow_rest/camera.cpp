#include "camera.h"
#include <cmath>
#include <cstring>
namespace vn {

// ---- CalcFCurve (0x1CC410) ----
FCurve FCurve::fromBytes(const uint8_t* p) {
    FCurve c; uint16_t n, it; std::memcpy(&n, p, 2); std::memcpy(&it, p + 2, 2);
    c.n = n; c.iters = it; c.keys = reinterpret_cast<const float*>(p + 4); return c;
}

float calcFCurve(const FCurve& c, float x) {
    const int n = c.n;
    if (n <= 0 || !c.keys) return 0.f;
    auto T = [&](int i) { return c.keys[4 * i]; };
    int a1 = n >> 1, a2 = n >> 2;                                   // u16 >= 0: sra directo
    // Busqueda binaria "iters - 1" pasos: la clave de a1 es menor que x -> avanza a2, si no retrocede a2; a2 se divide por 2 (hacia cero)
    for (int v1 = 0; v1 < c.iters - 1; v1++) {
        int ai = a1 < 0 ? 0 : (a1 > n - 1 ? n - 1 : a1);            // el original no acota (lee fuera); aqui si
        int d = (T(ai) < x) ? a2 : -a2;
        a1 += d;
        a2 = (a2 >= 0) ? (a2 >> 1) : ((a2 + 1) >> 1);
    }
    if (a1 < 0) a1 = 0; if (a1 > n - 1) a1 = n - 1;
    if (T(a1) < x) {
        if (a1 < n - 1) a1 += 1;                                    // interpolar entre a1-1 y a1
        else return c.keys[4 * a1 + 1];                             // pasada la ultima clave: valor constante
    }
    if (a1 == 0) return c.keys[1];                                  // x <= t0: el original lee antes de la curva (ver cabecera)
    const float* k0 = &c.keys[4 * (a1 - 1)]; const float* k1 = &c.keys[4 * a1];
    float t0 = k0[0], v0 = k0[1], mo = k0[2];
    float t1 = k1[0], v1 = k1[1], mi = k1[3];
    float s = x - t0;                                               // f7
    float d = t1 - t0;                                              // f10
    float s2 = s * s;                                               // f8
    float d2 = d * d;                                               // f11
    float a = (3.0f * s2) / d2;                                     // f13
    float s3 = s2 * s;                                              // f9
    float d3 = d2 * d;                                              // f12
    float h0 = (2.0f * s3) / d3;                                    // f6
    float h1 = (-2.0f * s3) / d3;                                   // f3
    h0 = h0 - a;
    h1 = h1 + a;
    float h00 = 1.0f + h0;                                          // f5: 1 - 3s2/d2 + 2s3/d3
    // mula.s f5,f4 ; madd.s f4 = f3,f1  (ACC = h00*v0 ; f4 = ACC + h1*v1)
    float acc = h00 * v0;
    float f4 = acc + h1 * v1;
    float u = (2.0f * s2) / d;                                      // f1/f3
    float w = s3 / d2;                                              // f11
    float q = s2 / d;                                               // f1
    float h10 = w - u;                                              // s3/d2 - 2s2/d
    h10 = s + h10;                                                  // f3: s - 2s2/d + s3/d2
    float f2 = h10 * mo;
    float h11 = w - q;                                              // f1
    float acc2 = f2 + f4;                                           // adda.s f2,f4
    return acc2 + h11 * mi;                                         // madd.s f0 = f1 * f0
}

// ---- CamPathMgr ----
void getCameraPath(const CamCurveSet& s, float frame, int camNo, float pos[3], float look[3], int32_t* flag, float* extra) {
    FCurve c; float v[8] = {};
    for (int k = 0; k < 7; k++) { if (s.curve(camNo, k, c)) v[k] = calcFCurve(c, frame); }
    pos[0] = v[0]; pos[1] = v[1]; pos[2] = v[2];
    look[0] = v[3]; look[1] = v[4]; look[2] = v[5];
    *extra = v[6];
    if (s.gp730C() & 1) { float f = 0; if (s.curve(camNo, 7, c)) f = calcFCurve(c, frame); *flag = (int32_t)f; }   // cvt.w.s
    else *flag = 0;
}
void getObjectPath(const CamCurveSet& s, float frame, int camNo, float out[6]) {
    FCurve c;
    for (int k = 0; k < 6; k++) { out[k] = 0; if (s.curve(camNo, k, c)) out[k] = calcFCurve(c, frame); }
}

// ---- utilidades matematicas ----
int vecAng(CamEngine& e, const float a[3], const float b[3]) {
    float dot = a[0] * b[0]; dot = dot + a[1] * b[1]; dot = dot + a[2] * b[2];
    float aa = a[0] * a[0]; aa = aa + a[1] * a[1]; aa = aa + a[2] * a[2];
    float bb = b[0] * b[0]; bb = bb + b[1] * b[1]; bb = bb + b[2] * b[2];
    float r = aa * bb; r = r - dot * dot;
    float sq = std::sqrt(r < 0 ? 0.f : r);                           // func_001B0AA0 (raiz); el EE la clampa a >= 0
    return (int16_t)e.arcTan2(sq, dot);                              // dsll32/dsra32: 16 bits con signo
}
float interDivPosI(float cur, float dst, int a, int b) {
    float acc = cur * (float)b;
    float num = acc + dst * (float)a;
    return num / (float)(a + b);
}

// ---- CameraMgrInternal ----
uint32_t CameraGeo::recWord(int s, int i) const { return q.mem->rd(q.ringRec[q.slot[s].ring] + 4u * (uint32_t)i); }

void CameraGeo::calcLookAt(Anpo& pos, const Anpo& look, int32_t flag) {   // calcLookAt (0x1C9D30)
    int yaw = 0, pitch = 0;
    eng.getAngleYX(pos.x - look.x, pos.y - look.y, pos.z - look.z, &yaw, &pitch);
    pos.angY = yaw; pos.angX = pitch; pos.flag = flag;
}

void CameraGeo::setAfterSpeed(int s) {                              // setAfterSpeed (0x1CB370)
    CamGeoState& c = cam[s]; CamSlotGeo& g = slot[s];
    float a[3] = { c.look.x - c.pos.x, c.look.y - c.pos.y, c.look.z - c.pos.z };
    float b[3] = { g.tgtLook[0] - c.pos.x, g.tgtLook[1] - c.pos.y, g.tgtLook[2] - c.pos.z };
    int ang = vecAng(eng, a, b);
    if ((ang & 0xFFFF) >= 0x2000) ang = 0x1FFF;
    int tbl = 2;
    if (c.d3 != 1) { int ci = q.slot[s].cafsIdx; tbl = ci >= 6 ? 1 : ci; }   // movz: >=6 -> 1; si no, el propio indice (puede ser negativo: el original no lo acota)
    int idx = ang >> 7; if (ang < 0) idx = (ang + 0x7F) >> 7;
    int8_t e = (tbl >= 0 && tbl < 3 && speed.t[tbl]) ? speed.t[tbl][idx] : 0;
    int num = (int)e * 50;                                          // 50 Hz PAL: valor de 60 Hz * 50 / 60 (division con signo, truncando)
    g.afterSpeed = (int16_t)(num / 60);
}

void CameraGeo::setFinishPos(int s) {                               // setFinishPos (0x1CAC80)
    // registro CCTB 0x30: [3] fotograma, [4] camara, [5] indice de sub-registro (0..2)
    float frame = (float)(int32_t)recWord(s, 3); int camNo = (int16_t)recWord(s, 4); int k = (int32_t)recWord(s, 5);
    if (k < 0 || k > 2) { q.finishJob(s, *this); return; }          // el original no comprueba el rango; aqui no se escribe fuera
    float p[3], l[3]; int32_t fl; float ex;
    getCameraPath(paths, frame, camNo, p, l, &fl, &ex);
    slot[s].subPos[k].x = p[0]; slot[s].subPos[k].y = p[1]; slot[s].subPos[k].z = p[2];
    slot[s].subLook[k].x = l[0]; slot[s].subLook[k].y = l[1]; slot[s].subLook[k].z = l[2];
    q.slot[s].sub[1 + k] = -1;                                      // +0x194 + 0x3C*k = -1
    q.finishJob(s, *this);
}

void CameraGeo::traceCamPath(int s) {                               // traceCamPath (0x1CAD50)
    // registro CCTB 0x40: [3] fotograma inicial, [4] fotograma final, [5] camara, [6] modo (1/2 = fijar, otro = recorrer)
    uint32_t w3 = recWord(s, 3), w4 = recWord(s, 4), w5 = recWord(s, 5), w6 = recWord(s, 6);
    CamGeoState& c = cam[s]; CamSlotGeo& g = slot[s]; CamState& cs = q.cam[s];
    c.camNo = (int16_t)w5; g.flag47 = 1;
    if (w6 == 1 || w6 == 2) {
        if (w6 == 2) g.flag47 = 0;
        q.slot[s].sub[0] = (int16_t)w5;                             // +0x158
        float f = (float)(int32_t)w3; f = f - cs.speed;
        g.frameInt = (int)f;                                        // fptosi (trunca)
        cs.endFrame = (int32_t)w4; g.endInt = (int32_t)w4;
        q.finishJob(s, *this); return;
    }
    if ((int32_t)w3 == (int32_t)w4) {                               // un solo fotograma: se coloca la camara y termina
        cs.frame = (float)(int32_t)w3;
        float p[3], l[3]; int32_t fl; float ex;
        getCameraPath(paths, cs.frame, (int16_t)w5, p, l, &fl, &ex);
        c.pos.x = p[0]; c.pos.y = p[1]; c.pos.z = p[2]; c.look.x = l[0]; c.look.y = l[1]; c.look.z = l[2];
        fov[s] = ex;
        calcLookAt(c.pos, c.look, fl);
        q.finishJob(s, *this); return;
    }
    cs.frame = (float)(int32_t)w3 - cs.speed;                       // empieza un paso antes: traceCamPath_000 suma la velocidad
    cs.endFrame = (int32_t)w4;
    q.slot[s].handler = CH_TRACE000;
    q.runSlot(s, *this);
}

void CameraGeo::traceCamPath_000(int s) {                           // traceCamPath_000 (0x1CAF40): un fotograma de ruta
    // NOTA del original: lee el registro de ring[head] (q.head), no de ring[slot.ring]
    uint32_t rec = q.ringRec[q.head & 7];
    int32_t endF = (int32_t)q.mem->rd(rec + 0x10);
    CamGeoState& c = cam[s]; CamState& cs = q.cam[s]; int camNo = c.camNo;
    if (eng.isDemoPlay() || eng.demoWord500DF8() != 0) {
        cs.frame = cs.frame + cs.speed;
        float e = (float)endF;
        if (!(cs.frame < e)) { cs.frame = e; cs.speed = 1.2f; }     // 0x3F99999A
    }
    int32_t fl = 0; float ex = 0;
    if (cs.splCnt != 0) {                                           // mezcla con la posicion anterior durante splCnt fotogramas
        float p[3], l[3];
        getCameraPath(paths, cs.frame, camNo, p, l, &fl, &ex);
        c.pos.x = interDivPosI(c.pos.x, p[0], 1, cs.splCnt); c.pos.y = interDivPosI(c.pos.y, p[1], 1, cs.splCnt); c.pos.z = interDivPosI(c.pos.z, p[2], 1, cs.splCnt);
        c.look.x = interDivPosI(c.look.x, l[0], 1, cs.splCnt); c.look.y = interDivPosI(c.look.y, l[1], 1, cs.splCnt); c.look.z = interDivPosI(c.look.z, l[2], 1, cs.splCnt);
        cs.splCnt -= 1;
    } else {
        float p[3], l[3];
        getCameraPath(paths, cs.frame, camNo, p, l, &fl, &ex);
        c.pos.x = p[0]; c.pos.y = p[1]; c.pos.z = p[2]; c.look.x = l[0]; c.look.y = l[1]; c.look.z = l[2];
    }
    fov[s] = ex;
    calcLookAt(c.pos, c.look, fl);
    if (!(cs.frame < (float)endF)) { cs.splCnt = 0; q.finishJob(s, *this); }
}

void CameraGeo::game(int s) {                                       // game (0x1C9FA0): arranque del modo de camara de juego
    CamGeoState& c = cam[s]; CamSlotGeo& g = slot[s]; CamState& cs = q.cam[s];
    cs.mode = 2; c.mode = 2; c.d3 = 0;                              // +0xD2 = 2, +0xD3 = 0
    int id = q.slot[s].sub[0];                                      // +0x158 (s16)
    if (id >= 0) {
        c.camNo = (int16_t)id;
        cs.frame = (float)g.frameInt;                               // +0xB0 = (float)slot+0x150
        if (cs.splCnt == 0) {
            float p[3], l[3]; int32_t fl; float ex;
            getCameraPath(paths, cs.frame + cs.speed, id, p, l, &fl, &ex);
            c.pos.x = p[0]; c.pos.y = p[1]; c.pos.z = p[2]; c.pos.flag = fl;       // t0 = cs+0x94 = pos.flag
            g.gameLook.x = l[0]; g.gameLook.y = l[1]; g.gameLook.z = l[2];
            fov[s] = ex;
            c.look.x = g.gameLook.x; c.look.y = g.gameLook.y; c.look.z = g.gameLook.z;
            g.tgtLook[0] = c.look.x; g.tgtLook[1] = c.look.y; g.tgtLook[2] = c.look.z;
            g.tgtPos[0] = c.pos.x; g.tgtPos[1] = c.pos.y; g.tgtPos[2] = c.pos.z;
        }
    }
    for (int k = 0; k < 3; k++) {                                   // sub-registros libres (-2) se rellenan con la camara actual
        if (q.slot[s].sub[1 + k] == -2) {
            g.subPos[k].x = c.pos.x; g.subPos[k].y = c.pos.y; g.subPos[k].z = c.pos.z;
            g.subLook[k].x = c.look.x; g.subLook[k].y = c.look.y; g.subLook[k].z = c.look.z;
        }
    }
    c.d4 = 0; cs.flagD5 = 0; cs.flagD6 = 0; g.flag46 = 0;           // +0xC4, +0xC5, +0xC6, slot+0x46
    g.tgtLook[0] = c.look.x; g.tgtLook[1] = c.look.y; g.tgtLook[2] = c.look.z;
    setAfterSpeed(s);
    q.slot[s].handler = CH_GAME000;                                 // D_00266888: game_000
    q.runSlot(s, *this);
}

void CameraGeo::entryObj(int s, const float p[3], float w) {        // entryObj (0x1CB310)
    CamSlotGeo& g = slot[s];
    if (g.objCount < 0 || g.objCount >= 16) return;                 // el original no acota
    CamObj& o = g.obj[g.objCount]; o.p[0] = p[0]; o.p[1] = p[1]; o.p[2] = p[2]; o.w = w;
    g.objCount++;
}

void CameraGeo::runHandler(int h, int s) {
    switch (h) {
        case CH_GAME: game(s); break;
        case CH_FINISHPOS: setFinishPos(s); break;
        case CH_TRACE: traceCamPath(s); break;
        case CH_TRACE000: traceCamPath_000(s); break;
        case CH_GAME000: default: pending++; lastPending = h; break;   // game_000 no reescrita
    }
}
}

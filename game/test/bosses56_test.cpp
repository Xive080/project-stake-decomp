// Tests de game/bosses56/ (jefes 5 y 6, overlays bs5mod / bs6mod): movimiento por rutas, orquestacion por fotograma,
// avisos de sonido por fotograma, murcielagos y clases de efecto. Los valores esperados salen del asm leido a mano
// (comentarios) y de las tablas de disc/TBL/BS6MOD.BIN; no hay ejecucion real del original.
// Compilar y ejecutar desde la raiz del repo (rutas relativas):
//   mkdir -p build/verify_bosses56 && c++ -std=c++17 -Wall -Wextra game/test/bosses56_test.cpp game/bosses56/bosses56_b5.cpp \
//     game/bosses56/bosses56_b6.cpp game/bosses56/bosses56_data.cpp game/bosses_b/boss6.cpp game/bosses_b/boss6_data.cpp \
//     -o build/verify_bosses56/bosses56_test && build/verify_bosses56/bosses56_test
#include "../bosses56/bosses56_b5.h"
#include "../bosses56/bosses56_b6.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <fstream>
using namespace vn::boss56;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)
static float fb(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// ======================================================================== jefe 5
struct SoundEv { int id, id2, stop; };
struct EffEv { float scale; int id; float pos[3]; };
struct M5 : B5MoveEnv {
    int track = 0x272; float cam[3] = {10, 0, 20};
    int randV = 7; float randF = 0.5f; int len = 100;
    std::vector<SoundEv> snd; std::vector<EffEv> eff; std::vector<float> gauge; int bigEff = 0; int turnCalls = 0; int turnSpeed = 0;
    int camTrackId() override { return track; }
    void camPos(float o[3]) override { for (int i = 0; i < 3; i++) o[i] = cam[i]; }
    void pathEval(float t, int id, B5PathSample& o) override { o.pos[0] = t * 10; o.pos[1] = 5; o.pos[2] = (float)id; o.rot[0] = 0; o.rot[1] = 0x100; o.rot[2] = 0; }
    int pathLen(int) override { return len; }
    void rotYaw(int ang, const float in[3], float out[3]) override { out[0] = in[0] + (float)ang; out[1] = in[1]; out[2] = in[2]; }
    void rotOrdered(int order, const int32_t rot[3], const float in[3], float out[3]) override { out[0] = in[0] + (float)order; out[1] = in[1] + (float)rot[1]; out[2] = in[2]; }
    int atan2Ang(float dx, float dz) override { return (int)(dx * 100 + dz); }
    int rand() override { return randV; }
    float randFloat() override { return randF; }
    void sound(int id, int id2, int stop) override { snd.push_back({id, id2, stop}); }
    void effect(float s, int id, const float p[3]) override { eff.push_back({s, id, {p[0], p[1], p[2]}}); }
    void effectBig(float, float, int, const float*) override { bigEff++; }
    void posRotY(float x, float y, float z, float o[3]) override { o[0] = x; o[1] = y; o[2] = z; }
    void turnToCam(const float*, int sp) override { turnCalls++; turnSpeed = sp; }
    void setGaugeLevel(int, float l) override { gauge.push_back(l); }
};
struct Steps : B5StepEnv { std::vector<int> v; void step(int w) override { v.push_back(w); } };
struct Steps6 : B6StepEnv { std::vector<int> v; void step(int w) override { v.push_back(w); } };

static void test_boss5() {
    M5 env;
    // --- GetCamRot (asm 0x9C7160): ids 0x273..0x27E -> (id-0x273)*0x1555; el resto 0
    env.track = 0x272; CHECK(B5Move::getCamRot(env) == 0);
    env.track = 0x273; CHECK(B5Move::getCamRot(env) == 0);
    env.track = 0x274; CHECK(B5Move::getCamRot(env) == 0x1555);
    env.track = 0x27E; CHECK(B5Move::getCamRot(env) == 11 * 0x1555);
    env.track = 0x27F; CHECK(B5Move::getCamRot(env) == 0);
    env.track = 0x274; { float o[3], in[3] = {1, 2, 3}; B5Move::getCamPos(env, in, o); CHECK(o[0] == 1.0f + 0x1555 && o[1] == 2 && o[2] == 3); }

    // --- UpdateMotionFrame
    { B5MoveState b; b.frame3C = 1; b.frame40 = 2; b.rate = 0.5f; b.rate2 = 0.25f;
      B5Move::updateMotionFrame(b); CHECK(b.frame3C == 1.5f && b.frame40 == 2.25f);
      b.f0C = B5F0C_FROZEN; B5Move::updateMotionFrame(b); CHECK(b.frame3C == 1.5f && b.frame40 == 2.25f); }

    // --- ControlByMotion (0x1C9: cruces en 72 y 240)
    { B5MoveState b; b.motion = 0x1C9; b.rate = 1.0f; M5 e;
      b.frame = 71.5f; B5Move::controlByMotion(b, e); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A0124 && e.snd[0].stop == 0);
      e.snd.clear(); b.frame = 72.0f; B5Move::controlByMotion(b, e); CHECK(e.snd.empty());   // prev no es < 72
      b.frame = 239.0f; B5Move::controlByMotion(b, e); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A0125);
      e.snd.clear(); b.motion = 0x1CA; b.frame = 71.5f; B5Move::controlByMotion(b, e); CHECK(e.snd.empty()); }

    // --- ControlByMotionX: cruce prev < T <= prev+rate; gana la primera regla
    auto cue = [&](int motion, float frame, uint32_t flags, M5& e, B5MoveState& b) {
        b = B5MoveState(); b.motion = motion; b.frame = frame; b.rate = 1.0f; b.f1E40 = flags; b.counter = 1; b.pos[0] = 3; b.pos[2] = 4;
        B5Move::controlByMotionX(b, e); };
    { B5MoveState b; M5 e;
      cue(0x1D3, 74.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A010C && e.snd[0].stop == 0 && e.snd[0].id2 == 0x1A010C);
      e = M5(); cue(0x1D3, 79.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1B0022);
      e = M5(); cue(0x1D3, 219.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A000E);
      e = M5(); cue(0x1D3, 100.0f, 0, e, b); CHECK(e.snd.empty());
      e = M5(); cue(0x1D6, 0.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A000D);
      e = M5(); cue(0x1D6, 74.5f, 0, e, b); CHECK(e.snd[0].id == 0x1A010C);
      e = M5(); cue(0x1D6, 79.5f, 0, e, b); CHECK(e.snd[0].id == 0x1B0013);
      e = M5(); cue(0x1D4, 13.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A000F);
      // 0x1DC: a 66 sonido + efecto 1.0/0x39; a 100 solo efecto 20.0/0x2F (el punto es (x, -11.1, z) con bits 0xC1319999)
      e = M5(); cue(0x1DC, 65.5f, 0, e, b);
      CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A0114 && e.eff.size() == 1 && e.eff[0].id == 0x39 && e.eff[0].scale == 1.0f);
      CHECK(e.eff[0].pos[0] == 3 && e.eff[0].pos[1] == fb(0xC1319999u) && e.eff[0].pos[2] == 4);
      e = M5(); cue(0x1DC, 99.5f, 0, e, b); CHECK(e.snd.empty() && e.eff.size() == 1 && e.eff[0].id == 0x2F && e.eff[0].scale == 20.0f);
      // 0x1DA: a 29 efecto 0x39 sin sonido de cruce; el sonido continuo 1A0113 arranca (bit 1 de 1E5D)
      e = M5(); cue(0x1DA, 28.5f, 0, e, b);
      CHECK(e.eff.size() == 1 && e.eff[0].id == 0x39 && e.snd.size() == 1 && e.snd[0].id == 0x1A0113 && e.snd[0].stop == 0 && (b.cueBits & 1));
      // ... contador % 5 == 0: efecto 0x2E de escala 15 + 5*rand
      e = M5(); b = B5MoveState(); b.motion = 0x1DA; b.frame = 0; b.rate = 1; b.counter = 10; e.randF = 0.5f; B5Move::controlByMotionX(b, e);
      CHECK(e.eff.size() == 1 && e.eff[0].id == 0x2E && e.eff[0].scale == 17.5f);
      // segunda llamada con el mismo movimiento: no repite el sonido continuo
      e = M5(); b.counter = 11; B5Move::controlByMotionX(b, e); CHECK(e.snd.empty());
      // cambio de movimiento: se para (stop = 1) y se limpia el bit
      e = M5(); b.motion = 0x100; B5Move::controlByMotionX(b, e);
      CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A0113 && e.snd[0].stop == 1 && !(b.cueBits & 1));
      // 0x1D8: a 30 sonido 1A0010 (y no mira 25); a 25 sonido 1A0115 + efecto; luego mira el cruce de 56 (posRotY(0,-11.2,12))
      e = M5(); cue(0x1D8, 29.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A0010 && e.eff.empty());
      e = M5(); cue(0x1D8, 24.5f, 0, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A0115 && e.eff.size() == 1 && e.eff[0].id == 0x39);
      e = M5(); cue(0x1D8, 55.5f, 0, e, b);
      CHECK(e.snd.empty() && e.eff.size() == 1 && e.eff[0].pos[0] == 0 && e.eff[0].pos[1] == fb(0xC1333333u) && e.eff[0].pos[2] == 12.0f);
      e = M5(); cue(0x1DD, 55.5f, 0, e, b); CHECK(e.eff.size() == 1 && e.snd.empty());
      // bandera 0x400: nada
      e = M5(); cue(0x1D3, 74.5f, B5F_NO_CUES, e, b); CHECK(e.snd.empty() && e.eff.empty());
      // bandera 0x10000 (pilares): sonido continuo 1B001C con alta/baja
      e = M5(); cue(0x100, 0, B5F_PILLARS, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1B001C && e.snd[0].stop == 0 && (b.cueBits & 2));
      e = M5(); b.f1E40 = 0; B5Move::controlByMotionX(b, e); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1B001C && e.snd[0].stop == 1 && !(b.cueBits & 2));
      // bandera 0x2000: sonido 1C000F y, con contador % 15 == 0, efecto 1.6 / 0x31
      e = M5(); cue(0x100, 0, B5F_SND_2000, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1C000F && (b.cueBits & 4) && e.eff.empty());
      e = M5(); b = B5MoveState(); b.motion = 0x100; b.f1E40 = B5F_SND_2000; b.cueBits = 4; b.counter = 30; B5Move::controlByMotionX(b, e);
      CHECK(e.snd.empty() && e.eff.size() == 1 && e.eff[0].id == 0x31 && e.eff[0].scale == fb(0x3FCCCCCDu));
      // prioridad: un cruce de sonido impide arrancar los continuos en el mismo fotograma
      e = M5(); cue(0x1D3, 74.5f, B5F_PILLARS, e, b); CHECK(e.snd.size() == 1 && e.snd[0].id == 0x1A010C && !(b.cueBits & 2)); }

    // --- InitPath (0x9C6B30)
    { B5MoveState b; b.pathT = 9; b.pathLimit = 9; b.pathKind = 3; b.pathBlend = 0.2f; b.pathBlendStep = 0.1f; b.pathId = 4;
      B5Move::initPath(b);
      CHECK(b.pathT == 0 && b.pathLimit == 0 && b.pathKind == 0 && b.pathId == 0 && b.pathSpeed == fb(0x3F99999Au) && b.pathBlend == 1.0f && b.pathBlendStep == 0); }

    // --- ControlPath modo 0/1 (mira a la camara). camRot = 0 (track 0x272); pathEval: pos=(t*10,5,id)
    { M5 e; e.track = 0x272; e.cam[0] = 10; e.cam[2] = 20;
      B5MoveState b; B5Move::initPath(b); b.pathId = 50; b.pathLimit = 100; b.pathT = 2; b.pathSpeed = 1.5f; b.pathKind = 0;
      B5Move::controlPath(b, e);
      // p = rotYaw(0, (20,5,50)) = (20,5,50); yaw = (int16)atan2(cam.x - 20, cam.z - 50) = (int16)((-10)*100 + (-30)) = -1030
      CHECK(b.pos[0] == 20 && b.pos[1] == 5 && b.pos[2] == 50);
      CHECK(b.rot[0] == 0 && b.rot[1] == -1030 && b.rot[2] == 0 && b.pathYaw == -1030);
      CHECK(b.pathT == 3.5f);
      // con el desplazamiento (flag 0x80000) se suma al resultado
      b.f1E40 = B5F_OFFSET; b.off[0] = 1; b.off[1] = 2; b.off[2] = 3; b.pathT = 2; B5Move::controlPath(b, e);
      CHECK(b.pos[0] == 21 && b.pos[1] == 7 && b.pos[2] == 53);
      // tope: pathT se queda en pathLimit
      b.pathT = 99.5f; B5Move::controlPath(b, e); CHECK(b.pathT == 100.0f); }
    // modo 2: mira hacia delante; con camRot (track 0x274 -> 0x1555) se suma al giro
    { M5 e; e.track = 0x274;
      B5MoveState b; B5Move::initPath(b); b.pathId = 50; b.pathLimit = 100; b.pathT = 2; b.pathSpeed = 1; b.pathKind = 2;
      B5Move::controlPath(b, e);
      // path(2)=(20,5,50) -> girado +0x1555 en x: p.x = 20+0x1555. t2 = 0.001 + 2 <= 100: s=(20.01,5,50);
      // yaw = camRot + (int16)atan2(s.x - path.x, s.z - path.z) = 0x1555 + (int)(0.01*100 + 0) = 0x1555 + 1
      { float t2 = fb(0x3A83126Fu) + 2.0f; int want = 0x1555 + (int16_t)(int)((t2 * 10 - 20.0f) * 100 + 0); CHECK(b.pathYaw == want && b.rot[1] == want); }
      CHECK(b.pos[0] == 20.0f + (float)0x1555);
      // pathT > limit - 0.001: no recalcula, conserva el giro previo
      b.pathT = 100.0f; b.pathYaw = 77; B5Move::controlPath(b, e); CHECK(b.pathYaw == 77 && b.rot[1] == 77); }
    // modo 5: rot.y = rot de la ruta + camRot
    { M5 e; e.track = 0x274; B5MoveState b; B5Move::initPath(b); b.pathLimit = 10; b.pathKind = 5; B5Move::controlPath(b, e);
      CHECK(b.rot[1] == 0x100 + 0x1555); }
    // mezcla: pathBlend sube con el paso y se acota a 1
    { M5 e; B5MoveState b; B5Move::initPath(b); b.pathLimit = 10; b.pathBlend = 0.5f; b.pathBlendStep = 0.3f;
      B5Move::controlPath(b, e); CHECK(b.pathBlend == 0.8f && b.pathBlendStep == 0.3f);
      B5Move::controlPath(b, e); CHECK(b.pathBlend == 1.0f && b.pathBlendStep == 0.0f); }

    // --- InitWaitPath: sel = 0xC0 + rand % 4 (resto con signo C); offPrev = off
    { M5 e; B5MoveState b;
      e.randV = 7; B5Move::initWaitPath(b, e); CHECK(b.waitSel == 0xC3 && b.waitT == 0 && b.waitSpeed == fb(0x3F99999Au));
      // GetWaitPos: pathEval(0, sel) = (0,5,sel) -> rotOrdered(order=rotOrder(0), rot.y=0) = (0, 5, sel)
      CHECK(b.off[0] == 0 && b.off[1] == 5 && b.off[2] == (float)0xC3 && b.offPrev[2] == b.off[2]);
      e.randV = -3; B5Move::initWaitPath(b, e); CHECK(b.waitSel == 0xC0 + (-3 % 4)); }
    // --- GetWaitPos fuera de 0xC0..0xC3 no escribe
    { M5 e; B5MoveState b; b.waitSel = 0xC4; float o[3] = {9, 9, 9}; B5Move::getWaitPos(b, e, o); CHECK(o[0] == 9 && o[1] == 9 && o[2] == 9);
      b.waitSel = 0xBF; B5Move::getWaitPos(b, e, o); CHECK(o[0] == 9); }
    // --- ControlWaitPath: guarda offPrev, avanza y da la vuelta a la longitud
    { M5 e; e.len = 100; B5MoveState b; b.waitSel = 0xC1; b.waitT = 99.0f; b.waitSpeed = 1.5f; b.off[0] = 4; b.off[1] = 5; b.off[2] = 6; b.rotOrder = 0;
      B5Move::controlWaitPath(b, e);
      CHECK(b.offPrev[0] == 4 && b.offPrev[1] == 5 && b.offPrev[2] == 6);
      CHECK(b.waitT == 0.5f);        // 99 + 1.5 = 100.5 > 100 -> 0.5
      b.waitSel = 0xC4; b.waitT = 3; B5Move::controlWaitPath(b, e); CHECK(b.waitT == 3);
      b.waitSel = 0xC1; b.waitT = 100.0f; b.waitSpeed = 0; B5Move::controlWaitPath(b, e); CHECK(b.waitT == 100.0f);   // 100 <= 100: no resta
      e.len = 0; b.waitT = 5; b.waitSpeed = 1; B5Move::controlWaitPath(b, e); CHECK(b.waitT == 6.0f); }   // len 0: el original se colgaria; aqui sale

    // --- ControlPos
    { M5 e; B5MoveState b; b.pos[0] = 1; b.pos[1] = 1; b.pos[2] = 1; b.off[0] = 5; b.offPrev[0] = 2; b.off[1] = 7; b.offPrev[1] = 7; b.off[2] = 1; b.offPrev[2] = 4;
      B5Move::controlPos(b, e); CHECK(e.turnCalls == 0 && b.pos[0] == 1);                       // sin flags: nada
      b.f1E40 = B5F_OFFSET; B5Move::controlPos(b, e); CHECK(b.pos[0] == 4 && b.pos[1] == 1 && b.pos[2] == -2);
      b.f1E40 = B5F_TURN_TO_CAM; B5Move::controlPos(b, e); CHECK(e.turnCalls == 1 && e.turnSpeed == 0x400);
      b.f1E40 = 0; b.f0C = B5F0C_ATTACKING; B5Move::controlPos(b, e); CHECK(e.turnCalls == 2);
      // con ruta (1DB8 == 1) usa ControlPath y NO suma el desplazamiento de espera
      B5MoveState c; B5Move::initPath(c); c.pathMode = 1; c.pathLimit = 10; c.pathId = 9; c.pathKind = 5; c.f1E40 = B5F_OFFSET; c.off[0] = 100;
      B5Move::controlPos(c, e); CHECK(c.pos[0] == 0 + 100 && c.pos[2] == 9 && c.pathT == fb(0x3F99999Au)); }   // pathEval(0).x = 0, + off[0] = 100 (via controlPath)

    // --- CtrlPre: orden de pasos
    { M5 e; Steps st; B5MoveState b; b.kind = 1; b.pos[0] = 5; b.rot[1] = 9; b.objScale[2] = 3; b.f1E40 = B5F_CLEAR_AT_PRE | B5F_LIFEBAR;
      B5Move::ctrlPre(b, e, st);
      std::vector<int> want2 = {B5S_DISPATCH_STATE, B5S_FALL_CHECK, B5S_CHECK_DAMAGE_END, B5S_CHECK_DAMAGE, B5S_CONTROL_ATTACK, B5S_WAIT_PATH,
                                B5S_CONTROL_POS, B5S_WATER_PILLAR, B5S_BY_MOTION_X, B5S_LIFE_DISPLAY};
      CHECK(st.v == want2);
      CHECK(b.savedPos[0] == 5 && b.savedRot[1] == 9 && b.savedScale[2] == 3 && !(b.f1E40 & B5F_CLEAR_AT_PRE));
      Steps s1; b.kind = 0; b.f1E40 = 0; B5Move::ctrlPre(b, e, s1);
      std::vector<int> want1 = {B5S_DISPATCH_STATE, B5S_FALL_CHECK, B5S_CHECK_DAMAGE_END, B5S_CHECK_DAMAGE, B5S_CONTROL_ATTACK, B5S_CONTROL_POS,
                                B5S_CONTROL_ALPHA, B5S_BY_MOTION};
      CHECK(s1.v == want1);
      Steps s2; b.kind = 7; B5Move::ctrlPre(b, e, s2); CHECK(s2.v.empty());   // tipo desconocido: no llama a ninguno
      // 1D74 != 0: espera la ruta de camara 0x272
      Steps s3; b.kind = 1; b.kind74 = 1; e.track = 0x271; B5Move::ctrlPre(b, e, s3); CHECK(s3.v.empty() && b.kind74 == 1 && e.bigEff == 0);
      e.track = 0x272; B5Move::ctrlPre(b, e, s3); CHECK(b.kind74 == 0 && e.bigEff == 1 && s3.v.size() == want2.size() - 1); }
    // --- CtrlPre: nivel 43E4 (0..5, paso 1/12)
    { M5 e; Steps st; B5MoveState b; b.kind = 1; b.gaugeObj = 1; b.f1E40 = B5F_GAUGE_UP; b.gaugeLevel = 4.95f;
      B5Move::ctrlPre(b, e, st); CHECK(b.gaugeLevel == 5.0f && e.gauge.size() == 1 && e.gauge[0] == 5.0f);
      b.gaugeLevel = 1.0f; B5Move::ctrlPre(b, e, st); CHECK(b.gaugeLevel == 1.0f + fb(0x3DAAAAABu));
      b.f1E40 = 0; b.gaugeLevel = 0.05f; B5Move::ctrlPre(b, e, st); CHECK(b.gaugeLevel == 0.0f);
      b.gaugeLevel = 0.0f; B5Move::ctrlPre(b, e, st); CHECK(b.gaugeLevel == 0.0f);
      b.gaugeObj = 0; size_t n = e.gauge.size(); B5Move::ctrlPre(b, e, st); CHECK(e.gauge.size() == n); }
}

// ======================================================================== jefe 6
struct M6 : B6MoveEnv {
    float cam[3] = {10, 4, 20}; std::vector<int> events;
    int circleDir = 0; int afterImages = 0;
    void camPos(float o[3]) override { for (int i = 0; i < 3; i++) o[i] = cam[i]; }
    void pathEval(float f, int id, B6Anpo& o) override { o.pos[0] = f; o.pos[1] = 2; o.pos[2] = (float)id; o.rot[0] = 7; o.rot[1] = 8; o.rot[2] = 9; }
    void pathEvalCT(float f, int id, B6Anpo& o, float t[3]) override { pathEval(f, id, o); t[0] = 100; t[1] = 50; t[2] = 25; }
    void lookAt(float dx, float dy, float dz, int32_t& r0, int32_t& r1) override { r0 = (int)dy; r1 = (int)(dx + dz); }
    int atan2Ang(float dx, float dz) override { return (int)(dx * 100 + dz); }
    void transformSaved(const B6MoveState& b, B6Anpo& o) override { o.pos[0] += b.savedPos[0]; o.pos[2] += b.savedPos[2]; }
    void rotYawVec(int ang, float v[3]) override { v[0] += (float)ang; }
    void rotCircle(int ang, float r, float y, float out[3]) override { out[0] = (float)ang; out[1] = y; out[2] = -r; }
    float sqrtf_(float v) override { return std::sqrt(v); }
    void spawnCircleCamera(int d) override { circleDir = d; }
    void spawnAfterImage() override { afterImages++; }
};

struct MB : B6BatEnv {
    std::vector<float> rf; size_t ri = 0; int randV = 0; int len = 50; float samp[3] = {1, 2, 3};
    std::vector<int> snd; std::vector<int> alerts; int spawned = -1; int dmg = 0; float dist = 100; int shotCalls = 0, rotCalls = 0; int dmgCalls = 0;
    int rand() override { return randV; }
    float randFloat() override { return ri < rf.size() ? rf[ri++] : 0.5f; }
    int pathLen(int) override { return len; }
    void pathEval(float t, int id, float out[3]) override { out[0] = samp[0] + t; out[1] = samp[1]; out[2] = samp[2] + (float)id; }
    void rotMatrixAdd(const float v[3], float dest[3]) override { for (int i = 0; i < 3; i++) dest[i] = v[i] * 2.0f; }
    float sinAng(int a) override { return (float)a; }
    float cosAng(int a) override { return (float)(a + 1); }
    void sound(int id, int, int, int) override { snd.push_back(id); }
    void alert(int v) override { alerts.push_back(v); }
    void spawnBat(int i) override { spawned = i; }
    int damagePlayer(int, int, int) override { dmgCalls++; return dmg; }
    float distToCam(const float*) override { return dist; }
    void setShotParam(B6BatState&) override { shotCalls++; }
    void rotPos(B6BatState&) override { rotCalls++; }
};

static void test_boss6_move() {
    // --- AddMotionFrame / AddPathFrame
    { B6MoveState b; b.rate = 0.5f; b.rate1E1C = 2.0f; B6Move::addMotionFrame(b); CHECK(b.frame3C == 0.5f && b.frame40 == 2.0f);
      b.f0C = B6F0C_FROZEN; B6Move::addMotionFrame(b); CHECK(b.frame3C == 0.5f);
      B6MoveState c; c.rate2 = 1.5f; c.f1DA4 = B6A_PATH; B6Move::addPathFrame(c); CHECK(c.pathFrame == 1.5f);
      c.f1DA4 = B6A_APPROACH; c.apprFrame = 1; c.apprStep = 2; c.apprTotal = 5; B6Move::addPathFrame(c); CHECK(c.apprFrame == 3 && c.pathFrame == 1.5f);
      B6Move::addPathFrame(c); CHECK(c.apprFrame == 5);
      B6Move::addPathFrame(c); CHECK(c.apprFrame == 5 && c.apprStep == 0.0f);                 // 5 no es < 5: se queda y para
      c.f1DA4 = 0; c.apprFrame = 1; c.apprStep = 3; B6Move::addPathFrame(c); CHECK(c.apprFrame == 1); }

    // --- ApproachANPO
    { B6MoveState b; B6Anpo t{}; t.pos[0] = 10; t.pos[1] = 20; t.pos[2] = 30; t.rot[0] = 0x2000; t.rot[1] = 0x1000; t.rot[2] = 0;
      b.f1DA4 = B6A_APPROACH; b.apprPos[0] = 0; b.apprPos[1] = 0; b.apprPos[2] = 0; b.apprRot[0] = 0x1000; b.apprRot[1] = 0x2000; b.apprRot[2] = 0;
      b.apprFrame = 5; b.apprTotal = 10;
      B6Move::approachANPO(b, t);
      CHECK(b.pos[0] == 5 && b.pos[1] == 10 && b.pos[2] == 15);
      CHECK(b.rot[0] == 0x1800 && b.rot[1] == 0x1800 && b.rot[2] == 0 && (b.f1DA4 & B6A_APPROACH));   // mismos casos que B6InterDivAngF en boss6_test
      b.apprFrame = 10; B6Move::approachANPO(b, t);
      CHECK(b.pos[0] == 10 && b.rot[0] == 0x2000 && !(b.f1DA4 & B6A_APPROACH));
      B6MoveState c; c.pos[0] = 1; c.f1DA4 = 0; B6Move::approachANPO(c, t); CHECK(c.pos[0] == 1);      // sin modo: no toca
      c.f1DA4 = B6A_PATH; B6Move::approachANPO(c, t); CHECK(c.pos[0] == 10 && c.rot[1] == 0x1000);
      B6MoveState d; d.f1DA4 = B6A_CIRCLE; B6Move::approachANPO(d, t); CHECK(d.pos[2] == 30); }

    // --- GetPathParam: tipos, id -1, bit 0x400/0x800, fin de ruta
    { M6 e; B6MoveState b; b.pathEnd = 100; b.pathFrame = 10; b.pathId = 5; b.kind = 1; b.f1DA4 = B6A_PATH; B6Anpo o{};
      B6Move::getPathParam(b, e, o);
      // kind 1: rot = (0, atan2(cam.x - 10, cam.z - 5)=(int)(0*100+15)=15, 0); pos de la muestra = (10, 2, 5)
      CHECK(o.pos[0] == 10 && o.pos[1] == 2 && o.pos[2] == 5 && o.rot[0] == 0 && o.rot[1] == 15 && o.rot[2] == 0 && !(b.f1DA4 & B6A_PATH_DONE));
      // kind 0: rot2 = 0, rot0/rot1 = lookAt(cam - pos)
      b.kind = 0; B6Move::getPathParam(b, e, o); CHECK(o.rot[0] == 2 && o.rot[1] == 0 + 15 && o.rot[2] == 0);   // dy = 4-2 = 2; dx + dz = 0 + 15
      // kind 2: sin giro y altura minima 8
      b.kind = 2; B6Move::getPathParam(b, e, o); CHECK(o.rot[0] == 0 && o.rot[1] == 0 && o.rot[2] == 0 && o.pos[1] == 8.0f);
      // kind 3: no cambia la rotacion de la muestra
      b.kind = 3; B6Move::getPathParam(b, e, o); CHECK(o.rot[0] == 7 && o.rot[1] == 8 && o.rot[2] == 9);
      // 0x1000: 100 -> da la vuelta; sin 0x1000: se queda en el fin y marca 0x200
      b.kind = 1; b.pathFrame = 100.0f; b.f1DA4 = B6A_PATH | B6A_PATH_LOOP; B6Move::getPathParam(b, e, o); CHECK(b.pathFrame == 0.0f && !(b.f1DA4 & B6A_PATH_DONE));
      b.pathFrame = 150.5f; b.f1DA4 = B6A_PATH; B6Move::getPathParam(b, e, o); CHECK(b.pathFrame == 100.0f && (b.f1DA4 & B6A_PATH_DONE));
      b.pathFrame = 99.9f; B6Move::getPathParam(b, e, o); CHECK(!(b.f1DA4 & B6A_PATH_DONE));                 // (int)99.9 = 99 < 100
      // id -1: la propia posicion con Y = 0 y el giro Y propio
      b.pathId = -1; b.pos[0] = 3; b.pos[1] = 9; b.pos[2] = 4; b.rot[1] = 0x55; B6Move::getPathParam(b, e, o);
      CHECK(o.pos[0] == 3 && o.pos[1] == 0 && o.pos[2] == 4 && o.rot[0] == 0 && o.rot[1] == 0x55 && o.rot[2] == 0);
      // id < -1: no escribe
      b.pathId = -2; o.pos[0] = 77; B6Move::getPathParam(b, e, o); CHECK(o.pos[0] == 77);
      // ruta de camara (0x100)
      b.pathId = 5; b.f1DA4 = B6A_PATH | B6A_CAMPATH; b.pathFrame = 10; B6Move::getPathParam(b, e, o);
      CHECK(o.rot[0] == 50 - 2 && o.rot[1] == (int)((100 - 10) + (25 - 5)) && o.rot[2] == 0);
      // 0x400: giro en cuartos de vuelta: ang = (-sel) << 14
      b.f1DA4 = B6A_PATH | B6A_PATH_400; b.sel1DC4 = 2; b.kind = 2; B6Move::getPathParam(b, e, o);
      CHECK(o.pos[0] == 10.0f + (float)(-2 * 0x4000) && o.rot[1] == 0 + (-2 * 0x4000));
      // 0x800: transformSaved + giro Y hacia la camara; y 0x400 no se aplica a la vez
      b.f1DA4 = B6A_PATH | B6A_PATH_800 | B6A_PATH_400; b.kind = 1; b.savedPos[0] = 100; b.savedPos[2] = 1; B6Move::getPathParam(b, e, o);
      CHECK(o.pos[0] == 110 && o.pos[2] == 6 && o.rot[1] == (int)((10 - 110) * 100 + (20 - 6))); }

    // --- TracePathParam: al terminar (0x200) apaga los modos de ruta
    { M6 e; B6MoveState b; b.pathEnd = 10; b.pathFrame = 20; b.pathId = 1; b.kind = 3;
      b.f1DA4 = B6A_PATH | B6A_CAMPATH | B6A_PATH_400 | B6A_PATH_800 | B6A_PATH_LOOP | B6A_LIFEBAR;
      b.f1DA4 &= ~B6A_PATH_LOOP;   // sin vuelta: llega al fin
      B6Move::tracePathParam(b, e);
      CHECK(b.f1DA4 == (B6A_PATH_DONE | B6A_LIFEBAR));
      CHECK(b.pathOut.pos[0] == 10);   // pathFrame acotado a 10 (el 0x800 sumo savedPos 0)
    }
    { M6 e; B6MoveState b; b.pathEnd = 100; b.pathFrame = 3; b.pathId = 1; b.kind = 3; b.f1DA4 = B6A_PATH;
      B6Move::tracePathParam(b, e); CHECK((b.f1DA4 & B6A_PATH) && b.pathOut.pos[0] == 3 && b.pathOut.rot[1] == 8); }

    // --- UpdatePath: con ruta copia el resultado a lastPath y llama a ApproachANPO (copia pos/rot al objeto)
    { M6 e; B6MoveState b; b.pathEnd = 100; b.pathFrame = 4; b.pathId = 6; b.kind = 3; b.f1DA4 = B6A_PATH;
      B6Move::updatePath(b, e);
      CHECK(b.lastPath.pos[0] == 4 && b.lastPath.pos[2] == 6 && b.pos[0] == 4 && b.pos[2] == 6 && b.rot[1] == 8);
      B6MoveState c; c.f1DA4 = 0; c.pos[0] = 42; B6Move::updatePath(c, e); CHECK(c.pos[0] == 42 && c.lastPath.pos[0] == 0); }

    // --- SetCircleMove / CircleMove (valores de kPoseB leidos de la tabla D_009D3750, [4][4] poses de 24 bytes)
    { M6 e; B6MoveState b; b.pos[0] = 30; b.pos[1] = 6; b.pos[2] = -40; b.sel1DC4 = 3; b.sel1DC5 = 2;
      const vn::boss6::Pose& p = vn::boss6::kPoseB_flat[0 * 4 + 2];   // (3 + 1 + 4) % 4 = 0 con dir = +1
      B6Move::setCircleMove(b, e, 1);
      CHECK(b.sel1DC4 == 0 && e.circleDir == 1);
      CHECK(b.circY0 == 6.0f && b.circR0 == 50.0f);                       // sqrt(30^2 + 40^2)
      CHECK(b.circAng0 == ((-(int)(30 * 100 + 40)) & 0xFFFF));            // atan2Ang(x, -z) = (int)(30*100 + 40)
      CHECK(b.circY1 == p.p[1] && b.circAng1 == (int)((uint32_t)p.r[1] & 0xFFFF));
      CHECK(b.circR1 == std::sqrt(p.p[0] * p.p[0] + p.p[2] * p.p[2]));
      CHECK(b.circDY == b.circY1 - b.circY0 && b.circDR == b.circR1 - b.circR0);
      int v = (b.circAng1 - b.circAng0) & 0xFFFF; if (v >= 0x8000) v -= 0x10000;
      CHECK(b.circAngDelta == v - 0x30000);                               // dir > 0: -3 vueltas extra
      CHECK(b.circCnt == 0 && b.circTotal == 240.0f && (b.f1DA4 & B6A_CIRCLE) && !(b.f1DA4 & B6A_APPROACH));
      B6MoveState c; c.sel1DC4 = 0; c.sel1DC5 = 0; B6Move::setCircleMove(c, e, -1); CHECK(c.sel1DC4 == 3 && e.circleDir == -1);   // (0 - 1 + 4) % 4 = 3
      CHECK(c.circAngDelta == (((c.circAng1 - c.circAng0) & 0xFFFF) >= 0x8000 ? (((c.circAng1 - c.circAng0) & 0xFFFF) - 0x10000) : ((c.circAng1 - c.circAng0) & 0xFFFF)) + 0x30000);
      // CircleMove: t = cnt/240
      b.circCnt = 0; B6Move::circleMove(b, e);
      CHECK(b.circCnt == 1.0f && b.pathOut.pos[0] == (float)b.circAng0 && b.pathOut.pos[1] == b.circY0 && b.pathOut.pos[2] == -b.circR0);
      CHECK(b.pathOut.rot[0] == 0 && b.pathOut.rot[1] == b.circAng0 && b.pathOut.rot[2] == 0);
      B6MoveState d; d.circAng0 = 1000; d.circAngDelta = -2400; d.circY0 = 0; d.circDY = 24; d.circR0 = 10; d.circDR = 48; d.circCnt = 120; d.circTotal = 240;
      d.circAng1 = 5; d.circY1 = 1; d.circR1 = 2; d.f1DA4 = B6A_CIRCLE;
      B6Move::circleMove(d, e);   // t = 0.5: ang = 1000 + (-1200) = -200; y = 12; r = 34
      CHECK(d.pathOut.rot[1] == -200 && d.pathOut.pos[1] == 12 && d.pathOut.pos[2] == -34 && (d.f1DA4 & B6A_CIRCLE) && d.circCnt == 121);
      d.circCnt = 239; B6Move::circleMove(d, e);   // cnt pasa a 240: fija el destino y apaga el bit
      CHECK(d.pathOut.rot[1] == 5 && d.pathOut.pos[1] == 1 && d.pathOut.pos[2] == -2 && !(d.f1DA4 & B6A_CIRCLE)); }
    // CircleMove completo: 240 pasos y el bit se apaga exactamente al paso 240
    { M6 e; B6MoveState b; b.pos[0] = 30; b.pos[2] = -40; b.sel1DC5 = 1; B6Move::setCircleMove(b, e, 1); int n = 0;
      while ((b.f1DA4 & B6A_CIRCLE) && n < 1000) { B6Move::circleMove(b, e); n++; }
      CHECK(n == 240 && b.pathOut.rot[1] == b.circAng1 && b.pathOut.pos[1] == b.circY1); }
    // updatePath con circulo
    { M6 e; B6MoveState b; b.circAng0 = 0; b.circTotal = 10; b.circY1 = 3; b.circY0 = 0; b.circR1 = 4; b.f1DA4 = B6A_CIRCLE; B6Move::updatePath(b, e);
      CHECK(b.circCnt == 1 && b.pos[0] == b.pathOut.pos[0] && b.lastPath.rot[1] == b.pathOut.rot[1]); }

    // --- EffectCtrl: tipo 1, circulo y tick par
    { M6 e; B6MoveState b; b.kind = 1; b.f1DA4 = B6A_CIRCLE; b.effectTick = 4; B6Move::effectCtrl(b, e); CHECK(e.afterImages == 1);
      b.effectTick = 5; B6Move::effectCtrl(b, e); CHECK(e.afterImages == 1);
      b.effectTick = 6; b.kind = 2; B6Move::effectCtrl(b, e); CHECK(e.afterImages == 1);
      b.kind = 1; b.f1DA4 = 0; B6Move::effectCtrl(b, e); CHECK(e.afterImages == 1); }

    // --- CtrlPre
    { Steps6 st; B6MoveState b; b.pos[0] = 3; b.rot[2] = 8; b.obj0C[1] = 5; b.objScale[0] = 6; b.kind = 1; b.f1DA4 = B6A_LIFEBAR;
      B6Move::ctrlPre(b, st);
      std::vector<int> w = {B6S_CHECK_DAMAGE_END, B6S_DISPATCH_STATE, B6S_UPDATE_PATH, B6S_CHECK_DAMAGE, B6S_CONTROL_ATTACK, B6S_BODY_ALPHA, B6S_SET_ALPHA, B6S_LIFE_DISPLAY};
      CHECK(st.v == w && b.savedF[0] == 3 && b.savedF[4] == 5 && b.savedF[6] == 6 && b.savedI[2] == 8);
      Steps6 s2; b.kind = 0; b.f1DA4 = 0; B6Move::ctrlPre(b, s2); CHECK(s2.v.size() == 5);
      Steps6 s3; b.kind = 2; B6Move::ctrlPre(b, s3); CHECK(s3.v.size() == 7); }
}

static void test_boss6_bat() {
    using vn::boss6::Boss6State; using vn::boss6::Boss6Globals;
    // --- tablas (valores del asm; ver tambien la comparacion con el BIN al final)
    CHECK(kBatSpeedA_bits[0] == 0x3F99999Au && kBatSpeedA_bits[2] == 0x3F800000u && kBatSpeedA_bits[16] == 0x3F133333u);
    CHECK(kBatSpeedB_bits[1] == 0x3F8CCCCDu && kBatPathIds[0] == 0xD0 && kBatPathIds[16] == 0xE0);

    // --- SetApproach / Approach
    { B6BatState s; s.dest[0] = 10; s.dest[1] = 20; s.dest[2] = 30; B6Bat::approach(s); CHECK(s.pos[0] == 10 && s.pos[2] == 30);   // sin bit 8: pos = dest
      B6Bat::setApproach(s, 0.25f); CHECK((s.flags & BATF_APPROACH) && s.apprT == 0 && s.apprStep == 0.25f);
      s.pos[0] = 0; s.pos[1] = 0; s.pos[2] = 0;
      B6Bat::approach(s); CHECK(s.pos[0] == 0 && s.apprT == 0.25f);                    // t = 0: no se mueve
      B6Bat::approach(s); CHECK(s.pos[0] == 2.5f && s.pos[1] == 5 && s.apprT == 0.5f);
      B6Bat::approach(s);                                                               // t: 0.5 -> 0.75
      B6Bat::approach(s); CHECK(s.apprT == 1.0f && (s.flags & BATF_APPROACH));          // 1.0 <= 1.0: sigue
      B6Bat::approach(s); CHECK(s.apprT == 1.0f && s.apprStep == 0 && !(s.flags & BATF_APPROACH)); }

    // --- SetFallParam: x,z = 3r-1.5; y = 2 + r; apaga bits 8 y 1
    { MB e; e.rf = {0.0f, 1.0f, 0.5f}; B6BatState s; s.flags = BATF_APPROACH | BATF_ACTIVE | BATF_PATH;
      B6Bat::setFallParam(s, e); CHECK(s.fall[0] == -1.5f && s.fall[1] == 3.0f && s.fall[2] == 0.0f && s.flags == BATF_PATH); }
    // --- FallDown: gravedad 0xBDA740DA; el destino suma la velocidad
    { B6BatState s; s.fall[0] = 1; s.fall[1] = 2; s.fall[2] = 3; B6Bat::fallDown(s);
      CHECK(s.fall[1] == 2.0f + fb(0xBDA740DAu) && s.dest[0] == 1 && s.dest[1] == s.fall[1] && s.dest[2] == 3); }
    { B6BatState s; B6Bat::chgDead(s); CHECK(s.mode == BAT_DEAD); }

    // --- SetPathParam: ruta kBatPathIds[rand % 17]; paso = 1.2 * (1 / (velBase * kBatSpeedA[atkDiff]))
    { MB e; e.randV = 20; e.len = 77; Boss6State o; o.speedBase = 2.0f; o.atkDiff = 3; B6BatState s;
      B6Bat::setPathParam(s, o, e);
      CHECK(s.pathId == 0xD0 + 3 && s.pathT == 0 && s.pathLen == 77.0f && (s.flags & BATF_PATH));
      CHECK(s.pathStep == fb(0x3F99999Au) * (1.0f / (2.0f * fb(0x3F733333u)))); }
    // --- TraceShotPath
    { MB e; B6BatState s; s.pathLen = 10; s.pathT = 2; s.pathStep = 0.5f; s.pathId = 4; s.shotRatio = 0.5f;
      B6Bat::traceShotPath(s, e);
      // muestra = (1+2, 2, 3+4*1)=(3,2,7); z *= 0.5 -> 3.5; dest = v*2 = (6,4,7)
      CHECK(s.sample[0] == 3 && s.sample[2] == 3.5f && s.dest[0] == 6 && s.dest[1] == 4 && s.dest[2] == 7 && s.pathT == 2.5f);
      s.pathT = 10; B6Bat::traceShotPath(s, e); CHECK(s.mode == BAT_DEAD && s.pathT == 10); }
    // --- Espiral: SetSpiralParam / SetBatSpiral / AddBatSpiralCount / TraceSpiralPath
    { B6BatGlobals g; Boss6State o; o.speedBase = 1.0f; B6Bat::setBatSpiral(g, o, 2, 100.0f);
      float len = (50.0f * (200.0f * 1.0f * 1.0f)) / 60.0f;
      CHECK(g.spiralLen == len && g.spiralSlope == 100.0f / len && g.spiralStart == 0 && g.spiralRate == 1.0f && g.spiralHitFlag == 0);
      B6Bat::addBatSpiralCount(g); B6Bat::addBatSpiralCount(g); CHECK(g.spiralStart == 2.0f);
      o.gameMode = 2; o.target = 1; B6BatState s; s.index = 5; B6Bat::setSpiralParam(s, g, o);
      CHECK(s.gameMode == 2 && s.target == 1 && (s.flags & BATF_SPIRAL) && s.pathT == 0 && s.pathLen == g.spiralLen + 5.0f);
      MB e; g.spiralStart = 3.0f; s.pathLen = 5.5f; B6Bat::traceSpiralPath(s, g, e);
      float rad = fb(0x3E19999Au) + fb(0x3F59999Au) * ((5.5f - 3.0f) / g.spiralLen);
      int a = (3 - 5) * 1024;
      CHECK(s.sample[0] == 35.0f * (rad * (float)a) && s.sample[1] == 35.0f * (rad * (float)(a + 1)) && s.sample[2] == -g.spiralSlope * (5.5f - 3.0f));
      CHECK(s.dest[0] == s.sample[0] * 2 && s.pathT == 1.0f);
      g.spiralStart = 6.0f; B6Bat::traceSpiralPath(s, g, e); CHECK(s.mode == BAT_DEAD); }
    // --- RotInit: rand&0xFFFF x3 y (rand&0xFF + 0x200)*1.2
    { MB e; e.randV = 0x12345; B6BatState s; B6Bat::rotInit(s, e);
      CHECK(s.rotA == 0x2345 && s.rotB == 0x2345 && s.rotC == 0x2345 && s.rotSpeed == (int)(fb(0x3F99999Au) * (float)(0x45 + 0x200))); }

    // --- ChkMode
    { MB e; B6BatGlobals g; Boss6State o; B6BatState s;
      // modo 0 -> 1
      B6Bat::chkMode(s, g, o, e); CHECK(s.mode == BAT_WAIT);
      // modo 1, jefe sin atacar y sin 0x800000: no hace nada
      B6Bat::chkMode(s, g, o, e); CHECK(s.mode == BAT_WAIT && !(s.flags & BATF_APPROACH));
      // con 0x800000: acercamiento con paso 0.0001 + (0.001*(idx+1))^2
      s.index = 9; o.f1DA4 = 0x800000; B6Bat::chkMode(s, g, o, e);
      float k = fb(0x3A83126Fu) * 10.0f; CHECK((s.flags & BATF_APPROACH) && s.apprStep == fb(0x38D1B717u) + k * k);
      // atacando con atkFlag == 0: nada
      s.flags = 0; o.f1DA4 = vn::boss6::FA_ATTACK; o.atkFlag = 0; o.atkKind = 0; o.atkCount = 9; B6Bat::chkMode(s, g, o, e); CHECK(s.mode == BAT_WAIT && s.flags == 0);
      // disparo (atkKind 0): le toca (index == atkCount) -> modo 2, bits 1|0x10 y 2 (ruta), SetShotParam, sonido 0x1D000D
      o.atkFlag = 1; o.speedBase = 1; o.atkDiff = 0; e.randV = 0; B6Bat::chkMode(s, g, o, e);
      CHECK(s.mode == BAT_SHOT && (s.flags & (BATF_ACTIVE | BATF_SHOT | BATF_PATH)) == (BATF_ACTIVE | BATF_SHOT | BATF_PATH) && e.shotCalls == 1 && !e.snd.empty() && e.snd.back() == 0x1D000D);
      // no le toca: sigue en espera
      B6BatState t; t.mode = BAT_WAIT; t.index = 3; B6Bat::chkMode(t, g, o, e); CHECK(t.mode == BAT_WAIT && t.flags == 0);
      // espiral (atkKind 1): index en [start, start+rate)
      o.atkKind = 1; g.spiralStart = 4; g.spiralRate = 2; g.spiralLen = 10; B6BatState u; u.mode = BAT_WAIT; u.index = 5; B6Bat::chkMode(u, g, o, e);
      CHECK(u.mode == BAT_SPIRAL && (u.flags & BATF_SPIRAL) && (u.flags & BATF_APPROACH) && u.apprStep == fb(0x3CA3D70Au));
      B6BatState u2; u2.mode = BAT_WAIT; u2.index = 6; B6Bat::chkMode(u2, g, o, e); CHECK(u2.mode == BAT_WAIT);   // 6 no < 4+2
      B6BatState u3; u3.mode = BAT_WAIT; u3.index = 3; B6Bat::chkMode(u3, g, o, e); CHECK(u3.mode == BAT_WAIT);   // 3 < 4
      // modo 3: sigue mientras el jefe ataca; al acabar vuelve a 1 con acercamiento 0.02 y sin bits 4 y 1
      u.flags |= BATF_ACTIVE; o.f1DA4 = vn::boss6::FA_ATTACK; B6Bat::chkMode(u, g, o, e); CHECK(u.mode == BAT_SPIRAL);
      o.f1DA4 = vn::boss6::FA_ATTACK | 0x4000; B6Bat::chkMode(u, g, o, e); CHECK(u.mode == BAT_WAIT && !(u.flags & (BATF_SPIRAL | BATF_ACTIVE)));
      u.mode = BAT_SPIRAL; o.f1DA4 = 0; B6Bat::chkMode(u, g, o, e); CHECK(u.mode == BAT_WAIT);
      u.mode = BAT_SPIRAL; o.f1DA4 = vn::boss6::FA_DEAD; B6Bat::chkMode(u, g, o, e); CHECK(u.mode == BAT_FALL); }
    // vida agotada: modo 4, suma bits al jefe (con ataque y bandera 0x200000), aviso, caida, sonido 0x13001B
    { MB e; B6BatGlobals g; Boss6State o; o.f1DA4 = vn::boss6::FA_ATTACK | vn::boss6::FA_ATK_200000; B6BatState s; s.mode = BAT_WAIT; s.hp = 0; s.killBits = 0x10; s.alertSound = 1;
      B6Bat::chkMode(s, g, o, e);
      CHECK(s.mode == BAT_FALL && o.hitFlags == 0x10 && e.alerts.size() == 1 && e.alerts[0] == 0x1A && e.snd.size() == 1 && e.snd[0] == 0x13001B && s.fall[1] == 2.5f);
      // jefe muerto (0x40000): modo 4 sin sumar ni sonar
      MB e2; Boss6State o2; o2.f1DA4 = vn::boss6::FA_DEAD; B6BatState s2; s2.mode = BAT_WAIT; s2.hp = 5; B6Bat::chkMode(s2, g, o2, e2);
      CHECK(s2.mode == BAT_FALL && e2.snd.empty() && o2.hitFlags == 0);
      // modo 4: al bajar de Y < 0 -> modo 5 y apaga el aviso
      s.pos[1] = -1; s.alertSound = 1; B6Bat::chkMode(s, g, o, e); CHECK(s.mode == BAT_DEAD && s.alertSound == 0);
      s.mode = BAT_FALL; s.pos[1] = 0; B6Bat::chkMode(s, g, o, e); CHECK(s.mode == BAT_FALL);
      // modo 4/5 no vuelven a evaluar la muerte
      B6BatState d; d.mode = BAT_DEAD; d.hp = 0; B6Bat::chkMode(d, g, o, e); CHECK(d.mode == BAT_DEAD && e.snd.size() == 1); }

    // --- ExecMode
    { MB e; B6BatGlobals g; Boss6State o; B6BatState s; s.dest[0] = 5;
      s.mode = BAT_IDLE; B6Bat::execMode(s, g, o, e); CHECK(s.pos[0] == 5 && e.rotCalls == 0);       // siempre Approach
      s.mode = BAT_WAIT; B6Bat::execMode(s, g, o, e); CHECK(e.rotCalls == 1);
      s.mode = BAT_SHOT; s.pathLen = 10; s.pathT = 0; s.pathStep = 0.5f; B6Bat::execMode(s, g, o, e); CHECK(s.pathT > 0 || s.mode == BAT_DEAD);
      s.mode = BAT_SPIRAL; s.pathT = 0; s.flags = BATF_ACTIVE | BATF_SPIRAL; g.spiralHitFlag = 1; g.spiralStart = 0; s.pathLen = 3; g.spiralLen = 10; B6Bat::execMode(s, g, o, e);
      CHECK(!(s.flags & BATF_ACTIVE) && s.pathT == 1.0f);
      s.mode = BAT_FALL; s.dest[1] = 0; s.fall[1] = 1; B6Bat::execMode(s, g, o, e); CHECK(s.dest[1] != 0);
      s.mode = BAT_DEAD; s.index = 7; B6Bat::execMode(s, g, o, e); CHECK(s.deleted && e.spawned == 7);
      MB e2; B6BatState s2; s2.mode = BAT_DEAD; s2.index = 2; o.f1DA4 = vn::boss6::FA_DEAD; B6Bat::execMode(s2, g, o, e2); CHECK(s2.deleted && e2.spawned == -1); }

    // --- PlHitChk
    { MB e; B6BatGlobals g; Boss6State o; Boss6Globals og; og.gameMode = 1; og.diff[0] = 10; og.diff[1] = 10;
      B6BatState s; s.flags = BATF_ACTIVE | BATF_SPIRAL; s.hp = 1; s.radius = 3; s.target = 1; e.dist = 7.9f; e.dmg = 1;
      CHECK(B6Bat::plHitChk(s, g, o, og, e) == true);
      CHECK(e.snd.size() == 2 && e.snd[0] == 0x1D0011 && e.snd[1] == 0x800114 && og.diff[0] == 8 && og.diff[1] == 8 && g.spiralHitFlag == 1 && e.dmgCalls == 1);
      e = MB(); e.dist = 8.0f; g.spiralHitFlag = 0; CHECK(B6Bat::plHitChk(s, g, o, og, e) == false && e.snd.empty());    // 8 no < 8
      e = MB(); e.dist = 1; e.dmg = 0; CHECK(B6Bat::plHitChk(s, g, o, og, e) == true && e.snd.size() == 1 && og.diff[0] == 8);   // sin dano: no baja dificultad
      s.target = -1; e = MB(); e.dist = 1; e.dmg = 1; B6Bat::plHitChk(s, g, o, og, e); CHECK(e.dmgCalls == 0);
      s.hp = 0; CHECK(B6Bat::plHitChk(s, g, o, og, e) == false);
      s.hp = 1; s.flags = BATF_SPIRAL; CHECK(B6Bat::plHitChk(s, g, o, og, e) == false);
      s.flags = BATF_ACTIVE; g.spiralHitFlag = 0; B6Bat::plHitChk(s, g, o, og, e); CHECK(g.spiralHitFlag == 0); }

    // --- SetTargetID
    { MB e; Boss6Globals og; og.gameMode = 2; og.playersMode = 1; og.lastHitTarget = 1; int8_t t = 5, m = 0; B6Bat::setTargetID(t, m, og, e); CHECK(t == 1 && m == 2);
      og.playersMode = 2; e.randV = 3; B6Bat::setTargetID(t, m, og, e); CHECK(t == 1);
      e.randV = 2; B6Bat::setTargetID(t, m, og, e); CHECK(t == 0);
      og.playersMode = 0; t = 9; B6Bat::setTargetID(t, m, og, e); CHECK(t == 9); }
}

struct SkyEnv : B6SkyFadeEnv {
    int busy = 0, a = 0, b = 0; std::vector<float> alphas;
    int fadeBusy() override { return busy; } int skipFlagA() override { return a; } int skipFlagB() override { return b; }
    void setSkyAlpha(float v) override { alphas.push_back(v); }
};
static void test_effects() {
    // SkyFade modo 0: sube; llega a total: alfa 1.0 y borra
    { SkyEnv e; B6SkyFade s; s.total = 4; s.mode = 0;
      b6SkyFadeCtrl(s, e); CHECK(e.alphas.size() == 1 && e.alphas[0] == 0.0f && s.cnt == 1 && !s.deleted);
      b6SkyFadeCtrl(s, e); CHECK(e.alphas.back() == 0.25f && s.cnt == 2);
      s.cnt = 4; b6SkyFadeCtrl(s, e); CHECK(s.deleted && e.alphas.back() == 1.0f && s.cnt == 5); }
    // modo 1: baja (1 - ratio) y acaba en 0.0
    { SkyEnv e; B6SkyFade s; s.total = 4; s.mode = 1; s.cnt = 2; b6SkyFadeCtrl(s, e); CHECK(e.alphas[0] == 0.5f && !s.deleted);
      s.cnt = 4; b6SkyFadeCtrl(s, e); CHECK(e.alphas.back() == 0.0f && s.deleted); }
    // fundido ocupado: no avanza; con globales de salto fija el final y borra
    { SkyEnv e; e.busy = 1; B6SkyFade s; s.total = 4; b6SkyFadeCtrl(s, e); CHECK(s.cnt == 0 && e.alphas.empty() && !s.deleted);
      e.a = 1; b6SkyFadeCtrl(s, e); CHECK(s.deleted && e.alphas.back() == 1.0f);
      SkyEnv e2; e2.busy = 1; e2.b = 1; B6SkyFade s2; s2.mode = 1; b6SkyFadeCtrl(s2, e2); CHECK(s2.deleted && e2.alphas.back() == 0.0f); }
    // MaskFeather
    { B6MaskFeather s; s.cnt = 0; b6MaskFeatherPre(s); CHECK(s.dy == fb(0xBF19999Au) * -0.5f && s.dz == fb(0xBE99999Au) * -0.5f);
      s.cnt = 13; s.total = 13; b6MaskFeatherPre(s); CHECK(s.dy == fb(0xBF19999Au) * 0.5f);
      B6MaskFeather p; p.cnt = 11; b6MaskFeatherPost(p); CHECK(p.cnt == 12 && !p.deleted); b6MaskFeatherPost(p); CHECK(p.cnt == 13 && p.deleted); }
    // AfterImage: -0.02 por fotograma; borra con <= 0
    { B6AfterImage s; int n = 0; while (!s.deleted && n < 1000) { b6AfterImagePre(s); n++; } CHECK(n == 50 || n == 51);
      B6AfterImage t; t.alpha = 0.02f; b6AfterImagePre(t); CHECK(t.deleted && t.alpha <= 0.0f); }
}

// Comparacion de las tablas generadas con el BIN del overlay (si esta el disco): offset = direccion - 0x9C4800.
static void test_tables_vs_bin() {
    std::ifstream f("disc/TBL/BS6MOD.BIN", std::ios::binary);
    if (!f) { printf("bosses56_test: sin disc/TBL/BS6MOD.BIN, se omite la comparacion de tablas\n"); return; }
    auto rd = [&](uint32_t addr) { uint32_t v = 0; f.seekg(addr - 0x9C4800); f.read((char*)&v, 4); return v; };
    for (int i = 0; i < 17; i++) {
        CHECK(rd(0x9D4720 + 4 * i) == kBatSpeedA_bits[i]);
        CHECK(rd(0x9D47A8 + 4 * i) == kBatSpeedB_bits[i]);
        CHECK(rd(0x9D4830 + 4 * i) == (uint32_t)kBatPathIds[i]);
    }
    // constantes inmediatas de los murcielagos y rutas que se leyeron del asm (comprobacion cruzada con la instruccion lui/ori)
    CHECK(rd(0x9D4720) == 0x3F99999Au);
}

int main() {
    test_boss5();
    test_boss6_move();
    test_boss6_bat();
    test_effects();
    test_tables_vs_bin();
    if (fails) { printf("bosses56_test: %d fallos\n", fails); return 1; }
    printf("bosses56_test: OK\n");
    return 0;
}

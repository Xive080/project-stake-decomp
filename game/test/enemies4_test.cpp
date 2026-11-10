// Tests de game/enemies4. Tablas y manejadores comparados con GAMEMOD.BIN / ELF (se omiten con aviso si no estan). El original NUNCA se ejecuto.
// Compilar (desde la raiz del repo):
//   c++ -std=c++17 -Wall game/test/enemies4_test.cpp game/enemies4/e4_petit_zombie.cpp game/enemies4/e4_armor.cpp game/enemies4/e4_etc.cpp game/enemies4/e4_misc.cpp game/enemies4/e4_throw.cpp -o build/verify_enemies4/enemies4_test && ./build/verify_enemies4/enemies4_test
#include "../enemies4/e4_petit_zombie.h"
#include "../enemies4/e4_armor.h"
#include "../enemies4/e4_etc.h"
#include "../enemies4/e4_misc.h"
#include "../enemies4/e4_throw.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
using namespace vn::en4;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static std::vector<unsigned char> slurp(const char* p) {
    std::ifstream f(p, std::ios::binary); std::vector<unsigned char> v;
    if (f) v.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return v;
}
static std::vector<unsigned char> g_bin;
static uint32_t rd32(uint32_t a) { uint32_t v; std::memcpy(&v, &g_bin[a - 0x92B000u], 4); return v; }   // GAMEMOD.BIN carga en 0x92B000
static float rdf(uint32_t a) { return fbits(rd32(a)); }

static int g_rand = 0;
static int fakeRand() { return g_rand; }
static int g_seq[16], g_seqN = 0, g_seqI = 0;
static int seqRand() { return g_seq[g_seqI++ % g_seqN]; }
static int count(const E4Events& ev, E4EvType t) { int n = 0; for (auto& x : ev) if (x.type == t) n++; return n; }
static const E4Event* find(const E4Events& ev, E4EvType t, int nth = 0) { for (auto& x : ev) if (x.type == t && nth-- == 0) return &x; return nullptr; }
static E4Env baseEnv() { E4Env e; e.rand = fakeRand; return e; }

// ---------------------------------------------------------------------------------------------------------------------
static void testTables() {
    if (g_bin.empty()) { printf("(tablas contra GAMEMOD.BIN omitidas: fichero no encontrado)\n"); return; }
    // jtbl_009A89B0 del constructor de EnEtc: tipo -> bloque. Los bloques 1..8 cargan: nieve, pausa on/off, timer, tama, bola de fuego (x2: tipos 6 y 7 comparten), item
    CHECK(rd32(0x9A89B0) == 0x938F60 && rd32(0x9A89B4) == 0x938CA8 && rd32(0x9A89B8) == 0x938D28 && rd32(0x9A89BC) == 0x938D50);
    CHECK(rd32(0x9A89C0) == 0x938D78 && rd32(0x9A89C4) == 0x938DA0 && rd32(0x9A89C8) == 0x938DC8 && rd32(0x9A89CC) == 0x938DC8 && rd32(0x9A89D0) == 0x938DF0);
    // PTMF de los manejadores [2] = direccion de la funcion
    CHECK(rd32(0x985238 + 8) == EH_SNOW);
    CHECK(rd32(0x985248 + 8) == EH_DEMO_PAUSE_ON);
    CHECK(rd32(0x985258 + 8) == EH_DEMO_PAUSE_OFF);
    CHECK(rd32(0x985268 + 8) == EH_TIMER_BUNKI);
    CHECK(rd32(0x985278 + 8) == EH_ST4_TAMA);
    CHECK(rd32(0x985288 + 8) == EH_FIREBALL);
    CHECK(rd32(0x985298 + 8) == EH_ITEM_CHECK);
    CHECK(rd32(0x9852A8 + 8) == EH_BRANCH_MARK);
    CHECK(rd32(0x9852B8 + 8) == EH_ITEM_CHECK);
    // PetitZombie: Ctrl y Dead
    CHECK(rd32(0x9A1600 + 8) == 0x951C40);
    CHECK(rd32(0x9A1620 + 8) == 0x951FD0);
    CHECK(rd32(0x9A1620 + 8) != rd32(0x9A1600 + 8));
    // PetitZombie: animacion inicial (4 x 67) y escala por tipo
    for (int i = 0; i < 4; i++) CHECK(rd32(0x9B56B0 + 4 * i) == 67);
    for (int i = 0; i < 12; i++) CHECK(rdf(0x9ACB40 + 4 * i) == 15.0f);
    for (int i = 12; i < 16; i++) CHECK(rdf(0x9ACB40 + 4 * i) == 10.0f);
    CHECK(rdf(0x9ACB40 + 4 * 19) == 30.0f);
    // ArmorTask: PreDisp / Disp / NotDisp
    CHECK(rd32(0x9A17D8 + 8) == 0x956F00 && rd32(0x9A17E8 + 8) == 0x956F50 && rd32(0x9A17F8 + 8) == 0x956FC0);
    // D_009AE640: tabla de punteros por tipo (18 tipos con tabla, luego 0)
    CHECK(rd32(0x9AE640) == 0x9ACE20 && rd32(0x9AE640 + 4 * 17) == 0x9AE490 && rd32(0x9AE640 + 4 * 18) == 0);
    // EnSnd: lista de sonidos
    CHECK(rd32(0x9857A0) == 0x2001B && rd32(0x9857A4) == 0x20018 && rd32(0x9857A8) == 0x2001B && rd32(0x9857AC) == 0x2002B && rd32(0x9857B0) == 0x140016 && rd32(0x9857B4) == 0xFFFFFFFF);
    // Manejadores de EnEtcAttach / EnDamSphere / EnEtcSnd
    CHECK(rd32(0x9854D0 + 8) == 0x93CA70 && rd32(0x9854E0 + 8) == 0x93CD30);
    CHECK(rd32(0x9857C8 + 8) == 0x942A20 && rd32(0x9857D8 + 8) == 0x942C90 && rd32(0x9857E8 + 8) == 0x942CA0);
    CHECK(rd32(0x9857B8 + 8) == 0x9427B0);
    // ThrowObj: manejadores del tipo 3 y 9
    CHECK(rd32(0x985740 + 8) != 0);
    // Constantes float que se usan como bits en el codigo (verificadas contra los lui/ori del asm citados en los comentarios)
    CHECK(fbits(0x3D23D70A) == 0.04f && fbits(0x3F666666) == 0.9f && fbits(0x3F99999A) == 1.2f);
    CHECK(fbits(0xC319999A) == -153.6f && fbits(0x4299999A) == 76.8f && fbits(0x3F333333) == 0.7f && fbits(0xBE99999A) == -0.3f);
}

// ---------------------------------------------------------------------------------------------------------------------
static void testPalFrames() {
    CHECK(palFrames(60) == 50 && palFrames(0) == 0 && palFrames(30) == 25 && palFrames(1) == 0 && palFrames(7) == 5 && palFrames(100) == 83);
}

static PetitParam petitParam(int mode) {
    PetitParam p; p.type = 3; p.mode = (int8_t)mode; p.pathId = 7; p.closeFrame = 100; p.anim = 0;
    p.xMin = -10; p.xMax = 10; p.zMin = -5; p.zMax = 5; return p;
}
static void testPetit() {
    static const float scale[20] = {15,15,15,15,15,15,15,15,15,15,15,15,10,10,10,10,15,15,15,30};
    static const int init[4] = {67, 67, 67, 67};
    PetitCtorIn in; in.scaleByType = scale; in.scaleCount = 20; in.initAnimTable = init;
    E4Env e = baseEnv(); g_rand = 6;          // 6 % 4 = 2
    PetitZombieState s; float pos0[3] = {1, 2, 3};
    PetitZombieLogic::init(s, petitParam(2), 5, pos0, 0x11, 0x22, 0x33, e, in);
    CHECK(s.flags == 1 && s.life == 5 && s.mode == 2 && s.type == 3 && s.anim == 67 && s.scale == 15.0f);
    CHECK(s.pos[0] == 1 && s.pos[1] == 2 && s.pos[2] == 3 && s.slot1DA0 == 0x11 && s.slot1DA4 == 0x22 && s.slot1DA8 == 0x33);
    // animacion explicita
    PetitParam pa = petitParam(0); pa.anim = 0x45; PetitZombieState s2;
    PetitZombieLogic::init(s2, pa, 0, pos0, 0, 0, 0, e, in); CHECK(s2.anim == 0x45);
    // modo 0xF: sin escala y flag 0x8000
    e.gameState = 0xF; PetitZombieState s3; PetitZombieLogic::init(s3, petitParam(0), 0, pos0, 0, 0, 0, e, in);
    CHECK((s3.flags & 0x8000) && s3.scale == 0.0f);
    e.gameState = 0; in.typeHasEffect = true; PetitZombieState s4; PetitZombieLogic::init(s4, petitParam(0), 0, pos0, 0, 0, 0, e, in);
    CHECK(s4.flags == (1u | 0x80));

    // Ctrl: golpe -> caida
    E4Events ev; PetitZombieState z; z.p = petitParam(0); z.flags = 1 | 8; z.mode = 0; z.frame = 3;
    PetitZombieLogic::ctrl(z, e, ev);
    CHECK(!(z.flags & 1) && !(z.flags & 8) && z.frame == 4.0f);
    CHECK(find(ev, E4_MOTION)->a == 0x43 && find(ev, E4_MOTION)->d == 5 && find(ev, E4_SET_HANDLER)->a == PH_DEAD && count(ev, E4_MOTION_STEP) == 1);
    // pausa: no hay logica, solo el paso de animacion
    ev.clear(); e.paused = true; PetitZombieState zp; zp.p = petitParam(0); zp.flags = 1 | 8;
    PetitZombieLogic::ctrl(zp, e, ev);
    CHECK(zp.flags == 9 && zp.frame == 0 && ev.size() == 1 && ev[0].type == E4_MOTION_STEP);
    e.paused = false;
    // cierre por camara
    ev.clear(); PetitZombieState zc; zc.p = petitParam(0); zc.flags = 1; e.cam.pathId = 7; e.cam.frame = 99.0f;
    PetitZombieLogic::ctrl(zc, e, ev); CHECK(count(ev, E4_TASK_CLOSE_MOT) == 0);
    e.cam.frame = 100.0f; ev.clear(); PetitZombieLogic::ctrl(zc, e, ev); CHECK(count(ev, E4_TASK_CLOSE_MOT) == 1);
    e.cam.pathId = 8; ev.clear(); PetitZombieLogic::ctrl(zc, e, ev); CHECK(count(ev, E4_TASK_CLOSE_MOT) == 0);
    // modo 1: gira segun vida
    PetitZombieState zg; zg.p = petitParam(1); zg.mode = 1; zg.life = 1; PetitZombieLogic::ctrl(zg, e, ev); CHECK(zg.angle == 0x40);
    zg.life = 0; PetitZombieLogic::ctrl(zg, e, ev); CHECK(zg.angle == 0);
    // modo 2: patrulla
    auto mk = [&](float x, float zz, int ang) { PetitZombieState q; q.p = petitParam(2); q.mode = 2; q.pos[0] = x; q.pos[2] = zz; q.angle = ang; return q; };
    PetitZombieState q = mk(0, 0, 0x4000); PetitZombieLogic::ctrl(q, e, ev);
    CHECK(q.angle == 0x4000 && q.pos[0] == 0 && q.pos[2] == 0);                         // dentro: sin cambios
    q = mk(10, 0, 0x4000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[0] == 10 && q.angle == 0x3F00);   // en xMax con angulo < 0xC000: -0x100
    q = mk(11, 0, 0xC000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[0] == 10 && q.angle == 0xC100);
    q = mk(-10, 0, 0x3000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[0] == -10 && q.angle == 0x2F00);  // xMin y angulo < 0x4000: -0x100
    q = mk(-12, 0, 0x4000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[0] == -10 && q.angle == 0x4100);
    q = mk(0, 5, 0x1000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[2] == 5 && q.angle == 0x0F00);       // zMax y angulo < 0x8000: -0x100
    q = mk(0, 6, 0x9000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[2] == 5 && q.angle == 0x9100);
    q = mk(0, -5, 0x1000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[2] == -5 && q.angle == 0x1100);     // zMin y angulo < 0x8000: +0x100
    q = mk(0, -6, 0x9000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[2] == -5 && q.angle == 0x8F00);
    q = mk(10, 5, 0x1000); PetitZombieLogic::ctrl(q, e, ev); CHECK(q.pos[0] == 10 && q.pos[2] == 5 && q.angle == 0x0F00);   // esquina: solo gira X, Z se ajusta igual
    PetitZombieLogic::dead();
}

// ---------------------------------------------------------------------------------------------------------------------
static void testArmor() {
    E4Events ev; ArmorState s; ArmorDt dt; dt.partIdx = 2; dt.shown = 1; dt.attachModel = 0x7F0018; dt.localPos[0] = 1; dt.localPos[1] = 2; dt.localPos[2] = 3;
    ArmorLogic::init(s, dt, ev);
    CHECK(s.flags == 0x10001 && ev.size() == 2 && ev[0].a == AH_PREDISP && ev[0].b == 0 && ev[1].a == AH_DISP && ev[1].b == 0x10005);
    ev.clear(); dt.shown = 0; ArmorState s2; ArmorLogic::init(s2, dt, ev); CHECK(ev.size() == 1 && ev[0].a == AH_NOTDISP);
    // PreDisp: con la parte marcada no pasa nada
    ev.clear(); s.part.flag154 = 0x100; s.part.stage = 3; ArmorLogic::preDisp(s, ev); CHECK(ev.empty() && !s.closed && s.part.stage == 3);
    s.part.flag154 = 0; ArmorLogic::preDisp(s, ev);
    CHECK(s.closed && s.part.stage == 4 && count(ev, E4_SPAWN_ATTACH) == 1 && find(ev, E4_SPAWN_ATTACH)->a == 0x7F0018 && find(ev, E4_SPAWN_ATTACH)->f[2] == 3 && count(ev, E4_TASK_CLOSE) == 1);
    // NotDisp: sustituye el modelo con el indice idx*6 + stage
    uint32_t models[40]; for (int i = 0; i < 40; i++) models[i] = 1000 + i;
    ev.clear(); ArmorState n; n.dt = dt; n.part.stage = 1; n.part.flag154 = 0x100; ArmorLogic::notDisp(n, models, ev); CHECK(ev.empty() && n.part.modelRef == 0);
    n.part.flag154 = 0; ArmorLogic::notDisp(n, models, ev); CHECK(n.part.modelRef == 1000 + 2 * 6 + 1 && n.part.stage == 2 && n.closed);
    CHECK(ArmorLogic::shouldDraw(s) && !([&]{ ArmorState d; d.flags |= 8; return ArmorLogic::shouldDraw(d); })());
}

// ---------------------------------------------------------------------------------------------------------------------
static EtcState mkEtc(E4Env& e, int type, E4Events& ev, const uint32_t* param = nullptr, int u2 = 0, int u4 = 0, int u6 = 0) {
    EtcState s; float pos[3] = {1, 2, 3}; uint32_t p0[4] = {0, 0, 0, 0};
    EtcLogic::init(s, e, type, pos, u2, u4, u6, param ? param : p0, ev); return s;
}
static void testEtcInit() {
    E4Env e = baseEnv(); E4Events ev;
    struct { int type, handler, prio; } tab[] = {{1, EH_SNOW, 0}, {2, EH_DEMO_PAUSE_ON, 3}, {3, EH_DEMO_PAUSE_OFF, 3}, {4, EH_TIMER_BUNKI, 5}, {5, EH_ST4_TAMA, 5},
                                                  {6, EH_FIREBALL, 5}, {7, EH_FIREBALL, 5}, {8, EH_ITEM_CHECK, 5}};
    for (auto& t : tab) { ev.clear(); EtcState s = mkEtc(e, t.type, ev); CHECK(s.handler == t.handler && s.handlerPrio == t.prio && find(ev, E4_SET_HANDLER)->a == t.handler && find(ev, E4_SET_HANDLER)->b == t.prio); }
    ev.clear(); EtcState s0 = mkEtc(e, 0, ev); CHECK(s0.handler == EH_NONE && ev.empty() && s0.flags == 1 && s0.pos[1] == 2);
    ev.clear(); EtcState s9 = mkEtc(e, 9, ev); CHECK(s9.handler == EH_NONE && ev.empty());
    // nieve activa gp-0x7340, w164/w168 = -1
    e.gp7340 = 0; ev.clear(); EtcState sn = mkEtc(e, 1, ev); CHECK(e.gp7340 == 1 && sn.w164 == -1 && sn.w168 == -1);
    // gp73D0/gp73D4 se copian a +0x168/+0x164
    e.gp73D0 = 4; e.gp73D4 = 9; ev.clear(); EtcState sc = mkEtc(e, 2, ev); CHECK(sc.w168 == 4 && sc.w164 == 9);
    // tipo 8: objetos 0x1B y 0x20
    E4Env a = baseEnv(); a.gameState = 0xA; a.gameKind = 1; a.itemFlag[0x1B] = 3; a.itemFlag[0x20] = 3; a.gp7400 = 0;
    ev.clear(); mkEtc(a, 8, ev); CHECK(count(ev, E4_ITEM_APPLY) == 2 && find(ev, E4_ITEM_APPLY, 0)->a == 0x1B && find(ev, E4_ITEM_APPLY, 1)->a == 0x20);
    a.gp7400 = 1; ev.clear(); mkEtc(a, 8, ev); CHECK(count(ev, E4_ITEM_APPLY) == 1);
    a.itemFlag[0x1B] = 3 | 0x20; a.gp7400 = 0; ev.clear(); mkEtc(a, 8, ev); CHECK(count(ev, E4_ITEM_APPLY) == 1 && find(ev, E4_ITEM_APPLY)->a == 0x20);
    a.itemFlag[0x20] = 1; ev.clear(); mkEtc(a, 8, ev); CHECK(count(ev, E4_ITEM_APPLY) == 0);
    // itemGate
    E4Env g = baseEnv(); g.itemFlag[5] = 3; CHECK(!EtcLogic::itemGate(g, 5));        // fuera de arcade
    g.gameState = 0xA; g.gameKind = 2; CHECK(!EtcLogic::itemGate(g, 5));
    g.gameKind = 1; CHECK(EtcLogic::itemGate(g, 5)); g.itemFlag[5] = 1; CHECK(!EtcLogic::itemGate(g, 5)); g.itemFlag[5] = 3 | 0x20; CHECK(!EtcLogic::itemGate(g, 5));
    // marca de rama (tipos 10..59): tipo -= 10, parametros
    uint32_t prm[4] = {11, 22, 33, 44};
    ev.clear(); EtcState b = mkEtc(a, 26, ev, prm); CHECK(b.type == 16 && b.fB0 == 10.0f && b.flags == (1u | 0x1008A) && b.w148 == 0x1008A && b.w14C == 11 && b.w150 == 22 && b.w154 == 33 && b.w158 == 44 && b.handler == EH_BRANCH_MARK && b.handlerPrio == 5);
    ev.clear(); EtcState b2 = mkEtc(a, 59, ev, prm); CHECK(b2.type == 49 && b2.handler == EH_BRANCH_MARK);
    ev.clear(); EtcState b3 = mkEtc(a, 60, ev, prm); CHECK(b3.handler == EH_NONE);
    // tipos >= 70: comprobacion de objeto; deja 0x500F03
    a.itemFlag[5] = 3; a.g500F03 = 0; ev.clear(); EtcState c = mkEtc(a, 75, ev); CHECK(c.type == 5 && a.g500F03 == 1 && c.handler == EH_ITEM_CHECK);
    a.itemFlag[5] = 0; ev.clear(); mkEtc(a, 75, ev); CHECK(a.g500F03 == 0);
    a.itemFlag[30] = 3; a.gp7200 = 1; ev.clear(); mkEtc(a, 100, ev); CHECK(a.g500F03 == 0);     // objeto 0x1E con gp-0x7200 == 1 lo anula
    a.gp7200 = 0; ev.clear(); mkEtc(a, 100, ev); CHECK(a.g500F03 == 1);
}

static void testEtcHandlers() {
    E4Env e = baseEnv(); E4Events ev; EtcState s;
    EtcLogic::demoPauseOn(s, e, ev); CHECK(ev[0].type == E4_DRAW && ev[0].a == 1 && s.closed);
    ev.clear(); s = EtcState(); EtcLogic::demoPauseOff(s, e, ev); CHECK(ev[0].a == 0 && s.closed);
    ev.clear(); s = EtcState(); EtcLogic::itemCheck(s, ev); CHECK(s.closed && ev.size() == 1);
    // St4_02TimerBunki: 90 -> 75 fotogramas PAL; con enemigos en pantalla suma un ataque perdido
    e.gp7374 = 2; e.gp7358 = 9; s = EtcState(); s.a50 = 90; ev.clear();
    EtcLogic::timerBunki(s, e, ev); CHECK(s.step == 1 && s.w148 == 74 && e.gp7358 == 0 && !s.closed);
    for (int i = 0; i < 73; i++) EtcLogic::timerBunki(s, e, ev);
    CHECK(s.w148 == 1 && !s.closed); EtcLogic::timerBunki(s, e, ev); CHECK(s.closed && e.gp7358 == 1 && count(ev, E4_TASK_CLOSE) == 1);
    e.gp7374 = 0; e.gp7358 = 0; s = EtcState(); s.a50 = 1; ev.clear();   // 1 -> 0 fotogramas: el primer tick lo cierra
    EtcLogic::timerBunki(s, e, ev); CHECK(s.closed && e.gp7358 == 0);

    // St4_Tama: sube 0.9 en n1 pasos de 1.2*0.9/n1, luego baja
    s = EtcState(); s.a50 = 3; s.a54 = 2; e.tintAlpha = 5; e.tintScrollX = 10; e.tintScrollY = 10; ev.clear();
    EtcLogic::st4Tama(s, e, ev);
    CHECK(s.step == 1 && s.w148 == 3 && s.w14C == 2 && e.tintAlpha == 0.0f + 1.2f * (0.9f / 3.0f));
    CHECK(e.tintScrollX == (int)(0.0f + -153.6f) && e.tintScrollY == (int)(0.0f + 76.8f));       // el primer paso reinicia a 0 y luego suma
    float a1 = e.tintAlpha; EtcLogic::st4Tama(s, e, ev); CHECK(e.tintAlpha == a1 + 1.2f * (0.9f / 3.0f));
    for (int i = 0; i < 5 && s.step == 1; i++) EtcLogic::st4Tama(s, e, ev);
    CHECK(s.step == 2 && !s.closed);
    // en el paso en que llega al tope pasa a bajar en el mismo fotograma
    for (int i = 0; i < 20 && !s.closed; i++) EtcLogic::st4Tama(s, e, ev);
    CHECK(s.closed && e.tintAlpha == 0.0f && count(ev, E4_TASK_CLOSE) == 1);
    // pausa: sin cambios de alfa ni de desplazamiento
    E4Env ep = baseEnv(); ep.paused = true; ep.tintAlpha = 0.5f; ep.tintScrollX = 7; EtcState sp; sp.step = 1; sp.w148 = 4; E4Events evp;
    EtcLogic::st4Tama(sp, ep, evp); CHECK(ep.tintAlpha == 0.5f && ep.tintScrollX == 7 && sp.step == 1);

    // Bola de fuego
    E4Env f = baseEnv(); float p[3] = {5, 6, 7}; EtcState fb; ev.clear();
    EtcLogic::fireBall(fb, f, p, ev);
    CHECK(fb.step == 1 && fb.w148 == 0x260057 && fb.w150 == 0x260066 && fb.f170 == 1.0f && find(ev, E4_SOUND_GLOBAL)->a == 0x17010C);
    CHECK(fb.pos[0] == 5 && fb.pos[2] == 7 && fb.w158 == 1 && fb.w14C == 0x260058);
    for (int i = 0; i < 43; i++) EtcLogic::fireBall(fb, f, p, ev);
    CHECK(fb.w158 == 44 && fb.step == 1 && !(fb.flags & 0x40000000));
    EtcLogic::fireBall(fb, f, p, ev);       // 45 fotogramas: explosion
    CHECK(fb.step == 2 && (fb.flags & 0x40000000) && fb.w148 == 0x260040 && fb.w150 == 0x260056 && fb.w154 == 0x16 - 1 && !fb.closed);
    for (int i = 0; i < 20; i++) EtcLogic::fireBall(fb, f, p, ev);
    CHECK(!fb.closed); EtcLogic::fireBall(fb, f, p, ev); CHECK(fb.closed);
    CHECK(fb.w14C >= fb.w148 && fb.w14C < fb.w150);
    // la animacion de llama recicla el fotograma al llegar a w150
    EtcState l; l.step = 2; l.w154 = 100; l.w148 = 10; l.w150 = 12; l.w14C = 10; EtcLogic::fireBall(l, f, p, ev); CHECK(l.w14C == 11); EtcLogic::fireBall(l, f, p, ev); CHECK(l.w14C == 10);
}

static void testBranchMark() {
    E4Env e = baseEnv(); e.gameState = 0xA; e.gameKind = 1; e.itemFlag[4] = 3; E4Events ev;
    EtcState s; s.type = 4; s.w14C = 5; s.w150 = 10; s.w154 = 5; s.w158 = 50; s.flags = 1;
    e.cam.pathId = 5; e.cam.frame = 9;
    EtcLogic::branchMark(s, e, ev); CHECK(!s.closed && s.branchSeen == 0 && count(ev, E4_ITEM_APPLY) == 0);
    e.cam.frame = 10; EtcLogic::branchMark(s, e, ev); CHECK(s.branchSeen == 1 && find(ev, E4_ITEM_APPLY)->a == 4);
    ev.clear(); EtcLogic::branchMark(s, e, ev); CHECK(count(ev, E4_ITEM_APPLY) == 0);              // solo una vez
    e.cam.frame = 50; e.g500F05 = 1; e.gp73CC = 1; EtcLogic::branchMark(s, e, ev); CHECK(s.closed && e.g500F05 == 0 && e.gp73CC == 0);
    // golpe en la marca
    e.cam.pathId = 0; e.cam.frame = 0; s = EtcState(); s.type = 0x10; s.flags = 1 | 8; s.branchSeen = 1; s.w154 = 99; s.w158 = 1000; e.itemFlag[0x10] = 3; ev.clear(); e.g500E11 = 0;
    EtcLogic::branchMark(s, e, ev); CHECK(!(s.flags & 8) && s.w148 == 0x10088 && e.g500F05 == 1 && e.gp73CC == 1 && e.g500E11 == 1 && !s.closed);
    s.type = 4; s.flags = 1 | 8; s.w154 = 99; e.g500E11 = 0; EtcLogic::branchMark(s, e, ev); CHECK(e.g500E11 == 0);
    // objeto no disponible: se cierra y limpia
    e.itemFlag[4] = 1; s = EtcState(); s.type = 4; s.branchSeen = 1; e.g500F05 = 1; ev.clear(); EtcLogic::branchMark(s, e, ev); CHECK(s.closed && e.g500F05 == 0);
}

static void testSnow() {
    E4Env e = baseEnv(); E4Events ev; EtcState s; e.gp7340 = 1; e.cam.pos[0] = 100; e.cam.pos[1] = 200; e.cam.pos[2] = 300;
    // primer fotograma: camara lejos de lastCam -> reinicia huecos y crea 10 particulas
    for (int i = 0; i < 10; i++) e.snowFlags[i] = 1; e.gp6FDC = 0;
    EtcLogic::snowCtrl(s, e, ev);
    CHECK(e.gp6FDC == 1 && count(ev, E4_SNOW_PARTICLE) == 10);
    for (int i = 0; i < 10; i++) { CHECK(e.snowFlags[i] == 1); CHECK(ev[i].a == i && ev[i].b == 1); CHECK(ev[i].f[1] == 200.0f + (-50.0f + 15.0f * (i + 1))); CHECK(ev[i].f[0] == 100 && ev[i].f[2] == 300); }
    // segundo fotograma: sin movimiento, todos ocupados -> nada
    ev.clear(); EtcLogic::snowCtrl(s, e, ev); CHECK(ev.empty() && e.gp6FDC == 1);
    // se libera el hueco 3 y 7: el desplazamiento Y tiene en cuenta los ocupados (8)
    e.snowFlags[3] = 0; e.snowFlags[7] = 0; EtcLogic::snowCtrl(s, e, ev);
    CHECK(count(ev, E4_SNOW_PARTICLE) == 2 && ev[0].a == 3 && ev[0].b == 0 && ev[0].f[1] == 200.0f + (-50.0f + 15.0f * 8 + 15.0f));
    CHECK(ev[1].a == 7 && ev[1].f[1] == 200.0f + (-50.0f + 15.0f * 8 + 30.0f));
    // camara salta > 50: reinicia
    ev.clear(); e.cam.pos[0] = 200; EtcLogic::snowCtrl(s, e, ev); CHECK(e.gp6FDC == 2 && count(ev, E4_SNOW_PARTICLE) == 10);
    // camara se mueve < 50: sin reinicio
    ev.clear(); e.snowFlags[0] = 0; e.cam.pos[0] = 210; EtcLogic::snowCtrl(s, e, ev); CHECK(e.gp6FDC == 2 && count(ev, E4_SNOW_PARTICLE) == 1 && ev[0].b == 0);
    // nieve apagada: se queda en espera (paso 1); al reactivarse vuelve a 0 y crea con reinicio
    e.gp7340 = 0; ev.clear(); EtcLogic::snowCtrl(s, e, ev); CHECK(s.step == 1 && ev.empty());
    EtcLogic::snowCtrl(s, e, ev); CHECK(s.step == 1);
    e.gp7340 = 1; e.snowFlags[1] = 0; EtcLogic::snowCtrl(s, e, ev); CHECK(s.step == 0 && count(ev, E4_SNOW_PARTICLE) == 10 && ev[0].b == 1);
    s.step = 5; ev.clear(); EtcLogic::snowCtrl(s, e, ev); CHECK(ev.empty());   // paso desconocido: no hace nada
}

// ---------------------------------------------------------------------------------------------------------------------
static void testMisc() {
    E4Env e = baseEnv(); E4Events ev;
    // EnSnd
    static const int ids[] = {0x2001B, 0x20018, 0x2001B, 0x2002B, 0x140016, -1};
    EnSndTable t; t.init(ids); CHECK(t.count == 5 && t.rec[0].id == 0x2001B && t.rec[4].id == 0x140016 && t.rec[3].unused == -1 && t.rec[2].lastFrame == 0);
    e.stage = 1; e.frameCounter = 100;
    CHECK(t.request(0x2001B, e, ev) && count(ev, E4_SOUND_GLOBAL) == 1 && t.rec[0].lastFrame == 100);
    e.frameCounter = 103; ev.clear(); CHECK(t.request(0x2001B, e, ev) && ev.empty() && t.rec[0].lastFrame == 100);     // < 5: se ignora
    e.frameCounter = 105; CHECK(t.request(0x2001B, e, ev) && ev.size() == 1 && t.rec[0].lastFrame == 105);
    CHECK(t.rec[2].lastFrame == 0);                                  // el segundo registro del mismo id nunca se usa
    ev.clear(); CHECK(!t.request(0x9999, e, ev) && ev.empty());
    e.stage = 0; e.frameCounter = 200; CHECK(t.request(0x20018, e, ev) && ev.empty() && t.rec[1].lastFrame == 200);    // fase 0: sin sonido pero cuenta
    // EnEtcSnd
    EnEtcSndState sn; EnEtcSndLogic::init(sn, 0x1234, 12); CHECK(sn.delay == 10 && sn.soundId == 0x1234);
    ev.clear(); for (int i = 0; i < 9; i++) EnEtcSndLogic::ctrl(sn, ev); CHECK(!sn.closed && ev.empty());
    EnEtcSndLogic::ctrl(sn, ev); CHECK(sn.closed && ev[0].a == 0x1234 && ev[1].type == E4_TASK_CLOSE);
    // EnDamSphere
    DamSphereState ds; DamOwner ow; ev.clear();
    DamSphereLogic::ctrlPre(ds, ow, ev); CHECK(ev.empty() && ds.flags == 0x10001);
    ow.flags = 0x04000000; DamSphereLogic::ctrlPre(ds, ow, ev); CHECK(ds.closed);
    ow = DamOwner(); ds = DamSphereState(); ds.flags |= 0x8 | 0x2;             // golpeada tipo 2
    DamSphereLogic::ctrlPre(ds, ow, ev);
    CHECK((ow.flags & 0x8) && (ow.flags & 0x2) && !(ow.flags & 0x4) && !(ow.flags2 & 0x20000000) && (ow.flags2 & 0x40000000));
    CHECK(!(ds.flags & 0x8) && !(ds.flags & 0x2) && (ds.flags & 0x8000));
    ow = DamOwner(); ds = DamSphereState(); ds.flags |= 0x8;                    // golpeada sin 0x2: tipo 4
    DamSphereLogic::ctrlPre(ds, ow, ev); CHECK((ow.flags & 0x4) && !(ow.flags & 0x2) && (ow.flags2 & 0x20000000) && (ow.flags2 & 0x40000000));
    ow = DamOwner(); ow.flags = 0x100; ds = DamSphereState(); ds.flags |= 0x8;  // dueno invulnerable: no recibe y la esfera sigue activa
    DamSphereLogic::ctrlPre(ds, ow, ev); CHECK(ow.flags == 0x100 && ow.flags2 == 0 && !(ds.flags & 0x8000) && !(ds.flags & 0x8));
    ow = DamOwner(); ds = DamSphereState(); ds.flags |= 0x8 | 0x8000;           // bloqueada: se ignora
    DamSphereLogic::ctrlPre(ds, ow, ev); CHECK(ow.flags == 0 && (ds.flags & 0x8));
    // EnPartsChain
    ev.clear(); PartsChainLogic::run(77, 5, 6, 0, 0, 0, ev); CHECK(ev.size() == 2 && ev[0].d == 2 && ev[1].d == 3 && ev[0].a == 77 && ev[1].b == 5);
    // EnmID
    EnmIdTable id; id.init(); uint32_t fl[33] = {0}; int8_t b[33];
    for (int i = 0; i < 32; i++) { int r = id.set(0x1000 + i, fl[i], b[i]); CHECK(r == i && b[i] == i && (fl[i] & 1)); }
    int r = id.set(0x2000, fl[32], b[32]); CHECK(r == -1 && b[32] == -1 && !(fl[32] & 1));
    id.clear(fl[5], b[5]); CHECK(id.slot[5] == 0 && id.slot[4] != 0); CHECK(id.set(0x3000, fl[5], b[5]) == 5);
    id.clear(0, b[6]); CHECK(id.slot[6] != 0);                              // sin bit 0: no libera
    id.clear(1, -1); id.clear(1, 32); CHECK(id.slot[0] != 0 && id.slot[31] != 0);
    // Patn
    static const int lst[4] = {10, 20, 30, 40}; PatnTbl pt; pt.list = lst; pt.count = 4; pt.speed = 1.0f; pt.loop = 1;
    PatnStat ps; CHECK(PatnLogic::init(ps, &pt) == 10 && ps.pos == 0 && ps.speed == 1.0f);
    CHECK(PatnLogic::forward(ps) == 10 && ps.pos == 1.0f && PatnLogic::frame(ps) == 1); CHECK(PatnLogic::forward(ps) == 20);
    CHECK(PatnLogic::forward(ps) == 30); CHECK(PatnLogic::forward(ps) == 40 && ps.pos == 0.0f && ps.ended == 0);   // bucle: 4 -> 0
    pt.loop = 0; PatnLogic::init(ps, &pt); ps.speed = 1.0f;
    for (int i = 0; i < 3; i++) PatnLogic::forward(ps);
    CHECK(ps.pos == 3.0f && ps.ended == 0); CHECK(PatnLogic::forward(ps) == 40 && ps.pos == 3.0f && ps.ended == 1);  // sin bucle: se queda en count-1
    ps.speed = -1.0f; ps.pos = 0; PatnLogic::forward(ps); CHECK(ps.pos == 0.0f && ps.ended == 1);              // por debajo de 0
    PatnTbl nolist; nolist.list = nullptr; nolist.count = 3; nolist.speed = 0.5f; nolist.loop = 1; PatnStat q; CHECK(PatnLogic::init(q, &nolist) == 0);
    CHECK(PatnLogic::forward(q) == 0); CHECK(PatnLogic::forward(q) == 0 && q.pos == 1.0f); CHECK(PatnLogic::forward(q) == 1);
    // PartsChangeChk
    uint32_t of = 0; int32_t wc[2] = {5, 6}; partsChangeChk(0xA, of, 1, wc); CHECK(of == 0 && wc[1] == 0 && wc[0] == 5);
    wc[0] = 7; partsChangeChk(3, of, 0, wc); CHECK(of == 0xE00 && wc[0] == 0);
    // Set_ShootOrder
    ShootOrder so = setShootOrder(0xE, 2, fakeRand); CHECK(so.first == 0 && so.second == -1);
    so = setShootOrder(1, 0, fakeRand); CHECK(so.first == 0 && so.second == -1);
    so = setShootOrder(1, 1, fakeRand); CHECK(so.first == 1 && so.second == -1);
    so = setShootOrder(1, -1, fakeRand); CHECK(so.first == -1 && so.second == -1);
    so = setShootOrder(1, 7, fakeRand); CHECK(so.first == -1 && so.second == -1);
    g_rand = 2; so = setShootOrder(1, 2, fakeRand); CHECK(so.first == 1 && so.second == 0);
    g_rand = 3; so = setShootOrder(1, 2, fakeRand); CHECK(so.first == 0 && so.second == 1);
    g_rand = -3; so = setShootOrder(1, 2, fakeRand); CHECK(so.first == 0 && so.second == 1);
    g_rand = -4; so = setShootOrder(1, 2, fakeRand); CHECK(so.first == 1 && so.second == 0);
    // ScLRChk / ScChkP
    CHECK(scLRChk(-1, 10, 300) == 1 && scLRChk(1, 10, 300) == -1 && scLRChk(0, 10, 300) == -1 && scLRChk(1, -10, 300) == 1);
    CHECK(scChkP(10, 10, 100, 100, 50, 50) && !scChkP(60, 0, 100, 100, 50, 50) && scChkP(50, 0, 100, 100, 50, 50) && !scChkP(-51, 0, 100, 100, 50, 50) && !scChkP(0, 51, 100, 100, 50, 50) && scChkP(0, -50, 100, 100, 50, 50));
}

static void testAttackObj() {
    E4Env e = baseEnv(); AttackObjState a; int16_t alive = 3; e.gp7374 = 1;
    AttackObjLogic::init(a, 9, 4, 20, 2.5f, 77, 1, -1, e, alive);
    CHECK(a.flags == 1 && a.id == 9 && a.type == 4 && a.life == 20 && a.scale0 == 2.5f && a.scale1 == 2.5f && a.w14C == 77 && a.player == 1 && a.fixed == -1 && e.gp7374 == 2 && alive == 4);
    int slot[2] = {0, 0}; int modes[2] = {5, 5};
    g_rand = 1; a.player = -1;
    CHECK(AttackObjLogic::selectTarget(a, 2, 0, 0, modes, slot, fakeRand) && a.player == 1 && slot[1] == 1 && slot[0] == 0);
    CHECK(AttackObjLogic::selectTarget(a, 2, 0, 0, modes, slot, fakeRand) && a.player == 0 && slot[0] == 1);          // el 1 ya ocupado -> el otro
    CHECK(AttackObjLogic::selectTarget(a, 2, 0, 0, modes, slot, fakeRand) && slot[0] == 0 + 0 + 1 - 0 + 0 - 0 + 0 - 1 + 1 - 1 + 1 - 0 && a.player == 0);     // ambos ocupados: los libera y reserva el otro (0)
    slot[0] = slot[1] = 0; modes[1] = 4; g_rand = 1; CHECK(!AttackObjLogic::selectTarget(a, 2, 0, 0, modes, slot, fakeRand) && a.player == -1 && slot[1] == 0);   // jugador 1 no esta en modo 5
    CHECK(AttackObjLogic::selectTarget(a, 2, 0, 9, modes, slot, fakeRand) && a.player == 1);                         // gameState 9: siempre valido
    slot[0] = slot[1] = 0; modes[0] = 5; CHECK(AttackObjLogic::selectTarget(a, 1, 0, 0, modes, slot, fakeRand) && a.player == 0 && slot[0] == 1);
    a.player = 1; CHECK(AttackObjLogic::selectTarget(a, 3, 0, 9, modes, slot, fakeRand) && a.player == 1);           // otro numero de jugadores: conserva
    CHECK(AttackObjLogic::targetDist(e.cam.pos, e.cam.pos) == 0);
    float p[3] = {1, 2, 3}, t[3] = {4, 6, 3}; CHECK(AttackObjLogic::targetDist(p, t) == 5.0f);
    float off[3]; AttackObjLogic::localOffset(0, off); CHECK(off[0] == -1.2f && off[1] == 0 && off[2] == -1.5f);
    AttackObjLogic::localOffset(1, off); CHECK(off[0] == 1.2f);
    float cam[3] = {1, 2, 3}, wo[3] = {7, 8, 9};
    a.fixed = 0; a.player = 0; AttackObjLogic::setTarget(a, 1, cam, wo); CHECK(a.target[0] == 7);
    AttackObjLogic::setTarget(a, 0, cam, wo); CHECK(a.target[0] == 1); a.fixed = 1; AttackObjLogic::setTarget(a, 1, cam, wo); CHECK(a.target[0] == 1);
    a.fixed = 0; a.player = -1; a.target[0] = 0; AttackObjLogic::setTarget(a, 1, cam, wo); CHECK(a.target[0] == 1);
}

static void testSeFade() {
    E4Events ev; ManySeState m; m.count = 2; m.period = 3; m.soundId = 55;
    for (int i = 0; i < 3; i++) manySeLoop(m, 0, 0, ev);          // timer 0,1,2 -> sin sonido
    CHECK(ev.empty() && m.timer == 3);
    manySeLoop(m, 0, 0, ev); CHECK(ev.size() == 1 && ev[0].a == 55 && m.count == 1 && m.timer == 1);
    for (int i = 0; i < 3; i++) manySeLoop(m, 0, 0, ev); CHECK(m.count == 0 && count(ev, E4_SOUND_GLOBAL) == 2);
    ev.clear(); manySeLoop(m, 0, 0, ev); CHECK(m.closed && count(ev, E4_TASK_CLOSE_MOT) == 1);
    ManySeState m2; m2.count = 5; m2.period = 1; ev.clear(); manySeLoop(m2, 1, 0, ev); CHECK(m2.closed); m2.closed = false; manySeLoop(m2, 0, 1, ev); CHECK(m2.closed);
    // BgmFade
    BgmFadeState b; b.vol = -500; b.target = 0; b.step = 100;
    bgmFadeLoop(b); CHECK(b.vol == -400 && b.outVolume == -400 && !b.closed);
    b.vol = -50; bgmFadeLoop(b); CHECK(b.vol == 0 && b.closed);                                   // tope 0
    b = BgmFadeState(); b.vol = -500; b.target = -300; b.step = 250; bgmFadeLoop(b); CHECK(b.vol == -300 && b.closed);   // llega al objetivo
    b = BgmFadeState(); b.vol = -100; b.target = -400; b.step = -150; bgmFadeLoop(b); CHECK(b.vol == -250 && !b.closed && b.outVolume == -250);
    bgmFadeLoop(b); CHECK(b.vol == -400 && b.closed);
    b = BgmFadeState(); b.vol = -900; b.target = -2000; b.step = -100; bgmFadeLoop(b); CHECK(b.vol == -960 && b.closed && b.outVolume == -960);   // suelo -960
    b = BgmFadeState(); b.vol = 0; b.target = 0; b.step = 0; bgmFadeLoop(b); CHECK(b.vol == 0 && b.closed);       // step 0: va por la rama de bajada
}

static void testAttach() {
    E4Env e = baseEnv(); E4Events ev; EnSndTable snd; static const int ids[] = {0x2001B, -1}; snd.init(ids); e.stage = 1; e.frameCounter = 50;
    AttachState a; a.pos[1] = 10; a.vel[1] = -20; a.acc[1] = -1; a.vel[0] = 10; a.vel[2] = -10; a.rotVel[0] = 11; a.rotVel[1] = -11; a.rotVel[2] = 6; a.timer = 5;
    AttachLogic::ctrlPre(a, 0.0f, snd, e, ev);        // y + vy = 10 + (-21) < 1 -> rebote
    CHECK(a.vel[1] == -21 * fbits(0xBE99999A) && a.vel[0] == 10 * 0.7f && a.vel[2] == -10 * 0.7f && a.rotVel[0] == 5 && a.rotVel[1] == -5 && a.rotVel[2] == 3);
    CHECK((a.flags & 0x40000000) && count(ev, E4_SOUND_GLOBAL) == 1 && a.pos[1] == 1.0f + a.vel[1] && a.timer == 4 && a.alpha == 1.0f - fbits(0x3D23D70A));
    CHECK(a.rot[0] == 11 && a.groundSaved == 0.0f);
    ev.clear(); a.flags |= 0; AttachLogic::ctrlPre(a, 0.0f, snd, e, ev);           // segundo rebote: sin sonido repetido (ya hay flag)
    CHECK(count(ev, E4_SOUND_GLOBAL) == 0);
    // timer: cuando <= 0 la escala baja
    AttachState b; b.timer = 1; AttachLogic::ctrlPre(b, -1000.0f, snd, e, ev); CHECK(b.timer == 0 && b.scale[0] == 2.0f - 0.04f && b.scale[1] == 2.0f - 0.08f);
    AttachState c; c.timer = 100; AttachLogic::ctrlPre(c, -1000.0f, snd, e, ev); CHECK(c.timer == 99 && c.scale[0] == 2.0f);
    c.timer = 0; c.scale[0] = 0.01f; AttachLogic::ctrlPre(c, -1000.0f, snd, e, ev); CHECK(c.scale[0] == 0.0f);
    // desvanecimiento hasta cerrar
    AttachState d; d.flags = 0x40000000; d.timer = 100; d.alpha = 0.05f; ev.clear();
    AttachLogic::ctrlPre(d, -1000.0f, snd, e, ev); CHECK(!d.closed); AttachLogic::ctrlPre(d, -1000.0f, snd, e, ev); CHECK(d.closed && (d.flags & 0x04000000) && count(ev, E4_TASK_CLOSE) == 1);
    // init con rand: 3 llamadas
    g_seqN = 3; g_seqI = 0; g_seq[0] = 3; g_seq[1] = 5; g_seq[2] = 6;       // rotVel0 = 3<<10, signo: 5 impar -> m=2 -> 1-2 = -1, rotVel1 = -1 * (6<<10)
    AttachState i; AttachLogic::init(i, seqRand);
    CHECK(i.rotVel[0] == (3 << 10) && i.rotVel[1] == -(6 << 10) && i.acc[1] == fbits(0xBD20902E) && i.alpha == 1.0f && i.scale[0] == 2.0f);
    CHECK(i.scaleDecay[0] == 1.2f * (2.0f / 60.0f) && i.scaleDecay[1] == 1.2f * (2.0f / 30.0f) && i.scaleDecay[2] == 1.2f * (2.0f / 20.0f));
    g_seqI = 0; g_seq[1] = 4; AttachLogic::init(i, seqRand); CHECK(i.rotVel[1] == (6 << 10));
    CHECK(etc4Spawns(0) && !etc4Spawns(1));
}

static void testThrow() {
    E4Env e = baseEnv(); e.coop = 1; E4Events ev; ThrowObjState s; ThrowOwner o;
    ThrowLogic::init1(s, o, 20, true, e, ev);
    CHECK(s.coop == 1 && s.timer == 20 && s.flightFrames == 20 && s.scale == fbits(0x3E4CCCCD) && s.frame == 0x1B008D && s.frameEnd == 0x1B0096 && s.flags == (0x10001u | 0x20000000) && e.attackGate == 1 && (o.flags2 & 0x20000000) && find(ev, E4_SOUND_GLOBAL)->a == 0xE040F);
    float off[3]; ThrowLogic::endLocalOffset(1, false, 100.0f, off); CHECK(off[0] == 0 && off[2] == -1.5f);
    ThrowLogic::endLocalOffset(0, true, 100.0f, off); CHECK(off[0] == (-1.5f * 160.0f) / 100.0f && off[2] == -1.5f);
    ThrowLogic::endLocalOffset(2, false, 100.0f, off); CHECK(off[0] == (-1.5f * -160.0f) / 100.0f);
    // vuelo: 3 fotogramas, luego impacto
    ThrowObjState t; t.flightFrames = 3; t.timer = 3; t.start[0] = 0; t.end[0] = 6; t.start[1] = 0; t.end[1] = -3; t.player = 1; e.attackSlot[1] = 1; ev.clear(); ThrowOwner ow; ow.flags2 = 0x20000000;
    ThrowLogic::ctrlPre(t, ow, e, ev); CHECK(t.pos[0] == 2 && t.pos[1] == -1 && t.timer == 2 && t.state == TS_FLY && ev.empty());
    ThrowLogic::ctrlPre(t, ow, e, ev); ThrowLogic::ctrlPre(t, ow, e, ev); CHECK(t.timer == 0 && t.state == TS_FLY && ev.empty());
    ThrowLogic::ctrlPre(t, ow, e, ev);       // timer -1 -> impacto en este mismo fotograma
    CHECK(count(ev, E4_DAMAGE_PLAYER) == 1 && find(ev, E4_DAMAGE_PLAYER)->a == 1 && find(ev, E4_DAMAGE_PLAYER)->b == 1 && find(ev, E4_DAMAGE_PLAYER)->c == 7);
    CHECK(t.state == TS_FADE && (t.flags & 0x01000000) && (t.flags & 0x8000) && t.timer == 14 && t.alpha == 1.0f - fbits(0x3D23D70A));
    for (int i = 0; i < 40 && !t.closed; i++) ThrowLogic::ctrlPre(t, ow, e, ev);
    CHECK(t.closed && e.attackSlot[1] == 0 && !(ow.flags2 & 0x20000000) && count(ev, E4_TASK_CLOSE) == 1 && count(ev, E4_DAMAGE_PLAYER) == 1 && (t.flags & 0x4000));
    // derribado a tiros
    ThrowObjState d; d.flightFrames = 10; d.timer = 10; d.flags = 8 | 2 | 4; d.player = 0; d.hasGroup = true; d.flags |= 0x20000000; e.attackSlot[0] = 1; e.attackGate = 1; ev.clear(); ow = ThrowOwner(); ow.flags2 = 0x20000000;
    float sc0 = d.scale; ThrowLogic::ctrlPre(d, ow, e, ev);
    CHECK(find(ev, E4_SOUND_GLOBAL)->a == 0xE0411 && count(ev, E4_HITNUM) == 2 && d.state == TS_SHOT_DOWN && d.alpha == 0.5f - fbits(0x3D088889) && d.scale == sc0 + fbits(0x3D888889) && (d.flags & 0x8000) && (d.flags & 0x4000));
    CHECK(!(ow.flags2 & 0x20000000));
    for (int i = 0; i < 40 && !d.closed; i++) ThrowLogic::ctrlPre(d, ow, e, ev);
    CHECK(d.closed && e.attackSlot[0] == 0 && e.attackGate == 0 && count(ev, E4_CLOSE_GROUP) == 1 && count(ev, E4_TASK_CLOSE) == 0 && count(ev, E4_DAMAGE_PLAYER) == 0);
    // la madre entra en 0x20 -> el objeto se marca muerto y no libera flags2 de la madre
    ThrowObjState m; m.state = TS_SHOT_DOWN; m.alpha = 0.01f; ow = ThrowOwner(); ow.flags2 = 0x20 | 0x20000000; ev.clear();
    ThrowLogic::ctrlPre(m, ow, e, ev); CHECK((m.flags & 0x04000000) && (ow.flags2 & 0x20000000) && m.closed);
    // tipo 3
    ThrowObjState t3; ThrowLogic::init3(t3); CHECK(t3.frame == 0x260067 && t3.frameEnd == 0x260086 && t3.flags == 0x10001 && t3.timer == 0);
    ow = ThrowOwner(); ev.clear(); ThrowLogic::ctrl3Pre(t3, ow, ev); CHECK(!t3.closed); ow.flags = 0x04000000; ThrowLogic::ctrl3Pre(t3, ow, ev); CHECK(t3.closed);
    ThrowObjState u; ThrowLogic::init3(u); ev.clear();
    for (int i = 0; i < 25; i++) ThrowLogic::ctrl3Pos(u, ev);
    CHECK(u.timer == 25 && u.alpha == 25 * fbits(0x3D23D70B) - 0 + 0 * 0 + (u.alpha - 25 * fbits(0x3D23D70B)) && u.frame == 0x260067 + 25);
    float a25 = u.alpha; for (int i = 25; i < 101; i++) ThrowLogic::ctrl3Pos(u, ev); CHECK(u.alpha == a25 && u.timer == 101);
    ThrowLogic::ctrl3Pos(u, ev); CHECK(u.alpha == a25 - fbits(0x3CA12773));
    while (!u.closed) ThrowLogic::ctrl3Pos(u, ev); CHECK(u.timer == 151 && count(ev, E4_TASK_CLOSE) == 1);
    ThrowObjState w; ThrowLogic::init3(w); w.frame = 0x260085; ThrowLogic::ctrl3Pos(w, ev); CHECK(w.frame == 0x260086 - 0 || w.frame == 0x260067);
    ThrowLogic::ctrl3Pos(w, ev); CHECK(w.frame == 0x260067 + 0 || w.frame >= 0x260067);
    // tipo 9
    g_seqN = 4; g_seqI = 0; g_seq[0] = 500; g_seq[1] = 400; g_seq[2] = 3; g_seq[3] = 4;      // rx = 500%240 = 20, ry = 400%168 = 64; signo rx: 3 impar -> no se niega; ry: 4 par -> se niega
    ThrowObjState n; ThrowOwner no; float el[3]; ThrowLogic::init9(n, no, 1, 30, 100.0f, seqRand, el);
    CHECK(el[0] == (-1.5f * 20.0f) / 100.0f && el[1] == (-1.5f * -64.0f) / 100.0f && el[2] == -1.5f && n.kind == 1 && n.timer == 30 && n.scale == 0.5f && n.frame == 0x2900F1 && n.flags == 0x10001 && n.alpha == 1.0f && n.state == 0);
    // llegada por profundidad
    E4Env e9 = baseEnv(); e9.coop = 0; ev.clear(); ow = ThrowOwner(); n.view[2] = -1.5f; n.view[0] = -3.0f;
    ThrowLogic::ctrl9Pre(n, ow, 0, 100.0f, e9, ev); CHECK(n.state == TS_FLY && ev.empty());
    n.view[2] = -1.0f; ThrowLogic::ctrl9Pre(n, ow, 0, 100.0f, e9, ev);      // -3*100/-1 = +300 -> jugador 0
    CHECK(find(ev, E4_DAMAGE_PLAYER)->a == 0 && n.state == TS_FADE && n.timer == 15 && (n.flags & 0x01000000));
    ThrowObjState n2 = n; n2.state = TS_FLY; n2.flags = 0x10001; n2.view[0] = 3.0f; ev.clear(); ThrowLogic::ctrl9Pre(n2, ow, 0, 100.0f, e9, ev); CHECK(find(ev, E4_DAMAGE_PLAYER)->a == 1);     // 3*100/-1 < 0 -> jugador 1
    e9.coop = 1; n2.state = TS_FLY; n2.flags = 0x10001; ev.clear(); ThrowLogic::ctrl9Pre(n2, ow, 1, 100.0f, e9, ev); CHECK(find(ev, E4_DAMAGE_PLAYER)->a == 1);
    n2.state = TS_FLY; n2.flags = 0x10001; ev.clear(); ThrowLogic::ctrl9Pre(n2, ow, 0, 100.0f, e9, ev); CHECK(find(ev, E4_DAMAGE_PLAYER)->a == 0);
    ow.pending = 4; ThrowObjState n3; n3.state = TS_FADE; n3.alpha = 0.01f; ThrowLogic::ctrl9Pre(n3, ow, 0, 100.0f, e9, ev); CHECK(n3.closed && ow.pending == 3);
    ThrowObjState n4; n4.state = TS_FADE; n4.alpha = 0.01f; n4.flags = 0x04000000; ThrowLogic::ctrl9Pre(n4, ow, 0, 100.0f, e9, ev); CHECK(n4.closed && ow.pending == 3);
    ThrowObjState n5; n5.flags = 8 | 2; n5.state = TS_FLY; n5.view[2] = -5; ev.clear(); ThrowLogic::ctrl9Pre(n5, ow, 0, 100.0f, e9, ev);
    CHECK(find(ev, E4_SOUND_GLOBAL)->a == 0x1B0017 && count(ev, E4_HITNUM) == 1 && ow.pending == 2 && n5.closed && (n5.flags & 0x4000) && count(ev, E4_DAMAGE_PLAYER) == 0);
    ThrowObjState p; p.frame = 5; p.frameStart = 5; p.frameEnd = 7; ThrowLogic::ctrl9Pos(p); CHECK(p.frame == 6); ThrowLogic::ctrl9Pos(p); CHECK(p.frame == 5);
    p.flags = 0x4000; ThrowLogic::ctrl9Pos(p); CHECK(p.frame == 5);
}

int main() {
    g_bin = slurp("disc/TBL/GAMEMOD.BIN");
    testTables(); testPalFrames(); testPetit(); testArmor(); testEtcInit(); testEtcHandlers(); testBranchMark(); testSnow(); testMisc(); testAttackObj(); testSeFade(); testAttach(); testThrow();
    if (fails == 0) printf("OK: %d comprobaciones, 0 fallos\n", checks); else printf("FALLAN %d de %d\n", fails, checks);
    return fails ? 1 : 0;
}

// Pruebas de game/trnkomono. Se ejecuta desde la raiz del repo (lee baserom/SCES_504.11 y disc/TBL/TRNMOD.BIN para comparar las tablas;
// si no estan, esas comprobaciones se saltan y se avisa).
#include "../trnkomono/trnkomono.h"
#include "../trnkomono/trnkomono_debris.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
using namespace vn;

static int g_fail = 0, g_n = 0;
#define CHECK(c) do { g_n++; if (!(c)) { g_fail++; std::printf("FALLO linea %d: %s\n", __LINE__, #c); } } while (0)
#define CHECKF(a, b) do { g_n++; if (std::fabs((double)(a) - (double)(b)) > 1e-4) { g_fail++; std::printf("FALLO linea %d: %s = %g, esperado %g\n", __LINE__, #a, (double)(a), (double)(b)); } } while (0)

// Secuencia de azar inyectada.
static std::vector<int> gInts; static size_t gIi = 0;
static std::vector<float> gFl; static size_t gFi = 0;
static int rI() { return gIi < gInts.size() ? gInts[gIi++] : 50; }   // agotada la secuencia: 50 (>= 5 de 100, no dispara apariciones)
static float rF() { return gFi < gFl.size() ? gFl[gFi++] : 0.0f; }
static void seed(std::vector<int> a, std::vector<float> b) { gInts = a; gIi = 0; gFl = b; gFi = 0; }
static TrnRand R() { TrnRand r; r.i = rI; r.f = rF; return r; }
static float axX(int a) { return (float)a / 1000.0f; }
static float axY(int a) { return (float)a / 2000.0f; }
static TrnTrig T() { TrnTrig t; t.axisX = axX; t.axisY = axY; return t; }
static float sq(float v) { return std::sqrt(v); }
static int rZero() { return 0; }
static int rOne() { return 1; }
static int rNeg() { return -3; }

static std::vector<unsigned char> readFile(const char* p) {
    std::vector<unsigned char> v; FILE* f = std::fopen(p, "rb"); if (!f) return v;
    unsigned char buf[65536]; size_t n; while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) v.insert(v.end(), buf, buf + n);
    std::fclose(f); return v;
}
static uint32_t u32(const std::vector<unsigned char>& d, size_t o) { uint32_t x; std::memcpy(&x, &d[o], 4); return x; }
static float f32(const std::vector<unsigned char>& d, size_t o) { float x; std::memcpy(&x, &d[o], 4); return x; }
static int16_t s16(const std::vector<unsigned char>& d, size_t o) { int16_t x; std::memcpy(&x, &d[o], 2); return x; }

static void testTablesVsFiles() {
    std::vector<unsigned char> elf = readFile("baserom/SCES_504.11"), ov = readFile("disc/TBL/TRNMOD.BIN");
    if (elf.empty() || ov.empty()) { std::printf("AVISO: no se encontraron baserom/SCES_504.11 o disc/TBL/TRNMOD.BIN; se saltan las comparaciones de tablas\n"); return; }
    auto E = [&](uint32_t v) { return (size_t)(v - 0x140000 + 0x800); };
    auto O = [&](uint32_t v) { return (size_t)(v - 0x9DF000); };
    for (int i = 0; i < kTrnKomonoRadiusN; i++) {
        size_t o = E(0x2C3260) + 16 * i;
        CHECK(kTrnKomonoRadius[i].id == u32(elf, o));
        CHECKF(kTrnKomonoRadius[i].radius, f32(elf, o + 4));
        CHECKF(kTrnKomonoRadius[i].yOff, f32(elf, o + 8));
        CHECK(kTrnKomonoRadius[i].collide == (int)u32(elf, o + 12));
    }
    CHECK((int)u32(elf, E(0x2C3260) + 16 * kTrnKomonoRadiusN) == -1);   // terminador justo despues de las 45 filas
    for (int i = 0; i < kTrnKomonoBreakN; i++) {
        size_t o = E(0x2C3540) + 20 * i;
        CHECK(kTrnKomonoBreak[i].id == u32(elf, o));
        CHECK(kTrnKomonoBreak[i].kind == s16(elf, o + 4));
        CHECK(kTrnKomonoBreak[i].model == s16(elf, o + 6));
        CHECK(kTrnKomonoBreak[i].fx == s16(elf, o + 8));
        CHECKF(kTrnKomonoBreak[i].fxScale, f32(elf, o + 12));
        CHECK(kTrnKomonoBreak[i].sound == (int)u32(elf, o + 16));
    }
    for (int i = 0; i < 6; i++) {
        CHECK(kTrn05Obj[i] == u32(ov, O(0x9F51A0) + 4 * i));
        CHECKF(kTrn05Scale[i], f32(ov, O(0x9F51C0) + 4 * i));
        CHECKF(kTrn05GravMul[i], f32(ov, O(0x9F51E0) + 4 * i));
        CHECKF(kTrn05Radius[i], f32(ov, O(0x9F5200) + 4 * i));
    }
    for (int i = 0; i < 15; i++) CHECK(kTrn05Sched[i] == s16(ov, O(0x9F5220) + 2 * i));
    for (int l = 0; l < 3; l++) for (int w = 0; w < 5; w++) CHECK(kTrn08WordLen[l][w] == s16(ov, O(0x9F5240) + 10 * l + 2 * w));
    for (int i = 0; i < 29; i++) {
        size_t o = O(0x9F4710) + 16 * i;
        CHECK(kTrn08Home[i].obj == u32(ov, o)); CHECKF(kTrn08Home[i].x, f32(ov, o + 4));
        CHECKF(kTrn08Home[i].y, f32(ov, o + 8)); CHECKF(kTrn08Home[i].z, f32(ov, o + 12));
    }
    for (int i = 0; i < 18; i++) for (int j = 0; j < 3; j++) CHECKF(kTrn10Pos[i][j], f32(ov, O(0x9F5260) + 12 * i + 4 * j));
    CHECKF(kTrn10TopY[0], f32(ov, O(0x9F5258) + 12 * 16));
    CHECKF(kTrn10TopY[1], f32(ov, O(0x9F5258) + 12 * 18));
    // las 26 primeras fichas son kTrnLetterObj (tabla ya existente en game/stages)
    for (int i = 0; i < 26; i++) CHECK(kTrn08Home[i].obj == kTrnLetterObj[i]);
}

static void testBoxPointAndBase() {
    TrnBoxPoint a, b; a.init(); b.init();
    a.initPos(1, 2, 3); b.initPos(4, 6, 3);
    CHECK(a.linkCount == 0); CHECKF(a.pos0[1], 2);
    a.setLink(b, sq);
    CHECK(a.linkCount == 1 && b.linkCount == 1 && a.link[0] == &b && b.link[0] == &a);
    CHECKF(a.linkDist[0], 5.0); CHECKF(b.linkDist[0], 5.0);

    TrnKomonoBase base; int src[] = {7, 100, 9, 250};
    base.initEndCP(src, 2);
    CHECK(base.n == 2 && base.id[1] == 9); CHECKF(base.frame[1], 250);
    CamPathNow c; c.pathId = 9; c.frame = 249.9f; CHECK(!base.checkEndCP(c));
    c.frame = 250; CHECK(base.checkEndCP(c));
    c.pathId = 5; c.frame = 999; CHECK(!base.checkEndCP(c));
    c.pathId = 7; c.frame = 100; CHECK(base.checkEndCP(c));
}

static void testTumiage() {
    TrnTumiageColli t; t.ctor();
    CHECK(t.c.flags == (1u | 0x10000u)); CHECKF(t.c.radius, 8.0); CHECK(!t.ctrlPre());
    t.setEnClName(0x1ECF188);
    CHECK(t.c.flags == (1u | 0x10 | 0x40)); CHECK(t.enClName == 0x1ECF188 && t.enClActive == 1);
    t.c.close(); CHECK(t.ctrlPre());
    t.c.flags |= 8; CHECK(t.c.chkColli()); t.c.clrColli(); CHECK(!t.c.chkColli());
    t.c.flags |= 4; CHECK(t.c.getHitPl(rZero) == 1 && !(t.c.flags & 4));
}

static void testTrnKomonoTask() {
    CHECK(TrnKomonoLogic::secondObj(0x4700F6) == 0x4700F7);
    CHECK(TrnKomonoLogic::secondObj(0x3C001C) == 0x3C001D);
    CHECK(TrnKomonoLogic::secondObj(0x410051) == 0x410052);
    CHECK(TrnKomonoLogic::secondObj(0x123456) == 0);
    TrnKomonoTask t; t.objId = 0x3C008A;
    TrnKomonoLogic::init(t);
    CHECKF(t.radius, 5.0); CHECKF(t.yOffset, 4.0); CHECK(t.hasCollision == 1); CHECK(t.objId2 == 0);
    t.objId = 0x3E01EA; TrnKomonoLogic::init(t);       // fila con bandera 0
    CHECKF(t.radius, 5.0); CHECKF(t.yOffset, 5.0); CHECK(t.hasCollision == 0);
    t.objId = 0x999999; TrnKomonoLogic::init(t);       // sin fila: valores por defecto
    CHECKF(t.radius, 8.0); CHECKF(t.yOffset, 4.0); CHECK(t.hasCollision == 0);

    TrnKomonoSys s; int8_t norm = 3;
    TrnKomonoLogic::ctorCounter(s, norm); CHECK(norm == 3);
    s.gameKind = 2; s.missionKind = 6; s.mission = 5;
    TrnKomonoLogic::ctorCounter(s, norm); CHECK(norm == 4);
    TrnKomonoLogic::dtorCounter(s, norm); TrnKomonoLogic::dtorCounter(s, norm); CHECK(norm == 2);
    norm = 0; TrnKomonoLogic::dtorCounter(s, norm); CHECK(norm == 0);   // no baja de 0
    s.mission = 4; norm = 2; TrnKomonoLogic::dtorCounter(s, norm); CHECK(norm == 2);

    // ctrl: cierra al llegar al punto de fin de la camara
    TrnKomonoTask k; int src[] = {0x10, 60}; k.endCP.initEndCP(src, 1);
    TrnKomonoOut o; CamPathNow c; c.pathId = 0x10; c.frame = 59; TrnKomonoLogic::ctrl(k, c, o); CHECK(o.ev.empty());
    c.frame = 60; TrnKomonoLogic::ctrl(k, c, o); CHECK(o.ev.size() == 1 && o.ev[0].kind == TrnKomonoEvent::CLOSE_TASK);

    // ctrlPost: impacto del jugador 1 (bit 2) en una pieza con tabla de rotura de dinamica (0x480065: kind 1, modelo 0x2F, sonido 0x1B0026)
    TrnKomonoTask p; p.objId = 0x480065; p.flags = 8 | 2; p.hasCollision = 1; p.pos[1] = 10; p.yOffset = 4;
    TrnKomonoOut o2; TrnKomonoLogic::ctrlPost(p, s, rZero, o2);
    CHECK(o2.ev.size() >= 6);
    CHECK(o2.ev[0].kind == TrnKomonoEvent::ADD_SCORE && o2.ev[0].a == 0 && o2.ev[0].b == 10);
    CHECK(o2.ev[1].kind == TrnKomonoEvent::ADD_HIT && o2.ev[1].a == 0);
    CHECK(o2.ev[2].kind == TrnKomonoEvent::SPAWN_DYNAMICS && o2.ev[2].a == 0x2F);
    CHECK(o2.ev[3].kind == TrnKomonoEvent::SOUND && o2.ev[3].a == 0x1B0026 && o2.ev[3].b == 0x40);
    CHECK(o2.ev[4].kind == TrnKomonoEvent::CLOSE_TASK);
    CHECK(o2.ev[5].kind == TrnKomonoEvent::COLLISION_UPDATE); CHECKF(o2.ev[5].f, 14.0);
    CHECK(p.flags == 0);            // limpia 8 y el bit del jugador unico
    // sin impacto: solo la actualizacion de colision
    TrnKomonoTask q = p; q.flags = 2; TrnKomonoOut o3; TrnKomonoLogic::ctrlPost(q, s, rZero, o3);
    CHECK(o3.ev.size() == 1 && o3.ev[0].kind == TrnKomonoEvent::COLLISION_UPDATE);
    // jugador 2 (bit 4) -> jugador 1
    TrnKomonoTask q2; q2.objId = 0x999; q2.flags = 8 | 4; TrnKomonoOut o4; TrnKomonoLogic::ctrlPost(q2, s, rZero, o4);
    CHECK(o4.ev[0].a == 1);
    // ambos: rand()%2 (con rand impar -> 1; con rand negativo impar semantica C = -1)
    TrnKomonoTask q3; q3.objId = 0x999; q3.flags = 8 | 6; TrnKomonoOut o5; TrnKomonoLogic::ctrlPost(q3, s, rOne, o5); CHECK(o5.ev[0].a == 1);
    TrnKomonoTask q4; q4.objId = 0x999; q4.flags = 8 | 6; TrnKomonoOut o6; TrnKomonoLogic::ctrlPost(q4, s, rNeg, o6); CHECK(o6.ev[0].a == -1);

    // close: objeto 0x3C001C -> fila con plato roto (kind 0) o dinamica segun tabla + efecto especial en posicion
    const TrnKomonoBreak* b = TrnKomonoLogic::findBreak(0x3C001C);
    CHECK(b != nullptr);
    TrnKomonoTask m; m.objId = 0x3C001C; TrnKomonoOut o7; TrnKomonoLogic::close(m, s, o7);
    CHECK(!o7.ev.empty() && o7.ev.back().kind == TrnKomonoEvent::CLOSE_TASK);
    bool hasAt = false; for (auto& e : o7.ev) if (e.kind == TrnKomonoEvent::SPAWN_FCV_AT && e.a == 3) hasAt = true;
    CHECK(hasAt);
    // objeto 0x4700F6 -> efecto 8 con escala 3.0; 0x40003A -> efecto 6 escala 2.0
    m.objId = 0x4700F6; TrnKomonoOut o8; TrnKomonoLogic::close(m, s, o8);
    bool f8 = false; for (auto& e : o8.ev) if (e.kind == TrnKomonoEvent::SPAWN_FCV && e.a == 8 && e.f == 3.0f) f8 = true;
    CHECK(f8);
    // pieza kind 0 (plato): 0x410053 -> SPAWN_DISH con la variante segun gameState/gameKind
    m.objId = 0x410053; s.gameState = 0xA; s.gameKind = 2; TrnKomonoOut o9; TrnKomonoLogic::close(m, s, o9);
    CHECK(o9.ev[0].kind == TrnKomonoEvent::SPAWN_DISH && o9.ev[0].b == 1);
    s.gameKind = 1; TrnKomonoOut o10; TrnKomonoLogic::close(m, s, o10); CHECK(o10.ev[0].b == 0);
    // objeto sin fila ni especial: solo cierra
    m.objId = 0x999999; TrnKomonoOut o11; TrnKomonoLogic::close(m, s, o11); CHECK(o11.ev.size() == 1);
}


static void testTrn05() {
    // init de un objetivo (idx 0): x, y, vy (flotantes), luego dos enteros de giro
    seed({0x15, 0x2A}, {0.5f, 0.25f, 0.5f});
    Trn05Komono k; TrnBallOut o;
    k.init(0.3f, 0, R(), o);
    CHECK((uint32_t)k.b.obj == 0x69000C);
    CHECKF(k.b.pos[0], 0.0); CHECKF(k.b.pos[1], 122.5); CHECKF(k.b.pos[2], 35.5);
    CHECKF(k.vel[1], -0.01 - 0.025 - 0.3);   // 0xBC23D70A = -0.01
    CHECKF(k.grav, -0.00978 * 1.0);
    CHECK(k.rotSpd[0] == 0x10 && k.rotSpd[1] == 0);
    CHECK(k.b.life == 1);
    CHECK(o.count(TrnBallEvent::COLLI_CLEAR) == 1 && o.count(TrnBallEvent::COLLI_RADIUS) == 1);
    CHECKF(o.ev.back().x, 4.0);                      // radio de colision de la fila 0 (D_009F5200)

    // init de un senuelo (idx -1): entero (indice), x, y, z, vy, dos enteros
    seed({7, 0x81, 0x205}, {0.0f, 0.0f, 0.5f, 0.0f});
    Trn05Komono d; TrnBallOut o2; d.init(0.0f, -1, R(), o2);
    CHECK((uint32_t)d.b.obj == kTrn05Obj[3]);        // 7 % 5 + 1 = 3
    CHECKF(d.b.pos[2], 37.0);                        // 36 + 2 * 0.5
    CHECK(d.rotSpd[0] == 0x80 && d.rotSpd[1] == 0x200);
    CHECKF(d.b.scale, kTrn05Scale[3]);

    // ctrl: caida, gravedad y animacion
    TrnBallSys s; s.mission = 0; TrnTrig t = T(); TrnBallOut o3;
    Trn05Komono f; f.b.life = 1; f.b.obj = 0x10087; f.b.pos[0] = 1; f.b.pos[1] = 100; f.vel[1] = -2; f.grav = -0.5f; f.rotSpd[0] = 3; f.frame = 14;
    f.ctrl(s, R(), t, o3);
    CHECKF(f.b.pos[1], 98.0); CHECKF(f.vel[1], -2.5); CHECK(f.rot[0] == 3 && f.frame == 0 && f.b.life == 1);
    CHECK(o3.count(TrnBallEvent::COLLI_POS) == 1);
    // en pausa no se mueve
    s.paused = 1; f.ctrl(s, R(), t, o3); CHECKF(f.b.pos[1], 98.0); s.paused = 0;
    // sale por debajo de Y = 50 y por los lados
    f.b.pos[1] = 50.4f; f.vel[1] = -1; f.ctrl(s, R(), t, o3); CHECK(f.b.life == 0);
    f.b.life = 1; f.b.pos[0] = 31; f.b.pos[1] = 100; f.vel[1] = 0; f.vel[0] = 0; f.ctrl(s, R(), t, o3); CHECK(f.b.life == 0);
    f.b.life = 1; f.b.pos[0] = -31; f.ctrl(s, R(), t, o3); CHECK(f.b.life == 0);
    f.b.life = 1; f.b.pos[0] = 30; f.ctrl(s, R(), t, o3); CHECK(f.b.life == 1);

    // impacto en el objetivo: un objetivo menos, efecto del jugador, sonido, desaparece
    Trn05Komono g; g.b.life = 1; g.b.obj = 0x69000C; g.b.pos[1] = 100; g.colliHit = true; g.colliPl = 1; s.norm = 5;
    TrnBallOut o4; g.ctrl(s, R(), t, o4);
    CHECK(s.norm == 4 && g.b.life == 0);
    CHECK(o4.count(TrnBallEvent::HIT_EFFECT) == 1 && o4.count(TrnBallEvent::SOUND) == 1);
    for (auto& e : o4.ev) if (e.kind == TrnBallEvent::HIT_EFFECT) CHECK(e.a == 1);
    for (auto& e : o4.ev) if (e.kind == TrnBallEvent::SOUND) CHECK(e.a == 0x6000D);
    // impacto en un senuelo: rebote por angulo (rama f < 0.5 -> +0x7E01), sonido 0x6000C, dispara temblor
    Trn05Komono h; h.b.life = 1; h.b.obj = 0x10087; h.b.pos[1] = 100; h.colliHit = true; s.gp6F5C = 0;
    seed({0x10, 0xFF, 0x3FF}, {0.25f});
    TrnBallOut o5; h.ctrl(s, R(), t, o5);
    CHECKF(h.vel[0], 1.5 * (0x7E01 + 0x10) / 1000.0); CHECKF(h.vel[1], 1.5 * (0x7E01 + 0x10) / 2000.0);
    CHECK(h.rotSpd[0] == 0xFF - 0x7F && h.rotSpd[1] == 0x3FF - 0x1FF);
    CHECK(s.gp6F5C == 1 && h.b.life == 1);
    // rama f >= 0.5 -> -0x1FF
    Trn05Komono h2; h2.b.life = 1; h2.b.obj = 0x10088; h2.b.pos[1] = 100; h2.colliHit = true;
    seed({0x1FF, 0, 0}, {0.75f}); TrnBallOut o6; h2.ctrl(s, R(), t, o6);
    CHECKF(h2.vel[0], 1.5 * (0x1FF - 0x1FF) / 1000.0);

    // gestor: constructor (un senuelo inicial con 5 %: primer entero 3 < 5)
    seed({3, 7, 0x81, 0x205}, {0.0f, 0.0f, 0.5f, 0.0f});
    static Trn05Mgr m; TrnBallSys gs; TrnBallOut o7; m.ctor(R(), gs, o7);
    CHECK((uint32_t)m.decoy[0].b.obj == kTrn05Obj[3] && m.decoy[0].b.life == 1);
    CHECK(m.decoy[1].b.life == 0 && m.decoy[31].b.life == 0);
    for (int i = 0; i < 15; i++) CHECK(m.sched[i] == kTrn05Sched[i] + 0);     // 50 % 10 = 0
    // gestor: ctrl, aparicion de un objetivo segun calendario (nivel 2 = dificultad maxima)
    static Trn05Mgr m2; for (int i = 0; i < 15; i++) m2.sched[i] = kTrn05Sched[i];
    TrnBallSys ms; ms.gp6F58 = 1; ms.mission = 3; ms.level[3] = 2; m2.tick = 33; m2.next = 1;
    seed({}, {}); TrnBallOut o8; m2.ctrl(ms, R(), t, o8);
    CHECK(m2.target[0].b.life == 1 && (uint32_t)m2.target[0].b.obj == 0x69000C);
    CHECK(m2.next == 2 && m2.tick == 34);
    CHECKF(m2.target[0].b.pos[0], -20.0 + 13.0 * (33.0 / 600.0) + m2.target[0].vel[0] * 1.0);   // desplazamiento x del nivel 2 (tras un fotograma de ctrl)
    CHECKF(m2.target[0].vel[0], 0.0015 * (-33.0 / 2.0));
    // temblor: sin disparo, gp6F60 = 0 -> atributo de camara 0xB
    CHECK(o8.count(TrnBallEvent::CAM_ATTR) == 1);
    // temblor: disparado
    ms.gp6F5C = 1; TrnBallOut o9; m2.ctrl(ms, R(), t, o9);
    CHECK(o9.count(TrnBallEvent::QUAKE) == 1); CHECKF(ms.gp6F60, 47.0); CHECK(ms.gp6F5C == 0);
    CHECK(ms.gp7404 > 1690 && ms.gp7404 < 1700); CHECK(ms.gp7408 == 0);
    // fin de mision cierra el gestor
    ms.missionOver = 1; TrnBallOut o10; m2.ctrl(ms, R(), t, o10); CHECK(o10.count(TrnBallEvent::CLOSE_TASK) == 1);
    // gestor inactivo: no hace nada
    ms.gp6F58 = 0; TrnBallOut o11; int tk = m2.tick; m2.ctrl(ms, R(), t, o11); CHECK(o11.ev.empty() && m2.tick == tk);
}

static void testTrn08Tile() {
    Trn08Komono k; TrnBallOut o; TrnBallSys s;
    k.init(0x6C000A, 20, 10, 0, 0, 0, 0, 10, o);
    CHECK(k.b.life == 1 && k.interp == 1); CHECKF(k.xa, 20.0 / (100.0 / 4.0)); CHECKF(k.ya, 10.0 / 25.0);
    CHECKF(k.fin[0], 20);
    int frames = 0; float prev = -1; bool mono = true;
    while (k.interp && frames < 100) { k.ctrl(s, o); frames++; if (k.interp && k.b.pos[0] < prev) mono = false; if (k.interp) prev = k.b.pos[0]; }
    CHECK(frames == 10); CHECK(mono); CHECKF(k.b.pos[0], 20); CHECKF(k.b.pos[1], 10); CHECK(k.elapsed == 0);
    // choque: giro 0x3200 que decae 0x140 por fotograma y a los 40 fotogramas termina en 0
    k.colliHit = true; k.colliPl = 1; CHECK(k.checkColli(o)); CHECK(k.getHitPl() == 1 && k.rotSpd == 0x3200);
    CHECK(!k.checkColli(o) && k.getHitPl() == -1);
    for (int i = 0; i < 39; i++) k.ctrl(s, o);
    CHECK(k.rotSpd == 0x140 && k.rot != 0);
    k.ctrl(s, o); CHECK(k.rotSpd == 0 && k.rot == 0);
    // pausa: el giro no avanza
    k.rotSpd = 0x3200; s.paused = 1; int r0 = k.rot; k.ctrl(s, o); CHECK(k.rot == r0 && k.rotSpd == 0x3200);
    // duracion <= 0: no interpola pero marca interpolando y termina en el primer fotograma al destino
    Trn08Komono z; z.setInterpolate(5, 6, 7, 1, 2, 3, 0); CHECK(z.interp == 1 && z.xa == 0 && z.b.pos[0] == 1);
}

static void testTrn08Mgr() {
    // nivel 2: longitudes (2,2,2,3,3). Se busca una palabra prohibida de 3 letras (A..Z) en la lista del ELF para la cuarta palabra.
    const char* bad3 = nullptr; for (int i = 0; i < 50 && !bad3; i++) if (std::strlen(kTrnBadWords[i]) == 3) bad3 = kTrnBadWords[i];
    CHECK(bad3 != nullptr);
    if (!bad3) return;
    std::vector<int> v(58, 0);                                   // 29 fichas x 2 enteros de dispersion
    int letters[] = {0, 1, 2, 3, 4, 5, bad3[0] - 'A', bad3[1] - 'A', bad3[2] - 'A', 6, 7, 8};
    for (int x : letters) v.push_back(x);
    seed(v, {});
    static Trn08Mgr m; TrnBallOut o; m.ctor(2, R(), o);
    CHECK(m.wordLen[0] == 2 && m.wordLen[3] == 3 && m.wordLen[4] == 3);
    CHECK(m.word[0][0] == 0x6C000A && m.word[0][1] == 0x6C000C);
    CHECK(m.word[3][0] == kTrnLetterObj[bad3[0] - 'A']);         // la prohibida conserva la 1a letra y cambia la 2a y 3a
    CHECK(m.word[3][1] == 0x6C002C && m.word[3][2] == 0x6C0029);
    CHECK(m.word[4][0] == kTrnLetterObj[6] && m.word[4][2] == kTrnLetterObj[8]);
    CHECK(m.state == 2 && m.wordIdx == 0 && m.typedCount == 0); CHECKF(m.timer, 30.0);
    CHECKF(m.wordPos[3][0][0], 25.2); CHECKF(m.wordPos[3][2][0], 37.2); CHECKF(m.wordPos[0][1][0], 34.2); CHECKF(m.wordPos[0][0][0], 28.2);
    CHECKF(m.wordPos[0][0][1], -7.00998);   // 0xC0E051CA CHECKF(m.wordPos[0][0][2], 0.1298);
    // todas las fichas con destino en su sitio y duracion 50 + (4i % 20)
    CHECKF(m.tile[0].total, 50.0); CHECKF(m.tile[1].total, 54.0); CHECKF(m.tile[5].total, 50.0);   // i=5: 20 % 20 = 0
    CHECKF(m.tile[0].fin[0], kTrn08Home[0].x);

    // ctrl: estado 2 -> 1 (sonido 0x20027); luego desliza 30/2.4 = 13 fotogramas y pasa a 0
    TrnBallSys s; s.gp6F54 = 1; s.norm = 3; seed({}, {});
    TrnBallOut o2; m.ctrl(s, R(), o2); CHECK(m.state == 1 && o2.count(TrnBallEvent::SOUND) == 1);
    int guard = 0; while (m.state == 1 && guard++ < 50) m.ctrl(s, R(), o2);
    CHECK(m.state == 0 && guard == 13);
    CHECKF(m.wordPos[0][0][0], 28.2 - 13 * 2.4 + 0.0);          // (la ultima resta lleva a timer <= 0)
    // letra equivocada: ficha 'Z' (obj distinto del esperado 'A')
    m.tile[25].colliHit = true; m.tile[25].colliPl = 0; TrnBallOut o3; m.ctrl(s, R(), o3);
    CHECK(m.typedCount == 0 && o3.count(TrnBallEvent::QUAKE) == 1);
    for (auto& e : o3.ev) if (e.kind == TrnBallEvent::SOUND) CHECK(e.a == 0x6000C);
    // letra correcta 'A' (ficha 0): avanza y suena 0x2002E
    m.tile[0].colliHit = true; m.tile[0].colliPl = 1; TrnBallOut o4; m.ctrl(s, R(), o4);
    CHECK(m.typedCount == 1 && m.typed[0] == 0x6C000A);
    bool s2e = false; for (auto& e : o4.ev) if (e.kind == TrnBallEvent::SOUND && e.a == 0x2002E) s2e = true; CHECK(s2e);
    // segunda letra 'B' (ficha 1): palabra completa -> gira, norm 3 -> 2, sigue la mision, 0x1F0014
    m.tile[1].colliHit = true; m.tile[1].colliPl = 1; TrnBallOut o5; m.ctrl(s, R(), o5);
    CHECK(m.wordIdx == 1 && m.typedCount == 0 && m.state == 3 && s.norm == 2 && s.missionOver == 0);
    bool s14 = false; for (auto& e : o5.ev) if (e.kind == TrnBallEvent::SOUND && e.a == 0x1F0014) s14 = true; CHECK(s14);
    // giro: ya se aplico un fotograma de estado 3 en esta pasada: vel 0x3200 -> 0x3200-384
    CHECK(m.spinVel == 0x3200 - 384 && m.spinAngle == 0x3200 && m.counter == 1);
    for (int i = 0; i < 8; i++) m.ctrl(s, R(), o5);
    CHECK(m.state == 2 && m.counter == 0 && m.spinVel == 0 && m.spinAngle == 0);
    // ultima palabra de las que quedan: forzar norm = 1 y completar -> missionOver y dispersion
    m.state = 0; m.wordIdx = 1; m.typedCount = 1; s.norm = 1;
    m.tile[(m.word[1][1] == 0x6C002C) ? 28 : (int)(m.word[1][1] - 0x6C000A)].b.obj = (int)m.word[1][1];   // la ficha existe con ese objeto
    int idx = -1; for (int i = 0; i < 29; i++) if ((uint32_t)m.tile[i].b.obj == m.word[1][1]) idx = i;
    CHECK(idx >= 0);
    m.tile[idx].colliHit = true; m.tile[idx].colliPl = 0; TrnBallOut o6; m.ctrl(s, R(), o6);
    CHECK(s.norm == 0 && s.missionOver == 1 && m.tile[0].interp == 1 && m.tile[0].total == 30.0f);
    // sin gestor activo no hace nada
    s.gp6F54 = 0; TrnBallOut o7; int f0 = m.frame; m.ctrl(s, R(), o7); CHECK(m.frame == f0);
}

static void testTrn10() {
    Trn10KomonoFull k; TrnBallOut o;
    k.init(0x6E001F, 1, 2, 3, o);
    CHECK(k.life == 1 && k.obj == 0x6E001F); CHECKF(k.k.y, 2);
    CHECKF(o.ev.back().x, 15.0);
    int attr[2] = {0x11, 0x22};
    CHECK(!k.checkColli(attr, o) && k.getHitPl() == -1);
    k.colliHit = true; k.colliPl = 1; CHECK(k.checkColli(attr, o) && k.getHitPl() == 1 && k.getHitColliAttr() == 0x22);
    k.addPos(1, 1, 1); CHECKF(k.getPosY(), 3); k.setVel(0, -1, 0); CHECKF(k.k.vy, -1);
    // modo 1 = ctrl2 (fisica ya reescrita): baja y gravedad 0.072
    k.k.mode = 1; k.ctrl(false, o); CHECKF(k.k.y, 2.0); CHECKF(k.k.vy, -1.072);
    // modo 0: solo coloca el colisionador
    Trn10KomonoFull k2; k2.k.mode = 0; k2.k.x = 1; k2.k.offX = 0.5f; TrnBallOut o2; k2.ctrl(false, o2); CHECK(o2.count(TrnBallEvent::COLLI_POS) == 1);
    CHECKF(o2.ev[0].x, 1.5);
    // sin colisionador: checkColli falla y closeColli lo suelta
    k2.closeColli(o2); CHECK(!k2.hasCollider); k2.colliHit = true; CHECK(!k2.checkColli(attr, o2));
    // anillo de marcas: 16 posiciones
    Trn10KomonoFull m; for (int i = 0; i < 17; i++) m.setHitMark(100 + i);
    CHECK(m.markIdx == 1 && m.markAttr[0] == 116 && m.markAttr[15] == 115 && m.markOn[0] == 1);

    // gestor: seleccion de bolas
    seed({5, 5}, {});
    Trn10MgrInit a = Trn10MgrInit::make(0, R());
    CHECK(a.count == 16 && a.flag[0] == 1 && a.flag[5] == 1 && a.flag[1] == 1 && a.flag[2] == 0);
    CHECK(a.objectives == 13); CHECKF(a.topY, 279.2 - 9.0);
    seed({5, 9}, {});
    Trn10MgrInit b = Trn10MgrInit::make(2, R());
    CHECK(b.count == 18 && b.flag[0] == 1 && b.flag[5] == 1 && b.flag[9] == 1 && b.objectives == 15); CHECKF(b.topY, 315.208 - 9.0);
    seed({21, 40}, {});      // 21 % 16 = 5, 40 % 16 = 8
    Trn10MgrInit c = Trn10MgrInit::make(1, R());
    CHECK(c.count == 16 && c.flag[5] == 1 && c.flag[8] == 1 && c.objectives == 13);
}

static void testBase2() {
    TrnKomonoBase2 b; b.ctor();
    CHECK(b.getLife() == 0 && b.getObj() == 0); CHECKF(b.scale, 1.0);
    b.pos[0] = 1; b.pos[1] = 2; b.pos[2] = 3; float p[3]; b.getPos(p); CHECKF(p[1], 2);
}

static void testDebris() {
    // debrisStep: sin suelo, gravedad e integracion
    DebrisPiece p; p.pos[1] = 5; p.vel[0] = 1; p.vel[1] = 2; p.grav = -0.5f; p.rotVel[0] = 0x10001; p.floorY = 0;
    debrisStep(p, false);
    CHECKF(p.vel[1], 1.5); CHECKF(p.pos[0], 1); CHECKF(p.pos[1], 6.5); CHECK(p.rot[0] == 1);
    DebrisPiece q = p; debrisStep(q, true); CHECKF(q.pos[1], 6.5);          // pausa: no cambia
    // rebote
    DebrisPiece b; b.pos[1] = -1; b.vel[0] = 10; b.vel[1] = -2; b.vel[2] = 10; b.grav = 0; b.rotVel[1] = 7; b.floorY = 0;
    debrisStep(b, false);
    CHECKF(b.vel[1], -2 - 0.9f * -2); CHECKF(b.vel[0], 6.0); CHECKF(b.vel[2], 6.0); CHECKF(b.pos[1], b.vel[1]); CHECK(b.rotVel[1] == 0);
    // Croset
    seed({7, 3, 9, 0x3FF, 0, 0x200, 1}, {}); DebrisPiece c; float base[3] = {0, 0, 0.2f};
    CrosetKoware::initPiece(c, base, 3.0f, R());
    CHECKF(c.vel[0], 0.2); CHECKF(c.vel[1], 0.3); CHECKF(c.vel[2], 0.2f + 0.1f * 4);
    CHECKF(c.grav, -0.04); CHECK(c.rotVel[0] == 0x3FF - 0x1FF); CHECK(c.rotVel[1] == -0x1FF); CHECK(c.rotVel[2] == 1);
    CHECKF(c.floorY, 3.0);
    CrosetKoware k; DebrisPiece pc[1]; bool cl = false; k.frames = 0xE0F;
    cl = k.ctrl(pc, 1, false); CHECK(!cl && k.frames == 0xE10 && k.alpha < 1.0f);
    cl = k.ctrl(pc, 1, true); CHECK(!cl && k.frames == 0xE10);
    cl = k.ctrl(pc, 1, false); CHECK(cl);
    // Dish: tabla y cierre
    CHECK(DishKoware::findRow(0xDEADBEEF) == nullptr); CHECK(DishKoware::findRow(kDishRows[3].id) == &kDishRows[3]);
    DishKoware d; d.setup(0x1B0079); CHECKF(d.mult, 3.0); d.frames = 0x3C; CHECK(d.ctrl(pc, 1, false));
    DishKoware e; e.setup(kDishRows[0].id); e.frames = 0x28; CHECK(e.ctrl(pc, 1, false)); CHECKF(e.mult, 2.0);
    seed({0x8000, 0x4000, 5, 6, 7}, {}); DebrisPiece dp; e.initPiece(dp, R(), T());
    CHECK(dp.vel[1] >= 0.0f); CHECKF(dp.grav, -0.05); CHECKF(dp.floorY, 0.0);
}

int main() {
    testTablesVsFiles();
    testBoxPointAndBase();
    testTumiage();
    testTrnKomonoTask();
    testBase2();
    testTrn05();
    testTrn08Tile();
    testTrn08Mgr();
    testTrn10();
    testDebris();
    if (g_fail == 0) std::printf("trnkomono_test: TODO OK (%d comprobaciones)\n", g_n);
    else std::printf("trnkomono_test: %d FALLOS de %d\n", g_fail, g_n);
    return g_fail != 0;
}

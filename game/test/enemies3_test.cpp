// Tests de game/enemies3. Constantes y tablas comparadas con disc/TBL/GAMEMOD.BIN (carga en 0x92B000); se omiten con aviso si no esta.
// El original NUNCA se ejecuto: no hay comparacion de comportamiento en vivo.
// Compilar (desde la raiz del repo):
//   c++ -std=c++17 -Wall game/test/enemies3_test.cpp game/enemies3/enemies3_misc.cpp game/enemies3/enemies3_throw.cpp -o build/verify_enemies3/enemies3_test && ./build/verify_enemies3/enemies3_test
#include "../enemies3/enemies3.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
using namespace vn::en3;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

// ---- acceso a GAMEMOD.BIN ----
static std::vector<unsigned char> g_bin;
static bool loadBin() {
    std::ifstream f("disc/TBL/GAMEMOD.BIN", std::ios::binary);
    if (!f) return false;
    g_bin.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return g_bin.size() > 0x60000;
}
static uint32_t rd32(uint32_t a) { uint32_t v; std::memcpy(&v, &g_bin[a - 0x92B000u], 4); return v; }
static float rdf(uint32_t a) { float v; std::memcpy(&v, &g_bin[a - 0x92B000u], 4); return v; }
static uint32_t fbits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
// La constante de 32 bits `v` se carga en la funcion [addr, addr+len) con `lui rt, hi` (+ `ori rt, rt, lo` si lo != 0).
static bool hasConst(uint32_t addr, uint32_t len, uint32_t v) {
    uint32_t hi = v >> 16, lo = v & 0xFFFF;
    bool gotHi = false, gotLo = (lo == 0);
    for (uint32_t a = addr; a < addr + len; a += 4) {
        uint32_t w = rd32(a);
        if ((w >> 26) == 0xF && (w & 0xFFFF) == hi) gotHi = true;      // lui
        if ((w >> 26) == 0xD && (w & 0xFFFF) == lo) gotLo = true;      // ori
        if (v < 0x10000 && ((w >> 26) == 9 || (w >> 26) == 0xA) && (w & 0xFFFF) == lo) return true;   // addiu / slti con inmediato
        if (v < 0x10000 && (w >> 26) == 0xD && (w & 0xFFFF) == lo) return true;
    }
    return gotHi && gotLo;
}

// ---- entorno de prueba ----
static int g_seq[32]; static int g_seqN = 0, g_seqI = 0;
static int fakeRand() { return g_seq[(g_seqI++) % (g_seqN ? g_seqN : 1)]; }
static double g_r01[16]; static int g_r01N = 0, g_r01I = 0;
static double fakeRand01() { return g_r01[(g_r01I++) % (g_r01N ? g_r01N : 1)]; }
struct TestCam : Camera3 {   // camara en `c`, sin rotacion
    V3 c;
    V3 worldToView(const V3& w) const override { V3 r; r.x = w.x - c.x; r.y = w.y - c.y; r.z = w.z - c.z; return r; }
    V3 viewToWorld(const V3& v) const override { V3 r; r.x = v.x + c.x; r.y = v.y + c.y; r.z = v.z + c.z; return r; }
    V3 normalize(const V3& v) const override {
        float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); V3 r; if (l > 0) { r.x = v.x / l; r.y = v.y / l; r.z = v.z / l; } return r;
    }
};
static TestCam g_cam;
static Env3 mkEnv() { Env3 e; e.rand = fakeRand; e.rand01 = fakeRand01; e.cam = &g_cam; e.focal = 500.0f; return e; }
static int count(const Ev3s& q, Ev3Type t) { int n = 0; for (auto& e : q) if (e.type == t) n++; return n; }
static const Ev3* find(const Ev3s& q, Ev3Type t, int nth = 0) { for (auto& e : q) if (e.type == t && nth-- == 0) return &e; return nullptr; }
static bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

// =====================================================================================================================
static void testTables(bool bin) {
    if (!bin) { printf("(tablas contra GAMEMOD.BIN omitidas: fichero no encontrado)\n"); return; }
    // EnSndReq: D_009857A0
    for (int i = 0; i < 6; i++) CHECK((int32_t)rd32(0x9857A0 + 4 * i) == kEnSndTable[i]);
    // D_00985430: claves de color (5 filas de 4 floats)
    for (int i = 0; i < 5; i++) for (int j = 0; j < 4; j++) CHECK(rdf(0x985430 + 16 * i + 4 * j) == kVtxColorKey[i][j]);
    // Constantes cargadas con lui/ori en la propia funcion original
    struct K { const char* fn; uint32_t addr, len, v; };
    const K ks[] = {
        {"Init1", 0x93DB60, 0x300, 0x1B008D}, {"Init1", 0x93DB60, 0x300, 0x1B0096}, {"Init1", 0x93DB60, 0x300, 0xE040F},
        {"Init1", 0x93DB60, 0x300, fbits(0.2f)}, {"Init1", 0x93DB60, 0x300, fbits(0.5f)}, {"Init1", 0x93DB60, 0x300, 0x10001},
        {"Init2", 0x93EBC0, 0x300, 0x260000}, {"Init2", 0x93EBC0, 0x300, 0x26001F}, {"Init2", 0x93EBC0, 0x300, fbits(1.2f)},
        {"Init2", 0x93EBC0, 0x300, fbits(3.0f)}, {"Init2", 0x93EBC0, 0x300, 0x29001E}, {"Init2", 0x93EBC0, 0x300, 0x2900F1},
        {"Init2", 0x93EBC0, 0x300, fbits(0.7f)}, {"Init2", 0x93EBC0, 0x300, fbits(2.0f)},
        {"Init3", 0x93F160, 0xD0, 0x260067}, {"Init3", 0x93F160, 0xD0, 0x260086},
        {"Init5", 0x93FDC0, 0x1F0, fbits(1.7f)}, {"Init5", 0x93FDC0, 0x1F0, 0x88888889},
        {"Init6", 0x9408A0, 0x260, 0x800002}, {"Init6", 0x9408A0, 0x260, fbits(1.8f)},
        {"Init7", 0x941480, 0x210, 0x100019},
        {"Pre1", 0x93D2F0, 0x400, 0xE0411}, {"Pre1", 0x93D2F0, 0x400, fbits(0.04f)}, {"Pre1", 0x93D2F0, 0x400, fbits(0.033333335f)},
        {"Pre1", 0x93D2F0, 0x400, fbits(0.06666667f)},
        {"Pre2", 0x93DE60, 0x7A0, 0x18020F}, {"Pre2", 0x93DE60, 0x7A0, 0x20025}, {"Pre2", 0x93DE60, 0x7A0, 0x1B000E},
        {"Pre2", 0x93DE60, 0x7A0, 0x260040}, {"Pre2", 0x93DE60, 0x7A0, 0x260056}, {"Pre2", 0x93DE60, 0x7A0, fbits(-320.0f)},
        {"Pre2", 0x93DE60, 0x7A0, fbits(224.0f)}, {"Pre2", 0x93DE60, 0x7A0, fbits(-240.0f)}, {"Pre2", 0x93DE60, 0x7A0, fbits(168.0f)},
        {"Pre6", 0x93FFB0, 0x670, 0x100014}, {"Pre6", 0x93FFB0, 0x670, 0x800002}, {"Pre6", 0x93FFB0, 0x670, fbits(10.0f)},
        {"Tama", 0x9385D0, 0x1D0, fbits(0.9f)}, {"Tama", 0x9385D0, 0x1D0, fbits(1.2f)}, {"Tama", 0x9385D0, 0x1D0, fbits(-153.6f)},
        {"Tama", 0x9385D0, 0x1D0, fbits(76.8f)},
        {"Fire", 0x938230, 0x3A0, 0x260057}, {"Fire", 0x938230, 0x3A0, 0x260066}, {"Fire", 0x938230, 0x3A0, 0x17010C},
        {"Fire", 0x938230, 0x3A0, 0x260040}, {"Fire", 0x938230, 0x3A0, 0x260056},
        {"Bone", 0x93BF40, 0x580, fbits(0.75f)}, {"Bone", 0x93BF40, 0x580, fbits(-0.0392f)}, {"Bone", 0x93BF40, 0x580, fbits(-0.4f)},
        {"Bone", 0x93BF40, 0x580, fbits(0.2f)}, {"Bone", 0x93BF40, 0x580, 0x2001B},
        {"Attach", 0x93CA70, 0x2C0, fbits(0.7f)}, {"Attach", 0x93CA70, 0x2C0, fbits(-0.3f)}, {"Attach", 0x93CA70, 0x2C0, 0x2001B},
        {"Prtc", 0x93B280, 0x490, fbits(25.0f)}, {"Prtc", 0x93B280, 0x490, fbits(26.0f)}, {"Prtc", 0x93B280, 0x490, fbits(120.0f)},
        {"Prtc", 0x93B280, 0x490, 0x18F},
        {"Petit", 0x951C40, 0x2C0, 0x43},
    };
    for (const K& k : ks) { bool ok = hasConst(k.addr, k.len, k.v); if (!ok) printf("  (const 0x%X de %s no encontrada)\n", k.v, k.fn); CHECK(ok); }
}

static void testIds() {
    EnmIdTable t; uint32_t f2 = 0; int32_t id = 0;
    for (int i = 0; i < 32; i++) CHECK(t.slot[i] == 0);
    CHECK(t.set(0x1111, f2, id) == 0 && id == 0 && (f2 & 1) && t.slot[0] == 0x1111);
    uint32_t g2 = 0; int32_t id2 = 0;
    CHECK(t.set(0x2222, g2, id2) == 1 && id2 == 1);
    t.clear(f2, id);                                     // libera el 0
    CHECK(t.slot[0] == 0 && t.slot[1] == 0x2222);
    uint32_t h2 = 0; int32_t id3 = 0;
    CHECK(t.set(0x3333, h2, id3) == 0 && id3 == 0);      // reutiliza el hueco 0
    uint32_t z2 = 0; t.clear(z2, 1); CHECK(t.slot[1] == 0x2222);   // sin bit 1 en flags2: no libera
    t.clear(g2, 99); CHECK(t.slot[1] == 0x2222);         // id fuera de rango
    // tabla llena -> id = -1 y flags2 sin tocar
    EnmIdTable full; for (int i = 0; i < 32; i++) full.slot[i] = 7;
    uint32_t q = 0; int32_t qi = 5; CHECK(full.set(9, q, qi) == -1 && qi == -1 && q == 0);
}

static void testColli() {
    ColliBuffer b; ColliTask t; t.id = 5; t.flags = 0; t.viewZ = -2.0f; t.collPos.x = 1; t.collPos.y = 2; t.collPos.z = 3; t.hit70 = 9; t.hit71 = 9;
    Ev3s ev;
    CHECK(enemySetColliBuff(t, 0xA, b, ev) && b.count == 1 && t.hit70 == 0 && t.hit71 == 0);
    CHECK(b.e[0].task == 5 && b.e[0].pos.y == 2 && b.e[0].flags == 0);
    t.viewZ = 1.0f; CHECK(!enemySetColliBuff(t, 0xA, b, ev) && b.count == 1);        // detras de la camara
    t.flags = F_BIT10; CHECK(enemySetColliBuff(t, 0xA, b, ev) && b.count == 2 && count(ev, E3_COLLI_XFORM) == 1);
    t.flags = F_NOCOLLI; t.hit70 = 4; CHECK(!enemySetColliBuff(t, 0xA, b, ev) && t.hit70 == 0);   // se limpia aunque no entre
    t.flags = 0; t.viewZ = -1; t.hit70 = 4; CHECK(!enemySetColliBuff(t, 0xF, b, ev) && t.hit70 == 4);  // estado 0xF: no toca nada
    b.count = 0xFF; CHECK(!enemySetColliBuff(t, 0xA, b, ev));
    // WaitPos
    WaitPosBuffer w; CHECK(enemySetWaitPosBuff(1, 7, 99, 3.57f, w) && w.count == 1 && w.e[0].distTimes10 == 35 && w.e[0].task == 7 && w.e[0].waitPos == 99);
    CHECK(!enemySetWaitPosBuff(0, 7, 1, 1, w) && !enemySetWaitPosBuff(1 | F_DEAD, 7, 1, 1, w) && w.count == 1);
    w.count = 14; CHECK(!enemySetWaitPosBuff(1, 7, 1, 1, w));
}

static void testSnd() {
    Env3 e = mkEnv(); SndSlot sl[5]; Ev3s ev;
    e.frame739C = 100; e.soundOn = 1;
    CHECK(enSndReq(0x2001B, e, sl, ev) && count(ev, E3_SOUND) == 1 && sl[0].lastFrame == 100);
    e.frame739C = 103; CHECK(!enSndReq(0x2001B, e, sl, ev) && count(ev, E3_SOUND) == 1);       // < 5 fotogramas
    e.frame739C = 105; CHECK(enSndReq(0x2001B, e, sl, ev) && count(ev, E3_SOUND) == 2);
    // el segundo 0x2001B de la tabla (indice 2) nunca se usa: gana la primera aparicion
    CHECK(sl[2].lastFrame == 0);
    e.frame739C = 0; CHECK(!enSndReq(0x2002B, e, sl, ev) && sl[3].lastFrame == 0 && count(ev, E3_SOUND) == 2);  // lastFrame inicial 0, |0-0| < 5: no suena
    e.frame739C = 9; CHECK(enSndReq(0x2002B, e, sl, ev) && sl[3].lastFrame == 9 && count(ev, E3_SOUND) == 3);
    e.soundOn = 0; e.frame739C = 50; CHECK(!enSndReq(0x140016, e, sl, ev) && sl[4].lastFrame == 50);   // actualiza pero no suena
    CHECK(!enSndReq(0x123, e, sl, ev));                                                              // id no listado
    // frame negativo: |d| se toma en valor absoluto
    e.soundOn = 1; sl[1].lastFrame = 10; e.frame739C = 3; CHECK(enSndReq(0x20018, e, sl, ev));
}

static void testMisc() {
    // entryRankingChar
    CHECK(entryRankingChar(0) == 'A' && entryRankingChar(25) == 'Z' && entryRankingChar(26) == '0' && entryRankingChar(35) == '9');
    CHECK(entryRankingChar(36) == '&' && entryRankingChar(37) == '!' && entryRankingChar(38) == '?' && entryRankingChar(39) == '.');
    CHECK(entryRankingChar(40) == '-' && entryRankingChar(41) == ' ' && entryRankingChar(-1) == '@' && entryRankingChar(0x10000 + 3) == 'D');
    // EnPartsChain
    Ev3s ev; enPartsChain(ev, 0xFF, 2, -3, 4, -5);
    CHECK(ev.size() == 2 && ev[0].type == E3_NEW_PARTS && ev[0].c == 2 && ev[1].c == 3);
    CHECK(ev[0].a == 0xFF && ev[0].b == 2 && ev[0].d == -3 && ev[0].e == 4 && ev[0].f == -5);
}

static void testMgr() {
    Env3 e = mkEnv(); Ev3s ev;
    //             0   1   2   3   4  5   6
    std::vector<int32_t> s = {7, 10, 11, 12, -1, 99};
    CHECK(mgrEntryEvent(s, 0, ev) == 5 && ev.size() == 3 && ev[0].a == 1 && ev[2].a == 3);
    // entryEvent2 modo 0 (MENM): una entrada por fallo acumulado
    std::vector<int32_t> m = {0, 0, 21, 22, 23, -1, 55};
    e.missCount = 2; ev.clear();
    CHECK(mgrEntryEvent2(m, 0, e, ev) == 6 && ev.size() == 2 && ev[0].a == 2 && ev[1].a == 3);
    e.missCount = 0; ev.clear(); mgrEntryEvent2(m, 0, e, ev); CHECK(ev.empty());
    // modo 1 (RAND): s[idx]=2 entradas de 3. rand01 = {0.0, 0.0}: toma la 0, luego (n-i=2)*0 -> 0 usada -> sonda a 1
    std::vector<int32_t> r = {0, 1, 2, 30, 31, 32, -1, 77};
    g_r01N = 2; g_r01[0] = 0.0; g_r01[1] = 0.0; g_r01I = 0; ev.clear();
    CHECK(mgrEntryEvent2(r, 0, e, ev) == 7 && ev.size() == 2 && ev[0].a == 3 && ev[1].a == 4);
    // {0.99, 0.99}: (3*0.99)=2.97 -> 2 ; luego (2*0.99)=1.98 -> 1
    g_r01[0] = 0.99; g_r01[1] = 0.99; g_r01I = 0; ev.clear(); mgrEntryEvent2(r, 0, e, ev);
    CHECK(ev.size() == 2 && ev[0].a == 5 && ev[1].a == 4);
    // count >= n: todas en orden
    std::vector<int32_t> r2 = {0, 1, 5, 30, 31, -1};
    ev.clear(); CHECK(mgrEntryEvent2(r2, 0, e, ev) == 6 && ev.size() == 2 && ev[0].a == 3 && ev[1].a == 4);
}

static void testEtc() {
    Env3 e = mkEnv(); Ev3s ev; EtcTask t;
    // St4_02TimerBunki: v50 = 120 -> 120*50/60 = 100 fotogramas
    t.v50 = 120; e.onScreen = 3; e.missCount = 9;
    etcSt4TimerBunki(t, e, ev);
    CHECK(t.state == 1 && t.a148 == 99 && e.missCount == 0 && ev.empty());
    for (int i = 0; i < 98; i++) etcSt4TimerBunki(t, e, ev);
    CHECK(t.a148 == 1 && ev.empty());
    etcSt4TimerBunki(t, e, ev); CHECK(count(ev, E3_TASK_CLOSE) == 1 && e.missCount == 1);
    // Tama: la alpha sube 1.2*0.9/v50 por fotograma hasta 0.9, luego baja
    St4Fade f; EtcTask u; u.v50 = 36; u.v54 = 18; ev.clear();
    etcSt4Tama(u, f, e, ev);
    CHECK(u.state == 1 && near(f.alpha, 1.2f * 0.9f / 36.0f) && f.u24 == -153 && f.u28 == 76);
    int n = 1; while (u.state == 1 && n < 100) { etcSt4Tama(u, f, e, ev); n++; }
    CHECK(u.state == 2 && near(f.alpha, 0.9f - 1.2f * 0.9f / 18.0f) && n >= 30 && n <= 31);   // 0.9/0.03 = 30 pasos; al llegar entra en el caso 2 el mismo fotograma
    int m = 0; while (!find(ev, E3_TASK_CLOSE) && m < 100) { etcSt4Tama(u, f, e, ev); m++; }
    CHECK(f.alpha == 0.0f && m > 5 && m < 20);
    e.freeze = 1; float a0 = f.alpha; int u24 = f.u24; etcSt4Tama(u, f, e, ev); CHECK(f.alpha == a0 && f.u24 == u24);   // congelado
    e.freeze = 0;
    // BranchMark
    Ev3s ev2; EtcTask b; BranchEnv be; be.gameState = 0xA; be.gameKind = 1;
    b.p14 = 3; be.flagsTable[3] = 0x03; b.a14C = 4; b.a150 = 10; b.a154 = 5; b.a158 = 20; be.camPath = 4; be.camFrame = 9;
    etcBranchMark(b, be, ev2); CHECK(ev2.empty() && b.w16C == 0);
    be.camFrame = 10; etcBranchMark(b, be, ev2); CHECK(b.w16C == 1 && count(ev2, E3_CALL) == 1 && ev2[0].a == 0xA10B00 && ev2[0].b == 3);
    be.camPath = 5; be.camFrame = 20; etcBranchMark(b, be, ev2); CHECK(count(ev2, E3_TASK_CLOSE) == 1 && be.d73CC == 0);
    b.flags = F_HIT; be.camPath = 0; b.p14 = 0x10; be.flagsTable[0x10] = 3; etcBranchMark(b, be, ev2);
    CHECK(!(b.flags & F_HIT) && b.a148 == 0x10088 && be.d500F05 == 1 && be.d73CC == 1 && be.d500E11 == 1);
    be.flagsTable[0x10] = 0x23; Ev3s ev3; be.d500F05 = 1; b.a154 = 77; etcBranchMark(b, be, ev3); CHECK(count(ev3, E3_TASK_CLOSE) == 1 && be.d500F05 == 0);   // bit 0x20: la marca se cierra
    Ev3s ev4; BranchEnv bz; bz.gameState = 3; EtcTask c; etcBranchMark(c, bz, ev4); CHECK(count(ev4, E3_TASK_CLOSE) >= 1);   // fuera del arcade: se cierra (y la 2a condicion de cierre tambien se cumple con camino 0/fotograma 0)
    // FireBall
    struct P { static V3 f(int type, int frame, void*) { V3 v; v.x = (float)type; v.y = (float)frame; v.z = 0; return v; } };
    EtcTask fb; fb.unk14 = 6; Ev3s ev5;
    etcFireBall(fb, P::f, nullptr, e, ev5);
    CHECK(fb.state == 1 && fb.a158 == 1 && fb.pos.x == 6 && fb.pos.y == 0 && fb.a150 == 0x260066 && count(ev5, E3_SOUND) == 1 && ev5[0].a == 0x17010C);
    CHECK(fb.a14C == 0x260058);                            // anim: 0x260057 + 1
    for (int i = 0; i < 44; i++) etcFireBall(fb, P::f, nullptr, e, ev5);
    CHECK(fb.a158 == 0x2D && (fb.flags & F_RESPONDED) && fb.state == 2 && fb.a154 == 0x16 - 1);   // pasa al caso 2 y descuenta ya en ese fotograma
    CHECK(fb.a148 == 0x260040);
    for (int i = 0; i < 25 && !find(ev5, E3_TASK_CLOSE); i++) etcFireBall(fb, P::f, nullptr, e, ev5);
    CHECK(count(ev5, E3_TASK_CLOSE) == 1);
}

static void testSmall() {
    Env3 e = mkEnv(); Ev3s ev;
    // Attach
    AttachObj a; a.pos.y = 5.0f; a.vel.y = -10.0f; a.acc.y = -1.0f; a.vel.x = 10; a.rotVel[0] = 100; a.rotVel[1] = -101; a.timer = 2; a.alpha = 0.05f;
    a.col[0] = 0.5f; a.colStep[0] = 0.3f; a.col[1] = 0.1f; a.colStep[1] = 0.3f;
    attachCtrlPre(a, e, 0.0f, ev);
    CHECK(e.groundHeight == 0.0f && near(a.vel.y, 3.3f) && near(a.pos.y, 1.0f + 3.3f) && near(a.vel.x, 7.0f));  // rebote: y=-11 -> *-0.3
    CHECK(a.rotVel[0] == 50 && a.rotVel[1] == -50 && (a.flags & F_RESPONDED) && count(ev, E3_SNDREQ) == 1 && ev[0].a == 0x2001B);
    CHECK(a.timer == 1 && a.col[0] == 0.5f && near(a.alpha, 0.01f));
    attachCtrlPre(a, e, 0.0f, ev);      // timer 1 -> 0: baja el color; alpha <= 0 -> cierra; no repite el sonido
    CHECK(a.timer == 0 && near(a.col[0], 0.2f) && a.col[1] == 0.0f && a.alpha < 0 && count(ev, E3_TASK_CLOSE) == 1 && (a.flags & F_DEAD) && count(ev, E3_SNDREQ) == 1);
    // DamSphere
    DamSphere s; DamOwner o; Ev3s ev2;
    s.flags = F_HIT | F_PLAYER0; o.flags2 = F_GATE;
    damSphereCtrlPre(s, o, ev2);
    CHECK((o.flags & F_HIT) && (o.flags & F_PLAYER0) && !(o.flags2 & F_GATE) && (o.flags2 & F_RESPONDED) && (s.flags & F_NOCOLLI) && !(s.flags & (F_HIT | F_PLAYER0)));
    DamSphere s2; DamOwner o2; s2.flags = F_HIT | F_PLAYER1; damSphereCtrlPre(s2, o2, ev2);
    CHECK((o2.flags & F_PLAYER1) && (o2.flags2 & F_GATE) && (o2.flags2 & F_RESPONDED));
    DamSphere s3; DamOwner o3; s3.flags = F_HIT; o3.flags = 0x100; damSphereCtrlPre(s3, o3, ev2);    // duenyo invulnerable: no hereda, queda activa
    CHECK(!(o3.flags & F_HIT) && !(s3.flags & F_NOCOLLI) && !(s3.flags & F_HIT));
    DamSphere s4; DamOwner o4; o4.flags = F_DEAD; Ev3s ev3; damSphereCtrlPre(s4, o4, ev3); CHECK(count(ev3, E3_TASK_CLOSE) == 1);
    // Armor
    ArmorTask at; at.armorId = 12; ArmorPart ap; Ev3s ev4;
    armorPreDisp(at, ap, ev4); CHECK(count(ev4, E3_NEW_ATTACH) == 1 && ev4[0].a == 12 && ap.cnt168 == 1 && count(ev4, E3_TASK_CLOSE) == 1);
    ap.flag154 = 0x100; Ev3s ev5; armorPreDisp(at, ap, ev5); armorNotDisp(at, ap, 5, ev5); CHECK(ev5.empty() && ap.a0 == 0);
    ap.flag154 = 0; armorNotDisp(at, ap, 77, ev5); CHECK(ap.a0 == 77 && ap.cnt168 == 2 && count(ev5, E3_NEW_ATTACH) == 1);
    // PetitZombie
    PetitZombie z; PetitArea ar; ar.xMin = -10; ar.xMax = 10; ar.zMin = -20; ar.zMax = 20; ar.pathId = 3; ar.frame = 40;
    g_cam.c = V3(); Env3 pe = mkEnv(); TestCam cam2; (void)cam2;
    struct FC : Camera3 { V3 worldToView(const V3& w) const override { return w; } V3 viewToWorld(const V3& v) const override { return v; } V3 normalize(const V3& v) const override { return v; } } fc;
    fc.pathId = 3; fc.frame = 40; pe.cam = &fc;
    Ev3s ev6; z.mode = 2; z.x = 11; z.z = 5; z.ang = 0x1000;
    petitCtrl(z, ar, pe, ev6);
    CHECK(z.x == 10 && z.ang == 0x1000 - 0x100 && z.f3C == 1.0f && count(ev6, E3_CALL) == 1 && ev6[0].a == C3_PETIT_CAM);
    z.x = -11; z.ang = 0x5000; petitCtrl(z, ar, pe, ev6); CHECK(z.x == -10 && z.ang == 0x5100);
    z.x = 0; z.z = 30; z.ang = 0x9000; petitCtrl(z, ar, pe, ev6); CHECK(z.z == 20 && z.ang == 0x9100);
    z.z = -30; z.ang = 0x9000; petitCtrl(z, ar, pe, ev6); CHECK(z.z == -20 && z.ang == 0x8F00);
    z.x = 11; z.z = 30; z.ang = 0x9000; petitCtrl(z, ar, pe, ev6); CHECK(z.ang == 0x8F00 && z.x == 10 && z.z == 20);   // x gira (0x9000 < 0xC000 -> -0x100); z no gira porque x ya lo hizo
    z.mode = 1; z.p18 = 1; z.ang = 0; petitCtrl(z, ar, pe, ev6); CHECK(z.ang == 0x40);
    z.p18 = 0; petitCtrl(z, ar, pe, ev6); CHECK(z.ang == 0);
    z.flags = F_HIT; Ev3s ev7; petitCtrl(z, ar, pe, ev7); CHECK(!(z.flags & 1) && !(z.flags & F_HIT) && count(ev7, E3_MOTION) == 1 && ev7[0].a == 0x43 && ev7[1].a == C3_PETIT_HIT);
    pe.freeze = 1; float f3 = z.f3C; petitCtrl(z, ar, pe, ev7); CHECK(z.f3C == f3);
}

static void testWeak() {
    Env3 e = mkEnv(); Ev3s ev; WeakOwner ow; WeakTask t;
    t.id = 5; t.slot = 2; ow.weakSlot[2] = 5; t.animStart = 0x1008B; t.animEnd = 0x1009E; t.animCur = t.animStart; t.viewZ = -1;
    // sin golpes: contador baja pero no pasa de 0; w |= 0x10 (visible) y anima
    weakCtrlPre(t, ow, e, ev);
    CHECK(t.counter == 0 && t.state == 0 && (t.w & 0x10) && t.animCur == 0x1008C);
    // golpe del jugador 1 (bit 2) y 2 (bit 4): resultado 1 + 2 = 3, pasa a estado 1
    t.w = 8 | 2 | 4; weakCtrlPre(t, ow, e, ev);
    CHECK(t.result == 3 && t.state == 1 && t.counter == 1 && (t.w & (8 | 2 | 4)) == 0);   // cae al caso 1 el mismo fotograma: contador 0 -> 1
    for (int i = 0; i < 0x95; i++) weakCtrlPre(t, ow, e, ev);
    CHECK(t.state == 1 && t.counter == 0x96);
    weakCtrlPre(t, ow, e, ev); CHECK(t.state == 2 && t.counter == 0);
    // bit 0x80 en w: no cuenta el golpe
    WeakTask t2; t2.state = 0; t2.w = 8 | 0x80; t2.viewZ = -1; t2.counter = 1; weakCtrlPre(t2, ow, e, ev); CHECK(t2.state == 0 && t2.counter == 0);
    // modo de camara != 2: sin animacion
    e.mode7218 = 1; t2.animStart = t2.animCur = 10; t2.animEnd = 20; t2.w = 0; weakCtrlPre(t2, ow, e, ev); CHECK(t2.animCur == 10 && !(t2.w & 0x10)); e.mode7218 = 2;
    // viewZ > 0: tampoco
    t2.viewZ = 1; weakCtrlPre(t2, ow, e, ev); CHECK(t2.animCur == 10);
    // golpe: hereda al duenyo
    WeakTask t3; t3.flags = F_HIT; t3.viewZ = -1; weakCtrlPre(t3, ow, e, ev); CHECK((ow.flags & F_HIT) && (t3.flags & F_RESPONDED) && !(t3.flags & F_HIT));
    // duenyo muerto: libera el hueco y cierra
    Ev3s ev2; ow.flags = F_DEAD; weakCtrlPre(t, ow, e, ev2); CHECK(ow.weakSlot[2] == 0 && count(ev2, E3_TASK_CLOSE) == 1);
    // con parte: la parte soltada (flags120 & 0x01000000) tambien cierra y f20 = part.f_D8
    WeakOwner o2; WeakTask t4; t4.partIdx = 3; t4.slot = 1; t4.id = 8; o2.weakSlot[1] = 8; o2.part[3].f_D8 = -2; o2.part[3].flags120 = F_HITTED;
    t4.animStart = t4.animCur = 1; t4.animEnd = 9; Ev3s ev3; weakCtrlPre(t4, o2, e, ev3);
    CHECK(o2.weakSlot[1] == 0 && count(ev3, E3_TASK_CLOSE) == 1 && t4.animCur == 2);
}

static void testBone() {
    Env3 e = mkEnv(); Ev3s ev; BoneObj b; BoneInput in; b.ground = 0.0f; b.partHeight = 2.0f; b.alpha = 1.0f;
    // estado 0 sin condicion: nada
    boneCtrlPre(b, false, in, e, ev); CHECK(b.state == 0 && e.groundHeight == 0.0f);
    // parte muerta + bit 0x400: pasa a 1 y evalua el 1 en el mismo fotograma: ground + height < partY -> sigue en 1
    in.partDead = true; in.ownerBit400 = true; in.partY = 10.0f;
    boneCtrlPre(b, false, in, e, ev); CHECK(b.state == 1);
    in.partY = 1.0f; boneCtrlPre(b, false, in, e, ev); CHECK(b.state == 2);       // 0+2 < 1 falso: avanza
    // estado 2: lanza el hueso. rand: &7 -> 3 (0x30); %2 -> 1 => signo -1; &7 -> 2 (0x20)
    g_seq[0] = 3; g_seq[1] = 1; g_seq[2] = 2; g_seqN = 3; g_seqI = 0;
    in.p50.x = 4; in.p50.y = 8; in.p50.z = 12; in.p60.x = 0; in.p60.y = 0; in.p60.z = 0; in.angles[0] = 7; in.angles[1] = 8; in.angles[2] = 9;
    e.soundOn = 1; boneCtrlPre(b, false, in, e, ev);
    CHECK(b.state == 3 && count(ev, E3_PART_FLAGS) == 1 && ev[0].a == 0x200 && (b.flags & 0x200));
    // en el mismo fotograma el estado 3 integro: rot += rotVel; vel = 0.75*(p50-p60) + acc; pos = p50 + vel
    CHECK(b.rotVel[0] == 0x30 && b.rotVel[1] == -0x20 && b.rotVel[2] == 0 && b.rot[0] == 7 + 0x30 && b.rot[1] == 8 - 0x20);
    CHECK(near(b.vel.x, 3.0f) && near(b.vel.y, 6.0f - 0.0392f) && near(b.vel.z, 9.0f) && near(b.pos.x, 7.0f) && near(b.pos.y, 8.0f + 6.0f - 0.0392f));
    // caida y rebote en el suelo (stage != 0 -> 0.5 / -0.4)
    b.pos.y = 0.5f; b.vel.y = -3.0f; b.vel.x = 4.0f; Ev3s ev2; boneCtrlPre(b, false, in, e, ev2);
    CHECK(near(b.vel.y, (-3.0f - 0.0392f) * -0.4f, 1e-3f) && near(b.vel.x, 2.0f) && (b.flags & F_RESPONDED) && count(ev2, E3_SNDREQ) == 1);
    e.soundOn = 0; BoneObj c; c.state = 3; c.pos.y = 0.5f; c.vel.y = -3.0f; c.vel.x = 5.0f; c.vel.z = 5.0f; c.acc.y = 0.0f; Ev3s ev3; boneCtrlPre(c, false, in, e, ev3);
    CHECK(near(c.vel.y, 0.3f) && near(c.vel.x, 1.0f) && near(c.vel.z, 1.0f));       // 0.2 / -0.1
    // desvanece tras el rebote
    c.alpha = 0.05f; c.flags |= F_RESPONDED; boneCtrlPre(c, false, in, e, ev3); boneCtrlPre(c, false, in, e, ev3);
    CHECK(count(ev3, E3_TASK_CLOSE) == 1 && (c.flags & F_DEAD));
    // congelado: nada
    e.freeze = 2; BoneObj d; d.state = 3; d.pos.y = 5; boneCtrlPre(d, false, in, e, ev3); CHECK(d.pos.y == 5);
}

static void testVtx() {
    Env3 e = mkEnv(); e.ground = 0.0f; VtxCtrl c; c.active = 10; VtxEff p;
    p.life = 30; p.scale = 0.5f; p.scaleStep = 0.1f; p.pos.y = 5.0f; p.vel.y = -1.0f; p.acc.y = -0.5f; p.col[0] = 1.0f; p.colStep[0] = -0.4f; p.stage = 4; p.kind = 1;
    vtxCtrlPrtc(p, c, e, 5, 3);
    CHECK(p.scale == 0.5f && near(p.vel.y, -1.5f) && near(p.pos.y, 3.5f) && near(p.col[0], 0.6f) && p.life == 29 && p.kind == 1);
    // life == 26: escala 1.0 y paso 1/25
    p.life = 26; p.scale = 0.3f; vtxCtrlPrtc(p, c, e, 5, 3); CHECK(near(p.scaleStep, 0.04f) && near(p.scale, 1.0f));
    // life <= 25: escala -= paso
    p.life = 20; p.scale = 1.0f; p.scaleStep = 0.04f; vtxCtrlPrtc(p, c, e, 5, 3); CHECK(near(p.scale, 0.96f));
    // aterriza: <= ground - 0.2
    VtxEff q; q.life = 50; q.pos.y = 0.1f; q.vel.y = -1.0f; q.acc.y = 0.0f; q.stage = 4; q.kind = 1; q.vel.x = 3; vtxCtrlPrtc(q, c, e, 5, 3);
    CHECK(q.landed == 1 && q.pos.y == 0.0f && q.vel.x == 0.0f);
    // muerte por gp733C < gp7380
    VtxEff r; r.life = 50; r.kind = 1; r.stage = 4; c.active = 10; vtxCtrlPrtc(r, c, e, 1, 2); CHECK(r.kind == 0 && c.active == 9 && r.life == 50);
    // muerte por vida agotada
    VtxEff s; s.life = 1; s.kind = 1; s.stage = 4; vtxCtrlPrtc(s, c, e, 5, 3); CHECK(s.kind == 0 && c.active == 8);
    // muerte por exceso de particulas con poca vida
    VtxEff u; u.life = 0x3C; u.kind = 1; u.stage = 4; c.active = 0x18F; vtxCtrlPrtc(u, c, e, 5, 3); CHECK(u.kind == 0 && c.active == 0x18E);
    // cambio de etapa de color: stage 0 -> 1, stageLife = 120 - 120*0.185 = 97.8
    VtxEff w; w.life = 120; w.stage = 0; w.stageLife = 120.0f; w.kind = 1; w.landed = 1; w.col[0] = 1.0f; w.col[1] = 0.9f; w.col[2] = 0.4f; c.active = 0;
    vtxCtrlPrtc(w, c, e, 5, 3);
    CHECK(w.stage == 1 && near(w.stageLife, 120.0f - 120.0f * 0.185f) && near(w.colStep[1], (0.4219f - 0.9f) / (119.0f - w.stageLife)));
    // Fire
    VtxCtrl c2; c2.active = 3; VtxEff f; f.kind = 2; f.fireFrame = 0; f.fireMax = 2;
    vtxCtrlFire(f, c2, e); vtxCtrlFire(f, c2, e); CHECK(f.kind == 2 && f.fireFrame == 2);
    vtxCtrlFire(f, c2, e); CHECK(f.kind == 0 && c2.active == 2);
    int32_t k[5] = {1, 2, 3, 4, 5}; vtxReqBufInit(k, 5); CHECK(k[0] == 0 && k[4] == 0);
}

// ---- ThrowObj ----
static void testThrowInit() {
    Env3 e = mkEnv(); g_cam.c.x = 10; g_cam.c.y = 20; g_cam.c.z = 30; Ev3s ev;
    ThrowObj o; ThrowOwner ow; e.coop = 0;
    throwInit1(o, 0, 40, ow, true, e, ev);
    CHECK(near(o.target.x, 10 + (-1.5f * 160.0f) / 500.0f) && near(o.target.y, 20.0f) && near(o.target.z, 30 - 1.5f));
    CHECK(o.timer == 40 && o.total == 40 && fbits(o.scale) == 0x3E4CCCCD && o.animStart == 0x1B008D && o.animEnd == 0x1B0096 && o.animCur == 0x1B008D);
    CHECK(o.radius == 0.5f && (ow.flags2 & 0x20000000) && o.flags == (0x10001 | F_GATE) && e.attackGate == 1 && o.alpha == 1.0f && o.state == 0);
    CHECK(count(ev, E3_SOUND) == 1 && ev[0].a == 0xE040F && count(ev, E3_ALLOC_BUF) == 1 && o.hasBuf);
    ThrowObj o2; ThrowOwner ow2; Ev3s ev2; throwInit1(o2, 1, 40, ow2, false, e, ev2);
    CHECK(near(o2.target.x, 10 + (-1.5f * -160.0f) / 500.0f) && o2.flags == 0x10001);
    e.coop = 1; ThrowObj o3; ThrowOwner ow3; Ev3s ev3; throwInit1(o3, 0, 40, ow3, false, e, ev3);
    CHECK(near(o3.target.x, 10.0f) && o3.coop == 1);
    // Init2: rand = {100 (x), 50 (y), 0 (signo x -> negativo), 1 (signo y -> positivo)}
    e.coop = 0; g_seq[0] = 100; g_seq[1] = 50; g_seq[2] = 0; g_seq[3] = 1; g_seqN = 4; g_seqI = 0;
    ThrowObj t; t.type = 5; throwInit2(t, 77, e);
    CHECK(near(t.target.x, 10 + (-1.5f * -100.0f) / 500.0f) && near(t.target.y, 20 + (-1.5f * 50.0f) / 500.0f) && near(t.target.z, 28.5f));
    CHECK(t.timer == 77 && t.total == 77 && fbits(t.scale) == fbits(0.7f) && t.animStart == 0x290000 && t.radius == 3.0f && t.animCur == 0x290000 && t.flags == 0x10001);
    ThrowObj t8; t8.type = 8; g_seqI = 0; throwInit2(t8, 1, e); CHECK(t8.animStart == 0x29001E && t8.radius == 2.0f && fbits(t8.scale) == fbits(0.1f));
    ThrowObj t9; t9.type = 9; g_seqI = 0; throwInit2(t9, 1, e); CHECK(t9.animEnd == 0x2900F1 && fbits(t9.scale) == fbits(0.5f) && t9.radius == 3.0f);
    ThrowObj t0; t0.type = 3; g_seqI = 0; throwInit2(t0, 1, e); CHECK(t0.animStart == 0x260000 && t0.animEnd == 0x26001F && fbits(t0.radius) == fbits(1.2f));
    // Init3
    ThrowObj t3; t3.alpha = 5; throwInit3(t3); CHECK(t3.animStart == 0x260067 && t3.animEnd == 0x260086 && t3.alpha == 0.0f && t3.flags == 0x10001 && t3.timer == 0);
    // Init5 / Init7: timer = ((n*5 + base) * 50) / 60 sin signo
    ThrowObj t5; Ev3s ev5; throwInit5(t5, 4, 99, ev5);
    CHECK(t5.total == 99 && t5.timer == ((4 * 5 + 0x50) * 0x32) / 60 && t5.timer == 83 && near(t5.scaleStep, 0.5f / 83.0f) && fbits(t5.radius) == 0x3FD9999A);
    CHECK(count(ev5, E3_ALLOC_BUF) == 1 && ev5[0].a == 0x790 && ev5[0].b == 8);
    ThrowObj t7; Ev3s ev7; throwInit7(t7, 4, 99, ev7); CHECK(t7.timer == ((4 * 5 + 0xA4) * 0x32) / 60 && count(ev7, E3_SOUND) == 1 && ev7[1].a == 0x100019);
    // Init6
    ThrowObj t6; t6.type = 1; g_seqI = 0; throwInit6(t6, 33, e); CHECK(t6.rotSpeed == -0x1000 && t6.animStart == 0x800002 && fbits(t6.radius) == 0x3FE66666 && t6.scale == 1.0f);
    ThrowObj t62; t62.type = 4; g_seqI = 0; throwInit6(t62, 33, e); CHECK(t62.rotSpeed == 0x1000);
    g_cam.c = V3();
}

static void testThrowPre() {
    Env3 e = mkEnv(); g_cam.c = V3(); e.coop = 0;
    // ---- tipo 1: vuelo lineal de `total` fotogramas ----
    {
        ThrowObj o; ThrowOwner ow; Ev3s ev; o.flags = 0x10001; o.player = 1; o.total = 4; o.timer = 4; o.alpha = 1.0f; o.scale = 0.2f;
        o.startPos.z = 0; o.target.z = -8.0f; o.hasBuf = true; e.attackSlot[1] = 1;
        for (int i = 0; i < 4; i++) { throwCtrlPre1(o, ow, e, ev); }
        CHECK(near(o.pos.z, -8.0f) && o.state == 0 && o.timer == 0 && ev.empty());
        throwCtrlPre1(o, ow, e, ev);            // timer -1 < 0 -> hiere (caso 1) y entra en el 2
        CHECK(count(ev, E3_DAMAGE_PLAYER) == 1 && ev[0].a == 1 && ev[0].b == 1 && ev[0].c == 7 && (o.flags & F_HITTED) && (o.flags & F_NOCOLLI));
        CHECK(o.state == 2 && o.timer == 0xE && near(o.alpha, 0.96f));
        for (int i = 0; i < 14; i++) throwCtrlPre1(o, ow, e, ev);
        CHECK((o.flags & F_FROZEN_ANIM));
        int n = 0; while (!find(ev, E3_TASK_CLOSE) && n < 100) { throwCtrlPre1(o, ow, e, ev); n++; }
        CHECK(count(ev, E3_TASK_CLOSE) == 1 && e.attackSlot[1] == 0 && count(ev, E3_FREE_BUF) == 1);
    }
    // ---- tipo 1: derribado ----
    {
        ThrowObj o; ThrowOwner ow; Ev3s ev; e.attackGate = 1; o.flags = 0x10001 | F_HIT | F_PLAYER1 | F_GATE; o.alpha = 1.0f; o.scale = 0.2f; o.player = 0; o.total = 10; o.timer = 10; ow.flags2 = 0x20000000;
        throwCtrlPre1(o, ow, e, ev);
        CHECK(o.state == 5 && ev[0].type == E3_SOUND && ev[0].a == 0xE0411 && count(ev, E3_HITNUM) == 1 && ev[1].a == 1 && (o.flags & F_NOCOLLI) && (o.flags & F_FROZEN_ANIM));
        CHECK(near(o.alpha, 0.5f - 0.033333335f) && near(o.scale, 0.2f + 0.06666667f) && !(ow.flags2 & 0x20000000));
        int n = 0; while (!find(ev, E3_TASK_CLOSE) && n < 100) { throwCtrlPre1(o, ow, e, ev); n++; }
        CHECK(n >= 13 && n <= 15 && e.attackGate == 0 && e.attackSlot[0] == 0);       // 0.5/0.0333 = 15 pasos en total (incluido el del golpe)
    }
    // ---- tipo 1: el duenyo muere -> DEAD ----
    { ThrowObj o; ThrowOwner ow; Ev3s ev; ow.flags2 = 0x20; o.state = 0; o.total = 1; o.timer = 5; throwCtrlPre1(o, ow, e, ev); CHECK(o.flags & F_DEAD); }
    // ---- tipo 2: llega y hiere (jugador segun x en pantalla) ----
    {
        ThrowObj o; ThrowOwner ow; Ev3s ev; ow.live = 3; o.flags = 0x10001; o.type = 0; o.total = 10; o.radius = 1.2f; o.scale = 0.1f; o.alpha = 1.0f;
        o.pos.x = 0; o.pos.y = 0; o.pos.z = -50.0f; o.target.x = 0; o.target.y = 0; o.target.z = -1.5f;
        int n = 0; while (!find(ev, E3_DAMAGE_PLAYER) && n < 200) { throwCtrlPre2(o, ow, e, ev); n++; }
        CHECK(n == 49 && ev[0].a == 0 && ev[0].b == 1 && ev[0].c == 7 && near(o.pos.z, -1.0f) && o.state == 2 && o.timer == 0xF - 1);
        CHECK((o.flags & F_HITTED) && (o.flags & F_NOCOLLI) && e.missCount == 0 && ow.live == 3);
        int m = 0; while (!find(ev, E3_TASK_CLOSE) && m < 100) { throwCtrlPre2(o, ow, e, ev); m++; }
        CHECK(near(o.alpha, 1.0f - 0.04f * (m + 1), 1e-3f) && ow.live == 2 && !(o.flags & F_DEAD));
    }
    // ---- tipo 2: llega por la izquierda de la pantalla -> jugador 1; coop -> `solo` ----
    {
        ThrowObj o; ThrowOwner ow; Ev3s ev; o.flags = 0x10001; o.total = 10; o.radius = 1.2f; o.scale = 0.1f; o.alpha = 1.0f;
        o.pos.x = 0.5f; o.pos.z = -22.0f; o.target.x = 0.5f; o.target.z = -1.5f;      // x = +0.5: sx = 0.5*500/z < 0 (la z de camara es negativa) -> jugador 1
        int n = 0; while (!find(ev, E3_DAMAGE_PLAYER) && n < 200) { throwCtrlPre2(o, ow, e, ev); n++; }
        CHECK(find(ev, E3_DAMAGE_PLAYER) && ev[0].a == 1);
        Env3 ec = mkEnv(); ec.coop = 1; ec.solo = 1;   // coop: siempre env.solo
        ThrowObj p; ThrowOwner pw; Ev3s ev2; p.flags = 0x10001; p.total = 10; p.radius = 1.2f; p.scale = 0.1f; p.alpha = 1.0f; p.pos.x = -0.5f; p.pos.z = -22.0f; p.target.x = -0.5f; p.target.z = -1.5f;
        n = 0; while (!find(ev2, E3_DAMAGE_PLAYER) && n < 200) { throwCtrlPre2(p, pw, ec, ev2); n++; }
        CHECK(ev2[0].a == 1);
        ThrowObj q; ThrowOwner qw; Ev3s ev3; q.flags = 0x10001; q.total = 10; q.radius = 1.2f; q.scale = 0.1f; q.alpha = 1.0f; q.pos.x = -0.5f; q.pos.z = -22.0f; q.target.x = -0.5f; q.target.z = -1.5f;
        n = 0; while (!find(ev3, E3_DAMAGE_PLAYER) && n < 200) { throwCtrlPre2(q, qw, e, ev3); n++; }
        CHECK(ev3[0].a == 0);     // x negativo -> sx positivo -> jugador 0
    }
    // ---- tipo 2: sale de pantalla -> se borra ----
    {
        ThrowObj o; ThrowOwner ow; Ev3s ev; ow.live = 2; o.flags = 0x10001; o.total = 10; o.radius = 1.2f; o.scale = 0.1f; o.alpha = 1.0f;
        o.pos.x = 1000.0f; o.pos.z = -10.0f; o.target.x = 1000.0f; o.target.z = -9.0f;       // sx = 1000*500/-10 = -50000
        throwCtrlPre2(o, ow, e, ev); CHECK(count(ev, E3_TASK_CLOSE) == 1 && ow.live == 1 && !(o.flags & F_DEAD));
    }
    // ---- tipo 2: derribado, por tipo ----
    {
        ThrowOwner ow; ow.live = 5;
        ThrowObj a; Ev3s ev; a.flags = 0x10001 | F_HIT | F_PLAYER0; a.type = 5; a.pos.x = 3;
        throwCtrlPre2(a, ow, e, ev);
        CHECK(ev[0].a == 0x20025 && find(ev, E3_EFFECT) && find(ev, E3_EFFECT)->b == 0x30 && find(ev, E3_EFFECT)->v.x == 3 && a.animCur == 0x260040 && a.animEnd == 0x260056 && count(ev, E3_HITNUM) == 1 && ev[1].a == 0);
        CHECK(count(ev, E3_TASK_CLOSE) == 1 && ow.live == 4);                // tipo 5: borra en el acto
        ThrowObj b; Ev3s ev2; b.flags = 0x10001 | F_HIT; b.type = 8; e.cam = &g_cam; g_cam.yaw = 12; g_cam.pitch = 34;
        throwCtrlPre2(b, ow, e, ev2); CHECK(ev2[0].a == 0x1B000E && find(ev2, E3_SPAWN_HIT)->a == 0x39 && find(ev2, E3_SPAWN_HIT)->b == 12 && find(ev2, E3_SPAWN_HIT)->c == 34);
        ThrowObj c; Ev3s ev3; c.flags = 0x10001 | F_HIT; c.type = 1; throwCtrlPre2(c, ow, e, ev3);
        CHECK(ev3[0].a == 0x18020F && c.state == 5 && count(ev3, E3_TASK_CLOSE) == 0 && c.animCur == 0x260041);   // sigue la animacion de muerte
        c.animCur = 0x260056; throwCtrlPre2(c, ow, e, ev3); CHECK(count(ev3, E3_TASK_CLOSE) == 1);     // animCur+1 = 0x260057 >= 0x260057 -> borra
    }
    // ---- tipo 6: marca DEAD y suma un fallo al golpear ----
    {
        ThrowObj o; ThrowOwner ow; Ev3s ev; ow.live = 2; o.flags = 0x10001; o.type = 1; o.total = 10; o.radius = 1.8f; o.scale = 1.0f; o.alpha = 1.0f;
        o.pos.z = -30.0f; o.target.z = -1.5f; e.missCount = 4;
        int n = 0; while (!find(ev, E3_DAMAGE_PLAYER) && n < 200) { throwCtrlPre6(o, ow, e, ev); n++; }
        CHECK(e.missCount == 5 && ev[0].a == 0);
        int m = 0; while (!find(ev, E3_TASK_CLOSE) && m < 100) { throwCtrlPre6(o, ow, e, ev); m++; }
        CHECK((o.flags & F_DEAD) && ow.live == 1);
        ThrowObj h; ThrowOwner hw; Ev3s ev2; h.flags = 0x10001 | F_HIT | F_PLAYER1; hw.live = 1; throwCtrlPre6(h, hw, e, ev2);
        CHECK(ev2[0].type == E3_SPAWN_HIT && ev2[0].a == 0x3A && ev2[1].a == 0x100014 && (h.flags & F_DEAD) && hw.live == 0 && count(ev2, E3_TASK_CLOSE) == 1 && ev2[2].type == E3_HITNUM && ev2[2].a == 1);
    }
    // ---- Pos ----
    {
        ThrowObj o; Ev3s ev; o.animStart = 5; o.animCur = 5; o.animEnd = 7; o.pos.z = -3; o.player = 1; e.attackSlot[1] = 0;
        CHECK(throwCtrlPos1(o, e, ev) && o.animCur == 6 && e.attackSlot[1] == 1 && o.animT == 1.0f && near(o.view.z, -3.0f));
        throwCtrlPos1(o, e, ev); CHECK(o.animCur == 5);                   // vuelve al inicio
        o.animT = 10.0f; throwCtrlPos1(o, e, ev); CHECK(o.animT == 0.0f);
        unsigned cur = o.animCur; o.flags = F_FROZEN_ANIM; throwCtrlPos2(o, e, ev); CHECK(o.animCur == cur);
        CHECK(ev.back().type == E3_CAM_ENTRY && ev.back().v.z == -3.0f);
        o.flags = F_HITTED; Ev3s ev2; throwCtrlPos2(o, e, ev2); CHECK(near(ev2.back().v.z, -1.5f));
        o.flags = F_RESPONDED; CHECK(!throwCtrlPos2(o, e, ev2) && !throwCtrlPos1(o, e, ev2));
        ThrowObj p; p.rotSpeed = 0x1000; p.animStart = 1; p.animCur = 1; p.animEnd = 3; Ev3s ev3;
        CHECK(throwCtrlPos6(p, e, ev3) && p.rotY == 0x1000 && p.animCur == 2);
        p.flags = F_NOCOLLI; CHECK(!throwCtrlPos6(p, e, ev3) && p.rotY == 0x2000);
        p.flags = F_FROZEN_ANIM; throwCtrlPos6(p, e, ev3); CHECK(p.rotY == 0x2000);
    }
}

int main() {
    bool bin = loadBin();
    testTables(bin);
    testIds(); testColli(); testSnd(); testMisc(); testMgr(); testEtc(); testSmall(); testWeak(); testBone(); testVtx();
    testThrowInit(); testThrowPre();
    if (fails == 0) printf("OK: %d comprobaciones, 0 fallos\n", checks);
    else printf("FALLOS: %d de %d\n", fails, checks);
    return fails ? 1 : 0;
}

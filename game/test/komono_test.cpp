// Test de game/komono (clases Komono* del overlay aomod). Los valores de tablas salen de disc/TBL/AOMOD.BIN y del asm (comentados).
// Compilar y ejecutar desde la raiz del repo (ver game/komono/MANIFEST.md).
#pragma STDC FP_CONTRACT OFF
#include "../komono/komono.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
using namespace vn::komono;

static int checks = 0, fails = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; std::printf("FALLO linea %d: %s\n", __LINE__, #c); } } while (0)

// Host de prueba: anota cada llamada en `log` como texto.
struct Mock : KomonoHost {
    std::string log;
    int randSeq[16]; int nrand = 0, irand = 0;
    float animP[3] = {0, 0, 0}; int32_t animR[3] = {0, 0, 0};
    ItemSpawn lastItem; int itemCount = 0;
    void setRand(std::initializer_list<int> v) { nrand = 0; irand = 0; for (int x : v) randSeq[nrand++] = x; }
    void add(const char* s) { log += s; log += ";"; }
    void addf(const char* fmt, double a = 0, double b = 0, double c = 0) { char buf[160]; std::snprintf(buf, sizeof buf, fmt, a, b, c); add(buf); }
    int rand() override { return irand < nrand ? randSeq[irand++] : 0; }
    void sound(int id, int a2, int a3) override { char b[64]; std::snprintf(b, sizeof b, "snd%X/%X/%X", id, a2, a3); add(b); }
    void addScore(int p, int pts) override { char b[64]; std::snprintf(b, sizeof b, "score%d/%d", p, pts); add(b); }
    void addHitNum(int p) override { char b[64]; std::snprintf(b, sizeof b, "hit%d", p); add(b); }
    void closeTask() override { add("close"); }
    void spawnBreakTask(int id, const float p[3]) override { char b[96]; std::snprintf(b, sizeof b, "dish%X(%g,%g,%g)", id, p[0], p[1], p[2]); add(b); }
    void breakEffectA(const float p[3], int rotY, int piece) override { char b[96]; std::snprintf(b, sizeof b, "bfx(%g,%g,%g,r%d,p%d)", p[0], p[1], p[2], rotY, piece); add(b); }
    void effect(float sc, int code, const float p[3]) override { char b[96]; std::snprintf(b, sizeof b, "fx%g/%X(%g,%g,%g)", sc, code, p[0], p[1], p[2]); add(b); }
    void groundEffect(int code, float x, float z) override { char b[64]; std::snprintf(b, sizeof b, "gfx%d(%g,%g)", code, x, z); add(b); }
    void spawnItem(const ItemSpawn& s) override { lastItem = s; itemCount++; add("item"); }
    void counterInc() override { add("cnt"); }
    void registerCollider(const float p[3], float out[3]) override { char b[96]; std::snprintf(b, sizeof b, "col(%g,%g,%g)", p[0], p[1], p[2]); add(b); out[0] = p[0] + 100; out[1] = p[1] + 200; out[2] = p[2] + 300; }
    void registerColliderAlt() override { add("colalt"); }
    void goggleEffect(const float p[3]) override { char b[96]; std::snprintf(b, sizeof b, "gog(%g,%g,%g)", p[0], p[1], p[2]); add(b); }
    void orgHitCollect(int pl, const float p[3]) override { char b[96]; std::snprintf(b, sizeof b, "collect%d(%g,%g,%g)", pl, p[0], p[1], p[2]); add(b); }
    void orgHitAgain(int pl, const float e[6]) override { char b[96]; std::snprintf(b, sizeof b, "again%d(%g,%g,%g)", pl, e[0], e[1], e[2]); add(b); }
    void camTransform(const float in[3], float out[3]) override { out[0] = in[0] + 1000; out[1] = in[1] + 2000; out[2] = in[2] + 3000; }
    void missionMessage(int m) override { char b[32]; std::snprintf(b, sizeof b, "msg%d", m); add(b); }
    int dynaFind(int id) override { return id * 10; }
    float angA(int a) override { return (float)a / 65536.0f; }
    float angB(int a) override { return 1.0f - (float)a / 131072.0f; }
    void animSample(float t, int id, float p[3], int32_t r[3]) override { char b[64]; std::snprintf(b, sizeof b, "anim(%g,%X)", t, id); add(b); for (int k = 0; k < 3; k++) { p[k] = animP[k]; r[k] = animR[k]; } }
};
static bool has(const Mock& m, const char* s) { return m.log.find(s) != std::string::npos; }
static bool is(const Mock& m, const char* s) { return m.log == s; }

static std::vector<unsigned char> g_aomod;
static std::vector<unsigned char> slurp(const char* p) { std::ifstream f(p, std::ios::binary); return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); }
static uint32_t rd32(size_t off) { return g_aomod[off] | (g_aomod[off + 1] << 8) | (g_aomod[off + 2] << 16) | ((uint32_t)g_aomod[off + 3] << 24); }
static float rdf(size_t off) { uint32_t u = rd32(off); float f; std::memcpy(&f, &u, 4); return f; }
static int16_t rd16(size_t off) { return (int16_t)(g_aomod[off] | (g_aomod[off + 1] << 8)); }

// --- Tablas contra el binario del overlay (offset = direccion - 0x9DF000) ---
static void testTables() {
    if (g_aomod.size() < 0x2AA2C) { std::printf("AVISO: sin disc/TBL/AOMOD.BIN, se salta la comparacion de tablas\n"); return; }
    for (int k = 0; k < 45; k++) {
        size_t o = 0xA093F0 - 0x9DF000 + 16 * k;
        CHECK((int32_t)rd32(o) == kKomonoTypeTable[k].id);
        CHECK(rdf(o + 4) == kKomonoTypeTable[k].radius);
        CHECK(rdf(o + 8) == kKomonoTypeTable[k].yOffset);
        CHECK((int32_t)rd32(o + 12) == kKomonoTypeTable[k].collide);
    }
    CHECK((int32_t)rd32(0xA093F0 - 0x9DF000 + 16 * 45) == -1);        // terminador
    for (int k = 0; k < 43; k++) {
        size_t o = 0xA096D0 - 0x9DF000 + 0x14 * k;
        CHECK((int32_t)rd32(o) == kKomonoBreakTable[k].id);
        CHECK(rd16(o + 4) == kKomonoBreakTable[k].isItem);
        CHECK(rd16(o + 6) == kKomonoBreakTable[k].piece);
        CHECK(rd16(o + 8) == kKomonoBreakTable[k].effect);
        CHECK(rdf(o + 12) == kKomonoBreakTable[k].scale);
        CHECK((int32_t)rd32(o + 16) == kKomonoBreakTable[k].sound);
    }
    float g[3] = {rdf(0xA09DB4 - 0x9DF000), rdf(0xA09DB4 - 0x9DF000 + 4), rdf(0xA09DB4 - 0x9DF000 + 8)};   // D_00A09DB4 (gafas de la seta)
    CHECK(g[0] == 62.599998474121094f && g[1] == 3.700000047683716f && g[2] == -4121.2998046875f);
}

static void testBase() {
    // initEndCP: (id, limite) -> id en w[1+k], limite float en w[6+k], n en w[0]
    KomonoEndCP cp; int32_t pairs[] = {0x230, 100, 0x20A, 55};
    CKomonoBase::initEndCP(cp, pairs, 2);
    CHECK(cp.count() == 2 && cp.id(0) == 0x230 && cp.id(1) == 0x20A && cp.limit(0) == 100.0f && cp.limit(1) == 55.0f);
    KomonoEnv env; env.camPathId = 0x230; env.camFrame = 99.0f;
    CHECK(CKomonoBase::checkEndCP(cp, env) == 0);                    // frame < limite
    env.camFrame = 100.0f; CHECK(CKomonoBase::checkEndCP(cp, env) == 1);   // !(frame < limite)
    env.camPathId = 0x20A; env.camFrame = 54.0f; CHECK(CKomonoBase::checkEndCP(cp, env) == 0);
    env.camFrame = 56.0f; CHECK(CKomonoBase::checkEndCP(cp, env) == 1);
    env.camPathId = 0x999; CHECK(CKomonoBase::checkEndCP(cp, env) == 0);
    KomonoEndCP none; CKomonoBase::initEndCP(none, pairs, 0); CHECK(CKomonoBase::checkEndCP(none, env) == 0);
    // n > 5: los ids solapan los primeros limites (w[1+5] = w[6+0]); el id 5 gana (se escribe despues)
    int32_t big[12]; for (int k = 0; k < 6; k++) { big[2 * k] = 0x100 + k; big[2 * k + 1] = 7 + k; }
    KomonoEndCP bg; CKomonoBase::initEndCP(bg, big, 6);
    CHECK(bg.w[6] == 0x105u && bg.id(5) == 0x105);
    // bornItem: tabla sintetica
    Mock m; float pos[3] = {1, 2, 3};
    CKomonoBase::bornItem(cp, pos, 10, 20, 30, 0x16, m);
    CHECK(m.itemCount == 1 && m.lastItem.rot[0] == 10 && m.lastItem.rot[1] == 20 && m.lastItem.rot[2] == 30);
    CHECK(m.lastItem.pos[2] == 3.0f && m.lastItem.count == 2 && m.lastItem.cpId[1] == 0x20A && m.lastItem.cpLimit[0] == 100 && m.lastItem.itemKind == 0x16);
    // limite fraccionario se trunca (fptosi)
    KomonoEndCP fr; fr.w[0] = 1; fr.w[1] = 5; uint32_t fb; float ff = 99.9f; std::memcpy(&fb, &ff, 4); fr.w[6] = fb;
    CKomonoBase::bornItem(fr, pos, 0, 0, 0, 1, m); CHECK(m.lastItem.cpLimit[0] == 99);
}

static void testHit() {
    Mock m; uint32_t f;
    f = 8 | 2; CHECK(komonoHitPlayer(f, m, 7, true) == 0 && f == 8);          // solo bit 2 -> jugador 0, se limpia
    f = 8 | 4; CHECK(komonoHitPlayer(f, m, 7, true) == 1 && f == 8);          // solo bit 4 -> jugador 1
    f = 8 | 4; CHECK(komonoHitPlayer(f, m, 7, false) == 1 && f == 12);        // sin consumir
    f = 8; CHECK(komonoHitPlayer(f, m, 7, true) == 7 && f == 8);              // ninguno -> valor por defecto
    f = 2 | 4; m.setRand({3}); CHECK(komonoHitPlayer(f, m, 0, true) == 1 && f == 6);   // ambos: rand%2, no limpia
    m.setRand({4}); CHECK(komonoHitPlayer(f, m, 0, true) == 0);
    m.setRand({-3}); CHECK(komonoHitPlayer(f, m, 0, true) == -1);             // % con signo de C (como el asm: andi + ajuste -2)
    m.setRand({-4}); CHECK(komonoHitPlayer(f, m, 0, true) == 0);
}

static void testColli() {
    ColliState s; float tp[3] = {5, 6, 7};
    ColliLogic::init(s, tp);
    CHECK(s.flags == 0x10001 && s.radius == 8.0f && s.pos[1] == 6.0f && s.closeReq == 0);
    CHECK(ColliLogic::chkColli(s) == 0); s.flags |= 8; CHECK(ColliLogic::chkColli(s) == 1);
    Mock m; s.flags |= 2; CHECK(ColliLogic::getHitPl(s, m) == 0 && (s.flags & 2) == 0 && (s.flags & 8));   // getHitPl no toca el bit 8
    s.flags |= 4; CHECK(ColliLogic::getHitPl(s, m) == 1 && (s.flags & 4) == 0);
    ColliLogic::clrColli(s); CHECK((s.flags & 8) == 0);
    ColliLogic::setPos(s, 1, 2, 3); CHECK(s.pos[0] == 1 && s.pos[2] == 3);
    ColliLogic::setRadius(s, 2.5f); CHECK(s.radius == 2.5f);
    ColliLogic::ctrlPre(s, m); CHECK(is(m, "col(1,2,3);"));
    m.log.clear(); ColliLogic::close(s); CHECK(s.closeReq == 1);
    ColliLogic::ctrlPre(s, m); CHECK(is(m, "close;col(1,2,3);"));
}

static void testKomono() {
    // altType (tabla de init)
    CHECK(KomonoLogic::altType(0x4700F6) == 0x4700F7 && KomonoLogic::altType(0x4700D6) == 0x4700D7 && KomonoLogic::altType(0x410051) == 0x410052);
    CHECK(KomonoLogic::altType(0x4700B3) == 0x4700B4 && KomonoLogic::altType(0x40003A) == 0x40003B && KomonoLogic::altType(0x430013) == 0x430014);
    CHECK(KomonoLogic::altType(0x3C003C) == 0x3C003D && KomonoLogic::altType(0x3C001C) == 0x3C001D && KomonoLogic::altType(0x123456) == 0);
    KomonoState s; float tp[3] = {10, 20, 30}; uint16_t tr[3] = {1, 2, 0xFFFF}; int32_t cps[] = {0x230, 60};
    KomonoLogic::init(s, tp, tr, 0x3C001C, cps, 1, 0x16);
    CHECK(s.radius == 5.0f && s.yOffset == 4.0f && s.collide == 1 && s.typeAlt == 0x3C001D && s.itemKind == 0x16 && s.flags == 0x10001);
    CHECK(s.rot[2] == 0xFFFF && s.pos[2] == 30.0f && s.endCp.count() == 1);
    KomonoState u; KomonoLogic::init(u, tp, tr, 0x123456, cps, 0, 0);                 // id desconocido: valores por defecto, sin colision
    CHECK(u.radius == 8.0f && u.yOffset == 4.0f && u.collide == 0 && u.typeAlt == 0);
    KomonoState w; KomonoLogic::init(w, tp, tr, 0x3E01EA, cps, 0, 0);                 // fila con colision 0 y yOffset 5
    CHECK(w.radius == 5.0f && w.yOffset == 5.0f && w.collide == 0);
    KomonoState z; KomonoLogic::init(z, tp, tr, 0x430013, cps, 0, 0);
    CHECK(z.radius == 13.0f && z.yOffset == 14.0f && z.collide == 1);
    // ctrl: cierra al llegar a la camara de fin; gafas si objeto >= 0x15 en entrenamiento arcade
    Mock m; KomonoEnv env; env.camPathId = 0x230; env.camFrame = 61.0f;
    KomonoLogic::ctrl(s, env, m); CHECK(is(m, "close;"));
    m.log.clear(); env.camPathId = 1; env.gameState = 0xA; env.gameKind = 1; KomonoLogic::ctrl(s, env, m); CHECK(is(m, "gog(10,24,30);"));
    m.log.clear(); s.itemKind = 0x14; KomonoLogic::ctrl(s, env, m); CHECK(m.log.empty());
    s.itemKind = 0x15; env.gameKind = 2; KomonoLogic::ctrl(s, env, m); CHECK(m.log.empty());
    // ctrlPost: golpe jugador 1 -> 10 puntos, rompe y registra colision
    KomonoState p; KomonoLogic::init(p, tp, tr, 0x3C006F, cps, 0, 0); p.flags |= 8 | 4;
    m.log.clear(); KomonoLogic::ctrlPost(p, m);
    CHECK(has(m, "score1/10;hit1;"));
    CHECK((p.flags & 12) == 0);
    CHECK(has(m, "col(10,24,30);"));                                  // y + 4.0 (fila 0x3C006F: yOffset 4)
    // sin golpe y sin colision: nada
    m.log.clear(); KomonoState q; KomonoLogic::init(q, tp, tr, 0x3E01EA, cps, 0, 0); KomonoLogic::ctrlPost(q, m); CHECK(m.log.empty());
    // close con fila "item" (isItem != 0): 0x3C001C -> fila 40 {1, 10, 10, 0, 0.5, 851983 = 0xD000F}
    CHECK(kKomonoBreakTable[40].id == 0x3C001C && kKomonoBreakTable[40].piece == 10 && kKomonoBreakTable[40].effect == 10 && kKomonoBreakTable[40].scale == 0.5f && kKomonoBreakTable[40].sound == 0xD000F);
    KomonoState c; KomonoLogic::init(c, tp, tr, 0x3C001C, cps, 0, 0);
    m.log.clear(); KomonoLogic::close(c, m);
    CHECK(is(m, "bfx(10,20,30,r2,p10);fx0.5/A(10,20,30);sndD000F/0/0;gfx3(10,30);cnt;close;"));
    // close con fila "no item" (isItem == 0): 0x3C0000 -> fila 38 {1,8,0,...}? isItem = 1; usamos 0x410053 (fila 23: isItem 0, efecto 18, escala 2, sonido 1507859)
    CHECK(kKomonoBreakTable[23].id == 0x410053 && kKomonoBreakTable[23].isItem == 0);
    KomonoState d; KomonoLogic::init(d, tp, tr, 0x410053, cps, 0, 0);
    m.log.clear(); KomonoLogic::close(d, m);
    CHECK(is(m, "dish410053(10,20,30);fx2/12(10,20,30);snd170213/0/0;cnt;close;"));
    // tipo sin fila (p. ej. 0x123456) con objeto: item + contador + cierre
    KomonoState e; KomonoLogic::init(e, tp, tr, 0x123456, cps, 1, 0x17);
    m.log.clear(); m.itemCount = 0; KomonoLogic::close(e, m); CHECK(is(m, "item;cnt;close;") && m.lastItem.itemKind == 0x17);
    // efectos por tipo
    KomonoState f; KomonoLogic::init(f, tp, tr, 0x4700F6, cps, 0, 0);
    m.log.clear(); KomonoLogic::close(f, m); CHECK(has(m, "fx3/8(10,20,30);cnt;close;"));
    KomonoState g; KomonoLogic::init(g, tp, tr, 0x40003A, cps, 0, 0);
    m.log.clear(); KomonoLogic::close(g, m); CHECK(has(m, "fx2/6(10,20,30);cnt;close;"));
    KomonoState hh; KomonoLogic::init(hh, tp, tr, 0x3C004F, cps, 0, 0);
    m.log.clear(); KomonoLogic::close(hh, m); CHECK(has(m, "gfx5(10,30);cnt;close;"));
}

static void testKomono3Helpers() {
    Mock m; KomonoEnv env; Komono3State s;
    Komono3Logic::setBranchNo(env); CHECK(env.branchFlag == 1 && env.branchNo == 1);
    Komono3Logic::clrBranchNo(env); CHECK(env.branchFlag == 0 && env.branchNo == 0);
    env.camPathId = 0x1D0; env.camFrame = 10.0f;
    CHECK(Komono3Logic::checkCamPath(env, 0x1D0, 10) && !Komono3Logic::checkCamPath(env, 0x1D0, 11) && !Komono3Logic::checkCamPath(env, 0x1D1, 0));
    // checkHit
    s.flags = 8 | 4; int pl = -1; CHECK(Komono3Logic::checkHit(s, m, &pl) == 1 && pl == 1 && s.flags == 0);
    s.flags = 4; CHECK(Komono3Logic::checkHit(s, m, &pl) == 0 && s.flags == 4);
    // calcEffPos
    s.colliPos[0] = 1; s.colliPos[1] = 2; s.colliPos[2] = 3; env.camVec[0] = 0x3F800000; env.camVec[1] = 0; env.camVec[2] = 0x40000000;
    float eff[6]; Komono3Logic::calcEffPos(s, env, m, eff);
    CHECK(eff[0] == 1001 && eff[1] == 2003 && eff[2] == 3005 && eff[3] == 1.0f && eff[5] == 2.0f);   // y + 1, z + 2 antes de transformar
    // checkGameMode: gameKind != 1 cierra; estado 0 sin mision valida limpia la rama y cierra
    env = KomonoEnv(); env.gameKind = 2; s = Komono3State(); s.mission = 3; env.branchNo = 1;
    Komono3Logic::checkGameMode(s, env, m); CHECK(is(m, "close;close;") && env.branchNo == 0);
    m.log.clear(); env = KomonoEnv(); env.gameState = 0xA; env.gameKind = 1; env.missionFlags[3] = 0x03;
    Komono3Logic::checkGameMode(s, env, m); CHECK(m.log.empty());                       // abierta + activa + no completada: sigue
    env.missionFlags[3] = 0x23; Komono3Logic::checkGameMode(s, env, m); CHECK(is(m, "close;"));   // completada: se retira
    m.log.clear(); env.missionFlags[3] = 0x01; Komono3Logic::checkGameMode(s, env, m); CHECK(is(m, "close;"));   // no activa
    m.log.clear(); env.missionFlags[3] = 0x03; env.gameState = 5; Komono3Logic::checkGameMode(s, env, m); CHECK(is(m, "close;"));   // no arcade
    m.log.clear(); env.gameState = 0xA; s.state = 1; env.missionFlags[3] = 0; Komono3Logic::checkGameMode(s, env, m); CHECK(m.log.empty());   // ya recogido: no comprueba
    // checkMissionEnd_setTrnsl
    env = KomonoEnv(); s = Komono3State(); s.endState = 1; env.msgPending = 1; Komono3Logic::checkMissionEnd_setTrnsl(s, env); CHECK(s.endState == 1);
    env.msgPending = 0; Komono3Logic::checkMissionEnd_setTrnsl(s, env); CHECK(s.endState == 2);
    Komono3Logic::checkMissionEnd_setTrnsl(s, env); CHECK(s.alpha == 1.0f - 0.033333335f);
    env.pauseLevel = 1; float a0 = s.alpha; Komono3Logic::checkMissionEnd_setTrnsl(s, env); CHECK(s.alpha == a0);
    env.pauseLevel = 0; s.alpha = 0.01f; Komono3Logic::checkMissionEnd_setTrnsl(s, env); CHECK(s.alpha == 0.0f);
    // aviso de mision cumplida
    env = KomonoEnv(); s = Komono3State(); s.msgPath = 0x1D0; s.msgFrame = 20; s.mission = 5; env.camPathId = 0x1D0; env.camFrame = 19.0f;
    m.log.clear(); Komono3Logic::checkMissionCompleteMessageDispTiming(s, env, m); CHECK(m.log.empty() && s.phase == 0);
    env.camFrame = 20.0f; Komono3Logic::checkMissionCompleteMessageDispTiming(s, env, m); CHECK(is(m, "msg5;") && s.phase == 1 && s.endState == 1 && s.state == 2);
    m.log.clear(); Komono3Logic::checkMissionCompleteMessageDispTiming(s, env, m); CHECK(m.log.empty());        // solo una vez
    // cierre por camara
    s.closePath = 0x300; s.closeFrame = 5; env.branchNo = 1; env.camPathId = 0x300; env.camFrame = 5;
    Komono3Logic::checkCloseTiming_clrBranchNo(s, env, m); CHECK(is(m, "close;") && env.branchNo == 0);
    // luz
    s.lightAngle = 0; Komono3Logic::lightCtrl(s, env); CHECK(s.lightAngle == 0x200); env.pauseLevel = 2; Komono3Logic::lightCtrl(s, env); CHECK(s.lightAngle == 0x200);
}

static Komono3State mk() { Komono3State s; s.mission = 0; s.closePath = 0x999; s.msgPath = 0x998; return s; }   // sin camaras alcanzables

static void testColliBranches() {
    Mock m; KomonoEnv env; Komono3State s; env.gameState = 0xA; env.gameKind = 1; env.missionFlags[0] = 3;
    // checkColli_setBranchNo estado 0 -> recoger (jugador 0)
    s.pos[0] = 1; s.pos[1] = 2; s.pos[2] = 3; s.flags = 8 | 2;
    Komono3Logic::checkColli_setBranchNo(s, env, m);
    CHECK(is(m, "col(1,2,3);collect0(1,2,3);snd" "B012B/0/0;") && s.state == 1 && env.branchNo == 1 && env.branchFlag == 1);
    // estado 1 -> golpe repetido: efecto + sonido 0x20025 con a2 = 0x1000
    m.log.clear(); s.flags = 8 | 4; Komono3Logic::checkColli_setBranchNo(s, env, m);
    CHECK(is(m, "col(1,2,3);again1(1101,2203,3305);snd20025/1000/0;"));        // colli (101,202,303) -> +1,+2 -> transform +1000.. 
    // sin golpe: solo registra
    m.log.clear(); Komono3Logic::checkColli_setBranchNo(s, env, m); CHECK(is(m, "col(1,2,3);"));
    // silverGrass: al final del recorrido (camara 0x228 frame >= 550) con estado 1 no registra colisionador ni golpes
    Komono3State g = mk(); g.state = 1; env.camPathId = 0x228; env.camFrame = 550.0f; g.flags = 8;
    m.log.clear(); Komono3Logic::silverGrassCtrl(g, env, m); CHECK(!has(m, "col(") && !has(m, "again") && (g.flags & 8));
    env.camFrame = 549.0f; Komono3Logic::silverGrassCtrl(g, env, m); CHECK(has(m, "col(") && has(m, "again0(") && has(m, "snd20025/1000/0"));
    // silverGrass estado 0
    Komono3State g0 = mk(); g0.flags = 8 | 2; env.camPathId = 0; m.log.clear(); Komono3Logic::silverGrassCtrl(g0, env, m);
    CHECK(has(m, "collect0(") && g0.state == 1 && g0.lightAngle == 0x200 && env.branchNo == 1);
}

static void testGolden() {
    Mock m; KomonoEnv env; env.gameState = 0xA; env.gameKind = 1; env.missionFlags[0] = 3;
    // goldenBorn: al golpe, recoge + 3 rafagas (6 efectos) con los desplazamientos del asm + 2 sonidos
    Komono3State b = mk(); b.flags = 8 | 2; b.pos[0] = 4; b.pos[1] = 5; b.pos[2] = 6;
    Komono3Logic::goldenBornCtrl(b, env, m);
    // colli = pos + (100,200,300); in = (104, 1+205, 2+306) -> transform +(1000,2000,3000) = (1104, 2206, 3308)
    CHECK(has(m, "collect0(4,5,6);fx1/5(1104,2206,3308);fx0.8/12(1104,2206,3308);fx1/5(1106,2205.5,3308);fx0.8/12(1106,2205.5,3308);fx1/5(1103,2206.5,3308);fx0.8/12(1103,2206.5,3308);"));
    CHECK(has(m, "sndB012B/0/0;sndF020C/0/0;") && b.state == 1 && env.branchNo == 1 && b.lightAngle == 0x200);
    // estado 0 sin golpe: gafas
    Komono3State b2 = mk(); m.log.clear(); Komono3Logic::goldenBornCtrl(b2, env, m); CHECK(has(m, "gog(0,0,0);") && b2.lightAngle == 0);
    // goldenRat: posicion = muestra + (-10.3,-149,-1919); angulo Y + 0x8000; temporizador
    Komono3State r = mk(); env.camPathId = 0; m.animP[0] = 1; m.animP[1] = 2; m.animP[2] = 3; m.animR[0] = 7; m.animR[1] = 0x100; m.animR[2] = 9;
    m.log.clear(); r.animTime = 59.0f; Komono3Logic::goldenRatCtrl(r, env, m);
    CHECK(r.animPos[0] == -10.3f + 1 && r.animPos[1] == -149.0f + 2 && r.animPos[2] == -1919.0f + 3 && r.animRot[1] == 0x8100 && r.animRot[0] == 7 && r.pos[0] == r.animPos[0]);
    CHECK(has(m, "anim(59,216);") && has(m, "snd190110/0/0;") && r.animTime == 60.0f && r.animFrame == 1);   // (int)60 % 90 == 0x3C
    m.log.clear(); Komono3Logic::goldenRatCtrl(r, env, m); CHECK(!has(m, "snd190110") && r.animTime == 61.0f);
    r.animTime = 340.0f; Komono3Logic::goldenRatCtrl(r, env, m); CHECK(r.animTime == 0.0f);       // 341 > 340 -> 0
    r.animTime = 5; r.animFrame = 14; r.state = 1; Komono3Logic::goldenRatCtrl(r, env, m); CHECK(r.animFrame == 0 && r.animState == 1);  // modulo 15; recogido -> anima
    env.camPathId = 0x23F; env.camFrame = 65.0f; r.animTime = 5; r.animState = 1; Komono3Logic::goldenRatCtrl(r, env, m); CHECK(r.animState == 2 && r.animTime == 0 && r.animFrame == 0);
    env.camPathId = 0;
    // pausa: congela temporizador
    Komono3State r2 = mk(); env.pauseLevel = 1; r2.animTime = 3; Komono3Logic::goldenRatCtrl(r2, env, m); CHECK(r2.animTime == 3.0f); env.pauseLevel = 0;
    // goldenFish
    Komono3State f = mk(); f.basePos[0] = 10; f.basePos[1] = 20; f.basePos[2] = 30; f.baseRot[1] = 5; m.animP[0] = 1; m.animP[1] = 2; m.animP[2] = 3; m.animR[1] = 8;
    f.animTime = 71.0f; m.log.clear(); Komono3Logic::goldenFishCtrl(f, env, m);
    CHECK(has(m, "anim(71,280);") && has(m, "snd1C010C/0/0;") && f.animPos[0] == 11 && f.animPos[1] == 22 && f.animRot[1] == 13 && f.animFrame == 1);
    f.animTime = 212.0f; m.log.clear(); Komono3Logic::goldenFishCtrl(f, env, m); CHECK(has(m, "snd1C010D/0/0;"));
    f.animTime = 260.0f; m.log.clear(); Komono3Logic::goldenFishCtrl(f, env, m); CHECK(has(m, "snd1C010C/0/0;"));
    f.animTime = 402.0f; m.log.clear(); Komono3Logic::goldenFishCtrl(f, env, m); CHECK(has(m, "snd1C010D/0/0;"));
    f.animTime = 420.0f; Komono3Logic::goldenFishCtrl(f, env, m); CHECK(f.animTime == 0.0f);
    f.animFrame = 36; Komono3Logic::goldenFishCtrl(f, env, m); CHECK(f.animFrame == 0);              // modulo 37
    f.state = 1; m.animP[1] = -50.0f; f.basePos[1] = 0; f.animTime = 7; Komono3Logic::goldenFishCtrl(f, env, m); CHECK(f.animState == 1 && f.animTime == 0.0f);   // cae por debajo de -12.7
    // silverKinoko: desvanecimiento y cierre
    Komono3State k = mk(); k.endState = 2; k.alpha = 0.02f; env.branchNo = 1; m.log.clear(); env.pauseLevel = 0;
    Komono3Logic::silverKinokoCtrl(k, env, m); CHECK(k.alpha == 0.0f && has(m, "close;") && env.branchNo == 0 && k.pulse == 0x200);
    Komono3State k2 = mk(); k2.pulse = 0xFE00; Komono3Logic::silverKinokoCtrl(k2, env, m); CHECK(k2.pulse == 0);   // & 0xFFFF
    Komono3State k3 = mk(); k3.endState = 1; env.msgPending = 0; Komono3Logic::silverKinokoCtrl(k3, env, m); CHECK(k3.endState == 2);
}

static void testOthers() {
    Mock m; KomonoEnv env; env.gameState = 0xA; env.gameKind = 1; env.missionFlags[0] = 3;
    // uroko A: recoger cuenta; B: a la quinta activa la rama
    Komono3State a = mk(); a.phase = 0x999; a.flags = 8 | 2; Komono3Logic::urokoCtrlA(a, env, m);
    CHECK(env.urokoCount == 1 && a.state == 1 && has(m, "collect0(") && has(m, "sndB012B/0/0;"));
    for (int i = 0; i < 40; i++) Komono3Logic::urokoCtrlA(a, env, m);
    CHECK(a.alpha == 0.0f && has(m, "close;"));
    Komono3State b = mk(); env.urokoCount = 3; b.flags = 8 | 2; Komono3Logic::urokoCtrlB(b, env, m); CHECK(env.urokoCount == 4 && env.branchNo == 0);
    Komono3State b2 = mk(); b2.flags = 8 | 2; Komono3Logic::urokoCtrlB(b2, env, m); CHECK(env.urokoCount == 5 && env.branchNo == 1 && b2.state == 1);
    env.urokoCount = 0;
    // uroko A: cierra en su camara
    Komono3State a2; a2.mission = 0; a2.phase = 0x240; a2.msgPath = 9; env.camPathId = 0x240; env.camFrame = 9; m.log.clear(); Komono3Logic::urokoCtrlA(a2, env, m); CHECK(has(m, "close;")); env.camPathId = 0;
    // makimono
    Komono3State mm = mk(); mm.state = 1; mm.animTime = 119.0f; m.log.clear(); Komono3Logic::makimonoCtrl(mm, env, m);
    CHECK(mm.animTime == 120.0f && mm.animState == 0); Komono3Logic::makimonoCtrl(mm, env, m); CHECK(mm.animTime == 120.0f && mm.animState == 1);
    // hitotsu: colisionador a y + 17.5; sonidos por ventanas de tiempo
    Komono3State h = mk(); h.pos[1] = 2; env.camPathId = 0x28D; env.camFrame = 0; m.log.clear(); Komono3Logic::hitotsuCtrl(h, env, m);
    CHECK(has(m, "col(0,19.5,0);") && has(m, "gog(0,19.5,0);") && h.sub204 == 1);
    env.camPathId = 0; h.animTime = 0.0f; m.log.clear(); Komono3Logic::hitotsuCtrl(h, env, m); CHECK(has(m, "sndB012B") == false && has(m, "snd1C010E/0/0;") && h.animTime == 1.2f);  // 1.2 en [1, 2.2)
    h.animTime = 1.2f; m.log.clear(); Komono3Logic::hitotsuCtrl(h, env, m); CHECK(!has(m, "snd1C010E"));  // 2.4: fuera
    h.animTime = 559.0f; m.log.clear(); Komono3Logic::hitotsuCtrl(h, env, m); CHECK(has(m, "snd1C010E"));  // 560.2
    h.animTime = 258.9f; m.log.clear(); Komono3Logic::hitotsuCtrl(h, env, m); CHECK(has(m, "snd1C010F/0/0;"));  // 260.1
    h.animTime = 459.0f; m.log.clear(); Komono3Logic::hitotsuCtrl(h, env, m); CHECK(has(m, "snd1C010F/0/0;"));  // 460.2
    h.animTime = 744.9f; Komono3Logic::hitotsuCtrl(h, env, m); CHECK(h.animTime == 745.0f);
    // kinoko: gafas en posicion fija
    Komono3State k = mk(); m.log.clear(); Komono3Logic::kinokoCtrl(k, env, m); CHECK(has(m, "gog(62.6,3.7,-4121.3);"));
    // nuts: recoge en la posicion global; sonidos a 56..57.2 y 61..62.2; fin a 90
    Komono3State n = mk(); env.nutsPos[0] = 1; env.nutsPos[1] = 2; env.nutsPos[2] = 3; n.flags = 8 | 4; m.log.clear();
    Komono3Logic::nutsCtrl(n, env, m);
    CHECK(has(m, "colalt;collect1(1,2,3);sndC000E/0/0;sndB012B/0/0;") && n.state == 1 && env.branchNo == 1 && has(m, "anim(0,164);anim(0,165);"));
    n.animTime = 55.0f; m.log.clear(); Komono3Logic::nutsCtrl(n, env, m); CHECK(has(m, "sndC000F/0/0;"));
    n.animTime = 60.0f; m.log.clear(); Komono3Logic::nutsCtrl(n, env, m); CHECK(has(m, "sndC000F/0/0;"));
    n.animTime = 89.5f; Komono3Logic::nutsCtrl(n, env, m); CHECK(n.animTime == 90.0f && n.animState == 1);
    // broche = orgCtrl
    Komono3State br = mk(); br.flags = 8; m.log.clear(); Komono3Logic::broochCtrl(br, env, m); CHECK(has(m, "collect0(") && br.state == 1 && br.lightAngle == 0x200);
    Komono3State oc = mk(); oc.flags = 8; m.log.clear(); Komono3Logic::orgCtrl(oc, env, m); CHECK(has(m, "collect0(") && oc.state == 1);
}

static void testStageObjs() {
    Mock m; KomonoEnv env;
    env.camPathId = 0x1A8; StageObjLogic::nakaDoorCtrl(env, m); CHECK(is(m, "close;"));
    m.log.clear(); env.camPathId = 0x1AF; StageObjLogic::nakaDoorCtrl(env, m); CHECK(is(m, "close;"));
    m.log.clear(); env.camPathId = 0x187; StageObjLogic::nakaDoorCtrl(env, m); CHECK(is(m, "close;"));
    m.log.clear(); env.camPathId = 0x188; StageObjLogic::nakaDoorCtrl(env, m); CHECK(m.log.empty());
    NakaDoorState d; d.pos[0] = 1; d.pos[1] = 2; d.pos[2] = 3; d.off[0] = 10; d.off[1] = 20; d.off[2] = 30; d.lenA = 10; d.lenB = 20; d.flags = 8;
    env.camPathId = 0; StageObjLogic::nakaDoorCtrlPost(d, env, m);
    CHECK(has(m, "sndD0012/0/0;") && d.phase == 1 && d.sub == 1 && has(m, "col(11,22,33);") && (d.flags & 8) == 0 && d.tA == 0.2f);
    env.camPathId = 0x1D0; StageObjLogic::nakaDoorCtrlPost(d, env, m); CHECK(d.sub == 2 && env.branchNo == 1);
    d.tB = 13.9f; m.log.clear(); StageObjLogic::nakaDoorCtrlPost(d, env, m); CHECK(has(m, "sndD0016/0/0;") && d.soundDone == 1);       // 14.1 > 20 - 6
    d.tB = 19.9f; StageObjLogic::nakaDoorCtrlPost(d, env, m); CHECK(d.tB == 20.0f);
    d.tB = 13.0f; d.soundDone = 0; m.log.clear(); StageObjLogic::nakaDoorCtrlPost(d, env, m); CHECK(!has(m, "snd"));
    NakaDoorState d2; d2.lenA = 1; d2.tA = 0.9f; env.camPathId = 0; StageObjLogic::nakaDoorCtrlPost(d2, env, m); CHECK(d2.tA == 0.0f);       // 1.1 > 1 -> 0
    // Knight bunki
    ColliState c0, c1; KnightState k; k.colli[0] = &c0; k.colli[1] = &c1; k.pos[0][0] = 1; k.pos[1][0] = 2; k.rot[0][1] = 5; k.rot[1][1] = 6;
    m.log.clear(); env.camPathId = 0x230; StageObjLogic::knightCtrlPre(k, env, m); CHECK(is(m, "close;") && c0.closeReq == 1 && c1.closeReq == 1);
    env.camPathId = 0x20A; m.log.clear(); StageObjLogic::knightCtrlPre(k, env, m); CHECK(is(m, "close;"));
    env.camPathId = 0x1; m.log.clear(); c0.closeReq = 0; StageObjLogic::knightCtrlPre(k, env, m); CHECK(m.log.empty() && c0.closeReq == 0);
    c0.flags = 8 | 2; m.log.clear(); StageObjLogic::knightCtrl(k, env, m);
    CHECK(is(m, "score0/10;hit0;snd17021C/0/0;bfx(1,0,0,r5,p17);") && k.colli[0] == nullptr && k.phase == 1 && k.first == 1 && c0.closeReq == 1 && (c0.flags & 8) == 0);
    CHECK(c1.pos[0] == 2.0f);                                                                  // setPos del segundo
    c1.flags = 8 | 4; m.log.clear(); StageObjLogic::knightCtrl(k, env, m); CHECK(has(m, "score1/10;hit1;") && k.first == 1 && k.colli[1] == nullptr);
    env.camPathId = 0x208; StageObjLogic::knightCtrl(k, env, m); CHECK(k.sub == 1 && env.branchNo == 1);
    ColliState c2, c3; KnightState k2; k2.colli[0] = &c2; k2.colli[1] = &c3; c3.flags = 8 | 2; env.camPathId = 0; StageObjLogic::knightCtrl(k2, env, m); CHECK(k2.first == 0 && k2.phase == 1);   // el primero en romperse fue el 1
    k2.sub = 0; env.camPathId = 0x208; StageObjLogic::knightCtrl(k2, env, m); CHECK(env.branchNo == 0);
}

static void testDyna() {
    Mock m; KomonoEnv env; static const int16_t tbl[4] = {5, 6, -7, 8}; env.dynaFrames = tbl; DynaSlots d;
    DynaLogic::setDynaParam(d, 2, 2, env, m); CHECK(d.handle[2] == 20 && d.frames[2] == -7);      // lh con signo
    DynaLogic::setDynaParam(d, 0, 3, env, m); CHECK(d.handle[0] == 30 && d.frames[0] == 8);
    env.camPathId = 0x77; env.camFrame = 10.0f;
    DynaLogic::checkCloseCamPath(env, 0x77, 10.5f, m); CHECK(m.log.empty());
    DynaLogic::checkCloseCamPath(env, 0x77, 10.0f, m); CHECK(is(m, "close;"));
    m.log.clear(); DynaLogic::checkCloseCamPath(env, 0x78, 0.0f, m); CHECK(m.log.empty());
    Komono3State s = mk(); env.gameState = 0xA; env.gameKind = 1; env.missionFlags[0] = 3; s.flags = 8; Komono3Logic::maboroshiCtrl(s, env, m); CHECK(has(m, "collect0(") && s.state == 1);
}

static void testTumiage() {
    Mock m; KomonoEnv env; TumiageBox bx[2]; ColliState c; TumiageState s; s.count = 2; s.boxes = bx; s.itemKind = 0x15;
    for (int q = 0; q < 8; q++) { bx[0].p[q][0] = (float)q; bx[0].p[q][1] = 10.0f + q * 2; bx[0].p[q][2] = -1.0f; }
    bx[0].active = 1; bx[0].colli = &c; bx[1].active = 0; bx[1].cooldown = 3; bx[1].p[0][0] = 99;
    TumiageLogic::ctrl(s, env, m);
    CHECK(bx[0].center4[0] == 5.5f && bx[0].center4[1] == 21.0f && bx[0].center4[2] == -1.0f);       // (4+5+6+7)/4, (18+20+22+24)/4
    CHECK(bx[0].center8[0] == 3.5f && bx[0].center8[1] == 17.0f && bx[0].center8[2] == -1.0f);       // 28/8, 136/8
    CHECK(c.pos[0] == 3.5f && c.pos[1] == 17.0f && bx[1].center8[0] == 0.0f && m.log.empty());       // la caja inactiva no se toca; sin arcade no hay gafas
    env.gameState = 0xA; env.gameKind = 1; TumiageLogic::ctrl(s, env, m); CHECK(is(m, "gog(5.5,25,-1);"));
    m.log.clear(); s.itemKind = 0x14; TumiageLogic::ctrl(s, env, m); CHECK(m.log.empty());
    bx[0].cooldown = 2; TumiageLogic::ctrlPost(s); CHECK(bx[0].cooldown == 1 && bx[1].cooldown == 3);   // solo las activas
    TumiageLogic::ctrlPost(s); TumiageLogic::ctrlPost(s); CHECK(bx[0].cooldown == 0);
}

static void testDebris() {
    Mock m; KomonoEnv env;
    // trozo en el aire: gravedad, mover, girar
    Debris d; d.pos[1] = 10; d.vel[0] = 1; d.vel[1] = 2; d.vel[2] = -1; d.grav = -0.5f; d.rot[0] = 0xFFFF; d.rotVel[0] = 2;
    DebrisLogic::stepKoware(d, 0.0f);
    CHECK(d.vel[1] == 1.5f && d.pos[0] == 1 && d.pos[1] == 11.5f && d.pos[2] == -1 && d.rot[0] == 1);
    // rebote: y < suelo
    Debris g; g.pos[1] = -1; g.vel[0] = 10; g.vel[1] = -5; g.vel[2] = 5; g.grav = -1; g.rotVel[0] = 4; g.rotVel[1] = 5; g.rotVel[2] = 6;
    DebrisLogic::stepKoware(g, 0.0f);
    float vy = -5.0f - (0.9f * -5.0f);                      // -0.5
    CHECK(g.vel[0] == 0.6f * 10.0f && g.vel[2] == 0.6f * 5.0f && g.vel[1] == vy + -1.0f && g.pos[1] == 0.0f + g.vel[1] && g.rotVel[2] == 0 && g.rot[1] == 0);
    // Kusari: gira antes de mover, amortigua 0.5 y solo anula rotVel X/Y
    Debris k; k.pos[1] = -2; k.vel[0] = 4; k.vel[1] = -6; k.vel[2] = 8; k.grav = -0.1f; k.rotVel[0] = 3; k.rotVel[1] = 4; k.rotVel[2] = 9; k.rot[2] = 5;
    DebrisLogic::stepKusari(k, 1.0f);
    CHECK(k.vel[0] == 2 && k.vel[2] == 4 && k.vel[1] == (-6.0f - 0.5f * -6.0f) + -0.1f && k.rotVel[0] == 0 && k.rotVel[1] == 0 && k.rotVel[2] == 9 && k.rot[2] == 5 && k.rot[0] == 0);
    // Dish: tipo normal -> alpha baja desde 21, cierra en 41; tipo 0x1B0079 -> 41 / 61
    Debris pcs[2]; DishState ds; ds.count = 2; ds.pieces = pcs; ds.typeId = 0x3C0001;
    for (int i = 0; i < 20; i++) DebrisTasks::dishCtrl(ds, env, m);
    CHECK(ds.frame == 20 && ds.alpha == 1.0f && m.log.empty());
    DebrisTasks::dishCtrl(ds, env, m); CHECK(ds.frame == 21 && ds.alpha == 1.0f - 0.05f);
    for (int i = 0; i < 19; i++) DebrisTasks::dishCtrl(ds, env, m);
    CHECK(ds.frame == 40 && !has(m, "close")); DebrisTasks::dishCtrl(ds, env, m); CHECK(has(m, "close;"));
    DishState d2; d2.count = 0; d2.typeId = 0x1B0079; m.log.clear();
    for (int i = 0; i < 40; i++) DebrisTasks::dishCtrl(d2, env, m);
    CHECK(d2.alpha == 1.0f && d2.frame == 40); DebrisTasks::dishCtrl(d2, env, m); CHECK(d2.alpha == 1.0f - 0.05f && m.log.empty());
    for (int i = 0; i < 19; i++) DebrisTasks::dishCtrl(d2, env, m); CHECK(d2.frame == 60 && m.log.empty()); DebrisTasks::dishCtrl(d2, env, m); CHECK(has(m, "close;"));
    env.pauseLevel = 1; int fr = d2.frame; DebrisTasks::dishCtrl(d2, env, m); CHECK(d2.frame == fr); env.pauseLevel = 0;   // pausa: congela
    // Box: suelo en floorY; 31 / 51
    Debris bp[1]; BoxState bs; bs.count = 1; bs.pieces = bp; bs.floorY = 3.0f; m.log.clear();
    DebrisTasks::boxCtrl(bs, env, m); CHECK(bp[0].pos[1] == 3.0f);                              // empieza en 0 < 3 -> rebota al suelo
    for (int i = 0; i < 29; i++) DebrisTasks::boxCtrl(bs, env, m); CHECK(bs.frame == 30 && bs.alpha == 1.0f);
    DebrisTasks::boxCtrl(bs, env, m); CHECK(bs.frame == 31 && bs.alpha == 1.0f - 0.05f);
    for (int i = 0; i < 19; i++) DebrisTasks::boxCtrl(bs, env, m); CHECK(bs.frame == 50 && m.log.empty()); DebrisTasks::boxCtrl(bs, env, m); CHECK(has(m, "close;"));
    // Croset: suelo por trozo; alpha desde 61; cierra al bajar de 0
    Debris cp2[1]; cp2[0].floorY = 7.0f; CrosetState cs; cs.count = 1; cs.pieces = cp2; m.log.clear();
    DebrisTasks::crosetCtrl(cs, env, m); CHECK(cp2[0].pos[1] == 7.0f);
    cs.frame = 60; DebrisTasks::crosetCtrl(cs, env, m); CHECK(cs.frame == 61 && cs.alpha == 1.0f - 0.05f && m.log.empty());
    cs.alpha = 0.04f; DebrisTasks::crosetCtrl(cs, env, m); CHECK(has(m, "close;"));
    // Kusari ciclo de vida: alpha -1/30 desde 21, cierra en 51
    KusariState ks; ks.floorY = 0; m.log.clear();
    for (int i = 0; i < 20; i++) DebrisTasks::kusariLoop(ks, env, m);
    CHECK(ks.frame == 20 && ks.alpha == 1.0f); DebrisTasks::kusariLoop(ks, env, m); CHECK(ks.frame == 21 && ks.alpha == 1.0f - 0.033333335f);
    for (int i = 0; i < 29; i++) DebrisTasks::kusariLoop(ks, env, m); CHECK(ks.frame == 50 && m.log.empty()); DebrisTasks::kusariLoop(ks, env, m); CHECK(has(m, "close;"));
    // Kusari init: orden de rand y formulas
    KusariState ki; m.setRand({3, 0x8000, 0x4000, 5, 0x10001, 2, 0x1FFF, 0x2000});  // 1er trozo; el resto 0
    DebrisTasks::kusariInit(ki, 1, 2, 3, -4, m);
    float speed = 0.4f + 0.06f * 3.0f; float a1 = (float)(0x8000 * 2) / 65536.0f; float b2 = 1.0f - (float)(0x4000 * 2) / 131072.0f;
    CHECK(ki.alpha == 1.0f && ki.floorY == -4.0f && ki.frame == 0 && ki.pieces[0].pos[2] == 3.0f);
    CHECK(ki.pieces[0].vel[0] == (speed * a1) * b2);
    float vy2 = (speed * a1) * ((float)(0x4000 * 2) / 65536.0f); if (vy2 < 0) vy2 = -vy2;
    CHECK(ki.pieces[0].vel[1] == vy2 + 0.2f);
    CHECK(ki.pieces[0].vel[2] == speed * (1.0f - (float)(0x8000 * 2) / 131072.0f));
    { volatile float five = 5.0f; CHECK(ki.pieces[0].grav == -0.04f - 0.004f * five); }
    CHECK(ki.pieces[0].rot[0] == (0x10001 & 0xFFFF) * 2 && ki.pieces[0].rot[1] == 4 && ki.pieces[0].rotVel[0] == (0x1FFF & 0xFFF) * 2 && ki.pieces[0].rotVel[1] == 0x2000 * 0 + ((0x2000 & 0xFFF) * 2));
    CHECK(ki.pieces[1].pos[0] == 1.0f && ki.pieces[2].pos[1] == 2.0f);
}

int main() {
    g_aomod = slurp("disc/TBL/AOMOD.BIN");
    testTables(); testBase(); testHit(); testColli(); testKomono(); testKomono3Helpers(); testColliBranches(); testGolden(); testOthers(); testStageObjs(); testDyna(); testTumiage(); testDebris();
    if (fails) { std::printf("%d FALLOS de %d comprobaciones\n", fails, checks); return 1; }
    std::printf("OK: %d comprobaciones, 0 fallos\n", checks);
    return 0;
}

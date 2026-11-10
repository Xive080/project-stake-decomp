// Tests de game/bosses_rest (interpretes de Boss2/Boss3 y auxiliares). Reglas sacadas del asm; tablas comparadas con disc/TBL/*.BIN si estan.
// Compilar (desde la raiz del repo):
//   c++ -std=c++17 -Wall game/test/bosses_rest_test.cpp game/bosses_rest/boss2_seq.cpp game/bosses_rest/boss3_seq.cpp game/bosses_rest/boss3_bomb.cpp game/bosses_rest/boss_rest_data.cpp \
//       game/bosses_a/boss2.cpp game/bosses_a/boss3.cpp game/bosses_a/boss_data.cpp -o build/verify_boss_rest/bosses_rest_test && build/verify_boss_rest/bosses_rest_test
#include "../bosses_rest/boss2_seq.h"
#include "../bosses_rest/boss3_seq.h"
#include "../bosses_rest/boss3_bomb.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
using namespace vn::boss;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

struct Log { std::vector<std::string> v;
    void add(const std::string& s) { v.push_back(s); }
    std::string str() const { std::string r; for (auto& e : v) { r += e; r += ';'; } return r; }
    int count(const std::string& p) const { int k = 0; for (auto& e : v) if (e.compare(0, p.size(), p) == 0) k++; return k; }
    void clear() { v.clear(); } };
// Formato de los eventos: nombre y hasta 3 argumentos enteros; se omiten los ceros finales ("ry 1024", "colon", "turn 10 30 5").
static std::string S(const char* n, int a = 0, int b = 0, int c = 0) { char buf[96]; int k = c ? 3 : b ? 2 : a ? 1 : 0; int v[3] = {a, b, c}; std::string r = n; for (int i = 0; i < k; i++) { snprintf(buf, sizeof buf, " %d", v[i]); r += buf; } return r; }
static std::string SND(int id, int a2, int a3, int vol) { char buf[96]; snprintf(buf, sizeof buf, "sound %d %d %d %d", id, a2, a3, vol); return buf; }

// --- lectura de ficheros reales (si faltan, los tests de tablas se omiten con mensaje)
static std::vector<uint8_t> readFile(const char* p) { std::vector<uint8_t> d; FILE* f = fopen(p, "rb"); if (!f) return d; uint8_t b[4096]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); fclose(f); return d; }
static uint32_t rd32(const std::vector<uint8_t>& d, size_t o) { return d[o] | d[o + 1] << 8 | d[o + 2] << 16 | (uint32_t)d[o + 3] << 24; }
static const uint32_t LOAD = 0x9C4800;

// ================= Boss3 =================
struct Mock3 : Boss3Engine {
    Log L; int mlen = 100; int rnd = 0;
    void sound(int id, int a2, int a3, int vol) override { L.add(SND(id, a2, a3, vol)); }
    void addScore(int, int) override {}
    int damagePlayer(int, int, int) override { return 1; }
    float playerDamageMul(int) override { return 1; }
    void addHitNum(int) override {}
    int startDifficulty() override { return 0; }
    int rand() override { L.add("rand"); return rnd; }
    bool playerValid(int) override { return true; }
    int motionLength(int) override { return mlen; }
    void setMotionNow(int a) override { L.add(S("motnow", a)); }
    void setMotionBlend(int a, int b) override { L.add(S("motblend", a, b)); }
    void overlayMotion(int, int) override {}
    void damageEffect(int) override {}
    void hitMark(int, int, int) override {}
    void spawnBomb() override { L.add("bomb"); }
    void spawnLaser() override { L.add("laser"); }
    void cameraEvent(int a, int b) override { L.add(S("camev", a, b)); }
    void blur(int, int) override {}
    int attackPathAux(int) override { return 0; }
    void unlockFlag(int, int) override {}
};
struct Host3 : Bs3SeqHost {
    Log L; float cam = 0; int track = 0; int pathEnd = 77; float ground = 0;
    void push() override { L.add("push"); } void pop() override { L.add("pop"); }
    void translate(float x, float y, float z) override { L.add(S("tr", (int)x, (int)y, (int)z)); }
    void rotX(int a) override { L.add(S("rx", a)); } void rotY(int a) override { L.add(S("ry", a)); } void rotZ(int a) override { L.add(S("rz", a)); }
    void scale(float x, float, float) override { L.add(S("sc", (int)(x * 100))); }
    void transformPoint(const float in[3], float out[3]) override { L.add(S("xf", (int)in[0], (int)in[1], (int)in[2])); out[0] = in[0] + 1; out[1] = in[1] + 2; out[2] = in[2] + 3; }
    float camFrame() override { return cam; } int camTrackId() override { return track; }
    int pathEndFrame(int id) override { L.add(S("pathend", id)); return pathEnd; }
    void spawnXWing() override { L.add("xwing"); } void blurInit() override { L.add("blurinit"); } void blurEnd() override { L.add("blurend"); }
    void releaseTrail() override { L.add("trail"); } void freeObject(void*) override { L.add("free"); }
    void loopSoundOffEvent() override { L.add("loopoff"); }
    float groundHeight(float, float y, float) override { L.add(S("ground", (int)y)); return ground; }
    void taskEnd() override { L.add("taskend"); }
};

static void testBoss3(const std::vector<uint8_t>& bs3) {
    // --- tabla de saltos del BIN: las operaciones sin implementar son justo las que saltan a 0x9C9130; la 48 a 0x9C9128
    if (!bs3.empty()) {
        size_t o = 0x9D1DE0 - LOAD; int same = 0;
        for (int op = 0; op < 59; op++) {
            uint32_t t = rd32(bs3, o + op * 4);
            bool hung = (op == 6 || (op >= 49 && op <= 51) || op == 53 || op == 55);
            CHECK((t == 0x9C9130) == hung);
            if (t == 0x9C9130) same++;
        }
        CHECK(same == 6);
        CHECK(rd32(bs3, o + 48 * 4) == 0x9C9128);
        // D_009D0A20
        size_t t = 0x9D0A20 - LOAD; bool ok = true;
        for (int p = 0; p < 3; p++) for (int d = 0; d < 34; d++) for (int k = 0; k < 2; k++) if ((int8_t)bs3[t + p * 68 + d * 2 + k] != kBs3HitRows[p][d][k]) ok = false;
        CHECK(ok);
    } else printf("(omitido: disc/TBL/BS3MOD.BIN no esta)\n");
    CHECK(kBs3HitRows[0][0][0] == 2 && kBs3HitRows[0][4][0] == 3 && kBs3HitRows[0][12][0] == 4);

    Mock3 g; Host3 h; Bs3State s; Bs3Ext x; Bs3Env e; Bs3SysGlobals sg;
    auto run = [&](std::vector<int16_t>& prog) { s.seqPc = prog.data(); return Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); };

    // espera de camara (op 0): cede sin avanzar; con la camara pasada avanza 2 elementos (4 B)
    { std::vector<int16_t> p = {0, 50, 9, 48}; h.cam = 10; CHECK(run(p) == 0 && s.seqPc == p.data());
      h.cam = 50; run(p); CHECK((s.st & 0x80000) && s.seqPc == p.data() + 3); }   // op 9 pone el bit y op 48 cede sin avanzar
    // op 1: Allow antes, Forbid despues; op 7/8
    { std::vector<int16_t> p = {1, 100, 48}; sg.cameraStartAllowed = 0; h.cam = 50; run(p); CHECK(sg.cameraStartAllowed == 1 && s.seqPc == p.data());
      h.cam = 100; run(p); CHECK(sg.cameraStartAllowed == 0 && s.seqPc == p.data() + 2); }
    { std::vector<int16_t> p = {7, 8, 48}; sg.cameraStartAllowed = 0; run(p); CHECK(sg.cameraStartAllowed == 1 && s.seqPc == p.data() + 1);   // 8 espera a 0
      sg.cameraStartAllowed = 0; s.seqPc = p.data(); run(p); CHECK(sg.cameraStartAllowed == 1); }
    // ops 2,3,4 (fotograma del movimiento)
    { std::vector<int16_t> p = {2, 30, 48}; s.frame = 29; run(p); CHECK(s.seqPc == p.data()); s.frame = 30; run(p); CHECK(s.seqPc == p.data() + 2); }
    { std::vector<int16_t> p = {3, 48}; g.mlen = 100; s.frame = 99.5f; run(p); CHECK(s.seqPc == p.data()); s.frame = 100; run(p); CHECK(s.seqPc == p.data() + 1); }
    { std::vector<int16_t> p = {4, 3, 48}; g.mlen = 10; s.frame = 29; run(p); CHECK(s.seqPc == p.data()); s.frame = 30; run(p); CHECK(s.seqPc == p.data() + 2); }
    // op 5: espera a que acabe la ventana de ataque
    { std::vector<int16_t> p = {5, 48}; s.enemyFlags = 0x10000000; run(p); CHECK(s.seqPc == p.data()); s.enemyFlags = 0; run(p); CHECK(s.seqPc == p.data() + 1); }
    // ops 9/10/23/24/25 (bits de +0x1DA4)
    { std::vector<int16_t> p = {9, 23, 48}; s.st = 0; run(p); CHECK(s.st == (0x80000 | 1)); }
    { std::vector<int16_t> p = {24, 10, 48}; s.st = 3; run(p); CHECK(s.st == 0x2); }
    { std::vector<int16_t> p = {23, 24, 25, 48}; s.st = 3; run(p); CHECK((s.st & 3) == 0); }
    // op 13: velocidad = 1.2 * arg/256
    { std::vector<int16_t> p = {13, 256, 48}; run(p); CHECK(std::fabs(s.speed - 1.2f) < 1e-5f); }
    // ops 14,15,17
    { std::vector<int16_t> p = {14, 1000, 15, 24, 48}; s.pos.ay = 0; run(p); CHECK(s.pos.ay == 1024 && x.lastTurn == 24); }
    { std::vector<int16_t> p = {17, 48}; s.pos.x = 5; s.pos.ay = 9; run(p); CHECK(s.pos.x == 0 && s.pos.ay == 0); }
    // op 16: gira alrededor de (3268.7, 0, -8.03): push, translate, rotY, transformPoint, pop
    { std::vector<int16_t> p = {16, 0x400, 48}; h.L.clear(); s.pos = Bs3Pos(); s.pos.x = 4000; s.pos.y = 5; s.pos.z = 7; run(p);
      CHECK(h.L.v.size() == 5 && h.L.v[0] == "push" && h.L.v[2] == S("ry", 1024) && h.L.v[4] == "pop");
      CHECK(s.pos.ay == 0x400);
      CHECK(h.L.v[1].compare(0, 8, "tr 3268 ") == 0);                                   // translate(3268.7, 0, -8.03) truncado por el mock
      CHECK(h.L.v[3] == S("xf", (int)(4000 - 3268.7f), 5, (int)(7 + 8.0267f)));       // punto relativo al centro (z - (-8.03))
      CHECK(s.pos.x == (float)(int)(4000 - 3268.7f) + 1 || std::fabs(s.pos.x - (4000 - 3268.7f) - 1) < 0.5f); }
    // ops 18/19: orden de rotaciones por eulerOrder; 18 escribe x,z (con -off), 19 x,y,z
    { const char* expect[6] = {"rx;ry;rz;", "rx;rz;ry;", "ry;rx;rz;", "ry;rz;rx;", "rz;rx;ry;", "rz;ry;rx;"};
      for (int ord = 0; ord < 6; ord++) { std::vector<int16_t> p = {19, 48}; x.eulerOrder = ord; x.scale = 2; x.off[0] = 1; x.off[1] = 2; x.off[2] = 3;
        s.pos = Bs3Pos(); s.pos.ax = 1; s.pos.ay = 2; s.pos.az = 3; h.L.clear(); x.bodyFlags = 0; run(p);
        std::string r; for (auto& t : h.L.v) if (t[0] == 'r') { r += t.substr(0, 2); r += ';'; }
        CHECK(r == expect[ord]);
        CHECK(h.L.v.front() == "push" && h.L.v.back() == "pop" && h.L.count("sc 200") == 1); CHECK(x.bodyFlags == 0x12); }
      std::vector<int16_t> p = {18, 48}; x.eulerOrder = 0; s.pos = Bs3Pos(); s.pos.y = 50; x.bodyFlags = 0x1F; h.L.clear(); run(p);
      CHECK(h.L.count("xf -1 -2 -3") == 1 && s.pos.x == 0 && s.pos.z == 0 && s.pos.y == 50 && x.bodyFlags == 0x0C);   // salida mock = in + (1,2,3): x = -1+1, z = -3+3
      std::vector<int16_t> q = {19, 20, 21, 48}; x.bodyFlags = 0x1F; s.pos = Bs3Pos(); run(q); CHECK(x.bodyFlags == 0x1E);   /* 19: |=0x12; 20: &=~0x13 -> 0x0C; 21: |=0x12 */  }
    // op 22: ataque solo si no esta la reaccion de dano completa
    { std::vector<int16_t> p = {22, 5, 48}; s.st = 0x4000; g.L.clear(); run(p); CHECK(s.seqPc == p.data() + 2 && s.attackId != 5); }
    // op 26/27/28/29/30 (rutas de camara)
    { std::vector<int16_t> p = {26, -1, 48}; e.gameState = 1; e.gameKind = 0; sg.g500E12 = 0; h.L.clear(); run(p);
      CHECK(x.pathId == -1 && sg.g500E12 == 1 && x.pathEnd == 1 && (s.st & 0x200) && !(s.st & 0x400) && !(s.st & 0x800) && h.L.count("pathend") == 0);
      e.gameState = 0xA; e.gameKind = 2; sg.g500E12 = 0; s.seqPc = p.data(); run(p); CHECK(sg.g500E12 == 0); }
    { std::vector<int16_t> p = {26, 3, 48}; h.pathEnd = 77; run(p); CHECK(x.pathId == 3 && x.pathEnd == 77 && x.pathFrame == 0); }
    { std::vector<int16_t> p = {27, 0, 48}; h.track = 0x55; h.cam = 12; run(p); CHECK(x.pathId == 0x55 && x.pathFrame == 12 && (s.st & 0x600) == 0x600 && !(s.st & 0x800)); }
    { std::vector<int16_t> p = {28, 29, 40, 48}; x.pathFrame = 39; s.st = 0x600; run(p);
      CHECK((s.st & 0x800) && !(s.st & 0x600)); CHECK(s.seqPc == p.data() + 1); }   // 28 avanza 1, 29 espera a 40
    { std::vector<int16_t> p = {29, 40, 48}; x.pathFrame = 40; run(p); CHECK(s.seqPc == p.data() + 2); }
    { std::vector<int16_t> p = {30, 48}; s.st = 0; run(p); CHECK(s.seqPc == p.data()); s.st = 0x800; run(p); CHECK(s.seqPc == p.data() + 1); }
    // ops 32 (espera aproximacion) / 33 (camara) / 34 (bits 0x200000/0x400000) / 37,38
    { std::vector<int16_t> p = {32, 48}; s.st = 0x1000; run(p); CHECK(s.seqPc == p.data()); s.st = 0; run(p); CHECK(s.seqPc == p.data() + 1); }
    { std::vector<int16_t> p = {33, 7, 48}; g.L.clear(); run(p); CHECK(g.L.str() == "camev 0 7;"); }
    { std::vector<int16_t> p = {34, 1, 48}; s.st = 0x400000; run(p); CHECK(s.st == 0x200000); p[1] = 2; s.seqPc = p.data(); run(p); CHECK(s.st == 0x400000); p[1] = 0; s.seqPc = p.data(); run(p); CHECK(s.st == 0);
      p[1] = 9; s.st = 0x200000; s.seqPc = p.data(); run(p); CHECK(s.st == 0x200000); }
    { std::vector<int16_t> p = {37, 38, 38, 48}; x.counter = 9; run(p); CHECK(x.counter == 2); }
    // ops 35/36 (fuego): arg/255
    { std::vector<int16_t> p = {35, 255, 48}; run(p); CHECK(x.fireCur == 1.0f && x.fireTarget == 1.0f); std::vector<int16_t> q = {36, 51, 48}; run(q); CHECK(std::fabs(x.fireTarget - 0.2f) < 1e-6f && x.fireCur == 1.0f); }
    // op 39: parametros de golpes por dificultad (forma 1) o 1 golpe de aturdimiento (otra forma)
    { std::vector<int16_t> p = {39, 48}; s.form = 1; s.phase = 1; e.diff[0] = 5; e.nPlayers = 1; e.gameState = 1; e.gameKind = 0; int d = Boss3Logic::getDifficulty(e);
      run(p); CHECK(s.hitCtr == kBs3HitRows[1][d][0] && s.hitReload == s.hitCtr && s.staggerHits == kBs3HitRows[1][d][1] && s.staggerMax == s.staggerHits);
      s.form = 0; s.hitReload = 6; s.seqPc = p.data(); run(p); CHECK(s.hitCtr == 6 && s.staggerHits == 1 && s.staggerMax == 1); }
    // ops 40/41: colision
    { std::vector<int16_t> p = {41, 40, 48}; s.st = 0; s.enemyFlags = 0; x.partPresent[0] = 1; x.partPresent[2] = 1; x.partFlags[0] = x.partFlags[1] = x.partFlags[2] = 0;
      Boss3SeqLogic::disableCollision(s, x); CHECK((s.enemyFlags & 0x8000) && x.partFlags[0] == 0x40 && x.partFlags[1] == 0 && x.partFlags[2] == 0x40);
      s.seqPc = p.data(); run(p); CHECK(!(s.enemyFlags & 0x8000) && x.partFlags[0] == 0 && x.partFlags[2] == 0);
      s.st = 0x100; Boss3SeqLogic::disableCollision(s, x); CHECK((s.enemyFlags & 0x8000) && x.partFlags[0] == 0); }   // con 0x100 no toca las partes
    // op 42, 47
    { std::vector<int16_t> p = {42, 47, 1, 48}; x.b1E10 = 0; x.b1E11 = 7; s.st = 0; run(p); CHECK(x.b1E10 == 1 && x.b1E11 == 0 && s.st == 0x120000); std::vector<int16_t> q = {47, 0, 48}; run(q); CHECK(s.st == 0); }
    // tablas: 43 (siguiente, sin ceder), 44 (pc avanzado antes de la llamada), 45/46 condicionales, 54
    { static const int16_t t0[] = {44, 2, 48}, t1[] = {48}, t2[] = {43, 48}, t3[] = {45, 2, 46, 1, 48};
      const int16_t* const tab[] = {t0, t2, t1, t3};
      Boss3Logic::seqCmdInit(s, tab); Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(s.seqTableIdx == 2 && s.seqPc == t1);   // 44 -> tabla 2
      s.seqTableIdx = 1; s.seqPc = t2; s.seqTable = tab; Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(s.seqTableIdx == 2);   // 43 -> tabla 2
      s.seqTableIdx = 3; s.seqPc = t3; s.st = 0x40000; Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(s.seqTableIdx == 2 && s.seqPc == t1);   // 45 con 0x40000: SetTable(2)
      s.seqTableIdx = 3; s.seqPc = t3; s.st = 0; Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(s.seqPc == t3 + 4);   // 45 y 46 sin condicion: avanza 2+2 y cede en 48
      s.seqPc = t3; s.st = 0x4000; Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(s.seqTableIdx == 2); }   // 46 con 0x4000: SetTable(1) = {43,48} -> 43 pasa a la tabla 2
    // op 52 (rand descartado; seqWait = g73CC) y 54 (elige tabla segun seqWait y cede)
    { static const int16_t a0[] = {52, 0, 0, 54, 1, 2, 48}, a1[] = {48}, a2[] = {48}; const int16_t* const tab[] = {a0, a1, a2};
      Boss3Logic::seqCmdInit(s, tab); sg.g73CC = 0; g.L.clear(); Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(g.L.count("rand") == 1 && s.seqTableIdx == 1 && s.seqWait == 0);
      Boss3Logic::seqCmdInit(s, tab); sg.g73CC = 3; Boss3SeqLogic::execSeqCmd(s, x, e, sg, g, h); CHECK(s.seqTableIdx == 2); }
    // op 56 (XWing) y 57 (desenfoque) y 58 (sonidos)
    { std::vector<int16_t> p = {56, 57, 1, 57, 0, 48}; s.st = 0; h.L.clear(); run(p);
      CHECK((s.st & 0x2000) && s.motionLen == 0x15E && s.motionBlend == 0 && h.L.str() == "xwing;blurinit;blurend;"); }
    { for (int k = 0; k <= 4; k++) { std::vector<int16_t> p = {58, (int16_t)k, 0, 48}; g.L.clear(); run(p);
        static const int ids[5] = {0, 0x170211, 0x800099, 0x80009E, 0x80009F};
        if (k == 0) CHECK(g.L.v.empty()); else CHECK(g.L.str() == SND(ids[k], ids[k], k == 1 ? 0 : 1, 64) + ";"); CHECK(s.seqPc == p.data() + 3); } }
    // ops sin implementar: el original no cede ni avanza (bucle infinito); aqui devuelve RUN_HUNG
    { std::vector<int16_t> p = {6}; CHECK(run(p) == Boss3SeqLogic::RUN_HUNG); p[0] = 59; CHECK(run(p) == Boss3SeqLogic::RUN_HUNG); p[0] = 55; CHECK(run(p) == Boss3SeqLogic::RUN_HUNG); }
    // op 11/12 -> MotSupSet2
    { std::vector<int16_t> p = {11, 2, 5, 1, 12, 3, 48}; s.motionIdx = -1; s.scene = 0; g.L.clear(); run(p); CHECK(g.L.count("motblend") == 1 && s.seqPc == p.data() + 6); }

    // ExecCmd: salta hasta el comando 1
    { int16_t cmds[] = {5, 7, 1, 9}; Bs3Ext y; y.cmdRun = -1; y.cmdPc = cmds; y.cmdEnd = cmds + 4; Boss3SeqLogic::execCmd(y); CHECK(y.cmdRun == 1 && y.cmdPc == cmds + 3);
      Boss3SeqLogic::execCmd(y); CHECK(y.cmdPc == cmds + 3);   // ya no lee mas
      int16_t none[] = {5, 7}; Bs3Ext z; z.cmdPc = none; z.cmdEnd = none + 2; Boss3SeqLogic::execCmd(z); CHECK(z.cmdRun == -1); }

    // FireAlphaControl: sin banderas no hace nada; con 0x200000 se acerca al objetivo a razon de 0.15
    { Bs3State t; Bs3Ext y; t.st = 0; y.fireTarget = 1; y.fireCur = 0; Boss3SeqLogic::fireAlphaControl(t, y); CHECK(y.fireCur == 0 && y.fireOut == 0);
      t.st = 0x200000; Boss3SeqLogic::fireAlphaControl(t, y); CHECK(std::fabs(y.fireCur - 0.15f) < 1e-6f && std::fabs(y.fireOut - 0.3f) < 1e-6f);
      y.fireCur = 0.5f; y.fireTarget = 1; Boss3SeqLogic::fireAlphaControl(t, y); CHECK(y.fireCur <= 0.5f && y.fireCur > 0.5f - 1e-6f);   // tope 0.5 en modo normal
      t.st = 0x200000 | 0x4000; y.fireCur = 0.3f; Boss3SeqLogic::fireAlphaControl(t, y); CHECK(y.fireCur < 0.3f);   // dano completo: objetivo 0.02
      t.st = 0x400000; y.fireTarget = 5; y.fireCur = 1.7f; Boss3SeqLogic::fireAlphaControl(t, y); CHECK(y.fireCur <= 1.8f && y.fireOut <= 1.8f && std::fabs(y.fireOut - 1.8f) < 1e-6f);
      t.st = 0x200000; y.fireTarget = 0; y.fireCur = 0.005f; Boss3SeqLogic::fireAlphaControl(t, y); CHECK(y.fireCur == 0 && y.fireOut == 0); }
    // GetPathEndFrame, SetHeight, FinishTask0, actDelete, LoopSoundOff
    { Bs3Ext y; h.L.clear(); y.pathId = -1; CHECK(Boss3SeqLogic::getPathEndFrame(y, h) == 1 && h.L.v.empty()); y.pathId = 4; CHECK(Boss3SeqLogic::getPathEndFrame(y, h) == 77 && y.pathEnd == 77); }
    { Bs3State t; Bs3Ext y; h.ground = 10; t.pos.y = 5; t.flags2 = 0; Boss3SeqLogic::setHeight(t, y, h); CHECK(t.pos.y == 10);   // por debajo del suelo: sube
      t.pos.y = 20; y.b1EB6 = 9; Boss3SeqLogic::setHeight(t, y, h); CHECK((t.flags2 & 0x40) && y.b1EB6 == 0 && t.pos.y == 20);   // por encima: empieza a caer
      t.pos.y = 5; Boss3SeqLogic::setHeight(t, y, h); CHECK(t.pos.y == 5);   // cayendo: no se recoloca
      h.L.clear(); t.pos.y = 5; t.flags2 = 0; Boss3SeqLogic::setHeight(t, y, h); CHECK(h.L.str() == "ground 11;"); }
    { Bs3State t; Bs3Ext y; Mock3 m; Host3 hh; int dummy[2]; t.form = 1; y.f1F04 = 1; y.fx1EA0[0] = &dummy[0]; y.fx1EA0[1] = &dummy[1];
      Boss3SeqLogic::finishTask0(t, y, hh, m); CHECK(m.L.str() == "camev 0 1;" && hh.L.str() == "loopoff;trail;free;free;" && y.f1F04 == 0 && !y.fx1EA0[0] && !y.fx1EA0[1]);
      hh.L.clear(); m.L.clear(); t.form = 0; Boss3SeqLogic::finishTask0(t, y, hh, m); CHECK(m.L.v.empty() && hh.L.str() == "trail;"); }
    { Bs3State t; Bs3Ext y; Bs3Env ev; Host3 hh; ev.nowEnemyAlive = 3; ev.nowEnemyNum = 5; t.target = 1; ev.attackerFlag[1] = 1;
      CHECK(Boss3SeqLogic::actDelete(t, y, ev, hh) == 0 && (t.enemyFlags & 0x04000000) && (t.enemyFlags & 0x4000) && (t.flags2 & 0x20) && ev.nowEnemyAlive == 2 && ev.nowEnemyNum == 4 && y.b1E11 == 1 && t.target == -1 && ev.attackerFlag[1] == 0);
      Boss3SeqLogic::actDelete(t, y, ev, hh); CHECK(hh.L.str() == "taskend;" && ev.nowEnemyAlive == 2);
      CHECK(Boss3SeqLogic::actNop() == 1 && Boss3SeqLogic::actDead() == 0); }
    { Bs3Ext y; Host3 hh; y.f1F04 = 0; Boss3SeqLogic::loopSoundOff(y, hh); CHECK(hh.L.v.empty()); }
}

// ================= Boss2 =================
struct Mock2 : Boss2Engine {
    Log L; int rnd = 0; int twoLevel = 0;
    void sound(int id, int a2, int a3, int vol) override { L.add(SND(id, a2, a3, vol)); }
    void addScore(int, int) override {} int damagePlayer(int, int, int) override { return 1; } float playerDamageMul(int) override { return 1; }
    void addHitNum(int) override {} void attackPosSet() override { L.add("atkpos"); } void setAttackDist(float d) override { L.add(S("atkdist", (int)d)); }
    void resetSwordFire() override {} int twoPlayerLevel() override { return twoLevel; } int startDifficulty() override { return 0; }
    int rand() override { L.add("rand"); return rnd; } bool playerValid(int) override { return true; } float groundHeight(float, float, float) override { return 0; }
    int motionAngle(int) override { return 0; } void setMotion(int m, int a, int b) override { L.add(S("motion", m, a, b)); }
    void playOverlayMotion(int, int) override {} void spawnSonicWave() override {} void pillarDynamics(int, int, int, int, int) override {}
    void damageEffect(int) override {} void hitMark(int, int, int) override {} void warpEffect(int a, int b, int c) override { L.add(S("warpeff", a, b, c)); }
    void unlockFlag(int, int) override {} void enableCollision() override { L.add("colon"); } void disableCollision() override { L.add("coloff"); }
    void setAlpha(float a) override { L.add(S("alpha", (int)(a * 100))); } void noWeakHitFeedback() override {}
};
struct Host2 : Bs2SeqHost {
    Log L; float cam = 0; int track = 0; int mlen = 80; float camPt[3] = {10, 20, 30}; int camAng = 0x4000; Bs2WarpPoint wp[4]; int scaled = 777;
    void push() override { L.add("push"); } void pop() override { L.add("pop"); }
    void translate(float x, float y, float z) override { L.add(S("tr", (int)x, (int)y, (int)z)); }
    void rotX(int) override {} void rotY(int a) override { L.add(S("ry", a)); } void rotZ(int) override {}
    void scale(float, float, float) override {}
    void transformPoint(const float in[3], float out[3]) override { L.add(S("xf", (int)(in[0] * 10), (int)in[1], (int)(in[2] * 10))); out[0] = in[0] + 100; out[1] = in[1] + 200; out[2] = in[2] + 300; }
    float camFrame() override { return cam; } int camTrackId() override { return track; }
    void camPoint(float o[3]) override { o[0] = camPt[0]; o[1] = camPt[1]; o[2] = camPt[2]; } int camAngle() override { return camAng; }
    int motionLength(int) override { return mlen; } const Bs2WarpPoint& warpPoint(int step) override { L.add(S("warppt", step)); return wp[step & 3]; }
    void startWarpIn() override { L.add("warpin"); } void startWeakDisp() override { L.add("weak"); } void initBunshin(Bs2Ext& x) override { L.add(S("bunshin", x.bunshinArgA, x.bunshinArgB)); }
    void turnToward(const float t[3], int step) override { L.add(S("turn", (int)t[0], (int)t[2], step)); }
    void getAngleYX(float dx, float dy, float dz, int& pitch, int& yaw) override { L.add(S("angle", (int)dx, (int)dy, (int)dz)); pitch = 0; yaw = 0x1234; }
    void scaleToLength(const float in[3], float len, float out[3]) override { float l = std::sqrt(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]); for (int i = 0; i < 3; i++) out[i] = in[i] * (len / l); }
    void releaseTrack(int t, int a) override { L.add(S("reltrack", t, a)); } void releaseTrail() override { L.add("trail"); } void freeObject(void*) override { L.add("free"); }
    void deleteWarpEff() override { L.add("delwarp"); } void taskEnd() override { L.add("taskend"); }
    void charInitClear() override { L.add("clear"); } void setWarpPos() override { L.add("setwarppos"); } void newSwordFire(int i, int hnd) override { L.add(S("newsf", i, hnd)); }
    void initSwordFire(int i, int a, int b, int c) override { L.add(S("initsf", i, a, b) + " " + std::to_string(c)); } void setTakion(int i) override { L.add(S("takion", i)); }
    void finalizeSwordFire(int i) override { L.add(S("finsf", i)); } int scaledLife() override { return scaled; } void form2Setup(Bs2Ext&) override { L.add("form2"); }
};

static void testBoss2(const std::vector<uint8_t>& bs2) {
    if (!bs2.empty()) {
        size_t o = 0x9D71E0 - LOAD; int noop = 0;
        for (int op = 0; op < 30; op++) { uint32_t t = rd32(bs2, o + op * 4); if (t == 0x9C9D78) noop++; }
        CHECK(noop == 1 && rd32(bs2, o + 8 * 4) == 0x9C9D78);   // solo la op 8 es un no-op puro
        size_t t = 0x9D6250 - LOAD; bool ok = true; for (int i = 0; i < 30; i++) if ((int8_t)bs2[t + i] != kBs2OpLen[i]) ok = false; CHECK(ok);
        for (int i = 0; i < 5; i++) CHECK(kBs2OpLen[i] == kBs2CancelSkip[i]);
    } else printf("(omitido: disc/TBL/BS2MOD.BIN no esta)\n");

    Mock2 g; Host2 h; Bs2State s; Bs2Ext x; Bs2Env e; Bs2SysGlobals sg;
    auto run = [&](std::vector<int16_t>& p) { s.seqPc = p.data(); return Boss2SeqLogic::execSeqCmd(s, x, e, sg, g, h); };
    // op 0 (espera camara; avanza 2 y reinicia seqWait), 8 (no-op de 2), 19 (ceder sin avanzar)
    { std::vector<int16_t> p = {0, 50, 8, 99, 19}; h.cam = 10; s.seqWait = 5; run(p); CHECK(s.seqPc == p.data() && s.seqWait == 5);
      h.cam = 50; run(p); CHECK(s.seqPc == p.data() + 4 && s.seqWait == 0); }
    // op 1: [arg, arg+200) = Forbid y avanza; antes o despues = Allow y cede
    { std::vector<int16_t> p = {1, 100, 19}; h.cam = 99; sg.cameraStartAllowed = 0; run(p); CHECK(sg.cameraStartAllowed == 1 && s.seqPc == p.data());
      h.cam = 150; run(p); CHECK(sg.cameraStartAllowed == 0 && s.seqPc == p.data() + 2);
      h.cam = 300; sg.cameraStartAllowed = 0; s.seqPc = p.data(); run(p); CHECK(sg.cameraStartAllowed == 1 && s.seqPc == p.data()); }
    // ops 2..7
    { std::vector<int16_t> p = {2, 30, 19}; s.frame = 29; run(p); CHECK(s.seqPc == p.data()); s.frame = 30; run(p); CHECK(s.seqPc == p.data() + 2); }
    { std::vector<int16_t> p = {3, 19}; h.mlen = 80; s.frame = 79; run(p); CHECK(s.seqPc == p.data()); s.frame = 80; run(p); CHECK(s.seqPc == p.data() + 1); }
    { std::vector<int16_t> p = {4, 2, 19}; s.frame = 159; run(p); CHECK(s.seqPc == p.data()); s.frame = 160; run(p); CHECK(s.seqPc == p.data() + 2); }
    { std::vector<int16_t> p = {5, 6, 7, 19}; s.st = 1; run(p); CHECK(s.seqPc == p.data()); s.st = 2; s.seqPc = p.data() + 1; run(p); CHECK(s.seqPc == p.data() + 1);
      s.st = 0; s.enemyFlags = 0x10000000; s.seqPc = p.data() + 2; run(p); CHECK(s.seqPc == p.data() + 2); s.enemyFlags = 0; run(p); CHECK(s.seqPc == p.data() + 3); }
    // op 9 MotSupSet2, 10 velocidad, 11 ir al final del movimiento
    { std::vector<int16_t> p = {9, 7, 3, 4, 19}; s.motion = -5; g.L.clear(); run(p); CHECK(g.L.count("motion") == 1 && s.seqPc == p.data() + 4); }
    { std::vector<int16_t> p = {10, 256, 19}; s.speedMul = 2; run(p); CHECK(std::fabs(s.baseSpeed - 1.2f) < 1e-5f && std::fabs(s.speed - 2.4f) < 1e-5f); }
    { std::vector<int16_t> p = {11, 19}; s.motion = 1; h.mlen = 55; run(p); CHECK(s.frame == 55); }
    // 12 / 13: StartWarpOut (13 incrementa el paso antes)
    { std::vector<int16_t> p = {13, 19}; s.branchStep = 1; h.L.clear(); run(p); CHECK(s.branchStep == 2 && h.L.v.size() == 1 && h.L.v[0] == "warppt 2");
      std::vector<int16_t> q = {12, 19}; h.L.clear(); run(q); CHECK(s.branchStep == 2 && h.L.v[0] == "warppt 2"); }
    // 14: StartWarpIn y limpia banderas
    { std::vector<int16_t> p = {14, 19}; s.enemyFlags = 0xC0000000u; s.st = 0x41; h.L.clear(); run(p); CHECK(h.L.str() == "warpin;" && s.enemyFlags == 0x80000000u && s.st == 1); }
    // 16: recarga y dificultad
    { std::vector<int16_t> p = {16, 19}; Bs2Env ee; s.hitReload = 4; s.hitCtr = 1; s.diffTimer = 0; s.form = 0; s.difficulty = 3; s.diffUp = 1; run(p); CHECK(s.hitCtr == 4 && (s.diffTimer & 0xFF) == 1 && s.difficulty == 3);
      Bs2State ref = s; ref.hitCtr = ref.hitReload; Boss2Logic::upDifficulty(ref); Boss2Logic::setDiffParam(ref, ee, g);
      s.diffTimer = 1; s.seqPc = p.data(); run(p); CHECK(s.difficulty == ref.difficulty && s.hitCtr == 4 && (s.diffTimer & 0xFF) == 1); }
    // 17, 22, 23, 26..29
    { std::vector<int16_t> p = {17, 22, 23, 26, 27, 1, 28, 1, 29, 19}; s.deadStage = 9; s.deadB5 = 4; s.deadB6 = 4; s.st = 0; g.L.clear(); h.L.clear(); run(p);
      CHECK(s.deadStage == 2 && s.deadB5 == 0 && s.deadB6 == 0 && s.st == (0x40000 | 0x200000) && g.L.str() == "colon;coloff;" + SND(0xD001D, 0xD001D, 0, 64) + ";" && h.L.str() == "weak;");
      std::vector<int16_t> q = {27, 0, 28, 0, 19}; run(q); CHECK(s.st == 0); }
    // 24: arg1 + rand % arg2 -> +0x2069 y gp-0x73CC
    { std::vector<int16_t> p = {24, 3, 5, 19}; g.rnd = 12; run(p); CHECK(s.branchSel == 5 && sg.g73CC == 5 && s.seqPc == p.data() + 3); std::vector<int16_t> q = {24, 3, 0, 19}; CHECK(run(q) == Boss2SeqLogic::RUN_HUNG); }
    // 20/21: Pop (+ CancelWait) y ceder; 18: siguiente tabla
    { static const int16_t sub[] = {2, 5, 19}; static const int16_t main_[] = {19}; (void)main_;
      std::vector<int16_t> p = {20, 19}; s.seqSp = 1; s.seqStack[0] = sub; run(p); CHECK(s.seqPc == sub && s.seqSp == 0);
      std::vector<int16_t> q = {21}; s.seqSp = 1; s.seqStack[0] = sub; run(q); CHECK(s.seqPc == sub + 2); }   // CancelWait salta el 2 y su argumento (2 elementos)
    // ops fuera de rango
    { std::vector<int16_t> p = {30}; CHECK(run(p) == Boss2SeqLogic::RUN_HUNG); }
    // ExecCmd
    { int16_t cmds[] = {0, 5, 0x10, 0, 3, 1, 9}; Bs2Ext y; y.cmdPc = cmds; y.cmdEnd = cmds + 7; h.L.clear(); Boss2SeqLogic::execCmd(y, h);
      CHECK(y.cmdRun == 1 && y.cmdPc == cmds + 6 && h.L.str() == "bunshin 5 16;" ); (void)cmds; }
    { int16_t cmds[] = {0, 5, 0x10, 0, 0, 1}; Bs2Ext y; y.cmdPc = cmds; y.cmdEnd = cmds + 6; h.L.clear(); Boss2SeqLogic::execCmd(y, h); CHECK(h.L.count("bunshin") == 2); }
    // ControlPos
    { Bs2State t; t.form = 0; t.st = 0x800; t.px = 10 + 3; t.py = 20 + 4; t.pz = 30; h.L.clear(); Boss2SeqLogic::controlPos(t, h);   // distancia 5 -> (int)(1.0922667*5) = 5
      CHECK(h.L.str() == "turn 10 30 5;"); h.L.clear(); t.form = 1; Boss2SeqLogic::controlPos(t, h); CHECK(h.L.v.empty()); t.form = 0; t.st = 0; Boss2SeqLogic::controlPos(t, h); CHECK(h.L.v.empty()); }
    // AttackPosSet
    { Bs2State t; float out[3]; t.px = 1; t.py = 2; t.pz = 3; t.playersAtStart = 1; t.target = 0; h.L.clear(); Boss2SeqLogic::attackPosSet(t, h, out);
      CHECK(out[0] == 10 && out[1] == 20 && out[2] == 30 && t.angleY == 0x1234 && h.L.str() == "angle 9 18 27;");
      t.playersAtStart = 2; t.target = 1; h.L.clear(); Boss2SeqLogic::attackPosSet(t, h, out);   // punto local (+1.2, 0, -1.5) con target 1
      CHECK(h.L.v.size() == 6 && h.L.v[5].compare(0, 5, "angle") == 0 && h.L.v[0] == "push" && h.L.v[1] == "tr 10 20 30" && h.L.v[2] == S("ry", 16384) && h.L.v[3] == "xf 12 0 -15" && h.L.v[4] == "pop");
      CHECK(std::fabs(out[0] - (1.2f + 100)) < 1e-4f && out[1] == 200 && std::fabs(out[2] - (-1.5f + 300)) < 1e-4f);
      t.target = 0; Boss2SeqLogic::attackPosSet(t, h, out); CHECK(h.L.count("xf -12 0 -15") == 1);   // target 0: -1.2 * 1
      t.target = -1; h.L.clear(); Boss2SeqLogic::attackPosSet(t, h, out); CHECK(out[0] == 10 && h.L.count("push") == 0); }
    // SetAttackDist: mantiene la direccion respecto al ancla (con y+15) y fija la distancia
    { Bs2State t; Bs2Ext y; y.anchor[0] = 0; y.anchor[1] = 0; y.anchor[2] = 0; t.px = 30; t.py = -15 + 40; t.pz = 0;   // v = (30, 40, 0), |v| = 50
      Boss2SeqLogic::setAttackDist(t, y, h, 100); CHECK(std::fabs(t.px - 60) < 1e-3f && std::fabs(t.py - (80 - 15)) < 1e-3f && std::fabs(t.pz) < 1e-3f); }
    // FinishTask0 / actDelete / actDead / actTest
    { Bs2State t; Bs2Ext y; Bs2Env ev; Mock2 m; Host2 hh; y.warpTrack = 3; y.warpTrackAux = 7; y.swordFirePresent[0] = 1; y.warpEffPresent = 1; int d; y.fx20E4[1] = &d; y.scene = 0x13; ev.gameState = 0xA; ev.gameKind = 2; ev.nowEnemyAlive = 2; ev.nowEnemyNum = 2;
      Boss2SeqLogic::finishTask0(t, y, ev, m, hh); CHECK(hh.L.str() == "reltrack 3 7;trail;delwarp;free;" && y.warpTrack == -1 && y.swordFireDetached[0] == 1 && y.swordFireDetached[1] == 0 && m.L.str() == "alpha 100;" && ev.nowEnemyAlive == 1 && ev.nowEnemyNum == 1);
      Mock2 m2; Host2 h2; Bs2Env e2; Bs2Ext y2; Boss2SeqLogic::finishTask0(t, y2, e2, m2, h2); CHECK(h2.L.str() == "trail;" && m2.L.v.empty()); }
    { Bs2State t; Bs2Ext y; Bs2Env ev; Host2 hh; t.target = 1; ev.attackerFlag[1] = 1; CHECK(Boss2SeqLogic::actDelete(t, y, ev, hh) == 0 && (t.enemyFlags & 0x4000) && (t.flags2 & 0x20) && t.deadB5 == 1 && ev.attackerFlag[1] == 0);
      Boss2SeqLogic::actDelete(t, y, ev, hh); CHECK(hh.L.str() == "taskend;"); CHECK(Boss2SeqLogic::actTest() == 0); }
    { Bs2State t; Bs2Ext y; Host2 hh; Bs2SysGlobals gs; t.st = 0x40000; t.outlineAlpha = 0.03f; t.frame = 99; Boss2SeqLogic::actDead(t, y, gs, hh); CHECK(std::fabs(t.outlineAlpha - 0.01f) < 1e-6f && (t.st & 0x40000) && gs.g73CC == 0);
      t.frame = 100; hh.track = 0x199; Boss2SeqLogic::actDead(t, y, gs, hh); CHECK(t.outlineAlpha == 0 && !(t.st & 0x40000) && t.deadStage == 2);   // pista 0x199
      t.deadStage = 0; hh.track = 5; gs.g729C = 1; Boss2SeqLogic::actDead(t, y, gs, hh); CHECK(gs.g73CC == 2 && gs.g72A0 == 1 && gs.cameraStartAllowed == 1 && t.deadStage == 0);
      t.deadB5 = 1; gs.g73CC = 0; Boss2SeqLogic::actDead(t, y, gs, hh); CHECK(gs.g73CC == 0); }
    // CharInit: valores de configuracion leidos del BIN (forma -> D_009D5D20)
    if (!bs2.empty()) {
        size_t base = 0x9D5D20 - LOAD; int configured = 0;
        for (int f = 0; f < 5; f++) {
            uint32_t p = rd32(bs2, base + f * 4);
            Bs2FormConfig c{}; const Bs2FormConfig* pc = nullptr;
            if (p) { size_t o = p - LOAD; c.orC = rd32(bs2, o); c.andC = rd32(bs2, o + 4); c.orSt = rd32(bs2, o + 8); c.andSt = rd32(bs2, o + 0xC);
                uint32_t a = rd32(bs2, o + 0x10), b = rd32(bs2, o + 0x14); std::memcpy(&c.alphaOutThr, &a, 4); std::memcpy(&c.alphaInThr, &b, 4);
                c.branch = nullptr; c.branchParam = 0; c.seqTable = nullptr; c.deadStage = bs2[o + 0x24]; c.hitReload = (int8_t)bs2[o + 0x25]; pc = &c; configured++; }
            Bs2State t; Bs2Ext y; Bs2Env ev; Mock2 m; Host2 hh; Bs2SysGlobals gs; t.form = (uint8_t)f; t.enemyFlags = 0; t.st = 0xFFFFFFFFu; t.branchStep = 9;
            Bs2State own; own.st = 0x1234; y.owner = &own;
            Boss2SeqLogic::charInit(t, y, ev, gs, m, hh, pc);
            CHECK(hh.L.v.size() >= 1 && hh.L.v[0] == "clear" && t.branchStep == 0);
            if (pc) { CHECK(t.hitReload == c.hitReload && t.hitCtr == c.hitReload); CHECK(t.alphaOutThr == c.alphaOutThr && t.alphaInThr == c.alphaInThr); CHECK((t.enemyFlags & ~c.andC) == 0); }
            if (f == 0) CHECK(hh.L.str() == "clear;setwarppos;newsf 0 -1;initsf 0 2 20 0;newsf 1 -1;initsf 1 1 30 1;takion 1;finsf 1;" && y.warpTrack == 0 && y.warpTrackAux == 7);
            if (f == 1) CHECK(hh.L.count("setwarppos") == 0 && hh.L.count("newsf") == 2);
            if (f == 2) CHECK(hh.L.count("form2") == 1 && (t.st & 0x8000));
            if (f == 3) CHECK(t.st == 0x1234);
            if (f == 4) CHECK(t.deadStage == 5 && t.deadB5 == 0);
        }
        CHECK(configured >= 1);
        printf("CharInit: %d de 5 formas con configuracion en el BIN\n", configured);
    }
    // CharInit con configuracion sintetica: banderas, tabla de secuencias y vida
    { Bs2FormConfig c{}; c.orC = 0x10; c.andC = 0xFFFFFFF0u | 0x10; c.orSt = 0x2; c.andSt = ~0x1u; c.alphaOutThr = 0.5f; c.alphaInThr = 0.7f; c.deadStage = 6; c.hitReload = 3; c.branchParam = 42;
      static const int16_t sq[] = {19}; static const int16_t* const tab[] = {sq}; c.seqTable = tab;
      Bs2State t; Bs2Ext y; Bs2Env ev; Mock2 m; Host2 hh; Bs2SysGlobals gs; t.form = 1; t.st = 0x1; ev.bossLifeMax = 0; hh.scaled = 555;
      Boss2SeqLogic::charInit(t, y, ev, gs, m, hh, &c); CHECK((t.enemyFlags & 0x10) && t.st == 0x2 && t.alphaOutThr == 0.5f && t.hitCtr == 3 && t.branchParam == 42 && t.seqPc == sq && t.deadStage == 6);
      Bs2State t2; t2.form = 2; ev.gameState = 0xA; ev.gameKind = 2; Boss2SeqLogic::charInit(t2, y, ev, gs, m, hh, nullptr); CHECK(t2.st & 0x8000); }
}

struct HostBomb : Bs3BombHost {
    Log L; std::vector<float> rs; size_t i = 0;
    float rand01() override { return rs[i++ % rs.size()]; }
    void setTargetLocalFloat(Bs3BombState& b) override { L.add("tlf"); b.target[0] = 10; b.target[1] = 20; b.target[2] = 30; }
    void setTargetWorld(Bs3BombState&) override { L.add("tw"); } void ctrlPreTarget2D(Bs3BombState&) override { L.add("t2d"); } void ctrlPreBound0(Bs3BombState&) override { L.add("b0"); }
};
static void testBomb() {
    HostBomb h; Bs3BombState b;
    Boss3BombLogic::resetAcc(b); CHECK(b.acc[0] == 0 && b.acc[2] == 0 && std::fabs(b.acc[1] - (-0.03929f)) < 1e-4f);
    b.pos[0] = 5; b.pos[1] = 7; b.pos[2] = 9; b.base[0] = 1; b.base[1] = 2; b.base[2] = 3; Boss3BombLogic::adjustLocalPos(b); CHECK(b.local[0] == 4 && b.local[1] == 5 && b.local[2] == 6);
    b.kindB = 1; h.rs = {0.75f, 0.25f}; Boss3BombLogic::setTargetLocal(b, h); CHECK(std::fabs(b.target[0] - 2.0f) < 1e-6f && std::fabs(b.target[1] - (-1.0f)) < 1e-6f && b.target[2] == -1.5f);   // 8*(0.75-0.5), 4*(0.25-0.5)
    b.kindB = 0; b.kindA = 0; h.i = 0; Boss3BombLogic::setTargetLocal(b, h); CHECK(std::fabs(b.target[0] - (4 * 0.25f - 2.0f)) < 1e-6f);   // 4*(r-0.5) - 2*(1-2*0)
    b.kindA = 1; h.i = 0; Boss3BombLogic::setTargetLocal(b, h); CHECK(std::fabs(b.target[0] - (4 * 0.25f + 2.0f)) < 1e-6f);
    b.kindA = -1; h.i = 0; Boss3BombLogic::setTargetLocal(b, h); CHECK(std::fabs(b.target[0] - 2.0f) < 1e-6f);
    b.pos[0] = 0; b.pos[1] = 0; b.pos[2] = 0; h.L.clear(); Boss3BombLogic::ctrlPreFloat(b, h); CHECK(h.L.str() == "tlf;" && std::fabs(b.pos[0] - 0.5f) < 1e-5f && std::fabs(b.vel[1] - 1.0f) < 1e-5f && std::fabs(b.pos[2] - 1.5f) < 1e-5f);
    h.L.clear(); Boss3BombLogic::ctrlPreFall(b, h); CHECK(h.L.str() == "b0;"); h.L.clear(); Boss3BombLogic::ctrlPreFly(b, h); CHECK(h.L.str() == "tw;b0;"); h.L.clear(); Boss3BombLogic::ctrlPreBound(b, h); CHECK(h.L.str() == "tw;t2d;b0;");
}

int main() {
    auto bs2 = readFile("disc/TBL/BS2MOD.BIN"), bs3 = readFile("disc/TBL/BS3MOD.BIN");
    testBoss3(bs3);
    testBoss2(bs2);
    testBomb();
    printf("%d comprobaciones, %d fallos\n", checks, fails);
    return fails ? 1 : 0;
}

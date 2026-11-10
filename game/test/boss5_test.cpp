// Tests de game/bosses_b/boss5 (jefe 5, overlay bs5mod). Valores sacados de asm/ELF/BIN, no de la salida del codigo.
// Compilar y ejecutar (desde la raiz del repo):
//   c++ -std=c++17 -Wall game/test/boss5_test.cpp game/bosses_b/boss5.cpp game/bosses_b/boss5_data.cpp -o build/verify_boss5/boss5_test && build/verify_boss5/boss5_test
#include "../bosses_b/boss5.h"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace vn::boss5;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

struct Mock : Boss5Env {
    Boss5State* st = nullptr;
    float cam = 0; int trackId = 0; int camRotV = 0; int rnd = 0; int dmgResult = 0; float scale = 1.0f; bool alive = true; float rf = 0.5f;
    int lastMotion = -1, lastStart = -1, lastBlend = -1; std::vector<int> sounds, hooks, hookArgs; int playerDamaged = -1, dmgGiven = -1;
    std::vector<std::pair<int, int>> registered; int motLen = 100; float camSpeed = 0; float ground = 0; std::vector<std::pair<int, int>> stats;
    float camFrame() override { return cam; }
    int camTrackId() override { return trackId; }
    int camRot() override { return camRotV; }
    void setCamSpeed(float v) override { camSpeed = v; }
    int motionLen(int) override { return motLen; }
    int rand() override { return rnd; }
    float randFloat() override { return rf; }
    int damagePlayer(int p, int, int d) override { playerDamaged = p; dmgGiven = d; return dmgResult; }
    void playMotion(int m, int s, int bl) override { lastMotion = m; lastStart = s; lastBlend = bl; if (st) { st->motion = m; st->motionFrame = (float)s; } }
    void sound(int id, int) override { sounds.push_back(id); }
    int statFunc(int a, int b) override { stats.push_back({a, b}); return 0; }
    float damageScale(int) override { return scale; }
    void registerHit(int part, int pts) override { registered.push_back({part, pts}); }
    bool playerAlive(int) override { return alive; }
    float groundY(const float*) override { return ground; }
    void hook(int what, int a, int, int) override { hooks.push_back(what); hookArgs.push_back(a); }
};
static bool hooked(const Mock& m, int h) { for (int x : m.hooks) if (x == h) return true; return false; }
static int countHook(const Mock& m, int h) { int n = 0; for (int x : m.hooks) n += x == h; return n; }

// ---- lectura de los binarios reales (omitir con mensaje si no estan)
static std::vector<unsigned char> readFile(const char* p) {
    std::vector<unsigned char> v; FILE* f = fopen(p, "rb"); if (!f) return v;
    unsigned char buf[65536]; size_t n; while ((n = fread(buf, 1, sizeof buf, f)) > 0) v.insert(v.end(), buf, buf + n); fclose(f); return v;
}
static std::vector<unsigned char> BIN, ELF;
static const unsigned BASE = 0x9C4800, GP = 0x2E00F0;
static const unsigned char* at(unsigned a) {
    if (a >= BASE && a < 0xA00000) return BIN.data() + (a - BASE);
    return ELF.data() + (a - 0x140000 + 0x800);
}
static int16_t rh(unsigned a) { int16_t v; memcpy(&v, at(a), 2); return v; }
static uint32_t rw(unsigned a) { uint32_t v; memcpy(&v, at(a), 4); return v; }
static float rf32(unsigned a) { float v; memcpy(&v, at(a), 4); return v; }

static void testTables() {
    BIN = readFile("disc/TBL/BS5MOD.BIN"); ELF = readFile("baserom/SCES_504.11");
    if (BIN.empty() || ELF.empty()) { printf("OMITIDO: tablas contra disc/TBL/BS5MOD.BIN / baserom/SCES_504.11 (no estan)\n"); return; }
    // valores a mano desde el .s de datos
    CHECK(kAttackTable[0].motion == 0x1C9 && kAttackTable[0].f[1] == 0x6E && kAttackTable[0].endFrame == 0x168);   // D_009DBD00
    CHECK(kAttackTable[2].motion == 0x1D4 && kAttackTable[2].dmg == 7 && kAttackTable[3].motion == 0x1D8 && kAttackTable[3].dmg == 9);
    CHECK(kKindConfig[0].orC == 0xE8600u && kKindConfig[0].or1E40 == 0x4000u && kKindConfig[1].orC == 0xC8600u && kKindConfig[1].or1E40 == 0x80u);   // 0x9CF410 / 0x9CF430
    CHECK(kKindConfig[0].andC == 0xFFFFFFFFu && kKindConfig[1].seqTab == kSeqTab1 && kKindConfig[0].seqTab == kSeqTab0);
    CHECK(kDiffStep[0][0] == 1 && kDiffStep[0][1] == 2 && kDiffStep[1][0] == 1 && kDiffStep[1][1] == 2);    // ELF 0x2D87F8
    // comparaciones con los ficheros
    for (int i = 0; i < 47; i++) CHECK(kCmdLen[i] == (int8_t)*at(0x9CF8D0 + i));
    for (int i = 0; i < 4; i++) {
        unsigned a = 0x9DBD00 + 20 * i;
        CHECK(kAttackTable[i].motion == rh(a) && kAttackTable[i].dmg == (int8_t)*at(a + 3) && kAttackTable[i].endFrame == rh(a + 0x12));
        for (int k = 0; k < 6; k++) CHECK(kAttackTable[i].f[k] == rh(a + 6 + 2 * k));
    }
    for (int l = 0; l < 2; l++) for (int d = 0; d < 17; d++) {
        CHECK(kDmgMul[l][d] == (int32_t)rw(0x9CF730 + l * 0x44 + d * 4));
        CHECK(kPillarSpeed[l][d] == rf32(0x9CF450 + l * 0x44 + d * 4));
        for (int k = 0; k < 3; k++) CHECK(kStrength[l][d][k] == rf32(0x9CF4E0 + l * 0xCC + d * 12 + 4 * k));
        for (int s = 0; s < 5; s++) CHECK(kPartLife[s][l][d] == (int8_t)*at(0x9CF680 + s * 0x22 + l * 0x11 + d));
    }
    for (int l = 0; l < 2; l++) { CHECK(kDiffStep[l][0] == rh(GP - 0x78F8 + 4 * l)); CHECK(kDiffStep[l][1] == rh(GP - 0x78F6 + 4 * l)); }
    for (int i = 0; i < 12; i++) CHECK(kPose[i / 6][i % 6] == rf32(0x9CE850 + 4 * i));
    CHECK(kPose[1][0] == rf32(0x9CE868));
    // scripts: cada puntero de las tablas debe dar el mismo contenido en kScript
    auto script = [&](unsigned addr, int16_t* out, int n) { for (int i = 0; i < n; i++) out[i] = rh(addr + 2 * i); };
    auto same = [&](unsigned addr, unsigned idx) { int16_t w[4]; script(addr, w, 4); for (int i = 0; i < 4; i++) if (kScript[idx + i] != w[i]) return false; return true; };
    for (int i = 0; kSeqTab1[i] != 0xFFFF; i++) CHECK(same(rw(0x9CF3C0 + 4 * i), kSeqTab1[i]));
    for (int i = 0; kSeqTab0[i] != 0xFFFF; i++) CHECK(same(rw(0x2D87D0 + 4 * i), kSeqTab0[i]));
    CHECK(same(rw(GP - 0x7930), kCallScript[0]) && same(rw(GP - 0x792C), kCallScript[1]));
    CHECK(rw(0x9CF3C0 + 4 * 17) == 0);   // la tabla 1 tiene 17 entradas
    {   // todo el bloque del overlay coincide
        bool ok = true; for (unsigned i = 0; i < (0x9CF3C0 - 0x9CEF60) / 2; i++) if (kScript[i] != rh(0x9CEF60 + 2 * i)) ok = false;
        CHECK(ok);
    }
}

// recorre un script desde `s` hasta un comando que no avanza el pc; devuelve false si ve un codigo invalido
static bool walk(const int16_t* s) {
    for (int n = 0; n < 400; n++) {
        unsigned op = (uint16_t)s[0]; if (op >= 47) return false;
        if (kCmdLen[op] == 0 || op == 1 || op == 0 || op == 2) return true;
        s += kCmdLen[op];
    }
    return false;
}

static void testVM() {
    Mock m; Boss5State b; Boss5Globals g; m.st = &b;
    // todos los scripts de las tablas se leen sin salirse de los codigos 0..46 y acaban en una instruccion que no avanza
    for (int i = 0; kSeqTab0[i] != 0xFFFF; i++) CHECK(walk(kScript + kSeqTab0[i]));
    for (int i = 0; kSeqTab1[i] != 0xFFFF; i++) CHECK(walk(kScript + kSeqTab1[i]));
    for (int i = 0; i < 2; i++) CHECK(walk(kScript + kCallScript[i]));

    // seqInit + tablas
    Boss5Logic::seqInit(b, kSeqTab1);
    CHECK(b.pc == kScript + kSeqTab1[0] && b.sp == 0 && b.timer[0] == -1);
    Boss5Logic::seqNextTable(b); CHECK(b.tabIdx == 1 && b.pc == kScript + kSeqTab1[1]);
    Boss5Logic::seqSetTable(b, 4); CHECK(b.tabIdx == 4 && b.pc == kScript + kSeqTab1[4] && b.sp == 0);
    const int16_t* saved = b.pc;
    Boss5Logic::seqCall(b, 1); CHECK(b.sp == 1 && b.stack[0] == saved && b.pc == kScript + kCallScript[1] && b.timer[1] == -1);
    Boss5Logic::seqPop(b); CHECK(b.sp == 0 && b.pc == saved);
    { static const int16_t w[] = {6, 0, 0}; b.pc = w; Boss5Logic::seqCancelWait(b); CHECK(b.pc == w + 1);   // op 6 (len 1)
      static const int16_t w2[] = {9, 5}; b.pc = w2; Boss5Logic::seqCancelWait(b); CHECK(b.pc == w2); }      // op 9 no se salta

    // tipo 0: primer script = {23,457,0,0, 24,0, 38,14,1, 38,15,0, 16, 29,0, 24,256, 5,359, 24,0, 38,14,0, 38,15,1, 9,60, 18}
    b = Boss5State(); b.kind = K_BATTLE1; m = Mock(); m.st = &b; g = Boss5Globals(); m.trackId = 5;
    Boss5Logic::seqInit(b, kKindConfig[0].seqTab);
    Boss5Logic::execSeqCmd(b, g, m);
    CHECK(m.lastMotion == 457 && m.lastStart == 0 && m.lastBlend == 0);   // op 23
    CHECK(b.rateBase == 0.0f && b.rate == 0.0f);                          // op 24 con 0
    CHECK(b.f1E40 == 0x4000u);                                             // op 38 (14,1) puso el bit; el (15,0) no toco nada
    CHECK(!(b.f1E40 & 0x8000));                                           // op 38 (15,0)
    CHECK(b.pc && b.pc[0] == 16);                                         // op 16 espera la ruta 0 (la de camara es 5)
    m.trackId = 0; g.lastHitTarget = 1;
    Boss5Logic::execSeqCmd(b, g, m);
    CHECK(b.attack == 0 && b.target == 1 && g.targetBusy[1] == 1 && (b.f0C & F0C_ATTACKING) && (b.f1E40 & FB_ATTACK));   // op 29: SelectAttackTarget + StartAttack(0)
    CHECK(b.rateBase == 1.2f);                                            // op 24 con 256
    CHECK(b.pc[0] == 5);                                                  // espera frame > 359
    b.motionFrame = 359; Boss5Logic::execSeqCmd(b, g, m); CHECK(b.pc[0] == 5);
    b.motionFrame = 360; b.f1E40 |= 0x8000;
    m.hooks.clear(); Boss5Logic::execSeqCmd(b, g, m);
    CHECK(!(b.f1E40 & 0x4000) && (b.f1E40 & 0x8000) && countHook(m, H_BIT_0F_SOUND) == 1);   // 38(14,0), 38(15,1)
    CHECK(b.pc[0] == 9);
    {   int n = 1; while (b.state != TS_FINISHED && n < 200) { Boss5Logic::execSeqCmd(b, g, m); n++; }
        CHECK(n == 61 && b.phaseCount == 1 && b.stateB == 0); }         // timer 60: 60 esperas + 1; luego 18 y op 1 del centinela del ELF

    // ops sueltos con scripts de prueba
    auto run = [&](const int16_t* sc) { b.pc = sc; b.timer[0] = -1; b.sp = 0; Boss5Logic::execSeqCmd(b, g, m); };
    {   static const int16_t s3[] = {3, 10, 0};
        m.cam = 5; run(s3); CHECK(b.pc == s3);
        m.cam = 11; run(s3); CHECK(b.pc == s3);                            // fuera de [10,11)
        m.cam = 10.5f; run(s3); CHECK(b.pc == s3 + 2);
        static const int16_t s4[] = {4, 10, 0};
        m.cam = 5; g.cameraStartAllowed = 0; run(s4); CHECK(b.pc == s4 && g.cameraStartAllowed == 1);
        m.cam = 10; run(s4); CHECK(b.pc == s4 + 2 && g.cameraStartAllowed == 0);
        static const int16_t s12[] = {12, 3, 0};
        g.enemiesLeft = 5; run(s12); CHECK(b.pc == s12); g.enemiesLeft = 2; run(s12); CHECK(b.pc == s12 + 2);
        static const int16_t s14[] = {14, 3, 1, 0};
        b.f0C = 0; run(s14); CHECK(b.pc == s14); b.f0C = 8; run(s14); CHECK(b.pc == s14 + 3);
        static const int16_t s14b[] = {14, 3, 0, 0};
        run(s14b); CHECK(b.pc == s14b); b.f0C = 0; run(s14b); CHECK(b.pc == s14b + 3);
        static const int16_t s15[] = {15, 5, 1, 0};
        b.f1E40 = 0; run(s15); CHECK(b.pc == s15); b.f1E40 = 0x20; run(s15); CHECK(b.pc == s15 + 3);
        static const int16_t s37[] = {37, 17, 1, 37, 4, 1, 37, 4, 0, 0};
        b.f0C = 0; run(s37); CHECK(b.f0C == 0x20000u && b.pc == s37 + 9);
        static const int16_t s39[] = {39, 0, 39, 1, 0};
        b.f1E40 = 0; run(s39); CHECK(b.f1E40 == 0 && b.pc == s39 + 4);    // 39(0) pone 0x400, 39(1) lo quita
        static const int16_t s39b[] = {39, 0, 0};
        run(s39b); CHECK(b.f1E40 == 0x400u);
        static const int16_t s44[] = {44, 1, 0};
        b.f1E40 = 0; run(s44); CHECK(b.f1E40 == FB_LIFEBAR);
        static const int16_t s46[] = {46, 3, 24, 128, 31, 512, 34, 256, 32, 1, 0};
        run(s46); CHECK(b.sel == 3 && b.rateBase == 1.2f * 0.5f && m.camSpeed == 1.2f * 2.0f && b.pathSpeed == 1.2f && b.pathMode == 1);
        static const int16_t s36[] = {36, 0};
        b.pathMode = 1; run(s36); CHECK(b.pathMode == 0);
        static const int16_t s45[] = {45, 1, 0};
        run(s45); CHECK(b.pos[2] == -48.0f && b.pc == s45 + 2);
        static const int16_t s45b[] = {45, -1, 0};
        m.camRotV = 0x1234; m.hooks.clear(); run(s45b); CHECK(b.rot[1] == (float)0x1234 && hooked(m, H_POSE_ROTATE));
        static const int16_t s25[] = {25, 26, 27, 28, 0};
        m.hooks.clear(); m.hookArgs.clear(); run(s25);
        CHECK(m.hooks.size() == 4 && m.hooks[0] == H_CLEAR_BODY_MOVE && m.hookArgs[0] == 0x12 && m.hooks[1] == H_SET_BODY_MOVE && m.hookArgs[3] == 0x10);
        static const int16_t s38[] = {38, 0x13, 1, 38, 0x12, 1, 0};
        m.hooks.clear(); run(s38); CHECK(m.hooks.size() == 2 && m.hooks[0] == H_INIT_WAIT_PATH && m.hooks[1] == H_INIT_SIPPO_POS);
        static const int16_t s35[] = {35, 10, 35, 0, 0};
        run(s35); CHECK(b.f1DF4 == 1.0f && b.f1DF8 == 0.0f && b.pathMode == 1);
        static const int16_t s35b[] = {35, 4, 0};
        run(s35b); CHECK(b.f1DF4 == 0.0f && b.f1DF8 == 0.25f);
        static const int16_t s16[] = {16, 0};
        b.camTrackId = 7; b.camTrackFrame = 20; m.trackId = 8; run(s16); CHECK(b.pc == s16);
        m.trackId = 7; m.cam = 19; run(s16); CHECK(b.pc == s16); m.cam = 20; run(s16); CHECK(b.pc == s16 + 1 && b.pc[0] == 0);
        static const int16_t s10[] = {10, 0};
        b.f10 = 0x40; run(s10); CHECK(b.pc == s10); b.f10 = 0; run(s10); CHECK(b.pc == s10 + 1);
        static const int16_t s11[] = {11, 13, 0};
        b.pathPos = 1; b.pathLimitA = 2; b.pathLimitB = 0; run(s11); CHECK(b.pc == s11); b.pathPos = 2; run(s11); CHECK(b.pc == s11 + 2);   // op 11 pasa (2 >= 2) y op 13 tambien (2 >= 0): llega al op 0
        b.pathLimitB = 3; run(s11); CHECK(b.pc == s11 + 1);   // op 13 espera (2 < 3)
        static const int16_t s30[] = {30, 0};
        b.diffArmed = 1; b.difficulty = 5; b.diffDown = 2; g.gameLevel = 0; g.lastHitTarget = 0; m.dmgResult = 1; m.playerDamaged = -1;
        run(s30); CHECK(m.playerDamaged == 0 && m.dmgGiven == 7 && b.difficulty == 3 && b.diffArmed == 0);
        b.diffArmed = 1; m.dmgResult = 0; run(s30); CHECK(b.difficulty == 3 && b.diffArmed == 0);
        static const int16_t s40[] = {40, 0};
        b.hitsPerCount = 3; b.hitCounter = 0; b.f0C = F0C_IN_DAMAGE; b.f1E40 = FB_ATK_WINDOW; b.diffArmed = 1; b.difficulty = 3; b.diffUp = 1; b.sel = 0; g.gameLevel = 0;
        run(s40); CHECK(b.hitCounter == 3 && !(b.f0C & F0C_IN_DAMAGE) && !(b.f1E40 & FB_ATK_WINDOW) && b.difficulty == 4 && b.diffArmed == 1 && b.partLife == kPartLife[0][0][4]);
        static const int16_t s41[] = {41, 42, 0};
        m.hooks.clear(); b.f1E40 = 0; run(s41); CHECK(m.hooks.size() == 2 && m.hooks[0] == H_ENABLE_PART_COLLISION && (b.f0C & F0C_COLLISION_OFF));
        static const int16_t s8[] = {8, 0};
        b.f0C = F0C_ATTACKING; run(s8); CHECK(b.pc == s8); b.f0C = 0; run(s8); CHECK(b.pc == s8 + 1);
        static const int16_t s7[] = {7, 2, 0};
        m.motLen = 10; b.motionFrame = 19; run(s7); CHECK(b.pc == s7); b.motionFrame = 20; run(s7); CHECK(b.pc == s7 + 2);
    }
    // op 21: rama aleatoria; se coloca en la tabla 1 en el script centinela {21,2,3,2}: 2 + 2*(7%3) = 4
    b = Boss5State(); Boss5Logic::seqInit(b, kSeqTab1); Boss5Logic::seqSetTable(b, 13);
    CHECK(b.pc[0] == 21 && b.pc[1] == 2 && b.pc[2] == 3 && b.pc[3] == 2);
    m.rnd = 7; m.cam = 0; Boss5Logic::execSeqCmd(b, g, m);
    CHECK(b.branch[0] == 4 && b.branch[1] == 7 && b.branch[2] == 7 && b.tabIdx == 4);
    Boss5Logic::initBranchParam(b); CHECK(b.branch[0] == 0 && b.branch[1] == 0 && b.branch[2] == 0);
    CHECK(Boss5Logic::actExecSeqCmd(b, g, m) == 0);
}

static void testDifficulty() {
    Mock m; Boss5State b; Boss5Globals g;
    g.gameLevel = 1; b.sel = 2; b.difficulty = 5;
    Boss5Logic::setDiffParam(b, g);
    CHECK(b.partLife == kPartLife[2][1][5] && b.partLifeMax == b.partLife && b.dmgMul == kDmgMul[1][5] && b.diffUp == 1 && b.diffDown == 2 && g.diffMirror == 5);
    CHECK(b.partLife == 6);                            // D_009CF680 + 2*0x22 + 0x11 + 5 (fila [2][1] = 6)
    b.difficulty = 15; Boss5Logic::upDifficulty(b); CHECK(b.difficulty == 16); Boss5Logic::upDifficulty(b); CHECK(b.difficulty == 16);
    b.difficulty = 1; b.diffArmed = 1; Boss5Logic::downDifficulty(b); CHECK(b.difficulty == 0 && b.diffArmed == 0);
    b.diffArmed = 1; Boss5Logic::noChgDifficultyReq(b); CHECK(b.diffArmed == 0);
    b.difficulty = 6; Boss5Logic::downDifficultyReq(b); CHECK(b.difficulty == 4);
    Boss5Logic::initDifficulty(b, 9); CHECK(b.difficulty == 9 && b.diffArmed == 0);
    // GetStrength: umbrales 0.333 y 0.666 del nivel 0
    g.gameLevel = 0; b.difficulty = 0;
    m.rf = 0.1f; CHECK(Boss5Logic::getStrength(b, g, m) == 0);
    m.rf = 0.5f; CHECK(Boss5Logic::getStrength(b, g, m) == 1);
    m.rf = 0.9f; CHECK(Boss5Logic::getStrength(b, g, m) == 2);
    // InitLife / ControlDispLife
    g.bossLife = g.bossLifeMax = 0;
    Boss5Logic::initLife(b, g, 0); CHECK(g.bossLife == 400 && g.bossLifeMax == 400 && b.lifeRatio == 1.0f);
    Boss5Logic::initLife(b, g, 100); CHECK(g.bossLife == 100 && g.bossLifeMax == 100);
    Boss5Logic::initLife(b, g, -5); CHECK(g.bossLife == 100);
    g.bossLife = 25; b.partLife = 1; b.partLifeMax = 4; Boss5Logic::controlDispLife(b, g); CHECK(b.lifeRatio == 0.25f && b.hitRatio == 0.25f);
}

static void testDamage() {
    Mock m; Boss5State b; Boss5Globals g; m.st = &b;
    b.kind = K_BATTLE2; b.partLife = 2; b.partLifeMax = 2; b.hitCounter = 1; b.hitsPerCount = 1; b.dmgMul = 50; g.bossLife = 120; g.bossLifeMax = 400;
    Boss5Logic::seqInit(b, kSeqTab1); Boss5Logic::seqSetTable(b, 2);
    CHECK(Boss5Logic::checkWeakHit(b, 11) && !Boss5Logic::checkWeakHit(b, 12));
    b.kind = K_BATTLE1; CHECK(!Boss5Logic::checkWeakHit(b, 3)); b.kind = K_BATTLE2;
    // 1.er golpe: la vida de fase baja a 1 -> DamageUpperBody
    Boss5Logic::weakHit(b, g, m, 3, 0);
    CHECK(g.bossLife == 70 && b.partLife == 1 && m.registered.size() == 1 && m.registered[0] == std::make_pair(0, 10));
    CHECK(m.sounds.size() == 1 && m.sounds[0] == 0x1A0013 && hooked(m, H_UPPER_HIT_FX) && hooked(m, H_HIT_MARK2) && (b.f1E40 & FB_HIT_STUN) && b.stunTimer == 0x19);
    // aturdido: otro golpe no cuenta
    Boss5Logic::weakHit(b, g, m, 3, 0); CHECK(g.bossLife == 70 && b.partLife == 1);
    // acaba el aturdimiento
    for (int i = 0; i < 0x19; i++) Boss5Logic::checkDamageEndTimer(b);
    CHECK(!(b.f1E40 & FB_HIT_STUN) && b.rate == b.rateBase);
    // 2.o golpe: partLife -> 0 -> Damage (sin nado: movimiento 0x1E8, mezcla 10), avanza al siguiente script y llama al de reaccion 0
    m.hooks.clear(); m.sounds.clear(); b.motion = 0;
    Boss5Logic::weakHit(b, g, m, 3, 1);
    CHECK(g.bossLife == 20 && b.partLife == 0 && m.sounds.size() == 1 && m.sounds[0] == 0x1A0011 && m.lastMotion == 0x1E8 && m.lastBlend == 10 && m.lastStart == 0);
    CHECK(b.tabIdx == 3 && b.phaseCount == 1 && b.sp == 1 && b.pc == kScript + kCallScript[0] && (b.f0C & F0C_IN_DAMAGE) && hooked(m, H_HIT_MARK1));
    CHECK(b.rateBase == 1.2f && b.rate == 1.2f && b.rateMul == 1.0f);
    // el siguiente golpe se ignora mientras dure "en dano"
    Boss5Logic::weakHit(b, g, m, 3, 0); CHECK(g.bossLife == 20);
    // muerte: bossLife llega a 0
    b.f0C = 0; b.partLife = 5; g.bossLife = 40; m.registered.clear(); m.stats.clear(); m.hooks.clear(); b.f1E40 = 0x30200;
    Boss5Logic::weakHit(b, g, m, 3, 1);
    CHECK(g.bossLife == 0 && b.state == TS_DEAD && b.stateB == 0 && (b.f0C & F0C_DEAD));
    CHECK(m.registered.size() == 2 && m.registered[1] == std::make_pair(1, 0x5DC));
    CHECK(m.stats.size() == 3 && m.stats[0] == std::make_pair(2, 6) && m.stats[1] == std::make_pair(2, 7) && m.stats[2] == std::make_pair(2, 0xD));
    CHECK(!(b.f1E40 & 0x2000) && !(b.f1E40 & 0x10000) && !(b.f1E40 & 0x200) && (b.f1E40 & FB_SWIM) && hooked(m, H_OWNER_CLEAR_FLAGS) && m.lastMotion == 0x1DD);
    // en modo especial no hay estadisticas
    {   Mock m2; Boss5State c; Boss5Globals g2; c.kind = K_BATTLE2; g2.gameState = 0xA; g2.gameKind = 2;
        Boss5Logic::dead(c, g2, m2, 0); CHECK(m2.stats.empty() && c.state == TS_DEAD);
        Mock m3; Boss5State d; Boss5Globals g3; g3.deathCount73FC = 1; g3.deathCount73F8 = 1; Boss5Logic::dead(d, g3, m3, 0);
        CHECK(m3.stats.size() == 1 && m3.stats[0] == std::make_pair(2, 6)); }
    // tipo 0: Damage no toca el script
    {   Mock m2; Boss5State c; Boss5Globals g2; c.kind = K_BATTLE1; Boss5Logic::damage(c, g2, m2);
        CHECK(m2.sounds.empty() && m2.lastMotion == -1 && (c.f0C & F0C_IN_DAMAGE) && c.rate == 1.2f);
        Boss5Logic::damageUpperBody(c, m2); CHECK((c.f1E40 & FB_HIT_STUN) && c.stunTimer == 0 && m2.sounds.empty()); }
    // damage en nado: movimiento 0x1DD y reaccion 1
    {   Mock m2; Boss5State c; Boss5Globals g2; c.kind = K_BATTLE2; c.motion = 0x1DA; Boss5Logic::seqInit(c, kSeqTab1);
        Boss5Logic::damage(c, g2, m2);
        CHECK((c.f1E40 & FB_SWIM) && m2.lastMotion == 0x1DD && c.pc == kScript + kCallScript[1]); }
    // CheckDamage
    {   Mock m2; Boss5State c; Boss5Globals g2; c.kind = K_BATTLE2; c.partLife = 9; c.hitCounter = 1; c.hitsPerCount = 1; c.dmgMul = 10; g2.bossLife = 100;
        Boss5Logic::HitInfo hit[2] = {{3, 4, 7}, {-1, 0, 0}};
        Boss5Logic::checkDamage(c, g2, m2, hit); CHECK(g2.bossLife == 100 && m2.hooks.empty());      // sin F0C_HIT no hace nada
        c.f0C = F0C_HIT; Boss5Logic::checkDamage(c, g2, m2, hit);
        CHECK(g2.bossLife == 90 && c.weakIdx == 7 && m2.hooks.front() == H_HIT_PREPARE && hooked(m2, H_HIT_CONSUMED) && m2.hooks.back() == H_CLEAR_WEAK_HIT);
        Mock m3; Boss5State d; Boss5Globals g3; d.kind = K_BATTLE1; d.f0C = F0C_HIT; g3.bossLife = 100; d.dmgMul = 10; d.hitCounter = 1;
        Boss5Logic::checkDamage(d, g3, m3, hit); CHECK(g3.bossLife == 100 && !hooked(m3, H_HIT_CONSUMED) && hooked(m3, H_CLEAR_WEAK_HIT));
        d.f0C = F0C_HIT | F0C_WEAK; d.partLife = 5; Boss5Logic::checkDamage(d, g3, m3, hit); CHECK(g3.bossLife == 90);
        d.f0C = F0C_HIT | 0x100; m3.hooks.clear(); Boss5Logic::checkDamage(d, g3, m3, hit); CHECK(g3.bossLife == 90 && hooked(m3, H_CLEAR_WEAK_HIT) && !hooked(m3, H_HIT_CONSUMED));
        Boss5Logic::HitInfo h0[2] = {{3, 0, 0}, {-1, 0, 0}}; d.f0C = F0C_HIT | F0C_WEAK; Boss5Logic::checkDamage(d, g3, m3, h0); CHECK(g3.bossLife == 90);   // partType <= 0 no cuenta
    }
    // actDelete
    {   Mock m2; Boss5State c; Boss5Globals g2; g2.targetBusy[0] = g2.targetBusy[1] = 1; c.f18 = 5;
        CHECK(Boss5Logic::actDelete(c, g2, m2) == 0);
        CHECK(g2.targetBusy[0] == 0 && g2.targetBusy[1] == 0 && (c.f0C & (F0C_DEAD | F0C_DELETE)) == (F0C_DEAD | F0C_DELETE) && (c.f10 & 0x20) && c.f18 == 0 && c.stateB == 1);
        Boss5Logic::actDelete(c, g2, m2); CHECK(hooked(m2, H_FINISH_TASK)); }
    // actDead
    {   Mock m2; Boss5State c; Boss5Globals g2; c.motion = 3; m2.motLen = 50; c.motionFrame = 49; c.rateBase = 1.2f;
        Boss5Logic::actDead(c, g2, m2); CHECK(c.stateB == 0);
        c.motionFrame = 50; Boss5Logic::actDead(c, g2, m2); CHECK(c.stateB == 1 && c.rateBase == 0.0f && c.rate == 0.0f);
        m2.trackId = 0x27F; Boss5Logic::actDead(c, g2, m2); CHECK(c.state == TS_FINISHED && c.stateB == 0);
        c.stateB = 1; m2.trackId = 1; g2.g729C = 0; Boss5Logic::actDead(c, g2, m2); CHECK(g2.g73CC == 1 && g2.g72A0 == 0 && g2.cameraStartAllowed == 1);
        g2.g72A0 = 0; g2.g729C = 1; Boss5Logic::actDead(c, g2, m2); CHECK(g2.g72A0 == 1); }
}

static void testAttack() {
    Mock m; Boss5State b; Boss5Globals g; m.st = &b;
    b.kind = K_BATTLE1; g.numPlayersB = 2; b.target = 0;
    Boss5Logic::startAttack(b, g, m, 2);
    CHECK(b.attack == 2 && b.atkStep == 0 && b.numPlayers == 2 && m.lastMotion == 0x1D4 && m.lastStart == 0 && m.lastBlend == 5 && (b.f0C & F0C_ATTACKING) && (b.f1E40 & FB_ATTACK));
    // ataque 2: fila {f=[0,40,50,-1,-1,-1], fin 60, dano 7}
    b.rate = 1.2f; b.motionFrame = 0; Boss5Logic::controlAttack(b, g, m);
    CHECK((b.f1E40 & FB_ATK_WINDOW) && b.atkStep == 1);
    b.motionFrame = 39; m.dmgResult = 0; Boss5Logic::controlAttack(b, g, m); CHECK(b.atkStep == 1 && m.playerDamaged == -1);
    b.motionFrame = 40; b.diffArmed = 1; Boss5Logic::controlAttack(b, g, m);
    CHECK(b.atkStep == 2 && m.playerDamaged == 0 && m.dmgGiven == 7 && b.diffArmed == 0);
    b.motionFrame = 50; Boss5Logic::controlAttack(b, g, m);
    CHECK(!(b.f1E40 & FB_ATK_WINDOW) && !(b.f0C & F0C_ATTACKING) && b.atkStep == 6);   // los pasos con -1 se saltan y el 5 siempre quita ATTACKING
    b.motionFrame = 58; Boss5Logic::controlAttack(b, g, m); CHECK(b.f1E40 & FB_ATTACK);        // 58 + 1.2 < 60
    b.motionFrame = 59; Boss5Logic::controlAttack(b, g, m); CHECK(!(b.f1E40 & FB_ATTACK));      // 60.2 >= 60
    // si el movimiento cambia se cancela
    b.f1E40 = FB_ATTACK; b.f0C = F0C_ATTACKING; b.motion = 5; Boss5Logic::controlAttack(b, g, m); CHECK(!(b.f1E40 & FB_ATTACK) && !(b.f0C & F0C_ATTACKING));
    // sin FB_ATTACK no hace nada
    b.f0C = F0C_ATTACKING; Boss5Logic::controlAttack(b, g, m); CHECK(b.f0C & F0C_ATTACKING);
    // ataque 0 (esfera): hito 1 a 110 lanza la esfera; ataque 1: hielo grande en el hito 1 (which 0) pero no en el 4
    {   Mock m2; Boss5State c; Boss5Globals g2; m2.st = &c; c.kind = K_BATTLE2; Boss5Logic::startAttack(c, g2, m2, 0); c.rate = 1.0f;
        c.motionFrame = 109; Boss5Logic::controlAttack(c, g2, m2); CHECK(!hooked(m2, H_SPAWN_SPHERE));
        c.motionFrame = 110; Boss5Logic::controlAttack(c, g2, m2); CHECK(countHook(m2, H_SPAWN_SPHERE) == 1 && c.atkStep >= 2);
        Mock m3; Boss5State d; Boss5Globals g3; m3.st = &d; Boss5Logic::startAttack(d, g3, m3, 1); d.rate = 1.0f; d.motionFrame = 0x12B; Boss5Logic::controlAttack(d, g3, m3);
        CHECK(countHook(m3, H_SPAWN_XICE) == 1); }   // fila 1: f = {50,80,85,190,225,230}, fin 299: al pasar por todos los hitos de golpe, el hielo grande sale una vez (hito 1) y el hito 4 no lo repite
    // DamageToPlayer
    {   Mock m2; Boss5State c; Boss5Globals g2; c.attack = 3; c.target = 1; c.difficulty = 5; c.diffDown = 2; m2.dmgResult = 1;
        Boss5Logic::damageToPlayer(c, g2, m2); CHECK(m2.playerDamaged == 1 && m2.dmgGiven == 9 && c.difficulty == 3 && m2.sounds.size() == 1 && m2.sounds[0] == 0x8000C6);
        m2.dmgResult = 0; c.diffArmed = 1; Boss5Logic::damageToPlayer(c, g2, m2); CHECK(c.diffArmed == 0 && c.difficulty == 3);
        c.target = -1; m2.playerDamaged = -1; Boss5Logic::damageToPlayer(c, g2, m2); CHECK(m2.playerDamaged == -1); }
    // SelectAttackTarget
    {   Mock m2; Boss5State c; Boss5Globals g2;
        g2.gameLevel = 0; g2.lastHitTarget = 1; CHECK(Boss5Logic::selectAttackTarget(c, g2, m2) && c.target == 1 && g2.targetBusy[1] == 1);
        g2.gameLevel = 1; m2.rnd = 3; g2.targetBusy[0] = g2.targetBusy[1] = 0;
        CHECK(Boss5Logic::selectAttackTarget(c, g2, m2) && c.target == 1);                               // rand&1 = 1, libre
        CHECK(Boss5Logic::selectAttackTarget(c, g2, m2) && c.target == 0 && g2.targetBusy[0] == 1);       // 1 ocupado -> 0
        CHECK(Boss5Logic::selectAttackTarget(c, g2, m2) && c.target == 0 && g2.targetBusy[0] == 1 && g2.targetBusy[1] == 0);   // ambos ocupados -> se borran y queda 0 (luego marca 0)
        m2.alive = false; CHECK(!Boss5Logic::selectAttackTarget(c, g2, m2) && c.target == -1);
        g2.gameLevel = 2; c.target = 1; m2.alive = true; CHECK(Boss5Logic::selectAttackTarget(c, g2, m2) && c.target == 1); }
    // DecNow*
    {   Boss5State c; Boss5Globals g2; g2.enemiesAlive = 3; g2.enemiesLeft = 4;
        Boss5Logic::decNowEnemyAlvNumChk(c, g2); Boss5Logic::decNowEnemyAlvNumChk(c, g2); CHECK(g2.enemiesAlive == 2 && (c.f10 & 2));
        Boss5Logic::decNowEnemyNumChk(c, g2); Boss5Logic::decNowEnemyNumChk(c, g2); CHECK(g2.enemiesLeft == 3 && (c.f10 & 4)); }
}

static void testMisc() {
    Mock m; Boss5State b; m.st = &b;
    // MotSupSet2 / CheckSwimMotion
    b.motion = 5; Boss5Logic::motSupSet2(b, m, 0x1DC, 3, 7); CHECK(m.lastMotion == 0x1DC && m.lastStart == 3 && m.lastBlend == 7 && (b.f1E40 & FB_SWIM));
    m.lastMotion = -1; Boss5Logic::motSupSet2(b, m, 0x1DC, 3, 7); CHECK(m.lastMotion == -1 && (b.f1E40 & FB_SWIM));   // mismo movimiento: no relanza
    Boss5Logic::motSupSet2(b, m, 9, 0, 0); CHECK(!(b.f1E40 & FB_SWIM));
    b.motion = 0x1D8; Boss5Logic::checkSwimMotion(b); CHECK(b.f1E40 & FB_SWIM);
    // ControlAlpha
    b.alpha = 0.0f; b.alphaStep = 0.25f; b.f1E40 = 0; for (int i = 0; i < 3; i++) Boss5Logic::controlAlpha(b); CHECK(b.alpha == 0.75f);
    Boss5Logic::controlAlpha(b); CHECK(b.alpha == 1.0f); Boss5Logic::controlAlpha(b); CHECK(b.alpha == 1.0f);
    b.f1E40 = FB_ALPHA_OUT; b.alpha = 0.1f; Boss5Logic::controlAlpha(b); CHECK(b.alpha == 0.0f); Boss5Logic::controlAlpha(b); CHECK(b.alpha == 0.0f);
    // Enable/Disable
    b.f1E40 = 0; b.f0C = 0; m.hooks.clear(); Boss5Logic::disableCollision(b, m); CHECK((b.f0C & F0C_COLLISION_OFF) && hooked(m, H_DISABLE_PART_COLLISION));
    m.hooks.clear(); Boss5Logic::enableCollision(b, m); CHECK(!(b.f0C & F0C_COLLISION_OFF) && hooked(m, H_ENABLE_PART_COLLISION));
    b.f1E40 = FB_NO_PART_COLLISION; m.hooks.clear(); Boss5Logic::disableCollision(b, m); CHECK(m.hooks.empty() && (b.f0C & F0C_COLLISION_OFF));
    // FallCheck (gravedad 0xBD20902E)
    {   Boss5State c; c.pos[1] = 10; c.f10 = 0x40; Boss5Logic::fallCheck(c, 0.0f);
        float gr; uint32_t u = 0xBD20902E; memcpy(&gr, &u, 4);
        CHECK(c.accelY == gr && c.velY == gr && c.pos[1] == 10 + gr && c.fallStep == 1);
        for (int i = 0; i < 1000 && (c.f10 & 0x40); i++) Boss5Logic::fallCheck(c, 0.0f);
        CHECK(!(c.f10 & 0x40) && c.pos[1] == 0.0f && c.velY == 0 && c.accelY == 0 && c.fallStep == 0);
        Boss5State d; d.f10 = 0; d.pos[1] = 5; Boss5Logic::fallCheck(d, 0.0f); CHECK(d.pos[1] == 5 && d.fallStep == 0); }
    // SetHeight
    {   Mock m2; Boss5State c; m2.ground = 7.0f; c.heightOffset = 2.0f; c.pos[1] = 100; c.motion = 5;
        Boss5Logic::setHeight(c, m2); CHECK(c.pos[1] == 9.0f);
        c.f10 = 0x40; c.pos[1] = 100; Boss5Logic::setHeight(c, m2); CHECK(c.pos[1] == 100.0f);        // cayendo: no toca
        c.f10 = 0; c.motion = 0x17C; c.pos[1] = 9.0f; Boss5Logic::setHeight(c, m2); CHECK(c.pos[1] == 9.0f && !(c.f10 & 0x40));
        c.pos[1] = 50; c.fallStep = 2; Boss5Logic::setHeight(c, m2); CHECK((c.f10 & 0x40) && c.fallStep == 0 && c.pos[1] == 50.0f); }
    // CharInit
    {   Mock m2; Boss5State c; Boss5Globals g2; c.kind = K_BATTLE2; c.f0C = 0x10; c.f1E40 = 0;
        Boss5Logic::charInit(c, g2, m2, 1234);
        CHECK(hooked(m2, H_CHAR_INIT_CLEAR) && c.f0C == (0x10u | 0xC8600u) && c.f1E40 == 0x80u && c.seqTab == kSeqTab1 && c.pc == kScript + kSeqTab1[0]);
        CHECK(g2.bossLife == 1234 && g2.bossLifeMax == 1234 && c.kind74 == 1 && c.state == 0 && c.hitCounter == 1 && c.hitsPerCount == 1);
        Mock m3; Boss5State d; Boss5Globals g3; d.kind = K_BATTLE1; Boss5Logic::charInit(d, g3, m3, 9999);
        CHECK(d.f0C == 0xE8600u && d.f1E40 == 0x4000u && d.seqTab == kSeqTab0 && g3.bossLife == 100 && d.alpha == 0.0f && d.alphaStep == 0.0200000014f && d.kind74 == 0); }
    // ETurn / AngChk (mismos casos que el modelo del jefe 4: la logica del asm es igual)
    CHECK(Boss5Logic::eTurn(0, 0x4000, 0x100) == 0x100);
    CHECK(Boss5Logic::eTurn(0x4000, 0, 0x100) == 0x3F00);
    CHECK(Boss5Logic::eTurn(0, 0xC000, 0x100) == -0x100);
    CHECK(Boss5Logic::eTurn(0x1000, 0x1050, 0x100) == 0x1050);
    CHECK(Boss5Logic::angChk(0x1000, 0x1010, 0x20) && !Boss5Logic::angChk(0x1000, 0x2000, 0x20) && Boss5Logic::angChk(0xFFF0, 0x0010, 0x40));
    // ControlWaterPillar
    {   Mock m2; Boss5State c; Boss5Globals g2; g2.gameLevel = 1; c.difficulty = 3; c.f1E40 = FB_PILLARS;
        Boss5Logic::controlWaterPillar(c, g2, m2);
        CHECK(c.pillarState == 1 && c.pillarBase == kPillarSpeed[1][3] && c.pillarTarget == c.pillarBase && countHook(m2, H_PILLAR_START) == 6 && c.pillarRise == 0x15 && c.pillarPhase == 0);
        Boss5Logic::controlWaterPillar(c, g2, m2); CHECK(c.pillarPhase == 0x15 && c.pillarRise == 0x2A);
        for (int i = 0; i < 100; i++) Boss5Logic::controlWaterPillar(c, g2, m2); CHECK(c.pillarRise == 0x28F);
        c.pillarTarget = c.pillarBase + 10.0f; float v0 = c.pillarBase; Boss5Logic::controlWaterPillar(c, g2, m2);
        CHECK(c.pillarAcc == 0.01f * 10.0f && c.pillarVel == (0.0f + 0.01f * 10.0f) * 0.99f && c.pillarBase == v0 + c.pillarVel);
        c.f1E40 = 0; c.pillarState = 1; Boss5Logic::controlWaterPillar(c, g2, m2); CHECK(c.pillarState == 2);
        c.f1E40 = FB_PILLARS; m2.hooks.clear(); Boss5Logic::controlWaterPillar(c, g2, m2); CHECK(c.pillarState == 0 && countHook(m2, H_PILLAR_START) == 6);
        c.f1E40 = FB_PILLARS; c.pillarState = 0; Boss5Logic::controlWaterPillar(c, g2, m2); c.f1E40 = FB_PILLARS; c.pillarState = 2;
        c.f1E40 = 0; Boss5Logic::controlWaterPillar(c, g2, m2); CHECK(c.pillarState == 0 && !(c.f1E40 & FB_PILLARS));
        c.pillarState = 0; Boss5Logic::controlWaterPillar(c, g2, m2); CHECK(c.pillarState == 2); }
    // proyectiles de hielo
    {   Mock m2; Boss5State o; IceState s; o.difficulty = 6; o.diffDown = 2; s.homeDist = 10; s.target = 1; m2.dmgResult = 1;
        IceLogic::checkAttackEnd(s, o, m2, 14.0f); CHECK(s.finished == 1 && m2.playerDamaged == 1 && m2.dmgGiven == 7 && o.difficulty == 4);   // 14 < 15
        s.finished = 0; IceLogic::checkAttackEnd(s, o, m2, 15.0f); CHECK(s.finished == 0);
        IceLogic::checkAttackEnd(s, o, m2, 200.0f); CHECK(s.finished == 0); IceLogic::checkAttackEnd(s, o, m2, 200.5f); CHECK(s.finished == 1);
        s.finished = 0; s.target = -1; o.diffArmed = 1; m2.playerDamaged = -1; IceLogic::checkAttackEnd(s, o, m2, 1.0f); CHECK(s.finished == 1 && m2.playerDamaged == -1 && o.diffArmed == 1);
        s.target = 0; m2.dmgResult = 0; IceLogic::checkAttackEnd(s, o, m2, 1.0f); CHECK(o.diffArmed == 0);
        IceState t; t.vel[0] = 1; t.vel[1] = 2; t.vel[2] = 3; t.acc[0] = .5f; t.acc[1] = 0; t.acc[2] = -1; t.homeDist = 0; t.target = -1;
        IceLogic::attackChaku(t, o, m2, 100.0f); CHECK(t.vel[0] == 1.5f && t.vel[2] == 2.0f && t.pos[0] == 1.5f && t.pos[1] == 2.0f && t.pos[2] == 2.0f && t.finished == 0);
        IceLogic::attackSphere(t, o, m2, 100.0f); CHECK(t.pos[0] == 3.0f && t.pos[2] == 4.0f);
        IceLogic::attackTagger(t, o, m2, 500.0f); CHECK(t.pos[0] == 4.5f && t.finished == 1);
        IceState u; u.pos[1] = 1; u.vel[1] = 0.5f; u.baseY = 2; IceLogic::upShape(u); CHECK(u.shapeState == 0 && u.pos[1] == 1.5f);
        IceLogic::upShape(u); IceLogic::upShape(u); CHECK(u.shapeState == 4 && u.pos[1] == 2.0f);
        IceState w; w.morphTimer = 25; IceLogic::morph(w); CHECK(w.morph[0] == 1 && w.morph[1] == 0 && w.morph[2] == 1 && w.morph[3] == 0);
        w.morphTimer = 50; IceLogic::morph(w); CHECK(w.morph[1] == 0.5f && w.morph[0] == 0.5f);
        w.morphTimer = 200; IceLogic::morph(w); CHECK(w.morph[1] == 1.0f && w.morph[0] == 0.0f);
        w.morphTimer = 0; IceLogic::morph(w); CHECK(w.morph[1] == 0.0f); }
}

int main() {
    testTables(); testVM(); testDifficulty(); testDamage(); testAttack(); testMisc();
    printf("boss5_test: %d comprobaciones, %d fallos\n", checks, fails);
    return fails ? 1 : 0;
}

// Tests de game/bosses_b/boss6.* (Boss6Task, overlay bs6mod). Los valores esperados salen de asm/ELF/BIN leidos a mano
// (ver comentarios), no de la salida del codigo. Ejecutar desde la raiz del repo (las rutas son relativas).
// Compilar: c++ -std=c++17 -Wall -Wextra game/test/boss6_test.cpp game/bosses_b/boss6.cpp game/bosses_b/boss6_data.cpp -o build/verify_boss6/bin/boss6_test
#include "../bosses_b/boss6.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
using namespace vn::boss6;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

struct Mock : Boss6Env {
    float cam = 0; int trackId = 7; int rnd = 0; float rndf = 0.5f; int motLen = 100; int pathEnd = 123;
    int dmgResult = 0; float scale = 1.0f; bool alive = true; int stat8 = 0;
    std::vector<int> hooks; std::vector<int> sounds; std::vector<std::pair<int, int>> registered; std::vector<std::pair<int, int>> stats;
    int lastMotion = -1, lastBlend = -1, lastMode = -1, camMode = -1; int maskCount = 0; Boss6State* st = nullptr;
    float camFrame() override { return cam; }
    int camTrackId() override { return trackId; }
    void setCamMode(int v) override { camMode = v; }
    int motionLen(int) override { return motLen; }
    int rand() override { return rnd; }
    float randFloat() override { return rndf; }
    void startMotion(int id) override { lastMotion = id; lastBlend = -1; lastMode = 0; if (st) st->motion = id; }
    void startMotionBlend(int id, int a, int mode) override { lastMotion = id; lastBlend = a; lastMode = mode; if (st) st->motion = id; }
    void spawnEffect(const float*, int) override {}
    void sound(int id, int, int) override { sounds.push_back(id); }
    int statFunc(int a, int b) override { stats.push_back({a, b}); return b == 8 ? stat8 : 0; }
    float damageScale(int) override { return scale; }
    void registerHit(int s, int p) override { registered.push_back({s, p}); }
    bool playerAlive(int) override { return alive; }
    int damagePlayer(int, int, int) override { return dmgResult; }
    int pathEndFrame(int) override { return pathEnd; }
    void spawnMask(const float*, int) override { maskCount++; }
    void hook(int w, int, int, int) override { hooks.push_back(w); }
};
static bool hooked(const Mock& m, int h) { for (int x : m.hooks) if (x == h) return true; return false; }
static bool soundHeard(const Mock& m, int id) { for (int x : m.sounds) if (x == id) return true; return false; }

// ---- lectura de los binarios reales
static std::vector<unsigned char> slurp(const char* p) {
    std::vector<unsigned char> v; FILE* f = fopen(p, "rb"); if (!f) return v;
    unsigned char buf[4096]; size_t n; while ((n = fread(buf, 1, sizeof buf, f)) > 0) v.insert(v.end(), buf, buf + n);
    fclose(f); return v;
}
static const unsigned kBase = 0x9C4800;
static std::vector<unsigned char> BIN, ELF;
static int32_t i32(unsigned a) { int32_t v; memcpy(&v, &BIN[a - kBase], 4); return v; }
static int16_t i16(unsigned a) { int16_t v; memcpy(&v, &BIN[a - kBase], 2); return v; }
static int8_t i8(unsigned a) { return (int8_t)BIN[a - kBase]; }
static float f32(unsigned a) { float v; memcpy(&v, &BIN[a - kBase], 4); return v; }

static void testTables() {
    // valores sueltos leidos del BIN a mano (volcado con python) -- no dependen de los ficheros en ejecucion
    CHECK(motionRow(0).id == 562 && motionRow(0).a == 564 && motionRow(13).id == 523 && motionRow(52).id == 560 && motionRow(52).a == 616 && motionRow(52).b == 640);
    CHECK(attackRow(0).motion == 16 && attackRow(0).endFrame == 460 && attackRow(0).dmgFrame == 450 && attackRow(0).f8 == -1 && attackRow(0).kind == 0);
    CHECK(attackRow(16).kind == 4 && attackRow(26).kind == 8 && attackRow(2).kind == 1);
    CHECK(kBulletNum[0] == 5 && kBulletNum[8] == 7 && kBulletNum[29] == 9);
    CHECK(kSpeedMul[0] == 1.2f && kSpeedMul[2] == 1.0f && kSpeedMul[33] == 0.5f);
    CHECK(kDmgPerHit[0] == 300 && kDmgPerHit[16] == 140 && kDmgPerHit[33] == 100);
    CHECK(kLifeRows_flat[0] == 21000.0f && kLifeRows_flat[3] == 12250.0f && kLifeRows_flat[4] == 3500.0f && kLifeRows_flat[5] == 0.0f && kLifeRows_flat[16 * 6] == 39000.0f);
    CHECK(kPoseA_flat[0].p[1] == 231.0f && kPoseA_flat[0].p[2] == -163.5f && std::fabs(kPoseD_flat[0].p[2] + 257.74f) < 0.001f && kPoseD_flat[0].p[1] == 8.0f);
    CHECK(kHitCount0_flat[0] == 1 && kHitCount0_flat[1] == 6 && kHitCount1_flat[0] == 2 && kHitCount1_flat[1] == 1);
    CHECK(kSoundIds[1] == 0x1D0012 - 0x1D0012 + 1900570);   // D_009D3B10[1]
    CHECK(kSeqTabAddr[0] == 0x9D3E80 && kSeqTabAddr[1] == 0x9D4150 && kSeqTabAddr[2] == 0x9D4330);
    CHECK(vmRead32(0x9D3E80) == 0x9D3B28 && vmRead32(0x9D3E84) == 0x9D3B40 && vmRead32(0x9D4150 + 8 * 4) == 0x2D880C);
    CHECK(vmRead16(0x2D880C) == 48 && vmRead16(0x2D880E) == 54);   // script centinela del ELF: {48 (estado), 54 (espera)}

    if (BIN.empty() || ELF.empty()) { printf("boss6_test: ficheros disc/TBL/BS6MOD.BIN o baserom/SCES_504.11 ausentes; se omite la comparacion con binarios\n"); return; }
    int bad = 0;
    for (int i = 0; i < 54 * 3; i++) if (kMotionTab_flat[i] != i32(0x9D2E40 + i * 4)) bad++;
    for (int i = 0; i < 28 * 6; i++) if (kAttackTab_flat[i] != i16(0x9D5000 + i * 2)) bad++;
    for (int i = 0; i < 36; i++) { if (kBulletNum[i] != i8(0x9D30D0 + i)) bad++; if (kSpeedMul[i] != f32(0x9D3100 + i * 4)) bad++; }
    for (int i = 0; i < 40; i++) if (kDmgPerHit[i] != i16(0x9D3440 + i * 2)) bad++;
    for (int r = 0; r < 3; r++) for (int i = 0; i < 68; i++) { if (kHitCount0_flat[r * 68 + i] != i8(0x9D3190 + r * 68 + i)) bad++; if (kHitCount2_flat[r * 68 + i] != i8(0x9D3370 + r * 68 + i)) bad++; }
    for (int r = 0; r < 4; r++) for (int i = 0; i < 68; i++) if (kHitCount1_flat[r * 68 + i] != i8(0x9D3260 + r * 68 + i)) bad++;
    for (int i = 0; i < 17 * 6; i++) if (kLifeRows_flat[i] != f32(0x9D3490 + i * 4)) bad++;
    for (int i = 0; i < 12; i++) for (int k = 0; k < 3; k++) { if (kPoseA_flat[i].p[k] != f32(0x9D3630 + i * 24 + k * 4)) bad++; if (kPoseA_flat[i].r[k] != i32(0x9D3630 + i * 24 + 12 + k * 4)) bad++; }
    for (int i = 0; i < 16; i++) for (int k = 0; k < 3; k++) { if (kPoseB_flat[i].p[k] != f32(0x9D3750 + i * 24 + k * 4)) bad++; if (kPoseB_flat[i].r[k] != i32(0x9D3750 + i * 24 + 12 + k * 4)) bad++; }
    for (int i = 0; i < 20; i++) for (int k = 0; k < 3; k++) { if (kPoseC_flat[i].p[k] != f32(0x9D38D0 + i * 24 + k * 4)) bad++; if (kPoseC_flat[i].r[k] != i32(0x9D38D0 + i * 24 + 12 + k * 4)) bad++; }
    for (int i = 0; i < 4; i++) for (int k = 0; k < 3; k++) { if (kPoseD_flat[i].p[k] != f32(0x9D3AB0 + i * 24 + k * 4)) bad++; if (kPoseD_flat[i].r[k] != i32(0x9D3AB0 + i * 24 + 12 + k * 4)) bad++; }
    for (int i = 0; i < 6; i++) if (kSoundIds[i] != i32(0x9D3B10 + i * 4)) bad++;
    for (int i = 0; i < 90; i++) { float a = kLight_flat[i], c = f32(0x9D430C + i * 4); if (!(a == c || (std::isnan(a) && std::isnan(c)))) bad++; }
    for (unsigned i = 0; i < kOvlImageLen; i++) if (kOvlImage[i] != i16(kOvlImageBase + i * 2)) bad++;
    for (int i = 0; i < 3; i++) if ((int32_t)kSeqTabAddr[i] != i32(0x9D4350 + i * 4)) bad++;
    const unsigned eo = 0x2D8800 - 0x140000 + 0x800;
    for (int i = 0; i < 16; i++) { int16_t v; memcpy(&v, &ELF[eo + i * 2], 2); if (kElfImage[i] != v) bad++; }
    CHECK(bad == 0);
    if (bad) printf("  %d valores de tabla difieren de los binarios\n", bad);
}

static void testDifficulty() {
    Boss6Globals g;
    Boss6Logic::initBS6Difficulty(g, 3, 5);
    CHECK(g.diff[0] == 5 && g.diff[1] == 5 && g.diffMirror == 5 && g.lifeRowIdx == 3);
    g.gameMode = 0;
    CHECK(Boss6Logic::incBS6Difficulty(g, 0, 2, 7) == 7 && g.diff[1] == 7);       // modo normal suma d1
    g.gameMode = 2;
    CHECK(Boss6Logic::incBS6Difficulty(g, 0, 2, 7) == 14);                         // modo 2 suma d2
    CHECK(Boss6Logic::incBS6Difficulty(g, 0, 2, 7) == 16 && g.diff[1] == 16);      // tope 16
    CHECK(Boss6Logic::decBS6Difficulty(g, 0, 1, 4) == 12 && g.diffMirror == 12);
    g.gameMode = 0;
    g.diff[0] = 1;
    CHECK(Boss6Logic::decBS6Difficulty(g, 0, 3, 9) == 0 && g.diff[1] == 9);        // suelo 0 por jugador
    g.diff[0] = 3; g.diff[1] = 4;
    g.playersMode = 2; CHECK(Boss6Logic::getBS6Difficulty(g, 0) == 3 + 0x11);       // (3+4)/2 + 0x11
    g.playersMode = 1; g.lastHitTarget = 1; CHECK(Boss6Logic::getBS6Difficulty(g, 0) == 4);
    g.lastHitTarget = 0; CHECK(Boss6Logic::getBS6Difficulty(g, 0) == 3 && g.diffMirror == 3);
    g.playersMode = 0; CHECK(Boss6Logic::getBS6Difficulty(g, 0) == 0);
}

static void testLife() {
    Boss6State b; Boss6Globals g; Mock m;
    g.lifeInitFlag = 1;
    Boss6Logic::initLife(b, g, m, 20000);
    CHECK(g.bossLife == 20000 && g.bossLifeMax == 20000 && g.lifeInitFlag == 0 && b.gauge[0] && b.gauge[1] && b.lifeRatio == 1.0f && b.hitRatio == 1.0f);
    Boss6Logic::initLife(b, g, m, 5);                       // ya inicializada: no cambia la vida
    CHECK(g.bossLife == 20000);
    g.bossLife = 15000;
    b.hitCounter = 2; b.hitCounterMax = 4; b.phaseHits = 3; b.phaseHitsMax = 6;
    Boss6Logic::controlDispLife(b, g, m);
    CHECK(std::fabs(b.lifeRatio - 0.75f) < 1e-6f);
    CHECK(std::fabs(b.hitRatio - (2.0 / 6.0 + (2.0 / 4.0) / 6.0)) < 1e-5);
    CHECK(b.blinkTimer == 0 && !(b.f1DA4 & FA_LIFE_BLINK));
    b.f1DA4 |= FA_ATTACK;
    Boss6Logic::controlDispLife(b, g, m);
    CHECK(b.blinkTimer == 0x31 && (b.f1DA4 & FA_LIFE_BLINK));
    b.f1DA4 = FA_DEAD; g.bossLife = -50;
    Boss6Logic::controlDispLife(b, g, m);
    CHECK(b.hitRatio == 0.0f && b.lifeRatio == 0.0f);
}

static void testDamage() {
    Boss6Globals g; g.playersMode = 1; g.lifeRowIdx = 0;   // fila 0: umbrales 12250 / 3500 / 0
    // --- tipo 0: cruza el umbral 12250 -> Damage + tabla 0xC
    {
        Mock m; Boss6State b; b.kind = 0; m.st = &b;
        Boss6Logic::seqInit(b, kSeqTabAddr[0]);
        g.bossLife = 12500; g.enemiesAlive = 3;
        Boss6Logic::weakHit(b, g, m, 1, 0);                     // dano = (int)(1.0 * kDmgPerHit[0]) = 300 -> 12200
        CHECK(g.bossLife == 12200);
        CHECK(b.f1DA4 & FA_DEAD); CHECK(b.f1DA4 & FA_IN_DAMAGE_ANIM); CHECK(b.f0C & F0C_IN_DAMAGE);
        CHECK(b.tabIdx == 0xC && b.pc == vmRead32(kSeqTabAddr[0] + 0xC * 4));
        CHECK((b.f10 & 2) && g.enemiesAlive == 2);
        CHECK(m.stats.size() == 1 && m.stats[0].first == 2 && m.stats[0].second == 0xE);
        CHECK(b.rate == 1.2f && b.rate2 == 1.2f && m.camMode == 1 && soundHeard(m, 0x130016));
        CHECK(b.curMotionIdx == 52 && m.lastMotion == 560 && m.lastBlend == 0 && m.lastMode == 5);   // typeId != 0x25 -> movimiento 0x34
        CHECK(hooked(m, H_HIT_MARK));
    }
    // --- tipo 1: umbral 3500 -> tabla 7, sin llamadas a statFunc
    {
        Mock m; Boss6State b; b.kind = 1; m.st = &b;
        Boss6Logic::seqInit(b, kSeqTabAddr[1]);
        g.bossLife = 3700; g.enemiesAlive = 3; g.deathCount73F8 = 0;
        Boss6Logic::weakHit(b, g, m, 1, 0);
        CHECK(g.bossLife == 3400 && b.tabIdx == 7 && (b.f1DA4 & FA_DEAD) && m.stats.empty());
        CHECK(b.light == 0);                                     // Damage de tipo 1 -> setLightColor(5) con luz 0 -> indice 0
    }
    // --- tipo 2: umbral 0 -> vida a 0, tabla 4, 3 llamadas de estadistica
    {
        Mock m; Boss6State b; b.kind = 2; m.st = &b;
        Boss6Logic::seqInit(b, kSeqTabAddr[2]);
        g.bossLife = 100; g.g72D0 = 7; g.deathCount73F8 = 0; g.deathCount73FC = 0; g.gameState = 0; g.gameKind = 0;
        Boss6Logic::weakHit(b, g, m, 2, 1);
        CHECK(g.bossLife == 0 && b.tabIdx == 4 && g.g72D0 == 5);
        CHECK(m.registered.size() == 1 && m.registered[0].first == 1 && m.registered[0].second == 0x9C4);
        CHECK(m.stats.size() == 3 && m.stats[0].second == 8 && m.stats[1].second == 9 && m.stats[2].second == 0xF);
        CHECK(hooked(m, H_HIT_MARK0));
    }
    // --- tipo 2 con statFunc(2,8) != 0: no llama a (2,9)
    {
        Mock m; Boss6State b; b.kind = 2; m.st = &b; m.stat8 = 1;
        Boss6Logic::seqInit(b, kSeqTabAddr[2]);
        g.bossLife = 100;
        Boss6Logic::weakHit(b, g, m, 2, 0);
        CHECK(m.stats.size() == 2 && m.stats[0].second == 8 && m.stats[1].second == 0xF);
    }
    // --- modo especial (0x500F14 == 0xA y gp-0x73B4 == 2): con vida > 0 bajo el umbral solo cuenta como golpe suelto
    {
        Mock m; Boss6State b; b.kind = 0; m.st = &b; b.hitCounter = 3; b.hitCounterMax = 3;
        g.gameState = 0xA; g.gameKind = 2; g.bossLife = 12000;
        Boss6Logic::weakHit(b, g, m, 1, 0);
        CHECK(g.bossLife == 11700 && !(b.f1DA4 & FA_DEAD) && b.hitCounter == 2 && !hooked(m, H_HIT_MARK));
        g.gameState = 0; g.gameKind = 0;
    }
    // --- golpe suelto sin ataque: tras agotar hitCounter -> DamageUpperBody (destello, rec[], sonido 0x130014)
    {
        Mock m; Boss6State b; b.kind = 0; m.st = &b; b.hitCounter = 1; b.typeId = 0;
        g.bossLife = 20000;
        Boss6Logic::weakHit(b, g, m, 1, 0);
        CHECK(g.bossLife == 19700 && (b.f1DA4 & FA_HITFLASH) && soundHeard(m, 0x130014) && m.registered.size() == 1 && m.registered[0].second == 0xA);
        CHECK(b.f1E24 == 3.75f && b.f1E1C == 3.75f && b.f1E2C == 1.0f && b.rec[0].a == 5 && b.rec[1].b == 5);   // typeId != 0x25 -> fila 0x32
        CHECK(b.rec[0].cur == motionRow(0x32).a && b.rec[1].cur == motionRow(0x32).b);
    }
    // --- golpe suelto con ataque en curso: la barra de fase se vacia y sube la dificultad
    {
        Mock m; Boss6State b; b.kind = 0; m.st = &b; b.f1DA4 = FA_ATTACK; b.hitCounter = 1; b.hitCounterMax = 2; b.phaseHits = 1;
        Boss6Globals g2; g2.playersMode = 1; g2.bossLife = 20000; g2.gameMode = 0; g2.diff[0] = 3; g2.diff[1] = 3;
        Boss6Logic::weakHit(b, g2, m, 1, 0);
        CHECK(b.hitCounter == 2 && b.phaseHits == 0);              // 1E4B reiniciado a 1E4D; 1F4C llega a 0 -> Damage
        CHECK(b.f0C & F0C_IN_DAMAGE);
        CHECK(g2.diff[0] == 4 && g2.diff[1] == 4);                  // IncBS6Difficulty(1,1)
        CHECK(m.registered.size() == 1 && m.registered[0].second == 0xA);
    }
    // --- ventanas que anulan WeakHit
    {
        Mock m; Boss6State b; b.f0C = 0x8000; g.bossLife = 1000;
        Boss6Logic::weakHit(b, g, m, 1, 0); CHECK(g.bossLife == 1000);
        b.f0C = 0; b.f1DA4 = FA_DEAD;
        Boss6Logic::weakHit(b, g, m, 1, 0); CHECK(g.bossLife == 1000);
    }
    // --- CheckWeakHit / ClearWeakHit / CheckDamage
    {
        Boss6State b;
        b.kind = 0; CHECK(Boss6Logic::checkWeakHit(b)); b.kind = 2; CHECK(Boss6Logic::checkWeakHit(b)); b.kind = 3; CHECK(!Boss6Logic::checkWeakHit(b));
        b.partPresent[0] = b.partPresent[2] = true; b.partF0C[0] = b.partF0C[1] = b.partF0C[2] = 0xFFFFFFFFu;
        Boss6Logic::clearWeakHit(b);
        CHECK(b.partF0C[0] == 0xBFFFFFFFu && b.partF0C[1] == 0xFFFFFFFFu && b.partF0C[2] == 0xBFFFFFFFu);
    }
    {
        Mock m; Boss6State b; b.kind = 3; m.st = &b; Boss6Globals g3; g3.bossLife = 20000;
        Boss6Logic::HitInfo h[2] = {{-1, 0, 0}, {1, 2, 5}};
        b.f0C = F0C_HIT;                                           // kind 3 y sin F0C_WEAK -> NoWeakHit, sin WeakHit
        Boss6Logic::checkDamage(b, g3, m, h);
        CHECK(g3.bossLife == 20000 && !hooked(m, H_HIT_CONSUMED) && hooked(m, H_HIT_PREPARE) && b.weakIdx == 5);
        b.f0C = F0C_HIT | F0C_WEAK; m.hooks.clear();               // con punto debil -> WeakHit + H_HIT_CONSUMED
        g3.playersMode = 1;
        Boss6Logic::checkDamage(b, g3, m, h);
        CHECK(g3.bossLife == 20000 - 300 && hooked(m, H_HIT_CONSUMED));
        b.f0C = F0C_HIT | 0x8100; m.hooks.clear(); g3.bossLife = 20000;   // ventana cerrada: no hay dano
        Boss6Logic::checkDamage(b, g3, m, h);
        CHECK(g3.bossLife == 20000 && !hooked(m, H_HIT_CONSUMED));
        // golpes de murcielagos (tipo 0): bits 1 y 2 de +0x1F9C
        Boss6State b0; b0.kind = 0; b0.hitFlags = 3; Boss6Globals g4; g4.bossLife = 20000; g4.playersMode = 1;
        Boss6Logic::checkDamage(b0, g4, m, h);
        CHECK(g4.bossLife == 20000 - 300 && b0.hitFlags == 0);   // el primer golpe suelto activa el destello (mascara 0x4C020) y el segundo no resta
    }
    // --- batKillAdd
    {
        Boss6State b; Boss6Logic::batKillAdd(b, 2); CHECK(b.hitFlags == 0);
        b.f1DA4 = FA_ATTACK | FA_ATK_200000; Boss6Logic::batKillAdd(b, 2); Boss6Logic::batKillAdd(b, 1); CHECK(b.hitFlags == 3);
    }
    // --- SetDamageMotion tipo 0x25 forma 2, CheckDamageEnd
    {
        Mock m; Boss6State b; m.st = &b; b.typeId = 0x25; b.f1DA4 = FA_FORM2 | FA_PATH | FA_CAMPATH;
        Boss6Logic::setDamageMotion(b, m);
        CHECK(m.lastMotion == motionRow(0x33).id && !(b.f1DA4 & (FA_PATH | FA_CAMPATH)) && (b.f1DA4 & FA_PATH_DONE) && hooked(m, H_DAMAGE_MOTION_GEOMETRY));
        Mock m2; Boss6State c; m2.st = &c; c.typeId = 0x25;
        Boss6Logic::setDamageMotion(c, m2);
        CHECK(m2.lastMotion == motionRow(0x30).id);
        // CheckDamageEnd: destello
        Mock m3; Boss6State d; d.f1DA4 = FA_HITFLASH; d.f40 = 29.0f; d.motionFrame = 7.0f; d.rec[0].prev = 11; d.rec[1].prev = 22;
        Boss6Logic::checkDamageEnd(d, m3);
        CHECK(d.f1DA4 & FA_HITFLASH);                                  // 29 < 30: sigue
        d.f40 = 30.0f; Boss6Logic::checkDamageEnd(d, m3);
        CHECK(!(d.f1DA4 & FA_HITFLASH) && d.rec[0].cur == 11 && d.rec[1].cur == 22 && d.rec[0].a == 17 && d.rec[0].b == 10);
        // fin de la animacion de dano: tipo != 0x25, kind 1 -> rate 2.4
        Boss6State e; e.kind = 1; e.f1DA4 = FA_IN_DAMAGE_ANIM; e.motionFrame = 90.0f; e.rate = 1.2f; m3.motLen = 100;
        Boss6Logic::checkDamageEnd(e, m3);
        CHECK((e.f1DA4 & FA_IN_DAMAGE_ANIM) && e.rate == 1.2f);       // 91.2 < 100: sigue
        e.motionFrame = 99.0f; Boss6Logic::checkDamageEnd(e, m3);
        CHECK(!(e.f1DA4 & FA_IN_DAMAGE_ANIM) && e.rate == 2.4f && (e.f0C & F0C_COLLISION_OFF));
        Boss6State f; f.typeId = 0x25; f.f1DA4 = FA_IN_DAMAGE_ANIM | FA_FORM2; f.motionFrame = 99.0f; f.rate = 1.2f; f.parentF1C9C = 0;
        f.seqTab = kSeqTabAddr[2];
        Boss6Logic::checkDamageEnd(f, m3);
        CHECK(f.curMotionIdx == 0x18 && f.tabIdx == 5 && (f.parentF1C9C & 2) && hooked(m3, H_RESTORE_POSE_AFTER_DAMAGE));
    }
}

static void testStates() {
    Boss6Globals g; Mock m; Boss6State b;
    // --- EnableCollision / DisableCollision
    b.partPresent[1] = b.partPresent[3] = true; b.f0C = 0;
    Boss6Logic::disableCollision(b);
    CHECK((b.f0C & 0x8000) && b.partF1A8[1] == 0x40 && b.partF1A8[3] == 0x40 && b.partF1A8[0] == 0);
    Boss6Logic::enableCollision(b);
    CHECK(!(b.f0C & 0x8000) && b.partF1A8[1] == 0 && b.partF1A8[3] == 0);
    b.f1DA4 = FA_NO_PART_COLLISION; Boss6Logic::disableCollision(b);
    CHECK((b.f0C & 0x8000) && b.partF1A8[1] == 0);                    // con 0x40 de 1DA4 no toca las partes
    // --- camara
    Boss6Logic::allowCameraStart(g); CHECK(g.cameraStartAllowed == 1); Boss6Logic::forbidCameraStart(g); CHECK(g.cameraStartAllowed == 0);
    // --- contadores de enemigos
    Boss6State c; g.enemiesAlive = 5; g.enemiesLeft = 9;
    Boss6Logic::decNowEnemyAlvNumChk(c, g); Boss6Logic::decNowEnemyAlvNumChk(c, g);
    Boss6Logic::decNowEnemyNumChk(c, g); Boss6Logic::decNowEnemyNumChk(c, g);
    CHECK(g.enemiesAlive == 4 && g.enemiesLeft == 8 && (c.f10 & 6) == 6);
    // --- actDelete: dos pasos
    {
        Boss6State d; Boss6Globals g2; g2.enemiesLeft = 3; g2.targetBusy[1] = 1; d.target = 1; d.f18 = 9;
        CHECK(Boss6Logic::actDelete(d, g2, m) == 0);
        CHECK((d.f0C & 0x04004000) == 0x04004000 && (d.f10 & 0x24) == 0x24 && d.f18 == 0 && d.stateB == 1 && g2.enemiesLeft == 2 && g2.targetBusy[1] == 0 && d.target == -1);
        m.hooks.clear();
        CHECK(Boss6Logic::actDelete(d, g2, m) == 0 && hooked(m, H_FINISH_TASK));
        d.stateB = 5; m.hooks.clear(); Boss6Logic::actDelete(d, g2, m); CHECK(m.hooks.empty());
        CHECK(Boss6Logic::actNop() == 1 && Boss6Logic::actDead() == 0);
    }
    // --- FinishTask0
    {
        Mock m2; Boss6State d; Boss6Globals g2; d.kind = 2; d.gauge[0] = d.gauge[1] = true; g2.gameState = 0xA; g2.gameKind = 2; g2.enemiesAlive = 2; g2.enemiesLeft = 2;
        Boss6Logic::finishTask0(d, g2, m2);
        CHECK(m2.camMode == 1 && !d.gauge[0] && !d.gauge[1] && soundHeard(m2, 0x1D0012) && g2.enemiesAlive == 1 && g2.enemiesLeft == 1);
        Mock m3; Boss6State e; e.kind = 0; Boss6Globals g3; Boss6Logic::finishTask0(e, g3, m3); CHECK(m3.camMode == -1);
    }
}

static void testAttackSelect() {
    Boss6Globals g; Mock m; Boss6State b;
    g.playersMode = 2; m.rnd = 1;
    CHECK(Boss6Logic::selectAttackTarget(b, g, m) && b.target == 1 && g.targetBusy[1] == 1);
    CHECK(Boss6Logic::selectAttackTarget(b, g, m) && b.target == 0 && g.targetBusy[0] == 1);
    CHECK(Boss6Logic::selectAttackTarget(b, g, m) && b.target == 0 && g.targetBusy[1] == 0 && g.targetBusy[0] == 1);   // ambos ocupados: se vacian y se elige (rand+1)&1
    g.playersMode = 1; g.lastHitTarget = 1; g.targetBusy[0] = g.targetBusy[1] = 0;
    CHECK(Boss6Logic::selectAttackTarget(b, g, m) && b.target == 1 && g.targetBusy[1] == 1);
    Boss6Logic::atcPlFlgClear(b, g); CHECK(b.target == -1 && g.targetBusy[1] == 0);
    Boss6Logic::atcPlFlgClear(b, g); CHECK(b.target == -1);
    m.alive = false; g.playersMode = 1; g.lastHitTarget = 0;
    CHECK(!Boss6Logic::selectAttackTarget(b, g, m) && g.targetBusy[0] == 0);
    // SetBulletNum: ataque 0 (tipo 0) -> kBulletNum[dif]; ataque 16 (tipo 4) idem; 17 (tipo 5) -> 6; 20 (tipo 6) -> 4; 2 (tipo 1) -> 1
    g.playersMode = 1; g.diff[0] = 8;
    Boss6Logic::setBulletNum(b, g, 0); CHECK(b.bulletNum == 7);
    Boss6Logic::setBulletNum(b, g, 16); CHECK(b.bulletNum == 7);
    Boss6Logic::setBulletNum(b, g, 17); CHECK(b.bulletNum == 6);
    Boss6Logic::setBulletNum(b, g, 20); CHECK(b.bulletNum == 4);
    Boss6Logic::setBulletNum(b, g, 2); CHECK(b.bulletNum == 1);
}

static void testAttackFlow() {
    Boss6Globals g; g.playersMode = 1; g.gameMode = 2; g.diff[0] = 0; g.lastHitTarget = 0;
    Mock m; Boss6State b; m.st = &b; m.rnd = 1234;
    Boss6Logic::setBulletNum(b, g, 0);
    CHECK(b.bulletNum == 5);                                     // kBulletNum[0]
    Boss6Logic::startAttack(b, g, m, 0);
    CHECK(b.attack == 0 && b.atkStep == 0 && b.gameMode == 2 && (b.f0C & F0C_ATTACKING) && (b.f1DA4 & FA_ATTACK));
    CHECK(m.lastMotion == motionRow(16).id && m.lastMode == 0xA && b.curMotionIdx == 16);   // fila 0 de ataques: movimiento 16, mezcla 0xA
    b.rate = 1.0f;
    b.motionFrame = 0.0f; Boss6Logic::controlAttack(b, g, m);
    CHECK(b.atkStep == 0 && b.atkFlag == 0);                      // fr <= umbral (0)
    b.motionFrame = 1.0f; Boss6Logic::controlAttack(b, g, m);
    CHECK(b.atkStep == 1 && b.atkFlag == 1 && b.target == 0 && b.atkCount == 34);   // AttackDamageFrame tipo 0: rand % 100
    // paso 1: umbral = 0 + 1 * (450/6) = 75 ; dmgMul = (1*0xFFFF)/5 = 13107
    b.motionFrame = 75.0f; Boss6Logic::controlAttack(b, g, m);
    CHECK(b.atkStep == 1 && b.dmgMul == 13107);
    b.motionFrame = 76.0f; Boss6Logic::controlAttack(b, g, m);
    CHECK(b.atkStep == 2);
    for (int i = 0; i < 3; i++) { b.motionFrame += 80.0f; Boss6Logic::controlAttack(b, g, m); }   // umbrales 150, 225, 300
    CHECK(b.atkStep == 100);                                      // agotadas las 5 balas
    b.motionFrame += 80.0f; Boss6Logic::controlAttack(b, g, m); CHECK(b.atkStep == -1);   // f8 == -1
    // un ataque que cambia de movimiento externo termina con sonido de bucle apagado
    m.sounds.clear(); b.motion = 99; Boss6Logic::controlAttack(b, g, m);
    CHECK(!(b.f1DA4 & FA_ATTACK) && !(b.f0C & F0C_ATTACKING) && soundHeard(m, 0x1D0012));
    // fin por fotograma final (ataque 0 -> fila 0: endFrame 460)
    Boss6Logic::startAttack(b, g, m, 0); m.sounds.clear();
    b.motionFrame = 459.5f; b.rate = 1.0f; b.atkStep = 100;
    Boss6Logic::controlAttack(b, g, m);
    CHECK(!(b.f1DA4 & FA_ATTACK) && !(b.f0C & F0C_ATTACKING) && soundHeard(m, 0x1D0012));
    // tipo 2 (fila 2: movimiento 20, kind 1... usamos fila 26: kind 8, start 0, dmg -1... ver tabla)
    Boss6State k; Mock mk; mk.st = &k; mk.dmgResult = 1; Boss6Globals gk; gk.playersMode = 1; gk.gameMode = 0; gk.diff[0] = 5; gk.diff[1] = 5;
    Boss6Logic::startAttack(k, gk, mk, 26);                         // fila 26: (45, 300, 0, -1, 0, 8)
    CHECK(k.f1DA4 & FA_ATTACK);
    k.rate = 1.0f;
    k.motionFrame = 0.0f; Boss6Logic::controlAttack(k, gk, mk); CHECK(k.atkStep == 10);        // inicio 0: salta al paso 10
    Boss6Logic::controlAttack(k, gk, mk); CHECK(k.atkStep == -1);                                // dmgFrame == -1
    // fila 27: (46, 145, -1, 60, 70, 8): inicio -1 -> 10; dmg 60 -> ventana; f8 = 70
    Boss6State q; Mock mq; mq.st = &q; mq.dmgResult = 1; Boss6Globals gq; gq.playersMode = 1; gq.diff[0] = 5; gq.diff[1] = 5; q.target = 0;
    Boss6Logic::startAttack(q, gq, mq, 27); q.rate = 1.0f;
    q.motionFrame = 0.0f; Boss6Logic::controlAttack(q, gq, mq); CHECK(q.atkStep == 10);
    q.motionFrame = 39.5f; Boss6Logic::controlAttack(q, gq, mq); CHECK(q.atkStep == 10 && soundHeard(mq, 0x13001A));   // 39.5 < 40 y 40.5 >= 40: sonido
    q.motionFrame = 60.0f; Boss6Logic::controlAttack(q, gq, mq); CHECK(q.atkStep == 11 && (q.f1DA4 & FA_ATK_8000));
    q.motionFrame = 70.0f; Boss6Logic::controlAttack(q, gq, mq);
    CHECK(q.atkStep == -1 && !(q.f1DA4 & FA_ATK_8000));                                            // AttackDamageFrame tipo 8: damagePlayer -> Dec dificultad (2,2)
    CHECK(gq.diff[0] == 3 && gq.diff[1] == 3);
}

static void testSeqVm() {
    // script 0 del tipo 0 en el BIN: {47, 19, 12, 256, 49, ...}; script 1: {10,13,0,0, 28,249, 2,290, 26, 35, ...}
    Boss6Globals g; Mock m; Boss6State b; m.st = &b; m.pathEnd = 321;
    Boss6Logic::seqInit(b, kSeqTabAddr[0]);
    CHECK(b.pc == 0x9D3B28 && b.tabIdx == 0 && b.timer == 0 && b.sel1DC5 == 0);
    b.parentF1C9C = 2; b.f1DA4 = 0;
    Boss6Logic::execSeqCmd(b, g, m);
    CHECK(b.timer == 0);                                             // resta 1 (-> -1) y SeqCmdNextTable (op 49) lo deja a 0
    CHECK(b.f0C & 0x8000);                                           // op 47 DisableCollision
    CHECK(hooked(m, H_CAM_TRANSFORM_A) && !(b.parentF1C9C & 2));     // op 19
    CHECK(b.rate == 1.2f * (0.00390625f * 256.0f));                  // op 12: 1.2 * P/256
    CHECK(b.tabIdx == 1);                                            // op 49 -> SeqCmdNextTable
    CHECK(m.lastMotion == 523 && b.curMotionIdx == 13);              // script 1: op 10 MotSupSet2(13,0,0) -> motionRow(13).id
    CHECK((b.f1DA4 & FA_PATH) && b.pathId == 249 && b.pathEnd == 321);   // op 28
    CHECK(b.pc == 0x9D3B40 + 12);                                    // esperando en op 2 (indice 6 del script 1)
    b.motionFrame = 289.0f; Boss6Logic::execSeqCmd(b, g, m);
    CHECK(b.pc == 0x9D3B40 + 12);
    b.motionFrame = 290.0f; Boss6Logic::execSeqCmd(b, g, m);
    CHECK(hooked(m, H_SPAWN_ALLBAT) && b.pc == 0x9D3B40 + 18);       // op 26, luego espera en op 35 (PATH_DONE)
    b.f1DA4 |= FA_PATH_DONE; Boss6Logic::execSeqCmd(b, g, m);
    CHECK(b.pc != 0x9D3B40 + 18);
    // temporizador de op 7: wait con 1DC2 = P; ExecSeqCmd decrementa y vuelve mientras >= 0
    {
        Boss6State c; Mock mc; Boss6Globals gc; c.timer = 2;
        Boss6Logic::execSeqCmd(c, gc, mc);  CHECK(c.timer == 1 && c.pc == 0);
        Boss6Logic::execSeqCmd(c, gc, mc);  CHECK(c.timer == 0);
        Boss6Logic::execSeqCmd(c, gc, mc);  CHECK(c.timer == -1);   // pc == 0 -> op 0 sobre imagen vacia: espera camara
    }
    // script 2 del tipo 0 (0x9D3BA0): {45, 17, 23,19,0, 46, 23,22,1, 10,15,0,10, 4, 45, 13, 24,0, 6, ...}
    {
        Boss6State c; Mock mc; mc.st = &c; Boss6Globals gc; gc.playersMode = 1; gc.diff[0] = 0; gc.diff[1] = 0;
        c.kind = 0; c.sel1DC5 = 0; c.sel1DC4 = 0;
        Boss6Logic::seqInit(c, kSeqTabAddr[0]);
        c.f1DA4 = 0x80000; c.f0C = 0x8000; Boss6Logic::seqSetTable(c, 2);
        CHECK(c.pc == 0x9D3BA0 && c.tabIdx == 2);
        Boss6Logic::execSeqCmd(c, gc, mc);
        CHECK(c.hitCounter == 1 && c.hitCounterMax == 1 && c.phaseHits == 6 && c.phaseHitsMax == 6);   // op 45: kHitCount0[fila 0][dif 0] = (1, 6)
        CHECK(c.pos[1] == 231.0f && c.pos[2] == -163.5f && c.f1E14 == 0);                              // op 17: kPoseA[0][0]
        CHECK(!(c.f1DA4 & 0x80000) && !(c.f0C & 0x8000) && (c.f1DA4 & 0x400000));                       // op 23 (bit 19 a 0), op 46, op 23 (bit 22 a 1)
        CHECK(mc.lastMotion == motionRow(15).id && mc.lastBlend == 0 && mc.lastMode == 10);            // op 10 con blend: id 501
        CHECK(c.pc == 0x9D3BA0 + 2 * 13);                                                                // op 4 (espera fin de movimiento)
        c.motionFrame = 0.0f; c.rate = 1.0f; mc.motLen = 100; Boss6Logic::execSeqCmd(c, gc, mc);
        CHECK(c.pc == 0x9D3BA0 + 2 * 13);
        c.motionFrame = 99.0f; Boss6Logic::execSeqCmd(c, gc, mc);                                       // pasa op 4, op 45 de nuevo, op 13: (F0C_IN_DAMAGE ausente) -> rate de velocidad, op 24 ataque
        CHECK((c.f0C & F0C_ATTACKING) && c.attack == 0 && c.bulletNum == 5);                            // op 24: StartAttack(0), SetBulletNum
        CHECK(c.pc == 0x9D3BA0 + 2 * 18);                                                                // op 6 (espera fin de ataque)
        // op 13: rate = 1.2 * (1 / (speedBase * kSpeedMul[0])); kind 0, sel1DC5 0 -> speedBase 0.95
        CHECK(std::fabs(c.rate - 1.2f * (1.0f / (0.95f * 1.2f))) < 1e-6f);
    }
}

static void testPureHelpers() {
    // --- B6InterDivAngF: camino corto con factor t (leido del asm: diff = (b-a)&0xFFFF; > 32767 -> negativo)
    CHECK(Boss6Logic::b6InterDivAngF(0x1000, 0x2000, 0.5f) == 0x1800);
    CHECK(Boss6Logic::b6InterDivAngF(0x2000, 0x1000, 0.5f) == 0x1800);
    CHECK(Boss6Logic::b6InterDivAngF(0x1000, 0x1000, 0.5f) == 0x1000);
    // --- BodyAlphaControl
    Boss6State b; b.alphaCur = 0.5f; b.alphaTarget = 1.0f;
    Boss6Logic::bodyAlphaControl(b);
    CHECK(b.alphaCur == 0.5f + 0.05f * (1.0f - 0.5f) && b.alphaOut1 == b.alphaCur && b.alphaOut2 == b.alphaCur);
    b.alphaCur = 0.96f; Boss6Logic::bodyAlphaControl(b); CHECK(b.alphaCur == 1.0f);
    b.alphaCur = 0.8f; b.alphaTarget = 0.2f; Boss6Logic::bodyAlphaControl(b);
    CHECK(b.alphaCur == 0.8f + 0.03f * (0.2f - 0.8f));
    b.alphaCur = 0.2f; b.alphaTarget = 0.0f; Boss6Logic::bodyAlphaControl(b); CHECK(b.alphaCur > 0.01f);
    b.alphaCur = 0.005f; b.alphaTarget = 0.0f; Boss6Logic::bodyAlphaControl(b); CHECK(b.alphaCur == 0.01f);
    b.alphaCur = 0.3f; b.alphaTarget = 0.1f; b.f1DA4 = FA_HITFLASH; Boss6Logic::bodyAlphaControl(b);      // destello: objetivo minimo 0.5
    CHECK(b.alphaCur == 0.3f + 0.05f * (0.5f - 0.3f));
    b.f1DA4 = FA_IN_DAMAGE_ANIM; b.alphaCur = 0.4f; b.alphaTarget = 0.0f; Boss6Logic::bodyAlphaControl(b);
    CHECK(b.alphaCur == 0.4f + 0.05f * (1.0f - 0.4f));
    Boss6Logic::bodyAlphaDirectSet(b, 0.25f); CHECK(b.alphaCur == 0.25f && b.alphaTarget == 0.25f);
    Boss6Logic::bodyAlphaSupSet(b, 0.75f); CHECK(b.alphaCur == 0.25f && b.alphaTarget == 0.75f);
    // --- ramas aleatorias
    Mock m; Boss6State r; Boss6Logic::initBranchRate(r);
    bool allOne = true; for (int i = 0; i < 18; i++) if (r.branchRate[i] != 1.0f) allOne = false;
    CHECK(allOne);
    m.rndf = 0.5f;
    // al inicio b1E10 == b1E11 == 0: se excluye la rama 0
    CHECK(Boss6Logic::getRandomBranchRate(r, m, 3) == 2);               // total 2 * 0.5 = 1 -> acumulado 1 (no < 1), luego 2 -> rama 2
    CHECK(r.b1E10 == 2 && r.b1E11 == 0 && std::fabs(r.branchRate[2] - 0.33f) < 1e-6f);
    CHECK(Boss6Logic::getRandomBranchRate(r, m, 3) == 1);               // 2 != 0: sin exclusion; total 2.33 * 0.5 = 1.165 -> acumulado 1, 2 -> rama 1
    CHECK(r.b1E10 == 1 && r.b1E11 == 2);
    CHECK(Boss6Logic::getRandomBranchRate(r, m, 3) == 0);               // total 1.66 * 0.5 = 0.83 < 1 -> rama 0
    { Boss6State w; Boss6Logic::initBranchRate(w); w.b1E10 = w.b1E11 = 1; CHECK(Boss6Logic::getRandomBranchRate(w, m, 3) == 2); }   // rama 1 excluida
    m.rndf = 0.99f; Boss6State z; Boss6Logic::initBranchRate(z);
    CHECK(Boss6Logic::getRandomBranchRate(z, m, 4) == 3);
    // --- velocidad base
    Boss6State s; s.kind = 0; s.sel1DC5 = 1; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 1.52f);
    s.sel1DC5 = 0; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 0.95f);
    s.sel1DC5 = 3; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 1.0f);
    s.kind = 1; s.b1E10 = 0; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 1.4f);
    s.b1E10 = 1; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 1.35f);
    s.b1E10 = 2; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 1.0f);
    s.kind = 2; Boss6Logic::setSpeedBaseRate(s); CHECK(s.speedBase == 1.0f);
    // --- luz
    Boss6State l;
    Boss6Logic::setLightColor(l, 0);                                    // idx < 5: fila 5 (D_009D43C0)
    CHECK(l.light == 0 && l.lightColA[0] == kLight_flat[45] && l.lightColB[1] == kLight_flat[45 + 4] && l.lightColC[2] == kLight_flat[45 + 8]);
    Boss6Logic::setLightColor(l, 7);                                    // idx 7: luz actual 0 != 2 -> carga fila 7
    CHECK(l.light == 7 && l.lightColA[0] == kLight_flat[63] && l.lightCount == 0);
    for (int i = 0; i < 0x24; i++) Boss6Logic::ctrlBossLight(l);
    CHECK(l.light == 7);
    Boss6Logic::ctrlBossLight(l); CHECK(l.light == 2);                  // a los 0x25 fotogramas vuelve a idx-5
    Boss6Logic::setLightColor(l, 7); CHECK(l.light == 2);               // idx-5 == luz actual: no cambia el indice
    Boss6Logic::setLightColor(l, 5); CHECK(l.light == 2 || l.light == 5);
    Boss6State l2; Boss6Logic::setLightColor(l2, 5); CHECK(l2.light == 0);
    // --- ruta y acercamiento
    Mock mp; Boss6State p; mp.pathEnd = 77;
    p.pathId = 3; CHECK(Boss6Logic::getPathEndFrame(p, mp) == 77 && p.pathEnd == 77);
    p.pathId = -1; CHECK(Boss6Logic::getPathEndFrame(p, mp) == 1);
    p.pos[0] = 1; p.pos[1] = 2; p.pos[2] = 3; p.rot[1] = 9;
    Boss6Logic::setApproach(p, 40);
    CHECK((p.f1DA4 & FA_APPROACH) && p.apprPos[2] == 3.0f && p.apprRot[1] == 9 && p.f1EC4 == 0 && p.f1EC8 == 1.0f && p.f1ECC == 40.0f);
    // --- SeqCmd*
    Boss6State q; Boss6Logic::seqInit(q, kSeqTabAddr[1]);
    CHECK(q.pc == 0x9D3EC0 && q.tabIdx == 0);
    Boss6Logic::seqNextTable(q); CHECK(q.tabIdx == 1 && q.pc == 0x9D3F00 && q.timer == 0);
    q.timer = 5; Boss6Logic::seqSetTable(q, 7); CHECK(q.tabIdx == 7 && q.pc == 0x9D4110 && q.timer == 0);
    Mock mq; Boss6Globals gq; Boss6Logic::seqSetTable(q, 8); CHECK(q.pc == 0x2D880C);
    CHECK(Boss6Logic::actExecSeqCmd(q, gq, mq) == 0);                    // centinela del ELF: {48, 54}
    CHECK(q.state == 1 && q.stateB == 0);                                // op 48
}

int main() {
    BIN = slurp("disc/TBL/BS6MOD.BIN"); ELF = slurp("baserom/SCES_504.11");
    testTables(); testDifficulty(); testLife(); testDamage(); testStates();
    testAttackSelect(); testAttackFlow(); testSeqVm(); testPureHelpers();
    if (fails == 0) printf("boss6_test: OK\n");
    return fails ? 1 : 0;
}

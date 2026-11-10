// Tests de game/bosses_pending (funciones pendientes de jefes 3 y 4). Los valores esperados salen del asm / ELF / BIN leido a mano
// (constantes, tablas, tablas de saltos), no de la salida del codigo. Sin ejecucion real del juego: ver MANIFEST.md.
// Compilar y ejecutar (raiz del repo): ver game/bosses_pending/MANIFEST.md
#include "../bosses_pending/boss4_pending.h"
#include "../bosses_pending/boss3_motion.h"
#include "../bosses_a/boss_data.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)
#define NEAR(a, b) CHECK(std::fabs((double)(a) - (double)(b)) < 1e-3)

using namespace vn::bosspend;
using vn::boss4::Boss4Logic;

static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
static uint32_t fbits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }

struct Mock : Boss4PendEnv {
    Boss4State* st = nullptr;
    std::vector<std::string> log;
    // configuracion
    float cam = 0; int motLen = 100; int rnd = 0; float rnd01 = 0.5f;
    float trackPos[3] = {0, 0, 0}; int32_t trackAng = 0;
    float dirA = 0.5f, dirB = 0.25f; float ground = 0; float screenX = 100, screenY = 100; float headingRet = 0;
    uint32_t pathOut[6] = {0, 0, 0, 0, 0, 0}; uint32_t jitterOut[6] = {0, 0, 0, 0, 0, 0};
    float ptAdd[3] = {0, 0, 0}; float vecMul = 1.0f;
    // registros
    int lastMotion = -1, lastStart = -1; std::vector<int> hooks;
    std::vector<int32_t> dirAngles, rotYs; std::vector<std::vector<float>> translates;
    std::vector<IwaSpawn> spawns; int nextHandle = 100;
    std::vector<float> turnPoint; int turnStep = 0;
    std::vector<std::vector<int>> fx;
    std::vector<std::pair<float, int>> paths;
    int stateRuns = 0, tableLoads = 0;
    // Boss4Env
    float camFrame() override { return cam; }
    int camTrackId() override { return 0; }
    void setCamSpeed(float) override {}
    int motionLen(int) override { return motLen; }
    int rand() override { return rnd; }
    int damagePlayer(int, int, int) override { return 0; }
    void playMotion(int m, int s) override { lastMotion = m; lastStart = s; log.push_back("play"); if (st) { st->motion = m; st->motionFrame = (float)s; } }
    void sound(int, int) override {}
    void projectToScreen(const float*, float& x, float& y) override { x = screenX; y = screenY; }
    int statFunc(int, int) override { return 0; }
    float damageScale(int) override { return 1.0f; }
    void registerHit(int, int) override {}
    bool playerAlive(int) override { return true; }
    void hook(int what, int, int, int) override { hooks.push_back(what); }
    // Boss4PendEnv
    void camTrack(float pos[3], int32_t& angle) override { for (int i = 0; i < 3; i++) pos[i] = trackPos[i]; angle = trackAng; }
    float rand01() override { return rnd01; }
    void dirXZ(int32_t angle, float& a, float& b) override { dirAngles.push_back(angle); a = dirA; b = dirB; }
    float headingOf(const float d[3]) override { log.push_back("heading"); return headingRet != 0 ? headingRet : d[0]; }
    float groundY(const float*) override { log.push_back("ground"); return ground; }
    void turnToward(const float p[3], int step) override { turnPoint.assign(p, p + 3); turnStep = step; }
    void mtxPush() override { log.push_back("push"); }
    void mtxTranslate(float x, float y, float z) override { translates.push_back({x, y, z}); log.push_back("translate"); }
    void mtxRotY(int32_t a) override { rotYs.push_back(a); log.push_back("rotY"); }
    void mtxPoint(const float in[3], float out[3]) override { for (int i = 0; i < 3; i++) out[i] = in[i] + ptAdd[i]; }
    void mtxVector(const float in[3], float out[3]) override { for (int i = 0; i < 3; i++) out[i] = in[i] * vecMul; log.push_back("vector"); }
    void pathSample(float frame, int id, uint32_t out[6]) override {
        paths.push_back({frame, id});
        const uint32_t* src = id == 0x59 ? jitterOut : pathOut;
        for (int i = 0; i < 6; i++) out[i] = src[i];
    }
    void loadStateTable(const uint32_t rows[6][3]) override { tableLoads++; log.push_back("load"); (void)rows; }
    int stateRun(int) override { stateRuns++; log.push_back("state"); return 7; }
    void hitInfo(Boss4Logic::HitInfo out[2]) override { log.push_back("hit"); for (int i = 0; i < 2; i++) { out[i].slot = -1; out[i].partType = 0; out[i].weakIdx = 0; } }
    void controlSound() override { log.push_back("sound"); }
    void controlGareki() override { log.push_back("gareki"); }
    void entryCameraTarget() override { log.push_back("camTarget"); }
    void entryCollision() override { log.push_back("collision"); }
    int spawnIwa(const IwaSpawn& s) override { spawns.push_back(s); return nextHandle++; }
    void fx9443A0(int a1, int a2, int a3, int t0, int t1) override { fx.push_back({a1, a2, a3, t0, t1}); }
};
static bool hooked(const Mock& m, int h) { for (int x : m.hooks) if (x == h) return true; return false; }
static int count(const Mock& m, const char* s) { int n = 0; for (auto& l : m.log) if (l == s) n++; return n; }

// ---------------------------------------------------------------- datos
static std::vector<uint8_t> readFile(const char* p) {
    std::ifstream f(p, std::ios::binary); std::vector<uint8_t> v;
    if (!f) return v;
    v.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return v;
}
static uint32_t w32(const std::vector<uint8_t>& v, uint32_t addr) { uint32_t o = addr - 0x9C4800; uint32_t r = 0; std::memcpy(&r, &v[o], 4); return r; }

static void testData() {
    // D_009D3AE0 / D_009D3B10 (asm: lb 0x2EA4 y lwc1 con indice [nivel*17 + dificultad])
    CHECK(kB4IwaCount[0][0] == 5 && kB4IwaCount[0][16] == 12 && kB4IwaCount[1][0] == 5 && kB4IwaCount[1][1] == 6 && kB4IwaCount[1][16] == 12);
    NEAR(kB4IwaSpeed[0][0], 2.04); NEAR(kB4IwaSpeed[0][16], 3.12); NEAR(kB4IwaSpeed[1][0], 2.4); NEAR(kB4IwaSpeed[1][3], 2.64); NEAR(kB4IwaSpeed[1][16], 3.36);
    // D_009D3D30 (7 puntos) y D_009D3DE0
    CHECK(kB4ShoukanSlot[0].angle == 32768 && kB4ShoukanSlot[1].angle == 49152 && kB4ShoukanSlot[2].angle == 0 && kB4ShoukanSlot[3].angle == 16384);
    CHECK(kB4ShoukanSlot[4].angle == 16384 && kB4ShoukanSlot[5].angle == 0 && kB4ShoukanSlot[6].angle == 49152);
    NEAR(kB4ShoukanSlot[0].z, 114.827); NEAR(kB4ShoukanSlot[2].x, -230.58); NEAR(kB4ShoukanSlot[5].x, 230.58); NEAR(kB4ShoukanSlot[0].y, 14.0);
    NEAR(kB4PathShift[0][1], 39.6); NEAR(kB4PathShift[0][2], -77.7); NEAR(kB4PathShift[1][0], -192.5); NEAR(kB4PathShift[4][0], 192.5);
    NEAR(kB4IwaOrigin[0][0], 9.4271); NEAR(kB4IwaOrigin[0][1], -24.7495);
    NEAR(kB4BunshinSpread[0][0], 1.0); NEAR(kB4BunshinSpread[0][1], 2.0); NEAR(kB4BunshinSpread[1][0], -1.0); NEAR(kB4BunshinSpread[2][1], -1.0);
    CHECK(kB4KiriPreAngle[0][0] == 2367 && kB4KiriPreAngle[0][1] == 3641 && kB4KiriPreAngle[1][0] == 63170 && kB4KiriPreAngle[1][1] == 61896);
    CHECK(kB4KiriPreAngleAdd[0] == 60076 && kB4KiriPreAngleAdd[1] == 7282 && kB4KiriPreMotion[0] == 421 && kB4KiriPreMotion[1] == 422);   // ELF gp-0x79B0 / gp-0x79A8
    NEAR(kB4KiriY[0], 36.4); NEAR(kB4KiriY[1], 39.7); CHECK(kB4KiriAngle[0] == 36227 && kB4KiriAngle[1] == 32768 && kB4KiriAngle[2] == 29309);
    CHECK(kB4KiriShoukanMotion[0] == 430 && kB4KiriShoukanMotion[1] == 429 && kB4KiriShoukanMotion[2] == 433);
    CHECK(kB4BunshinMotRows[0][0][0] == 374 && kB4BunshinMotRows[1][0][2] == 364 && kB4BunshinMotRows[2][1][4] == 371);
    // las 6 filas de D_009D2B00 (punteros a act* del overlay)
    CHECK(kB4StateRows[0][2] == 0x9CCDC0 && kB4StateRows[2][0] == 0x9CBB20 && kB4StateRows[3][1] == 0x9CBB50 && kB4StateRows[4][2] == 0x9CBA90 && kB4StateRows[1][2] == 0xFFFFFFFFu);
    // Boss3: D_009D0880 (13 entradas). Las 10 primeras coinciden con la tabla de bosses_a.
    CHECK(vn::boss::kBs3Motion[0].anim == kB3MotionFull[0].anim);
    for (int i = 0; i < 10; i++) CHECK(vn::boss::kBs3Motion[i].anim == kB3MotionFull[i].anim && vn::boss::kBs3Motion[i].length == kB3MotionFull[i].length);
    CHECK(kB3MotionFull[10].anim == 335 && kB3MotionFull[10].length == 350 && kB3MotionFull[11].anim == 340 && kB3MotionFull[11].length == 355 && kB3MotionFull[12].anim == 336 && kB3MotionFull[12].length == 351);

    // tablas de saltos leidas del BIN (se omite si falta)
    std::vector<uint8_t> bin = readFile("disc/TBL/BS4MOD.BIN");
    if (bin.size() < 0x18000) { printf("AVISO: falta disc/TBL/BS4MOD.BIN: se omite la comprobacion de las tablas de saltos\n"); return; }
    // jtbl_009DA760 (ControlBunshin_battle2): destinos 0x9C980C (pone 0x4000), 0x9C981C (lo quita), 0x9C982C (nada)
    for (int i = 0; i < 19; i++) {
        uint32_t t = w32(bin, 0x9DA760 + 4 * i);
        int want = t == 0x9C980C ? 0 : t == 0x9C981C ? 1 : 2;
        CHECK(t == 0x9C980C || t == 0x9C981C || t == 0x9C982C);
        CHECK(kB4BunshinMotAction[i] == want);
    }
    // jtbl_009DA690 (CtrlPre_bunshin_battle2): 0x9C6F6C fila[2+runD], 0x9C6F5C fila[0], 0x9C6F64 fila[1], 0x9C6F0C copia, 0x9C6F84 nada, 0x9C6F80 -> 0x17E
    for (int i = 0; i < 19; i++) {
        uint32_t t = w32(bin, 0x9DA690 + 4 * i);
        int want = t == 0x9C6F6C ? 0 : t == 0x9C6F5C ? 1 : t == 0x9C6F64 ? 2 : t == 0x9C6F0C ? 3 : t == 0x9C6F84 ? 4 : t == 0x9C6F80 ? 5 : -1;
        CHECK(want >= 0 && kB4BunshinB2Class[i] == want);
    }
    // D_009D3F30 (filas de movimientos de los clones)
    CHECK((int32_t)w32(bin, 0x9D3F30) == 374 && (int32_t)w32(bin, 0x9D3F30 + 4 * 10) == 373 && (int32_t)w32(bin, 0x9D3F30 + 4 * 20) == 373);
    std::vector<uint8_t> b3 = readFile("disc/TBL/BS3MOD.BIN");
    if (b3.size() >= 0xD000) {
        for (int i = 0; i < 13; i++) {
            CHECK((int32_t)w32(b3, 0x9D0880 + 8 * i) == kB3MotionFull[i].anim && (int32_t)w32(b3, 0x9D0884 + 8 * i) == kB3MotionFull[i].length);
        }
        // Boss3: el original indexa D_009D0880 sin limite; 324*8 cae en datos ajenos (no en la tabla): prueba de que 324 solo es un id valido en la escena 0x14
        CHECK(0x9D0880 + 324 * 8 >= 0x9D0A20);   // pasa D_009D0A20 (tabla de golpes de Boss3)
    }
}

// ---------------------------------------------------------------- Boss4

static void testSmall() {
    // UpdateMotionFrame
    { Boss4State b; Boss4Ext x; b.motionFrame = 10; b.rate = 1.2f; x.frameB = 5; x.rateB = 0.5f;
      Boss4Pending::updateMotionFrame(b, x); NEAR(b.motionFrame, 11.2); NEAR(x.frameB, 5.5);
      b.f0C = F0C_DELETE; Boss4Pending::updateMotionFrame(b, x); NEAR(b.motionFrame, 11.2); NEAR(x.frameB, 5.5); }
    // ControlCamera: solo pelea 1, movimientos 0x188..0x18A, al cruzar 195
    { Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE1; b.motion = 0x188; b.rate = 1.0f; g.cameraStartAllowed = 1;
      x.frameC = 190; Boss4Pending::controlCamera(b, x, g); CHECK(g.cameraStartAllowed == 1);
      x.frameC = 194.5f; Boss4Pending::controlCamera(b, x, g); CHECK(g.cameraStartAllowed == 0);
      g.cameraStartAllowed = 1; x.frameC = 195.5f; Boss4Pending::controlCamera(b, x, g); CHECK(g.cameraStartAllowed == 1);   // ya paso: f < 195 falla
      b.motion = 0x18A; x.frameC = 194.5f; Boss4Pending::controlCamera(b, x, g); CHECK(g.cameraStartAllowed == 0);
      g.cameraStartAllowed = 1; b.motion = 0x187; Boss4Pending::controlCamera(b, x, g); CHECK(g.cameraStartAllowed == 1);
      b.motion = 0x188; b.kind = K_BATTLE2; Boss4Pending::controlCamera(b, x, g); CHECK(g.cameraStartAllowed == 1); }
    // ClearWeakHit
    { Boss4Ext x; uint32_t a = 0xFFFFFFFFu, c = 0x40000000u; x.partFlags[0] = &a; x.partFlags[2] = &c; Boss4Pending::clearWeakHit(x);
      CHECK(a == 0xBFFFFFFFu && c == 0); }
    // SetHeight
    { Mock m; Boss4State b; Boss4Ext x; x.groundOffset = 3; m.ground = 10; b.pos[1] = 99; b.motion = 5;
      Boss4Pending::setHeight(b, x, m); NEAR(b.pos[1], 13);
      b.f10 = 0x40; b.pos[1] = 99; Boss4Pending::setHeight(b, x, m); NEAR(b.pos[1], 99);                // cayendo: no se pega al suelo
      b.f10 = 0; b.motion = 0x17C; b.pos[1] = 20; b.fallStep = 5; Boss4Pending::setHeight(b, x, m);
      CHECK((b.f10 & 0x40) && b.fallStep == 0 && b.pos[1] == 20);                                       // por encima del suelo: empieza la caida
      b.f10 = 0; b.pos[1] = 13; b.fallStep = 5; Boss4Pending::setHeight(b, x, m); CHECK(b.f10 == 0 && b.fallStep == 5);   // en el suelo (<=): nada
    }
    // DownDifficultyReq = DownDifficulty
    { Boss4State a, c; a.difficulty = c.difficulty = 8; a.diffDown = c.diffDown = 3; a.diffUp = c.diffUp = 1;
      Boss4Pending::downDifficultyReq(a); Boss4Logic::downDifficulty(c);
      CHECK(a.difficulty == c.difficulty && a.diffArmed == c.diffArmed && a.difficulty <= 8); }
    // MotSupSet (primera variante: modo 1, ignora si hay dano, compara con el objeto de movimiento)
    { Mock m; Boss4State b; Boss4Ext x; m.st = &b; x.motionObjCur = 10;
      Boss4Pending::motSupSet(b, x, m, 10, 3, 4); CHECK(m.lastMotion == -1 && b.motReqMode == 0);       // mismo movimiento: nada
      Boss4Pending::motSupSet(b, x, m, 11, 3, 4); CHECK(m.lastMotion == 11 && m.lastStart == 3 && b.motReqMode == 1 && b.motReqA == 3 && b.motReqB == 4);
      m.lastMotion = -1; b.f0C = F0C_IN_DAMAGE; Boss4Pending::motSupSet(b, x, m, 12, 0, 0); CHECK(m.lastMotion == -1);
      b.f0C = 0; b.typeId = 0x1C; Boss4Pending::motSupSet(b, x, m, 12, 0, 0); CHECK(hooked(m, H_PLAY_HAND_MOTIONS)); }
}

static void testAttacks() {
    // ControlAttack_battle3
    { Boss4State b; Boss4Ext x; Boss4Globals g; x.iwaCount = 3; x.iwaState[0] = 0; x.iwaState[1] = 1; x.iwaState[2] = 1;
      b.f1E14 = FB_ATTACK; b.attack = 0x12; b.f0C = F0C_ATTACKING; g.enemiesLeft = 5; b.s1F40 = 0;
      Boss4Pending::controlAttackBattle3(b, x, g);
      CHECK(x.iwaState[0] == 0 && x.iwaState[1] == 2 && x.iwaState[2] == 1 && b.s1F40 == 1);          // primera roca en estado 1 pasa a 2
      Boss4Pending::controlAttackBattle3(b, x, g); CHECK(x.iwaState[2] == 1 && b.s1F40 == 2);          // fuera de multiplo de 40
      b.s1F40 = 40; Boss4Pending::controlAttackBattle3(b, x, g); CHECK(x.iwaState[2] == 2 && b.s1F40 == 41);
      b.s1F40 = 80; Boss4Pending::controlAttackBattle3(b, x, g); CHECK(b.s1F40 == 81);                  // sin mas rocas en estado 1: nada
      g.enemiesLeft = 2; Boss4Pending::controlAttackBattle3(b, x, g);
      CHECK(!(b.f0C & F0C_ATTACKING) && !(b.f1E14 & FB_ATTACK));                                        // < 3 enemigos: cancela
      b.f1E14 = FB_ATTACK; b.attack = 5; b.f0C = F0C_ATTACKING; Boss4Pending::controlAttackBattle3(b, x, g); CHECK((b.f1E14 & FB_ATTACK) && (b.f0C & F0C_ATTACKING));   // otro ataque
      b.f1E14 = 0; b.attack = 0x12; g.enemiesLeft = 0; b.f0C = F0C_ATTACKING; Boss4Pending::controlAttackBattle3(b, x, g); CHECK(b.f0C & F0C_ATTACKING); }
    // StartAttackIwa: nivel 1, dificultad 3 -> 7 rocas, velocidad 2.64; rand = 30 -> 30 % 25 = 5
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; g.gameLevel = 1; b.difficulty = 3; x.slot = 3; m.rnd = 30;
      m.ptAdd[0] = 1; m.ptAdd[1] = 2; m.ptAdd[2] = 3;
      Boss4Pending::startAttackIwa(b, x, g, m);
      CHECK(x.iwaCount == 7 && m.spawns.size() == 7);
      CHECK(m.log.size() == 4 && m.log[0] == "push" && m.log[1] == "translate" && m.log[2] == "rotY" && m.log[3] == "translate");
      NEAR(m.translates[0][0], -230.58); CHECK(m.translates[0][1] == 0); NEAR(m.translates[0][2], 114.827);      // slot 3: x,0,z
      CHECK(m.rotYs[0] == 16384 - 32768);                                                                       // angulo del slot - angulo del slot 0
      NEAR(m.translates[1][0], -0.0); NEAR(m.translates[1][2], -114.827);                                       // -slot0.x, 0, -slot0.z
      for (int i = 0; i < 7; i++) {
          const float* o = kB4IwaOrigin[(i + 5) % 25];
          CHECK(m.spawns[i].index == i);
          NEAR(m.spawns[i].srcPos[0], o[0] + 1); NEAR(m.spawns[i].srcPos[1], o[1] + 2); NEAR(m.spawns[i].srcPos[2], o[2] + 3);
          NEAR(m.spawns[i].spawnPos[1], o[1] + 2 + 80.0f); NEAR(m.spawns[i].speed, 2.64);
          CHECK(x.iwaHandle[i] == 100 + i && x.iwaState[i] == 1);
      } }
    // el punto de aparicion sale del padre si existe
    { Mock m; Boss4State b; Boss4Ext x, px; Boss4Globals g; g.gameLevel = 0; b.difficulty = 0; x.slot = 3; px.slot = 5; x.ownerExt = &px; m.rnd = 0;
      Boss4Pending::startAttackIwa(b, x, g, m); CHECK(m.spawns.size() == 5 && m.rotYs[0] == 0 - 32768); NEAR(m.translates[0][0], 230.58); NEAR(m.spawns[0].speed, 2.04); }
    // slot invalido (-1): el original no comprueba; aqui no genera rocas
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; x.slot = -1; Boss4Pending::startAttackIwa(b, x, g, m); CHECK(m.spawns.empty()); }

    // StartAttackKiriPre arg 1, b1E12 = 1
    { Mock m; Boss4State b; Boss4Ext x; m.st = &b; x.b1E12 = 1; m.trackPos[0] = 1000; m.trackPos[2] = 2000; m.trackAng = 0x1000; m.rnd01 = 0.75f;
      b.b1DB8 = 7;
      Boss4Pending::startAttackKiriPre(b, x, m, 1);
      int32_t s1 = 0x1000 + 61896;                                                                       // D_009D4210[1][1]
      CHECK(m.dirAngles.size() == 1 && m.dirAngles[0] == s1 + 7282 + 0x8000);                           // + gp-0x79B0[1] + 0x8000
      NEAR(b.pos[1], 26.0 + 20.0 * 0.25); NEAR(b.pos[0], 1000 + 98.64 * 0.5); NEAR(b.pos[2], 2000 + 98.64 * 0.25);
      CHECK(b.rot[0] == 0 && b.rot[1] == (float)s1 && b.rot[2] == 0);
      CHECK(m.lastMotion == 422 && b.b1DB8 == 0); }
    // arg 0, b1E12 = 0
    { Mock m; Boss4State b; Boss4Ext x; m.st = &b; m.trackAng = 0; Boss4Pending::startAttackKiriPre(b, x, m, 0);
      CHECK(m.dirAngles[0] == 2367 + 60076 + 0x8000 && m.lastMotion == 421 && b.rot[1] == 2367.0f); }
    // StartAttackKiri: rand 7 -> 7 % 3 = 1
    { Mock m; Boss4State b; m.st = &b; m.rnd = 7; m.trackPos[0] = 500; m.trackPos[2] = 600; m.trackAng = 0x2000; b.b1DB8 = 3;
      Boss4Pending::startAttackKiri(b, m);
      int32_t ang = 0x2000 + 32768;                                                                      // D_009D4230[1]
      CHECK(m.dirAngles.size() == 1 && m.dirAngles[0] == ang);
      const double K = 129.0 + 0.0; (void)K;
      uint64_t kb = 0x40602FD7DBF487FDull; double Kd; std::memcpy(&Kd, &kb, 8);
      NEAR(Kd, 129.4951);                                                                                   // constante del original (ELF), ver boss4_pending.cpp
      NEAR(b.pos[0], 500 + Kd * 0.5); NEAR(b.pos[2], 600 + Kd * 0.25); NEAR(b.pos[1], 39.7 - 8.0);
      CHECK(b.rot[1] == (float)(ang + 0x8000) && m.lastMotion == 0x1A0 && b.b1DB8 == 0); }
    // StartAttackKiri_shoukan: rand 4 -> 4 % 3 = 1 -> movimiento 429
    { Mock m; Boss4State b; m.st = &b; m.rnd = 4; m.trackPos[0] = 100; m.trackPos[2] = 200; m.trackAng = 0x4000;
      Boss4Pending::startAttackKiriShoukan(b, m);
      CHECK(m.dirAngles[0] == 0x4000 + 0x8000);
      NEAR(b.pos[0], 100 + 157 * 0.5); NEAR(b.pos[1], -2.8); NEAR(b.pos[2], 200 + 157 * 0.25);
      CHECK(b.rot[1] == 16384.0f && m.lastMotion == 429); }
    // GetShoukanPos: slot 2 (x -230.58, z -115.753, angulo 0)
    { Mock m; Boss4Ext x; x.slot = 2; m.ptAdd[0] = 10; m.ptAdd[1] = 10; m.ptAdd[2] = 10;
      uint32_t in[6] = {fbits(1), fbits(2), fbits(3), 7u, 1000u, 9u}, out[6];
      Boss4Pending::getShoukanPos(x, m, in, out);
      NEAR(bitsF(out[0]), 11); NEAR(bitsF(out[1]), 12); NEAR(bitsF(out[2]), 13);
      CHECK(out[3] == 7u && (int32_t)out[4] == 1000 + 0 - 32768 && out[5] == 9u);
      NEAR(m.translates[0][0], -230.58); NEAR(m.translates[0][2], -115.753); CHECK(m.rotYs[0] == -32768);
      Boss4Ext px; px.slot = 3; x.ownerExt = &px; Boss4Pending::getShoukanPos(x, m, in, out);
      CHECK((int32_t)out[4] == 1000 + 16384 - 32768); }
}

static void testBunshin() {
    // BnStartBunshin / BnStartShusoku / BnStartStandby / BnCheckCmd. D_009D3F10[2] = {-2, -1}
    { Mock m; Boss4State o, b; Boss4Ext x; o.kind = K_BATTLE1; o.runB = 2; o.speedBase = 2.0f; b.owner = &o; x.bnIndex = 1; b.f1E14 = 0x20000 | 0x1;
      Boss4Pending::bnStartBunshin(b, x, m);
      NEAR(x.bnVel[0], 2.0 * -1.0); CHECK(x.bnVel[1] == 0 && x.bnVel[2] == 0 && x.bnMode == 2 && b.f1E14 == 0x1u);
      CHECK(m.fx.size() == 1 && m.fx[0] == std::vector<int>({1, 5, 0x1E, 5, 0}));
      Boss4Pending::bnStartShusoku(b, x); NEAR(x.bnVel[0], 2.0); CHECK(x.bnMode == 3);
      b.f1E14 = 0; Boss4Pending::bnStartStandby(b, x); CHECK(x.bnMode == 1 && (b.f1E14 & 0x20000));      // 3 -> 1 + visible
      x.bnMode = 2; Boss4Pending::bnStartStandby(b, x); CHECK(x.bnMode == 0);                           // 2 -> 0
      x.bnMode = 1; b.f1E14 = 0; Boss4Pending::bnStartStandby(b, x); CHECK(x.bnMode == 1 && b.f1E14 == 0);   // otros: nada
      // otro tipo de padre: no toca la velocidad
      o.kind = K_BATTLE2; x.bnVel[0] = 9; Boss4Pending::bnStartBunshin(b, x, m); CHECK(x.bnVel[0] == 9 && x.bnMode == 2);
      // BnCheckCmd
      x.bnMode = 0; x.bnCmd = 0; o.bunshinCmd = 1; o.kind = K_BATTLE1; m.fx.clear();
      Boss4Pending::bnCheckCmd(b, x, m); CHECK(x.bnCmd == 1 && x.bnMode == 2 && m.fx.size() == 1);
      Boss4Pending::bnCheckCmd(b, x, m); CHECK(m.fx.size() == 1);                                       // sin cambio: nada
      o.bunshinCmd = 2; Boss4Pending::bnCheckCmd(b, x, m); CHECK(x.bnCmd == 2 && x.bnMode == 3);
      o.bunshinCmd = 0; Boss4Pending::bnCheckCmd(b, x, m); CHECK(x.bnCmd == 0 && x.bnMode == 1);
      o.bunshinCmd = 5; Boss4Pending::bnCheckCmd(b, x, m); CHECK(x.bnCmd == 5 && x.bnMode == 1); }
    // ControlBunshin_battle1 (del jefe)
    { Mock m; Boss4State b; b.f1E14 = FB_BUNSHIN_ASK; b.f1E44 = 2; b.rot[1] = 0x1234; b.speedBase = 1.5f; b.runB = 0; b.pos[0] = 10; b.pos[2] = 20;
      Boss4Pending::controlBunshinBattle1(b, m);
      CHECK(m.dirAngles.size() == 1 && m.dirAngles[0] == 0x1234 && (b.f1E14 & FB_BN_CANCEL) && b.bunshinCmd == 1 && (int16_t)b.f1E44 == 1);
      NEAR(b.pos[0], 10 - 0.25 * 1.5); NEAR(b.pos[2], 20 - 0.5 * 1.5);                                  // 1er resultado -> z, 2o -> x
      b.runB = 2; Boss4Pending::controlBunshinBattle1(b, m); NEAR(b.pos[0], 10 - 0.375 + 0.375); NEAR(b.pos[2], 20 - 0.75 + 0.75); CHECK((int16_t)b.f1E44 == 0);
      b.runB = 1; Boss4Pending::controlBunshinBattle1(b, m);                                             // t == 0: termina sin mover
      CHECK(!(b.f1E14 & FB_BUNSHIN_ASK) && b.bunshinCmd == 0 && (int16_t)b.f1E44 == -1);
      NEAR(b.pos[0], 10); NEAR(b.pos[2], 20); }
    { Mock m; Boss4State b; b.f1E14 = FB_SHUSOKU_ASK; b.f1E44 = 1;
      Boss4Pending::controlBunshinBattle1(b, m); CHECK((b.f1E14 & FB_BN_CANCEL) && b.bunshinCmd == 2 && (int16_t)b.f1E44 == 0 && (b.f1E14 & FB_SHUSOKU_ASK));
      Boss4Pending::controlBunshinBattle1(b, m); CHECK(!(b.f1E14 & FB_BN_CANCEL) && !(b.f1E14 & FB_SHUSOKU_ASK) && b.bunshinCmd == 0);
      b.f1E14 = 0; b.bunshinCmd = 9; Boss4Pending::controlBunshinBattle1(b, m); CHECK(b.bunshinCmd == 9); }
    // ControlBunshin_battle2: 0x16C..0x177 y 0x179..0x17B ponen 0x4000; 0x178, 0x17C, 0x17D lo quitan; 0x17E y fuera de rango, nada
    { Boss4State b;
      for (int mo : {0x16C, 0x170, 0x177, 0x179, 0x17B}) { b.f1E14 = 0; b.motion = mo; Boss4Pending::controlBunshinBattle2(b); CHECK(b.f1E14 == FB_BN_CANCEL); }
      for (int mo : {0x178, 0x17C, 0x17D}) { b.f1E14 = FB_BN_CANCEL | 1; b.motion = mo; Boss4Pending::controlBunshinBattle2(b); CHECK(b.f1E14 == 1u); }
      for (int mo : {0x17E, 0x17F, 0x16B, 0x100, 0, -5}) { b.f1E14 = FB_BN_CANCEL; b.motion = mo; Boss4Pending::controlBunshinBattle2(b); CHECK(b.f1E14 == FB_BN_CANCEL); } }
    // CtrlPre_bunshin_battle1
    { Mock m; Boss4State o, b; Boss4Ext x; m.st = &b; b.owner = &o; o.kind = K_BATTLE1; o.motion = 777; o.motReqMode = 2; o.motReqA = 4; o.motReqB = 5;
      o.pos[0] = 100; o.pos[1] = 50; o.pos[2] = 25; x.bnOffset[0] = 1; x.bnOffset[1] = 2; x.bnOffset[2] = 3; x.bnMode = BN_OUT; x.bnVel[0] = 0.5f; x.bnVel[1] = 0; x.bnVel[2] = -1; b.alpha = 0.4f;
      Boss4Pending::ctrlPreBunshinBattle1(b, x, m);
      CHECK(m.lastMotion == 777 && m.lastStart == 4 && b.motReqMode == 2 && b.motReqB == 5);
      NEAR(b.alpha, 1.0); NEAR(x.bnOffset[0], 1.5); NEAR(x.bnOffset[2], 2.0); NEAR(b.pos[0], 101.5); NEAR(b.pos[1], 52); NEAR(b.pos[2], 27);
      // padre en dano: se desvanece 1/30 por fotograma, pasa a espera y queda visible
      o.f0C = F0C_IN_DAMAGE; b.alpha = 1.0f; x.bnMode = BN_OUT; b.f1E14 = 0; o.motReqMode = 0; x.bnCmd = o.bunshinCmd;
      Boss4Pending::ctrlPreBunshinBattle1(b, x, m); NEAR(b.alpha, 1.0 - 1.0 / 30.0); CHECK(x.bnMode == BN_STANDBY && (b.f1E14 & 0x20000));
      b.alpha = 0.01f; Boss4Pending::ctrlPreBunshinBattle1(b, x, m); NEAR(b.alpha, 0.0);
      // volviendo: al acercarse a menos de 0.1 pasa a espera
      o.f0C = 0; x.bnMode = BN_BACK; x.bnOffset[0] = 1.0f; x.bnOffset[1] = 0; x.bnOffset[2] = 0; x.bnVel[0] = -0.95f; x.bnVel[1] = 0; x.bnVel[2] = 0; b.f1E14 = 0;
      Boss4Pending::ctrlPreBunshinBattle1(b, x, m); CHECK(x.bnMode == BN_STANDBY && (b.f1E14 & 0x20000));
      x.bnMode = BN_BACK; x.bnOffset[0] = 1.0f; x.bnVel[0] = -0.5f; b.f1E14 = 0;
      Boss4Pending::ctrlPreBunshinBattle1(b, x, m); CHECK(x.bnMode == BN_BACK && b.f1E14 == 0); NEAR(x.bnOffset[0], 0.5);
      // modo 0 y 1: no cambia el desplazamiento
      x.bnMode = BN_IDLE; x.bnOffset[0] = 3; Boss4Pending::ctrlPreBunshinBattle1(b, x, m); NEAR(x.bnOffset[0], 3); NEAR(b.pos[0], 103);
      // modos de peticion 1 y 3
      o.motReqMode = 3; o.motion = 555; b.motion = 1; b.motionFrame = 9; m.lastMotion = -1; Boss4Pending::ctrlPreBunshinBattle1(b, x, m); CHECK(m.lastMotion == 555);
      o.motReqMode = 1; o.motion = 556; x.motionObjCur = 556; m.lastMotion = -1; Boss4Pending::ctrlPreBunshinBattle1(b, x, m); CHECK(m.lastMotion == -1);
      x.motionObjCur = 0; Boss4Pending::ctrlPreBunshinBattle1(b, x, m); CHECK(m.lastMotion == 556); }
    // CtrlPre_bunshin_battle2: padre runB=1, runD=1; clon 0 -> fila kB4BunshinMotRows[1][0] = {373,377,364,367,370}
    { Mock m; Boss4State o, b; Boss4Ext x; m.st = &b; b.owner = &o; o.runB = 1; o.runD = 1; x.bnIndex = 0; o.motReqMode = 2; o.motReqA = 2; o.motReqB = 3;
      o.motion = 0x16C; b.f1E14 = 0x20000;
      Boss4Pending::ctrlPreBunshinBattle2(b, x, m);                                                      // clase 0: fila[2 + runD] = 367
      CHECK(m.lastMotion == 367 && x.bnLastMot == 367 && !(b.f1E14 & 0x20000)); m.lastMotion = -1;
      Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(m.lastMotion == -1);                           // igual al anterior: nada
      o.motion = 0x175; Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(m.lastMotion == 373);        // clase 1: fila[0]
      o.motion = 0x179; Boss4Pending::ctrlPreBunshinBattle2(b, x, m);                                    // clase 2: fila[1] = 377 (no es 0x179..0x17B)
      CHECK(m.lastMotion == 377 && m.fx.size() == 1);   // 377 = 0x179: dispara el efecto
      o.motion = 0x17E; Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(m.lastMotion == 0x17E && x.bnLastMot == 0x17E);
      o.motion = 0x178; o.pos[0] = 11; o.pos[1] = 12; o.pos[2] = 13; o.rot[0] = 1; o.rot[1] = 2; o.rot[2] = 3; b.f1E14 = 0;
      Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(b.pos[0] == 11 && b.pos[2] == 13 && b.rot[1] == 2 && (b.f1E14 & 0x20000));   // copia y oculta
      b.f1E14 = 0; o.motion = 0x17C; Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(b.f1E14 == 0x20000u);
      b.f1E14 = 0; o.motion = 0x300; Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(b.f1E14 == 0x20000u);
      // runB = 0, clon 1: fila {375,379,366,369,372}; efecto cuando el movimiento pedido es 0x179..0x17B: se comprueba con una fila sintetica
      // (las filas reales no contienen 0x179..0x17B = 377..379 salvo fila[1]=379 -> 0x17B)
      o.runB = 0; x.bnIndex = 1; o.motion = 0x179; x.bnLastMot = 0; b.f1E14 = 0; m.fx.clear();          // clase 2: fila[1] = 379 = 0x17B -> efecto
      Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(m.lastMotion == 379 && m.fx.size() == 1 && m.fx[0] == std::vector<int>({2, 1, 0x19, 0x19, 0}));
      o.runB = 1; x.bnIndex = 0; o.motion = 0x179; x.bnLastMot = 0; m.fx.clear();                       // fila[1] = 377 = 0x179 -> efecto
      Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(m.lastMotion == 377 && m.fx.size() == 1);
      // motReqMode 1 usa MotSupSet con el objeto de movimiento
      o.motReqMode = 1; x.motionObjCur = 0; x.bnLastMot = 0; m.lastMotion = -1; o.motion = 0x16C;
      Boss4Pending::ctrlPreBunshinBattle2(b, x, m); CHECK(m.lastMotion == 367 && b.motReqMode == 1); }
    // CtrlPre_bunshin (CtrlPre_bunshin 0x9C68D0): consulta el estado, no hace nada si +0x1F1D == 3
    { Mock m; Boss4State o, b; Boss4Ext x; Boss4PendGlobals pg; b.owner = &o; o.kind = K_BATTLE1; b.stateB = 3;
      Boss4Pending::ctrlPreBunshin(b, x, pg, m); CHECK(m.tableLoads == 1 && pg.stateTableLoaded == 1 && m.stateRuns == 1 && x.stateResult == 7 && count(m, "play") == 0);
      b.stateB = 0; o.motReqMode = 2; o.motion = 5; o.motReqA = 0; m.st = &b; Boss4Pending::ctrlPreBunshin(b, x, pg, m);
      CHECK(m.tableLoads == 1 && m.stateRuns == 2 && m.lastMotion == 5); }
}

static void testFrame() {
    // CtrlPre: velocidad por fotograma
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; Boss4PendGlobals pg; b.kind = 99; b.f1E14 = 0x400 | 0x1; b.motReqMode = 5;
      b.pos[0] = 1; b.pos[1] = 2; b.pos[2] = 3;
      Boss4Pending::ctrlPre(b, x, g, pg, m);
      NEAR(x.prevDelta[0], 1); NEAR(x.prevDelta[1], 2); NEAR(x.prevDelta[2], 3); NEAR(x.accel[2], 3);   // vs. delta anterior (0)
      NEAR(x.prevPos[0], 1); NEAR(x.heading, 1); NEAR(x.headingDelta, 1);
      CHECK(b.f1E14 == 0x1u && b.motReqMode == 0);                                                      // quita 0x400 y pone el modo a 0
      b.pos[0] = 3; b.pos[1] = 2; b.pos[2] = 1; Boss4Pending::ctrlPre(b, x, g, pg, m);
      NEAR(x.prevDelta[0], 2); NEAR(x.prevDelta[1], 0); NEAR(x.prevDelta[2], -2);
      NEAR(x.accel[0], 1); NEAR(x.accel[1], -2); NEAR(x.accel[2], -5);
      NEAR(x.headingDelta, 2 - 1);
      CHECK(count(m, "sound") == 2 && m.stateRuns == 0);                                                // tipo desconocido: sin despacho, sonido si
    }
    // CtrlPre: despacho por tipo
    for (int kind = 0; kind <= 4; kind++) {
        Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; Boss4PendGlobals pg; b.kind = (uint8_t)kind; b.owner = nullptr;
        Boss4State o; o.kind = K_BATTLE1; if (kind == 4) { b.owner = &o; b.stateB = 3; }
        Boss4Pending::ctrlPre(b, x, g, pg, m);
        if (kind == 4) CHECK(m.stateRuns == 2 && count(m, "sound") == 0 && m.tableLoads == 1);          // doble func_1C6C80 y sin ControlSound
        else if (kind == 3) CHECK(m.stateRuns == 0 && count(m, "sound") == 1);                        // shoukan: CtrlPre_shoukan pendiente
        else CHECK(m.stateRuns == 1 && count(m, "ground") == 1 && count(m, "hit") == 1 && count(m, "sound") == 1 && m.tableLoads == 1);
        if (kind == 1) CHECK(count(m, "gareki") == 1); else CHECK(count(m, "gareki") == 0);
        if (kind == 3 || kind == 4) CHECK(count(m, "ground") == 0);
    }
    // la tabla de estados solo se copia una vez en todo el juego
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; Boss4PendGlobals pg; b.kind = K_BATTLE3;
      Boss4Pending::ctrlPre(b, x, g, pg, m); Boss4Pending::ctrlPre(b, x, g, pg, m); CHECK(m.tableLoads == 1 && m.stateRuns == 2);
      pg.stateTableLoaded = 1; Mock m2; Boss4Pending::ctrlPre(b, x, g, pg, m2); CHECK(m2.tableLoads == 0); }
    // el orden de CtrlPre_battle1: estado, suelo, ..., hit
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; Boss4PendGlobals pg; b.kind = K_BATTLE1;
      Boss4Pending::ctrlPreBattle1(b, x, g, pg, m);
      CHECK(m.log.size() >= 4 && m.log[0] == "load" && m.log[1] == "state" && m.log[2] == "ground" && m.log[3] == "hit"); }
    // barra de vida visible y no en 4: se llama a ControlDispLife (se nota en la proporcion de vida)
    { Mock m; Boss4State b, c; Boss4Ext x; Boss4Globals g; Boss4PendGlobals pg; b.kind = 99; c.kind = 99; g.bossLife = 50; g.bossLifeMax = 100; b.partLife = c.partLife = 3; b.partLifeMax = c.partLifeMax = 4;
      b.f1E14 = FB_LIFEBAR; Boss4Pending::ctrlPre(b, x, g, pg, m);
      Boss4Logic::controlDispLife(c, g);
      CHECK(b.lifeRatio == c.lifeRatio && b.hitRatio == c.hitRatio);
      Boss4State d; d.kind = 99; d.f1E14 = FB_LIFEBAR | 4; Boss4Ext xd; d.lifeRatio = 123.0f; Mock m3; Boss4Pending::ctrlPre(d, xd, g, pg, m3); CHECK(d.lifeRatio == 123.0f); }
    // CtrlPost
    { Mock m; Boss4State b; Boss4Ext x; b.motionFrame = 1; b.rate = 2; x.rateB = 1;
      Boss4Pending::ctrlPost(b, x, m); NEAR(b.motionFrame, 3); NEAR(x.frameB, 1); CHECK(x.frameCounter == 1);
      CHECK(m.log.size() == 2 && m.log[0] == "camTarget" && m.log[1] == "collision"); }
}

static void testCheckDamageEnd() {
    // aturdimiento por golpe
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.f1E14 = FB_HIT_STUN; b.stunTimer = 2; b.rate = 0.3f; b.rateBase = 1.5f; b.rateMul = 0.2f;
      Boss4Pending::checkDamageEnd(b, x, g, m); CHECK((b.f1E14 & FB_HIT_STUN) && b.stunTimer == 1 && b.rate == 0.3f);
      Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(!(b.f1E14 & FB_HIT_STUN) && b.stunTimer == 0 && b.rateMul == 1.0f && b.rate == 1.5f); }
    // pelea 3 en dano, subfase 0: quita 0x800000 si quedan < 3 enemigos
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE3; b.f0C = F0C_IN_DAMAGE; b.subPhase = 0; b.f1E14 = 0x800000; g.enemiesLeft = 2; b.rate = 1.0f;
      Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(!(b.f1E14 & 0x800000) && hooked(m, H_SET_SCREEN_OFFSET));
      b.f1E14 = 0x800000; g.enemiesLeft = 3; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(b.f1E14 & 0x800000); }
    // fuera de la pelea 3 o sin dano: nada
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE2; b.f0C = F0C_IN_DAMAGE; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(m.hooks.empty());
      b.kind = K_BATTLE3; b.f0C = 0; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(m.hooks.empty()); }
    // subfase 1: camara < 2 permite empezar camara
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE3; b.f0C = F0C_IN_DAMAGE; b.subPhase = 1; b.rate = 1.0f; g.cameraStartAllowed = 0;
      m.cam = 2.0f; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(g.cameraStartAllowed == 0);
      m.cam = 1.9f; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(g.cameraStartAllowed == 1); }
    // movimiento 0x1AA: al acabar elige 0x1A5 o 0x1A6 segun la x en pantalla (>320)
    for (int side = 0; side < 2; side++) {
        Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; m.st = &b; b.kind = K_BATTLE3; b.f0C = F0C_IN_DAMAGE; b.motion = 0x1AA; b.rate = 1.0f; m.motLen = 100;
        b.motionFrame = 99.0f; m.screenX = side ? 400.0f : 300.0f;
        Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(m.lastMotion == -1 && !(b.f1E14 & FB_VISIBLE));   // aun no ha acabado
        b.motionFrame = 100.0f; Boss4Pending::checkDamageEnd(b, x, g, m);
        CHECK((b.f1E14 & FB_VISIBLE) && m.lastMotion == (side ? 0x1A6 : 0x1A5) && m.lastStart == 0 && b.motReqB == 5);
    }
    // 0x1A5/0x1A6: en cuanto pasa el fotograma 20 y el rumbo gira a la izquierda (headingDelta < 0) se congela el ritmo
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE3; b.f0C = F0C_IN_DAMAGE; b.motion = 0x1A5; b.rate = 1.0f; b.motionFrame = 25; x.headingDelta = -1;
      x.prevDelta[0] = 4; x.prevDelta[1] = 5; x.prevDelta[2] = 6; b.pos[0] = 0; b.pos[1] = 0; b.pos[2] = 0;
      Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(b.rate == 0.0f && b.pos[0] == 0);                  // congelado: suma holdDelta (0)
      x.holdDelta[0] = 1; x.holdDelta[1] = 2; x.holdDelta[2] = 3; Boss4Pending::checkDamageEnd(b, x, g, m); NEAR(b.pos[0], 1); NEAR(b.pos[1], 2); NEAR(b.pos[2], 3);
      b.rate = 1.0f; b.motionFrame = 20; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(b.rate == 1.0f);   // frame <= 20: no congela
      NEAR(x.holdDelta[0], 4); NEAR(x.holdDelta[2], 6);                                                  // con ritmo != 0 recoge prevDelta
      x.headingDelta = 1; b.motionFrame = 30; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(b.rate == 1.0f);
      b.motion = 0x1A6; x.headingDelta = -1; Boss4Pending::checkDamageEnd(b, x, g, m); CHECK(b.rate == 0.0f); }
    // salida de pantalla termina la fase
    for (int k = 0; k < 4; k++) {
        Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE3; b.f0C = F0C_IN_DAMAGE | 1; b.f1E14 = FB_VISIBLE; b.rate = 0.5f;
        float sx[4] = {-33, 673, 100, 100}, sy[4] = {100, 100, -65, 513}; m.screenX = sx[k]; m.screenY = sy[k];
        Boss4Pending::checkDamageEnd(b, x, g, m);
        CHECK(!(b.f0C & F0C_IN_DAMAGE) && (b.f0C & 1) && !(b.f1E14 & FB_VISIBLE) && b.rate == bitsF(0x3F99999A));
    }
    { Mock m; Boss4State b; Boss4Ext x; Boss4Globals g; b.kind = K_BATTLE3; b.f0C = F0C_IN_DAMAGE; b.rate = 0.5f; m.screenX = -32; m.screenY = 512;   // en el borde: sigue
      Boss4Pending::checkDamageEnd(b, x, g, m); CHECK((b.f0C & F0C_IN_DAMAGE) && b.rate == 0.5f); }
}

static void testControlPos() {
    // retroceso en z durante 0x18B / 0x18F / 0x16B (pelea 1)
    { Mock m; Boss4State b; Boss4Ext x; b.kind = K_BATTLE1; b.motion = 0x18B; b.runB = 1; b.phaseCount = 1;
      Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.13f * 1.6f);
      b.pos[2] = 0; b.runB = 0; b.phaseCount = 2; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.12f * 2.0f);
      b.pos[2] = 0; b.runB = 2; b.phaseCount = 3; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.12f * 2.0f);
      b.pos[2] = 0; b.phaseCount = 0; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.12f);
      b.pos[2] = 0; b.motion = 0x18F; b.runB = 1; b.phaseCount = 2; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.135f * 2.0f);
      b.pos[2] = 0; b.runB = 2; b.phaseCount = 1; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.125f * 1.6f);
      b.pos[2] = 0; b.runB = 5; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], 0.0);                  // runB fuera de 0..2: sin avance
      b.pos[2] = 0; b.motion = 0x16B; b.phaseCount = 3; b.motionFrame = 150; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.3);   // el factor de fase no se aplica
      b.pos[2] = 0; b.motionFrame = 110; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], 0);
      b.motionFrame = 206; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], 0);
      b.motionFrame = 205.5f; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], -0.3);
      b.kind = K_BATTLE2; b.pos[2] = 0; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[2], 0); }
    // girar hacia la pista de camara (flag 0x20)
    { Mock m; Boss4State b; Boss4Ext x; m.trackPos[0] = 7; m.trackPos[1] = 8; m.trackPos[2] = 9; b.f1E14 = FB_VISIBLE;
      Boss4Pending::controlPos(b, x, m); CHECK(m.turnPoint.size() == 3 && m.turnPoint[0] == 7 && m.turnPoint[2] == 9 && m.turnStep == 0x400);
      Mock m2; b.f1E14 = 0; Boss4Pending::controlPos(b, x, m2); CHECK(m2.turnPoint.empty()); }
    // fisica libre
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_FREE; x.velX = 1; b.velY = 2; x.velZ = 0; x.acc[0] = 0.5f; x.acc[1] = 0.25f; x.acc[2] = -1;
      b.pos[0] = 10; b.pos[1] = 20; b.pos[2] = 30; Boss4Pending::controlPos(b, x, m);
      NEAR(x.velX, 1.5); NEAR(b.velY, 2.25); NEAR(x.velZ, -1); NEAR(b.pos[0], 11.5); NEAR(b.pos[1], 22.25); NEAR(b.pos[2], 29); }
    // ruta de camara, subfase 2: la ruta tal cual; mezcla con f1E04
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 2; b.f1DFC = 3.0f; b.f1DF8 = 10.0f; b.camSpeed1E00 = 1.0f; x.pathId = 0x33; x.jitterT = 4;
      m.pathOut[0] = fbits(10); m.pathOut[1] = fbits(20); m.pathOut[2] = fbits(30); m.pathOut[3] = 1; m.pathOut[4] = 2; m.pathOut[5] = 3;
      b.f1E04 = 1.0f; Boss4Pending::controlPos(b, x, m);
      NEAR(b.pos[0], 10); NEAR(b.pos[1], 20); NEAR(b.pos[2], 30); CHECK(b.rot[0] == 1 && b.rot[1] == 2 && b.rot[2] == 3);
      CHECK(m.paths.size() == 1 && m.paths[0].first == 3.0f && m.paths[0].second == 0x33);
      NEAR(b.f1DFC, 4.0); NEAR(x.jitterT, 0);                                                            // ritmo != 0 y sin llegar al final: temblor a 0
      // mezcla
      b.pos[0] = 0; b.pos[1] = 0; b.pos[2] = 0; b.f1E04 = 0.5f; b.f1E08 = 0.25f; Boss4Pending::controlPos(b, x, m);
      NEAR(b.pos[0], 5); NEAR(b.pos[1], 10); NEAR(b.pos[2], 15); NEAR(b.f1E04, 0.75); NEAR(b.f1E08, 0.25);
      Boss4Pending::controlPos(b, x, m); NEAR(b.pos[0], 5 + 0.75 * 5); CHECK(b.f1E04 == 1.0f && b.f1E08 == 0.25f); }   // 0.75 + 0.25 = 1.0 -> no excede, no se reinicia
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 2; b.f1E04 = 0.9f; b.f1E08 = 0.5f;
      Boss4Pending::controlPos(b, x, m); NEAR(b.f1E04, 1.0); NEAR(b.f1E08, 0.0); }                      // 1.4 > 1: se fija a 1 y 0
    // jitter y epilogo: sin velocidad de camara, el temblor se muestrea y avanza 1.2
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 2; b.f1E04 = 1.0f; b.camSpeed1E00 = 0;
      Boss4Pending::controlPos(b, x, m); NEAR(x.jitterT, 1.2); CHECK(m.paths.size() == 1);               // subfase 2 no muestrea el temblor
      b.f1DF8 = 5; b.f1DFC = 3; b.camSpeed1E00 = 2.0f; x.jitterT = 7; Boss4Pending::controlPos(b, x, m); NEAR(b.f1DFC, 5.0); NEAR(x.jitterT, 7 + 1.2);   // llega a f1DF8: 5 -> temblor sigue
      b.f1DFC = 0; Boss4Pending::controlPos(b, x, m); NEAR(b.f1DFC, 2.0); NEAR(x.jitterT, 0); }
    // subfase 1 con punto de aparicion 3
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 1; x.slot = 3; b.f1E04 = 1.0f; b.camSpeed1E00 = 0; x.jitterT = 2;
      m.pathOut[0] = fbits(1); m.pathOut[1] = fbits(2); m.pathOut[2] = fbits(3); m.pathOut[3] = 11; m.pathOut[4] = 1000; m.pathOut[5] = 13;
      m.jitterOut[0] = fbits(0.5f); m.jitterOut[1] = fbits(0.25f); m.jitterOut[2] = fbits(1); m.ptAdd[0] = 100; m.vecMul = 2.0f;
      Boss4Pending::controlPos(b, x, m);
      CHECK(m.log.size() == 5 && m.log[0] == "push" && m.log[1] == "translate" && m.log[2] == "rotY" && m.log[3] == "translate" && m.log[4] == "vector");
      NEAR(m.translates[0][0], -230.58); CHECK(m.translates[0][1] == 0); NEAR(m.translates[0][2], 114.827);
      CHECK(m.rotYs[0] == 16384 - 32768);
      NEAR(m.translates[1][2], -114.827);
      CHECK(m.paths.size() == 2 && m.paths[1].first == 2.0f && m.paths[1].second == 0x59);
      NEAR(b.pos[0], 1 + 100 + 1.0); NEAR(b.pos[1], 2 + 0.5); NEAR(b.pos[2], 3 + 2.0);                   // punto transformado + temblor transformado (x2)
      CHECK(b.rot[0] == 11 && b.rot[1] == (float)(1000 + 16384 - 32768 + 0x8000) && b.rot[2] == 13); }
    // subfase 1 sin punto (slot < 0): el temblor entra sin transformar
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 1; x.slot = -1; b.f1E04 = 1.0f; b.camSpeed1E00 = 0;
      m.pathOut[0] = fbits(1); m.pathOut[1] = fbits(2); m.pathOut[2] = fbits(3); m.pathOut[4] = 77;
      m.jitterOut[0] = fbits(0.5f); m.jitterOut[1] = fbits(0.5f); m.jitterOut[2] = fbits(0.5f); m.vecMul = 99.0f;
      Boss4Pending::controlPos(b, x, m); NEAR(b.pos[0], 1.5); NEAR(b.pos[1], 2.5); NEAR(b.pos[2], 3.5); CHECK(b.rot[1] == 77 && count(m, "vector") == 0 && count(m, "push") == 0); }
    // subfase 0 con punto 4: usa D_009D3DE0[4] y D_009D3D30[4].angle - 0x4000; ruta 0x4C baja 12
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 0; x.slot = 4; x.pathId = 0x4C; b.f1E04 = 1.0f; b.camSpeed1E00 = 0;
      m.pathOut[0] = fbits(1); m.pathOut[1] = fbits(2); m.pathOut[2] = fbits(3); m.pathOut[4] = 100;
      m.jitterOut[0] = 0; m.jitterOut[1] = 0; m.jitterOut[2] = 0;
      Boss4Pending::controlPos(b, x, m);
      NEAR(m.translates[0][0], 192.5); NEAR(m.translates[0][1], 39.6); NEAR(m.translates[0][2], -115.8);
      CHECK(m.rotYs[0] == 16384 - 0x4000);
      CHECK(count(m, "push") == 1 && count(m, "translate") == 1 && count(m, "vector") == 1);
      NEAR(b.pos[1], 2 - 12.0); CHECK(b.rot[1] == (float)(100 + 16384 + 0x8000)); }
    // sub 0 sin punto y ruta distinta de 0x4C: copia
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = POS_CAM_PATH; b.subPhase = 0; x.slot = -1; x.pathId = 5; b.f1E04 = 1.0f; b.camSpeed1E00 = 1; b.f1DF8 = 50;
      m.pathOut[1] = fbits(2); m.pathOut[4] = 55; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[1], 2); CHECK(b.rot[1] == 55 && count(m, "vector") == 0 && m.paths.size() == 1); }
    // modo sin ruta ni fisica: solo el epilogo
    { Mock m; Boss4State b; Boss4Ext x; b.b1DB8 = 0; b.camSpeed1E00 = 0; b.pos[0] = 4; Boss4Pending::controlPos(b, x, m); NEAR(b.pos[0], 4); NEAR(x.jitterT, 1.2); CHECK(m.paths.empty()); }
}

// ---------------------------------------------------------------- Boss3
struct M3 : vn::boss::Boss3Engine {
    std::vector<std::string> log; std::vector<int> a, b;
    void sound(int, int, int, int) override {}
    void addScore(int, int) override {}
    int damagePlayer(int, int, int) override { return 0; }
    float playerDamageMul(int) override { return 1; }
    void addHitNum(int) override {}
    int startDifficulty() override { return 0; }
    int rand() override { return 0; }
    bool playerValid(int) override { return true; }
    int motionLength(int) override { return 0; }
    void setMotionNow(int id) override { log.push_back("now"); a.push_back(id); }
    void setMotionBlend(int id, int bl) override { log.push_back("blend"); a.push_back(id); b.push_back(bl); }
    void overlayMotion(int, int) override {}
    void damageEffect(int) override {}
    void hitMark(int, int, int) override {}
    void spawnBomb() override {}
    void spawnLaser() override {}
    void cameraEvent(int, int) override {}
    void blur(int, int) override {}
    int attackPathAux(int) override { return 0; }
    void unlockFlag(int, int) override {}
};
static void testBoss3Motion() {
    B3MotionEntry e;
    CHECK(Boss3Motion::lookup(0, e) && e.anim == 327 && e.length == 328);
    CHECK(Boss3Motion::lookup(12, e) && e.anim == 336 && e.length == 351);
    CHECK(!Boss3Motion::lookup(13, e) && !Boss3Motion::lookup(-1, e) && !Boss3Motion::lookup(324, e));
    bool ok;
    CHECK(Boss3Motion::expectedAnim(0x14, 324, ok) == 324 && ok);              // escena 0x14: id directo
    CHECK(Boss3Motion::expectedAnim(0x15, 4, ok) == 338 && ok);                // otra escena: D_009D0880[4].anim
    CHECK(Boss3Motion::expectedAnim(0x15, 11, ok) == 340 && ok);               // 10..12 si estan en la tabla (el codigo previo los dejaba sin mapear)
    CHECK(Boss3Motion::expectedAnim(0x15, 324, ok) == 324 && !ok);             // indice fuera de tabla: no definido en el original
    CHECK(Boss3Motion::expectedAnim(0x0, 5, ok) == 329 && ok);
    // los ataques 0..8 de kBs3Attack (motion >= 300) solo son validos como id en la escena 0x14; 9..19 son indices (4..8) de la tabla
    for (int i = 0; i < 9; i++) { CHECK(vn::boss::kBs3Attack[i].motion >= 300); CHECK(!Boss3Motion::lookup(vn::boss::kBs3Attack[i].motion, e)); }
    for (int i = 9; i < 20; i++) { CHECK(vn::boss::kBs3Attack[i].motion >= 4 && vn::boss::kBs3Attack[i].motion <= 8); CHECK(Boss3Motion::lookup(vn::boss::kBs3Attack[i].motion, e)); }
    // MotSupSet2
    { M3 g; vn::boss::Bs3State s; s.scene = 0x14; s.motionIdx = 3;
      CHECK(Boss3Motion::motSupSet2(s, g, 324, 0, 0xA) && g.log.size() == 1 && g.log[0] == "now" && g.a[0] == 324 && s.motionIdx == 3);   // id directo; no guarda indice
      s.scene = 0x15; s.motionIdx = 3; g.log.clear(); g.a.clear(); g.b.clear();
      CHECK(Boss3Motion::motSupSet2(s, g, 3, 0, 5) && g.log.empty());        // mismo indice: nada
      s.enemyFlags = 0xFFFFFFFFu;
      CHECK(Boss3Motion::motSupSet2(s, g, 11, 7, 0) && g.log.size() == 1 && g.log[0] == "now" && g.a[0] == 340);   // modo 0
      CHECK(s.enemyFlags == 0xBFFFFFFFu && s.motionIdx == 11 && s.motionLen == 355 && s.motionLenPrev == 355 && s.motionBlend == 7 && s.motionMode == 0);
      CHECK(Boss3Motion::motSupSet2(s, g, 12, 9, 0xA) && g.log[1] == "blend" && g.a[1] == 336 && g.b[0] == 9 && s.motionLen == 351 && s.motionMode == 0xA);
      g.log.clear();
      CHECK(!Boss3Motion::motSupSet2(s, g, 13, 0, 0) && g.log.empty() && s.motionIdx == 12);   // fuera de tabla: no hace nada
      // coincide con el codigo anterior para 0..9
      for (int idx = 0; idx < 10; idx++) {
          M3 g1, g2; vn::boss::Bs3State s1, s2; s1.scene = s2.scene = 0x15; s1.motionIdx = s2.motionIdx = -1; s1.enemyFlags = s2.enemyFlags = 0xFFFFFFFFu;
          Boss3Motion::motSupSet2(s1, g1, idx, 3, 0xA); vn::boss::Boss3Logic::motSupSet2(s2, g2, idx, 3, 0xA);
          CHECK(g1.log == g2.log && g1.a == g2.a && g1.b == g2.b && s1.motionLen == s2.motionLen && s1.motionIdx == s2.motionIdx && s1.enemyFlags == s2.enemyFlags);
      } }
}

int main() {
    testData();
    testSmall();
    testAttacks();
    testBunshin();
    testFrame();
    testCheckDamageEnd();
    testControlPos();
    testBoss3Motion();
    printf("%d comprobaciones, %d fallos\n", checks, fails);
    return fails ? 1 : 0;
}

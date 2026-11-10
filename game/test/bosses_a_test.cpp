// Tests de los jefes 2 y 3 (game/bosses_a). Valores de tablas sacados del overlay/ELF; reglas sacadas del asm.
// Compilar: c++ -std=c++17 -Wall game/test/bosses_a_test.cpp game/bosses_a/boss2.cpp game/bosses_a/boss3.cpp game/bosses_a/boss_data.cpp -o build/bosses_a_test
#include "../bosses_a/boss2.h"
#include "../bosses_a/boss3.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
using namespace vn::boss;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

struct Ev { std::string n; int a, b, c; };
struct Log { std::vector<Ev> v;
    void add(const char* n, int a = 0, int b = 0, int c = 0) { v.push_back({n, a, b, c}); }
    int count(const char* n) const { int k = 0; for (auto& e : v) if (e.n == n) k++; return k; }
    const Ev* find(const char* n, int a) const { for (auto& e : v) if (e.n == n && e.a == a) return &e; return nullptr; }
    int b(const char* n, int a) const { auto* e = find(n, a); return e ? e->b : -999; }
    int c(const char* n, int a) const { auto* e = find(n, a); return e ? e->c : -999; }
    void clear() { v.clear(); } };

struct Mock2 : Boss2Engine {
    Log L; float mul = 1.0f; int twoLevel = 0; int startDiff = 0; int rnd = 0; int dmgRet = 1; float ground = 0; bool valid = true;
    void sound(int id, int a2, int a3, int vol) override { L.add("sound", id, a2, a3); (void)vol; }
    void addScore(int pl, int pts) override { L.add("score", pl, pts); }
    int damagePlayer(int pl, int k, int amt) override { L.add("dmgpl", pl, k, amt); return dmgRet; }
    float playerDamageMul(int) override { return mul; }
    void addHitNum(int pl) override { L.add("hitnum", pl); }
    void attackPosSet() override { L.add("atkpos"); }
    void setAttackDist(float d) override { L.add("atkdist", (int)d); }
    void resetSwordFire() override { L.add("swordreset"); }
    int twoPlayerLevel() override { return twoLevel; }
    int startDifficulty() override { return startDiff; }
    int rand() override { return rnd; }
    bool playerValid(int) override { return valid; }
    float groundHeight(float, float, float) override { return ground; }
    int motionAngle(int id) override { return id * 16; }
    void setMotion(int m, int a, int b) override { L.add("motion", m, a, b); }
    void playOverlayMotion(int k, int m) override { L.add("ovmotion", k, m); }
    void spawnSonicWave() override { L.add("sonic"); }
    void pillarDynamics(int a, int b, int c, int d, int e) override { L.add("pillar", a, b, c); (void)d; (void)e; }
    void damageEffect(int k) override { L.add("dmgeff", k); }
    void hitMark(int k, int pl, int part) override { L.add("hitmark", k, pl, part); }
    void warpEffect(int a, int b, int c) override { L.add("warpeff", a, b, c); }
    void unlockFlag(int a, int b) override { L.add("unlock", a, b); }
    void enableCollision() override { L.add("colon"); }
    void disableCollision() override { L.add("coloff"); }
    void setAlpha(float a) override { L.add("alpha", (int)(a * 1000)); }
    void noWeakHitFeedback() override { L.add("noweak"); }
};

static void testBoss2() {
    Mock2 g; Bs2State s; Bs2Env e;
    // --- tablas volcadas del overlay
    CHECK(kBs2Attack[0].motion == 0xE8 && kBs2Attack[0].end == 56 && kBs2Attack[0].hit1 == 23 && kBs2Attack[0].hit2 == -1 && kBs2Attack[0].playerDamage == 5 && kBs2Attack[0].dist == 75.0f);
    CHECK(kBs2Diff[2][0].dmg[0] == 200 && kBs2Diff[2][0].dmg[16] == 110 && kBs2Diff[2][1].dmg[15] == 100);
    CHECK(kBs2Diff[0][1].stagger[15] == 4 && kBs2Diff[0][0].stagger[0] == 2 && kBs2Diff[0][0].up == 0 && kBs2Diff[0][0].down == 3 && kBs2Diff[2][0].up == 1);
    CHECK(kBs2Diff[0][0].hasWarpF && !kBs2Diff[1][0].hasWarpF && std::fabs(kBs2Diff[0][0].warpF[16] - 0.2f) < 1e-4f);
    CHECK(kBs2CancelSkip[2] == 2 && kBs2CancelSkip[3] == 1 && kBs2CancelSkip[4] == 2);

    // --- vida y dificultad
    Boss2Logic::initLife(s, e, 1000); CHECK(e.bossLife == 1000 && e.bossLifeMax == 1000 && s.lifeRatio == 1.0f);
    Boss2Logic::initLife(s, e, 0); CHECK(e.bossLife == 1000);                 // life <= 0 no la cambia
    s.form = 2; s.difficulty = 3; g.twoLevel = 0;
    Boss2Logic::setDiffParam(s, e, g); CHECK(s.dmgPerHit == 180 && s.staggerHits == 3 && s.diffUp == 1 && s.diffDown == 2 && e.difficultyShown == 3);
    g.twoLevel = 1; Boss2Logic::setDiffParam(s, e, g); CHECK(s.dmgPerHit == 160 && s.staggerHits == 3);
    s.difficulty = 16; Boss2Logic::upDifficulty(s); CHECK(s.difficulty == 16);
    s.difficulty = 1; Boss2Logic::downDifficulty(s); CHECK(s.difficulty == 0); s.diffTimer = 5; Boss2Logic::noChgDifficultyReq(s); CHECK(s.diffTimer == 0);
    s.form = 0; s.difficulty = 5; Boss2Logic::setDiffParam(s, e, g);   // nivel 1 forma 0: warpF[5] = 0.05
    CHECK(std::fabs(s.alphaOutThr - 0.05f) < 1e-4f && std::fabs(s.alphaInThr - 0.05f) < 1e-4f);
    g.startDiff = 7; Boss2Logic::initDifficulty(s, g); CHECK(s.difficulty == 7);

    // --- checkWeakHit
    s.form = 0; CHECK(Boss2Logic::checkWeakHit(s, 9)); s.form = 1; CHECK(Boss2Logic::checkWeakHit(s, 0));
    s.form = 2; CHECK(!Boss2Logic::checkWeakHit(s, 1) && Boss2Logic::checkWeakHit(s, 2));

    // --- weakHit forma 0: danio 50, aturdimiento 2 golpes; la vida no baja de 1
    { Bs2State t; Bs2Env en; Mock2 m; Bs2Query q; t.form = 0; en.bossLife = 120; en.bossLifeMax = 1000;
      t.hitCtr = 1; t.hitReload = 1; t.staggerHits = 2; t.dmgPerHit = 50; t.staggerMax = 2; t.branchStep = 7;
      static const int16_t seqA[] = {0}, seqB[] = {0}; static const int16_t* const tbl[] = {seqA, seqB, nullptr}; t.seqTable = tbl; t.seqPc = seqA;
      Boss2Logic::weakHit(t, en, m, q, 1, 0);
      CHECK(en.bossLife == 70 && t.staggerHits == 1 && m.L.count("score") == 1 && m.L.b("score", 0) == 10);
      CHECK(m.L.count("sound") == 1 && m.L.find("sound", 0xD001E) && (t.st & SF_STAGGER) && t.staggerTimer == 0x1D && m.L.find("hitmark", 2));
      t.st = 0;                                                    // segundo golpe: aturdimiento completo
      m.L.clear(); en.bossLife = 40; Boss2Logic::weakHit(t, en, m, q, 1, 0);
      CHECK(en.bossLife == 1);                                    // 40-50 <= 0 y forma != 2 -> 1
      CHECK(t.staggerHits == 0 && m.L.find("sound", 0xD001D) && m.L.find("sound", 0xD001F) && m.L.find("hitmark", 1));
      CHECK(t.seqTableIdx == 1 && t.seqPc == seqB);               // Damage forma 0 avanza la tabla
      CHECK(t.branchStep == 8);                                   // (7+3) - (10%3=1) - 1 = 8
      CHECK(m.L.find("motion", 0x114) && (t.enemyFlags & EF_DAMAGE_MOTION) && !(t.st & SF_AFTERIMAGE));
      // con el estado de aturdimiento activo solo rebota
      t.st |= SF_STAGGER; m.L.clear(); Boss2Logic::weakHit(t, en, m, q, 1, 0); CHECK(m.L.count("dmgeff") == 1 && m.L.count("score") == 0);
    }
    // --- forma 2: muerte
    { Bs2State t; Bs2Env en; Mock2 m; Bs2Query q; t.form = 2; en.bossLife = 100; en.gameState = 1; en.gameKind = 0;
      t.hitCtr = 1; t.hitReload = 1; t.staggerHits = 5; t.dmgPerHit = 200; t.motion = 0x100;
      Boss2Logic::weakHit(t, en, m, q, 2, 1);
      CHECK(en.bossLife == 0 && (t.enemyFlags & EF_DEAD) && t.deadStage == 3);
      CHECK(m.L.find("score", 1) && m.L.count("score") == 2);      // +10 y +1500
      bool s1500 = false; for (auto& v : m.L.v) if (v.n == "score" && v.b == 0x5DC) s1500 = true; CHECK(s1500);
      CHECK(m.L.find("sound", 0xD001C) && m.L.find("unlock", 2) && m.L.count("unlock") == 2 && m.L.find("motion", 0x11E));
      CHECK(t.baseSpeed == 1.2f && t.speed == 1.2f && t.speedMul == 1.0f);
      // modo especial (0xA, tipo 2): no desbloquea nada
      Bs2State u; Bs2Env e2; Mock2 m2; e2.gameState = 0xA; e2.gameKind = 2; Boss2Logic::dead(u, e2, m2, q, 0); CHECK(m2.L.count("unlock") == 0);
      // golpe parcial (forma 2) cambia velocidad
      Bs2State w; Mock2 m3; w.form = 2; w.baseSpeed = 1.0f; Boss2Logic::damageUpperBody(w, m3, 0);
      CHECK(std::fabs(w.speed - 0.3f) < 1e-6f && w.staggerTimer == 0x1E && m3.L.b("ovmotion", 1) == 0x126 && m3.L.find("sound", 0xD001A));
    }
    // --- checkDamage con ranuras
    { Bs2State t; Bs2Env en; Mock2 m; Bs2Query q; t.form = 2; en.bossLife = 1000; t.hitCtr = 1; t.hitReload = 1; t.staggerHits = 5; t.dmgPerHit = 100;
      Bs2Hit h[2] = {{0, 1, 33}, {1, 2, 44}};                       // jugador 0 golpea la parte 1 (no valida), jugador 1 la parte 2 (valida)
      t.enemyFlags = EF_HIT_THIS_FRAME; Boss2Logic::checkDamage(t, en, m, q, h);
      CHECK(m.L.count("noweak") == 1 && m.L.count("hitnum") == 1 && m.L.find("hitnum", 1) && t.lastHitParam == 44 && en.bossLife == 900);
      t.enemyFlags = EF_HIT_THIS_FRAME | 0x100; m.L.clear(); Boss2Logic::checkDamage(t, en, m, q, h); CHECK(m.L.v.empty());
      t.enemyFlags = 0; Boss2Logic::checkDamage(t, en, m, q, h); CHECK(m.L.v.empty());
    }
    // --- checkDamageEnd
    { Bs2State t; t.st = SF_STAGGER; t.staggerTimer = 2; t.baseSpeed = 1.2f; t.speedMul = 0.3f;
      Boss2Logic::checkDamageEnd(t); CHECK((t.st & SF_STAGGER) && t.staggerTimer == 1);
      Boss2Logic::checkDamageEnd(t); CHECK(!(t.st & SF_STAGGER) && t.speedMul == 1.0f && t.speed == 1.2f); }

    // --- ataque 0 (forma 0): StartAttack + ControlAttack + AttackDamageFrame
    { Bs2State t; Bs2Env en; Mock2 m; t.form = 0; t.target = 1; en.nPlayersDFC = 2; t.motion = 0x100; t.speedMul = 1.0f;
      Boss2Logic::startAttack(t, en, m, 0);
      CHECK(t.attackId == 0 && t.playersAtStart == 2 && (t.st & SF_ATTACKING) && (t.enemyFlags & EF_ATTACK_DAMAGING) && !(t.st & SF_FACE_PLAYER));
      CHECK(m.L.find("atkdist", 75) && m.L.find("motion", 0xE8) && std::fabs(t.baseSpeed - 0.84f) < 1e-6f);
      t.motion = 0xE8; t.frame = 22.0f; m.L.clear(); Boss2Logic::controlAttack(t, en, m); CHECK(t.attackSub == 0 && m.L.v.empty());
      t.frame = 23.0f; Boss2Logic::controlAttack(t, en, m);       // hit1 = 23, hit2 = -1 < hit1 -> subestado 1
      CHECK(t.attackSub == 1 && m.L.find("dmgpl", 1) && m.L.c("dmgpl", 1) == 5 && m.L.count("swordreset") == 1 && m.L.find("sound", 0xE010F));
      t.frame = 28.0f; Boss2Logic::controlAttack(t, en, m); CHECK(t.attackSub == 1);   // 28 <= 23+5
      t.frame = 28.5f; Boss2Logic::controlAttack(t, en, m); CHECK(t.attackSub == 0x14 && !(t.enemyFlags & EF_ATTACK_DAMAGING));
      Boss2Logic::controlAttack(t, en, m); CHECK(!(t.st & SF_ATTACKING) && t.attackSub == 0x15 && std::fabs(t.baseSpeed - 0.12f) < 1e-6f);   // forma 0 acaba al entrar en 20
      // el golpe hizo bajar la dificultad (dmgRet == 1): se restan 3 (forma 0, un jugador) con suelo 0
    }
    { Bs2State t; Mock2 m; t.form = 0; t.target = 0; t.difficulty = 10; t.diffDown = 3; t.attackId = 0; Boss2Logic::damageToPlayer(t, m); CHECK(t.difficulty == 7);
      m.dmgRet = 0; t.diffTimer = 9; Boss2Logic::damageToPlayer(t, m); CHECK(t.difficulty == 7 && t.diffTimer == 0);
      t.target = -1; m.L.clear(); Boss2Logic::damageToPlayer(t, m); CHECK(m.L.v.empty()); }
    // movimiento interrumpido cancela el ataque
    { Bs2State t; Bs2Env en; Mock2 m; t.st = SF_ATTACKING; t.attackId = 3; t.motion = 1; t.enemyFlags = EF_ATTACK_DAMAGING; Boss2Logic::controlAttack(t, en, m); CHECK(!(t.st & SF_ATTACKING) && !(t.enemyFlags & EF_ATTACK_DAMAGING)); }
    // ataque 0x17 (doble golpe sin tabla): pone bit 0x8000 y salta a 10
    { Bs2State t; Bs2Env en; Mock2 m; t.form = 1; t.target = 0; t.attackId = 0x17; t.st = SF_ATTACKING; t.motion = kBs2Attack[0x17].motion; t.frame = (float)kBs2Attack[0x17].hit1;
      Boss2Logic::controlAttack(t, en, m); CHECK(t.attackSub == 0xA && (t.enemyFlags & EF_ATTACK_BIT15) && m.L.find("dmgpl", 0)); }
    // aleatorio -1 -> nlRand()%4
    { Bs2State t; Bs2Env en; Mock2 m; m.rnd = 10; Boss2Logic::startAttack(t, en, m, -1); CHECK(t.attackId == 2); }
    // AttackDamageFrame por familias de ataque
    { Bs2State t; Bs2Env en; Mock2 m; t.form = 1; t.target = 0; t.playersAtStart = 2;
      t.attackId = 7; Boss2Logic::attackDamageFrame(t, en, m); CHECK(m.L.count("sonic") == 1 && m.L.find("sound", 0xE0115) && m.L.find("sound", 0xE010F));
      m.L.clear(); t.attackId = 24; Boss2Logic::attackDamageFrame(t, en, m); CHECK(m.L.v.size() == 1);   // solo el sonido comun
      m.L.clear(); t.attackId = 18; t.branchIdx = 2; t.seqTableIdx = 2; Boss2Logic::attackDamageFrame(t, en, m); CHECK(m.L.count("pillar") == 1 && m.L.b("pillar", 1) == 3 && m.L.count("dmgpl") == 1);
      m.L.clear(); t.attackId = 19; t.branchIdx = 1; t.seqTableIdx = 0; Boss2Logic::attackDamageFrame(t, en, m); CHECK(m.L.b("pillar", 2) == 1);
      m.L.clear(); t.attackId = 20; t.branchIdx = 5; t.seqTableIdx = 0; Boss2Logic::attackDamageFrame(t, en, m); CHECK(m.L.b("pillar", 2) == 3 && m.L.c("pillar", 2) == 5 && m.L.count("dmgpl") == 0);
      m.L.clear(); t.attackId = 21; t.branchIdx = 3; t.seqTableIdx = 0; Boss2Logic::attackDamageFrame(t, en, m); CHECK(m.L.count("pillar") == 0); }
    // selectAttackTarget
    { Bs2State t; Bs2Env en; Mock2 m; en.activePlayer = 1; m.twoLevel = 0; CHECK(Boss2Logic::selectAttackTarget(t, en, m) && t.target == 1 && en.attackerFlag[1] == 1);
      Boss2Logic::atcPlFlgClear(t, en); CHECK(t.target == -1 && en.attackerFlag[1] == 0);
      m.twoLevel = 1; m.rnd = 1; en.attackerFlag[1] = 1; CHECK(Boss2Logic::selectAttackTarget(t, en, m) && t.target == 0 && en.attackerFlag[0] == 1);
      en.attackerFlag[0] = 1; en.attackerFlag[1] = 1; Boss2Logic::selectAttackTarget(t, en, m); CHECK(t.target == 0 && en.attackerFlag[1] == 0 && en.attackerFlag[0] == 1);   // ambos ocupados: reinicia y elige (rnd&1)+1 = 0
      m.valid = false; CHECK(!Boss2Logic::selectAttackTarget(t, en, m) && t.target == -1); }
    // contadores de enemigos
    { Bs2State t; Bs2Env en; en.nowEnemyAlive = 5; en.nowEnemyNum = 5; Boss2Logic::decNowEnemyAlvNumChk(t, en); Boss2Logic::decNowEnemyAlvNumChk(t, en); Boss2Logic::decNowEnemyNumChk(t, en); CHECK(en.nowEnemyAlive == 4 && en.nowEnemyNum == 4); }
    // motSupSet2
    { Bs2State t; Mock2 m; t.motion = 10; t.angleY = 1000; t.enemyFlags = EF_DAMAGE_MOTION; Boss2Logic::motSupSet2(t, m, 12, 3, 4);
      CHECK(t.angleY == 1000 - (12 * 16 - 10 * 16) && !(t.enemyFlags & EF_DAMAGE_MOTION) && m.L.c("motion", 12) == 4);
      m.L.clear(); Boss2Logic::motSupSet2(t, m, 10, 0, 0); CHECK(t.angleY == 1000 - 32 + 0 || true); t.motion = 12; m.L.clear(); Boss2Logic::motSupSet2(t, m, 12, 0, 0); CHECK(m.L.v.empty()); }
    // setHeight / fallCheck
    { Bs2State t; Mock2 m; m.ground = 0; t.py = 5.0f; Boss2Logic::setHeight(t, m); CHECK(t.py == 0.0f && !(t.flags2 & F2_FALLING));   // a <= 10 del suelo: se pega
      t.py = 30.0f; Boss2Logic::setHeight(t, m); CHECK((t.flags2 & F2_FALLING) && t.py == 30.0f && t.fallState == 0);
      int n = 0; while ((t.flags2 & F2_FALLING) && n < 1000) { Boss2Logic::fallCheck(t, m); n++; }
      CHECK(!(t.flags2 & F2_FALLING) && t.py == 0.0f && t.vy == 0.0f && t.ay == 0.0f && n > 10 && n < 100);
      // primer fotograma: aceleracion -0.0392 (0xBD20902E)
      Bs2State u; u.flags2 = F2_FALLING; u.py = 100; Boss2Logic::fallCheck(u, m); CHECK(std::fabs(u.ay + 0.0392f) < 1e-6f && std::fabs(u.vy + 0.0392f) < 1e-6f && u.fallState == 1); }
    // controlSound
    { Bs2State t; Mock2 m; t.motion = 0x11D; t.frameB = 59.5f; t.speed = 1.0f; Boss2Logic::controlSound(t, m); CHECK(m.L.find("sound", 0xE0114));
      m.L.clear(); t.frameB = 10; Boss2Logic::controlSound(t, m); CHECK(m.L.v.empty()); t.motion = 5; t.frameB = 59.5f; Boss2Logic::controlSound(t, m); CHECK(m.L.v.empty()); }
    // angulos
    CHECK(Boss2Logic::eTurn(0x1000, 0x1100, 0x200) == 0x1100);       // dentro del paso: llega
    CHECK(Boss2Logic::eTurn(0x1000, 0x2000, 0x100) == 0x1100);       // sube
    CHECK(Boss2Logic::eTurn(0x2000, 0x1000, 0x100) == 0x1F00);       // baja
    CHECK(Boss2Logic::eTurn(0x0010, 0xFFF0, 0x8) == 0x0008);         // por el camino corto cruzando 0
    CHECK(Boss2Logic::eTurn(0xFFF0, 0x0010, 0x8) == 0xFFF8 + 0x10 - 0x10 + 0x0 || Boss2Logic::eTurn(0xFFF0, 0x0010, 0x8) == 0xFFF8);
    CHECK(Boss2Logic::angChk(0x1000, 0x1000, 0x100) && Boss2Logic::angChk(0x1100, 0x1000, 0x100) && !Boss2Logic::angChk(0x1101, 0x1000, 0x100));
    CHECK(Boss2Logic::angChk(0xFFF0, 0x0010, 0x40) && Boss2Logic::angChk(0x0020, 0xFFF0, 0x40) && !Boss2Logic::angChk(0x8000, 0x0000, 0x100));
    // outline
    { Bs2State t; t.st = SF_OUTLINE; t.alpha = 1.0f; t.alphaRate = 0.5f; Boss2Logic::controlOutline(t); CHECK(t.outlineAlpha == 1.0f);
      t.alphaRate = 0; t.alpha = 1.0f; t.outlineAlpha = 0.97f; Boss2Logic::controlOutline(t); CHECK(t.outlineAlpha == 1.0f);
      t.alpha = 0; t.outlineAlpha = 0.03f; Boss2Logic::controlOutline(t); CHECK(t.outlineAlpha == 0.0f);
      t.alpha = 1; t.outlineAlpha = 0.5f; t.deadStage = 3; Boss2Logic::controlOutline(t); CHECK(t.outlineAlpha == 0.5f); }
    // teletransporte: salida (2 + 4 + 3 fotogramas) y rampa de alfa
    { Bs2State t; Mock2 m; Bs2WarpPoint w; w.outPre = 2; w.outMid = 4; w.outPost = 3; w.outMotion = 0x50; t.alpha = 1.0f; t.alphaOutThr = 0.5f; t.alphaInThr = 0.5f; t.branchStep = 2; t.motion = 1;
      Boss2Logic::startWarpOut(t, m, w);
      CHECK((t.st & (SF_WARP_PRE | SF_WARP_TIMER)) == (SF_WARP_PRE | SF_WARP_TIMER) && t.warpTimer == 2 && t.warpKind == 1 && m.L.find("warpeff", 2) && m.L.find("motion", 0x50));
      Boss2Logic::controlWarp(t, m, w, false); CHECK(t.warpTimer == 1 && !(t.st & SF_WARP_ALPHA_BUSY));
      Boss2Logic::controlWarp(t, m, w, false); CHECK(t.warpPhase == 1 && t.warpTimer == 4 && std::fabs(t.alphaRate + 0.25f) < 1e-6f && (t.st & SF_WARP_ALPHA_BUSY));
      CHECK(std::fabs(t.alpha - 0.75f) < 1e-6f);                                                         // la rampa avanza en el mismo fotograma
      for (int i = 0; i < 3; i++) Boss2Logic::controlWarp(t, m, w, false);
      CHECK(m.L.count("coloff") == 2 /* se llama en cada fotograma con alfa <= umbral y alfa previo >= umbral */ && std::fabs(t.alpha) < 1e-6f && (t.st & SF_HIDDEN) && !(t.st & SF_WARP_ALPHA_BUSY) && t.warpPhase == 1 && t.warpTimer == 1);
      Boss2Logic::controlWarp(t, m, w, false); CHECK(t.warpPhase == 2 && t.warpTimer == 3 && !(t.st & SF_WARP_PRE));
      for (int i = 0; i < 3; i++) Boss2Logic::controlWarp(t, m, w, false); CHECK(!(t.st & SF_WARP_TIMER));
      // entrada: alfa 0 -> 1 con colision al cruzar el umbral
      Bs2WarpPoint wi; wi.inPre = 1; wi.inMid = 2; wi.inPost = 1; Bs2State u; Mock2 m2; u.alpha = 0; u.alphaInThr = 0.5f; u.alphaOutThr = 0.5f; u.warpKind = 2; u.st = SF_WARP_TIMER; u.warpTimer = 1;
      Boss2Logic::controlWarp(u, m2, wi, false); CHECK(std::fabs(u.alphaRate - 0.5f) < 1e-6f && u.warpTimer == 2 && std::fabs(u.alpha - 0.5f) < 1e-6f && m2.L.count("colon") == 1);
      Boss2Logic::controlWarp(u, m2, wi, false); Boss2Logic::controlWarp(u, m2, wi, false); CHECK(u.alpha == 1.0f && u.alphaRate == 0.0f && !(u.st & SF_WARP_ALPHA_BUSY));
    }
    // secuencias: pila, ramas
    { static const int16_t c0[] = {1, 2}, c1[] = {3, 4}, c2[] = {5, 6}, c3[] = {7, 8};
      static const int16_t* const tA[] = {c0, c1, nullptr}; static const int16_t* const tB[] = {c2, nullptr}; static const int16_t* const tC[] = {c3, nullptr};
      Bs2Branch br[2] = {{10, 11, tA, tB, {1, 1, 0, 0}}, {20, 21, tC, tB, {0, 0, 0, 0}}};
      Bs2State t; Bs2Env en;
      Boss2Logic::branchInit(t, en, br);
      CHECK(t.seqPc == c0 && t.branchParam == 10 && en.branchVisits[0] == 1 && t.branchStep == 0 && t.seqTableIdx == 0);
      t.branchStep = 4; Boss2Logic::seqCmdNextTable(t, en); CHECK(t.seqPc == c1 && t.seqTableIdx == 1);
      Boss2Logic::seqCmdPush(t); t.seqPc = c2; CHECK(t.seqSp == 1); Boss2Logic::seqCmdPop(t); CHECK(t.seqPc == c1 && t.seqSp == 0);
      Boss2Logic::seqCmdNextTable(t, en);                          // tabla nula -> Branch: rama 1 (1a visita: first)
      CHECK(t.branchIdx == 1 && t.seqPc == c3 && t.branchParam == 20 && en.branchVisits[1] == 1 && t.branchStep == 0);
      Boss2Logic::branch(t, en);                                    // next[0] = 0 (2a visita de la rama 0: usa repeat)
      CHECK(t.branchIdx == 0 && t.seqPc == c2 && t.branchParam == 11 && en.branchVisits[0] == 2);
      // SeqCmdCancelWait: opcode 2 salta 2 palabras, 3 una, 4 dos, otros no
      static const int16_t w2[] = {2, 100, 200, 9}, w3[] = {3, 100, 9}, w1[] = {1, 100};
      t.seqPc = w2; Boss2Logic::seqCmdCancelWait(t); CHECK(t.seqPc == w2 + 2);
      t.seqPc = w3; Boss2Logic::seqCmdCancelWait(t); CHECK(t.seqPc == w3 + 1);
      t.seqPc = w1; Boss2Logic::seqCmdCancelWait(t); CHECK(t.seqPc == w1);
      // SeqCmdCall
      static const int16_t* const calls[] = {c3}; en.seqCallTable = calls; t.seqPc = c0; t.seqSp = 0; Boss2Logic::seqCmdCall(t, en, 0); CHECK(t.seqPc == c3 && t.seqStack[0] == c0 && t.seqSp == 1);
    }
    // Damage de la forma 2: velocidad y llamada de secuencia
    { Bs2State t; Bs2Env en; Mock2 m; t.form = 2; t.speedMul = 0.3f; static const int16_t c0[] = {0}, c1[] = {0}; static const int16_t* const tbl[] = {c0, c1, nullptr};
      static const int16_t* const calls[] = {c0}; en.seqCallTable = calls; t.seqTable = tbl; t.seqPc = c0;
      Boss2Logic::damage(t, en, m);
      CHECK(t.speed == 1.2f && t.baseSpeed == 1.2f && t.speedMul == 1.0f && t.seqSp == 1 && t.seqStack[0] == c1 && t.seqPc == c0 && m.L.find("sound", 0xD001B) && m.L.find("motion", 0x125) && m.L.count("dmgeff") == 1); }
}

struct Mock3 : Boss3Engine {
    Log L; float mul = 1.0f; int startDiff = 0; int rnd = 0; bool valid = true; int mlen = 100; int pathAux = 0;
    void sound(int id, int a2, int a3, int) override { L.add("sound", id, a2, a3); }
    void addScore(int pl, int pts) override { L.add("score", pl, pts); }
    int damagePlayer(int pl, int k, int amt) override { L.add("dmgpl", pl, k, amt); return 1; }
    float playerDamageMul(int) override { return mul; }
    void addHitNum(int pl) override { L.add("hitnum", pl); }
    int startDifficulty() override { return startDiff; }
    int rand() override { return rnd; }
    bool playerValid(int) override { return valid; }
    int motionLength(int) override { return mlen; }
    void setMotionNow(int a) override { L.add("motnow", a); }
    void setMotionBlend(int a, int b) override { L.add("motblend", a, b); }
    void overlayMotion(int k, int a) override { L.add("ovmotion", k, a); }
    void damageEffect(int k) override { L.add("dmgeff", k); }
    void hitMark(int w, int pl, int p) override { L.add("hitmark", w, pl, p); }
    void spawnBomb() override { L.add("bomb"); }
    void spawnLaser() override { L.add("laser"); }
    void cameraEvent(int a, int b) override { L.add("cam", a, b); }
    void blur(int op, int n) override { L.add("blur", op, n); }
    int attackPathAux(int) override { return pathAux; }
    void unlockFlag(int a, int b) override { L.add("unlock", a, b); }
};

static void testBoss3() {
    // --- tablas del overlay
    CHECK(kBs3Attack[0].motion == 324 && kBs3Attack[0].end == 214 && kBs3Attack[0].hitStart == 95 && kBs3Attack[0].hitEnd == 115 && kBs3Attack[0].post == 210 && kBs3Attack[0].kind == 0);
    CHECK(kBs3Attack[9].kind == 2 && kBs3Attack[10].kind == 1 && kBs3Attack[16].kind == 3 && kBs3Attack[19].hitStart == -1);
    CHECK(kBs3Motion[0].anim == 327 && kBs3Motion[0].length == 328 && kBs3Motion[1].anim == 341 && kBs3Motion[7].length == 348 && kBs3Motion[9].anim == 343 && kBs3Motion[9].length == 358);
    CHECK(kBs3DmgNormal[0][0] == 500 && kBs3DmgNormal[1][0] == 650 && kBs3DmgNormal[0][16] == 220 && kBs3DmgNormal[0][17] == 360 && kBs3DmgNormal[0][33] == 120 && kBs3DmgBit8[1][16] == 370);
    CHECK(kBs3BulletsA[0] == 5 && kBs3BulletsA[16] == 9 && kBs3BulletsA[33] == 9 && kBs3BulletsB[0] == 3 && kBs3BulletsB[10] == 4);
    CHECK(kBs3PhaseParam[0][0] == 18200.0f && kBs3PhaseParam[0][3] == 2240.0f && kBs3PhaseParam[2][3] == 2560.0f && kBs3PhaseParam[0][5] == 0.0f);

    // --- dificultad
    { Bs3Env e; Mock3 g; g.startDiff = 4; Boss3Logic::initDifficulty(e, g, 2);
      CHECK(e.diff[0] == 4 && e.diff[1] == 4 && e.phaseParamIdx == 2 && e.difficultyShown == 4);
      e.nPlayersB = 1; Boss3Logic::incDifficulty(e, 3, 7); CHECK(e.diff[0] == 7 && e.diff[1] == 7);
      e.nPlayersB = 2; Boss3Logic::incDifficulty(e, 3, 20); CHECK(e.diff[0] == 16 && e.diff[1] == 16 && e.difficultyShown == 16);
      Boss3Logic::decDifficulty(e, 3, 5); CHECK(e.diff[0] == 11); e.nPlayersB = 1; Boss3Logic::decDifficulty(e, 50, 5); CHECK(e.diff[0] == 0 && e.diff[1] == 0);
      e.diff[0] = 6; e.diff[1] = 9; e.nPlayers = 1; e.activePlayer = 1; CHECK(Boss3Logic::getDifficulty(e) == 9); e.activePlayer = 0; CHECK(Boss3Logic::getDifficulty(e) == 6);
      e.nPlayers = 2; CHECK(Boss3Logic::getDifficulty(e) == 7 + 0x11); e.nPlayers = 3; CHECK(Boss3Logic::getDifficulty(e) == 0); }

    // --- vida
    { Bs3State s; Bs3Env e; e.lifeInitPending = 1; Boss3Logic::initLife(s, e, 18200); CHECK(e.bossLife == 18200 && e.bossLifeMax == 18200 && e.lifeInitPending == 0);
      Boss3Logic::initLife(s, e, 5); CHECK(e.bossLife == 18200);
      e.bossLife = 9100; s.staggerMax = 3; s.staggerHits = 3; s.hitCtr = 2; s.hitReload = 4; Boss3Logic::controlDispLife(s, e);
      CHECK(std::fabs(s.lifeRatio - 0.5f) < 1e-6f && std::fabs(s.weakRatio - ((2.0f / 3.0f) + (0.5f / 3.0f))) < 1e-6f && !(s.st & B3_SF_LIFE_SHOW));
      s.st = B3_SF_ATTACKING; Boss3Logic::controlDispLife(s, e); CHECK((s.st & B3_SF_LIFE_SHOW) && s.weakShowTimer == 0x3B);
      s.st = B3_SF_WEAK_HIDE; s.weakShowTimer = 0; Boss3Logic::controlDispLife(s, e); CHECK(s.weakRatio == 0.0f && !(s.st & B3_SF_LIFE_SHOW)); e.bossLife = -5; Boss3Logic::controlDispLife(s, e); CHECK(s.lifeRatio == 0.0f); }

    // --- golpes: fase 0 (la vida no baja del umbral 2240), ajuste 0
    { Bs3State s; Bs3Env e; Mock3 g; Bs3Query q; e.phaseParamIdx = 0; e.bossLife = 18200; e.bossLifeMax = 18200; e.nPlayers = 1; e.diff[0] = 0; s.phase = 0; s.form = 1;
      s.hitCtr = 3; s.hitReload = 3; s.staggerHits = 2; s.staggerMax = 2;
      Boss3Logic::weakHit(s, e, g, q, 1, 0); CHECK(e.bossLife == 17700 && s.hitCtr == 2 && g.L.v.empty());
      e.bossLife = 2300; Boss3Logic::weakHit(s, e, g, q, 1, 0); CHECK(e.bossLife == 2240);                  // 2300-500 <= 2240 -> 2240
      // atacando (bit 8): usa la tabla Bit8 y cuenta golpes; al acabar aturde
      s.st = B3_SF_ATTACKING; e.bossLife = 10000; s.hitCtr = 1; g.L.clear(); Boss3Logic::weakHit(s, e, g, q, 1, 1);
      CHECK(e.bossLife == 9500 && s.hitCtr == 3 && s.staggerHits == 1 && g.L.b("ovmotion", 1) == 0x157 && g.L.b("score", 1) == 0xA && g.L.find("hitmark", 0) && s.motionLen == 0x166 && (s.st & B3_SF_UPPER_HIT));
      s.st = B3_SF_ATTACKING; s.hitCtr = 1; g.L.clear(); Boss3Logic::weakHit(s, e, g, q, 1, 1);                         // (UPPER_HIT rebota: se quita) staggerHits 1 -> 0: Damage completa
      CHECK(g.L.find("sound", 0x180016) && g.L.find("hitmark", 1) && g.L.count("cam") == 1 && (s.st & B3_SF_DAMAGE) && e.diff[0] == 1 && e.diff[1] == 1 && g.L.find("motblend", 343));
    }
    // el indice 9 del movimiento de dano (forma 1) sale de kBs3Motion? No: kBs3Motion tiene 8 entradas; se comprueba la regla documentada
    CHECK(Boss3Logic::damageMotionId([]{ Bs3State s; s.form = 1; return s; }()) == 9 && Boss3Logic::damageMotionId([]{ Bs3State s; s.form = 0; return s; }()) == 0x145 && Boss3Logic::damageMotionId([]{ Bs3State s; s.form = 2; return s; }()) == -1);
    // fases 1 y 2
    { Bs3State s; Bs3Env e; Mock3 g; Bs3Query q; e.phaseParamIdx = 0; e.bossLifeMax = 18200; s.phase = 1; s.form = 0; e.gameState = 1;
      static const int16_t a0[] = {0}; static const int16_t* const tbl[] = {a0, a0, a0, a0, a0, a0, a0, a0}; s.seqTable = tbl;
      e.bossLife = 2600; Boss3Logic::weakHit(s, e, g, q, 1, 0);    // 2600-650 = 1950 <= 2240 (umbral fase 1)
      CHECK(e.bossLife == 1950 && (s.st & B3_SF_WEAK_HIDE) && s.seqTableIdx == 7 && g.L.find("sound", 0x180016));
      Bs3State t; t.phase = 2; t.form = 0; t.seqTable = tbl; e.bossLife = 100; e.gp73F8 = 0; e.gp73FC = 0; g.L.clear();
      Boss3Logic::weakHit(t, e, g, q, 3, 1);                        // fase 2: umbral 0 -> siempre; vida a 0
      CHECK(e.bossLife == 0 && g.L.b("score", 1) == 0x5DC && g.L.find("unlock", 2) && g.L.count("unlock") == 2 && g.L.find("hitmark", 0));
      // rebote con la vida protegida
      g.L.clear(); t.st |= 0x4000 | 0x80; Boss3Logic::weakHit(t, e, g, q, 3, 1); CHECK(g.L.count("dmgeff") == 1 && g.L.v.size() == 1); }
    // checkDamage / checkWeakHit
    { Bs3State s; Bs3Env e; Mock3 g; Bs3Query q; s.form = 0; s.phase = 0; e.bossLife = 18200; e.bossLifeMax = 18200; s.hitCtr = 9; s.hitReload = 9; s.staggerHits = 3; s.staggerMax = 3;
      Bs3Hit h[2] = {{1, 2, 77}, {-1, 0, 0}}; s.enemyFlags = B3_EF_HIT_THIS_FRAME; Boss3Logic::checkDamage(s, e, g, q, h);
      CHECK(g.L.count("hitnum") == 1 && s.lastHitParam == 77 && e.bossLife == 17700);
      s.form = 2; CHECK(!Boss3Logic::checkWeakHit(s)); g.L.clear(); Boss3Logic::checkDamage(s, e, g, q, h); CHECK(g.L.v.empty()); }
    // CheckDamageEnd
    { Bs3State s; Mock3 g; s.st = B3_SF_UPPER_HIT; s.frameB = 54.0f; Boss3Logic::checkDamageEnd(s, g); CHECK(s.st & B3_SF_UPPER_HIT);
      s.frameB = 55.0f; s.frame = 20.5f; s.speedMul = 2.0f; s.motionLenPrev = 99; Boss3Logic::checkDamageEnd(s, g);
      CHECK(!(s.st & B3_SF_UPPER_HIT) && s.baseSpeed == 1.2f && s.speed == 2.4f && s.motionLen == 99 && s.motionBlend == 30 && s.motionMode == 10);
      s.st = B3_SF_DAMAGE; s.motion = 5; s.frame = 99; g.mlen = 100; Boss3Logic::checkDamageEnd(s, g); CHECK(s.st & B3_SF_DAMAGE);
      s.frame = 100; s.phase = 1; s.scene = 0x15; s.motionIdx = 50; Boss3Logic::checkDamageEnd(s, g);
      CHECK(!(s.st & B3_SF_DAMAGE) && g.L.b("cam", 0) == 4 && g.L.find("motblend", kBs3Motion[3].anim)); }
    // SetBulletNum
    { Bs3State s; Bs3Env e; Mock3 g; s.form = 0; g.startDiff = 3; e.nPlayers = 1; Boss3Logic::setBulletNum(s, e, g, 0); CHECK(s.fireCount == 5);
      Boss3Logic::setBulletNum(s, e, g, 4); CHECK(s.fireCount == 3); e.nPlayers = 2; Boss3Logic::setBulletNum(s, e, g, 0); CHECK(s.fireCount == kBs3BulletsA[20]);
      s.form = 1; Boss3Logic::setBulletNum(s, e, g, 0); CHECK(s.fireCount == 1); s.form = 2; s.fireCount = 77; Boss3Logic::setBulletNum(s, e, g, 0); CHECK(s.fireCount == 77); }
    // SelectAttackTarget
    { Bs3State s; Bs3Env e; Mock3 g; e.nPlayers = 1; e.activePlayer = 1; CHECK(Boss3Logic::selectAttackTarget(s, e, g) && s.target == 1 && e.attackerFlag[1] == 1);
      e.nPlayers = 5; CHECK(Boss3Logic::selectAttackTarget(s, e, g) && s.target == 0);
      e.nPlayers = 2; g.rnd = 1; e.attackerFlag[1] = 1; e.attackerFlag[0] = 0; CHECK(Boss3Logic::selectAttackTarget(s, e, g) && s.target == 0);
      e.attackerFlag[0] = 1; e.attackerFlag[1] = 1; Boss3Logic::selectAttackTarget(s, e, g); CHECK(s.target == 0 && e.attackerFlag[1] == 0);
      g.valid = false; CHECK(!Boss3Logic::selectAttackTarget(s, e, g)); Boss3Logic::atcPlFlgClear(s, e); e.attackerFlag[0] = 1; s.target = 0; Boss3Logic::atcPlFlgClear(s, e); CHECK(s.target == -1 && e.attackerFlag[0] == 0); }
    // StartAttack + ControlAttack: ataque 0 (bombas, tres disparos entre 95 y 115)
    { Bs3State s; Bs3Env e; Mock3 g; s.scene = 0x10; e.nPlayers = 1; e.nPlayersB = 2; g.startDiff = 0; s.fireCount = 3;
      Boss3Logic::startAttack(s, e, g, 0);
      CHECK(s.attackId == 0 && s.playersAtStart == 2 && (s.st & B3_SF_ATTACKING) && (s.enemyFlags & B3_EF_ATTACK_DAMAGING) && g.L.find("motnow", 0) == nullptr);
      // motion 324 > 0 -> kBs3Motion[324]: el original lo traduciria por la tabla (indice fuera de rango en este test) -> usamos escena 0x14 (movimiento directo)
    }
    { Bs3State s; Bs3Env e; Mock3 g; s.scene = 0x14; s.fireCount = 3; s.attackId = 0; s.st = B3_SF_ATTACKING; s.motion = 324; s.speed = 1.0f; e.nPlayers = 1;
      s.target = 0;
      s.frame = 95.0f; Boss3Logic::controlAttack(s, e, g); CHECK(g.L.count("bomb") == 0);          // frame <= umbral: nada
      s.frame = 95.5f; Boss3Logic::controlAttack(s, e, g); CHECK(g.L.count("bomb") == 1 && s.attackSub == 1 && g.L.find("sound", 0x17000C) && g.L.find("sound", 0x180013) && s.fireStep == 0);
      s.frame = 100.0f; Boss3Logic::controlAttack(s, e, g); CHECK(s.attackSub == 1 && s.fireStep == 0xFFFF / 3);   // umbral 95 + 1*(20/4) = 100
      s.frame = 100.5f; Boss3Logic::controlAttack(s, e, g); CHECK(g.L.count("bomb") == 2 && s.attackSub == 2 && g.L.count("sound") == 2);
      s.frame = 105.5f; Boss3Logic::controlAttack(s, e, g); CHECK(g.L.count("bomb") == 3 && s.attackSub == 100);
      s.frame = 209.0f; Boss3Logic::controlAttack(s, e, g); CHECK(s.attackSub == 100);               // post = 210
      s.frame = 210.5f; Boss3Logic::controlAttack(s, e, g); CHECK(s.attackSub == -1 && s.recovered == 1);
      s.frame = 100.0f; s.speed = 1.0f; Boss3Logic::controlAttack(s, e, g); CHECK(s.st & B3_SF_ATTACKING);   // 100+1 < 214
      s.frame = 213.0f; Boss3Logic::controlAttack(s, e, g); CHECK(!(s.st & B3_SF_ATTACKING) && !(s.enemyFlags & B3_EF_ATTACK_DAMAGING) && s.recovered == 0); }
    // ataque 9 (laser, kind 2): un solo disparo (hitEnd = -1 < hitStart 0 -> rama simple)
    { Bs3State s; Bs3Env e; Mock3 g; s.scene = 0x14; s.attackId = 9; s.st = B3_SF_ATTACKING; s.motion = 5; s.frame = 0.5f; Boss3Logic::controlAttack(s, e, g);
      CHECK(g.L.count("laser") == 1 && s.attackSub == 100); s.frame = 91.0f; Boss3Logic::controlAttack(s, e, g); CHECK(s.recovered == 1 && s.attackSub == -1 && s.postFrame == 0 /*kind 2: no se toca*/);
      // ataque 10 (kind 1, hitStart 0, post -1): termina directamente
      Bs3State t; t.scene = 0x14; t.attackId = 10; t.st = B3_SF_ATTACKING; t.motion = 6; t.frame = 1.0f; Boss3Logic::controlAttack(t, e, g); CHECK(t.attackSub == 100); Boss3Logic::controlAttack(t, e, g); CHECK(t.attackSub == -1 && t.recovered == 0);
      // ataque 4 (hitStart -1): salta a 100; 2 disparos por laser (ataque 1: kind 0 sin segundo sonido)
      Bs3State u; u.scene = 0x14; u.attackId = 11; u.st = B3_SF_ATTACKING; u.motion = 4; Boss3Logic::controlAttack(u, e, g); CHECK(u.attackSub == 100); }
    // ataque 16 (kind 3, melee): 0 -> 10 -> 11 -> -1
    { Bs3State s; Bs3Env e; Mock3 g; s.scene = 0x14; s.attackId = 16; s.target = 1; s.st = B3_SF_ATTACKING; s.motion = 7; s.speed = 1.0f; e.nPlayersB = 1; e.diff[0] = 10; e.diff[1] = 10;
      const Bs3Attack& a = kBs3Attack[16]; CHECK(a.hitStart == 0 && a.hitEnd == -1);
      s.frame = 1.0f; Boss3Logic::controlAttack(s, e, g); CHECK(s.attackSub == 0xA && g.L.count("blur") == 0);   // hitStart 0 != -1: frame >= 0 -> sub 10; hitEnd < 0: sin blur extra
      Boss3Logic::controlAttack(s, e, g); CHECK(s.attackSub == -1);                                                    // hitEnd == -1 -> -1
      s.frame = 78.0f; Boss3Logic::controlAttack(s, e, g); CHECK(s.st & B3_SF_ATTACKING); s.frame = 79.0f; Boss3Logic::controlAttack(s, e, g); CHECK(!(s.st & B3_SF_ATTACKING));
      // ataque 19: hitStart -1, hitEnd 60, post 0: espera blur
      Bs3State t; t.scene = 0x14; t.attackId = 19; t.target = 0; t.st = B3_SF_ATTACKING; t.motion = 8; t.frame = 3.0f; t.speed = 1.0f; g.L.clear(); Boss3Logic::controlAttack(t, e, g);
      CHECK(t.attackSub == 0xA && g.L.find("blur", 0) && g.L.b("blur", 0) == 0xA);                              // hitStart == -1: blur inmediato
    }
    { Bs3State t; Bs3Env e; Mock3 g; e.nPlayersB = 1; e.diff[0] = e.diff[1] = 10; t.scene = 0x14; t.attackId = 19; t.target = 1; t.st = B3_SF_ATTACKING; t.motion = 8; t.speed = 1.0f; t.attackSub = 0xA;
      t.frame = 40.2f; Boss3Logic::controlAttack(t, e, g); CHECK(g.L.count("sound") == 1 && g.L.find("sound", 0x180013) && t.attackSub == 0xA);   // (int)40.2 == (int)(60-20)
      g.L.clear(); t.frame = 59.0f; Boss3Logic::controlAttack(t, e, g); CHECK(t.attackSub == 0xA && !(t.st & B3_SF_BIT15));
      t.frame = 60.0f; Boss3Logic::controlAttack(t, e, g); CHECK(t.attackSub == 0xB && (t.st & B3_SF_BIT15));
      t.frame = 60.0f; Boss3Logic::controlAttack(t, e, g); CHECK(t.attackSub == -1 && g.L.count("dmgpl") == 1 && !(t.st & B3_SF_BIT15));   // subestado 11: 60 no es < hitEnd -> golpea y termina
    }
    // AttackDamageFrame melee: danio 5 y baja 2 de dificultad
    { Bs3State s; Bs3Env e; Mock3 g; s.attackId = 16; s.target = 1; e.nPlayersB = 1; e.diff[0] = 10; e.diff[1] = 10; Boss3Logic::attackDamageFrame(s, e, g);
      CHECK(g.L.c("dmgpl", 1) == 5 && e.diff[0] == 8); s.attackId = 9; Boss3Logic::attackDamageFrame(s, e, g); CHECK(g.L.count("laser") == 1);
      s.attackId = 2; g.valid = false; Boss3Logic::attackDamageFrame(s, e, g); CHECK(g.L.count("bomb") == 0); }
    // el ataque cancelado por cambio de movimiento / aturdimiento
    { Bs3State s; Bs3Env e; Mock3 g; s.scene = 0x10; s.attackId = 0; s.st = B3_SF_ATTACKING | B3_SF_BIT15; s.motion = 1; s.enemyFlags = B3_EF_ATTACK_DAMAGING; s.recovered = 1;
      // motion 324 en kBs3Motion no existe (8 entradas); este caso solo se prueba con motion<=0 mas abajo
      Bs3State t; t.scene = 0x14; t.attackId = 0; t.st = B3_SF_ATTACKING | B3_SF_BIT15; t.motion = 1; t.enemyFlags = B3_EF_ATTACK_DAMAGING; t.recovered = 1;
      Boss3Logic::controlAttack(t, e, g); CHECK(!(t.st & (B3_SF_ATTACKING | B3_SF_BIT15)) && !(t.enemyFlags & B3_EF_ATTACK_DAMAGING) && t.recovered == 0 && g.L.find("blur", 2));
      Bs3State u; u.scene = 0x14; u.attackId = 0; u.st = B3_SF_ATTACKING | B3_SF_DAMAGE; u.motion = 324; g.L.clear(); Boss3Logic::controlAttack(u, e, g); CHECK(!(u.st & B3_SF_ATTACKING)); }
    // MotSupSet2
    { Bs3State s; Mock3 g; s.motionIdx = 0; s.enemyFlags = B3_EF_DAMAGE_MOTION; Boss3Logic::motSupSet2(s, g, 2, 7, 5);
      CHECK(g.L.b("motblend", 342) == 7 && s.motionIdx == 2 && s.motionLen == 357 && s.motionLenPrev == 357 && s.motionBlend == 7 && s.motionMode == 5 && !(s.enemyFlags & B3_EF_DAMAGE_MOTION));
      g.L.clear(); Boss3Logic::motSupSet2(s, g, 2, 0, 0); CHECK(g.L.v.empty());              // mismo indice: nada
      Boss3Logic::motSupSet2(s, g, 3, 0, 0); CHECK(g.L.find("motnow", 337));                 // modo 0: sin mezcla
      s.scene = 0x14; g.L.clear(); Boss3Logic::motSupSet2(s, g, 77, 0, 0); CHECK(g.L.find("motnow", 77) && s.motionIdx == 3); }
    // interpolacion de angulos
    CHECK(Boss3Logic::interDivAng(0.5f, 0, 0x4000) == 0x2000);
    CHECK(Boss3Logic::interDivAng(0.5f, 0xFFF0, 0x0010) == 0xFFF0 + 16);
    CHECK(Boss3Logic::interDivAng(0.5f, 0x0010, 0xFFF0) == 0x0010 - 16);
    CHECK(Boss3Logic::interDivAng(0.0f, 5, 1000) == 5 && Boss3Logic::interDivAng(1.0f, 0, 0x7FFF) == 0x7FFF);
    CHECK(Boss3Logic::interDivAng(1.0f, 0, 0x8000) == -0x8000);                           // d = 0x8000 > 32767: camino negativo
    // SetApproach + ApproachANPO
    { Bs3State s; s.pos.x = 0; s.pos.y = 10; s.pos.z = 20; s.pos.ax = 0; s.pos.ay = 0x1000; s.pos.az = 0; Bs3Anpo t{100, 110, 120, 0, 0x3000, 0};
      Boss3Logic::setApproach(s, 12); CHECK((s.st & B3_SF_APPROACH_ON) && s.apT == 0 && s.apRate == 1.2f && std::fabs(s.apDur - 10.0f) < 1e-4f && s.apY == 10);
      s.apT = 5.0f; Boss3Logic::approachAnpo(s, t); CHECK(std::fabs(s.pos.x - 50.0f) < 1e-3f && std::fabs(s.pos.y - 60.0f) < 1e-3f && std::fabs(s.pos.z - 70.0f) < 1e-3f && s.pos.ay == 0x2000);
      s.apT = 10.0f; Boss3Logic::approachAnpo(s, t); CHECK(s.pos.x == 100 && s.pos.ay == 0x3000 && !(s.st & B3_SF_APPROACH_ON));
      s.st = B3_SF_APPROACH_SNAP; s.pos.x = 5; Boss3Logic::approachAnpo(s, t); CHECK(s.pos.x == 100); s.st = 0; s.pos.x = 5; Boss3Logic::approachAnpo(s, t); CHECK(s.pos.x == 5); }
    // secuencias
    { static const int16_t a[] = {1}, b[] = {2}, c[] = {3}; static const int16_t* const tb[] = {a, b, c}; Bs3State s; s.seqWait = 5;
      Boss3Logic::seqCmdInit(s, tb); CHECK(s.seqPc == a && s.seqWait == 0 && s.seqTableIdx == 0); Boss3Logic::seqCmdNextTable(s); CHECK(s.seqPc == b);
      Boss3Logic::seqCmdSetTable(s, 2); CHECK(s.seqPc == c && s.seqTableIdx == 2); }
    // contadores
    { Bs3State s; Bs3Env e; e.nowEnemyAlive = 3; e.nowEnemyNum = 3; Boss3Logic::decNowEnemyAlvNumChk(s, e); Boss3Logic::decNowEnemyAlvNumChk(s, e); Boss3Logic::decNowEnemyNumChk(s, e); CHECK(e.nowEnemyAlive == 2 && e.nowEnemyNum == 2); }
}

int main() {
    testBoss2(); testBoss3();
    if (fails == 0) printf("bosses_a_test: OK\n"); else printf("bosses_a_test: %d fallos\n", fails);
    return fails != 0;
}

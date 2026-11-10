// Tests de game/bosses_b (jefes de bs4mod, bs5mod, bs6mod). Valores sacados de asm/ELF/BIN, no de la salida del codigo.
// Compilar (desde la raiz del repo):  c++ -std=c++17 -Wall game/test/bosses_b_test.cpp game/bosses_b/boss4.cpp game/bosses_b/boss4_data.cpp [...]
#include "../bosses_b/boss4.h"
#include <cstdio>
#include <vector>
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

// ------------------------------------------------------------------ Boss4
namespace b4 {
using namespace vn::boss4;
struct Mock : Boss4Env {
    Boss4State* st = nullptr;
    float cam = 0; int trackId = 0; int rnd = 0; int dmgResult = 0; float scale = 1.0f; bool alive = true;
    int lastMotion = -1, lastStart = -1; std::vector<int> sounds; std::vector<int> hooks; int playerDamaged = -1, dmgGiven = -1;
    int motLen = 100;
    float camSpeed = 0;
    std::vector<std::pair<int, int>> registered;
    float camFrame() override { return cam; }
    int camTrackId() override { return trackId; }
    void setCamSpeed(float v) override { camSpeed = v; }
    int motionLen(int) override { return motLen; }
    int rand() override { return rnd; }
    int damagePlayer(int p, int, int d) override { playerDamaged = p; dmgGiven = d; return dmgResult; }
    void playMotion(int m, int s) override { lastMotion = m; lastStart = s; if (st) { st->motion = m; st->motionFrame = (float)s; } }
    void sound(int id, int) override { sounds.push_back(id); }
    void projectToScreen(const float*, float& x, float& y) override { x = 100; y = 100; }
    int statFunc(int, int) override { return 0; }
    float damageScale(int) override { return scale; }
    void registerHit(int part, int pts) override { registered.push_back({part, pts}); }
    bool playerAlive(int) override { return alive; }
    void hook(int what, int, int, int) override { hooks.push_back(what); }
};
static bool hooked(const Mock& m, int h) { for (int x : m.hooks) if (x == h) return true; return false; }

void run() {
    // --- tablas del BIN (BS4MOD.BIN)
    CHECK(kCmdLen[0] == 2 && kCmdLen[11] == 4 && kCmdLen[17] == 0 && kCmdLen[54] == 4 && kCmdLen[66] == 2);   // D_009D4250
    CHECK(kAttackTable[0].motion == 396 && kAttackTable[0].dmg == 5 && kAttackTable[0].f[1] == 90 && kAttackTable[0].endFrame == 140);  // D_009DA500 fila 0
    CHECK(kAttackTable[2].motion == 398 && kAttackTable[2].dmg == 4 && kAttackTable[2].f[1] == 81);
    CHECK(kAttackTable[6].motion == 364 && kAttackTable[6].endFrame == 64);
    CHECK(kAttackTable[0].range == 75.0f);
    CHECK(kRunRowA[0].mot[0] == 405 && kRunRowA[1].mot[0] == 406 && kRunRowA[0].atk[2] == 2 && kRunRowA[1].atk[0] == 3);  // D_009D2BB0 / D_009D2BBA
    CHECK(kRunRowB[0].mot[0] == 373 && kRunRowB[2].mot[1] == 379 && kRunRowB[0].atk[0] == 6 && kRunRowB[2].atk[2] == 14);  // D_009D2BD0 / D_009D2BD4
    CHECK(kPose[1][0] == 908.0f && kPose[1][1] == 54.0f && kPose[1][2] == 1820.0f);                                      // D_009D2B60
    CHECK(kReappearPos[0][2] == 700.0f && kReappearPos[2][2] == 1230.0f);                                                // D_009D2BF0
    CHECK(kSchedFrame[0] == 496 && kSchedFrame[1] == 992 && kSchedFrame[2] == 1456 && kSchedFrame[3] == 1958);          // ELF 0x2D86F8
    CHECK(kKindConfig[0].orC == 0x80008600u && kKindConfig[0].or1E14 == 0x2802u && kKindConfig[2].and1E14 == 0xFFFFFFFEu && kKindConfig[4].seqTab == nullptr && kKindConfig[3].initState == 2);
    CHECK(kDiffRows[0].hp[0] == 3 && kDiffRows[0].hp[16] == 6 && kDiffRows[0].dmgMul[5] == 50 && kDiffRows[0].speed16[0] == 19.2f);
    CHECK(kDiffRows[0].stepUp == 0 && kDiffRows[0].stepDown == 3 && kDiffRows[4].stepUp == 1 && kDiffRows[4].stepDown == 2);   // ELF 0x2D8720/0x2D8730

    // --- utilidades puras
    CHECK(Boss4Logic::eTurn(0, 0x4000, 0x100) == 0x100);
    CHECK(Boss4Logic::eTurn(0x4000, 0, 0x100) == 0x3F00);
    CHECK(Boss4Logic::eTurn(0, 0xC000, 0x100) == -0x100);      // camino corto por detras
    CHECK(Boss4Logic::eTurn(0x1000, 0x1050, 0x100) == 0x1050); // llega
    CHECK(Boss4Logic::angChk(0x1000, 0x1010, 0x20) && !Boss4Logic::angChk(0x1000, 0x2000, 0x20) && Boss4Logic::angChk(0xFFF0, 0x0010, 0x40));

    // --- VM: primer script del tipo 0 = {15, 28, 29, 22, 11,404,0,0, 3, ...}
    Mock m; Boss4State b; Boss4Globals g; m.st = &b;
    b.kind = K_BATTLE1; g.gameLevel = 0; b.difficulty = 0; b.hitsPerCount = 1;
    Boss4Logic::seqInit(b, kKindConfig[0].seqTab);
    Boss4Logic::execSeqCmd(b, g, m);
    CHECK(b.diffArmed == 1);                       // op 15
    CHECK(b.runC == 0 && b.runA == 0);             // op 28 (rand = 0), op 29
    CHECK(b.f0C & F0C_COLLISION_OFF);              // op 22
    CHECK(m.lastMotion == 404 && m.lastStart == 0);   // op 11 (motion, 0, 0)
    CHECK(b.pc && b.pc[0] == 3);                   // espera fin de movimiento
    b.motionFrame = 100.0f; Boss4Logic::execSeqCmd(b, g, m); CHECK(b.pc[0] == 3);   // frame <= len: sigue esperando
    b.motionFrame = 101.0f; Boss4Logic::execSeqCmd(b, g, m);
    CHECK(!(b.f0C & F0C_COLLISION_OFF));           // op 21 (enable)
    CHECK(m.lastMotion == 405);                    // op 30: kRunRowA[0].mot[0]
    // temporizador NTSC->PAL: P=60 -> 50 fotogramas
    {
        Boss4State t; t.kind = K_BATTLE1; static const int16_t scr[] = {7, 60, 18};
        t.pc = scr; t.timer[0] = -1; int n = 0; while (true) { Boss4Logic::execSeqCmd(t, g, m); n++; if (t.pc == scr + 2) break; if (n > 100) break; }
        CHECK(n == 51);                            // 50 esperas + 1 ejecucion que avanza
        static const int16_t one[] = {7, 1, 18}; Boss4State u; u.pc = one; u.timer[0] = -1; n = 0;
        while (true) { Boss4Logic::execSeqCmd(u, g, m); n++; if (u.pc == one + 2 || n > 10) break; }
        CHECK(n == 2);                             // P == 1 -> 1 fotograma de espera
    }
    // op 9 / 37: bit de +0x0C
    {
        Boss4State t; static const int16_t s1[] = {37, 3, 1, 9, 3, 1, 18}; t.pc = s1;
        Boss4Logic::execSeqCmd(t, g, m); CHECK((t.f0C & 8) && t.pc == s1 + 6);
        static const int16_t s2[] = {9, 5, 1, 18}; Boss4State u; u.pc = s2; Boss4Logic::execSeqCmd(u, g, m); CHECK(u.pc == s2);   // bit 5 no puesto: espera
        u.f0C = 1u << 5; Boss4Logic::execSeqCmd(u, g, m); CHECK(u.pc == s2 + 3);
    }
    // op 50: puerta de golpes del tipo 1 (>= 6 -> script 4 de la tabla = centinela del ELF {16, 18}); otros tipos se saltan a si mismos
    {
        Boss4State t; t.kind = K_BATTLE2; Boss4Logic::seqInit(t, kKindConfig[1].seqTab);
        static const int16_t s[] = {50, 18}; t.pc = s; t.phaseCount = 5; Boss4Logic::execSeqCmd(t, g, m); CHECK(t.pc == s + 1 && t.tabIdx == 0);
        t.pc = s; t.phaseCount = 6; Boss4Logic::execSeqCmd(t, g, m); CHECK(t.tabIdx == 4 && t.state == TS_FINISHED && t.pc == kScript + kSeqTab1[4] + 1);
        Boss4State u; u.kind = K_BATTLE1; u.pc = s; u.phaseCount = 99; Boss4Logic::execSeqCmd(u, g, m); CHECK(u.pc == s + 1);
    }
    // pila: Call / Pop
    {
        Boss4State t; t.kind = K_BATTLE1; Boss4Logic::seqInit(t, kKindConfig[0].seqTab);
        const int16_t* p0 = t.pc;
        Boss4Logic::seqCall(t, 0); CHECK(t.sp == 1 && t.stack[0] == p0 && t.pc == kScript + kCallScript[0] && t.timer[1] == -1);
        Boss4Logic::seqPop(t); CHECK(t.sp == 0 && t.pc == p0);
        Boss4Logic::seqNextTable(t); CHECK(t.tabIdx == 1 && t.pc == kScript + kKindConfig[0].seqTab[1]);
        Boss4Logic::seqSetTable(t, 2); CHECK(t.tabIdx == 2 && t.pc == kScript + kKindConfig[0].seqTab[2]);
        static const int16_t w[] = {4, 3, 25}; t.pc = w; Boss4Logic::seqCancelWait(t); CHECK(t.pc == w + 2);   // op 4 (len 2) salta
        static const int16_t w2[] = {3, 25}; t.pc = w2; Boss4Logic::seqCancelWait(t); CHECK(t.pc == w2 + 1);
        static const int16_t w3[] = {9, 1, 1}; t.pc = w3; Boss4Logic::seqCancelWait(t); CHECK(t.pc == w3);    // otro op: no toca
    }

    // --- dificultad (SetDiffParam / Up / Down)
    {
        Boss4State t; Boss4Globals gg; t.kind = K_BATTLE1; gg.gameLevel = 0; t.difficulty = 16;
        Boss4Logic::setDiffParam(t, gg);
        CHECK(t.partLife == 6 && t.partLifeMax == 6 && t.dmgMul == 50 && t.speedBase == 36.0f / 16.0f && t.diffDown == 3 && t.diffUp == 0 && gg.diffMirror == 16);
        t.difficulty = 2; Boss4Logic::downDifficulty(t); CHECK(t.difficulty == 0 && t.diffArmed == 0);
        t.diffUp = 5; t.difficulty = 14; Boss4Logic::upDifficulty(t); CHECK(t.difficulty == 16);
        Boss4State k; k.kind = K_BATTLE2; k.difficulty = 0; gg.gameLevel = 1; Boss4Logic::setDiffParam(k, gg);
        CHECK(k.loopCount == kLoopKind1[1][0] && k.dmgMul == kDiffRows[3].dmgMul[0]);
        Boss4State s; s.kind = K_BATTLE3; s.subPhase = 2; s.difficulty = 3; gg.gameLevel = 0; Boss4Logic::setDiffParam(s, gg);
        CHECK(s.loopCount == kLoopKind2Sub2[0][3] && s.partLife == kHpKind2[2][0][3]);
        s.subPhase = 1; s.loopCount = 77; Boss4Logic::setDiffParam(s, gg); CHECK(s.loopCount == 77);   // subfase 1 no toca loopCount
    }
    // --- vida
    {
        Boss4State t; Boss4Globals gg; Boss4Logic::initLife(t, gg, 1000); CHECK(gg.bossLife == 1000 && gg.bossLifeMax == 1000 && t.lifeRatio == 1.0f);
        Boss4Logic::initLife(t, gg, -1); CHECK(gg.bossLife == 1000);                        // valor no valido: conserva
        gg.bossLife = 0; gg.bossLifeMax = 0; Boss4Logic::initLife(t, gg, -1); CHECK(gg.bossLife == 400 && gg.bossLifeMax == 400);
        gg.bossLife = 100; gg.bossLifeMax = 400; t.partLife = 1; t.partLifeMax = 4; Boss4Logic::controlDispLife(t, gg); CHECK(t.lifeRatio == 0.25f && t.hitRatio == 0.25f);
    }
    // --- WeakHit: descuenta vida, y a la ultima de la fase llama Damage; kind 2 con vida <= 0 -> Dead
    {
        Mock mm; Boss4State t; Boss4Globals gg; mm.st = &t; t.kind = K_BATTLE1; gg.gameLevel = 0; t.difficulty = 0; t.hitsPerCount = 1; t.hitCounter = 1;
        Boss4Logic::seqInit(t, kKindConfig[0].seqTab);
        Boss4Logic::setDiffParam(t, gg); gg.bossLife = 400; gg.bossLifeMax = 400;
        Boss4Logic::weakHit(t, gg, mm, 5, 3);
        CHECK(gg.bossLife == 350);                  // dmgMul 50 * escala 1.0
        CHECK(t.partLife == 2 && (t.f1E14 & FB_HIT_STUN) && t.stunTimer == 25);    // golpe flojo: aturdimiento
        CHECK(mm.registered.size() == 1 && mm.registered[0].second == 10 && mm.sounds.size() == 1 && mm.sounds[0] == 0x11000C);
        t.f1E14 &= ~(uint32_t)FB_HIT_STUN; t.hitCounter = 1;
        Boss4Logic::weakHit(t, gg, mm, 5, 3); CHECK(t.partLife == 1);
        t.f1E14 &= ~(uint32_t)FB_HIT_STUN; t.hitCounter = 1; int ph = t.phaseCount;
        Boss4Logic::weakHit(t, gg, mm, 5, 3);        // partLife -> 0: Damage()
        CHECK(t.phaseCount == ph + 1 && (t.f0C & F0C_IN_DAMAGE) && t.rate == 1.2f && t.rateMul == 1.0f);
        CHECK(t.sp == 1 && t.pc == kScript + kCallScript[0]);       // reaccion tipo 0 = script de llamada 0
        CHECK(mm.sounds.back() == 0x11000D && (t.f1E14 & FB_ALPHA_RUN) && t.alphaStep < 0);
        CHECK(mm.lastMotion == 0x197);               // SetDamageMotion tipo 0
        CHECK(hooked(mm, H_HIT_MARK2) && hooked(mm, H_HIT_MARK1) && hooked(mm, H_SPAWN_EFFECT));
        // golpe ignorado con la bandera 0x100 puesta
        t.f1E14 |= FB_HIT_STUN; int bl = gg.bossLife; t.hitCounter = 1; Boss4Logic::weakHit(t, gg, mm, 5, 3); CHECK(gg.bossLife == bl);
        t.f1E14 &= ~(uint32_t)FB_HIT_STUN; t.f0C = F0C_COLLISION_OFF; bl = gg.bossLife; Boss4Logic::weakHit(t, gg, mm, 5, 3); CHECK(gg.bossLife == bl);   // 0x8000 en la mascara 0x44008100
    }
    {   // tipo 2: la vida llega a 0 -> Dead; tipos != 2 nunca bajan de 1
        Mock mm; Boss4State t; Boss4Globals gg; mm.st = &t; t.kind = K_BATTLE3; t.hitCounter = 1; t.hitsPerCount = 1; t.dmgMul = 500; t.partLife = 5;
        gg.bossLife = 300; gg.bossLifeMax = 400; gg.gameState = 1;
        Boss4Logic::weakHit(t, gg, mm, 5, 0);
        CHECK(gg.bossLife == 0 && t.state == TS_DEAD && t.b1E4C == 5 && (t.f0C & F0C_DEAD) && mm.lastMotion == 0x1AA);
        CHECK(mm.registered.size() == 2 && mm.registered[1].second == 0x5DC);
        Boss4State u; u.kind = K_BATTLE1; u.hitCounter = 1; u.hitsPerCount = 1; u.dmgMul = 500; u.partLife = 5; gg.bossLife = 300; mm.st = &u;
        Boss4Logic::weakHit(u, gg, mm, 5, 0); CHECK(gg.bossLife == 1);
    }
    // --- seleccion de objetivo y dano al jugador
    {
        Mock mm; Boss4State t; Boss4Globals gg; gg.gameLevel = 0; gg.lastHitTarget = 1;
        CHECK(Boss4Logic::selectAttackTarget(t, gg, mm) && t.target == 1 && gg.targetBusy[1] == 1);
        mm.alive = false; CHECK(!Boss4Logic::selectAttackTarget(t, gg, mm) && t.target == -1);
        mm.alive = true; gg.gameLevel = 1; gg.targetBusy[0] = 0; gg.targetBusy[1] = 0; mm.rnd = 0;
        CHECK(Boss4Logic::selectAttackTarget(t, gg, mm) && t.target == 0 && gg.targetBusy[0] == 1);
        mm.rnd = 0; CHECK(Boss4Logic::selectAttackTarget(t, gg, mm) && t.target == 1);   // el 0 esta ocupado -> el 1
        CHECK(Boss4Logic::selectAttackTarget(t, gg, mm) && gg.targetBusy[0] == 0 && gg.targetBusy[1] == 1 && t.target == 1);   // ambos ocupados: se vacian y se marca solo el elegido
    }
    {   // DamageToPlayer: dano de la tabla, baja dificultad si acierta
        Mock mm; Boss4State t; Boss4Globals gg; t.target = 1; t.attack = 2; t.difficulty = 10; t.diffDown = 3; mm.dmgResult = 1;
        Boss4Logic::damageToPlayer(t, gg, mm); CHECK(mm.playerDamaged == 1 && mm.dmgGiven == 4 && t.difficulty == 7);
        mm.dmgResult = 0; t.diffArmed = 1; Boss4Logic::damageToPlayer(t, gg, mm); CHECK(t.diffArmed == 0 && t.difficulty == 7);
    }
    // --- StartAttack / ControlAttack (ataque 0 = movimiento 396, dano en el fotograma 90, termina en 140)
    {
        Mock mm; Boss4State t; Boss4Globals gg; mm.st = &t; t.kind = K_BATTLE2; t.target = 0; mm.dmgResult = 1; gg.numPlayersB = 2; t.difficulty = 4; t.diffDown = 1;
        Boss4Logic::startAttack(t, gg, mm, 0);
        CHECK(t.attack == 0 && mm.lastMotion == 396 && (t.f0C & F0C_ATTACKING) && (t.f1E14 & FB_ATTACK) && t.b1F26 == 2 && t.atkStep == 0);
        t.motionFrame = 50; Boss4Logic::controlAttack(t, gg, mm); CHECK(t.atkStep == 1 && mm.playerDamaged == -1);   // f[0] = -1 (se salta); espera f[1] = 90
        t.motionFrame = 90; Boss4Logic::controlAttack(t, gg, mm);
        CHECK(mm.playerDamaged == 0 && mm.dmgGiven == 5 && t.atkStep == 6 && !(t.f0C & F0C_ATTACKING) && (t.f1E14 & FB_ATTACK));   // dano; pasos 2..5 con umbral -1; espera el fin
        CHECK(mm.sounds.size() >= 2 && mm.sounds[mm.sounds.size() - 2] == 0x100010 && mm.sounds.back() == 0x8000B2);
        t.motionFrame = 138; Boss4Logic::controlAttack(t, gg, mm); CHECK(t.f1E14 & FB_ATTACK);        // 138 + rate(1.0) < 140
        t.motionFrame = 139; Boss4Logic::controlAttack(t, gg, mm); CHECK(!(t.f1E14 & FB_ATTACK));
        t.f1E14 |= FB_ATTACK; t.motion = 7; Boss4Logic::controlAttack(t, gg, mm); CHECK(!(t.f1E14 & FB_ATTACK));   // movimiento distinto: cancela
    }
    {   // fallCheck: gravedad -0.0392 (0xBD20902E) hasta tocar suelo
        Boss4State t; t.f10 = 0x40; t.pos[1] = 10.0f;
        Boss4Logic::fallCheck(t, 0.0f); CHECK(t.fallStep == 1 && t.velY == t.accelY && t.pos[1] < 10.0f);
        for (int i = 0; i < 1000 && (t.f10 & 0x40); i++) Boss4Logic::fallCheck(t, 0.0f);
        CHECK(!(t.f10 & 0x40) && t.pos[1] == 0.0f && t.velY == 0 && t.fallStep == 0);
    }
}
}  // namespace b4

int main() {
    b4::run();
    if (fails == 0) printf("bosses_b_test: OK\n");
    return fails ? 1 : 0;
}

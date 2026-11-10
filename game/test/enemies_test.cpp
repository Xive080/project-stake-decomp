// Tests de game/enemies. Valores de tablas sacados de GAMEMOD.BIN / ORGMOD.BIN / ELF (ver MANIFEST.md).
// Compilar: c++ -std=c++17 -Wall game/test/enemies_test.cpp game/enemies/enemy_common.cpp game/enemies/vampire.cpp game/enemies/vampire_tables.cpp game/enemies/people.cpp -o build/enemies_test
#include "../enemies/vampire.h"
#include "../enemies/people.h"
#include <cstdio>
#include <cmath>
using namespace vn::en;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static int g_rand = 0;
static int fakeRand() { return g_rand; }
static double g_r01 = 0.0;
static double fakeRand01() { return g_r01; }
static int count(const EventList& ev, EvType t) { int n = 0; for (auto& e : ev) if (e.type == t) n++; return n; }
static const Event* find(const EventList& ev, EvType t, int nth = 0) { for (auto& e : ev) if (e.type == t && nth-- == 0) return &e; return nullptr; }

static EnemyEnv baseEnv() {
    EnemyEnv e; e.rand = fakeRand; e.rand01 = fakeRand01;
    e.pl[0].mode = 5; e.pl[0].life = 5; e.pl[0].maxLife = 5; e.pl[1].mode = 5; e.pl[1].life = 3; e.pl[1].maxLife = 5;
    return e;
}

int main() {
    // --- tablas ---
    CHECK(kVampDamage[0][1][0] == 210 && kVampDamage[0][2][0] == 260);       // tipo 0 (0x9AE770): partes 1 y 2
    CHECK(kVampDamage[1][1][0] == 140 && kVampDamage[1][1][1] == 240);       // tipo 1: dos niveles
    CHECK(kVampDamage[3][1][2] == 70 && kVampDamage[5][11][0] == 240);
    CHECK(kVampLifeByType[0] == 200 && kVampLifeByType[10] == 280 && kVampLifeByType[17] == 160);
    CHECK(kVampLifeByLevel[0] == -100 && kVampLifeByLevel[16] == 60);
    CHECK(kVampLifeByDiff1P[0] == -80 && kVampLifeByDiff2P[16] == 140);
    CHECK(rules::kWeaponDamage[7] == 2.0f && rules::kWeaponDamage[14] == 3.0f && rules::kWeaponDamage[11] == 0.4f);
    CHECK(kPeopleBaseDamage[1][1] == 90 && kPeopleDamageMot[2][0] == 132 && kPeoplePartClass[12] == 6);

    // --- vida inicial (charInit) ---
    {
        EnemyEnv e = baseEnv(); e.levelA = 8; e.levelB = 4; e.solo = 0;
        VampireState v; v.type = 0; VampireLogic::initLife(v, e);
        CHECK(v.life == 200 + 0 + (-40)); CHECK(v.lifeMax == v.life && v.lifePrev == v.life);
        e.solo = 2; VampireLogic::initLife(v, e); CHECK(v.life == 200 + 0 + 20);
        v.type = 8; VampireLogic::initLife(v, e); CHECK(v.reachLimit == -20.0f);
        e.gameState = 0xA; e.gameKind = 1; VampireLogic::initLife(v, e); CHECK(v.life == 90 + 20);   // escala de vida siempre 1.0
    }
    // --- dano y arma ---
    {
        EnemyEnv e = baseEnv(); e.lastHitPlayer = 0;
        VampireState v; v.life = 500;
        VampireLogic::decLife(v, e, 100); CHECK(v.life == 400 && v.lifePrev == 500);
        e.gameState = 0xA; e.gameKind = 1; e.pl[0].weapon = 7;
        VampireLogic::decLife(v, e, 100); CHECK(v.life == 200);              // x2
        e.pl[0].weapon = 8; e.pl[0].oneHit = 0; VampireLogic::decLife(v, e, 101); CHECK(v.life == 200 - 50);   // 0.5 trunc(50.5)=50
        e.pl[0].oneHit = 1; VampireLogic::decLife(v, e, 1); CHECK(v.life == 0);     // entrenamiento: mata
        CHECK(rules::scaledDamage(e, 10, 0) == 20);   // arma 8 con oneHit=1: se queda con el valor de tabla (2.0)
    }
    // --- checkDamage: golpe sin matar, y muerte ---
    {
        EnemyEnv e = baseEnv(); VampireState v; v.type = 1; v.life = 300; v.flags = VF_HITCHECK;
        v.parts[1].hitLevel = 0; v.parts[1].flag154 = 0x100;
        HitInput h; h.shooter[0] = 0; h.partOfPlayer[0] = 1; h.weaponClass[0] = 0;
        EventList ev; VampireLogic::checkDamage(v, e, h, ev);
        CHECK(v.life == 300 - 140); CHECK(!(v.flags & VF_DEAD)); CHECK(v.stateFlags & VS_HIT);
        CHECK(count(ev, EV_SCORE) == 1 && find(ev, EV_SCORE)->b == 10); CHECK(count(ev, EV_HITNUM) == 1);
        CHECK(find(ev, EV_HITMARK)->c == 0x3E8); CHECK(count(ev, EV_DAMAGE_MOT) == 1);
        CHECK(!(v.parts[1].flag154 & 0x100));
        bool snd2002C = false, snd180010 = false; for (auto& x : ev) if (x.type == EV_SOUND) { if (x.a == 0x2002C) snd2002C = true; if (x.a == 0x180010) snd180010 = true; }
        CHECK(snd2002C && snd180010);
        // segundo golpe con nivel 1 (240) mata (life 160 - 240 <= 0)
        v.parts[1].hitLevel = 1; EventList ev2; VampireLogic::checkDamage(v, e, h, ev2);
        CHECK(v.flags & VF_DEAD); CHECK((v.stateFlags & 3) == 3);
        CHECK(find(ev2, EV_SCORE)->b == 100 && find(ev2, EV_SCORE, 1)->b == 10);
        CHECK(find(ev2, EV_HITMARK)->c == 0x3E9); CHECK(count(ev2, EV_PARTS_CHAIN) == 1);
        bool sndDead = false; for (auto& x : ev2) if (x.type == EV_SOUND && x.a == 0x180011) sndDead = true; CHECK(sndDead);
        // muerto: no vuelve a recibir dano ni contar
        EventList ev3; int lifeBefore = v.life; VampireLogic::checkDamage(v, e, h, ev3); CHECK(v.life == lifeBefore && count(ev3, EV_HITNUM) == 0);
        // invulnerable: no baja vida pero cuenta acierto
        VampireState w; w.type = 1; w.life = 300; w.flags = VF_HITCHECK | VF_INVULN; EventList ev4; VampireLogic::checkDamage(w, e, h, ev4);
        CHECK(w.life == 300 && count(ev4, EV_HITNUM) == 1 && count(ev4, EV_SCORE) == 0);
        // sin VF_HITCHECK: nada
        VampireState z; z.life = 10; EventList ev5; VampireLogic::checkDamage(z, e, h, ev5); CHECK(ev5.empty());
    }
    // --- sufferDamage por clase de arma ---
    {
        EnemyEnv e = baseEnv(); VampireState v; EventList ev;
        CHECK(VampireLogic::sufferDamage(v, e, ev, 0, 3, 4) == 0);
        CHECK(VampireLogic::sufferDamage(v, e, ev, 0, 3, 3) == 0x180010);
        CHECK(VampireLogic::sufferDamage(v, e, ev, 0, 3, 2) == 0x20025);
        e.gameState = 0xA; e.gameKind = 1; e.pl[0].weapon = 0xE; EventList ev2; VampireLogic::sufferDamage(v, e, ev2, 0, 3, 2);
        CHECK(count(ev2, EV_EFFECT_TRACER) == 1 && count(ev2, EV_EFFECT_HIT) == 0);
    }
    // --- damagePlayer ---
    {
        EnemyEnv e = baseEnv(); VampireState v; v.reachLimit = -20.0f; EventList ev;
        e.playerCount = 1; e.solo = 1;
        CHECK(VampireLogic::damagePlayer(v, e, ev, 0.0f, 7) && find(ev, EV_DAMAGE_PLAYER)->a == 1 && find(ev, EV_DAMAGE_PLAYER)->c == 7);
        EventList ev2; CHECK(!VampireLogic::damagePlayer(v, e, ev2, -20.0f, 7));      // reach <= limite
        e.cam.state = 3; CHECK(!VampireLogic::damagePlayer(v, e, ev2, 0.0f, 7)); e.cam.state = 2;
        e.playerCount = 2; v.screenX = -5; EventList ev3; CHECK(VampireLogic::damagePlayer(v, e, ev3, 0.0f, 1) && find(ev3, EV_DAMAGE_PLAYER)->a == 0);
        v.screenX = 5; EventList ev4; CHECK(VampireLogic::damagePlayer(v, e, ev4, 0.0f, 1) && find(ev4, EV_DAMAGE_PLAYER)->a == 1);
        e.pl[1].mode = 3; EventList ev5; CHECK(!VampireLogic::damagePlayer(v, e, ev5, 0.0f, 1));   // modo 3 no recibe dano
        e.pl[1].mode = 1; e.pl[1].life = 0; EventList ev6; CHECK(!VampireLogic::damagePlayer(v, e, ev6, 0.0f, 1));
    }
    // --- chkParam ---
    {
        EnemyEnv e = baseEnv(); VampireState v; v.paramKind = 1; v.paramA = 10; v.frame3C = 5;
        CHECK(!VampireLogic::chkParam(v, e) && v.paramKind == 1); v.frame3C = 10; CHECK(VampireLogic::chkParam(v, e) && v.paramKind == -1);
        v.paramKind = 0; v.motActive = 1; CHECK(!VampireLogic::chkParam(v, e)); v.motActive = 0; CHECK(VampireLogic::chkParam(v, e));
        v.paramKind = 2; v.paramA = 4; v.paramB = 30; e.cam.pathId = 4; e.cam.frame = 29; CHECK(!VampireLogic::chkParam(v, e)); e.cam.frame = 30; CHECK(VampireLogic::chkParam(v, e));
    }
    // --- setAtcPlID (EnVampire) ---
    {
        EnemyEnv e = baseEnv(); VampireState v; e.playerCount = 1; e.solo = 1;
        CHECK(VampireLogic::setAtcPlID(v, e) == 1 && e.attackSlot[1] == 1 && v.atkPlayer == 1);
        VampireState v2; CHECK(VampireLogic::setAtcPlID(v2, e) == 0 && v2.atkPlayer == 0);     // hueco 1 ocupado y 0 libre -> ataca al 0
        VampireState v3; CHECK(VampireLogic::setAtcPlID(v3, e) == -1 && v3.atkPlayer == -1);   // ya estan los dos reservados
    }
    {
        EnemyEnv e = baseEnv(); VampireState v; e.attackSlot[0] = 1;                  // hueco 0 ocupado -> ataca al 1
        CHECK(VampireLogic::setAtcPlID(v, e) == 1);
        VampireState v2; EnemyEnv f = baseEnv(); f.attackSlot[0] = 1; f.attackSlot[1] = 1; CHECK(VampireLogic::setAtcPlID(v2, f) == -1);
        EnemyEnv g = baseEnv(); g.playerCount = 1; g.coop = 1; g_rand = 3; VampireState v3; CHECK(VampireLogic::setAtcPlID(v3, g) == 1);
        EnemyEnv h = baseEnv(); h.playerCount = 2; h.pl[0].life = 6; h.pl[1].life = 2; g_r01 = 0.2;   // bajo = jugador 1 (2), total 8: 8*0.2=1.6 < 2 -> elige al de menos vida
        VampireState v4; CHECK(VampireLogic::setAtcPlID(v4, h) == 1);
        EnemyEnv k = baseEnv(); k.playerCount = 2; k.pl[0].life = 6; k.pl[1].life = 2; g_r01 = 0.5; VampireState v5; CHECK(VampireLogic::setAtcPlID(v5, k) == 0);
    }
    // --- checkDamageMotEnd ---
    {
        int16_t len[8] = {0, 40, 0, 0, 0, 0, 0, 0}; VampireState v; v.curMot = 1; v.flags = VF_DAMAGE_MOT; v.curFrame = 19; v.motActive = 3;
        VampireLogic::checkDamageMotEnd(v, len); CHECK((v.flags & VF_DAMAGE_MOT) && v.motActive == 3);
        v.curFrame = 20; VampireLogic::checkDamageMotEnd(v, len); CHECK(!(v.flags & VF_DAMAGE_MOT) && v.motActive == -1);
        VampireState w; w.stateFlags = VS_DAMMOT_KIND; w.flags = VF_DAMAGE_MOT; w.layerBusy = 0;
        VampireLogic::checkDamageMotEnd(w, len); CHECK(!(w.flags & VF_DAMAGE_MOT) && !(w.stateFlags & VS_DAMMOT_KIND));
        VampireState u; u.stateFlags = VS_DAMMOT_KIND; u.layerBusy = 1; u.layerMot = 1; u.layerFrame = 10; VampireLogic::checkDamageMotEnd(u, len); CHECK(u.layerEndFrame == 40.0f);
        u.layerEndFrame = 0; u.layerFrame = 9; VampireLogic::checkDamageMotEnd(u, len); CHECK(u.layerEndFrame == 0.0f);
    }
    // --- Dead ---
    {
        EnemyEnv e = baseEnv(); VampireState v; VampireLogic::Counters c; c.alive = 5; c.onScreen = 5; v.atkPlayer = 1; e.attackSlot[1] = 1; v.type = 8; v.paramA = 25;
        VampireLogic::DeadIn in; in.aboveGround = false; EventList ev;
        VampireLogic::dead(v, e, c, in, ev);
        CHECK(c.alive == 4 && e.attackSlot[1] == 0 && v.atkPlayer == -1 && (v.flags2 & 2) && v.step == 3);
        CHECK(count(ev, EV_BEAT_RECORD) == 1); CHECK(v.motActive == 1 && v.paramA == 0x19 && v.paramKind == 0x14);
        v.motActive = 0; VampireLogic::dead(v, e, c, in, ev); CHECK((v.flags & VF_FINISHED) && c.onScreen == 4 && (v.flags2 & 4));
        // en el aire
        VampireState w; w.paramA = 10; VampireLogic::Counters c2; c2.alive = 1; VampireLogic::DeadIn in2; in2.aboveGround = true; in2.kickX = 0.5f; in2.kickZ = -1.0f; EventList ev2;
        VampireLogic::dead(w, e, c2, in2, ev2); CHECK(w.step == 1 && (w.flags & VF_AIRBORNE_DEAD) && (w.stateFlags & VS_AIRBORNE) && w.motLoop == -1);
        w.frame3C = 24; VampireLogic::dead(w, e, c2, in2, ev2); CHECK(w.step == 1);    // changeDeadMot fijo paramA = 25
        w.frame3C = 25; VampireLogic::dead(w, e, c2, in2, ev2); CHECK(w.step == 2 && w.velY == -1.5f && w.velX == 0.5f && w.velZ == -1.0f && w.gravity == -0.0392f && !(w.flags & VF_AIRBORNE_DEAD));
        in2.landed = false; VampireLogic::dead(w, e, c2, in2, ev2); CHECK(w.step == 2); in2.landed = true; VampireLogic::dead(w, e, c2, in2, ev2); CHECK(w.step == 3 && w.motActive == 1);
        w.velX = 0.5f; w.velZ = 0.5f; VampireLogic::dead(w, e, c2, in2, ev2); CHECK(std::fabs(w.velX - 0.4f) < 1e-6);
        for (int i = 0; i < 40; i++) VampireLogic::dead(w, e, c2, in2, ev2);
        CHECK(!(w.stateFlags & VS_AIRBORNE) && w.velX == 0 && w.velZ == 0);
    }
    // --- salto de ataque: la trayectoria llega al objetivo (y - 5) en T fotogramas ---
    {
        VampireState v; v.posX = 1; v.posY = 2; v.posZ = 3; EventList ev; VampireLogic::attackJump(v, 72.0f, 11, 7, -9, ev);
        float T = 72.0f / 1.44f;
        CHECK(std::fabs(v.posX + v.velX * T - 11) < 1e-3); CHECK(std::fabs(v.posZ + v.velZ * T + 9) < 1e-3);
        CHECK(std::fabs(v.posY + v.velY * T + 0.5f * v.gravity * T * T - (7 - 5)) < 1e-2);
        CHECK(v.step == 3 && v.speedScale == 1.2f && (v.stateFlags & VS_AIRBORNE));
    }
    // --- changeDeadMot ---
    {
        VampireState v; v.stateFlags = VS_DEADSND; v.emitter = 3; EventList ev; VampireLogic::changeDeadMot(v, ev);
        CHECK(!(v.stateFlags & VS_DEADSND) && v.speedScale == 1.2f && find(ev, EV_SOUND)->a == 0x17030D && find(ev, EV_SOUND)->b == (3 << 12) + 0x17030D);
        CHECK(find(ev, EV_MOTION)->a == 0x49 && find(ev, EV_MOTION)->b == 10 && count(ev, EV_ADJ_CHEST) == 0);
        VampireState w; w.flags = VF_BIT2000; EventList ev2; VampireLogic::changeDeadMot(w, ev2); CHECK(count(ev2, EV_ADJ_CHEST) == 1);
    }

    // --- Attack_Eknd ---
    {
        EnemyEnv e = baseEnv(); e.playerCount = 1; e.solo = 0;
        VampireState v; VampireLogic::AttackIn in; int16_t len[0x400] = {0}; len[0x30D] = 60; in.motLen = len; EventList ev;
        v.curMot = 0x100;                                                       // distinta de 0x312
        VampireLogic::attackEknd(v, e, in, ev);                                 // paso 0 -> 1: pide animacion 0x312
        CHECK(v.motId == 0x312 && v.step == 1 && v.atkPlayer == 0 && find(ev, EV_MOTION)->a == 0x312 && find(ev, EV_MOTION)->b == 10);
        v.curMot = 0x312; v.frame3C = 44; v.curFrame = 25; v.speedScale = 1.0f; EventList ev1; VampireLogic::attackEknd(v, e, in, ev1);
        CHECK(v.step == 1 && find(ev1, EV_SOUND)->a == 0x17030C);              // ventana de sonido en el fotograma 25
        v.frame3C = 45; v.curFrame = 0; v.h1DD4 = 0x313; v.emitter = 2; EventList ev2; VampireLogic::attackEknd(v, e, in, ev2);
        CHECK(v.step == 2 && v.motLoop == -1 && (v.stateFlags & 0x10) && v.speedScale == 0.96f && find(ev2, EV_MOTION)->a == 0x313 && find(ev2, EV_SOUND)->b == (2 << 12) + 0x17030D);
        // paso 2: ventana de sonido (anim 0x325, frame 20) y salto cuando dist < 50
        v.curMot = 0x325; v.curFrame = 20; v.speedScale = 1.0f; v.dist1DD0 = 60; EventList ev3; VampireLogic::attackEknd(v, e, in, ev3);
        CHECK(find(ev3, EV_SOUND)->a == 0x2001F && find(ev3, EV_SCRIPT)->a == 200);   // 50 <= 60 < 80 -> pide el salto (Attack_Eknd_Sub)
        v.dist1DD0 = 79; v.curFrame = 100; EventList ev4; VampireLogic::attackEknd(v, e, in, ev4); CHECK(find(ev4, EV_SCRIPT)->a == 200);
        v.dist1DD0 = 90; EventList ev5; VampireLogic::attackEknd(v, e, in, ev5); CHECK(ev5.empty());
        v.dist1DD0 = 79; v.curMot = 0x313; v.curFrame = 3.0f; EventList ev6; VampireLogic::attackEknd(v, e, in, ev6); CHECK(ev6.empty());      // frame <= 3: espera
        v.curFrame = 11.0f; EventList ev7; VampireLogic::attackEknd(v, e, in, ev7); CHECK(ev7.empty());
        v.curFrame = 5.0f; EventList ev8; VampireLogic::attackEknd(v, e, in, ev8); CHECK(find(ev8, EV_SCRIPT)->a == 200);
        v.dist1DD0 = 49; v.atkPlayer = -1; EventList ev9; VampireLogic::attackEknd(v, e, in, ev9); CHECK(find(ev9, EV_SCRIPT)->a == 200);
        // paso 3: golpea al jugador al acercarse
        v.step = 3; v.dist1DD0 = 16; EventList ev10; VampireLogic::attackEknd(v, e, in, ev10); CHECK(v.step == 3 && ev10.empty());
        v.dist1DD0 = 15; v.stateFlags |= VS_AIRBORNE; e.pl[0].mode = 5; e.pl[0].life = 3; v.reachLimit = -20.0f; in.reach = 0;
        EventList ev11; VampireLogic::attackEknd(v, e, in, ev11);
        CHECK(v.step == 4 && !(v.stateFlags & VS_AIRBORNE) && find(ev11, EV_DAMAGE_PLAYER)->a == 0 && find(ev11, EV_DAMAGE_PLAYER)->c == 7);
        // paso 4: rebote tras 10 fotogramas
        v.velX = 1; v.velY = 2; v.velZ = 3; v.aiTimer = 9; EventList ev12; VampireLogic::attackEknd(v, e, in, ev12); CHECK(v.step == 4);
        v.aiTimer = 10; e.attackSlot[0] = 1; v.atkPlayer = 0; VampireLogic::attackEknd(v, e, in, ev12);
        CHECK(v.step == 5 && (v.stateFlags & VS_AIRBORNE) && std::fabs(v.velX + 1.2f) < 1e-6 && std::fabs(v.velZ + 3.6f) < 1e-5 && e.attackSlot[0] == 0 && v.atkPlayer == -1);
        // paso 5: sin llegar al suelo no hace nada; aterriza si dist > marca
        in.height = 0; v.posY = 6; v.dist1DD0 = 10; v.dist1DC0 = 20; in.speedXZ = 1.0f; EventList ev13; VampireLogic::attackEknd(v, e, in, ev13); CHECK(v.step == 5 && ev13.empty());
        v.posY = 4; EventList ev14; VampireLogic::attackEknd(v, e, in, ev14); CHECK(v.step == 5 && v.velX < 0);       // rebote: dist <= marca y velocidad >= 0.5 pero posY >= suelo -> nada
        v.posY = -1; v.velX = 1; v.velY = 2; v.velZ = 4; EventList ev15; VampireLogic::attackEknd(v, e, in, ev15);
        CHECK(v.step == 5 && v.velX == 0.75f && v.velY == -1.5f && v.velZ == 3.0f);
        v.posY = 4; in.speedXZ = 0.4f; EventList ev16; VampireLogic::attackEknd(v, e, in, ev16);                         // lento -> aterriza
        CHECK(v.step == 6 && v.motLoop == 0 + (v.hasMark1DD8 ? 0 : v.motLoop) && v.gravity == 0 && v.velY == 0 && !(v.stateFlags & 0x18));
        CHECK(find(ev16, EV_MOTION)->a == 0x30D && count(ev16, EV_SOUND) == 2);
        // paso 6: sonido en frame 25 y fin
        v.curMot = 0x30D; v.frame3C = 25; v.speedScale = 1.0f; EventList ev17; VampireLogic::attackEknd(v, e, in, ev17); CHECK(v.step == 6 && find(ev17, EV_SOUND)->a == 0x20022);
        v.frame3C = 58; v.speedScale = 1.0f; EventList ev18; VampireLogic::attackEknd(v, e, in, ev18); CHECK(v.step == 6);
        v.frame3C = 59; VampireLogic::attackEknd(v, e, in, ev18); CHECK(v.step == 0 && v.aiMode == 1 && v.aiTimer == 0);
        // paso 5 aterrizaje por distancia (dist1DD0 > dist1DC0) con salto marcado
        VampireState w; w.step = 5; w.posY = 0; w.dist1DD0 = 30; w.dist1DC0 = 20; w.h1DD4 = 0x313; w.hasMark1DD8 = true; w.motLoop = 5; EventList ev19;
        VampireLogic::attackEknd(w, e, in, ev19); CHECK(w.step == 6 && w.motLoop == 0);
        VampireState x; x.step = 5; x.posY = 0; x.dist1DD0 = 30; x.dist1DC0 = 20; x.h1DD4 = -1; x.motLoop = 5; EventList ev20; VampireLogic::attackEknd(x, e, in, ev20); CHECK(x.motLoop == -1);
        // con marca: velocidad hacia la marca
        VampireState y; y.step = 4; y.aiTimer = 11; y.hasMark1DC4 = true; y.posX = 0; y.posZ = 0; in.markX = 3; in.markZ = 4; in.speedXZ = 10; in.camToMark = 77;
        EventList ev21; VampireLogic::attackEknd(y, e, in, ev21); CHECK(std::fabs(y.velX - 6.0f) < 1e-5 && std::fabs(y.velZ - 8.0f) < 1e-5 && y.dist1DC0 == 77 && y.step == 5);
    }

    // ===== PeopleTask =====
    {   // contadores
        PeopleCounters c; EnemyEnv e = baseEnv(); e.stage = 3; EventList ev;
        PeopleLogic::addPeopleNum(c, 3); PeopleLogic::addPeopleNum(c, 3); CHECK(c.total == 2 && c.totalByStage[3] == 2);
        PeopleLogic::addRescuePeopleNum(c, e, ev); CHECK(c.rescued == 1 && c.rescuedByStage[3] == 1 && count(ev, EV_GALLERY) == 1);
        e.gameState = 0xA; e.gameKind = 2; EventList ev2; PeopleLogic::addRescuePeopleNum(c, e, ev2); CHECK(c.rescued == 2 && ev2.empty());
    }
    {   // golpe a civil-vampiro (clase de parte y dano base)
        EnemyEnv e = baseEnv(); PeopleTaskState p; p.type = 1; p.life = 100; p.flags = PF_HITCHECK; p.emitter = 2; e.stage = 0;
        HitInput h; h.shooter[0] = 1; h.partOfPlayer[1] = 1;               // parte 1 -> clase 2 -> tipo 1: 60
        g_rand = 1; EventList ev; PeopleLogic::checkVampDamage(p, e, h, ev);
        CHECK(p.life == 40 && !(p.flags & PF_DEAD)); CHECK(count(ev, EV_SCORE) == 1 && find(ev, EV_SCORE)->a == 1 && find(ev, EV_SCORE)->b == 10);
        CHECK(count(ev, EV_HITNUM) == 1);
        CHECK(find(ev, EV_SOUND)->a == 0x15000C && find(ev, EV_SOUND)->b == (2 << 12));      // rand impar, fase 0
        CHECK((p.flags & PF_BIT80000) && (p.flags & PF_DAMAGE_MOT) && (p.stateFlags & PS_DAMMOT_KIND));   // parte <= 10: motDivOnce
        // parte 12 (>= 11): motSupplement + loop = 1
        PeopleTaskState q; q.type = 0; q.life = 1000; q.flags = PF_HITCHECK; HitInput h2; h2.shooter[1] = 0; h2.partOfPlayer[0] = 12; g_rand = 0; EventList ev2;
        PeopleLogic::checkVampDamage(q, e, h2, ev2);
        CHECK(q.life == 1000 - 45 * 0 - kPeopleBaseDamage[0][6] && q.loop == 1 && !(q.stateFlags & PS_DAMMOT_KIND) && find(ev2, EV_MOTION)->a == 6101 && q.p1DD0 == 0);
        CHECK(find(ev2, EV_SOUND)->a == 0x15000D);                                               // rand par
        // muerte: +100 y +10, sonido de muerte de la fase
        PeopleTaskState d; d.life = 30; d.type = 2; d.flags = PF_HITCHECK; e.stage = 2; HitInput h3; h3.shooter[0] = 0; h3.partOfPlayer[0] = 1; EventList ev3;   // 180*... clase 2 tipo 2: 70
        PeopleLogic::checkVampDamage(d, e, h3, ev3);
        CHECK((d.flags & PF_DEAD) && count(ev3, EV_SCORE) == 2 && find(ev3, EV_SCORE)->b == 100 && find(ev3, EV_SCORE, 1)->b == 10 && find(ev3, EV_SOUND)->a == 0x080011);
        // fase 5: sin sonido
        PeopleTaskState s; s.life = 500; s.flags = PF_HITCHECK; e.stage = 5; EventList ev4; PeopleLogic::checkVampDamage(s, e, h3, ev4); CHECK(count(ev4, EV_SOUND) == 0);
        // invulnerable: solo cuenta acierto
        PeopleTaskState iv; iv.life = 500; iv.flags = PF_HITCHECK | PF_INVULN; EventList ev5; PeopleLogic::checkVampDamage(iv, e, h3, ev5); CHECK(iv.life == 500 && count(ev5, EV_HITNUM) == 1 && count(ev5, EV_SCORE) == 0);
        // flag 0x2000: sin animacion de dano
        PeopleTaskState f; f.life = 500; f.flags = PF_HITCHECK | PF_BIT2000; e.stage = 0; EventList ev6; PeopleLogic::checkVampDamage(f, e, h3, ev6); CHECK(!(f.flags & PF_DAMAGE_MOT) && !(f.flags & PF_BIT80000));
    }
    {   // checkParaDamage: rescate
        EnemyEnv e = baseEnv(); PeopleTaskState p; PeopleCounters c; uint32_t partner = 8 | 4; HitInput h; EventList ev; e.stage = 1;
        PeopleLogic::checkParaDamage(p, partner, e, h, c, ev);
        CHECK(p.rescuer == 1 && c.rescued == 1 && (p.stateFlags & 0x11) == 0x11); CHECK(find(ev, EV_SCORE)->a == 1 && find(ev, EV_SCORE)->b == 400);
        bool snd = false; for (auto& x : ev) if (x.type == EV_SOUND && x.a == 0x2002D) snd = true; CHECK(snd && count(ev, EV_CAMERA_CHG) == 1);
        CHECK(count(ev, EV_SOUND) == 1);                                    // el bit 2 no esta puesto: sin sonido de fase
        PeopleTaskState q; uint32_t partner2 = 8 | 2; EventList ev2; PeopleLogic::checkParaDamage(q, partner2, e, h, c, ev2); CHECK(q.rescuer == 0);
        PeopleTaskState r; r.flags = PF_INVULN; uint32_t partner3 = 8 | 6; EventList ev3; PeopleLogic::checkParaDamage(r, partner3, e, h, c, ev3); CHECK((partner3 & 0xE) == 0 && count(ev3, EV_SCORE) == 0);
        // golpe directo al civil: -200 una sola vez
        PeopleTaskState s; s.flags = PF_HITCHECK; uint32_t none = 0; HitInput h2; h2.shooter[0] = 0; h2.partOfPlayer[0] = 4; EventList ev4; e.stage = 4;
        PeopleLogic::checkParaDamage(s, none, e, h2, c, ev4);
        CHECK(s.hitPart == 4 && (s.stateFlags & PS_PARA_HIT) && find(ev4, EV_SCORE)->b == -200 && find(ev4, EV_SOUND)->a == 0x0A000E);
        EventList ev5; PeopleLogic::checkParaDamage(s, none, e, h2, c, ev5); CHECK(count(ev5, EV_SCORE) == 0 && count(ev5, EV_HITNUM) == 1 && find(ev5, EV_SOUND)->a == 0x0A000E);
    }
    {   // damagePlayer civil
        EnemyEnv e = baseEnv(); PeopleTaskState p; EventList ev; e.playerCount = 2; p.screenX = -1; p.screenLimit = 0;
        CHECK(PeopleLogic::damagePlayer(p, e, ev) && find(ev, EV_DAMAGE_PLAYER)->a == 0 && find(ev, EV_DAMAGE_PLAYER)->c == 9);
        p.screenLimit = -10; EventList ev2; CHECK(!PeopleLogic::damagePlayer(p, e, ev2));
    }
    {   // cambios de estado
        EnemyEnv e = baseEnv(); e.stage = 6; e.attackSlot[0] = 1; PeopleTaskState p; p.atkPlayer = 0; p.flags = PF_BIT10000000; PeopleCounters c; c.stage6Alive = 4; uint32_t partner = 0; EventList ev;
        PeopleLogic::changeSurvive(p, e, c, partner, ev);
        CHECK(!(p.flags & PF_BIT10000000) && (p.flags & PF_BIT80000) && partner == PF_DEAD && p.waitKind == 0 && p.loop == 1 && e.attackSlot[0] == 0 && p.atkPlayer == -1 && c.stage6Alive == 3 && p.p1DD0 == -1);
        PeopleLogic::changeVampire(p, e, c, ev); CHECK((p.flags & PF_INVULN) && p.waitKind == 0x2A && c.flag500E34 == 1 && c.flag7200 == 1);
        e.stage = 2; PeopleCounters c2; PeopleLogic::changeVampire(p, e, c2, ev); CHECK(c2.flag500E34 == 0);
    }
    {   // lifeUp
        EnemyEnv e = baseEnv(); PeopleTaskState p; p.rescuer = 0; p.emitter = 1; EventList ev; PeopleLogic::lifeUp(p, e, ev);
        CHECK(find(ev, EV_SOUND)->a == 0x2002A && find(ev, EV_SCORE) && find(ev, EV_SCORE)->b == 1000 && !find(ev, EV_LIFEUP_ITEM));
        p.rescuer = 1; EventList ev2; PeopleLogic::lifeUp(p, e, ev2); CHECK(find(ev2, EV_LIFEUP_ITEM)->a == 1 && !find(ev2, EV_SCORE));
    }
    {   // setAtcPlID civil, motionSelect
        EnemyEnv e = baseEnv(); PeopleTaskState p; e.playerCount = 1; e.solo = 0; g_rand = 7;
        CHECK(PeopleLogic::setAtcPlID(p, e, 3) == 7 % 3 && e.attackSlot[0] == 1 && p.atkPlayer == 0);
        PeopleTaskState q; CHECK(PeopleLogic::setAtcPlID(q, e, 3) == -1 && q.atkPlayer == -1);
        EnemyEnv f = baseEnv(); f.playerCount = 2; f.pl[1].invuln = true; PeopleTaskState r; PeopleLogic::setAtcPlID(r, f, 2); CHECK(r.atkPlayer == 0);
        EnemyEnv g = baseEnv(); g.playerCount = 2; g.pl[0].invuln = true; PeopleTaskState s; PeopleLogic::setAtcPlID(s, g, 2); CHECK(s.atkPlayer == 1);
        int16_t list[3] = {10, 20, 30}; g_rand = 5; CHECK(PeopleLogic::motionSelect(list, 3, fakeRand) == 30);
    }
    {   // sonidos groan / helpMe
        EnemyEnv e = baseEnv(); PeopleTaskState p; p.type = 1; e.stage = 0; CHECK(PeopleLogic::groanSound(p, e) == 0x15001D);
        p.type = 0; g_rand = 1; CHECK(PeopleLogic::groanSound(p, e) == 0x150015); g_rand = 2; CHECK(PeopleLogic::groanSound(p, e) == 0x150016);
        e.stage = 4; g_rand = 1; CHECK(PeopleLogic::groanSound(p, e) == 0x0A000C); e.stage = 5; CHECK(PeopleLogic::groanSound(p, e) == 0);
        e.stage = 6; g_rand = 2; CHECK(PeopleLogic::groanSound(p, e) == 0x150016);
        e.stage = 0; CHECK(PeopleLogic::helpMeSound(p, e) == 0x150017); p.type = 1; CHECK(PeopleLogic::helpMeSound(p, e) == 0x150020);
        p.type = 0; e.stage = 3; g_rand = 1; CHECK(PeopleLogic::helpMeSound(p, e) == 0x090019); g_rand = 0; CHECK(PeopleLogic::helpMeSound(p, e) == 0x090010);
        p.type = 1; CHECK(PeopleLogic::helpMeSound(p, e) == 0x090017);
    }
    {   // checkVampDamageMotEnd civil y esperas
        int16_t len[4] = {0, 30, 0, 0}; PeopleTaskState p; p.curMot = 1; p.flags = PF_DAMAGE_MOT; p.curFrame = 20; p.speed = 5;
        PeopleLogic::checkVampDamageMotEnd(p, len); CHECK((p.flags & PF_DAMAGE_MOT) && p.loop == 0);
        p.speed = 10; PeopleLogic::checkVampDamageMotEnd(p, len); CHECK(!(p.flags & PF_DAMAGE_MOT) && p.loop == -1);
        PeopleTaskState q; q.stateFlags = PS_DAMMOT_KIND; q.flags = PF_DAMAGE_MOT; q.layerBusy = 1; PeopleLogic::checkVampDamageMotEnd(q, len); CHECK(q.flags & PF_DAMAGE_MOT);
        q.layerBusy = 0; PeopleLogic::checkVampDamageMotEnd(q, len); CHECK(!(q.flags & PF_DAMAGE_MOT) && !(q.stateFlags & PS_DAMMOT_KIND));
    }
    {
        EnemyEnv e = baseEnv(); PeopleCounters c; c.walking = 3; WaitIn in; uint8_t flags[8] = {0}; in.eventFlags = flags; int16_t len[2] = {0, 100};
        PeopleTaskState p; p.waitKind = 0; p.loop = 1; p.curMot = 1; p.curFrame = 10;
        CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len) && p.waitKind == 0 && p.loop == 1);
        p.curFrame = 100; in.baseMatch = true; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len) && p.waitKind == -1 && p.loop == 0 && c.walking == 2);
        p.waitKind = 1; p.frame3C = 4; p.p1DC0 = 5; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); p.frame3C = 5; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 3; p.p1DC0 = 2; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); flags[2] = 1; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 7; p.p1DC0 = 4; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); in.backWaitDone = true; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len)); in.backWaitDone = false;
        p.waitKind = 39; p.counter1E64 = 0; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 37; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 5; in.viewScale = 1.0f; in.angleDeg = 0.5f; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); in.angleDeg = 0.8f; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));   // limite 640/448/2 = 0.714
        p.waitKind = 6; p.p1DC0 = 50; in.dist1DF8 = 51; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); in.dist1DF8 = 50; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 2; p.p1DC0 = 3; p.p1DC4 = 8; e.cam.pathId = 3; e.cam.frame = 8; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 4; in.onScreen = true; CHECK(!PeopleLogic::lastCtrlWait(p, e, c, in, len)); in.onScreen = false; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
        p.waitKind = 1; p.frame3C = 0; p.p1DC0 = 5; in.abortAll = true; CHECK(PeopleLogic::lastCtrlWait(p, e, c, in, len));
    }

    // ===== Interpretes de script (getParam) =====
    {   // EnVampire: flags, sonido, espera por fotograma; la lectura se detiene en la espera
        const uint8_t sc[] = {9,0, 0,0, 0x00,0x01,0,0,        // op 9: flags |= 0x100
                              19,0, 0,0, 0x0D,0x03,0x17,0,    // op 19: sonido 0x17030D
                              8,0, 0,0, 0,0,0,0x40,           // op 8: speedScale = 1.2 * 2.0
                              1,0, 10,0,                      // op 1: espera frame3C >= 10
                              9,0, 0,0, 0xFF,0,0,0};          // (no se ejecuta)
        EnemyEnv e = baseEnv(); VampireState v; EventList ev;
        int n = VampireLogic::getParam(v, e, sc, sizeof sc, ev, 100);
        CHECK(n == 4 && (v.flags & 0x100) && v.paramKind == 1 && v.paramA == 10 && v.scriptPos == 2 + 6 + 2 + 6 + 2 + 6 + 2 + 2);
        CHECK(std::fabs(v.speedScale - 2.4f) < 1e-6 && find(ev, EV_SOUND)->a == 0x17030D);
        CHECK(VampireLogic::getParam(v, e, sc, sizeof sc, ev, 100) == 0);        // ya hay una espera: no lee
        v.frame3C = 10; CHECK(VampireLogic::chkParam(v, e)); CHECK(VampireLogic::getParam(v, e, sc, sizeof sc, ev, 100) == 1 && (v.flags & 0xFF));
        // orden 13, 17, 18, 20
        const uint8_t sc2[] = {13,0, 0xFE,0,   17,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,   18,0, 1,0,   2,0, 4,0, 30,0, 0,0};
        VampireState w; EventList ev2; VampireLogic::getParam(w, e, sc2, sizeof sc2, ev2, 100);
        CHECK(w.aiMode == -2 && w.motActive == -1 && w.step == 0 && w.scriptMark1DC4 == 4 + 2 + 2 && (w.flags1CCC & 2) && w.paramKind == 2 && w.paramA == 4 && w.paramB == 30);
        // fin de flujo: no se pasa del limite
        const uint8_t sc3[] = {5,0}; VampireState z; EventList ev3; VampireLogic::getParam(z, e, sc3, sizeof sc3, ev3, 100); CHECK(z.scriptPos <= 4);
    }
    {   // PeopleTask: ordenes sin espera, esperas, y modo de salto (abortAll)
        const uint8_t sc[] = {13,0, 0,0, 0x80,0,0,0,          // flags |= 0x80
                              28,0, 0,0, 0x2A,0,2,0,          // sonido 0x2002A
                              18,0, 60,0,                     // p1DC8 = 60*50/60 = 50
                              1,0, 20,0,                      // espera frame
                              13,0, 0,0, 0x40,0,0,0};
        EnemyEnv e = baseEnv(); PeopleTaskState p; p.emitter = 2; PeopleCounters c; VmEnv vm; EventList ev;
        int n = PeopleLogic::getParam(p, e, c, vm, sc, sizeof sc, ev, 100);
        CHECK(n == 4 && (p.flags & 0x80) && !(p.flags & 0x40) && p.waitKind == 1 && p.p1DC0 == 20 && p.p1DC8 == 50);
        CHECK(find(ev, EV_SCRIPT)->a == 28 && find(ev, EV_SCRIPT)->b == 0x2002A && find(ev, EV_SCRIPT)->d == (2 << 12));
        // abortAll: las esperas se saltan y los sonidos (orden 28) tambien tras la primera cancelacion
        PeopleTaskState q; PeopleCounters c2; VmEnv vm2; vm2.abortAll = 1; EventList ev2;
        const uint8_t sc2[] = {1,0, 5,0,  28,0, 0,0, 1,0,0,0,  13,0, 0,0, 0x10,0,0,0};
        PeopleLogic::getParam(q, e, c2, vm2, sc2, sizeof sc2, ev2, 100);
        CHECK(q.waitKind == -1 && (q.flags & 0x10) && count(ev2, EV_SCRIPT) == 0);
        // sin abortAll, la orden 37 se cancela sola; la 37 con abortAll se mantiene
        PeopleTaskState r; PeopleCounters c3; VmEnv vm3; EventList ev3; const uint8_t sc3[] = {37,0, 0,0};
        PeopleLogic::getParam(r, e, c3, vm3, sc3, sizeof sc3, ev3, 10); CHECK(r.waitKind == -1);
        PeopleTaskState r2; VmEnv vm4; vm4.abortAll = 1; PeopleLogic::getParam(r2, e, c3, vm4, sc3, sizeof sc3, ev3, 10); CHECK(r2.waitKind == 37);
        // ordenes 16/38/17/20/25/26 y flujo de caminar
        PeopleTaskState t; PeopleCounters c4; c4.walking = 2; c4.stage6Alive = 3; VmEnv vm5; uint8_t evf[8] = {0}; vm5.eventFlags = evf; e.attackSlot[1] = 1; t.atkPlayer = 1; EventList ev4;
        const uint8_t sc4[] = {17,0, 0,0,   20,0, 0,0,   25,0, 3,0,   26,0, 7,0,   38,0, 0,0};
        PeopleLogic::getParam(t, e, c4, vm5, sc4, sizeof sc4, ev4, 20);
        CHECK(c4.stage6Alive == 2 && e.attackSlot[1] == 0 && t.atkPlayer == -1 && evf[3] == 1 && t.p1DD0 == 7 && c4.walking == 1 && t.waitKind == 38);
        // orden 6: espera solo si la distancia supera 5 + param
        PeopleTaskState u; PeopleCounters c5; VmEnv vm6; vm6.dist1DF8 = 12; EventList ev5; const uint8_t sc6[] = {6,0, 6,0};
        PeopleLogic::getParam(u, e, c5, vm6, sc6, sizeof sc6, ev5, 10); CHECK(u.waitKind == 6 && u.p1DD0 == -1);
        PeopleTaskState u2; vm6.dist1DF8 = 11; PeopleLogic::getParam(u2, e, c5, vm6, sc6, sizeof sc6, ev5, 10); CHECK(u2.waitKind == -1);
        // orden 35: lifeUp solo en modo normal; 40 incrementa el contador global
        PeopleTaskState w; w.rescuer = 1; PeopleCounters c6; VmEnv vm7; EventList ev6; const uint8_t sc7[] = {35,0, 0,0,  40,0, 0,0};
        PeopleLogic::getParam(w, e, c6, vm7, sc7, sizeof sc7, ev6, 10); CHECK(find(ev6, EV_LIFEUP_ITEM) && vm7.g500F04 == 1);
        // 0x200 en el estado: toma un hueco de caminar si no hay (walking <= 0)
        PeopleTaskState x; x.stateFlags = 0x200; PeopleCounters c7; VmEnv vm8; EventList ev7; const uint8_t sc8[] = {42,0};
        PeopleLogic::getParam(x, e, c7, vm8, sc8, sizeof sc8, ev7, 10); CHECK(c7.walking == 1 && !(x.stateFlags & 0x200) && x.waitKind == 42);
    }
    printf(fails ? "enemies_test: %d FALLOS\n" : "enemies_test: OK\n", fails);
    return fails ? 1 : 0;
}

// Tests de game/enemies_rest. Las tablas se comparan con los ficheros reales (disc/TBL/GAMEMOD.BIN, baserom/SCES_504.11); si no estan se omiten con mensaje.
// Compilar y ejecutar desde la raiz del repo:
//   c++ -std=c++17 -Wall game/test/enemies_rest_test.cpp game/enemies_rest/enemies_rest.cpp game/enemies_rest/rest_tables.cpp game/enemies/enemy_common.cpp game/enemies/vampire.cpp game/enemies/vampire_tables.cpp game/enemies/people.cpp -o build/verify_enemies_rest/enemies_rest_test && ./build/verify_enemies_rest/enemies_rest_test
#include "../enemies_rest/enemies_rest.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iterator>
using namespace vn::en;
using namespace vn::en::rest;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static int g_seq[16]; static int g_n = 0, g_i = 0;
static int fakeRand() { int v = g_i < g_n ? g_seq[g_i] : 0; g_i++; return v; }
static void setRand(std::initializer_list<int> l) { g_n = 0; g_i = 0; for (int v : l) g_seq[g_n++] = v; }
static int count(const EventList& ev, EvType t) { int n = 0; for (auto& e : ev) if (e.type == t) n++; return n; }
static int countS(const EventList& ev, int code) { int n = 0; for (auto& e : ev) if (e.type == EV_SCRIPT && e.a == code) n++; return n; }
static const Event* find(const EventList& ev, EvType t, int nth = 0) { for (auto& e : ev) if (e.type == t && nth-- == 0) return &e; return nullptr; }
static const Event* findS(const EventList& ev, int code) { for (auto& e : ev) if (e.type == EV_SCRIPT && e.a == code) return &e; return nullptr; }

static std::vector<unsigned char> readAll(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static std::vector<unsigned char> g_bin, g_elf;
static bool binOk(unsigned v, unsigned n) { return v >= 0x92B000 && v - 0x92B000 + n <= g_bin.size(); }
static bool elfOk(unsigned v, unsigned n) { return v >= 0x140000 && v - 0x140000 + 0x800 + n <= g_elf.size(); }
static uint32_t bw(unsigned v) { const unsigned char* p = &g_bin[v - 0x92B000]; return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24); }
static int16_t bh(unsigned v) { const unsigned char* p = &g_bin[v - 0x92B000]; return (int16_t)(p[0] | (p[1] << 8)); }
static int8_t bb(unsigned v) { return (int8_t)g_bin[v - 0x92B000]; }
static uint32_t ew(unsigned v) { const unsigned char* p = &g_elf[v - 0x140000 + 0x800]; return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24); }

static EnemyEnv baseEnv() {
    EnemyEnv e; e.rand = fakeRand;
    e.pl[0].mode = 5; e.pl[0].life = 5; e.pl[0].maxLife = 5; e.pl[1].mode = 5; e.pl[1].life = 3; e.pl[1].maxLife = 5;
    return e;
}

int main() {
    // ---------------- tablas contra los binarios ----------------
    g_bin = readAll("disc/TBL/GAMEMOD.BIN"); g_elf = readAll("baserom/SCES_504.11");
    if (g_bin.empty() || g_elf.empty()) {
        printf("OMITIDO: faltan disc/TBL/GAMEMOD.BIN o baserom/SCES_504.11, no se comparan las tablas con los binarios\n");
    } else {
        for (int t = 0; t < 18; t++) {
            unsigned a = bw(0x9A22C0 + 4 * t);
            CHECK(a >= 0x2D8860 && (a - 0x2D8860) % 4 == 0 && kWaitMotBase[t] == (a - 0x2D8860) / 4);
            unsigned c = bw(0x9A2470 + 4 * t);
            CHECK(binOk(c, 8) && kEkndChainParam[t][0] == bb(c) && kEkndChainParam[t][1] == bb(c + 4));
            CHECK(bw(0x9A23D0 + 4 * t) == 0x9A23C0);
        }
        for (int s = 0; s < 4; s++) CHECK(bw(0x9A23C0 + 4 * s) == 0x9B5D50);
        for (int i = 0; i < 16; i++) CHECK(kDamageMotByClass[i] == bh(0x9B5D50 + 2 * i));
        for (int i = 0; i < 25; i++) {
            unsigned p = ew(0x2D8860 + 4 * i);
            CHECK(elfOk(0x2D8860 + 4 * i, 4) && binOk(p, 4));
            CHECK(kWaitMotPairs[i][0] == bh(p) && kWaitMotPairs[i][1] == bh(p + 2));
        }
        printf("tablas comparadas con los binarios\n");
    }
    // valores concretos (leidos a mano de la salida del extractor)
    CHECK(kWaitMotBase[0] == 0 && kWaitMotBase[3] == 3 && kWaitMotBase[4] == 2 && kWaitMotBase[17] == 13);
    CHECK(kWaitMotPairs[0][0] == 15 && kWaitMotPairs[8][0] == 6023 && kWaitMotPairs[13][1] == 765);
    CHECK(kEkndChainParam[2][0] == 5 && kEkndChainParam[2][1] == 10 && kEkndChainParam[0][1] == 40);
    CHECK(kDamageMotByClass[0] == 132 && kDamageMotByClass[7] == 132);

    // ---------------- changeDamageMot ----------------
    {
        VampireState v; EventList ev; DamageMotIn in;
        v.flags = 0x2000; RestLogic::changeDamageMot(v, ev, 12, 0, in); CHECK(ev.empty() && v.flags == 0x2000);       // 0x10002000: nada
        v.flags = 0x10000000; RestLogic::changeDamageMot(v, ev, 12, 0, in); CHECK(ev.empty());
        // parte alta (>= 11): capa de dano con animacion de la clase (parte 12 -> clase 6 -> 132)
        v = VampireState(); ev.clear();
        RestLogic::changeDamageMot(v, ev, 12, 0, in);
        CHECK(v.flags & VF_DAMAGE_MOT); CHECK(v.motActive == 1); CHECK(!(v.stateFlags & VS_DAMMOT_KIND));
        const Event* m = find(ev, EV_MOTION); CHECK(m && m->a == 132 && m->b == 10 && m->c == 1);
        // arma clase 2: no
        v = VampireState(); ev.clear(); RestLogic::changeDamageMot(v, ev, 12, 2, in); CHECK(ev.empty() && v.flags == 0);
        // con la capa ya activa y sin bit 4: no
        v = VampireState(); v.flags = VF_DAMAGE_MOT; ev.clear(); RestLogic::changeDamageMot(v, ev, 12, 0, in); CHECK(ev.empty());
        // con la capa activa y bit 4 (arranque por parte baja): se convierte (bit 4 se limpia)
        v = VampireState(); v.flags = VF_DAMAGE_MOT; v.stateFlags = VS_DAMMOT_KIND; ev.clear();
        RestLogic::changeDamageMot(v, ev, 12, 0, in); CHECK(find(ev, EV_MOTION) && !(v.stateFlags & VS_DAMMOT_KIND) && v.motActive == 1);
        // parte baja (< 11): marca el flag y el bit 4, evento de arranque; una segunda vez no hace nada
        v = VampireState(); ev.clear();
        RestLogic::changeDamageMot(v, ev, 5, 0, in);
        CHECK((v.flags & VF_DAMAGE_MOT) && (v.stateFlags & VS_DAMMOT_KIND)); CHECK(countS(ev, RE_DAMAGE_LAYER) == 1);
        const Event* d = findS(ev, RE_DAMAGE_LAYER); CHECK(d && d->b == 1 && d->c == 132);   // a2 = 1, a3 = animacion de la clase de la parte 5
        ev.clear(); RestLogic::changeDamageMot(v, ev, 5, 0, in); CHECK(ev.empty());
        // frontera: parte 10 es baja, parte 11 es alta
        v = VampireState(); ev.clear(); RestLogic::changeDamageMot(v, ev, 10, 0, in); CHECK(countS(ev, RE_DAMAGE_LAYER) == 1);
        v = VampireState(); ev.clear(); RestLogic::changeDamageMot(v, ev, 11, 0, in); CHECK(count(ev, EV_MOTION) == 1);
    }

    // ---------------- checkDamage_Eknd ----------------
    {
        // golpe sin matar: dos armaduras marcadas (partes 2 y 4); rand%2 decide cual se rompe
        for (int rv = 0; rv < 2; rv++) {
            EnemyEnv e = baseEnv(); VampireState v; v.type = 0; v.life = 500; v.flags = VF_HITCHECK;
            v.parts[2].flag154 = 0x100; v.parts[4].flag154 = 0x100;
            HitInput h; h.shooter[0] = 0; h.partOfPlayer[0] = 3;
            EkndIn in; in.armorCount = 6; in.body[3][0] = 1; in.body[3][1] = 2; in.body[3][2] = 3; in.body154[3] = 4; in.body1CA0 = 0.5f;
            EkndOut out; EventList ev; setRand({rv});
            RestLogic::checkDamageEknd(v, e, h, in, out, ev);
            CHECK(v.life == 455);                                       // 45 de dano base, arma sin multiplicador
            if (rv == 0) CHECK(!(v.parts[4].flag154 & 0x100) && (v.parts[2].flag154 & 0x100));   // r=0 < 1: se limpia la ultima (parte 4)
            else CHECK((v.parts[4].flag154 & 0x100) && !(v.parts[2].flag154 & 0x100));            // r=1: la ultima se salta; la ultima restante se limpia
            CHECK(g_i == (rv == 0 ? 2 : 1));                             // rand consumido: 1 llamada en el primer paso; si no se rompe, s3==s5 en el segundo
            CHECK(out.spark && out.sparkPos[0] == 1 && out.sparkPos[1] == 2 && out.sparkPos[2] == 3 + 4 * 0.5f);
            CHECK(countS(ev, RE_SPARK) == 1 && find(ev, EV_HITMARK) && find(ev, EV_HITMARK)->c == 0x3E8);
            CHECK(count(ev, EV_SCORE) == 1 && find(ev, EV_SCORE)->b == 10 && count(ev, EV_HITNUM) == 1);
            CHECK(find(ev, EV_SOUND) && find(ev, EV_SOUND)->a == 0x20025);
            CHECK(!(v.flags & VF_DEAD) && e.lastHitPlayer == 0);
        }
        // sin armaduras marcadas: no consume rand
        {
            EnemyEnv e = baseEnv(); VampireState v; v.life = 500; v.flags = VF_HITCHECK;
            HitInput h; h.shooter[0] = 1; h.partOfPlayer[1] = 2; EkndIn in; in.armorCount = 6; EkndOut out; EventList ev; setRand({1});
            RestLogic::checkDamageEknd(v, e, h, in, out, ev); CHECK(g_i == 0 && v.life == 455 && e.lastHitPlayer == 1);
        }
        // muerte: vida 40 - 45 <= 0
        {
            EnemyEnv e = baseEnv(); VampireState v; v.type = 2; v.life = 40; v.flags = VF_HITCHECK;
            v.parts[2].flag154 = 0x100; v.parts[3].flag154 = 0x100;
            HitInput h; h.shooter[0] = 0; h.partOfPlayer[0] = 3;
            EkndIn in; in.armorCount = 6; EkndOut out; EventList ev; setRand({});
            RestLogic::checkDamageEknd(v, e, h, in, out, ev);
            CHECK(v.life == -5 && (v.flags & VF_DEAD) && (v.stateFlags & VS_HIT) && !(v.stateFlags & VS_DEADHIT));
            CHECK(!(v.parts[2].flag154 & 0x100) && !(v.parts[3].flag154 & 0x100) && g_i == 0);
            const Event* c = find(ev, EV_PARTS_CHAIN); CHECK(c && c->a == 1 && c->b == 0 && c->c == 5 && c->d == 10);   // tipo 2: (5, 10)
            CHECK(count(ev, EV_SCORE) == 2 && find(ev, EV_SCORE, 0)->b == 100 && find(ev, EV_SCORE, 1)->b == 10);
            CHECK(find(ev, EV_HITMARK) && find(ev, EV_HITMARK)->c == 0x3E9 && find(ev, EV_SOUND)->a == 0x180011 && !out.spark);
            // si la parte 1 ya esta desprendida no se encadena
            VampireState v2; v2.life = 1; v2.flags = VF_HITCHECK; v2.parts[1].flag150 = 0x01000000; ev.clear();
            RestLogic::checkDamageEknd(v2, e, h, in, out, ev); CHECK(count(ev, EV_PARTS_CHAIN) == 0 && (v2.flags & VF_DEAD));
        }
        // invulnerable: sin dano, pero +10 y golpe
        {
            EnemyEnv e = baseEnv(); VampireState v; v.life = 500; v.flags = VF_HITCHECK | VF_INVULN;
            HitInput h; h.shooter[0] = 0; h.partOfPlayer[0] = 3; EkndIn in; in.armorCount = 6; EkndOut out; EventList ev;
            RestLogic::checkDamageEknd(v, e, h, in, out, ev);
            CHECK(v.life == 500 && count(ev, EV_SCORE) == 1 && find(ev, EV_SCORE)->b == 10 && count(ev, EV_HITNUM) == 1 && count(ev, EV_SOUND) == 0);
        }
        // ya muerto: solo cuenta el golpe
        {
            EnemyEnv e = baseEnv(); VampireState v; v.life = 0; v.flags = VF_HITCHECK | VF_DEAD;
            HitInput h; h.shooter[0] = 0; h.partOfPlayer[0] = 3; EkndIn in; EkndOut out; EventList ev;
            RestLogic::checkDamageEknd(v, e, h, in, out, ev);
            CHECK(count(ev, EV_HITNUM) == 1 && count(ev, EV_SCORE) == 0 && ev.size() == 1);
        }
        // parte <= 0 o sin flag de colision: nada, pero lastHitPlayer se fija en el primer caso
        {
            EnemyEnv e = baseEnv(); VampireState v; v.life = 500; v.flags = VF_HITCHECK;
            HitInput h; h.shooter[1] = 1; h.partOfPlayer[1] = 0; EkndIn in; EkndOut out; EventList ev;
            RestLogic::checkDamageEknd(v, e, h, in, out, ev); CHECK(ev.empty() && e.lastHitPlayer == 1 && v.life == 500);
            v.flags = 0; h.partOfPlayer[1] = 3; RestLogic::checkDamageEknd(v, e, h, in, out, ev); CHECK(ev.empty() && v.life == 500);
        }
        // trazadora (arcade, entrenamiento, arma 0xE)
        {
            EnemyEnv e = baseEnv(); e.gameState = 0xA; e.gameKind = 1; e.pl[0].weapon = 0xE;
            VampireState v; v.life = 5000; v.flags = VF_HITCHECK;
            HitInput h; h.shooter[0] = 0; h.partOfPlayer[0] = 2; EkndIn in; in.armorCount = 6; in.partLocal[2][0] = 7; in.partLocal[2][2] = 9;
            EkndOut out; EventList ev;
            RestLogic::checkDamageEknd(v, e, h, in, out, ev);
            CHECK(out.tracer && out.tracerLocal[0] == 7 && out.tracerLocal[2] == 10.0f && out.tracerScale == 0.8f && count(ev, EV_EFFECT_TRACER) == 1);
        }
    }

    // ---------------- Wait ----------------
    {
        EnemyEnv e = baseEnv(); VampireState v; v.type = 0; v.subType = 0; EventList ev;
        static int16_t motLen[0x2000]; for (auto& m : motLen) m = 30;
        // paso 0: elige la animacion (rand=1 -> pares[0][1]=15), temporizador rand%50, paso 1, y la reproduce (curMot != motId)
        setRand({1, 77, 0, 45});
        v.curMot = 99;
        RestLogic::wait(v, e, ev, motLen);
        CHECK(v.motId == 15 && v.aiTimer == 27.0f && v.step == 1);
        // paso 1 en el mismo fotograma: curMot(99) != 15 -> nueva eleccion (rand 0 -> 15) y arranque con frame rand%30 = 45%30 = 15
        const Event* m = find(ev, EV_MOTION); CHECK(m && m->a == 15 && m->b == 10 && m->d == 15);
        CHECK(g_i == 4);
        // tipo 8, subtipo 1 (base 6 -> indice 7): pares[7] = 777; temporizador > 100 y hueco libre -> aiMode 2
        v = VampireState(); v.type = 8; v.subType = 1; v.step = 1; v.aiTimer = 101.0f; v.curMot = 777; v.motId = 777; ev.clear();
        e.attackSlot[0] = 1; e.attackSlot[1] = 1; RestLogic::wait(v, e, ev, motLen); CHECK(v.aiMode == 0 && v.step == 1);    // sin hueco
        e.attackSlot[1] = 0; RestLogic::wait(v, e, ev, motLen); CHECK(v.aiMode == 2 && v.step == 0 && v.aiTimer == 0);
        // el tipo distinto de 8 no pasa a atacar; timer == 100 tampoco
        v = VampireState(); v.type = 1; v.step = 1; v.aiTimer = 500; v.curMot = 31; v.motId = 31; e.attackSlot[0] = 0; RestLogic::wait(v, e, ev, motLen); CHECK(v.aiMode == 0);
        v = VampireState(); v.type = 8; v.step = 1; v.aiTimer = 100.0f; v.curMot = 1; v.motId = 1; RestLogic::wait(v, e, ev, motLen); CHECK(v.aiMode == 0);
        // paso != 0/1: no hace nada; con la capa de dano no cambia animacion
        v = VampireState(); v.step = 5; v.aiTimer = 500; v.type = 8; ev.clear(); RestLogic::wait(v, e, ev, motLen); CHECK(v.aiMode == 0 && ev.empty());
        v = VampireState(); v.step = 1; v.flags = VF_DAMAGE_MOT; v.curMot = 1; v.motId = 2; setRand({}); RestLogic::wait(v, e, ev, motLen); CHECK(ev.empty() && g_i == 0);
    }

    // ---------------- turnClamp / turnTarget ----------------
    {
        CHECK(RestLogic::turnClamp(100, 5, 10) == 105);                     // dentro del limite: gira todo
        CHECK(RestLogic::turnClamp(100, 50, 10) == 110);                    // limitado
        CHECK(RestLogic::turnClamp(100, -50, 10) == 90);
        CHECK(RestLogic::turnClamp(100, -10, 10) == 90);
        CHECK(RestLogic::turnClamp(0, (int16_t)-20000, 100) == -100);
        CHECK(RestLogic::turnSideOffsetX(1) == 3.5f && RestLogic::turnSideOffsetX(0) == -3.5f && RestLogic::turnSideOffsetX(-1) == 0.0f);
        VampireState v; VampExt x; TurnIn in; in.camPos[0] = 1; in.camPos[1] = 2; in.camPos[2] = 3; in.sideTarget[0] = 9; in.sideTarget[1] = 8; in.sideTarget[2] = 7;
        in.angleToTarget = 6; x.turnStep = 10; x.rotY = 1000;
        v.motLoop = -1; v.atkPlayer = -1; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 1 && x.target[1] == 2 && x.target[2] == 3 && x.rotY == 1006);
        v.atkPlayer = 0; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 9 && x.target[2] == 7);
        in.coop = 1; in.gp7374 = 1; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 1);              // cooperativo con gp-0x7374 == 1: camara
        in.gp7374 = 2; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 9);
        v.posX = 10; v.posZ = 20; in.mirrorSrc[0] = 4; in.mirrorSrc[2] = 5;
        v.motLoop = -2; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 16 && x.target[1] == 2 && x.target[2] == 35);
        v.motLoop = -3; x.target[0] = 0; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 16);
        x.mark1DC4[0] = 1; x.mark1DC4[1] = 6; x.mark1DC4[2] = 2; v.motLoop = -4; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 19 && x.target[1] == 6 && x.target[2] == 38);
        in.explicitTarget[0] = 5; in.explicitTarget[1] = 5; in.explicitTarget[2] = 5; v.motLoop = 0x1234; RestLogic::turnTarget(v, x, in); CHECK(x.target[0] == 5);
        // los saltos (0x313/0x314) no giran
        x.rotY = 0; v.curMot = 0x313; RestLogic::turnTarget(v, x, in); CHECK(x.rotY == 0);
        v.curMot = 0x314; RestLogic::turnTarget(v, x, in); CHECK(x.rotY == 0);
        v.curMot = 0x312; RestLogic::turnTarget(v, x, in); CHECK(x.rotY == 6);
    }

    // ---------------- FirstCtrl (EnVampire) ----------------
    {
        EnemyEnv e = baseEnv(); VampireState v; VampExt x; TurnIn t; EventList ev;
        x.spawnPathId = 3; x.spawnFrame = 100;
        e.cam.pathId = 2; e.cam.frame = 500;
        CHECK(!RestLogic::firstCtrl(v, x, e, t, ev) && x.closeCountdown == 0);           // otro camino: nada
        e.cam.pathId = 3; e.cam.frame = 99;
        CHECK(!RestLogic::firstCtrl(v, x, e, t, ev) && x.closeCountdown == 0);           // todavia no
        e.cam.frame = 100;
        CHECK(!RestLogic::firstCtrl(v, x, e, t, ev) && x.closeCountdown == 1 && ev.empty());   // marca para cerrar
        v.atkPlayer = 1; e.attackSlot[1] = 1; e.attackSlot[0] = 1;
        CHECK(RestLogic::firstCtrl(v, x, e, t, ev) && x.closeCountdown == 0);            // cierra en el siguiente
        CHECK(e.attackSlot[1] == 0 && e.attackSlot[0] == 1 && countS(ev, RE_PRE_CLOSE) == 1 && countS(ev, RE_TASK_CLOSE) == 1);
        // sin jugador asignado: no toca los huecos
        ev.clear(); x.closeCountdown = 1; v.atkPlayer = -1; e.attackSlot[0] = 1; CHECK(RestLogic::firstCtrl(v, x, e, t, ev) && e.attackSlot[0] == 1);
        // movimiento en el aire
        v = VampireState(); x = VampExt(); e.cam.pathId = 0; x.spawnPathId = 9;
        v.stateFlags = VS_AIRBORNE; v.posX = 1; v.posY = 2; v.posZ = 3; v.velX = 0.5f; v.velY = 1; v.velZ = -1; v.gravity = -0.25f; x.accelX = 0.1f; x.accelZ = 0.2f;
        RestLogic::firstCtrl(v, x, e, t, ev);
        CHECK(v.posX == 1.5f && v.posY == 3 && v.posZ == 2 && v.velX == 0.5f + 0.1f && v.velY == 0.75f && v.velZ == -1 + 0.2f);
        // giro cuando +0x1DAC != 0
        v.stateFlags = 0; v.motLoop = -1; v.atkPlayer = -1; x.turnStep = 3; t.angleToTarget = 100; x.rotY = 0; RestLogic::firstCtrl(v, x, e, t, ev); CHECK(x.rotY == 3);
        v.motLoop = 0; RestLogic::firstCtrl(v, x, e, t, ev); CHECK(x.rotY == 3);
    }

    // ---------------- LastCtrl (EnVampire) ----------------
    {
        EnemyEnv e = baseEnv(); VampireState v; VampExt x; CtrlIn in; CtrlOut out; EventList ev;
        static int16_t motLen[0x400]; for (auto& m : motLen) m = 0; motLen[0x312] = 60;
        uint8_t script[4] = {20, 0, 0, 0}; in.script = script; in.scriptSize = 4; in.motLen = motLen;
        v.curMot = 0x312; v.curFrame = 59.5f; v.speedScale = 1.0f; v.motActive = 2; v.paramKind = 20; x.vec34C[0] = 2;
        RestLogic::lastCtrl(v, x, e, in, out, ev);
        CHECK(v.motActive == 1);                                          // 60.5 > 60 -> una repeticion menos
        CHECK(v.frame3C == 1.0f && v.layerEndFrame == 1.0f);              // y avanza los fotogramas (motActive != 0, misma animacion)
        CHECK(out.camEntry && out.camEntryScale == 100.0f / 4.0f && countS(ev, RE_CAM_ENTRY) == 1 && findS(ev, RE_CAM_ENTRY)->b == 0);
        CHECK(countS(ev, RE_WAITPOS_BUFF) == 0);
        // curFrame + speed == motLen: no resta (c.le.s)
        v.curFrame = 59.0f; v.motActive = 2; RestLogic::lastCtrl(v, x, e, in, out, ev); CHECK(v.motActive == 2);
        // bit 0x10000000: escala 1000; FINISHED: no hay entrada en camara; arcade: buffer de espera
        v.flags = VF_BIT10000000; e.gameState = 0xA; out = CtrlOut(); ev.clear(); RestLogic::lastCtrl(v, x, e, in, out, ev);
        CHECK(out.camEntryScale == 250.0f && findS(ev, RE_CAM_ENTRY)->b == 1 && countS(ev, RE_WAITPOS_BUFF) == 1);
        v.flags = VF_FINISHED; out = CtrlOut(); ev.clear(); RestLogic::lastCtrl(v, x, e, in, out, ev); CHECK(!out.camEntry && countS(ev, RE_CAM_ENTRY) == 0);
        // getParam si paramKind == -1: orden 20 deja paramKind = 20
        v = VampireState(); v.curMot = 1; v.paramKind = -1; v.motActive = 0; v.flags = VF_FINISHED; ev.clear(); RestLogic::lastCtrl(v, x, e, in, out, ev);
        CHECK(v.paramKind == 20 && v.frame3C == 0);
        // el cierre pendiente marca flags2 |= 0x20
        x.closeCountdown = 1; v.flags2 = 0; RestLogic::lastCtrl(v, x, e, in, out, ev); CHECK(v.flags2 == 0x20);
        x.closeCountdown = 0; v.flags2 = 0; RestLogic::lastCtrl(v, x, e, in, out, ev); CHECK(v.flags2 == 0);
        // si getParam cambia la animacion no avanza: se simula con motActive == 0
        v.motActive = 0; v.speedScale = 2; v.frame3C = 5; RestLogic::lastCtrl(v, x, e, in, out, ev); CHECK(v.frame3C == 5);
    }

    // ---------------- KneadEffect ----------------
    {
        VampireState v; KneadIn in; KneadOut out; in.height = 10;
        v.stateFlags = 0; v.posY = 10; v.curMot = 0x310; CHECK(!RestLogic::kneadEffect(v, in, out));             // sin bit 0x10
        v.stateFlags = VS_DEADSND; v.velY = -1; CHECK(!RestLogic::kneadEffect(v, in, out));                      // cayendo
        v.velY = 0;
        v.curMot = 0x313; CHECK(!RestLogic::kneadEffect(v, in, out)); v.curMot = 0x314; CHECK(!RestLogic::kneadEffect(v, in, out));
        v.curMot = 0x310; v.posY = 10; v.frame3C = 12;
        CHECK(RestLogic::kneadEffect(v, in, out) && out.draw && out.scale == 0.7f * 1.0f && out.effectId == 0x2600B8 + 2);     // pegado al suelo: s = 1
        v.posY = 13.5f; CHECK(RestLogic::kneadEffect(v, in, out) && std::fabs(out.scale - 0.7f * 0.5f) < 1e-6f);               // 1 - 3.5/7
        v.posY = 17; CHECK(!RestLogic::kneadEffect(v, in, out));                                                               // s = 0
        v.posY = 20; CHECK(!RestLogic::kneadEffect(v, in, out));
        // resto de animaciones: minimo de (viewY - radio) de las partes 1..17
        v.curMot = 0; for (int i = 1; i < kVampParts; i++) { in.viewY[i] = 30; in.radius[i] = 1; }
        in.viewY[5] = 12; in.radius[5] = 0.5f; in.height = 10;
        CHECK(RestLogic::kneadEffect(v, in, out) && std::fabs(out.scale - 0.7f * (1.0f - 1.5f / 7.0f)) < 1e-5f);
        in.viewY[1] = 0; in.radius[1] = 0;                                                                                     // la parte 1 es la mas baja: s > 1 -> se acota a 1
        v.frame3C = 4.9f; CHECK(RestLogic::kneadEffect(v, in, out) && out.scale == 0.7f && out.effectId == 0x2600B8 + 4);
        in.viewY[1] = 30; in.viewY[5] = 40; in.viewY[2] = 17; in.radius[2] = 0; in.height = 10;                                // minimo = 17 -> s = 0 -> nada ... (parte 2 mas baja que el resto)
        for (int i = 1; i < kVampParts; i++) if (i != 2) in.viewY[i] = 99;
        CHECK(!RestLogic::kneadEffect(v, in, out));
    }

    // ---------------- PeopleTask FirstCtrl / LastCtrl ----------------
    {
        EnemyEnv e = baseEnv(); PeopleTaskState p; PeopleExt x; PeopleFirstIn in; EventList ev;
        x.spawnPathId = 4; x.spawnFrame = 50; e.cam.pathId = 1; e.cam.frame = 10;
        p.posX = 3; p.posY = 4; p.posZ = 0; in.camPos[0] = 0; in.camPos[1] = 0; in.camPos[2] = 0; p.p1DD0 = 0;
        CHECK(!RestLogic::peopleFirstCtrl(p, x, e, in, ev) && ev.empty());
        CHECK(x.dist1DF8 == 3.0f && x.dist1DFC == 5.0f);                            // camara como objetivo (p1DD0 != -1)
        p.p1DD0 = -1; x.target[0] = 3; x.target[1] = 0; x.target[2] = 4; p.posX = 0; p.posY = 0; p.posZ = 0;
        RestLogic::peopleFirstCtrl(p, x, e, in, ev); CHECK(x.dist1DF8 == 5.0f && x.dist1DFC == 5.0f && countS(ev, RE_TURN_TARGET) == 1);   // -1 tambien es != 0: pide giro
    }
    {
        EnemyEnv e = baseEnv(); PeopleTaskState p; PeopleExt x; PeopleFirstIn in; EventList ev;
        x.spawnPathId = 4; x.spawnFrame = 50;
        // pasa por el camino/fotograma -> cierra al instante
        e.cam.pathId = 4; e.cam.frame = 50; p.atkPlayer = 0; e.attackSlot[0] = 1;
        CHECK(RestLogic::peopleFirstCtrl(p, x, e, in, ev) && e.attackSlot[0] == 0);
        CHECK(countS(ev, RE_CHILD_ALL_CLOSE) == 1 && countS(ev, RE_FUNC_9357A0) == 1 && countS(ev, RE_TASK_CLOSE) == 1);
        // orden de los eventos
        CHECK(ev[0].a == RE_CHILD_ALL_CLOSE && ev[1].a == RE_FUNC_9357A0 && ev[2].a == RE_TASK_CLOSE);
        // un fotograma antes: no
        ev.clear(); e.cam.frame = 49; CHECK(!RestLogic::peopleFirstCtrl(p, x, e, in, ev) && ev.empty());
        // cuenta atras +0x1DC8
        p.p1DC8 = 2; CHECK(!RestLogic::peopleFirstCtrl(p, x, e, in, ev) && p.p1DC8 == 1);
        e.cam.pathId = 4; e.cam.frame = 90;                                           // con cuenta atras activa el camino no cuenta
        p.atkPlayer = -1; CHECK(RestLogic::peopleFirstCtrl(p, x, e, in, ev) && p.p1DC8 == 0);
        // fuera de pantalla solo con stateFlags & 0x100
        ev.clear(); e.cam.pathId = 0; in.offScreen = true; p.stateFlags = 0; CHECK(!RestLogic::peopleFirstCtrl(p, x, e, in, ev));
        p.stateFlags = 0x100; CHECK(RestLogic::peopleFirstCtrl(p, x, e, in, ev));
        // giro: evento si p1DD0 != 0
        ev.clear(); in.offScreen = false; p.p1DD0 = 7; RestLogic::peopleFirstCtrl(p, x, e, in, ev); CHECK(countS(ev, RE_TURN_TARGET) == 1);
        ev.clear(); p.p1DD0 = 0; RestLogic::peopleFirstCtrl(p, x, e, in, ev); CHECK(countS(ev, RE_TURN_TARGET) == 0);
    }
    {
        EnemyEnv e = baseEnv(); PeopleTaskState p; PeopleExt x; PeopleCounters c; VmEnv vm; PeopleLastIn in; CtrlOut out; EventList ev;
        static int16_t motLen[0x400]; for (auto& m : motLen) m = 100;
        uint8_t script[4] = {1, 0, 5, 0};                                              // orden 1: espera frame3C >= 5
        in.motLen = motLen; in.script = script; in.scriptSize = 4;
        p.curMot = 7; p.waitKind = -1; p.loop = 1; p.speed = 0.5f; p.curFrame = 1; p.frame3C = 0;
        in.gp7210 = 0; c.stage6Alive = 0; x.vec34C[0] = 5; p.flags = 0;
        RestLogic::peopleLastCtrl(p, x, e, c, vm, in, out, ev);
        CHECK(p.waitKind == 1 && p.p1DC0 == 5);                                         // getParam leyo la orden 1
        CHECK(p.frame3C == 0.5f && x.layerEndFrame == 0.5f);                            // avance de fotogramas (loop != 0)
        CHECK(!out.camEntry);                                                           // gp7210 < stage6Alive es falso (0 < 0)
        // espera 1: sigue esperando mientras frame3C < 5; luego libera y lee la siguiente orden (script agotado)
        for (int i = 0; i < 20; i++) RestLogic::peopleLastCtrl(p, x, e, c, vm, in, out, ev);
        CHECK(p.frame3C >= 5.0f && p.waitKind != 1);
        // entrada en camara: stage6Alive > gp7210
        c.stage6Alive = 3; in.gp7210 = 2; p.flags = PF_BIT10000000; x.vec34C[0] = 10; ev.clear(); out = CtrlOut();
        RestLogic::peopleLastCtrl(p, x, e, c, vm, in, out, ev);
        CHECK(out.camEntry && out.camEntryScale == 10.0f && findS(ev, RE_CAM_ENTRY)->b == 1);
        in.gp7210 = 3; out = CtrlOut(); ev.clear(); p.flags = 0; RestLogic::peopleLastCtrl(p, x, e, c, vm, in, out, ev); CHECK(!out.camEntry);
        p.flags = PF_FINISHED; in.gp7210 = 0; RestLogic::peopleLastCtrl(p, x, e, c, vm, in, out, ev); CHECK(!out.camEntry);
        // sin repeticiones (loop == 0) no avanza
        p.flags = 0; p.loop = 0; p.waitKind = 37; float f = p.frame3C; RestLogic::peopleLastCtrl(p, x, e, c, vm, in, out, ev); CHECK(p.frame3C == f);
    }

    // ---------------- ErsPplTask ----------------
    {
        EventList ev; bool closed = true;
        CHECK(RestLogic::ersFirstCtrl(0.0f, closed, ev) == 0.0f && closed && countS(ev, RE_TASK_CLOSE) == 1);
        ev.clear(); CHECK(RestLogic::ersFirstCtrl(0.25f, closed, ev) == 0.5f && !closed && ev.empty());
        CHECK(RestLogic::ersFirstCtrl(0.5f, closed, ev) == 1.0f);
        CHECK(RestLogic::ersFirstCtrl(0.75f, closed, ev) == 0.5f);
        CHECK(RestLogic::ersFirstCtrl(1.0f, closed, ev) == 0.0f && !closed);
        float f3c = 1, fe = 2; RestLogic::ersLastCtrl(0.5f, f3c, fe); CHECK(f3c == 1.5f && fe == 2.5f);
    }

    printf("%s: %d comprobaciones, %d fallos\n", fails ? "FALLO" : "OK", checks, fails);
    return fails ? 1 : 0;
}

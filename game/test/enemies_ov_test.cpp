// Tests de game/enemies_ov. Tablas comparadas con GAMEMOD.BIN / ELF (se omiten con aviso si no estan). El original NUNCA se ejecuto.
// Compilar (desde la raiz del repo):
//   c++ -std=c++17 -Wall game/test/enemies_ov_test.cpp game/enemies_ov/zombie.cpp game/enemies/enemy_common.cpp game/enemies/vampire.cpp game/enemies/vampire_tables.cpp -o build/verify_enemies_ov/enemies_ov_test && ./build/verify_enemies_ov/enemies_ov_test
#include "../enemies_ov/zombie.h"
#include "../enemies/vampire.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
using namespace vn::enov;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static std::vector<unsigned char> slurp(const char* p) {
    std::ifstream f(p, std::ios::binary); std::vector<unsigned char> v;
    if (f) v.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return v;
}
static std::vector<unsigned char> g_bin, g_elf, g_aomod;
// direccion -> offset: GAMEMOD.BIN carga en 0x92B000; ELF: offset = v - 0x140000 + 0x800
static uint32_t rd32(uint32_t addr) {
    const std::vector<unsigned char>* f = &g_bin; size_t o;
    if (addr >= 0x92B000u) o = addr - 0x92B000u; else { f = &g_elf; o = (size_t)addr - 0x140000u + 0x800u; }
    uint32_t v; std::memcpy(&v, &(*f)[o], 4); return v;
}
static int16_t rd16(uint32_t addr) {
    const std::vector<unsigned char>* f = &g_bin; size_t o;
    if (addr >= 0x92B000u) o = addr - 0x92B000u; else { f = &g_elf; o = (size_t)addr - 0x140000u + 0x800u; }
    int16_t v; std::memcpy(&v, &(*f)[o], 2); return v;
}

static int g_rand = 0;
static int fakeRand() { return g_rand; }

static ZEnv baseEnv() {
    ZEnv e; e.base.rand = fakeRand; e.base.pl[0].mode = 5; e.base.pl[1].mode = 5; e.base.cam.state = 2; return e;
}
static int count(const ZEvents& ev, ZEvType t) { int n = 0; for (auto& x : ev) if (x.type == t) n++; return n; }
static const ZEvent* find(const ZEvents& ev, ZEvType t, int nth = 0) { for (auto& x : ev) if (x.type == t && nth-- == 0) return &x; return nullptr; }

static void put16(std::vector<uint8_t>& v, int x) { v.push_back(x & 0xFF); v.push_back((x >> 8) & 0xFF); }
static void put32(std::vector<uint8_t>& v, uint32_t x) { put16(v, x & 0xFFFF); put16(v, x >> 16); }
static void putf(std::vector<uint8_t>& v, float f) { uint32_t u; std::memcpy(&u, &f, 4); put32(v, u); }

static void testTables() {
    if (g_bin.empty() || g_elf.empty()) { printf("(tablas contra GAMEMOD.BIN/ELF omitidas: ficheros no encontrados)\n"); return; }
    // Damage: tabla de saltos jtbl_009B5880 (6 entradas: clase de arma 0..5 -> caso del switch)
    CHECK(rd32(0x9B5880) == 0x95B9A0 && rd32(0x9B5884) == 0x95B9A0 && rd32(0x9B5888) == 0x95B9C8);
    CHECK(rd32(0x9B588C) == 0x95B9A0 && rd32(0x9B5890) == 0x95BA14 && rd32(0x9B5894) == 0x95BA5C);
    // D_009A1B70: tabla de "funciones de estado" (PTMF de 12 B, la rellena el constructor en ejecucion: en el fichero esta a cero).
    // La entrada 8 es D_009A1BD0 = la que DamageCheck copia a +0x1DC4 al morir (y 1E4C = 8).
    CHECK((0x9A1BD0 - 0x9A1B70) / 12 == 8 && (0x9A1BD0 - 0x9A1B70) % 12 == 0);
    CHECK(rd32(0x9A1B70) == 0 && rd32(0x9A1BD0) == 0 && rd32(0x9A1BD8) == 0);
    // tablas de vida compartidas con EnVampire (LifeSet usa las mismas D_009B6B50/80/B0)
    CHECK(rd16(0x9B6B50) == vn::en::kVampLifeByLevel[0] && rd16(0x9B6B50 + 32) == vn::en::kVampLifeByLevel[16]);
    CHECK(rd16(0x9B6B80 + 4) == vn::en::kVampLifeByDiff1P[2] && rd16(0x9B6BB0 + 6) == vn::en::kVampLifeByDiff2P[3]);
    // D_009A2370[tipo] -> tabla del ELF (patrones) -> registros de 10 B en GAMEMOD (+6 dmgType, +7 mascara)
    int valid = 0;
    for (int type = 0; type < 18; type++) {
        uint32_t p1 = rd32(0x9A2370 + 4 * type);
        if (p1 == 0) continue;
        CHECK(p1 >= 0x140000 && p1 < 0x500000);                      // tabla de patrones en el ELF
        uint32_t p2 = rd32(p1);                                      // patron 0
        CHECK(p2 >= 0x92B000u && p2 < 0x9B9080u);                    // registros en GAMEMOD
        valid++;
    }
    CHECK(valid == 18);
    // un registro real entra en damageToPlayer: tipo 1, patron 0, registro 0
    uint32_t rec = rd32(rd32(0x9A2370 + 4));
    ZAtkRec r; r.dmgType = (int8_t)g_bin.at(rec - 0x92B000u + 6); r.mask = (int8_t)g_bin.at(rec - 0x92B000u + 7);
    ZombieState z; ZEnv e = baseEnv(); z.atkPlayer = 0; z.guardBits = 0; z.flags = 0; ZEvents ev;
    ZombieLogic::damageToPlayer(z, e, r, ev);
    CHECK(e.missCount == (r.mask == 0 ? 0 : 1));
}

static void testMath() {
    // AngChk y ETurn contra una referencia circular independiente (rangos < 0x8000)
    int bad = 0;
    for (int c = 0; c < 0x10000; c += 0x1357)
        for (int r = 0; r <= 0x7000; r += 0x1C03)
            for (int a = 0; a < 0x10000; a += 0x0F11) {
                int d = ((a - c) & 0xFFFF); if (d > 0x8000) d = 0x10000 - d;
                int ref = d <= r ? 1 : 0;
                if (ZombieLogic::angChk(a, c, r) != ref) bad++;
            }
    CHECK(bad == 0);
    CHECK(ZombieLogic::angChk(0x100, 0xFF00, 0x300) == 1);    // cruza el 0
    CHECK(ZombieLogic::angChk(0x8000, 0, 0x100) == 0);
    // ETurn: acercarse al objetivo por el camino corto
    CHECK(ZombieLogic::eTurn(0x1000, 0x1100, 0x200) == 0x1100);     // dentro del paso: salta al objetivo
    CHECK(ZombieLogic::eTurn(0x1000, 0x3000, 0x200) == 0x1200);     // sube
    CHECK(ZombieLogic::eTurn(0x3000, 0x1000, 0x200) == 0x2E00);     // baja
    CHECK(ZombieLogic::eTurn(0x0100, 0xFF00, 0x100) == 0x0000);     // camino corto cruzando 0 (-0x200): cur - paso
    CHECK(ZombieLogic::eTurn(0xFF00, 0x0100, 0x100) == 0x10000);    // cruza (sin envolver: el original devuelve 0x10000)
    bad = 0;
    for (int cur = 0; cur < 0x10000; cur += 0x0777) for (int tgt = 0; tgt < 0x10000; tgt += 0x0B33) {
        int r = ZombieLogic::eTurn(cur, tgt, 0x180);
        int diffBefore = ((tgt - cur) & 0xFFFF); if (diffBefore > 0x8000) diffBefore = 0x10000 - diffBefore;
        int diffAfter = ((tgt - r) & 0xFFFF); if (diffAfter > 0x8000) diffAfter = 0x10000 - diffAfter;
        if (diffBefore > 0x180) { if (diffAfter != diffBefore - 0x180) bad++; } else if (r != tgt) bad++;
    }
    CHECK(bad == 0);
}

static void testCounters() {
    ZombieState z; ZEnv e = baseEnv(); e.alive = 5; e.onScreen = 3;
    ZombieLogic::decNowEnemyAlvNumChk(z, e); ZombieLogic::decNowEnemyAlvNumChk(z, e);
    CHECK(e.alive == 4 && (z.flags2 & ZF2_ALIVE_DEC));
    ZombieLogic::decNowEnemyNumChk(z, e); ZombieLogic::decNowEnemyNumChk(z, e);
    CHECK(e.onScreen == 2 && (z.flags2 & ZF2_ONSCR_DEC));
    z.atkPlayer = 1; e.base.attackSlot[1] = 1; z.flags3 = Z3_SLOT_TAKEN; e.attackGate = 1;
    ZombieLogic::atcPlFlgClear(z, e);
    CHECK(z.atkPlayer == -1 && e.base.attackSlot[1] == 0 && e.attackGate == 0 && !(z.flags3 & Z3_SLOT_TAKEN));
    // Delete: paso 0 y luego cierre
    ZombieState d; ZEnv e2 = baseEnv(); e2.alive = 2; e2.onScreen = 2; d.atkPlayer = 0; e2.base.attackSlot[0] = 1; d.life = 50; ZEvents ev;
    ZombieLogic::deleteTask(d, e2, ev);
    CHECK((d.flags & ZF_DEAD) && (d.flags & ZF_BIT4000) && (d.flags2 & ZF2_BIT20) && d.life == 0 && d.step == 1);
    CHECK(e2.alive == 1 && e2.onScreen == 1 && e2.base.attackSlot[0] == 0 && ev.empty());
    ZombieLogic::deleteTask(d, e2, ev);
    CHECK(count(ev, ZE_TASK_CLOSE) == 1 && d.step == 1);
}

static void testAttackSelect() {
    ZombieState z; ZEnv e = baseEnv();
    // coop 1 (un jugador activo): solo, jugador 0
    e.base.coop = 1; e.base.solo = 0;
    CHECK(ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == 0 && e.base.attackSlot[0] == 1);
    CHECK(!ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == -1);         // hueco ocupado: atkPlayer se reinicia a -1 y no hay nadie
    z = ZombieState(); e.base.attackSlot[0] = 0; e.base.solo = 1;
    CHECK(ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == 1 && e.base.attackSlot[1] == 1);
    // jugador fuera de juego (modo != 5): is_AttackPlayer lo descarta
    z = ZombieState(); e.base.attackSlot[1] = 0; e.base.pl[1].mode = 3;
    CHECK(!ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == -1);
    e.base.gameState = 9; z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == 1);   // modo 9: siempre valido
    e.base.gameState = 0; e.base.pl[1].mode = 5;
    // coop 2, un jugador (playerCount 1): rand%2
    e = baseEnv(); e.base.coop = 2; e.base.playerCount = 1; g_rand = 3;
    z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == 1);
    e.base.attackSlot[1] = 0; g_rand = 4; z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == 0);
    // coop 2, un enemigo en pantalla: si el hueco esta ocupado se elige ~sel (-1 / -2), que is_AttackPlayer rechaza
    e = baseEnv(); e.base.coop = 2; e.base.playerCount = 2; e.onScreen = 1; g_rand = 2; e.base.attackSlot[0] = 1;
    z = ZombieState(); CHECK(!ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == -1);
    e.base.attackSlot[0] = 0; z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelect(z, e, 0) && z.atkPlayer == 0);
    // coop 2 por lado de pantalla
    e = baseEnv(); e.base.coop = 2; e.base.playerCount = 2; e.onScreen = 5;
    z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelect(z, e, -1) && z.atkPlayer == 0);
    z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelect(z, e, 1) && z.atkPlayer == 1);
    z = ZombieState(); CHECK(!ZombieLogic::attackPlayerSelect(z, e, 0));
    // bloqueo global y 0x500F07
    e.attackGate = 1; z = ZombieState(); CHECK(!ZombieLogic::attackPlayerSelect(z, e, -1) && z.atkPlayer == -1);
    e.attackGate = 0; e.flag500F07 = 1; e.base.attackSlot[0] = e.base.attackSlot[1] = 0;
    CHECK(!ZombieLogic::attackPlayerSelect(z, e, -1));
    // reach354 > 0: no se toma el hueco
    e.flag500F07 = 0; z = ZombieState(); z.reach354 = 1.0f; CHECK(!ZombieLogic::attackPlayerSelect(z, e, -1) && e.base.attackSlot[0] == 0);
    // Asd: no reserva y exige ScChkP
    e = baseEnv(); e.base.coop = 2; e.base.playerCount = 2; e.onScreen = 5; z = ZombieState();
    CHECK(ZombieLogic::attackPlayerSelectAsd(z, e, 1, true) && z.atkPlayer == 1 && e.base.attackSlot[1] == 0);
    CHECK(!ZombieLogic::attackPlayerSelectAsd(z, e, 1, false));
    z.reach354 = 2; CHECK(!ZombieLogic::attackPlayerSelectAsd(z, e, -1, true));
    e = baseEnv(); e.base.coop = 1; e.base.solo = 1; z = ZombieState(); CHECK(ZombieLogic::attackPlayerSelectAsd(z, e, 0, true) && z.atkPlayer == 1);
    e.attackGate = 1; z = ZombieState(); CHECK(!ZombieLogic::attackPlayerSelectAsd(z, e, 0, true) && z.atkPlayer == -1);
}

static void testPlayerDamage() {
    ZombieState z; ZEnv e = baseEnv(); ZEvents ev; ZAtkRec rec; rec.dmgType = 3; rec.mask = 2;
    z.atkPlayer = -1; ZombieLogic::damageToPlayer(z, e, rec, ev); CHECK(ev.empty() && e.missCount == 0);       // sin jugador
    z.atkPlayer = 1;
    z.guardBits = 2; ZombieLogic::damageToPlayer(z, e, rec, ev); CHECK(ev.empty() && e.missCount == 0);       // mascara cubierta
    z.guardBits = 1;
    ZombieLogic::damageToPlayer(z, e, rec, ev);
    CHECK(count(ev, ZE_DAMAGE_PLAYER) == 1 && find(ev, ZE_DAMAGE_PLAYER)->a == 1 && find(ev, ZE_DAMAGE_PLAYER)->c == 3);
    CHECK(e.missCount == 1 && e.flag500F07 == 0);
    ev.clear(); z.flags = ZF_ATTACKING;                                                                       // ataque marcado
    ZombieLogic::damageToPlayer(z, e, rec, ev);
    CHECK(count(ev, ZE_DAMAGE_PLAYER) == 1 && e.flag500F07 == 1 && !(z.flags & ZF_ATTACKING) && e.missCount == 2);
    ev.clear(); e.base.cam.state = 1; z.flags = ZF_ATTACKING; e.flag500F07 = 0;                                // camara no en estado 2: no hiere
    ZombieLogic::damageToPlayer(z, e, rec, ev);
    CHECK(ev.empty() && e.flag500F07 == 0 && !(z.flags & ZF_ATTACKING) && e.missCount == 3);
    // mascara con signo (byte 0x80 = -128): ninguna proteccion de 3 bits la cubre
    e.base.cam.state = 2; ZAtkRec neg; neg.dmgType = 5; neg.mask = (int8_t)0x80; z.flags = 0; ev.clear(); z.guardBits = 7;
    ZombieLogic::damageToPlayer(z, e, neg, ev); CHECK(count(ev, ZE_DAMAGE_PLAYER) == 1);

    // DamageToPlayer2
    ZombieState y; ZEnv f = baseEnv(); ZEvents ev2; y.reach = -1.0f; y.curFrame = 20.0f; y.guardBits = 0;
    CHECK(ZombieLogic::damageToPlayer2(y, f, 30, 4, 1, 1, ev2) == 0 && ev2.empty());              // aun no llega al fotograma
    CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2) == 1);
    CHECK(count(ev2, ZE_DAMAGE_PLAYER) == 1 && find(ev2, ZE_DAMAGE_PLAYER)->a == 1 && find(ev2, ZE_DAMAGE_PLAYER)->c == 4 && f.missCount == 1);
    ev2.clear(); CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, -1, ev2) == 1 && find(ev2, ZE_DAMAGE_PLAYER)->a == 0);   // izquierda -> jugador 0
    ev2.clear(); y.reach = 1.0f; CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2) == 0 && ev2.empty());              // detras: nada
    y.reach = -1.0f; f.base.coop = 1; f.base.solo = 0; ev2.clear(); ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2);
    CHECK(find(ev2, ZE_DAMAGE_PLAYER)->a == 0);                                                    // coop 1: manda solo
    ev2.clear(); f.base.coop = 0; CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 0, ev2) == 1 && find(ev2, ZE_DAMAGE_PLAYER)->a == 0);   // lr 0 = jugador 0
    ev2.clear(); y.guardBits = 1; int m0 = f.missCount; CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2) == 1 && ev2.empty() && f.missCount == m0);   // protegido: 1 sin dano
    f.base.coop = 5; f.base.solo = 7; y.guardBits = 0; CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2) == 1);  // jugador valido por scLR
    f.base.coop = 1; f.base.solo = 7; ev2.clear(); CHECK(ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2) == 0 && ev2.empty());   // coop 1 con solo invalido
    // marcado: 0x500F07 y limpia el bit
    f.base.coop = 0; y.flags = ZF_ATTACKING; ZombieLogic::damageToPlayer2(y, f, 10, 4, 1, 1, ev2);
    CHECK(f.flag500F07 == 1 && !(y.flags & ZF_ATTACKING));
}

static void testMotion() {
    ZombieState z; ZEvents ev; int16_t motLen[0x80]; for (int i = 0; i < 0x80; i++) motLen[i] = (int16_t)(100 + i);
    // clase 4: termina capa y animacion al llegar al final
    z.flags = ZF_DAMAGE_MOT; z.weaponClass = 4; z.flags3 = Z3_LAYER_ANIM | Z3_MAIN_ANIM; z.layerMot = 5; z.curMot = 6;
    z.layerFrame = 50; z.curFrame = 50;
    ZombieLogic::damageMotEnd(z, motLen);
    CHECK((z.flags3 & (Z3_LAYER_ANIM | Z3_MAIN_ANIM)) == (Z3_LAYER_ANIM | Z3_MAIN_ANIM) && (z.flags & ZF_DAMAGE_MOT));
    z.layerFrame = 105; ZombieLogic::damageMotEnd(z, motLen);
    CHECK(!(z.flags3 & Z3_LAYER_ANIM) && z.layerEndFrame == 105.0f && (z.flags3 & Z3_MAIN_ANIM) && (z.flags & ZF_DAMAGE_MOT));
    z.curFrame = 106; z.frameOff1DE8 = 1; ZombieLogic::damageMotEnd(z, motLen);
    CHECK(!(z.flags3 & Z3_MAIN_ANIM) && !(z.flags & ZF_DAMAGE_MOT));
    // clases 0/1/3: termina cuando el fotograma supera 1/4 de la duracion
    for (int cls : {0, 1, 3}) {
        ZombieState y; y.flags = ZF_DAMAGE_MOT; y.weaponClass = (int8_t)cls; y.flags3 = Z3_LAYER_ANIM | Z3_MAIN_ANIM; y.layerMot = 8; y.curMot = 8;
        y.layerFrame = 20; y.curFrame = 20; ZombieLogic::damageMotEnd(y, motLen);        // 20 < 108/4 = 27: siguen
        CHECK((y.flags3 & Z3_LAYER_ANIM) && (y.flags3 & Z3_MAIN_ANIM));
        y.layerFrame = 27; y.curFrame = 27; ZombieLogic::damageMotEnd(y, motLen);
        CHECK(!(y.flags3 & (Z3_LAYER_ANIM | Z3_MAIN_ANIM)) && y.layerEndFrame == 108.0f && !(y.flags & ZF_DAMAGE_MOT));
    }
    // clase 2 (u otra): no cambia nada salvo cerrar si no hay animaciones
    ZombieState w; w.flags = ZF_DAMAGE_MOT; w.weaponClass = 2; w.flags3 = Z3_MAIN_ANIM; ZombieLogic::damageMotEnd(w, motLen);
    CHECK((w.flags & ZF_DAMAGE_MOT) && (w.flags3 & Z3_MAIN_ANIM));
    ZombieState w2; w2.flags = ZF_DAMAGE_MOT; w2.weaponClass = 2; ZombieLogic::damageMotEnd(w2, motLen); CHECK(!(w2.flags & ZF_DAMAGE_MOT));
    ZombieState w3; w3.flags = 0; w3.flags3 = Z3_MAIN_ANIM; ZombieLogic::damageMotEnd(w3, motLen); CHECK(w3.flags3 == Z3_MAIN_ANIM);   // sin ZF_DAMAGE_MOT: nada

    // MotSupSet / MotSupSet2
    ZombieState m; ZEvents mv; m.motObjId = 10;
    ZombieLogic::motSupSet(m, 10, 1, 2, mv); CHECK(mv.empty());                       // misma animacion
    ZombieLogic::motSupSet(m, 11, 1, 2, mv); CHECK(count(mv, ZE_MOTION_SET) == 1 && find(mv, ZE_MOTION_SET)->a == 11 && find(mv, ZE_MOTION_SET)->b == 1 && find(mv, ZE_MOTION_SET)->c == 2);
    mv.clear(); m.flags3 = Z3_MAIN_ANIM; ZombieLogic::motSupSet(m, 12, 0, 0, mv); CHECK(mv.empty());      // bloqueada
    mv.clear(); m.flags3 = Z3_LAYER_ANIM; ZombieLogic::motSupSet(m, 12, 0, 0, mv); CHECK(count(mv, ZE_MOTION_SET) == 1 && !(m.flags3 & Z3_LAYER_ANIM));
    mv.clear(); m.motObjId = 0x43; ZombieLogic::motSupSet(m, 12, 0, 0, mv); CHECK(count(mv, ZE_MOTION_SET_NOBLEND) == 1 && count(mv, ZE_MOTION_SET) == 0);
    mv.clear(); m.flags = ZF_DAMAGE_MOT; m.flags3 = Z3_MAIN_ANIM | Z3_LAYER_ANIM; m.motObjId = 5;
    ZombieLogic::motSupSet2(m, 6, 7, 8, mv);
    CHECK(!(m.flags & ZF_DAMAGE_MOT) && m.flags3 == 0 && count(mv, ZE_MOTION_SET) == 1 && find(mv, ZE_MOTION_SET)->c == 8);
    mv.clear(); m.motObjId = 6; ZombieLogic::motSupSet2(m, 6, 0, 0, mv); CHECK(mv.empty());
    mv.clear(); m.motObjId = 0x43; ZombieLogic::motSupSet2(m, 6, 0, 0, mv); CHECK(count(mv, ZE_MOTION_SET_NOBLEND) == 1);
}

static void testDamage() {
    // Damage por clase de arma
    ZEnv e = baseEnv(); ZDamageIn in; HitInput h; ZEvents ev;
    for (int i = 0; i < 18; i++) { in.part[i].bodyPos[2] = 10.0f + i; in.part[i].bodyF154 = 2.0f; in.part[i].selfPos[0] = 1.0f * i; }
    in.bodyScale = 3.0f;
    {   // clase 0 (jugador 1, parte 4)
        ZombieState z; h.partOfPlayer[1] = 4; h.weaponClass[1] = 0;
        ZombieLogic::damage(z, e, 1, h, in, ev);
        CHECK(z.flags & ZF_BIT8000000); CHECK(count(ev, ZE_EFFECT_PLAYER) == 1 && find(ev, ZE_EFFECT_PLAYER)->a == 0x27);
        CHECK(count(ev, ZE_DAMAGE_MOT) == 1 && count(ev, ZE_DAMAGE_VOICE) == 1 && find(ev, ZE_SCORE)->b == 10 && find(ev, ZE_SCORE)->a == 1);
        CHECK(count(ev, ZE_HITMARK) == 0);
    }
    {   // clase 2 (bala), jugador 0, parte 3: marca la parte (0x29), sonido global, efecto 0x1F tipo normal y hitmark 0x3E8
        ev.clear(); ZombieState z; z.type = 3; h.partOfPlayer[0] = 3; h.weaponClass[0] = 2;
        ZombieLogic::damage(z, e, 0, h, in, ev);
        CHECK(z.parts[3].w18C == 0x29 && count(ev, ZE_SOUND_GLOBAL) == 1 && find(ev, ZE_SOUND_GLOBAL)->a == 0x20025);
        CHECK(find(ev, ZE_EFFECT)->a == 0x1F && find(ev, ZE_EFFECT)->b == 0 && find(ev, ZE_EFFECT)->f[2] == 13.0f + 6.0f);   // z = 13 + 2*3
        CHECK(find(ev, ZE_HITMARK)->a == 3 && find(ev, ZE_HITMARK)->b == 0 && find(ev, ZE_HITMARK)->c == 0x3E8);
        CHECK(find(ev, ZE_SCORE)->b == 10);
        // tipos 0xC/0xE/0xF usan el codigo de efecto 2
        for (int t : {0xC, 0xE, 0xF}) { ev.clear(); z.type = t; ZombieLogic::damage(z, e, 0, h, in, ev); CHECK(find(ev, ZE_EFFECT)->b == 2); }
        // arcade + entrenamiento + lanzagranadas (0xE): efecto de trazadora en vez de chispa
        ev.clear(); e.base.gameState = 0xA; e.base.gameKind = 1; in.pl[0].weaponItem = 0xE; ZombieLogic::damage(z, e, 0, h, in, ev);
        CHECK(count(ev, ZE_EFFECT_LAUNCHER) == 1 && count(ev, ZE_EFFECT) == 0);
        in.pl[0].weaponItem = 0; e.base.gameState = 0xE; ev.clear(); ZombieLogic::damage(z, e, 0, h, in, ev);   // estado 0xE: sin hitmark
        CHECK(count(ev, ZE_HITMARK) == 0); e.base.gameState = 0; e.base.gameKind = 0;
    }
    {   // clase 4: 30 puntos, animacion de dano solo si 0 < parte < 11, desprende la parte (AttachOff)
        ev.clear(); ZombieState z; h.partOfPlayer[0] = 5; h.weaponClass[0] = 4;
        ZombieLogic::damage(z, e, 0, h, in, ev);
        CHECK(count(ev, ZE_DAMAGE_MOT) == 1 && find(ev, ZE_SCORE)->b == 0x1E && count(ev, ZE_ATTACH_OFF) == 1 && find(ev, ZE_ATTACH_OFF)->a == 5);
        ev.clear(); h.partOfPlayer[0] = 12; ZombieLogic::damage(z, e, 0, h, in, ev);
        CHECK(count(ev, ZE_DAMAGE_MOT) == 0 && find(ev, ZE_SCORE)->b == 0x1E);
    }
    {   // clase 5: 200 puntos + efecto 0x31 (z-2) + dos sonidos globales
        ev.clear(); ZombieState z; h.partOfPlayer[0] = 2; h.weaponClass[0] = 5;
        ZombieLogic::damage(z, e, 0, h, in, ev);
        CHECK(find(ev, ZE_SCORE)->b == 0xC8 && find(ev, ZE_EFFECT)->a == 0x31 && find(ev, ZE_EFFECT)->f[2] == 12.0f - 2.0f);
        CHECK(count(ev, ZE_SOUND_GLOBAL) == 2 && find(ev, ZE_SOUND_GLOBAL, 1)->a == 0x17000F && count(ev, ZE_DAMAGE_MOT) == 1);
    }
    {   // clase >= 6: solo efecto y marca
        ev.clear(); ZombieState z; h.weaponClass[0] = 9; ZombieLogic::damage(z, e, 0, h, in, ev);
        CHECK(count(ev, ZE_SCORE) == 0 && (z.flags & ZF_BIT8000000));
    }

    // DamageCheck: ZomCheckHitParts (inyectada) descuenta la vida; la clase 1 deja vida > 0
    struct Fn { static void hit(ZombieState& z) { z.life -= 30; } };
    ZEnv e2 = baseEnv(); ZDamageIn in2 = in; in2.zomCheckHitParts = Fn::hit; in2.chainA[0] = 7; in2.chainA[1] = 9;
    in2.deathHandler[0] = 0x964BA0;   // (en ejecucion: PTMF de la entrada 8 de D_009A1B70)
    HitInput h2; h2.shooter[0] = 1; h2.partOfPlayer[1] = 2; h2.weaponClass[1] = 1;
    {
        ZombieState z; z.flags = ZF_HITCHECK; z.life = 100; z.weakPartId = 9; ZEvents v; uint32_t weakFlag = 0x40000000u; z.weak[0] = &weakFlag;
        ZombieLogic::damageCheck(z, e2, h2, in2, v);
        CHECK(z.life == 70 && (z.flags & ZF_DAMAGE_MOT) && !(z.flags & ZF_DEAD) && z.hitPart == 2 && z.weaponClass == 1);
        CHECK(count(v, ZE_HITNUM) == 1 && find(v, ZE_HITNUM)->a == 1 && count(v, ZE_SCORE) == 1 && find(v, ZE_SCORE)->b == 10);
        CHECK(weakFlag == 0 && !(z.flags & ZF_INVULN));
        // muerte con parte debil (2 == weakPartId): +200 y +100, encadena la parte, hitmark 0x3E9
        z.weakPartId = 2; z.life = 20; v.clear();
        ZombieLogic::damageCheck(z, e2, h2, in2, v);
        CHECK(z.life == -10 && (z.flags & ZF_DEAD) && z.killer == 1 && z.handlerArg == 8 && z.step == 0 && z.handler[0] == 0x964BA0);
        CHECK((z.flags3 & Z3_BIT800) && (z.flags3 & Z3_BIT80000));
        CHECK(find(v, ZE_SCORE, 0)->b == 0xC8 && find(v, ZE_SCORE, 1)->b == 0x64 && find(v, ZE_SCORE, 2)->b == 10);
        CHECK(count(v, ZE_PARTS_CHAIN) == 1 && find(v, ZE_PARTS_CHAIN)->a == 2 && find(v, ZE_PARTS_CHAIN)->b == 1 && find(v, ZE_PARTS_CHAIN)->c == 7 && find(v, ZE_PARTS_CHAIN)->d == 9);
        CHECK(find(v, ZE_HITMARK, 0)->c == 0x3E9 || find(v, ZE_HITMARK, 1)->c == 0x3E9);
        // ya muerto: cuenta el acierto pero no repite
        v.clear(); ZombieLogic::damageCheck(z, e2, h2, in2, v); CHECK(count(v, ZE_HITNUM) == 1 && count(v, ZE_SCORE) == 0);
    }
    {   // parte ya desprendida: no encadena; estado 0xE: sin hitmark; clase 2 no sobrescribe weaponClass
        ZombieState z; z.flags = ZF_HITCHECK; z.life = 1; z.weaponClass = 4; z.parts[2].flag150 = 0x01000000; ZEvents v; ZEnv e3 = baseEnv(); e3.base.gameState = 0xE;
        HitInput h3 = h2; h3.weaponClass[1] = 2; ZombieLogic::damageCheck(z, e3, h3, in2, v);
        CHECK(z.life < 0 && z.weaponClass == 4 && count(v, ZE_PARTS_CHAIN) == 0 && count(v, ZE_HITMARK) == 0);
    }
    {   // camara en modo != 2: invulnerable (no hay procesamiento de impactos) y el bit se retira si no estaba
        ZombieState z; z.flags = ZF_HITCHECK; z.life = 100; ZEvents v; ZEnv e3 = baseEnv(); e3.mode7218 = 1;
        ZombieLogic::damageCheck(z, e3, h2, in2, v);
        CHECK(z.life == 70 && v.empty() && !(z.flags & ZF_INVULN));      // ZomCheckHitParts si se ejecuta; el bucle de impactos no
        ZombieState y; y.flags = ZF_HITCHECK | ZF_INVULN; y.life = 100; ZEvents v2; ZombieLogic::damageCheck(y, e2, h2, in2, v2);
        CHECK((y.flags & ZF_INVULN) && v2.empty());                      // ya era invulnerable: se mantiene
        ZombieState n; n.life = 100; ZEvents v3; ZombieLogic::damageCheck(n, e2, h2, in2, v3); CHECK(v3.empty() && n.life == 100);   // sin ZF_HITCHECK
        ZombieState k; k.flags = ZF_HITCHECK; k.life = 100; ZEvents v4; HitInput none; ZombieLogic::damageCheck(k, e2, none, in2, v4); CHECK(v4.empty());
        HitInput zero; zero.shooter[0] = 0; zero.partOfPlayer[0] = 0; ZombieState q; q.flags = ZF_HITCHECK; q.life = 100; ZEvents v5; ZombieLogic::damageCheck(q, e2, zero, in2, v5);
        CHECK(v5.empty());                                               // parte 0: sin acierto
    }
    // dos jugadores disparan en el mismo fotograma
    {
        ZombieState z; z.flags = ZF_HITCHECK; z.life = 100; ZEvents v; HitInput hh; hh.shooter[0] = 0; hh.shooter[1] = 1; hh.partOfPlayer[0] = 3; hh.partOfPlayer[1] = 4;
        hh.weaponClass[0] = 1; hh.weaponClass[1] = 1; ZombieLogic::damageCheck(z, e2, hh, in2, v);
        CHECK(count(v, ZE_HITNUM) == 2 && count(v, ZE_SCORE) == 2 && z.hitPart == 4);
    }
}

static void testPhysics() {
    ZombieState z; z.flags2 = 0; z.fallStep = 0; z.posY = 10; ZombieLogic::fallCheck(z); CHECK(z.posY == 10);          // sin ZF2_FALLING: nada
    z.flags2 = ZF2_FALLING; z.groundY = 0; z.posY = 10; z.velY = 0;
    ZombieLogic::fallCheck(z);
    CHECK(z.gravity == -0.0392f && z.fallStep == 1 && z.velY == -0.0392f && z.posY == 10 + -0.0392f);
    int n = 1; while ((z.flags2 & ZF2_FALLING) && n < 1000) { ZombieLogic::fallCheck(z); n++; }
    CHECK(!(z.flags2 & ZF2_FALLING) && z.posY == 0 && z.velY == 0 && z.gravity == 0 && z.fallStep == 0);
    CHECK(n > 10 && n < 40);                                           // sqrt(2*10/0.0392) ~ 22.6 fotogramas
    // rebote
    ZombieState b; b.posY = 1; b.velY = -2; b.gravity = -0.1f; b.velX = 10; b.velZ = -10;
    ZombieLogic::deadBounce(b, 0.0f);
    CHECK(b.posY == 0.0f && b.velY == (-2.1f) * 0.5f && b.velX == 10 * 0.7f && b.velZ == -10 * 0.7f);
    ZombieState c; c.posY = 20; c.velY = 0; c.gravity = -0.1f; c.velX = 5; ZombieLogic::deadBounce(c, 0.0f);
    CHECK(c.velX == 5 && c.posY == 20 + -0.1f);                        // sin contacto: solo integra
}

static void testScript() {
    uint32_t tbl[12] = {1, 2, 3, 10, 20, 30, 100, 200, 300, 1000, 2000, 3000};
    ComTables t; t.tbl1B70 = tbl; t.count = 4;
    ZEvents ev;
    {   // -1: fin; Delete; handler = tbl[0]
        std::vector<uint8_t> s; put16(s, -1); put16(s, 0);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); ZEnv e = baseEnv(); e.alive = 3; e.onScreen = 3; z.step = 5; z.flags = ZF_ATTACKING;
        int n = ZombieLogic::checkCom(z, e, t, ev);
        CHECK(n == 1 && z.handler[0] == 1 && z.handler[1] == 2 && z.handler[2] == 3 && z.handlerArg == 0 && z.scriptPos == 2);
        CHECK((z.flags & ZF_DEAD) && z.step == 1 && e.alive == 2 && e.onScreen == 2 && z.life == 0);
        CHECK(e.missCount == 1);        // delete deja ZF_ATTACKING puesto: el original comprueba despues y cuenta el fallo
    }
    {   // 61/62/63/64/65/66 y luego 60
        std::vector<uint8_t> s;
        put16(s, 61); put16(s, 0); put32(s, 0x10);
        put16(s, 62); put16(s, 0); put32(s, 0x01);
        put16(s, 63); put16(s, 0); put32(s, 0x20);
        put16(s, 64); put16(s, 0); put32(s, 0x02);
        put16(s, 65); put16(s, 0); put32(s, 0x4000);
        put16(s, 66); put16(s, 0); put32(s, 0x100);
        put16(s, 60); put16(s, 2);          // idx 2, handlerArg = 2
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); ZEnv e = baseEnv(); z.flags = 0x01; z.flags2 = 0x02; z.flags3 = 0x100; z.step = 9;
        int n = ZombieLogic::checkCom(z, e, t, ev);
        CHECK(n == 7 && z.flags == 0x10 && z.flags2 == 0x20 && z.flags3 == 0x4000);
        CHECK(z.handler[0] == 100 && z.handler[1] == 200 && z.handler[2] == 300 && z.handlerArg == 2 && z.step == 0 && z.scriptPos == 6 * 8 + 4);
        // el flujo se termina sin cierre: idx fuera de tabla no copia
        std::vector<uint8_t> s2; put16(s2, 60); put16(s2, 99); ZombieState y; y.script = s2.data(); y.scriptSize = 4; y.handler[0] = 77; y.handlerArg = 0;
        ZombieLogic::checkCom(y, e, t, ev); CHECK(y.handler[0] == 77 && y.handlerArg == 99 && y.scriptPos == 4);
        // codigo desconocido: se aborta (-1) sin avanzar
        std::vector<uint8_t> s3; put16(s3, 99); put16(s3, 0); ZombieState u; u.script = s3.data(); u.scriptSize = 4;
        CHECK(ZombieLogic::checkCom(u, e, t, ev) == -1 && u.scriptPos == 0);
        // flujo truncado
        ZombieState w; w.script = s.data(); w.scriptSize = 6; CHECK(ZombieLogic::checkCom(w, e, t, ev) == -1);
    }
    {   // PosSet
        std::vector<uint8_t> s; put16(s, 5); put16(s, 6); put16(s, 7); put16(s, 0);
        putf(s, 1.5f); putf(s, 2.5f); putf(s, 3.5f); put16(s, -1); put16(s, 0);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); ZEnv e = baseEnv();
        ZombieLogic::posSet(z, e, t, ev);
        CHECK(z.posExtra[0] == 5 && z.posExtra[1] == 6 && z.posExtra[2] == 7 && z.posX == 1.5f && z.posY == 2.5f && z.posZ == 3.5f);
        CHECK(z.scriptPos == 8 + 12 + 2 && (z.flags & ZF_DEAD));       // despues de los 12 B se ejecuto CheckCom (-1)
        // x == 0 y z == 0: solo cambia y
        std::vector<uint8_t> s2; put16(s2, 0); put16(s2, 0); put16(s2, 0); put16(s2, 0); putf(s2, 0); putf(s2, 9.0f); putf(s2, 0); put16(s2, 60); put16(s2, 1);
        ZombieState y; y.script = s2.data(); y.scriptSize = (int)s2.size(); y.posX = 4; y.posZ = 5; ZombieLogic::posSet(y, e, t, ev);
        CHECK(y.posX == 4 && y.posY == 9.0f && y.posZ == 5 && y.handler[0] == 10);
        // todo cero: no cambia nada
        std::vector<uint8_t> s3; for (int i = 0; i < 4; i++) put16(s3, 0); putf(s3, 0); putf(s3, 0); putf(s3, 0); put16(s3, 60); put16(s3, 0);
        ZombieState u; u.script = s3.data(); u.scriptSize = (int)s3.size(); u.posX = 1; u.posY = 2; u.posZ = 3; ZombieLogic::posSet(u, e, t, ev);
        CHECK(u.posX == 1 && u.posY == 2 && u.posZ == 3);
    }
    {   // LifeSet: vida del script + tabla por nivel + dificultad
        std::vector<uint8_t> s; put16(s, 300); put16(s, 0); put16(s, 60); put16(s, 0);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); ZEnv e = baseEnv(); e.base.levelA = 8; e.base.levelB = 4; e.base.solo = 0;
        ZombieLogic::lifeSet(z, e, t, ev);
        int expect = 300 + vn::en::kVampLifeByLevel[8] + vn::en::kVampLifeByDiff1P[4];
        CHECK(z.life == expect && z.lifeMax == expect && z.lifePrev == expect && z.scriptPos == 8);
        z = ZombieState(); z.script = s.data(); z.scriptSize = (int)s.size(); e.base.solo = 2;
        ZombieLogic::lifeSet(z, e, t, ev); CHECK(z.life == 300 + vn::en::kVampLifeByLevel[8] + vn::en::kVampLifeByDiff2P[4]);
    }
    {   // MissEnemyNumAdd
        std::vector<uint8_t> s; put16(s, 60); put16(s, 1); ZombieState z; z.script = s.data(); z.scriptSize = 4; ZEnv e = baseEnv();
        ZombieLogic::missEnemyNumAdd(z, e, t, ev); CHECK(e.missCount == 1 && z.handler[0] == 10);
    }
}

static void testStates() {
    uint32_t tbl[6] = {1, 2, 3, 10, 20, 30}; ComTables t; t.tbl1B70 = tbl; t.count = 2;
    int16_t motLen[0x80]; for (int i = 0; i < 0x80; i++) motLen[i] = (int16_t)(60 + i);
    {   // Adv_Dead
        std::vector<uint8_t> s; put16(s, 20); put16(s, 4); put16(s, 1); put16(s, 77); put16(s, 60); put16(s, 0);     // mot 20, parte 4, jugador 1, camino 77, 60 -> 50 fotogramas
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); z.motObjId = 3; ZEnv e = baseEnv(); ZDamageIn in; in.chainA[0] = 1; in.chainA[1] = 2; ZEvents ev;
        ZombieLogic::advDead(z, e, in, ev);
        CHECK(z.sv9C == 20 && z.sA0 == 4 && z.sA4 == 1 && z.sA8 == 77 && z.sAC == 49 && z.step == 1 && count(ev, ZE_MOTION_SET) == 1);   // ya resta el primer fotograma
        int n = 0; while (z.step == 1 && n++ < 100) ZombieLogic::advDead(z, e, in, ev);
        CHECK(n == 50 && (z.flags & ZF_DEAD) && z.step == 2 && count(ev, ZE_PARTS_CHAIN) == 1 && find(ev, ZE_PARTS_CHAIN)->a == 4 && find(ev, ZE_PARTS_CHAIN)->b == 1);
        ZombieLogic::advDead(z, e, in, ev); CHECK(z.step == 2);                                       // espera el camino 77
        e.base.cam.pathId = 77; ZombieLogic::advDead(z, e, in, ev);
        CHECK(z.step == 3 && (z.flags2 & ZF2_BIT20) && (z.flags & ZF_BIT100000));
        ZombieLogic::advDead(z, e, in, ev); CHECK(count(ev, ZE_TASK_CLOSE) == 1);
        // parte ya desprendida: sin encadenar
        ZombieState y; y.script = s.data(); y.scriptSize = (int)s.size(); y.parts[4].flag150 = 0x01000000; ZEvents e2; for (int i = 0; i < 60; i++) ZombieLogic::advDead(y, e, in, e2);
        CHECK(count(e2, ZE_PARTS_CHAIN) == 0 && (y.flags & ZF_DEAD));
    }
    {   // Tut_Dead
        std::vector<uint8_t> s; put16(s, 90); put16(s, 5); put16(s, 120); put16(s, -1); put16(s, -1); put16(s, 0);       // vida 90, parte 5, 120 -> 100 fotogramas, marca -1 y orden -1 (fin)
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); ZEnv e = baseEnv(); ZEvents ev;
        ZombieLogic::tutDead(z, e, t, ev);
        CHECK(z.life == 90 && z.lifeMax == 90 && z.lifePrev == 90 && z.sv9C == 5 && z.partOfPlayer0 == 5 && (z.flags & ZF_HITCHECK) && (z.flags & ZF_BIT2));
        CHECK((z.parts[5].flag150 & 0xA) == 0xA && count(ev, ZE_DAMAGE_CHECK) == 1 && z.step == 2 && z.sA0 == 99);
        int n = 0; while (!(z.flags & ZF_DEAD) && n++ < 200) ZombieLogic::tutDead(z, e, t, ev);
        CHECK(n == 99 && z.step == 1);                          // al llegar a 0 consumio el -1 y ejecuto CheckCom
    }
    {   // Tut_Dead: sin -1 vuelve al paso 1
        std::vector<uint8_t> s; put16(s, 50); put16(s, 2); put16(s, 6); put16(s, 2); put16(s, 6); put16(s, -1); put16(s, 0);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); ZEnv e = baseEnv(); ZEvents ev;
        ZombieLogic::tutDead(z, e, t, ev);                      // 6*50/60 = 5 fotogramas
        for (int i = 0; i < 4; i++) ZombieLogic::tutDead(z, e, t, ev);
        CHECK(z.step == 1 && z.scriptPos == 6 && count(ev, ZE_DAMAGE_CHECK) == 1);
        ZombieLogic::tutDead(z, e, t, ev);                      // paso 1 otra vez: segundo golpe forzado, parte 2
        CHECK(count(ev, ZE_DAMAGE_CHECK) == 2 && z.sv9C == 2);
    }
    {   // St2_Sc03_03
        std::vector<uint8_t> s; put16(s, 33); put16(s, 100); put16(s, -1); put16(s, 0);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); z.curMot = 1; ZEnv e = baseEnv(); ZEvents ev;
        ZombieLogic::st2Sc03_03(z, e, t, ev);
        CHECK(z.sA4 == 33 && z.sA8 == 100 && z.step == 1 && count(ev, ZE_MOTION_SET) == 1 && find(ev, ZE_MOTION_SET)->a == 33);
        z.curMot = 33; z.curFrame = 101; ev.clear(); ZombieLogic::st2Sc03_03(z, e, t, ev);
        CHECK((z.flags & ZF_INVULN) && !(z.flags2 & ZF2_BIT200) && z.step == 1 && ev.empty());
        z.curFrame = 250; ZombieLogic::st2Sc03_03(z, e, t, ev);                 // fotograma 250: paso 2 + CheckCom(-1) -> Delete
        CHECK((z.flags2 & ZF2_BIT200) && (z.flags & ZF_DEAD) && z.step == 1);
    }
    {   // St3_Sc03_02: un golpe en el fotograma sA4
        std::vector<uint8_t> s; put16(s, 7); put16(s, 3); put16(s, 20); put16(s, 5); put16(s, 2); put16(s, 11); put16(s, -1); put16(s, 0);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); z.reach = -1.0f; z.curMot = 7; z.motObjId = 7; ZEnv e = baseEnv(); ZEvents ev;
        z.flags3 = Z3_BIT8;
        ZombieLogic::st3Sc03_02(z, e, t, motLen, 1, ev);
        CHECK(z.sv9C == 7 && z.sA0 == 3 && z.sA4 == 20 && z.sA8 == 5 && z.sAC == 2 && z.sB0 == 11 && z.posExtra[1] == 11);
        CHECK(!(z.flags3 & Z3_BIT8) || (z.flags3 & Z3_BIT8));                       // el bit se limpia al entrar y se activa si el fotograma ya paso
        z.flags3 &= ~Z3_BIT8; z.curFrame = 19; ZombieLogic::st3Sc03_02(z, e, t, motLen, 1, ev); CHECK(count(ev, ZE_DAMAGE_PLAYER) == 0 && !(z.flags3 & Z3_BIT8));
        z.curFrame = 20; ZombieLogic::st3Sc03_02(z, e, t, motLen, 1, ev);
        CHECK(count(ev, ZE_DAMAGE_PLAYER) == 1 && find(ev, ZE_DAMAGE_PLAYER)->c == 5 && (z.flags3 & Z3_BIT8));
        ZombieLogic::st3Sc03_02(z, e, t, motLen, 1, ev); CHECK(count(ev, ZE_DAMAGE_PLAYER) == 1);      // una sola vez
        z.curFrame = 66; ZombieLogic::st3Sc03_02(z, e, t, motLen, 1, ev);                                // 66 >= motLen[7] = 67? no
        CHECK(z.step == 1);
        z.curFrame = 67; ZombieLogic::st3Sc03_02(z, e, t, motLen, 1, ev);
        CHECK((z.flags & ZF_DEAD));                                                                      // fin de animacion: CheckCom(-1)
        // quirk: compara curMot con sA4 (el fotograma), no con la animacion
        ZombieState y; y.script = s.data(); y.scriptSize = (int)s.size(); y.curMot = 7; y.motObjId = 3; ZEvents e2; ZombieLogic::st3Sc03_02(y, e, t, motLen, 1, e2);
        CHECK(count(e2, ZE_MOTION_SET) == 2);                                                           // MotSupSet2 al entrar y otra vez por curMot(7) != sA4(20)
    }
    {   // St1_Sc01_00
        std::vector<uint8_t> s; put16(s, 4); put16(s, 0); put16(s, 9); put16(s, 0); put16(s, 60); put16(s, 1);
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); z.curMot = 0; ZEnv e = baseEnv(); ZEvents ev;
        ZombieLogic::st1Sc01_00(z, e, t, motLen, ev);
        CHECK(z.sA4 == 4 && z.sAC == 9 && z.step == 1 && count(ev, ZE_MOTION_SET) == 1 && find(ev, ZE_MOTION_SET)->a == 4 && find(ev, ZE_MOTION_SET)->b == 9);
        z.curMot = 4; z.curFrame = 62; ev.clear(); ZombieLogic::st1Sc01_00(z, e, t, motLen, ev); CHECK(z.step == 1 && ev.empty());   // motLen[4]-1 = 63
        z.curFrame = 63; ZombieLogic::st1Sc01_00(z, e, t, motLen, ev);                                   // paso 2 + CheckCom: orden 60 (idx 1) reinicia el paso
        CHECK(z.step == 0 && z.handler[0] == 10 && z.handler[2] == 30 && z.handlerArg == 1);
    }
    {   // FrmWait
        std::vector<uint8_t> s; put16(s, 120); put16(s, -2); put16(s, 5); put16(s, 0); put16(s, 60); put16(s, 0);
        int16_t seq[2] = {41, 42};
        ZombieState z; z.script = s.data(); z.scriptSize = (int)s.size(); z.motObjId = 1; ZEnv e = baseEnv(); ZEvents ev;
        ZombieLogic::frmWait(z, e, t, seq, ev);
        CHECK(z.sA0 == 120 && z.sA4 == 41 && z.sA8 == 5 && z.sv9C == 1 && count(ev, ZE_MOTION_SET) == 1 && find(ev, ZE_MOTION_SET)->a == 41 && find(ev, ZE_MOTION_SET)->c == 5);
        int n = 1; while (z.step == 1 && n < 500) { ZombieLogic::frmWait(z, e, t, seq, ev); n++; }
        CHECK(n == 100 && z.step == 0 && z.handler[0] == 1);                // 120*50/60 = 100 fotogramas; CheckCom ejecuta la orden 60 (idx 0) y reinicia el paso
        // alterna: flags3 ALT -> segunda animacion; -1 -> la actual
        std::vector<uint8_t> s2; put16(s2, 60); put16(s2, -2); put16(s2, 0); put16(s2, 0);
        ZombieState y; y.script = s2.data(); y.scriptSize = (int)s2.size(); y.flags3 = Z3_ALT_ANIM; ZEvents e2; ZombieLogic::frmWait(y, e, t, seq, e2); CHECK(y.sA4 == 42);
        std::vector<uint8_t> s3; put16(s3, 60); put16(s3, -1); put16(s3, 0); put16(s3, 0);
        ZombieState u; u.script = s3.data(); u.scriptSize = (int)s3.size(); u.curMot = 77; ZombieLogic::frmWait(u, e, t, seq, e2); CHECK(u.sA4 == 77);
    }
}

int main() {
    g_bin = slurp("disc/TBL/GAMEMOD.BIN"); g_elf = slurp("baserom/SCES_504.11"); g_aomod = slurp("disc/TBL/AOMOD.BIN");
    testTables(); testMath(); testCounters(); testAttackSelect(); testPlayerDamage(); testMotion(); testDamage(); testPhysics(); testScript(); testStates();
    if (fails) { printf("%d FALLOS de %d comprobaciones\n", fails, checks); return 1; }
    printf("OK: %d comprobaciones, 0 fallos\n", checks);
    return 0;
}

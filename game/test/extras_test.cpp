// Test de game/extras (GunMgr, tienda/objetos, opciones, cuaderno de caza).
// Compilar y ejecutar desde la raiz del proyecto:
//   c++ -std=c++17 -Wall game/test/extras_test.cpp game/extras/gunmgr.cpp game/extras/shop.cpp game/extras/options.cpp game/extras/hunt.cpp -o build/extras_test && build/extras_test
#include "../extras/gunmgr.h"
#include "../extras/shop.h"
#include "../extras/options.h"
#include "../extras/hunt.h"
#include "../extras/orgrules.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
using namespace vn;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static std::vector<unsigned char> slurp(const char* p) {
    std::ifstream f(p, std::ios::binary); std::vector<unsigned char> v;
    if (f) v.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return v;
}
static int seqRand() { static int n = 0; return n++; }

static void testTablesAgainstFiles() {
    auto shp = slurp("disc/TBL/SHPMOD.BIN");
    auto opt = slurp("disc/TBL/OPTMOD.BIN");
    auto hnt = slurp("disc/TBL/HNTMOD.BIN");
    auto elf = slurp("baserom/SCES_504.11");
    if (shp.empty() || opt.empty() || hnt.empty() || elf.empty()) { printf("(tablas contra disco/ELF omitidas: ficheros no encontrados)\n"); return; }
    for (int i = 0; i < 52; i++) { int v; std::memcpy(&v, &shp[0xED60 + 4 * i], 4); CHECK(v == kItemPrice[i]); }
    size_t a = 0x2C2A10 - 0x140000 + 0x800;
    for (int i = 0; i < 56; i++) { CHECK(elf[a + 2 * i] == kItemAttr[i][0]); CHECK(elf[a + 2 * i + 1] == kItemAttr[i][1]); }
    for (int i = 0; i < 5; i++) CHECK((int8_t)opt[0x12B18 + i] == kDifficultyValue[i]);
    for (int i = 0; i < 17; i++) CHECK((int8_t)opt[0x12B20 + i] == kDifficultyMenu[i]);
    for (int i = 0; i < 11; i++) { CHECK(opt[0x12C50 + i] == kBgmLevel[i]); CHECK(opt[0x12C60 + i] == kVoiceLevel[i]); }
    for (int i = 0; i < 5; i++) {
        int v; std::memcpy(&v, &hnt[0x6A80 + 4 * i], 4); CHECK(v == kHuntBookFirst[i]);
        std::memcpy(&v, &hnt[0x6A94 + 4 * i], 4); CHECK(v == kHuntBookSimpleEnd[i]);
    }
    int sum = 0; for (int i = 0; i < 5; i++) sum += kHuntBookPages[i];
    CHECK(sum == 0x85);          // tamano del bloque copiado por Read/WriteCfgGameBuffer
    CHECK(kCfgCopy[1].size == (uint32_t)sum);
}

static void testGun() {
    GunMgr g; GunEnv e; GunLogic::init(g); GunLogic::start(g);
    CHECK(g.enabled == 1 && g.slot[1].index == 1 && g.slot[0].index == 0);
    // reset de la mira
    e.device[0] = 0; e.device[1] = 1; GunLogic::resetSightPos(g, e);
    CHECK(g.slot[0].sightX == -0x50 && g.slot[1].sightX == 0x50);
    e.device[0] = 2; g.slot[0].sightX = 7; GunLogic::resetSightPosPl(g, e, 0); CHECK(g.slot[0].sightX == 7);
    // mando: velocidad cfg 0 -> 1.8 ; stick a tope derecho 127*1.8/120*1.2 = 2.28 -> 2
    GunMgr p; GunLogic::start(p); GunEnv q; q.device[0] = 1; q.stickX[0] = 0xFF;
    GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 2 && p.slot[0].sightY == 0 && p.slot[0].sightValid == 1);
    q.speedCfg[0] = 10; q.stickX[0] = 0xFF; p.slot[0].sightX = 0; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 12);
    q.stickX[0] = 0x70; p.slot[0].sightX = 0; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 0);   // zona muerta
    q.stickX[0] = 0x80; q.stickY[0] = 0x00; p.slot[0].sightY = 0; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightY > 0);  // stick arriba: Y sube
    q.stickY[0] = 0x80; q.buttons[0] = 0x20; q.speedCfg[0] = 0; p.slot[0].sightX = 0; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 2);  // cruceta: 1.8*1.2 = 2.16
    q.boostMask[0] = 0x20; p.slot[0].sightX = 0; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 4);   // x2 = 4.32
    q.pauseLevel = 1; p.slot[0].sightX = 0; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 0);        // en pausa no se mueve
    q.pauseLevel = 0; q.boostMask[0] = 0; q.buttons[0] = 0x80; q.speedCfg[0] = 10; p.slot[0].sightX = -0x13F;
    GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == -0x140);                                            // limite
    q.recenterMask[0] = 0x80; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].sightX == 0 && p.slot[0].sightY == 0);
    q.pullTrigger[0] = true; GunLogic::readSynchPad(p, q, 0); CHECK(p.slot[0].trigger == 1);
    // GunCon: gatillo con 4 fotogramas de retraso
    GunMgr c; GunLogic::start(c); GunEnv r; r.device[0] = 2; r.gc1Status[0] = 2; r.gc1X[0] = 0x140 + 10; r.gc1Y[0] = 0xE0 - 20;
    r.pullTrigger[0] = true;
    bool fl = GunLogic::readSynchGun1(c, r, 0); CHECK(fl && c.slot[0].trigger == 0 && c.slot[0].flash == 3);
    r.pullTrigger[0] = false;
    CHECK(c.slot[0].sightX == 10 && c.slot[0].sightY == 20 && c.slot[0].sightValid == 1);
    GunLogic::readSynchGun1(c, r, 0); GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].trigger == 0);
    GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].trigger == 1);   // 4o fotograma
    GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].trigger == 0);
    r.gc1Status[0] = 0; GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].sightValid == 0 && c.slot[0].rawX == 0xFFFF);
    r.gc1Status[0] = 2; r.gc1X[0] = 0; r.gc1Y[0] = 100; GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].sightValid == 0);   // x = -0x140 fuera
    r.gc1X[0] = 0xFFFF; r.gc1Y[0] = 0xFFFF; GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].rawValid == 0);
    r.gc1X[0] = 0xFFFF; r.gc1Y[0] = 5; GunLogic::readSynchGun1(c, r, 0); CHECK(c.slot[0].rawValid == 1);
    // calibracion de usuario
    GunSlot s; s.index = 0; GunEnv k; k.device[0] = 3; k.calibX = 3; k.calibY = 4; s.rawX = 0x140; s.rawY = 0xE0;
    GunLogic::adjustValCz(s, k); CHECK(s.sightX == -6 && s.sightY == 8);
    k.gpCalibFlag = 1; GunLogic::adjustValCz(s, k); CHECK(s.sightX == 0 && s.sightY == 0 && s.sightValid == 1);
    k.gpCalibFlag = 0; k.gameState = 0x14; GunLogic::adjustValCz(s, k); CHECK(s.sightX == 0);
    // GunCon2
    GunMgr d; GunLogic::start(d); GunEnv t; t.device[1] = 3; t.gc2Trigger[1] = 5; t.gc2X[1] = 0x140 + 1; t.gc2Y[1] = 0xE0 - 2;
    GunLogic::readSynchGun2(d, t, 1); CHECK(d.slot[1].trigger == 1 && d.slot[1].sightX == 1 && d.slot[1].sightY == 2);
    // read_SYNCH completo: recarga, pausa, desactivado
    GunMgr m; GunLogic::init(m); GunEnv u; u.device[0] = 1; u.device[1] = 0; u.pressReload[1] = true;
    GunOut o = GunLogic::readSynch(m, u); CHECK(!o.reloadReq[1] && m.slot[0].sightValid == 0);    // enabled = 0: no hace nada
    GunLogic::start(m); o = GunLogic::readSynch(m, u); CHECK(o.reloadReq[1] && !o.reloadReq[0] && m.slot[0].sightValid == 1);
    u.pullTrigger[0] = true; u.pauseLevel = 2; GunLogic::readSynch(m, u); CHECK(m.slot[0].trigger == 0);
    // armas de rafaga (objeto 11/13) con mando: cada 4 fotogramas
    GunMgr b; GunLogic::start(b); GunEnv w; w.gameState = 0xA; w.gameKind = 1; w.active7418 = 1; w.device[0] = 1; w.itemSlot0[0] = 11;
    w.pullTriggerNow[0] = true; GunOut ob;
    for (int i = 0; i < 3; i++) { GunLogic::orgAddTrig(b, w, ob, 0); CHECK(b.slot[0].trigger == 0); }
    GunLogic::orgAddTrig(b, w, ob, 0); CHECK(b.slot[0].trigger == 1 && b.slot[0].hold == 0);
    GunLogic::orgAddTrig(b, w, ob, 0); CHECK(b.slot[0].hold == 0);        // con trigger ya a 1 se reinicia
    w.itemSlot0[0] = 8; b.slot[0].trigger = 0; b.slot[0].hold = 3; GunLogic::orgAddTrig(b, w, ob, 0); CHECK(b.slot[0].hold == 0);
    w.itemSlot0[0] = 13; w.device[0] = 3; w.playerBullet[0] = 5; b.slot[0].trigger = 0; b.cooldown = 0;
    GunLogic::orgAddTrig(b, w, ob, 0); CHECK(ob.flash[0] && ob.forceRefire[0]);
    ob = GunOut(); b.cooldown = 3; GunLogic::orgAddTrig(b, w, ob, 0); CHECK(!ob.flash[0]);
    w.device[0] = 2; b.cooldown = 0; GunLogic::orgAddTrig(b, w, ob, 0); CHECK(ob.flash[0] && !ob.forceRefire[0] && b.slot[0].flash == 4);
    w.playerMode[0] = 8; ob = GunOut(); b.slot[0].flash = 0; GunLogic::orgAddTrig(b, w, ob, 0); CHECK(!ob.flash[0]);
    // contador de cadencia 0..24
    GunMgr cd; GunLogic::start(cd); for (int i = 0; i < 24; i++) GunLogic::orgAddTrig0(cd, w); CHECK(cd.cooldown == 24);
    GunLogic::orgAddTrig0(cd, w); CHECK(cd.cooldown == 0);
    w.gameKind = 0; GunLogic::orgAddTrig0(cd, w); CHECK(cd.cooldown == 0);
}

static void testShop() {
    ShopCtx c; c.active[0] = true;
    // precios (tabla shpmod 0x9D3560)
    CHECK(kItemPrice[9] == 50000 && kItemPrice[51] == 1500 && kItemPrice[0] == 0);
    CHECK(ShopLogic::calcPrice(c, 1, 3, false) == 900);
    c.save.equipped[0][1] = 0x19; CHECK(ShopLogic::calcPrice(c, 1, 3, false) == 720);
    c.save.equipped[0][1] = 0x18; CHECK(ShopLogic::calcPrice(c, 1, 3, false) == 810);
    c.save.equipped[0][1] = 0; CHECK(ShopLogic::calcPrice(c, 1, 3, true) == 540);
    c.save.equipped[0][1] = 0x1A; CHECK(ShopLogic::calcPrice(c, 1, 3, true) == 675 && ShopLogic::calcPrice(c, 1, 3, false) == 900);
    c.active[0] = false; CHECK(ShopLogic::calcPrice(c, 1, 3, true) == 540);      // jugador ausente: no cuenta
    c.active[1] = true; c.save.equipped[1][1] = 0x19; c.save.equipped[1][1] = 0x19; CHECK(ShopLogic::calcPrice(c, 9, 1, false) == 40000);
    // plata
    c.save.silver = 999990; ShopLogic::silverAdd(c, 500); CHECK(c.save.silver == 999999 && c.silverAnimTimer == 0);
    c.save.silver = 1000; ShopLogic::silverSub(c, 40); CHECK(c.save.silver == 960 && c.silverAnimTimer == 25 && c.silverAnimA == 1000);
    ShopLogic::silverSub(c, 10); CHECK(c.save.silver == 950 && c.silverAnimTimer == 0 && c.silverAnimA == 950);
    c.save.silver = 100; ShopLogic::silverAdd(c, 29); CHECK(c.save.silver == 129 && c.silverAnimTimer == 0);
    ShopLogic::silverAdd(c, 30); CHECK(c.save.silver == 159 && c.silverAnimTimer == 25 && c.silverAnimA == 129);
    CHECK(ShopLogic::trnsIconToEquipType(10) == 0 && ShopLogic::trnsIconToEquipType(11) == 1 && ShopLogic::trnsIconToEquipType(12) == 2 && ShopLogic::trnsIconToEquipType(3) == -1);

    // equipo del otro jugador: solo cuenta si esta jugando (modos 7, 9 o > 10)
    ShopCtx e; e.active[0] = e.active[1] = true; e.save.equipped[1][2] = 33;
    e.playerMode[1] = 5; CHECK(ShopLogic::otherEquipCount(e, 0, 33) == 0);
    e.playerMode[1] = 7; CHECK(ShopLogic::otherEquipCount(e, 0, 33) == 1);
    e.playerMode[1] = 8; CHECK(ShopLogic::otherEquipCount(e, 0, 33) == 0);
    e.playerMode[1] = 9; CHECK(ShopLogic::otherEquipCount(e, 0, 33) == 1);
    e.playerMode[1] = 10; CHECK(ShopLogic::otherEquipCount(e, 0, 33) == 0);
    e.playerMode[1] = 11; CHECK(ShopLogic::otherEquipCount(e, 0, 33) == 1);
    ShopLogic::otherUnequip(e, 0, 33); CHECK(e.save.equipped[1][2] == 0);

    // compatibilidad: 16 y 17 comparten grupo 1 (exclusivo entre jugadores)
    ShopCtx k; k.active[0] = k.active[1] = true; k.save.stock[17] = 1; k.save.stock[34] = 1; k.save.stock[31] = 1; k.save.stock[8] = 1;
    k.save.equipped[0][1] = 16;
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 1, 17) == 1);       // misma ranura: sustituye
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 2, 17) == 0);       // otra ranura: mismo grupo ocupado
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 1, 1, 17) == 0);       // el otro jugador: grupo exclusivo entre jugadores
    k.active[0] = false; CHECK(ShopLogic::checkItemNum(k, k.save.stock, 1, 1, 17) == 1);   // jugador 0 ausente
    k.active[0] = true;
    k.save.equipped[1][2] = 33; CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 2, 34) == 0);   // 33/34 grupo 9 tipo 2
    k.save.equipped[1][2] = 0; k.save.equipped[0][2] = 30; CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 3, 31) == 0);   // grupo 8 tipo 1: mismo jugador
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 1, 3, 31) == 1);        // tipo 1: no afecta al otro jugador
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 0, 0) == 1);         // objeto 0 siempre valido
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 0, 12) == 0);        // sin unidades ni equipado
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 0, 8) == 1);         // tipo 0: sin restriccion
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, -1, 0, 8) == 1 && ShopLogic::checkItemNum(k, k.save.stock, -1, 0, 9) == 0);
    k.save.equipped[1][0] = 9; k.playerMode[1] = 7;
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, -1, 0, 9) == 0 && ShopLogic::checkItemNum(k, k.save.stock, -2, 0, 9) == 1);
    CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 0, 9) == 1);         // se lo puede quitar al otro (esta jugando)
    k.playerMode[1] = 5; CHECK(ShopLogic::checkItemNum(k, k.save.stock, 0, 0, 9) == 0);

    // existencia por ranura (rango 1..15 / 16..29 / 30..52)
    ShopCtx x; CHECK(ShopLogic::checkExistItem(x, x.save.stock, 0) == 0);
    x.save.stock[15] = 1; CHECK(ShopLogic::checkExistItem(x, x.save.stock, 0) == 1 && ShopLogic::checkExistItem(x, x.save.stock, 1) == 0);
    x.save.stock[16] = 2; CHECK(ShopLogic::checkExistItem(x, x.save.stock, 1) == 1);
    x.save.stock[52] = 1; CHECK(ShopLogic::checkExistItem(x, x.save.stock, 2) == 1 && ShopLogic::checkExistItem(x, x.save.stock, 3) == 1);
    CHECK(ShopLogic::checkExistItem(x, x.save.stock, 9) == 1);   // ranura desconocida
    ShopCtx y; y.save.equipped[0][3] = 40; y.playerMode[0] = 7; CHECK(ShopLogic::checkExistItem(y, y.save.stock, 2) == 1);   // equipado por jugador 0 en juego

    // seleccion del siguiente objeto
    ShopCtx d; d.save.stock[3] = 1; d.save.stock[5] = 2;
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 0, 1) == 3);
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 3, 1) == 5);
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 5, 1) == 3);      // vuelta al principio
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 3, -1) == 5);     // vuelta al final
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 0, -1) == 5);
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 5, 0) == 5 && ShopLogic::decideItemNum(d, d.save.stock, -1, 0, 4, 0) == 0);
    d.active[0] = true;
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, 0, 0, 3, 1) == 5);
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, 0, 0, 5, 1) == 0);      // jugador: sin vuelta
    CHECK(ShopLogic::decideItemNum(d, d.save.stock, 0, 0, 0, -1) == 5);
    ShopCtx empty; CHECK(ShopLogic::decideItemNum(empty, empty.save.stock, -1, 0, 0, 1) == 0);

    // devolver el equipo al inventario (tope 99)
    ShopCtx u; u.save.equipped[1][0] = 9; u.save.equipped[1][2] = 40; u.save.stock[9] = 5; u.save.stock[40] = 99;
    ShopLogic::unEquipTool(u, 1); CHECK(u.save.stock[9] == 6 && u.save.stock[40] == 99 && u.save.equipped[1][0] == 0 && u.save.equipped[1][2] == 0);

    // equipar
    ShopCtx q; q.active[0] = true; q.save.stock[8] = 1; q.save.stock[9] = 1; q.selSlot[0] = 0; q.selItem[0] = 8;
    ShopEvents ev = ShopLogic::setEquipmentItem(q, 0); CHECK(ev.sound == 0x7000F && q.save.equipped[0][0] == 8 && q.save.stock[8] == 0);
    q.selItem[0] = 9; ev = ShopLogic::setEquipmentItem(q, 0); CHECK(q.save.equipped[0][0] == 9 && q.save.stock[8] == 1 && q.save.stock[9] == 0 && q.hunt.page[3][7] == 1);
    q.selItem[0] = 0; ev = ShopLogic::setEquipmentItem(q, 0); CHECK(ev.sound == 0x7000D && q.save.equipped[0][0] == 0 && q.save.stock[9] == 1);
    q.selItem[0] = 12; ev = ShopLogic::setEquipmentItem(q, 0); CHECK(ev.sound == 0 && q.save.equipped[0][0] == 0);    // sin existencias
    // quitarselo al otro jugador
    ShopCtx z; z.active[0] = z.active[1] = true; z.playerMode[1] = 7; z.save.equipped[1][0] = 10; z.selItem[0] = 10; z.selSlot[0] = 0;
    z.selSlot[1] = 0; z.selItem[1] = 10;
    ev = ShopLogic::setEquipmentItem(z, 0); CHECK(z.save.equipped[0][0] == 10 && z.save.equipped[1][0] == 0 && z.selItem[1] == 0);

    // objetos conseguidos
    ShopCtx g; ShopLogic::orgAddItemG(g, 5); CHECK(g.save.stock[5] == 1 && g.hunt.page[3][4] == 1);
    g.save.stock[5] = 99; ShopLogic::orgAddItemG(g, 5); CHECK(g.save.stock[5] == 99);
    ShopLogic::orgAddItemG(g, 0); ShopLogic::orgAddItemG(g, 0x35); CHECK(g.save.stock[0] == 0);
    ShopLogic::orgAddShopItemG(g, 7); CHECK(g.save.shopAvail[7] == 1 && g.save.shopNewFlag == 1);

    // efectos al empezar
    ShopCtx f; f.active[0] = f.active[1] = true;
    f.save.equipped[0][2] = 0x24; f.save.equipped[0][3] = 0x21; f.save.equipped[1][2] = 0x22;
    CreditResult cr = ShopLogic::setCredit(f); CHECK(!cr.freePlay && cr.bonus == 16);
    f.save.equipped[1][3] = 0x25; cr = ShopLogic::setCredit(f); CHECK(cr.freePlay && cr.bonus == 0);
    f.save.equipped[1][0] = 0x23; f.save.equipped[1][3] = 0; cr = ShopLogic::setCredit(f); CHECK(cr.bonus == 16);   // ranuras 0/1 no cuentan
    ShopCtx df; df.active[0] = true; CHECK(ShopLogic::setDifficulty(df) == 8);
    df.save.equipped[0][2] = 0x34; CHECK(ShopLogic::setDifficulty(df) == 16);
    df.save.equipped[0][3] = 0x34; CHECK(ShopLogic::setDifficulty(df) == 16);   // 24 -> tope 16
    df.save.equipped[0][2] = 0x31; df.save.equipped[0][3] = 0x32; CHECK(ShopLogic::setDifficulty(df) == 1);
    df.save.equipped[0][2] = 0x33; df.save.equipped[0][3] = 0x33; CHECK(ShopLogic::setDifficulty(df) == 0);
    df.active[0] = false; CHECK(ShopLogic::setDifficulty(df) == 8);
    ShopCtx ds; ds.active[1] = true; ds.save.equipped[1][1] = 0x12; CHECK(ShopLogic::setDispScore(ds) == 1);
    ds.active[1] = false; CHECK(ShopLogic::setDispScore(ds) == 0);
    // peticiones
    ShopSave rq; CHECK(ShopLogic::countRemainReq(rq) == REQ_COUNT && ShopLogic::countUnsolvedReq(rq) == 0);
    rq.reqFlags[0] = 1; rq.reqFlags[3] = 3; rq.reqFlags[32] = 2;
    CHECK(ShopLogic::countRemainReq(rq) == REQ_COUNT - 2 && ShopLogic::countUnsolvedReq(rq) == 2);
    CHECK(ShopLogic::shotswPlCheck(c, 1, true) == 1 && ShopLogic::shotswPlCheck(c, 0, true) == 0 && ShopLogic::dirswCheck() == 0);
}

static void testOptions() {
    CHECK(OptionsLogic::calcBgmBalance(0) == 0 && OptionsLogic::calcBgmBalance(1) == 1 && OptionsLogic::calcBgmBalance(0x1A) == 1);
    CHECK(OptionsLogic::calcBgmBalance(0x1B) == 2 && OptionsLogic::calcBgmBalance(0xFF) == 10 && OptionsLogic::calcBgmBalance(0xE7) == 10);
    CHECK(OptionsLogic::calcVoiceBalance(0x17) == 1 && OptionsLogic::calcVoiceBalance(0xE6) == 10 && OptionsLogic::calcVoiceBalance(0xFF) == 10);
    for (int i = 0; i < 11; i++) CHECK(OptionsLogic::calcBgmBalance(kBgmLevel[i]) == i && OptionsLogic::calcVoiceBalance(kVoiceLevel[i]) == i);
    CHECK(OptionsLogic::chgConfigBitR(0) == 0 && OptionsLogic::chgConfigBitR(1) == 0x2000 && OptionsLogic::chgConfigBitR(2) == 0x8000);
    CHECK(OptionsLogic::chgConfigBitR(0x3F) == (0x2000 | 0x8000 | 0x800 | 0x400 | 0x200 | 0x100));
    SysCfg s; CsOptData m;
    OptionsLogic::gamesystemDefault(m, s);
    CHECK(s.difficulty == 8 && s.initLife == 3 && s.maxLife == 5 && s.credits == 5 && m.diff == 2);
    { const int exp[17] = {0,0,1,1,1,1,1,1,2,2,3,3,3,3,3,3,4}; for (int v = 0; v <= 16; v++) { s.difficulty = (int8_t)v; OptionsLogic::sysCfgToMenu(m, s); CHECK(m.diff == exp[v]); } }
    s.difficulty = 16; OptionsLogic::sysCfgToMenu(m, s); CHECK(m.diff == 4);
    s.difficulty = 8; OptionsLogic::sysCfgToMenu(m, s); CHECK(m.diff == 2);
    s.credits = 0; s.sightType = 1; s.bgmVol = 0x80; s.voiceVol = 0x73; s.voiceVol2 = 0; s.padCfgA = 1; s.padCfgB = 0; s.vibration1 = 1; s.vibration2 = 2;
    OptionsLogic::sysCfgToMenu(m, s);
    CHECK(m.credits == 10 && m.sight == 1 && m.bgm == 5 && m.voice == 5 && m.voice2 == 0 && m.padA0 == 4 && m.padA1 == 1 && m.padB0 == 1 && m.padB1 == 4 && m.vib1 == 1 && m.vib2 == 0 && m.c == 2 && m.f == 2);
    m.vib1 = 0; m.vib2 = 1; OptionsLogic::setVibrationSysCfg(s, m); CHECK(s.vibration1 == 0 && s.vibration2 == 1);
    // dificultad
    for (int ch = 1; ch <= 5; ch++) { OptionsLogic::difficultyChoose(m, s, ch); }
    CHECK(s.difficulty == 16 && m.diff == 4);
    OptionsLogic::difficultyChoose(m, s, 3); CHECK(s.difficulty == 0);
    OptionsLogic::difficultyChoose(m, s, 2); CHECK(s.difficulty == 10);
    OptionsLogic::difficultyChoose(m, s, 4); CHECK(s.difficulty == 8);
    OptionsLogic::difficultyChoose(m, s, 1); CHECK(s.difficulty == 6);
    OptionsLogic::difficultyChoose(m, s, 0); CHECK(s.difficulty == 6);
    // vidas
    OptionsLogic::lifeDefault(m, s); CHECK(m.initLife == 3 && m.maxLife == 5 && s.initLife == 3);
    LifeResult r = OptionsLogic::lifeAdjust(m, s, LIFE_INIT_INC); CHECK(r.changed && m.initLife == 4 && s.initLife == 4 && r.warnTimer == -1);
    OptionsLogic::lifeAdjust(m, s, LIFE_INIT_INC); CHECK(m.initLife == 5);
    r = OptionsLogic::lifeAdjust(m, s, LIFE_INIT_INC); CHECK(!r.changed && m.initLife == 5 && r.warnTimer == 0x78);
    r = OptionsLogic::lifeAdjust(m, s, LIFE_MAX_INC); CHECK(r.changed && m.maxLife == 6 && s.maxLife == 6 && r.warnTimer == 0);
    for (int i = 0; i < 5; i++) OptionsLogic::lifeAdjust(m, s, LIFE_MAX_INC);
    CHECK(m.maxLife == 9); r = OptionsLogic::lifeAdjust(m, s, LIFE_MAX_INC); CHECK(!r.changed && m.maxLife == 9);
    for (int i = 0; i < 4; i++) OptionsLogic::lifeAdjust(m, s, LIFE_INIT_INC);
    CHECK(m.initLife == 9); r = OptionsLogic::lifeAdjust(m, s, LIFE_INIT_INC); CHECK(!r.changed && r.warnTimer == -1);
    OptionsLogic::lifeAdjust(m, s, LIFE_MAX_DEC); CHECK(m.maxLife == 8 && m.initLife == 8 && s.initLife == 8);
    for (int i = 0; i < 20; i++) OptionsLogic::lifeAdjust(m, s, LIFE_MAX_DEC);
    CHECK(m.maxLife == 1 && m.initLife == 1 && s.maxLife == 1);
    r = OptionsLogic::lifeAdjust(m, s, LIFE_INIT_DEC); CHECK(!r.changed && m.initLife == 1 && r.warnTimer == 0);
    // creditos
    m.credits = 5; s.unlockFlag = 0;
    for (int i = 0; i < 8; i++) OptionsLogic::creditsAdjust(m, s, true);
    CHECK(m.credits == 9 && s.credits == 9);
    s.unlockFlag = 1; OptionsLogic::creditsAdjust(m, s, true); CHECK(m.credits == 10 && s.credits == 0);
    OptionsLogic::creditsAdjust(m, s, true); CHECK(m.credits == 10);
    OptionsLogic::creditsAdjust(m, s, false); CHECK(m.credits == 9 && s.credits == 9);
    for (int i = 0; i < 20; i++) OptionsLogic::creditsAdjust(m, s, false);
    CHECK(m.credits == 1 && s.credits == 1);
    // volumenes
    m.bgm = 0; s.bgmVol = 0; OptionsLogic::balanceAdjust(m, s, 0, false); CHECK(m.bgm == 0 && s.bgmVol == 0);
    OptionsLogic::balanceAdjust(m, s, 0, true); CHECK(m.bgm == 1 && s.bgmVol == 0x1A);
    m.voice = 9; OptionsLogic::balanceAdjust(m, s, 1, true); CHECK(m.voice == 10 && s.voiceVol == 0xE6);
    OptionsLogic::balanceAdjust(m, s, 1, true); CHECK(m.voice == 10);
    m.voice2 = 3; OptionsLogic::balanceAdjust(m, s, 2, false); CHECK(m.voice2 == 2 && s.voiceVol2 == 0x2E);
    int8_t sv = 9; OptionsLogic::sightSpeedAdjust(s, sv, true); CHECK(sv == 10 && s.sightSpeed == 10);
    OptionsLogic::sightSpeedAdjust(s, sv, true); CHECK(sv == 10);
    sv = 0; OptionsLogic::sightSpeedAdjust(s, sv, false); CHECK(sv == 0 && s.sightSpeed == 0);
    // copia de seguridad (189 palabras)
    std::vector<uint32_t> a(0xBD, 7), b(0xBD, 0);
    CHECK(!OptionsLogic::checkSysCfgBak(b.data(), a.data()));
    OptionsLogic::copySysCfgBak(b.data(), a.data()); CHECK(OptionsLogic::checkSysCfgBak(b.data(), a.data()));
    a[0xBC] = 1; CHECK(!OptionsLogic::checkSysCfgBak(b.data(), a.data()));
    // posicion de la mira en el menu
    GunSlot gs; gs.sightX = 10; gs.sightY = 20; gs.sightValid = 1;
    SightScreenPos sp = OptionsLogic::getSightPos(gs, 3, 0); CHECK(sp.x == 0x14A && sp.y == 0xE0 - 20 && !sp.drawSight);
    gs.sightValid = 0; sp = OptionsLogic::getSightPos(gs, 2, 0); CHECK(sp.x == 0 && sp.y == 0);
    sp = OptionsLogic::getSightPos(gs, 1, 0); CHECK(sp.x == 0x14A && sp.drawSight);
    sp = OptionsLogic::getSightPos(gs, 1, 0xB); CHECK(!sp.drawSight);
    sp = OptionsLogic::getSightPos(gs, 0, 0); CHECK(sp.x == 0 && sp.y == 0);
    CHECK(kCfgCopy[0].size == 0xC0 && kCfgCopy[0].live == 0x502690 && kCfgCopy[0].saved == 0x50293C);
}

static void testHunt() {
    HuntSave h;
    CHECK(HuntLogic::addPage(h, 0, 0) == 1 && HuntLogic::addPage(h, 0, 0) == 0 && h.page[0][0] == 1);
    CHECK(HuntLogic::addPage(h, 3, 1) == 1 && h.page[3][0] == 1);        // libro 3: indice - 1
    CHECK(HuntLogic::addPage(h, 5, 0) == 0 && HuntLogic::addPage(h, 3, 53) == 0);
    // CheckBookComp
    CHECK(HuntLogic::checkBookComp(h, 0) == 0 && HuntLogic::checkBookComp(h, 9) == 1);
    for (int b = 0; b < HUNT_BOOKS; b++) for (int i = 0; i < kHuntBookPages[b]; i++) h.page[b][i] = 1;
    for (int b = 0; b < HUNT_BOOKS; b++) CHECK(HuntLogic::checkBookComp(h, b) == 1);
    h.page[4][38] = 0; CHECK(HuntLogic::checkBookComp(h, 4) == 0);
    // galeria aleatoria: solo 7..38 sin ver
    HuntSave g;
    for (int i = 7; i < 39; i++) g.page[4][i] = 1;
    CHECK(HuntLogic::addOtherGallery(g, seqRand) == 0);
    g.page[4][10] = 0; g.page[4][20] = 0; g.page[4][5] = 0;            // la 5 no es candidata
    int n = HuntLogic::addOtherGallery(g, seqRand); CHECK(n == 1 && (g.page[4][10] == 1) != (g.page[4][20] == 1) && g.page[4][5] == 0);
    // CheckNoData: libro 0, zona simple (paginas 0..4) y doble (5..)
    HuntSave d;
    CHECK(HuntLogic::checkNoData(d, 0, 0, 2) == 1);
    d.page[0][2] = 1; CHECK(HuntLogic::checkNoData(d, 0, 0, 2) == 0 && HuntLogic::checkNoData(d, 0, 1, 2) == 0);
    d.page[0][5] = 1; CHECK(HuntLogic::checkNoData(d, 0, 1, 5) == 0 && HuntLogic::checkNoData(d, 0, 0, 5) == 1);   // pagina global 5 -> bytes 5 (der) y 6 (izq)
    d.page[0][6] = 1; CHECK(HuntLogic::checkNoData(d, 0, 0, 5) == 0);
    d.page[0][10] = 1; CHECK(HuntLogic::checkNoData(d, 0, 0, 7) == 0 && HuntLogic::checkNoData(d, 0, 1, 7) == 1);
    CHECK(HuntLogic::checkNoData(d, 0, 2, 5) == 1);                       // lado invalido
    // libro 1: paginas 8..0x16, un byte por pagina
    d.page[1][3] = 1; CHECK(HuntLogic::checkNoData(d, 1, 0, 8 + 3) == 0 && HuntLogic::checkNoData(d, 1, 0, 8 + 4) == 1);
    CHECK(HuntLogic::checkNoData(d, 1, 0, 0x17) == 1);                    // fuera de rango: sin zona doble
    // libro 3: dos bytes por pagina (der = par, izq = impar)
    d.page[3][2 * 5] = 1; CHECK(HuntLogic::checkNoData(d, 3, 1, 0x24 + 5) == 0 && HuntLogic::checkNoData(d, 3, 0, 0x24 + 5) == 1);
    d.page[3][2 * 5 + 1] = 1; CHECK(HuntLogic::checkNoData(d, 3, 0, 0x24 + 5) == 0);
    // libro 2 y 4 zona doble
    d.page[2][10] = 1; CHECK(HuntLogic::checkNoData(d, 2, 1, 0x21) == 0 && HuntLogic::checkNoData(d, 2, 0, 0x21) == 1);
    d.page[4][7] = 1; d.page[4][8] = 1; CHECK(HuntLogic::checkNoData(d, 4, 1, 0x45) == 0 && HuntLogic::checkNoData(d, 4, 0, 0x45) == 0);
    d.page[4][0] = 1; CHECK(HuntLogic::checkNoData(d, 4, 0, 0x3E) == 0);
    CHECK(HuntLogic::checkNoData(d, 7, 0, 0) == 1);
    // interpolacion
    CHECK(HuntLogic::calcLinePos(10.0f, 20.0f, 0, 4.0f) == 10.0f && HuntLogic::calcLinePos(10.0f, 20.0f, 4, 4.0f) == 20.0f);
    CHECK(HuntLogic::calcLinePos(10.0f, 20.0f, 2, 4.0f) == 15.0f);
}

static void testOrg() {
    OrgMissionCtx c; c.gameMode = 1; c.stageMode = 10; c.flagsA = 3;
    c.playerScore[1] = 29999; CHECK(OrgRules::missionOver(c) == 0);
    c.playerScore[1] = 30000; CHECK(OrgRules::missionOver(c) == 1);
    c.flagsA = 0x23; c.flagsB = 3; c.counter = 999; CHECK(OrgRules::missionOver(c) == 0);
    c.counter = 1000; CHECK(OrgRules::missionOver(c) == 2);
    c.flagsB = 0; c.flagsC = 3; c.playerScore[0] = 139999; c.playerScore[1] = 0; CHECK(OrgRules::missionOver(c) == 0);
    c.playerScore[0] = 140000; CHECK(OrgRules::missionOver(c) == 3);
    c.gameMode = 2; CHECK(OrgRules::missionOver(c) == 0);
    CHECK(OrgRules::dispScoreJoinIn(1, 0x12)); CHECK(!OrgRules::dispScoreJoinIn(7, 0x12)); CHECK(!OrgRules::dispScoreJoinIn(1, 0x11));
}

int main() {
    testTablesAgainstFiles(); testGun(); testShop(); testOptions(); testHunt(); testOrg();
    printf("%d comprobaciones, %d fallos\n", checks, fails);
    return fails ? 1 : 0;
}

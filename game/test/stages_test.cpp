#include "../stages/org.h"
#include "../stages/komono.h"
#include "../stages/training.h"
#include "../stages/title.h"
#include <cstdio>
using namespace vn;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static int g_seq[32], g_n = 0, g_i = 0;
static void seq(std::initializer_list<int> v) { g_n = 0; g_i = 0; for (int x : v) g_seq[g_n++] = x; }
static int fakeRand() { return g_seq[g_i++ % g_n]; }
static float fakeRandF() { return 0.5f; }

int main() {
    // ---- orgmod ----
    StageSys s; s.gameState = 0xA; s.gameKind = 1;
    CHECK(kOrgWeaponDamage[7] == 2.0f && kOrgWeaponDamage[10] > 0.59f && kOrgWeaponDamage[10] < 0.61f);   // D_00A12500
    s.equip[0][0] = 7; CHECK(OrgLogic::weaponDamage(s, 0) == 2.0f);
    s.equip[1][0] = 8; s.pl[1].flag346 = 0; CHECK(OrgLogic::weaponDamage(s, 1) == 0.5f);
    s.pl[1].flag346 = 1; CHECK(OrgLogic::weaponDamage(s, 1) == kOrgWeaponDamage[8]);
    // vida enemiga
    s.pl[0].mode = 5; s.pl[1].mode = 7; s.equip[0][2] = 0x26; s.equip[1][3] = 0x27;
    CHECK(OrgLogic::enemyLifeScale(s) > 0.6665f && OrgLogic::enemyLifeScale(s) < 0.6667f);   // el jugador 1 (modo 7) no cuenta
    s.pl[1].mode = 4; CHECK(OrgLogic::enemyLifeScale(s) == 0.5f);
    // inventario
    CHECK(OrgLogic::addItem(s, 5) && s.invCount[5] == 1);
    s.invCount[5] = 98; OrgLogic::addItem(s, 5); CHECK(s.invCount[5] == 99); OrgLogic::addItem(s, 5); CHECK(s.invCount[5] == 99);
    CHECK(!OrgLogic::addItem(s, 0) && !OrgLogic::addItem(s, 0x35));
    // tienda: tabla ORGMOD.BIN 0xA12B70
    CHECK(kOrgShopLevel[1] == 1 && kOrgShopLevel[4] == 6 && kOrgShopLevel[9] == 4 && kOrgShopLevel[0x34] == 2);
    OrgLogic::shopLevel(s, 4);
    CHECK(s.shopOpen[9] && s.shopOpen[15] && s.shopOpen[27] && s.shopOpen[37] && s.shopOpen[39] && !s.shopOpen[1] && s.shopDirty == 1);
    // plata
    SilverEvent se = OrgLogic::addSilver(s, 500, false); CHECK(se.create && s.silver == 500 && s.silverTotal == 500);
    se = OrgLogic::addSilver(s, 999999, true); CHECK(se.recharge && s.silver == 999999 && s.silverTotal == 1000499);
    s.gameKind = 2; se = OrgLogic::addSilver(s, 10, true); CHECK(!se.create && !se.recharge && s.silver == 999999 && s.silverTotal == 1000509);
    s.gameKind = 1;
    // mision especial
    for (auto& f : s.missionFlags) f = 0;
    CHECK(OrgLogic::missionOver(s) == 0);
    s.missionFlags[14] = 3; s.pl[0].score = 29999; CHECK(OrgLogic::missionOver(s) == 0);
    s.pl[1].score = 30000; CHECK(OrgLogic::missionOver(s) == 1);
    s.missionFlags[14] = 3 | 0x20; s.missionFlags[24] = 3; s.silverTotal = 999; CHECK(OrgLogic::missionOver(s) == 0);
    s.silverTotal = 1000; CHECK(OrgLogic::missionOver(s) == 2);
    s.missionFlags[24] = 0; s.missionFlags[31] = 3; s.pl[0].score = 0x222E0; CHECK(OrgLogic::missionOver(s) == 3);
    s.pl[0].score = 0x222DF; s.pl[1].score = 0; CHECK(OrgLogic::missionOver(s) == 0);
    OrgLogic::missionComplete(s, 14); CHECK((s.missionFlags[14] & 0x14) == 0x14 && !(s.missionFlags[14] & 2));
    s.missionFlags[24] = 0x7F; OrgLogic::missionComplete(s, 14); CHECK(s.missionFlags[24] == (0x7F & ~0x25));
    // equipo del otro jugador (jtbl 0xA12BD0: modos 7, 9 y > 10 actuan)
    StageSys j; j.pl[1].mode = 7; j.equip[1][0] = 0x10; j.equip[1][2] = 0x10; j.equip[1][3] = 0x11;
    CHECK(OrgLogic::anotherPlayerHas(j, 0, 0x10) == 2);
    j.pl[1].mode = 5; CHECK(OrgLogic::anotherPlayerHas(j, 0, 0x10) == 0);
    j.pl[1].mode = 9; OrgLogic::unequipAnotherPlayer(j, 0, 0x10); CHECK(j.equip[1][0] == 0 && j.equip[1][2] == 0x10);
    j.pl[1].mode = 11; CHECK(OrgLogic::anotherPlayerHas(j, 0, 0x10) == 1);
    // unirse: tabla ELF 0x2C2A10: obj 0x10/0x11 = grupo 1, tipo 2; 0x12 = grupo 2
    CHECK(kItemInfo[0x10][0] == 1 && kItemInfo[0x10][1] == 2 && kItemInfo[0x12][0] == 2 && kItemInfo[0x1E][1] == 1 && kItemInfo[0x1E][0] == 8);
    StageSys u; u.pl[1].mode = 3; u.equip[0][0] = 0x10; u.equip[0][1] = 0x12; u.equip[1][0] = 0x11;
    OrgLogic::unequipJoinIn(u, 0); CHECK(u.equip[0][0] == 0 && u.equip[0][1] == 0x12 && u.invCount[0x10] == 1);
    u.pl[1].mode = 7; u.equip[0][0] = 0x10; OrgLogic::unequipJoinIn(u, 0); CHECK(u.equip[0][0] == 0x10);   // otro en modo 7: no hace nada
    CHECK(OrgLogic::setDispScoreJoinIn(3, 0x12) && !OrgLogic::setDispScoreJoinIn(7, 0x12) && !OrgLogic::setDispScoreJoinIn(3, 1));

    // ---- aomod ----
    StageSys k; k.gameState = 0xA; k.gameKind = 1; k.pl[0].mode = 5; k.pl[1].mode = 8;
    k.equip[0][1] = 0x13;
    CHECK(KomonoLogic::checkEquipItem(k, 1, 0x13)); CHECK(!KomonoLogic::checkEquipItem(k, 1, 0x14));
    k.pl[0].mode = 2; CHECK(!KomonoLogic::checkEquipItem(k, 1, 0x13));          // modo 2 salta
    k.pl[0].mode = 5; k.gameKind = 2; CHECK(!KomonoLogic::checkEquipItem(k, 1, 0x13)); k.gameKind = 1;
    ItemTbl tb{}; tb.kind = 0; tb.count = 3; tb.e[0] = {1, 50}; tb.e[1] = {9, 30}; tb.e[2] = {0x16, 20};
    StageSys none; none.gameState = 0xA; none.gameKind = 1;
    seq({10}); CHECK(KomonoLogic::calcItemNo(none, tb, fakeRand) == 1);        // 10 < 50
    seq({50}); CHECK(KomonoLogic::calcItemNo(none, tb, fakeRand) == 9);        // 50..79
    seq({79}); CHECK(KomonoLogic::calcItemNo(none, tb, fakeRand) == 9);
    seq({80}); CHECK(KomonoLogic::calcItemNo(none, tb, fakeRand) == 0x16);
    seq({105}); CHECK(KomonoLogic::calcItemNo(none, tb, fakeRand) == 1);       // 105 % 100 = 5
    // 0x13 equipado: 14 -> luego regla 0x15: sin 0x15, queda 14
    seq({0}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 14);
    k.equip[0][1] = 0x14; seq({0}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 16);
    // 0x17 equipado: pesos >= 21 suben 25: 50/30/45 = 125 -> exceso 25 recortado de los objetos 8..20 (el 9: 30 -> 5)
    k.equip[0][1] = 0x17; seq({0}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 1);
    seq({50}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 9);          // 50..54
    seq({55}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 0x16);
    // 0x15 equipado: objeto 9 (>= 8, < 20) con rand%200 < 25 -> 20
    k.equip[0][1] = 0x15; seq({50, 24}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 20);
    seq({50, 25}); CHECK(KomonoLogic::calcItemNo(k, tb, fakeRand) == 9);
    tb.kind = 1; CHECK(KomonoLogic::calcItemNo(none, tb, fakeRand) == -1);
    // camara / puntos de fin
    CamPathNow cam; cam.pathId = 0x228; cam.frame = 550.0f;
    CHECK(KomonoLogic::checkCamPath(cam, 0x228, 550)); cam.frame = 549.0f; CHECK(!KomonoLogic::checkCamPath(cam, 0x228, 550));
    KomonoLogic::EndCP cp; cp.n = 2; cp.id[0] = 5; cp.frame[0] = 10.0f; cp.id[1] = 0x228; cp.frame[1] = 500.0f;
    CHECK(KomonoLogic::checkEndCP(cp, cam)); cam.pathId = 5; cam.frame = 9.0f; CHECK(!KomonoLogic::checkEndCP(cp, cam));
    // Komono3: rama y mision
    KomonoLogic::K3 k3; KomonoLogic::K3Globals kg; k3.missionNo = 14; k3.msgPath = 7; k3.msgFrame = 100; k3.closePath = 8; k3.closeFrame = 20;
    CHECK(KomonoLogic::onHit(k3, kg, false).sound == 0);
    KomonoLogic::K3Event e = KomonoLogic::onHit(k3, kg, true); CHECK(e.sound == 0xB012B && k3.branchState == 1 && kg.branchNo500F05 == 1 && kg.branchNoGp73CC == 1);
    e = KomonoLogic::onHit(k3, kg, true); CHECK(e.sound == 0x20025 && e.soundArg == 0x1000 && e.hitEffect);
    cam.pathId = 7; cam.frame = 100.0f;
    e = KomonoLogic::checkMissionCompleteMessageDispTiming(k3, cam); CHECK(e.showMissionComplete && e.missionNo == 14 && k3.missionState == 1 && k3.branchState == 2 && k3.doneFlag == 1);
    e = KomonoLogic::checkMissionCompleteMessageDispTiming(k3, cam); CHECK(!e.showMissionComplete);
    KomonoLogic::checkMissionEnd_setTrnsl(k3, kg, false); CHECK(k3.missionState == 2);   // gp-0x72E0 == 0
    k3.trnsl = 0.02f; KomonoLogic::checkMissionEnd_setTrnsl(k3, kg, false); CHECK(k3.trnsl == 0.0f);
    k3.trnsl = 1.0f; KomonoLogic::checkMissionEnd_setTrnsl(k3, kg, true); CHECK(k3.trnsl == 1.0f);
    cam.pathId = 8; cam.frame = 20; e = KomonoLogic::checkCloseTiming_clrBranchNo(k3, kg, cam); CHECK(e.closeTask && kg.branchNo500F05 == 0 && kg.branchNoGp73CC == 0);
    KomonoLogic::K3 k4; k4.missionNo = 3; StageSys ks; ks.gameState = 0xA; ks.gameKind = 1; KomonoLogic::K3Globals kg4; kg4.branchNo500F05 = 1;
    e = KomonoLogic::checkGameMode(k4, ks, kg4); CHECK(e.closeTask && kg4.branchNo500F05 == 0);    // mision 3 no abierta
    ks.missionFlags[3] = 3; kg4.branchNo500F05 = 1; e = KomonoLogic::checkGameMode(k4, ks, kg4); CHECK(!e.closeTask && kg4.branchNo500F05 == 1);
    ks.missionFlags[3] = 3 | 0x20; e = KomonoLogic::checkGameMode(k4, ks, kg4); CHECK(e.closeTask);
    // recogida de objeto
    KomonoLogic::ItemTaskState it; it.flags = 8 | 2; it.itemNo = 9; int life[2] = {5, 5}, mlife[2] = {5, 7};
    KomonoLogic::Pickup pk = KomonoLogic::itemCtrlPost(it, life, mlife, fakeRand);
    CHECK(pk.player == 0 && pk.showGetDisp && pk.giveItem && pk.sound == 0xB011E && it.picked == 1 && it.flags == 0);
    it = {}; it.flags = 8 | 6; it.itemNo = 0x14; seq({1}); pk = KomonoLogic::itemCtrlPost(it, life, mlife, fakeRand);
    CHECK(pk.player == 1 && pk.sound == 0xB011F && it.flags == 6);
    it = {}; it.flags = 8 | 4; it.kind = 0x100BC; pk = KomonoLogic::itemCtrlPost(it, life, mlife, fakeRand); CHECK(pk.player == 1 && pk.heal && !pk.giveItem && pk.sound == 0x2001A);
    it = {}; it.flags = 8 | 2; it.kind = 0x100BC; pk = KomonoLogic::itemCtrlPost(it, life, mlife, fakeRand); CHECK(pk.player == 0 && !pk.heal && pk.giveItem);
    it = {}; it.flags = 8 | 2; it.itemNo = 3; pk = KomonoLogic::itemCtrlPost(it, life, mlife, fakeRand); CHECK(pk.sound == 0x2002E && !pk.showGetDisp);
    it = {}; it.flags = 2; pk = KomonoLogic::itemCtrlPost(it, life, mlife, fakeRand); CHECK(pk.player == -1);

    // ---- trnmod ----
    CHECK(kTrnSeconds[0][0] == 15 && kTrnSeconds[2][1] == 11 && kTrnSeconds[3][2] == 35 && kTrnSeconds[11][0] == 30 && kTrnSeconds[1][0] == -1);
    TrnGlobals tg; tg.level[0] = 2; TrnTask t0; std::vector<TrnEvent> ev;
    TrainingLogic::init01(t0, tg); CHECK(t0.c9c == 750 && tg.norm == 17 && t0.cA0 == 0x21);
    tg.level[0] = 0; TrainingLogic::init01(t0, tg); CHECK(t0.cA0 == 0x25 && tg.norm == 13);
    // trn_01: genera cada cA0+1 fotogramas mientras queden >= 26 fotogramas
    t0.c98 = t0.cA0; TrainingLogic::tick01(t0, tg, ev); CHECK(ev.size() == 1 && ev[0].a == 1 && t0.c98 == 0 && t0.c9c == 749);
    tg.paused = 1; int c = t0.c9c; TrainingLogic::tick01(t0, tg, ev); CHECK(t0.c9c == c); tg.paused = 0;
    tg.norm = 0; TrainingLogic::tick01(t0, tg, ev); CHECK(tg.missionOver == 1); tg.missionOver = 0;
    // trn_02
    TrnTask t2; t2.mission = 1; tg.level[1] = 1; tg.levelGp72C8 = 2; seq({1, 3, 6, 7});
    TrainingLogic::init02(t2, tg, fakeRand); CHECK(tg.norm == 16 && t2.c9c == 16 && t2.cA0 == 0x29 && tg.targets[0] == 1 + 2 && tg.targets[1] == 3 + 5 && tg.targets[2] == 6 % 4 + 9 && tg.targets[3] == 7 % 4 + 0xD);
    ev.clear(); t2.c98 = t2.cA0 - 1; tg.targets[0] = 1; TrainingLogic::tick02(t2, tg, ev);
    CHECK(ev.size() == 1 && ev[0].a == 0x17 && ev[0].b == 1 && t2.cA8 == 1 && t2.c9c == 15 && t2.cA4 == 1);
    // trn_03
    TrnTask t3; t3.mission = 2; tg.level[2] = 1; ev.clear(); TrainingLogic::init03(t3, tg, ev);
    CHECK(t3.c9c == 11 * 50 && tg.norm == 65 && t3.cA0 == 0x3C && ev.size() == 1 && ev[0].a == 3 && ev[0].c == 65);
    tg.norm = 0; ev.clear(); TrainingLogic::tick03(t3, tg, ev); CHECK(t3.cA4 == 1 && tg.flagE37 == 1 && ev.size() == 2 && ev[1].a == 0x10 && t3.cA0 == 0x3B && tg.missionOver == 0);
    t3.cA0 = 1; TrainingLogic::tick03(t3, tg, ev); CHECK(tg.missionOver == 1); tg.missionOver = 0;
    // trn_04 / 05 / 12
    TrnTask t4; t4.mission = 3; tg.level[3] = 2; TrainingLogic::init04(t4, tg); CHECK(t4.c9c == 35 * 50 && tg.norm == 85);
    tg.countDF8 = 0; TrainingLogic::tick04(t4, tg); CHECK(tg.missionOver == 1 && tg.flagE34 == 1); tg.missionOver = tg.flagE34 = 0;
    TrnTask t5; t5.mission = 4; TrainingLogic::init05(t5, tg, 0); CHECK(tg.norm == 9 && t5.c9c == 750); TrainingLogic::init05(t5, tg, 2); CHECK(tg.norm == 0xB);
    TrainingLogic::tick05(t5, tg); CHECK(tg.mgrFlag6F58 == 1 && t5.c9c == 749 && tg.missionOver == 0);
    TrnTask t12; t12.mission = 11; TrainingLogic::init12(t12, tg); CHECK(t12.c9c == 1500);
    t12.c9c = 1; ev.clear(); TrainingLogic::tick12(t12, tg, ev); CHECK(tg.missionOver == 1 && ev.size() == 1 && ev[0].a == 0x1D0012); tg.missionOver = 0;
    // trn_06 pulso
    TrnTask t6; t6.mission = 5; TrainingLogic::init06(t6, tg, 1); CHECK(t6.c98 == 0x2A6 && t6.f84[0] == 1.0f && t6.f84[1] > 0.79f && t6.f84[1] < 0.81f);
    tg.norm = 1; TrainingLogic::tick06(t6, tg); CHECK(t6.dir[0] == 1 && t6.f84[0] == 1.0f);
    TrainingLogic::tick06(t6, tg); CHECK(t6.f84[0] < 1.0f);
    // trn_07 / 11
    TrnTask t7; t7.mission = 6; tg.level[6] = 1; TrainingLogic::init07(t7, tg); CHECK(t7.c98 == 0x2AA && t7.f80 == 1020.0f && tg.norm == 0x12 && t7.cA8 == 1);
    ev.clear(); TrainingLogic::tick07(t7, tg, 67.574f + 33.787f * 3 + 1.0f, ev); CHECK(t7.c9c == 3 && ev.size() == 1 && ev[0].z > 196.9f && ev[0].z < 197.1f);
    t7.f7c = 1019.0f; TrainingLogic::tick07(t7, tg, 0, ev); CHECK(t7.f7c == 1020.0f && tg.missionOver == 1); tg.missionOver = 0;
    TrnTask t11; t11.mission = 10; tg.level[10] = 0; TrainingLogic::init11(t11, tg); CHECK(t11.cA0 == 750 && t11.f80 == 1112.0f && t11.c98 == 0x2B3);
    TrainingLogic::tick11(t11, tg, -401.0f); CHECK(t11.c9c == 2 && t11.cA0 == 749);
    // trn_09
    TrnTask t9; t9.mission = 8; tg.level[8] = 1; TrainingLogic::init09(t9, tg); CHECK(t9.c9c == 750 && tg.norm == 13);
    ev.clear(); t9.c98 = 0x1A; tg.enemyAlive = 0; seq({0, 2, 5, 1}); TrainingLogic::tick09(t9, tg, fakeRand, ev);
    CHECK(ev.size() == 1 && ev[0].x == 1.0f * 8 && ev[0].z == -390.0f + (-1) * 5 && t9.c98 == 0);
    // boss, murcielagos, tiempo, palabras
    CHECK(TrainingLogic::bossOver(4, 9) && TrainingLogic::bossOver(6, 6) && !TrainingLogic::bossOver(4, 5) && !TrainingLogic::bossOver(3, 3));
    CHECK(TrainingLogic::decideBatNum(2) == 45 && TrainingLogic::decideBatNum(1) == 30 && TrainingLogic::decideBatNum(0) == 15);
    CHECK(TrainingLogic::timeChange(0) == 0 && TrainingLogic::timeChange(51) == 101 && TrainingLogic::timeChange(54) == 108 && TrainingLogic::timeChange(499) == 9 * 100 + 9 * 10 + 8);
    CHECK(TrainingLogic::timeChange(500) == 1000 && TrainingLogic::timeChange(3000) == 10000 + 100 * 1 && TrainingLogic::timeChange(30000) == 11000);
    CHECK(TrainingLogic::moji2obj('A') == 0x6C000A && TrainingLogic::moji2obj('Z') == 0x6C0041 && TrainingLogic::moji2obj('1') == 0x6C002C);
    uint32_t w1[3] = {TrainingLogic::moji2obj('A'), TrainingLogic::moji2obj('S'), TrainingLogic::moji2obj('S')}; CHECK(TrainingLogic::checkBadWord(w1, 3));
    uint32_t w2[3] = {TrainingLogic::moji2obj('A'), TrainingLogic::moji2obj('M'), TrainingLogic::moji2obj('O')}; CHECK(!TrainingLogic::checkBadWord(w2, 3));
    uint32_t w3[2] = {TrainingLogic::moji2obj('F'), TrainingLogic::moji2obj('U')}; CHECK(TrainingLogic::checkBadWord(w3, 2));
    CHECK(std::strcmp(kTrnBadWords[0], "FUC") == 0 && std::strcmp(kTrnBadWords[49], "IRA") == 0);
    CHECK(TrainingLogic::free1(15).size() == 6 && TrainingLogic::free1(12).size() == 2 && TrainingLogic::free1(3).empty());
    BatState bat; bat.life = 3; tg.enemyAlive = 5; tg.enemyAlvNum = 5; ev.clear();
    BatLogic::chkMode(bat, tg, fakeRandF, ev); CHECK(bat.mode == 1);
    bat.life = 0; BatLogic::chkMode(bat, tg, fakeRandF, ev); CHECK(bat.mode == 2 && tg.enemyAlive == 4 && tg.enemyAlvNum == 4 && ev.size() == 1 && ev[0].a == 0x13001B);
    CHECK(bat.fallVel[0] == 0.0f && bat.fallVel[1] == 2.5f);
    bat.y = -1; BatLogic::chkMode(bat, tg, fakeRandF, ev); CHECK(bat.mode == 3 && tg.enemyAlive == 4);
    bat.fallVel[1] = 0; BatLogic::fallDown(bat); CHECK(bat.fallVel[1] < -0.0816f && bat.fallVel[1] > -0.0817f);
    bat.rotA = 0; bat.rotB = 100; BatLogic::rotPos(bat); CHECK(bat.rotA == 1.0f);
    Trn10Komono kk; seq({0x7FF, 100}); Trn10Komono::setMode(kk, 1, fakeRand); CHECK(kk.rotVZ == 0x3FF - 0x200 && kk.rotVX == 100 - 0x400);
    kk.y = 5; kk.vy = 1; Trn10Komono::ctrl2(kk, false); CHECK(kk.y == 6 && kk.vy < 0.93f); kk.y = -20; kk.vy = 1; Trn10Komono::ctrl2(kk, false); CHECK(kk.y == -20);
    kk = Trn10Komono(); kk.hitPl = 0; kk.x = 10; kk.rotZ = 0x0FF0; seq({1000}); Trn10Komono::hitMove(kk, 20.0f, fakeRand, fakeRandF); CHECK(kk.rotZ == 0x1000 && kk.offX < -0.19f && kk.offX > -0.21f && kk.offZ < -0.19f);

    // ---- acmod ----
    CHECK(TitleLogic::getMainMenuNo(-0x6F, 0x41) == 0 && TitleLogic::getMainMenuNo(0x6F, 100) == 2 && TitleLogic::getMainMenuNo(0, 0x41) == 1 && TitleLogic::getMainMenuNo(-0x6E, 100) == -1);
    CHECK(TitleLogic::getMainMenuNo(-200, 0) == 3 && TitleLogic::getMainMenuNo(200, 0xF) == 5 && TitleLogic::getMainMenuNo(0, -0x80) == 4 && TitleLogic::getMainMenuNo(0, -0x81) == -1 && TitleLogic::getMainMenuNo(0, 0x10) == -1);
    DemoMgr dm; DemoGlobals dg; dg.gameState = 1; TitleLogic::setStageNo(dm, dg); CHECK(dm.started == 0);
    dg.gameState = 9; dm.index = 1; TitleLogic::setStageNo(dm, dg); CHECK(dm.index == 0 && dg.stageGp73D8 == 1 && dg.valGp73D4 == 20 && dm.v0 == 3584 && dm.started == 1);
    CHECK(TitleLogic::getStartPtid(dm) == 11 && TitleLogic::getDemoNo(dm) == 2);
    CHECK(TitleLogic::decideSound(0) == 0x2000C && TitleLogic::decideSound(1) == 0x2000D);
    int st[2] = {0, 3}; CHECK(TitleLogic::checkConnectGuncon(st)); st[1] = 1; CHECK(!TitleLogic::checkConnectGuncon(st));
    int ox, oy; CHECK(TitleLogic::getSightPos(1, -10, 20, false, ox, oy) && ox == 0x136 && oy == 0xE0 - 20);
    CHECK(!TitleLogic::getSightPos(2, 0, 0, false, ox, oy) && ox == 0 && TitleLogic::getSightPos(3, 0, 0, true, ox, oy));
    CHECK(TitleLogic::bulletRingStep(3) == 2 && TitleLogic::bulletRingStep(0) == 8);
    int tm = 5, fl = 1; TitleLogic::demoCtrlLoop(tm, fl); CHECK(fl == 1); TitleLogic::demoCtrlLoop(tm, fl); CHECK(fl == 0);

    if (fails == 0) printf("stages_test: TODO OK\n"); else printf("stages_test: %d fallos\n", fails);
    return fails != 0;
}

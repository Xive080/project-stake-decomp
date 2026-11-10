#include "org.h"
namespace vn {

float OrgLogic::weaponDamage(const StageSys& s, int pl) {
    int8_t it = s.equip[pl][0];
    float v = (unsigned)it < 16 ? kOrgWeaponDamage[it] : 1.0f;   // el original indexa sin acotar; los ids validos son 0..15
    if (it == 8 && s.pl[pl].flag346 == 0) v = 0.5f; // objeto 8 sin la marca +0x346: mitad de dano
    return v;
}

static const float kTwoThirds = 0.666599989f;   // constante 0x3F2AA64C del asm
float OrgLogic::enemyLifeScale(const StageSys& s) {
    float v = 1.0f;
    for (int pl = 0; pl < 2; pl++) {
        int m = s.pl[pl].mode;
        if (!(m == 0 || m == 1 || m == 3 || m == 4 || m == 5)) continue;
        for (int slot = 2; slot < 4; slot++) {
            int8_t it = s.equip[pl][slot];
            if (it == 0x26) { if (!(v < kTwoThirds)) v = kTwoThirds; }
            else if (it == 0x27) { if (!(v < 0.5f)) v = 0.5f; }
        }
    }
    return v;
}

bool OrgLogic::addItem(StageSys& s, int id) {
    if (id > 0 && id < 0x35) {
        int v = s.invCount[id] + 1;
        if (v < 0x64) s.invCount[id] = (int8_t)v;
        return true;
    }
    return false;
}

void OrgLogic::shopLevel(StageSys& s, int level) {
    for (int i = 1; i < 0x35; i++)
        if (kOrgShopLevel[i] == level) s.shopOpen[i] = 1;
    s.shopDirty = 1;
}

SilverEvent OrgLogic::addSilver(StageSys& s, int delta, bool displayExists) {
    SilverEvent ev;
    int32_t nv = s.silver + delta;
    s.silverTotal += delta;
    if (s.gameState != 0xA || s.gameKind != 1) return ev;
    if (nv > 999999) nv = 999999;
    ev.from = s.silver; ev.to = nv;
    if (displayExists) ev.recharge = true; else ev.create = true;
    s.silver = nv;
    return ev;
}

void OrgLogic::missionComplete(StageSys& s, int n) {
    if (n < 0 || n >= 0x28) return;
    s.missionFlags[n] &= ~0x02;
    s.missionFlags[n] |= 0x14;
    // Banderas fijas: el original escribe en 0x502720 / 0x502727 / 0x502728 (= misiones 24, 31, 32 de la tabla base 0x502708)
    if (n == 14) s.missionFlags[24] &= ~0x25;
    else if (n == 24) s.missionFlags[31] &= ~0x25;
    else if (n == 27) s.missionFlags[32] &= ~0x25;
}

static const int8_t* info(int item) { static const int8_t none[2] = {0, 0}; return (unsigned)item < 0x35 ? kItemInfo[item] : none; }
static bool flagsOpen(int8_t f) { return (f & 1) && (f & 2) && !(f & 0x20); }

int OrgLogic::missionOver(const StageSys& s) {
    if (s.gameKind != 1) return 0;
    int best = 0;
    for (int i = 0; i < 2; i++) if (best < s.pl[i].score) best = s.pl[i].score;
    bool arcade = s.gameState == 0xA;
    if (arcade && flagsOpen(s.missionFlags[14])) return best >= 0x7530 ? 1 : 0;
    if (arcade && flagsOpen(s.missionFlags[24])) return s.silverTotal >= 0x3E8 ? 2 : 0;
    if (arcade && flagsOpen(s.missionFlags[31]) && best >= 0x222E0) return 3;
    return 0;
}

// Modos del otro jugador en los que el original "actua": 7, 9 y > 10 (los modos 0..6, 8 y 10 saltan por la tabla jtbl 0xA12BD0/0xA12C00/0xA12C30).
static bool modeBlocks(int mode) { return (unsigned)mode <= 10 && mode != 7 && mode != 9; }

int OrgLogic::anotherPlayerHas(const StageSys& s, int pl, int item) {
    int o = 1 - pl;
    if (modeBlocks(s.pl[o].mode)) return 0;
    int n = 0;
    for (int j = 0; j < 4; j++) if (s.equip[o][j] == item) n++;
    return n;
}

void OrgLogic::unequipAnotherPlayer(StageSys& s, int pl, int item) {
    int o = 1 - pl;
    if (modeBlocks(s.pl[o].mode)) return;
    for (int j = 0; j < 4; j++) if (s.equip[o][j] == item) { s.equip[o][j] = 0; return; }
}

void OrgLogic::unequipJoinIn(StageSys& s, int pl) {
    int o = 1 - pl;
    if (!modeBlocks(s.pl[o].mode)) return;            // polaridad inversa: solo actua si el otro esta en 0..6, 8 o 10
    for (int j = 0; j < 4; j++) {
        int8_t item = s.equip[pl][j];
        if (info(item)[1] != 2) continue;   // solo tipo 2
        for (int k = 0; k < 4; k++) {
            int8_t oi = s.equip[o][k];
            if (info(item)[0] == info(oi)[0]) {
                addItem(s, item);
                s.equip[pl][j] = 0;
                break;
            }
        }
    }
}

bool OrgLogic::setDispScoreJoinIn(int stage, int slot1) { return stage != 7 && slot1 == 0x12; }
}

#include "options.h"
namespace vn {
// ReadCfgGameBuffer 0x1D3720: live <- saved; WriteCfgGameBuffer 0x1D37A0: saved <- live. Tamanos 0xC0, 0x85, 0x11, 0x66.
const CfgCopy kCfgCopy[4] = {{0x502690, 0x50293C, 0xC0}, {0x502750, 0x502A74, 0x85}, {0x5027E0, 0x5029FC, 0x11}, {0x502820, 0x502A0E, 0x66}};

SightScreenPos OptionsLogic::getSightPos(const GunSlot& s, int device, int menuMode) {
    SightScreenPos r{0, 0, false};
    if (device == 2 || device == 3) {
        if (s.sightValid != 0) { r.x = s.sightX + 0x140; r.y = 0xE0 - s.sightY; }
    } else if (device == 1) {
        r.x = s.sightX + 0x140; r.y = 0xE0 - s.sightY;
        if (menuMode != 0x17 && menuMode != 0x16 && menuMode != 0xB && menuMode != 0xA && menuMode != 9) r.drawSight = true;
    }
    return r;
}

const int8_t kDifficultyValue[5] = {0, 6, 8, 10, 16};
const int8_t kDifficultyMenu[17] = {0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 4};
const int kDifficultyChoice[6] = {0, 1, 3, 0, 2, 4};
const uint8_t kBgmLevel[11] = {0x00, 0x1A, 0x33, 0x4D, 0x66, 0x80, 0x99, 0xB3, 0xCC, 0xE6, 0xFF};
const uint8_t kVoiceLevel[11] = {0x00, 0x17, 0x2E, 0x45, 0x5C, 0x73, 0x8A, 0xA1, 0xB8, 0xCF, 0xE6};

// Primer indice cuyo umbral >= v; si v supera todos, 10.
static int levelIndex(const uint8_t* t, uint8_t v) {
    for (int i = 0; i < 10; i++) if (!(t[i] < v)) return i;
    return 10;
}
int OptionsLogic::calcBgmBalance(uint8_t v) { return levelIndex(kBgmLevel, v); }
int OptionsLogic::calcVoiceBalance(uint8_t v) { return levelIndex(kVoiceLevel, v); }

// Remapea los bits de la configuracion de botones a los bits del mando.
int OptionsLogic::chgConfigBitR(int b) {
    int r = 0;
    if (b & 0x01) r |= 0x2000;
    if (b & 0x02) r |= 0x8000;
    if (b & 0x10) r |= 0x800;
    if (b & 0x04) r |= 0x400;
    if (b & 0x20) r |= 0x200;
    if (b & 0x08) r |= 0x100;
    return r;
}

void OptionsLogic::setVibrationSysCfg(SysCfg& s, const CsOptData& m) {
    s.vibration1 = (m.vib1 == 1) ? 1 : 0;
    s.vibration2 = (m.vib2 == 1) ? 1 : 0;
}

void OptionsLogic::sysCfgToMenu(CsOptData& m, const SysCfg& s) {
    m.diff = kDifficultyMenu[(unsigned)s.difficulty < 17 ? s.difficulty : 0];   // el original indexa sin limite
    m.initLife = s.initLife;
    m.maxLife = s.maxLife;
    m.credits = (s.credits == 0) ? 10 : s.credits;
    m.vib1 = (s.vibration1 == 1) ? 1 : 0;
    m.vib2 = (s.vibration2 == 1) ? 1 : 0;
    if (s.sightType == 0) m.sight = 0; else if (s.sightType == 1) m.sight = 1;   // otros valores: no se toca
    m.bgm = (int8_t)calcBgmBalance(s.bgmVol);
    m.voice = (int8_t)calcVoiceBalance(s.voiceVol);
    m.voice2 = (int8_t)calcVoiceBalance(s.voiceVol2);
    m.c = 2;
    if (s.padCfgA != 0) { m.padA0 = 4; m.padA1 = 1; } else { m.padA0 = 1; m.padA1 = 4; }
    m.f = 2;
    if (s.padCfgB != 0) { m.padB0 = 4; m.padB1 = 1; } else { m.padB0 = 1; m.padB1 = 4; }
}

void OptionsLogic::gamesystemDefault(CsOptData& m, SysCfg& s) {
    m.initLife = 3; m.maxLife = 5; m.credits = 5;
    m.diff = 2;                                  // D_0093DB28[0]
    s.initLife = m.initLife; s.maxLife = m.maxLife;
    s.credits = (int8_t)(m.credits % 10);
    s.difficulty = kDifficultyValue[m.diff];
}
void OptionsLogic::lifeDefault(CsOptData& m, SysCfg& s) {
    m.initLife = 3; m.maxLife = 5;
    s.initLife = m.initLife; s.maxLife = m.maxLife;
}

void OptionsLogic::difficultyChoose(CsOptData& m, SysCfg& s, int choice) {
    if (choice < 1 || choice > 5) return;
    m.diff = (int8_t)kDifficultyChoice[choice];
    s.difficulty = kDifficultyValue[m.diff];
}

LifeResult OptionsLogic::lifeAdjust(CsOptData& m, SysCfg& s, int action) {
    LifeResult r{false, -1};
    switch (action) {
    case LIFE_INIT_DEC:
        if (m.initLife >= 2) { m.initLife--; s.initLife = m.initLife; r.changed = true; }
        r.warnTimer = 0; break;
    case LIFE_INIT_INC:
        if (m.initLife < 9) {
            if (m.initLife < m.maxLife) { m.initLife++; s.initLife = m.initLife; r.changed = true; }
            else r.warnTimer = 0x78;               // no puede superar las vidas maximas
        }
        break;
    case LIFE_MAX_DEC:
        if (m.maxLife >= 2) {
            m.maxLife--; s.maxLife = m.maxLife; r.changed = true;
            if (m.initLife > m.maxLife) { m.initLife = m.maxLife; s.initLife = m.initLife; }
        }
        r.warnTimer = 0; break;
    case LIFE_MAX_INC:
        if (m.maxLife < 9) { m.maxLife++; s.maxLife = m.maxLife; r.changed = true; }
        r.warnTimer = 0; break;
    }
    return r;
}

// 1..10 creditos; 10 solo si el indicador de desbloqueo (0x502928) vale 1. SysCfg guarda credits % 10 (0 = juego libre).
void OptionsLogic::creditsAdjust(CsOptData& m, SysCfg& s, bool inc) {
    if (inc) {
        if (m.credits < 10) {
            m.credits++;
            if (m.credits == 10 && s.unlockFlag != 1) m.credits--;
            s.credits = (int8_t)(m.credits % 10);
        }
    } else if (m.credits >= 2) {
        m.credits--;
        s.credits = m.credits;
    }
}

// Volumenes 0..10 -> byte de SysCfg por tabla. which: 0 = BGM (0x502920), 1 = voz (0x502921), 2 = voz 2 (0x502922).
void OptionsLogic::balanceAdjust(CsOptData& m, SysCfg& s, int which, bool inc) {
    int8_t* v = which == 0 ? &m.bgm : which == 1 ? &m.voice : &m.voice2;
    uint8_t* d = which == 0 ? &s.bgmVol : which == 1 ? &s.voiceVol : &s.voiceVol2;
    const uint8_t* t = which == 0 ? kBgmLevel : kVoiceLevel;
    if (inc) { if (*v >= 10) return; (*v)++; }
    else { if (*v <= 0) return; (*v)--; }
    *d = t[*v];
}

void OptionsLogic::sightSpeedAdjust(SysCfg& s, int8_t& menuVal, bool inc) {
    if (inc) { if (menuVal < 10) menuVal++; }
    else if (menuVal > 0) menuVal--;
    s.sightSpeed = menuVal;
}

bool OptionsLogic::checkSysCfgBak(const uint32_t* bak, const uint32_t* cur) {
    for (int i = 0; i < 0xBD; i++) if (bak[i] != cur[i]) return false;
    return true;
}
void OptionsLogic::copySysCfgBak(uint32_t* bak, const uint32_t* cur) { for (int i = 0; i < 0xBD; i++) bak[i] = cur[i]; }
}

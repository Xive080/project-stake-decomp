#include "training.h"
#include <cstdint>
namespace vn {

// Constantes float del asm (patrones de bits).
static float bitsF(uint32_t b) { float f; std::memcpy(&f, &b, 4); return f; }

void TrainingLogic::init01(TrnTask& t, TrnGlobals& g) {            // trn_01_init
    int lv = g.level[t.mission];
    t.c98 = 0; t.c9c = frames(t.mission, lv);
    g.norm = kTrnNormTrn01[lv];
    // gp-0x72BC / gp-0x72B8 = 0 (marcadores de HUD, no usados por las reglas)
    if (lv == 2) t.cA0 = 0x21; else if (lv == 1 || lv == 0) t.cA0 = 0x25;
}

void TrainingLogic::init02(TrnTask& t, TrnGlobals& g, RandFn rand) {   // trn_02_init
    int lv = g.level[t.mission];
    g.norm = kTrnNormTrn02[lv];
    t.c9c = g.norm; t.c98 = 0; t.cA4 = 0; t.cA8 = 0;
    int l2 = g.levelGp72C8;
    if (l2 == 2) t.cA0 = 0x29; else if (l2 == 1) t.cA0 = 0x2D; else if (l2 == 0) t.cA0 = 0x36;
    g.targets[0] = rand() % 3 + 2;            // gp-0x6F68
    g.targets[1] = (rand() % 4) + 5;          // gp-0x6F67 (el original usa & 3 con correccion de signo = % 4)
    g.targets[2] = (rand() % 4) + 9;          // gp-0x6F66
    g.targets[3] = rand() % 4 + 0xD;          // gp-0x6F65
}

void TrainingLogic::init03(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev) {   // trn_03_init
    int lv = g.level[t.mission];
    t.c9c = frames(t.mission, lv);
    g.norm = kTrnNormTrn03_04[lv];
    t.cA0 = 0x3C; t.cA4 = 0;
    TrnEvent e{TrnEvent::SPAWN_GENERAL}; e.a = 3; e.b = 0x3C; e.c = g.norm;
    e.x = 0.0f; e.y = bitsF(0x41320DC2); e.z = bitsF(0xC2BC0000);
    ev.push_back(e);
}

void TrainingLogic::init04(TrnTask& t, TrnGlobals& g) {              // trn_04_init
    int lv = g.level[t.mission];
    t.c9c = frames(t.mission, lv); g.norm = kTrnNormTrn03_04[lv];
}

void TrainingLogic::init05(TrnTask& t, TrnGlobals& g, int lv804) {   // trn_05_init (sin crear el gestor Trn05KomonoMgrTask)
    g.mgrFlag6F58 = 0;
    t.c9c = frames(t.mission, g.level[t.mission]);
    g.norm = (lv804 == 0) ? 9 : 0xB;
    g.mgr6F5C = 0; g.mgr6F60 = 0;
}

void TrainingLogic::init06(TrnTask& t, TrnGlobals& g, int sel) {      // trn_06_init
    t.f7c = 0; t.f80 = bitsF(0x42F00000);
    t.c9c = frames(t.mission, g.level[t.mission]);
    t.c98 = (sel == 0) ? 0x2A5 : (sel == 1 ? 0x2A6 : 0x2A7);
    t.f84[0] = 1.0f; t.f84[1] = bitsF(0x3F4CCCCD); t.f84[2] = bitsF(0x3F19999A); t.f84[3] = bitsF(0x3ECCCCCC);
    for (int i = 0; i < 4; i++) t.dir[i] = 0;
}

void TrainingLogic::init07(TrnTask& t, TrnGlobals& g) {              // trn_07_init
    int lv = g.level[t.mission];
    if (lv == 0) { t.c98 = 0x2AB; t.f80 = bitsF(0x44870000); }
    else if (lv == 1) { t.c98 = 0x2AA; t.f80 = bitsF(0x447F0000); }
    else if (lv == 2) { t.c98 = 0x2A8; t.f80 = bitsF(0x44610000); }
    t.f7c = 0; t.c9c = 0; t.cA4 = 0; t.cA8 = 1;
    g.norm = 0x12;
}

void TrainingLogic::init09(TrnTask& t, TrnGlobals& g) {              // trn_09_init
    int lv = g.level[t.mission];
    t.c98 = 0; t.c9c = frames(t.mission, lv); g.norm = kTrnNormTrn09[lv];
}

void TrainingLogic::init11(TrnTask& t, TrnGlobals& g) {              // trn_11_init (el tiempo va en +0xA0)
    t.c98 = 0x2B3; t.f7c = bitsF(0x42F00000); t.f80 = bitsF(0x448B0000);
    t.cA0 = frames(t.mission, g.level[t.mission]);
}

void TrainingLogic::init12(TrnTask& t, TrnGlobals& g) {              // trn_12_init
    t.c9c = frames(t.mission, g.level[t.mission]);
}

void TrainingLogic::tick01(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev) {   // trn_01
    if (!g.paused && t.c9c >= 0x1A) {
        int old = t.c98++;
        if (old >= t.cA0) { t.c98 = 0; TrnEvent e{TrnEvent::SPAWN_GENERAL}; e.a = 1; ev.push_back(e); }
    }
    if (!g.paused) t.c9c--;
    if (!(t.c9c > 0 && g.norm > 0)) g.missionOver = 1;
}

void TrainingLogic::tick02(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev) {   // trn_02
    if (!g.paused) t.c98++;
    if (t.c98 >= t.cA0) {
        t.c98 = 0;
        if (t.c9c > 0) {
            t.cA4++; t.c9c--;
            TrnEvent e{TrnEvent::SPAWN_GENERAL}; e.a = 0x17; e.b = 0;
            if (t.cA4 == g.targets[t.cA8]) { e.b = 1; t.cA8++; if (t.cA8 >= 4) t.cA8 = 3; }
            ev.push_back(e);
        }
    }
    if (g.norm <= 0) g.missionOver = 1;
}

void TrainingLogic::tick03(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev) {   // trn_03
    if (g.norm > 0 && !g.paused) t.c9c--;
    if (t.cA4 == 0 && g.norm <= 0) {
        TrnEvent fx{TrnEvent::EFFECT}; fx.a = 0x31; ev.push_back(fx);
        TrnEvent snd{TrnEvent::SOUND}; snd.a = 0x10; ev.push_back(snd);
        g.flagE37 = 1; t.cA4 = 1;
    }
    if (t.cA4 == 1 || t.c9c <= 0) {
        if (!g.paused) t.cA0--;
        if (!(t.cA0 > 0 && t.c9c > 0)) g.missionOver = 1;
    }
}

void TrainingLogic::tick04(TrnTask& t, TrnGlobals& g) {              // trn_04
    if (!g.paused) t.c9c--;
    if (!(t.c9c > 0 && g.countDF8 != 0)) { g.missionOver = 1; g.flagE34 = 1; }
}

void TrainingLogic::tick05(TrnTask& t, TrnGlobals& g) {              // trn_05
    g.mgrFlag6F58 = 1;
    if (!g.paused) t.c9c--;
    if (!(t.c9c > 0 && g.norm > 0)) g.missionOver = 1;
}

void TrainingLogic::tick06(TrnTask& t, TrnGlobals& g) {              // trn_06 (pulso de las 4 escalas y fin)
    if (!g.paused) {
        for (int i = 0; i < 4; i++) {
            if (t.dir[i] == 0) { t.f84[i] += 0.060000002f; if (!(t.f84[i] <= 1.0f)) { t.f84[i] = 1.0f; t.dir[i] = 1; } }
            else { t.f84[i] -= 0.060000002f; if (t.f84[i] < 0.8f) { t.f84[i] = 0.8f; t.dir[i] = 0; } }
        }
        if (g.norm == 0) g.missionOver = 1;
        t.c9c--;
    }
    if (!(t.c9c > 0 && g.norm > 0)) g.missionOver = 1;
}

void TrainingLogic::tick07(TrnTask& t, TrnGlobals& g, float camY, std::vector<TrnEvent>& ev) {   // trn_07
    t.c9c = (int)((camY - 67.574f) / 33.787f);          // fptosi
    if (t.c9c < 0) t.c9c = 0;
    if (t.c9c != t.cA4 && t.c9c < 0x12) {
        TrnEvent e{TrnEvent::SPAWN_ENEMY}; e.a = t.c9c;
        e.z = 95.661f + 33.787f * (float)t.c9c;           // 0x9F582C
        ev.push_back(e);
        t.cA4 = t.c9c;
    }
    if (!g.paused) {
        t.f7c += 1.2f;
        if (!(t.f7c < t.f80)) { t.f7c = t.f80; g.missionOver = 1; }
    }
}

void TrainingLogic::tick08(TrnTask& t, TrnGlobals& g) {              // trn_08
    g.mgrFlag6F54 = 1;
    if (!g.paused) t.c9c--;
    if (t.c9c <= 0) g.missionOver = 1;
}

void TrainingLogic::tick09(TrnTask& t, TrnGlobals& g, RandFn rand, std::vector<TrnEvent>& ev) {   // trn_09
    if (!g.paused && t.c9c >= 0x1A) {
        int old = t.c98++;
        if (old >= 0x1A) {
            t.c98 = 0;
            if (g.enemyAlive < 4) {
                TrnEvent e{TrnEvent::SPAWN_ENEMY};
                int i1 = rand() % 10; int m1 = (rand() % 5) * 4;
                e.x = (float)(kTrn09Table[i1] * m1); e.y = bitsF(0xC0AE00D2);
                int i2 = rand() % 10; int m2 = (rand() % 3) * 5;
                e.z = -390.0f + (float)(kTrn09Table[i2] * m2);
                ev.push_back(e);
            }
        }
    }
    if (!g.paused) t.c9c--;
    if (!(t.c9c > 0 && g.norm > 0)) g.missionOver = 1;
}

void TrainingLogic::tick11(TrnTask& t, TrnGlobals& g, float camY) {  // trn_11
    t.c9c = (int)(camY / -200.0f);
    if (!g.paused) {
        t.f7c += 1.2f;
        if (!(t.f7c < t.f80)) { t.f7c = t.f80; g.missionOver = 1; }
        t.cA0--;
        if (t.cA0 <= 0) g.missionOver = 1;
    }
}

void TrainingLogic::tick12(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev) {   // trn_12
    if (!g.paused) t.c9c--;
    if (t.c9c <= 0) { g.missionOver = 1; TrnEvent s{TrnEvent::SOUND}; s.a = 0x1D0012; ev.push_back(s); }
}

bool TrainingLogic::bossOver(int a, int b) {
    static const int endMode[12] = {0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0};     // ELF 0x27496C, primer int de cada fila de 0x1C B
    auto bad = [](int m) { return m >= 0 && m < 12 && endMode[m] != 0; };
    return bad(a) && bad(b);
}

int TrainingLogic::decideBatNum(int lv) { return lv == 2 ? 0x2D : (lv == 1 ? 0x1E : 0xF); }

int TrainingLogic::timeChange(uint32_t f) {
    int v0 = 0, v1 = 0, a1 = 0, a2 = 0, a3 = 0;
    int c = f >= 0x7530u ? 4 : (f >= 0xBB8u ? 3 : (f >= 0x1F4u ? 2 : 1));
    switch (c) {
    case 1: a1 = f / 50; a2 = (f % 50) / 5; a3 = kTrnTimeDigitTbl[(f % 50) % 5]; break;
    case 2: v1 = f / 500; a1 = (f % 500) / 50; a2 = ((f % 500) % 50) / 5; a3 = kTrnTimeDigitTbl[((f % 500) % 50) % 5]; break;
    case 3: v0 = 1; a1 = f / 3000; a2 = (f % 3000) / 500; a3 = ((f % 3000) % 500) / 50; break;
    case 4: v0 = 1; v1 = 1; break;
    }
    return a3 + (a2 * 10 + (a1 * 100 + (v0 * 10000 + v1 * 1000)));
}

uint32_t TrainingLogic::moji2obj(char c) { return (c >= 'A' && c < 'A' + 26) ? kTrnLetterObj[c - 'A'] : 0x6C002Cu; }

bool TrainingLogic::checkBadWord(const uint32_t* obj, int n) {
    for (int w = 0; w < 50; w++) {
        const char* s = kTrnBadWords[w];
        int len = (int)std::strlen(s);
        if (len != n) continue;
        int same = 0;
        for (int i = 0; i < n; i++) if (moji2obj(s[i]) == obj[i]) same++;
        if (same == n) return true;
    }
    return false;
}

std::vector<TrainingLogic::FreeOp> TrainingLogic::free1(int m) {
    // trn_free1 (0x9...): `set` = func_2192D0(&D_5161E0, id); `clear` = func_219650(&D_5161E0, id)
    switch (m) {
    case 15: return {{true, 8}, {true, 0x10}, {false, 6}, {false, 8}, {false, 9}, {false, 10}};
    case 14: return {{true, 7}, {true, 0xF}};
    case 13: return {{true, 6}, {true, 0xE}};
    case 12: return {{true, 5}, {false, 1}};
    case 16: return {{true, 9}, {true, 0x11}};
    }
    return {};
}

// ---- TrainingBat ----
void BatLogic::setFallParam(BatState& b, RandFFn rf) {
    b.fallVel[0] = 3.0f * rf() - 1.5f;
    b.fallVel[1] = 2.0f + rf();
    b.fallVel[2] = 3.0f * rf() - 1.5f;
}
void BatLogic::fallDown(BatState& b) { b.fallVel[1] += -0.08166666f; }
void BatLogic::rotPos(BatState& b) { b.rotA += 0.01f * (b.rotB - b.rotA); b.rotC += b.rotCStep; }

void BatLogic::chkMode(BatState& b, TrnGlobals& g, RandFFn rf, std::vector<TrnEvent>& ev) {
    if (b.mode != 2 && b.mode != 3) {
        if (b.life <= 0) {
            // el original recorre 2 bits de +0x18C y llama al motor por cada uno (func_9331E0)
            b.mode = 2;
            setFallParam(b, rf);
            if (!(b.flags & 4)) { b.flags |= 4; g.enemyAlive--; }          // DecNowEnemyNumChk (gp-0x7374)
            if (!(b.flags & 2)) { b.flags |= 2; g.enemyAlvNum--; }         // DecNowEnemyAlvNumChk (gp-0x7370)
            TrnEvent s{TrnEvent::SOUND}; s.a = 0x13001B; ev.push_back(s);
        }
    }
    if (b.mode == 0) b.mode = 1;
    else if (b.mode == 2 && b.y < 0.0f) b.mode = 3;       // ChgDead
}

// ---- CTrn10Komono ----
void Trn10Komono::setMode(Trn10Komono& k, int mode, RandFn rand) {
    k.mode = mode;
    if (mode == 1) {
        int v = rand() & 0x3FF;                           // el original corrige el signo; con rand >= 0 es & 0x3FF
        k.rotVZ = v - 0x200;
        k.rotVX = (rand() % 2048) - 0x400;
    }
}
void Trn10Komono::ctrl2(Trn10Komono& k, bool paused) {
    if (!paused && !(k.y <= -20.0f)) {
        k.x += k.vx; k.y += k.vy; k.z += k.vz;
        k.vy -= 0.072000004f;
        k.rotZ += k.rotVZ; k.rotX += k.rotVX;
    }
}
void Trn10Komono::hitMove(Trn10Komono& k, float aimX, RandFn rand, RandFFn rf) {
    if (k.hitPl != -1) {
        if (aimX < k.x) { k.rotZ -= rand() % 1024; k.offX += 0.4f * rf(); k.offZ -= 0.4f * rf(); }
        else            { k.rotZ += rand() % 1024; k.offX -= 0.4f * rf(); k.offZ -= 0.4f * rf(); }
    }
    if (k.rotZ > 0x1000) k.rotZ = 0x1000;
    if (k.rotZ < -0x1000) k.rotZ = -0x1000;
    if (!(k.offX <= 3.0f)) k.offX = 3.0f;
    if (k.offX < -3.0f) k.offX = -3.0f;
    if (!(k.offZ <= 3.0f)) k.offZ = 3.0f;
    if (k.offZ < -3.0f) k.offZ = -3.0f;
}
}

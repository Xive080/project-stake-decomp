#include "gunmgr.h"
namespace vn {

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// 0x1EDA10
void GunLogic::init(GunMgr& g) {
    g.enabled = 0;
    for (int i = 0; i < 4; i++) g.gpCalib[i] = 0;
    g.cooldown = 0;     // sb $zero, 0xCD
}

// 0x1EDA30: deja las dos entradas a cero, indices 0 y 1, y activa el gestor.
void GunLogic::start(GunMgr& g) {
    for (int i = 0; i < 2; i++) {
        GunSlot& s = g.slot[i];
        s.rawX = 0; s.rawY = 0; s.rawValid = 0;
        s.sightX = 0; s.sightY = 0; s.sightValid = 0;
        s.trigger = 0; s.hold = 0; s.flash = 0;
        s.index = (int16_t)i;       // +6 y +0x6A
    }
    g.cooldown = 0;                 // +0xCC (el +0xCD no se toca aqui)
    g.enabled = 1;
}

// 0x1EDA90: con mando o sin dispositivo la mira empieza a un lado (-0x50 el jugador 0, +0x50 el 1).
void GunLogic::resetSightPosPl(GunMgr& g, const GunEnv& e, int pl) {
    int d = e.device[pl];
    if (d == 0 || d == 1) {
        g.slot[pl].sightX = (pl == 0) ? -0x50 : 0x50;
        g.slot[pl].sightY = 0;
    }
}
// 0x1EDB40
void GunLogic::resetSightPos(GunMgr& g, const GunEnv& e) { for (int pl = 0; pl < 2; pl++) resetSightPosPl(g, e, pl); }

// 0x1EE4B0: de coordenada cruda del sensor (0..0x27F, 0..0xDF) a mira centrada, con la calibracion del usuario.
// El original solo la llama para pistolas (device 2 o 3); con otro dispositivo usa registros sin inicializar: aqui se descarta la lectura.
void GunLogic::adjustValCz(GunSlot& s, const GunEnv& e) {
    int d = e.device[s.index & 1];
    if (d != 2 && d != 3) { s.sightX = 0; s.sightY = 0; s.sightValid = 0; return; }
    int x, y;
    if (e.gameState == 0x14 || (e.gpCalibFlag != 0)) {      // sin offset de calibracion
        x = (int)s.rawX - 0x140;
        y = 0xE0 - (int)s.rawY;
    } else {
        x = (int)s.rawX - 0x140 - 2 * e.calibX;
        y = 0xE0 - (int)s.rawY + 2 * e.calibY;
    }
    if (x >= -0x13F && x < 0x140 && y >= -0xDF && y < 0xE0) {
        s.sightX = (int16_t)x; s.sightY = (int16_t)y; s.sightValid = 1;
    } else { s.sightX = 0; s.sightY = 0; s.sightValid = 0; }
}

// 0x1EDBA0: mando como pistola. Velocidad = 1 + 0.8 * ((cfg % 11) + 1); zona muerta del stick 0x61..0x9F.
void GunLogic::readSynchPad(GunMgr& g, const GunEnv& e, int pl) {
    GunSlot& s = g.slot[pl];
    int rem = (int)e.speedCfg[pl] % 11;
    float f = 1.0f + 0.8f * (float)(rem + 1);
    auto axis = [](int v) { return v < 0x61 ? v - 0x80 : (v < 0xA0 ? 0 : v - 0x80); };
    int dx = axis(e.stickX[pl]), dy = axis(e.stickY[pl]);
    float fx = ((float)dx * f) / 120.0f;
    float fy = ((float)(-dy) * f) / 120.0f;
    unsigned b = e.buttons[pl];
    if (b & 0xF0) {                          // cruceta anula al stick
        fy = 0.0f; fx = 0.0f;
        if (b & 0x20) fx = f;
        if (b & 0x80) fx = -f;
        if (b & 0x10) fy = f;
        if (b & 0x40) fy = -f;
    }
    if (e.boostMask[pl] & b) { fx *= 2.0f; fy *= 2.0f; }   // boton de aceleracion
    if (e.pauseLevel <= 0) {
        s.sightX = (int16_t)(s.sightX + (int)(1.2f * fx));
        s.sightY = (int16_t)(s.sightY + (int)(1.2f * fy));
    }
    s.sightX = (int16_t)clampi(s.sightX, -0x140, 0x140);
    s.sightY = (int16_t)clampi(s.sightY, -0xE0, 0xE0);
    s.trigger = e.pullTrigger[pl] ? 1 : 0;
    if (!(e.gameState == 7 && e.gp72AC != 0)) {
        if (e.recenterMask[pl] & b) { s.sightX = 0; s.sightY = 0; }
    }
    s.sightValid = 1;
    s.rawValid = 1;
}

// 0x1EDEA0: GunCon. El gatillo llega con 4 fotogramas de retraso (fogonazo).
bool GunLogic::readSynchGun1(GunMgr& g, const GunEnv& e, int pl) {
    GunSlot& s = g.slot[pl];
    s.trigger = 0;
    bool flashReq = false;
    if (e.pullTrigger[pl] && s.flash == 0) { s.flash = 4; flashReq = true; }
    if (s.flash > 0) { s.flash--; if (s.flash == 0) s.trigger = 1; }
    if (e.gc1Status[pl] == 0 || e.gc1Status[pl] == 1) { s.rawX = 0xFFFF; s.rawY = 0xFFFF; }
    else { s.rawX = e.gc1X[pl]; s.rawY = e.gc1Y[pl]; }
    if (s.rawX != 0xFFFF) s.rawValid = 1;
    else s.rawValid = (s.rawY == 0xFFFF) ? 0 : 1;
    if (s.rawValid) adjustValCz(s, e);
    else { s.sightX = 0; s.sightY = 0; s.sightValid = 0; }
    return flashReq;
}

// 0x1EDFF0: GunCon2. El motor ya ha leido el buffer (gun_action).
void GunLogic::readSynchGun2(GunMgr& g, const GunEnv& e, int pl) {
    GunSlot& s = g.slot[pl];
    s.trigger = e.gc2Trigger[pl] != 0 ? 1 : 0;
    s.rawX = (uint16_t)e.gc2X[pl]; s.rawY = (uint16_t)e.gc2Y[pl];
    if (s.rawX != 0xFFFF) s.rawValid = 1;
    else s.rawValid = (s.rawY == 0xFFFF) ? 0 : 1;
    if (s.rawValid) adjustValCz(s, e);
    else { s.sightX = 0; s.sightY = 0; s.sightValid = 0; }
}

static bool shopGunRules(const GunEnv& e) {
    return e.gameState == 0xA && e.gameKind == 1 && !(e.flags72D0 & 0x10);
}

// 0x1EE210: contador de cadencia 0..24 (solo con objetos equipados y gp-0x7418 != 0).
void GunLogic::orgAddTrig0(GunMgr& g, const GunEnv& e) {
    if (!shopGunRules(e) || e.active7418 == 0) return;
    g.cooldown++;
    if (g.cooldown >= 0x19) g.cooldown = 0;
}

// 0x1EE280: armas de ráfaga (objeto de arma 11 y 13): mantener el gatillo repite el disparo.
void GunLogic::orgAddTrig(GunMgr& g, const GunEnv& e, GunOut& o, int pl) {
    GunSlot& s = g.slot[pl];
    if (!shopGunRules(e) || e.active7418 == 0) return;
    if (e.playerMode[pl] == 8) return;
    int item = e.itemSlot0[pl];
    if (item != 0xB && item != 0xD) { s.hold = 0; return; }
    if (s.trigger != 0) { s.hold = 0; return; }
    if (e.pullTriggerNow[pl]) s.hold++; else s.hold = 0;
    switch (e.device[pl]) {
    case 1:   // mando: cada 4 fotogramas mantenido genera un disparo
        if (s.hold == 4) { s.trigger = 1; s.hold = 0; }
        break;
    case 2:   // GunCon: fogonazo y repeticion
        if (e.pullTriggerNow[pl] && g.cooldown == 0 && e.playerBullet[pl] > 0) { o.flash[pl] = true; s.flash = 4; s.hold = 0; }
        break;
    case 3:   // GunCon2: refire forzado por hardware
        if (e.pullTriggerNow[pl] && g.cooldown == 0 && e.playerBullet[pl] > 0) { o.flash[pl] = true; o.forceRefire[pl] = true; s.hold = 0; }
        break;
    default: break;
    }
}

// 0x1EE0D0
GunOut GunLogic::readSynch(GunMgr& g, const GunEnv& e) {
    GunOut o;
    if (!g.enabled) return o;
    orgAddTrig0(g, e);
    for (int pl = 0; pl < 2; pl++) {
        switch (e.device[pl]) {
        case 1: readSynchPad(g, e, pl); break;
        case 2:
            if (readSynchGun1(g, e, pl)) o.flash[pl] = true;
            break;
        case 3: readSynchGun2(g, e, pl); break;
        default: g.slot[pl].trigger = 0; break;
        }
        orgAddTrig(g, e, o, pl);
        if (e.pressReload[pl]) o.reloadReq[pl] = true;
        if (e.pauseLevel > 0) { g.slot[pl].trigger = 0; g.slot[pl].hold = 0; }
    }
    return o;
}
}

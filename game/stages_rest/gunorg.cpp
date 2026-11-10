#include "gunorg.h"
#include <cmath>
namespace vn {

// D_00A12300 de ORGMOD.BIN (offset 0x2B00 del fichero): {municion inicial (+0x4C), espera tras la rafaga (+0x340)} por arma 0..15.
const int8_t kGunOrgTable[16][2] = {
    {8, 0}, {10, 0}, {12, 0}, {16, 0}, {8, 0}, {8, 0}, {8, 0}, {8, 0},
    {6, 5}, {3, 5}, {6, 12}, {30, 0}, {6, 8}, {50, 0}, {2, 20}, {3, 10}};

// El original indexa sin acotar; aqui las armas fuera de 0..15 usan la fila 0 (el juego solo equipa 0..15 como arma de la ranura 0).
static int tbl(int item, int col) { return kGunOrgTable[(unsigned)item < 16 ? item : 0][col]; }

int GunOrg::shotSound(int item, int pl) {
    switch (item) {          // jtbl 0xA12BB0: objetos 8..15 (el 10 suena aparte, ver exe)
        case 8: case 9: return 0xB0112;
        case 10: return 0;
        case 11: return 0xB012E;
        case 12: return 0xB0116;
        case 13: return 0xB012D;
        case 14: return 0xB011A;
        case 15: return 0xB011C;
        default: return pl == 0 ? 0x2000C : 0x2000D;
    }
}

void GunOrg::bulletSet(GunOrgWork& w, GunOrgHost& h) {
    h.ammoDisplay();           // func_20F590
    w.burst = 0;               // +0x342
    w.phase = 0;               // +0x341
    w.cnt344 = 0;              // +0x344
    w.prevValid = 0;           // +0x3C
    // gunOrgSoundOff (0xA11430): funcion vacia
}

void GunOrg::exe(GunOrgWork& w, const GunOrgGun& g, GunOrgSys& sys, GunOrgHost& h) {
    const int pl = sys.pl;
    const int item = sys.equip0;
    int s3 = 0;                // 1 = municion infinita (flag 0x10 de gp-0x72D0 o modo 8)

    w.flag346 = 0;
    w.cool = (int8_t)(w.cool - 1);
    if (w.cool < 0) w.cool = 0;
    if (sys.d50064C) *sys.d50064C = (uint8_t)(*sys.d50064C | (1 << (6 - pl)));
    h.preUpdate();
    if (w.reloadFxFlag == 1) { h.reloadFx(); w.reloadFxFlag = 0; }
    if ((sys.flags72D0 & 0x10) || w.playerMode == 8) {
        s3 = 1;
        w.ammo = tbl(item, 0);
        w.emptyFlag = 0; w.w58 = 0; w.reloadFxFlag = 0; w.cool = 0;
    }

    const bool trig = g.trigger != 0;
    if (!((!trig && w.burst != 1) || w.cool > 0)) {   // si no: 0xA11E34 / 0xA11E30 (soundOff vacia) y salta la comprobacion de recarga
        do {
        int s1 = 0;        // 1 = este fotograma no se dispara (fase de pausa de rafaga 13/11)

        // Memoria del ultimo disparo (no para el objeto 10 en rafaga).
        if (!(item == 10 && w.burst != 0) && trig) {
            if (w.burst != 0) { w.prevX = w.lastX; w.prevY = w.lastY; w.prevValid = w.lastValid; }
            else w.prevValid = 0;
            w.lastX = w.sightX; w.lastY = w.sightY; w.lastValid = g.sightValid;
        }
        if (w.lastValid == 0) break;                                // soundOff, luego recarga
        if (!(w.ammo > 0 || w.burst == 1)) break;
        if (sys.active7418 == 0) {                                  // 0xA11DA8: juego parado, anula la rafaga
            w.burst = 0; w.cool = 0; w.phase = 0; w.cnt344 = 0; w.prevValid = 0; w.reloadReq = 0;
            break;
        }

        if (!s3) {                                                  // 0xA116C8: gasto de municion
            w.shotsA++; w.shotsB++;
            switch (item) {
                case 13: case 11:                                   // 0xA117F0: rafagas
                    if (!trig) { w.cnt344 = (int8_t)(w.cnt344 + 1); if (!(w.cnt344 < 4)) w.cnt344 = 0; }
                    else w.cnt344 = 0;
                    if (w.cnt344 != 0) { s1 = 1; break; }
                    w.ammo--;
                    if (!(w.ammo > 0)) { w.burst = 0; }
                    break;
                case 8:                                             // 0xA117A4: bala de plata, cuesta 5 de plata
                    w.ammo--;
                    if (sys.silver && *sys.silver >= 5) {
                        *sys.silver -= 5; w.flag346 = 1;
                        if (sys.silverTotal) *sys.silverTotal -= 5;
                    }
                    break;
                case 9: w.ammo--; w.flag346 = 1; break;             // 0xA11784
                case 10: if (w.phase == 0) w.ammo--; break;         // 0xA11764: un solo gasto por rafaga
                case 4: break;                                      // sin gasto
                default: w.ammo--; break;
            }
        }
        if (w.ammo == 0) { w.emptyFlag = 1; w.w58 = 0; }            // 0xA1187C
        if (s1) { w.firing = 0; h.shotEnd(); break; }
        w.firing = 1;

        if (item == 13 || item == 11) {                             // 0xA11A50
            if (g.trigger != 0) w.phase = 0; else w.phase = (int8_t)(w.phase + 1);
            if (w.prevValid != 0) {                                 // interpola entre el disparo previo y el actual (t = fase/7)
                float t = (float)w.phase / 7.0f;
                w.sightX = w.prevX + t * (w.lastX - w.prevX);
                w.sightY = w.prevY + t * (w.lastY - w.prevY);
            } else { w.sightX = w.lastX; w.sightY = w.lastY; }
            if (w.ammo <= 0) w.phase = 7;
            else if (!h.canContinueBurst(pl)) w.phase = 7;
            if (w.phase >= 7) { w.burst = 0; w.cool = (int8_t)tbl(item, 1); w.phase = 0; }
            else w.burst = 1;
        } else if (item == 10) {                                    // 0xA118FC: dispersion (balines)
            w.phase = (int8_t)(w.phase + 1);
            if (w.phase == 1) { w.burst = 1; w.spreadDone = 0; }
            else {
                int r1 = h.rand(); int sx = 1 - ((r1 % 2) * 2);
                int r2 = h.rand();
                w.sightX = w.lastX + (float)((r2 % 64) * sx);
                int r3 = h.rand(); int sy = 1 - ((r3 % 2) * 2);
                int r4 = h.rand();
                w.sightY = w.lastY + (float)((r4 % 48) * sy);
                w.spreadDone = 1;
            }
            if (w.phase >= 6) { w.burst = 0; w.cool = (int8_t)tbl(item, 1); w.phase = 0; }
        } else {                                                    // 0xA11BAC: arma de un disparo
            w.cool = (int8_t)tbl(item, 1);
            w.spreadDone = 0; w.phase = 0; w.cnt344 = 0;
        }
        h.setSight(pl, w.sightX, w.sightY);
        h.postUpdate();
        {
            int snd = 0;
            if (item == 10) { if (w.phase == 1) h.playSound(0xB010E); }
            else snd = shotSound(item, pl);
            if (snd) h.playSound(snd);
        }
        if (sys.gp7218_4 == 2 && !s3 && w.spreadDone == 0) {        // 0xA11D04: casquillo (n = min(municion, 8))
            int n = w.ammo < 8 ? w.ammo : 8;
            h.shellEject(pl, n);
        }
        h.shotEnd();                                                // func_210CE0
        } while (0);
        // 0xA11DF4: peticion de recarga
        if (g.trigger != 0 && g.sightValid == 0 && w.ammo != w.ammoPrev) w.reloadReq = 1;
    }

    // 0xA11E3C: cierre comun
    if (sys.gp738C != 0) w.reloadReq = 0;
    if (sys.stage == 7) w.reloadReq = 0;
    if (w.reloadReq != 0) h.ammoDisplay();
}

float orgPlWeaponDamageGet(const StageSys& s, int pl) {
    if (s.gameState == 0xA && s.gameKind == 1) return OrgLogic::weaponDamage(s, pl);
    return 1.0f;
}

void decLife(DecLifeTarget& e, const StageSys& s, int pl, int dmg) {
    e.prevLife = e.life;                                         // +0x20 = +0x18
    float f = (float)dmg * orgPlWeaponDamageGet(s, pl);
    int d = (int16_t)(int)f;                                     // func_14C518 (trunc) y luego s16
    if (s.gameState == 0xA && s.gameKind == 1 && pl >= 0 && pl < 2 && s.pl[pl].flag346 != 0) d = (int16_t)e.life;   // bala de plata/estaca: dano = vida
    if ((e.flags & 0x1000) == 0) e.life -= d;
}

bool rayVsBall(const ShotRay& r, float x, float y, float z, float radius) {
    float a = -x * r.cosB - z * r.sinB;
    float b = (-x * r.sinA) * r.sinB - y * r.cosA + r.cosB * (z * r.sinA);
    float d = std::sqrt(a * a + b * b);
    return d <= radius;
}
}

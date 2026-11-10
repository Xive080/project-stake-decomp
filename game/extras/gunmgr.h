// GunMgrClass de Vampire Night (ELF main, 0x1EDA10..0x1EE5B0), reescrito a mano.
// Entrada de la pistola/mando por jugador: posicion de la mira, gatillo, calibracion.
// Sin hardware PS2: lo que el original lee de globales (0x500xxx/0x502xxx/gp-0x7xxx) o pide al motor llega en `GunEnv`.
// Las llamadas al motor (ReqGunFlash, force_Refire_GC2, get_gunposiGC1, gun_action) se devuelven como eventos o entradas.
#pragma once
#include <cstdint>
namespace vn {

// Una entrada por jugador dentro del objeto GunMgr (0x64 B por jugador; offsets en comentarios).
struct GunSlot {
    uint16_t rawX = 0, rawY = 0;   // +0, +2 posicion cruda del sensor (0xFFFF = sin lectura)
    int16_t  rawValid = 0;         // +4
    int16_t  index = 0;            // +6 numero de jugador (start() deja 0 y 1)
    int16_t  sightX = 0, sightY = 0; // +8, +0xA mira en pantalla, centro 0,0; limites +-0x140, +-0xE0
    int16_t  sightValid = 0;       // +0xC
    uint8_t  trigger = 0;          // +0xE gatillo "efectivo" este fotograma
    uint8_t  hold = 0;             // +0xF fotogramas con el gatillo mantenido (armas de ráfaga)
    int8_t   flash = 0;            // +0x10 cuenta atras del fogonazo (4 fotogramas) de la pistola
};

struct GunMgr {
    GunSlot slot[2];
    int32_t enabled = 0;           // +0xC8 (init() = 0, start() = 1)
    uint8_t cooldown = 0;          // +0xCD contador 0..24 de la cadencia (orgAddTrig0)
    // Globales gp-0x7434/-0x7430/-0x742C/-0x7428 (halfwords) que init() pone a 0; -0x7428 = "calibrando" (ver adjustVal).
    int16_t gpCalib[4] = {0, 0, 0, 0};
};

// Lo que GunMgr consulta del resto del sistema.
struct GunEnv {
    int gameState = 0;             // 0x500F14 (7 = ?, 0xA = juego arcade, 0x14 = calibracion)
    int gameKind = 0;              // gp-0x73B4 (1 = tienda/objetos equipados)
    int flags72D0 = 0;             // gp-0x72D0 (bit 0x10 = balas infinitas)
    int active7418 = 0;            // gp-0x7418
    int8_t gp72AC = 0;             // gp-0x72AC
    int pauseLevel = 0;            // PauseMgr::get(&0x500F20)
    int device[2] = {0, 0};        // GetGunDevice(pl): 0 = nada, 1 = mando, 2 = GunCon (gun1), 3 = GunCon2 (gun2)
    // Mando (read_SYNCH_pad)
    int8_t   speedCfg[2] = {0, 0};     // 0x50290A + 12*pl: velocidad de la mira (valor % 11 + 1)
    uint8_t  stickX[2] = {0x80, 0x80}, stickY[2] = {0x80, 0x80};   // 0x50065C / 0x50065D + 26*pl
    uint16_t buttons[2] = {0, 0};      // 0x500652 + 26*pl
    uint16_t boostMask[2] = {0, 0};    // 0x502910 + 12*pl
    uint16_t recenterMask[2] = {0, 0}; // 0x502912 + 12*pl
    // Calibracion (adjustVal_cz)
    int16_t calibX = 0, calibY = 0;    // 0x502924, 0x502926
    int16_t gpCalibFlag = 0;           // gp-0x7428: distinto de 0 = calibrando, sin offset
    // GunCon (gun1)
    int16_t gc1Status[2] = {0, 0};     // 0x50065A + 26*pl (0 o 1 = pistola sin datos)
    uint16_t gc1X[2] = {0xFFFF, 0xFFFF}, gc1Y[2] = {0xFFFF, 0xFFFF};   // lo que devuelve get_gunposiGC1
    // GunCon2 (gun2): GetGunBuffer + gun_action
    int32_t gc2Trigger[2] = {0, 0};    // buffer+0xC
    int16_t gc2X[2] = {0, 0}, gc2Y[2] = {0, 0};   // buffer+0x1C, +0x20
    // Pulsaciones del motor
    bool pullTrigger[2] = {false, false};      // is_PullTrigger
    bool pullTriggerNow[2] = {false, false};   // is_PullTriggerNow
    bool pressReload[2] = {false, false};      // is_PressReload
    // Estado del jugador (PlayerTask a 0x500730 + 0x348*pl)
    int playerMode[2] = {0, 0};        // +4
    int playerBullet[2] = {0, 0};      // +0x4C
    int8_t itemSlot0[2] = {0, 0};      // 0x502690 + 4*pl: arma equipada
};

// Peticiones al motor / jugador generadas durante el fotograma.
struct GunOut {
    bool flash[2] = {false, false};        // ReqGunFlash(pl)
    bool forceRefire[2] = {false, false};  // force_Refire_GC2(pl)
    bool reloadReq[2] = {false, false};    // PlayerTask+0x5C = 1
};

struct GunLogic {
    static void init(GunMgr& g);                                  // GunMgrClass::init
    static void start(GunMgr& g);                                 // GunMgrClass::start
    static void resetSightPosPl(GunMgr& g, const GunEnv& e, int pl);   // reset_SightPosPl
    static void resetSightPos(GunMgr& g, const GunEnv& e);        // reset_SightPos
    static void adjustValCz(GunSlot& s, const GunEnv& e);         // adjustVal_cz
    static void readSynchPad(GunMgr& g, const GunEnv& e, int pl); // read_SYNCH_pad
    static bool readSynchGun1(GunMgr& g, const GunEnv& e, int pl);// read_SYNCH_gun1 (true = pide ReqGunFlash)
    static void readSynchGun2(GunMgr& g, const GunEnv& e, int pl);// read_SYNCH_gun2
    static void orgAddTrig0(GunMgr& g, const GunEnv& e);          // orgAddTrig0
    static void orgAddTrig(GunMgr& g, const GunEnv& e, GunOut& o, int pl);   // orgAddTrig
    static GunOut readSynch(GunMgr& g, const GunEnv& e);          // read_SYNCH (read_VB no hace nada)
};
}

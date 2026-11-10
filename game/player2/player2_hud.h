// Logica de HUD y efectos del jugador (sin dibujo): iconos de balas, aviso de recarga, cartuchos expulsados, particulas de vida,
// temblor de dano (PostPlayerTask) y la guarda de disparo (PrePlayerTask). ELF main 0x20D990..0x2133A0, reescrito a mano.
// Todo lo que el original dibuja (SprPutCE, PutPolyMatCE, nlSetScaleFactor...) se emite como valores/eventos para el motor.
// El generador aleatorio es nlRand() y se inyecta como puntero a funcion. Sin dependencias de plataforma.
#pragma once
#include <cstdint>
namespace vn {

using RandFn = int (*)();   // nlRand()

// ---- Iconos de balas (PlayerTask::dispBulletInit / dispBulletMoveOn / dispBulletLoop) ----
constexpr int kBulletIcons = 9;
struct BulletIconLayout { float x, y, z; };                 // D_51F270 + pl*0x6C: 9 x 12 B
struct BulletIcon {                                         // D_51F350 + pl*0x144: 9 x 0x24 B
    float x = 0, y = 0, z = 0;       // +0, +4, +8
    float vy = 0;                    // +0x10 velocidad vertical (hacia el hueco que deja la bala gastada)
    int32_t moving = 0;              // +0x18
    int32_t step = 0;                // +0x1C (0..3; al llegar a 4 se detiene)
};
struct BulletHud {
    BulletIconLayout layout[kBulletIcons];
    BulletIcon icon[kBulletIcons];
};
// Datos del jugador que lee el HUD de balas.
struct HudBulletIn {
    int playerNo = 0;
    int maxBullet = 0;               // +0x48
    int bullet = 0;                  // +0x4C
    int emptyFlag = 0;               // +0x50
    int flag58 = 0;                  // +0x58 (contador de fotogramas del aviso)
    int screenMode = 0;              // *(s8*)(gp-0x7218 + 4): 2 = pantalla de juego completa, 1 = reducida
    int active7418 = 0;              // gp-0x7418
    bool gunTrigger = false;         // D_500580 + 0x64*pl + 0xE (lb)
    bool gunSightValid = false;      // +0xC (lh)
    int pauseLevel = 0;              // PauseMgr::get(&D_500F20)
};
struct HudBulletOut {
    bool drawIcons = false;          // screenMode == 2
    int iconCount = 0;               // min(bullet, 9)
    bool drawCount = false;          // bullet >= 10: se escribe el numero (decenas, unidades)
    int tens = 0, ones = 0;
    bool reloadBlink = false;        // el aviso se ve (flag58 % 60 < 45)
    bool reloadStrong = false;       // flag58 >= 100: capas extra del aviso
    int sound = 0;                   // seqRequest(0x20017, 0, 0, volumen 0x40) o 0
    int soundParam = 0;
};
struct HudLogic {
    static void layoutInit(BulletHud& h, int playerNo);                                // parte de PlayerTask::__ct__ que rellena D_51F270
    static void bulletInit(BulletHud& h);                                              // dispBulletInit (0x210C60)
    static void bulletMoveOn(BulletHud& h, int maxBullet, int bullet);                 // dispBulletMoveOn (0x210CE0)
    static HudBulletOut bulletLoop(BulletHud& h, HudBulletIn& in);                     // dispBulletLoop (0x210DB0); modifica flag58
};

// ---- Cartuchos expulsados (dispCartridgeInit 0x210680 / Born 0x2106D0 / Loop 0x210990) ----
constexpr int kCartridges = 18;
struct Cartridge {                                          // 0x2C B desde PlayerTask+0x24E4
    float x = 0, y = 0, z = 0;       // +0, +4, +8
    float vx = 0, vy = 0, vz = 0;    // +0xC, +0x10, +0x14
    int32_t active = 0;              // +0x18
    int32_t age = 0;                 // +0x1C (se apaga al llegar a 0x79)
    int32_t kind = 0;                // +0x20 (9 al nacer)
    int32_t angle = 0;               // +0x24
    int32_t angleVel = 0;            // +0x28
};
struct CartridgePool { int32_t next = 0; Cartridge c[kCartridges]; };   // next = PlayerTask+0x24E0
struct CartridgeIn { int playerNo = 0; int gameState = 0; int gameKind = 0; int8_t itemSlot0 = 0; int pauseLevel = 0; };
struct CartridgeLogic {
    static void init(CartridgePool& p);                                                // dispCartridgeInit
    static void born(CartridgePool& p, const CartridgeIn& in, float x, float y, float z, RandFn rand);   // dispCartridgeBorn
    static void loop(CartridgePool& p, int pauseLevel);                                // dispCartridgeLoop
};

// ---- Particulas del indicador de vida (dispLifePtclInit 0x211C50 / Born 0x211DE0 / Loop 0x211FE0) ----
constexpr int kLifeParticles = 32;
struct LifeParticle {                                       // 0x40 B desde PlayerTask+0x1CC4
    float x = 0, y = 0, z = 0;       // +0, +4, +8
    float vx = 0, vy = 0, vz = 0;    // +0xC, +0x10, +0x14
    int32_t active = 0;              // +0x18
    int32_t age = 0;                 // +0x1C (se apaga con edad >= 0x51)
    int32_t spinA = 0;               // +0x20
    int32_t spinB = 0;               // +0x28
    int32_t spinAVel = 0;            // +0x2C
    int32_t spinBVel = 0;            // +0x34
    float scale = 0;                 // +0x38 (se apaga con escala <= 0)
    float scaleVel = 0;              // +0x3C
};
struct LifeParticlePool { LifeParticle p[kLifeParticles]; float velX = 0; /* PlayerTask+0x24DC: lo calcula dispLifePtclCalcVelX (no reescrita) */ };
struct LifeParticleLogic {
    static void init(LifeParticlePool& p);                                             // dispLifePtclInit
    static bool born(LifeParticlePool& p, int pauseLevel, float x, float y, float z, RandFn rand);   // dispLifePtclBorn; true si creo una
    static void loop(LifeParticlePool& p, int pauseLevel);                             // dispLifePtclLoop
};

// ---- Temblor de dano (PostPlayerTask::dmgEftPlayer, 0x2134D0) y guardas por fotograma ----
struct DmgShakeSys {
    int32_t hitP0 = 0;               // 0x5007EC (1 = jugador 0 recibio dano; el original lo pone a 0)
    int32_t hitP1 = 0;               // 0x500B34
    int32_t timer = 0;               // gp-0x7414
    int32_t shakeX = 0;              // gp-0x7404
    int32_t shakeY = 0;              // gp-0x7408
    uint32_t flags72D0 = 0;          // gp-0x72D0
};
using Sin16Fn = float (*)(int);      // func_001B09B4: seno con el angulo en 1/65536 de vuelta (entra con a0 & 0xFFFF)
struct DmgLogic {
    static void dmgEftPlayer(DmgShakeSys& s, Sin16Fn sin16);                           // PostPlayerTask::dmgEftPlayer
    // PostPlayerTask::exec (0x213450): despues del temblor fija gp-0x73F4 segun s16 0x500DF8 y 0x500734. Devuelve false si no la toca.
    static bool postExec(DmgShakeSys& s, Sin16Fn sin16, int16_t word500DF8, int word500734, int32_t& g73F4);
    // PrePlayerTask::exec (0x2132B0): activa/desactiva el bit 0 de gp-0x72D0
    static void preExec(uint32_t& flags72D0, int8_t camState, int8_t g73D8, int16_t gameState);
};
}

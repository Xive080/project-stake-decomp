// Menu de opciones (overlay optmod, CsOptionTask): reglas de datos reescritas a mano (sin dibujo, sin foco/animacion).
// Dos copias de la configuracion: SysCfg (0x502900, la real que lee el juego) y CsOptData (0x93F800, indices de menu).
#pragma once
#include <cstdint>
#include "gunmgr.h"
namespace vn {

// Configuracion de servicio (0x502900..). Solo los campos que el menu toca; offsets en comentarios.
struct SysCfg {
    int8_t difficulty = 8;     // +0: 0,6,8,10,16 (menu) ; coincide con el rango 0..16 de OrgSetDifficulty
    int8_t initLife = 3;       // +1 (1..9)
    int8_t maxLife = 5;        // +2 (1..9, initLife <= maxLife)
    int8_t credits = 5;        // +4: creditos por moneda; 0 = juego libre (menu 10)
    int8_t sightType = 0;      // +6: 0/1 grafico de la mira
    int8_t sightSpeed = 5;     // +0xA: 0..10 (gun speed del mando; GunMgr usa % 11 + 1)
    int8_t vibration1 = 0;     // +8
    int8_t padCfgA = 0;        // +9: asignacion de botones del jugador 1
    int8_t sightSpeed2 = 5;    // +0x16 (jugador 2)
    int8_t vibration2 = 0;     // +0x14
    int8_t padCfgB = 0;        // +0x15
    uint8_t bgmVol = 0xFF;     // +0x20
    uint8_t voiceVol = 0xFF;   // +0x21
    uint8_t voiceVol2 = 0xFF;  // +0x22
    int16_t calibX = 0, calibY = 0;   // +0x24, +0x26
    int32_t unlockFlag = 0;    // +0x28: 1 = permite 10 creditos (juego libre) en el menu
};

// Indices de menu (0x93F800..0x93F80F).
struct CsOptData {
    int8_t diff = 2;       // +0 indice 0..4
    int8_t initLife = 3;   // +1
    int8_t maxLife = 5;    // +2
    int8_t credits = 5;    // +3 (1..10)
    int8_t vib1 = 0;       // +4
    int8_t vib2 = 0;       // +5
    int8_t sight = 0;      // +6
    int8_t bgm = 0;        // +7 0..10
    int8_t voice = 0;      // +8
    int8_t voice2 = 0;     // +9
    int8_t padA0 = 0, padA1 = 0;   // +0xA, +0xB (1,4 o 4,1)
    int8_t c = 2;          // +0xC
    int8_t padB0 = 0, padB1 = 0;   // +0xD, +0xE
    int8_t f = 2;          // +0xF
};

struct LifeResult { bool changed; int warnTimer; };   // warnTimer: -1 = no se toca; 0x78 = aviso "no se puede" (this+0x70)
enum LifeAction { LIFE_INIT_DEC = 4, LIFE_INIT_INC = 5, LIFE_MAX_DEC = 6, LIFE_MAX_INC = 7 };   // numero de foco (jtbl 0x93F120)

extern const int8_t kDifficultyValue[5];     // 0x93DB18 menu -> SysCfg.difficulty
extern const int8_t kDifficultyMenu[17];     // 0x93DB20 SysCfg.difficulty (0..16) -> menu
extern const int kDifficultyChoice[6];       // 0x93F030 foco -> indice de menu
extern const uint8_t kBgmLevel[11];          // 0x93DC50
extern const uint8_t kVoiceLevel[11];        // 0x93DC60

// Posicion de la mira en pantalla (0..0x27F, 0..0xDF) para el cursor del menu (CsOptionTask::getSightPos 0x933260).
struct SightScreenPos { int x, y; bool drawSight; };   // drawSight: el original llama a func_20F570 (dibujo de la mira del mando)

// Bloques que Read/WriteCfgGameBuffer (lib 0x1D3720) copian entre la partida (live) y el buffer de guardado (saved), en bytes.
struct CfgCopy { uint32_t live, saved, size; };
extern const CfgCopy kCfgCopy[4];

struct OptionsLogic {
    static SightScreenPos getSightPos(const GunSlot& s, int device, int menuMode);
    static int calcBgmBalance(uint8_t v);                  // calcBgmBalance 0x92B5D0
    static int calcVoiceBalance(uint8_t v);                // calcVoiceBalance 0x92B6F0
    static int chgConfigBitR(int bits);                    // chgConfigBitR 0x9384A0
    static void setVibrationSysCfg(SysCfg& s, const CsOptData& m);          // setVibrationSysCfg 0x9367B0
    static void sysCfgToMenu(CsOptData& m, const SysCfg& s);                // Set_SysCfg_To_CsOptData 0x92C960
    static void gamesystemDefault(CsOptData& m, SysCfg& s);                 // gamesystemDefault 0x935490
    static void lifeDefault(CsOptData& m, SysCfg& s);                       // lifeDefault 0x935E70
    static void difficultyChoose(CsOptData& m, SysCfg& s, int choice);      // difficulty (foco 1..5) 0x935520
    static LifeResult lifeAdjust(CsOptData& m, SysCfg& s, int action);      // life 0x935B80
    static void creditsAdjust(CsOptData& m, SysCfg& s, bool inc);           // credits 0x935EB0 (foco 1 = +, 2 = -)
    static void balanceAdjust(CsOptData& m, SysCfg& s, int which, bool inc);// balance 0x939450 (0 bgm, 1 voz, 2 voz 2)
    static void sightSpeedAdjust(SysCfg& s, int8_t& menuVal, bool inc);     // sightspeed 0x936D70 (0..10)
    static bool checkSysCfgBak(const uint32_t* bak, const uint32_t* cur);   // CheckSysCfgBak 0x93B310 (189 palabras)
    static void copySysCfgBak(uint32_t* bak, const uint32_t* cur);          // CopySysCfgBak 0x93B2E0
};
}

// Reglas del modo Original (overlay orgmod, base 0xA0F800). Sin dependencias de plataforma.
#pragma once
#include <cstdint>

struct OrgMissionCtx {
    int  gameMode = 0;          // gp-0x73B4: solo vale 1 para evaluar
    int  playerScore[2] = {0, 0}; // PlayerTask+0xC (0x50073C + 0x348*pl)
    int  stageMode = 0;         // halfword en 0x500F14 (10 = modo mision)
    int  counter = 0;           // gp-0x72DC
    int8_t flagsA = 0;          // byte 0x502716
    int8_t flagsB = 0;          // byte 0x502720
    int8_t flagsC = 0;          // byte 0x502727
};

struct OrgRules {
    // SetOrgMissionOver__Fv (0xA11EA0): 0 = no superada, 1/2/3 = mision cumplida (tipo).
    // Cada mision exige bits 0 y 1 puestos y bit 5 limpio en su byte de flags.
    static int missionOver(const OrgMissionCtx& c);
    // setDispScoreJoinIn__Fi (0xA12270): si el modo (byte gp-0x73D8) no es 7 y el jugador
    // lleva el objeto 0x12 en la ranura 1, activa la marca de puntuacion (gp-0x737C).
    static bool dispScoreJoinIn(int8_t modeByte, int8_t slot1Item);
};

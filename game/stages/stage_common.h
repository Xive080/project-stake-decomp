// Tipos compartidos por la logica de fases (overlays orgmod/aomod/trnmod/acmod).
// Los globales de sistema (0x500xxx, 0x502xxx, gp-0x7xxx) viven en bss y no estan en el ELF:
// se modelan como estructuras inyectadas. Las llamadas a motor se devuelven como eventos.
#pragma once
#include <cstdint>
namespace vn {
typedef int (*RandFn)();      // nlRand (ELF 0x1A7EE0); el test inyecta una secuencia fija

// Estado minimo de un jugador que leen las reglas de fase (estructura de jugador, 0x348 B por jugador en 0x500730).
struct StagePlayer {
    int mode = 0;             // +4  (PlayerTask::changePlayerMode)
    int score = 0;            // +0xC
    int8_t flag346 = 0;       // +0x346 (marca interna de arma; gunOrg la pone a 1 al gastar bala de ciertos objetos)
};

// Globales de sistema que consultan las funciones de fase.
struct StageSys {
    int gameState = 0;        // 0x500F14 (s16); 0xA = juego arcade normal
    int gameKind = 0;         // gp-0x73B4; 1 = modo "Original" (tienda/objetos), 2 = otro modo especial
    StagePlayer pl[2];
    int8_t equip[2][4] = {};  // D_502690 + jugador*4: 4 objetos equipados (id de objeto, 0 = vacio)
    int32_t silver = 0;       // 0x502704 (plata actual, tope 999999)
    int32_t silverTotal = 0;  // gp-0x72DC (plata acumulada en la partida)
    int8_t  invCount[0x35] = {}; // 0x5026CE + id (1..0x34): unidades en inventario, tope 99
    int8_t  shopOpen[0x35] = {}; // 0x502698 + id: objeto desbloqueado en tienda
    int8_t  shopDirty = 0;    // 0x50274C
    int8_t  missionFlags[0x28] = {}; // 0x502708 + n: banderas por mision (bit0 = disponible, bit1 = vista, bit2 = 0x4, bit4 = 0x10, bit5 = 0x20 = completada)
};
}

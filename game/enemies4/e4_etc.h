// EnEtcTask (GAMEMOD 0x9381D0..0x939D70+): tarea "varios" de decorado/eventos. Segun el tipo (+0x14 = byte 1 de ENEMY_TABLE) elige un
// manejador distinto (jtbl_009A89B0, 9 entradas) y, para tipos 10..59 y >= 70, hace comprobaciones de objetos (ramas ocultas del mapa).
#pragma once
#include "e4_common.h"
namespace vn { namespace en4 {

// Manejadores que registra el constructor (func_1A3DC0 / D_009852xx [2]); valores = direccion de la funcion en GAMEMOD.
enum EtcHandler : int {
    EH_NONE = 0,
    EH_SNOW = 0x939D70,        // SnowCtrl
    EH_DEMO_PAUSE_ON = 0x9381D0,
    EH_DEMO_PAUSE_OFF = 0x938200,
    EH_TIMER_BUNKI = 0x9387A0, // St4_02TimerBunki
    EH_ST4_TAMA = 0x9385D0,    // EnEtc_St4_Tama
    EH_FIREBALL = 0x938230,    // EnEtcST3_SC02_02FireBall
    EH_ITEM_CHECK = 0x938B30,  // EnEtc_ItemCheck (tambien para los tipos >= 70)
    EH_BRANCH_MARK = 0x938860  // EnEtc_BranchMark (tipos 10..59)
};

struct EtcState {
    uint32_t flags = 0;       // +0x0C: bit 3 (0x8) = golpeado (marca de rama), 0x40000000 = bola de fuego en fase final, 0x1008A = marca de rama
    uint32_t flags2 = 0;      // +0x10
    int      type = 0;        // +0x14 (s16)
    float    pos[3] = {0, 0, 0};            // +0x2C/+0x30/+0x34
    float    lastCam[3] = {0, 0, 0};        // +0x38/+0x3C/+0x40 (nieve: ultima posicion de camara)
    int      a50 = 0, a54 = 0, a58 = 0;     // +0x50/+0x54/+0x58: u16 de ENEMY_TABLE +2,+4,+6
    int      w148 = 0, w14C = 0, w150 = 0, w154 = 0, w158 = 0;   // +0x148..+0x158: temporales (el significado depende del manejador)
    int8_t   step = 0;        // +0x160: paso de la maquina de estados del manejador
    int      w164 = 0, w168 = 0;            // +0x164/+0x168: gp-0x73D4 / gp-0x73D0 (bytes) en el constructor; -1 en nieve
    int      branchSeen = 0;  // +0x16C
    float    f170 = 0;        // +0x170: escala de la bola de fuego
    float    fB0 = 0;         // +0xB0: 10.0 en marca de rama
    int      wC4 = 0;         // +0xC4: puesto a 0 en marca de rama
    int      handler = EH_NONE; int handlerPrio = 0;
    uint32_t param[4] = {0, 0, 0, 0};       // palabras del bloque ENEMY_TABLE+0x14 (+0x28 de la tarea)
    bool     closed = false;
};

struct EtcLogic {
    // true si el objeto `item` esta "poseido y activo y no bloqueado" en arcade con gameKind == 1.
    // (patron repetido en el constructor y en BranchMark: flags D_502708[item] bit0 && bit1 && !bit5)
    static bool itemGate(const E4Env& e, int item);
    // __ct__9EnEtcTaskFP11ENEMY_TABLE (0x938B50), parte de reglas. `param` = 4 palabras de ENEMY_TABLE+0x14, `type` = byte 1,
    // u16 de +2,+4,+6, pos = +8,+0xC,+0x10. Escribe globales de E4Env y devuelve eventos E4_SET_HANDLER / E4_ITEM_APPLY.
    static void init(EtcState& s, E4Env& e, int type, const float pos0[3], int u2, int u4, int u6, const uint32_t param[4], E4Events& ev);
    static void demoPauseOn(EtcState& s, E4Env& e, E4Events& ev);    // DemoPauseOn 0x9381D0
    static void demoPauseOff(EtcState& s, E4Env& e, E4Events& ev);   // DemoPauseOff 0x938200
    static void itemCheck(EtcState& s, E4Events& ev);                // EnEtc_ItemCheck 0x938B30
    static void timerBunki(EtcState& s, E4Env& e, E4Events& ev);     // St4_02TimerBunki 0x9387A0
    static void st4Tama(EtcState& s, E4Env& e, E4Events& ev);        // EnEtc_St4_Tama 0x9385D0
    // EnEtcST3_SC02_02FireBall 0x938230: `pathPos` = posicion de la bola en el fotograma w158 (func_1CC710 evaluando el camino 0x229/0x22A).
    static void fireBall(EtcState& s, E4Env& e, const float pathPos[3], E4Events& ev);
    static void branchMark(EtcState& s, E4Env& e, E4Events& ev);     // EnEtc_BranchMark 0x938860 (parte de reglas)
    static void snowCtrl(EtcState& s, E4Env& e, E4Events& ev);       // SnowCtrl 0x939D70
};
}}

// ArmorTask (GAMEMOD 0x956D60..0x957130): armadura que lleva una parte de un enemigo (EnVampire/Zombie "Eknd").
// Mientras la parte del dueno tiene el bit 0x100 de +0x154 (marcada) la armadura se muestra; al romperse (bit limpio)
// la tarea suelta un trozo (EnEtcAttach), sube el contador de armadura de la parte y se cierra.
#pragma once
#include "e4_common.h"
namespace vn { namespace en4 {

// ARMORDT (parametro del constructor)
struct ArmorDt {
    int   partIdx = 0;        // +0x00 -> +0x14C: indice de la parte del dueno (cada parte ocupa 0xF0 B)
    int   shown = 0;          // +0x04 -> +0x148: != 0 la armadura se dibuja (PreDisp+Disp); == 0 solo sustituye el modelo (NotDisp)
    int   attachModel = 0;    // +0x08 -> +0x150: id de modelo del trozo que sale volando
    float localPos[3] = {0, 0, 0};   // +0x0C/+0x10/+0x14 -> +0x154/+0x158/+0x15C
    float scale = 0;          // +0x18 -> +0xB0
};

// Vista de la parte del dueno que usa la armadura (dueno + 0xA0 + idx*0xF0).
struct ArmorOwnerPart {
    uint32_t flag154 = 0;     // dueno + idx*0xF0 + 0x154: bit 0x100 = armadura puesta
    int16_t  stage = 0;       // parte + 0x168 (s16): contador de armaduras ya rotas (indice de modelo)
    uint32_t modelRef = 0;    // parte + 0xA0 (palabra): modelo actual de la parte
};

struct ArmorState {
    uint32_t flags = 0x10001; // +0x0C (ctor: 0x10001); Disp no dibuja si bit 3 (0x8)
    ArmorDt dt;
    int ownerType = 0;        // dueno +0x5C (s16): tipo de enemigo (indice de D_009AE640)
    ArmorOwnerPart part;
    bool closed = false;
};

enum ArmorHandler : int { AH_PREDISP = 0, AH_DISP = 1, AH_NOTDISP = 2 };   // D_009A17D8/17E8/17F8 [2]

struct ArmorLogic {
    // __ct__9ArmorTaskFP7ARMORDT (0x956D60), parte de reglas. Devuelve por eventos los manejadores que registra:
    //  shown != 0 -> PREDISP (prioridad 0) y DISP (prioridad 0x10005); shown == 0 -> NOTDISP (prioridad 0).
    static void init(ArmorState& s, const ArmorDt& dt, E4Events& ev);
    // PreDisp__9ArmorTaskFv (0x956F00): si la parte ya no esta marcada (bit 0x100 limpio) -> ComeOff.
    static void preDisp(ArmorState& s, E4Events& ev);
    // NotDisp__9ArmorTaskFv (0x956FC0): si no esta marcada, la parte pasa al modelo modelTable[tipo][idx*6 + stage] y -> ComeOff.
    // `modelTable` = D_009AE640[tipo] (puntero a palabras: 6 modelos por parte).
    static void notDisp(ArmorState& s, const uint32_t* modelTable, E4Events& ev);
    // ComeOff__9ArmorTaskFv (0x957040): crea el trozo (EnEtcAttach con attachModel), parte.stage += 1 y cierra la tarea.
    // La posicion del trozo sale de la matriz de camara y de la parte: la calcula el motor (evento con f[] = posicion local).
    static void comeOff(ArmorState& s, E4Events& ev);
    // Disp__9ArmorTaskFv (0x956F50): solo dibujo; devuelve true si se dibuja (bit 0x8 de flags limpio).
    static bool shouldDraw(const ArmorState& s) { return (s.flags & 0x8) == 0; }
};
}}

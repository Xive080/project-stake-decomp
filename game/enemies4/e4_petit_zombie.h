// PetitZombieTask (GAMEMOD 0x951940..0x951FE0) reescrita a mano. Zombi pequeno de decorado: se mueve en un recinto o gira sobre si mismo;
// al recibir un impacto (+0xC bit 8) pasa a la animacion de caida (0x43) y su "manejador" cambia a Dead.
// Campos nombrados por uso, con el offset original en comentario.
#pragma once
#include "e4_common.h"
namespace vn { namespace en4 {

// Bloque de parametros ENEMY_TABLE + 0x14 (puntero en +0x28 de la tarea).
struct PetitParam {
    int8_t  type = 0;        // +0x00: tipo (indice de la tabla de escala D_009ACB40; se copia a +0x5C)
    int8_t  mode = 0;        // +0x01: modo de movimiento (+0x1D78): 1 = gira, 2 = patrulla en recinto, otro = quieto
    int16_t pathId = 0;      // +0x02: camino de camara en el que se cierra
    int16_t closeFrame = 0;  // +0x04: fotograma de camara a partir del cual se cierra
    int16_t anim = 0;        // +0x06: animacion inicial (0 = sortear de D_009B56B0)
    float   xMin = 0, xMax = 0, zMin = 0, zMax = 0;   // +0x08, +0x0C, +0x10, +0x14
};

struct PetitZombieState {
    uint32_t flags = 0;       // +0x0C: bit 0 = vivo, bit 3 (0x8) = golpeado, 0x80 = tipo con efecto, 0x8000 = modo 0xF sin escala
    int      life = 0;        // +0x18 (se inicia con el byte +1 de ENEMY_TABLE; en modo giro decide el sentido)
    int      anim = 0;        // +0x34: animacion
    float    frame = 0;       // +0x3C: fotograma
    float    scale = 0;       // +0x1CE0: escala segun el tipo
    int      type = 0;        // +0x5C (s16)
    int      mode = 0;        // +0x1D78
    float    pos[3] = {0, 0, 0};   // +0x1D7C/+0x1D80/+0x1D84 (los apunta +0x30)
    int      angle = 0;       // +0x28 de la estructura de posicion: angulo de 16 bit (0x10000 = vuelta)
    int      slot1DA0 = 0, slot1DA4 = 0, slot1DA8 = 0;   // +0x1DA0.. : u16 de ENEMY_TABLE +2,+4,+6
    PetitParam p;
    uint32_t partFlags[8] = {0};   // +0x550,+0x634,+0x820,+0x904,+0xBE0,+0xCC4,+0xEB0,+0xF94: puestos a 0 en el constructor
};

enum PetitHandler : int { PH_CTRL = 0, PH_DEAD = 1 };   // D_009A1600[2] = Ctrl (0x951C40), D_009A1620[2] = Dead (0x951FD0)

// Entradas del constructor que no estan en el ELF.
struct PetitCtorIn {
    const float* scaleByType = nullptr;      // D_009ACB40 (floats por tipo: 12 x 15.0, 4 x 10.0, 3 x 15.0, 30.0 ...)
    int          scaleCount = 0;
    const int*   initAnimTable = nullptr;    // D_009B56B0 (4 palabras = 67)
    bool         typeHasEffect = false;      // *(0x1F2972? D_1F30000 - 0x68D0 + tipo*4)->s16[0x22] != 0 (tabla en RAM de ejecucion)
};

struct PetitZombieLogic {
    // __ct__15PetitZombieTaskFP11ENEMY_TABLE (0x951940), parte de reglas: copia de parametros, animacion inicial, escala (salvo modo 0xF).
    // `tblByte1` = byte +1 de ENEMY_TABLE (vida inicial, +0x14/+0x18). Requiere nlRand() >= 0 (con negativo el original indexa D_009B56B0[-3..-1], datos que no son de la tabla).
    static void init(PetitZombieState& s, const PetitParam& p, int8_t tblByte1, const float pos0[3], int u16a, int u16b, int u16c,
                     const E4Env& e, const PetitCtorIn& in);
    // Ctrl__15PetitZombieTaskFv (0x951C40). Un fotograma: golpe -> caida; cierre por camara; movimiento segun +0x1D78; avance de fotograma.
    static void ctrl(PetitZombieState& s, E4Env& e, E4Events& ev);
    // Dead__15PetitZombieTaskFv (0x951FD0): el original es una instruccion `break 0` (trampa) seguida de jr ra. No hace nada util.
    static void dead() {}
    // prtsDispFunc (0x951F00) es solo dibujo: tipos 0xE, 0xB, 7 y 4 de parte dibujan una segunda matriz. No se reescribe.
};
}}

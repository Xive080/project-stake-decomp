// Escombros del entrenamiento: KomonoTrnCrosetKowareTask (0x21B650) y KomonoTrnDishKowareTask (0x21C5B0), ELF principal.
// Solo la fisica de las particulas y la vida de la tarea; la animacion de partida (calcDynamicsMotion), la matriz de camara y el dibujo
// son del motor (se reciben ya calculados). Piezas de 0x40 B: pos +0, vel +0xC, gravedad +0x1C, giro +0x24, velocidad de giro +0x30, suelo +0x3C.
#pragma once
#include "trnkomono.h"
namespace vn {

struct DebrisPiece {
    float pos[3] = {};        // +0, +4, +8
    float vel[3] = {};        // +0xC, +0x10, +0x14
    float grav = 0;           // +0x1C se suma a vel[1] cada fotograma
    int   rot[3] = {};        // +0x24, +0x28, +0x2C (16 bits)
    int   rotVel[3] = {};     // +0x30, +0x34, +0x38
    float floorY = 0;         // +0x3C (Croset: altura inicial del objeto; Dish: 0.0)
};
// Paso comun de las dos clases (ctrl, bucle de piezas). Sin pausa: rebote en el suelo si pos[1] < floorY (vel Y -= 0.9*vel Y, vel X/Z *= 0.6,
// pos Y = suelo, se quita el giro), gravedad, integracion y giro (&0xFFFF).
void debrisStep(DebrisPiece& p, bool paused);

// KomonoTrnCrosetKowareTask (0x21B650). Vida en fotogramas (+0x2894) y opacidad (+0x2898).
struct CrosetKoware {
    int   count = 0;          // +0x64 numero de piezas (la suma de las cuatro listas +0x54..+0x60 en el modelo 0x3E0001; +0x54 en 0x3E015B)
    int   frames = 0;         // +0x2894
    float alpha = 1.0f;       // +0x2898
    // Valores iniciales de una pieza (ctor): vel = base + 0.1*(rand%10 - 5), 0.1*(rand%10), 0.1*(rand%10 - 5); gravedad 0xBD23D70A; giros (rand&0x3FF)-0x1FF.
    // `base` = (0, 0, 0.2) transformado por la matriz de camara (func_1AFC00), lo calcula el motor.
    static void initPiece(DebrisPiece& p, const float base[3], float floorY, const TrnRand& r);
    // ctrl (0x21C0E0), parte logica. Devuelve true si debe cerrarse (>= 0xE11 fotogramas). El dibujo solo ocurre si alpha > 0.
    bool ctrl(DebrisPiece* pieces, int n, bool paused);
    bool visible() const { return alpha > 0.0f; }
};

struct DishRow { uint32_t id; int count; int motion; uint32_t model; };
extern const DishRow kDishRows[16];
// KomonoTrnDishKowareTask (0x21C5B0).
struct DishKoware {
    uint32_t obj = 0;         // +0x48
    int   count = 0;          // +0x40
    int   frames = 0;         // +0x78
    float alpha = 1.0f;       // +0x80
    float scale84 = 1.0f;     // +0x84 (0.1 en el objeto 0x1B0079; uso no visto)
    float mult = 2.0f;        // +0x88 (3.0 en el objeto 0x1B0079)
    // newPtcl / busqueda en tabla: fila del objeto o nullptr.
    static const DishRow* findRow(uint32_t obj);
    // Parte de __ct__: numero de particulas y constantes segun objeto.
    void setup(uint32_t objId);
    // Valores iniciales de una pieza (4 azares: dos angulos de 16 bits x2 y tres giros); trig inyectado.
    void initPiece(DebrisPiece& p, const TrnRand& r, const TrnTrig& t) const;
    // ctrl (0x21C...): true = cerrar. Objeto 0x1B0079: desvanece desde 0x29 y cierra a 0x3D; resto: desde 0x15 y a 0x29.
    bool ctrl(DebrisPiece* pieces, int n, bool paused);
};

}

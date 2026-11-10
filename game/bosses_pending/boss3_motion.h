// Boss3 (overlay bs3mod): resolucion del indice de movimiento fuera de la escena 0x14.
// Contexto: game/bosses_a/boss3.cpp trataba kBs3Attack[].motion (324, 304, ...) como indice de kBs3Motion[10] y lo acotaba.
// Lo que dice el asm (StartAttack 0x9C66D0, ControlAttack 0x9C60B0, MotSupSet2 0x9C94A0): sin ninguna comprobacion de limites,
//  - escena 0x14 (la pelea): el valor es directamente el id de animacion (324 = 0x144...) -> func_1FA540(obj, pos, valor);
//  - cualquier otra escena: es un INDICE en D_009D0880 (8 B por entrada = {animacion, longitud}). La tabla tiene 13 entradas validas
//    (0..12; la 11 y la 12 faltaban en boss_data.cpp, que solo trae 10). Con un valor >= 13 el original lee datos ajenos del overlay,
//    asi que los ataques 0..8 de kBs3Attack (motion 304..324) solo pueden ejecutarse en la escena 0x14 y los 9..19 (motion 4..8) en otra.
#pragma once
#include "../bosses_a/boss3.h"
#include "boss_pending_data.h"
namespace vn { namespace bosspend {
using namespace vn::boss;

constexpr int kBs3MotionCount = 13;       // entradas validas de D_009D0880
constexpr int kBs3SceneFight = 0x14;

struct Boss3Motion {
    // Entrada de D_009D0880 (false si idx esta fuera de 0..12: en el original seria una lectura de datos ajenos).
    static bool lookup(int idx, B3MotionEntry& out);
    // Id de animacion que ControlAttack compara con +0x34: motion (escena 0x14) o D_009D0880[motion].anim. ok = false si el
    // indice esta fuera de la tabla fuera de la escena 0x14 (comportamiento no definido en el original).
    static int expectedAnim(int scene, int attackMotion, bool& ok);
    // MotSupSet2 (0x9C94A0) completo: acepta tambien los indices 10..12. Devuelve false si no hizo nada porque idx no esta en la tabla
    // (solo fuera de la escena 0x14); true en el resto de casos (incluido "ya estaba en ese movimiento").
    static bool motSupSet2(Bs3State& s, Boss3Engine& g, int idx, int blend, int mode);
};

}}

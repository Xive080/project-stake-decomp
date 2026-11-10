// Tablas extraidas de los ficheros reales por tools/gen_tables.py (ver boss_rest_data.cpp).
#pragma once
#include <cstdint>
namespace vn { namespace boss {
// Boss3Task D_009D0A20: [fase +0x1DAD][dificultad 0..33][0 = golpes por impacto (+0x1EAF/+0x1EB1), 1 = golpes para aturdir (+0x1EB0/+0x1EB2)]
extern const int8_t kBs3HitRows[3][34][2];
// Boss2Task D_009D6250 (bs2mod): cuanto avanza el puntero de secuencia cada operacion (en s16) si no cede. Las 5 primeras entradas coinciden con kBs2CancelSkip.
extern const int8_t kBs2OpLen[30];
}} // namespace

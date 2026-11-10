// GENERADO: tablas leidas de disc/TBL/GAMEMOD.BIN (vram = offset + 0x92B000) y baserom/SCES_504.11 (offset = v - 0x140000 + 0x800).
// No editar a mano; el test (game/test/enemies_rest_test.cpp) las vuelve a leer de los ficheros reales y las compara.
#include "enemies_rest.h"
namespace vn { namespace en { namespace rest {
// D_009A22C0[tipo] = direccion en el ELF (0x2D8860 + 4*i); cada entrada del ELF apunta a una lista de s16 en GAMEMOD (D_009B5A70..).
const uint8_t kWaitMotBase[18] = {0,1,2,3,2,3,4,5,6,6,7,8,9,9,10,11,12,13};
// Entradas 0..24 de la tabla ELF 0x2D8860 (indice = base[tipo] + subTipo): primeras dos animaciones de la lista (Wait elige rand%2).
const int16_t kWaitMotPairs[25][2] = {{15,15},{31,31},{122,122},{175,175},{197,197},{975,975},{806,806},{777,777},{6023,6023},{223,223},{690,690},{720,720},{773,773},{765,765},{19,95},{71,47},{139,57},{183,95},{957,60},{778,100},{6020,120},{205,40},{698,12},{710,40},{756,7}};
// D_009A2470[tipo] -> 2 s8 (offsets 0 y 4 de la ficha) pasados a EnPartsChain al morir.
const int8_t kEkndChainParam[18][2] = {{10,40},{10,40},{5,10},{10,40},{10,40},{10,40},{10,40},{10,40},{10,40},{10,40},{10,40},{5,10},{5,10},{5,10},{5,10},{5,10},{10,40},{10,40}};
// D_009A23D0[tipo] -> D_009A23C0[subTipo 0..3] -> D_009B5D50 (los 18 tipos y los 4 subtipos coinciden): animacion de dano por clase de parte (D_009ACB10).
const int16_t kDamageMotByClass[16] = {132,132,132,132,132,132,132,132,257,1,0,0,0,0,0,0};
}}}  // namespace

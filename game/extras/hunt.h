// Cuaderno de caza (hntmod, HuntFileTask) y galerias: logica de datos reescrita a mano.
// Datos de guardado en 0x502750..0x5027D5 (133 B): un byte por pagina, bit0 = pagina desbloqueada.
#pragma once
#include <cstdint>
namespace vn {

enum { HUNT_BOOKS = 5 };
// Paginas por libro (bytes en el guardado): libro 0 = 11 (0x502750), 1 = 15 (0x50275B), 2 = 16 (0x50276A),
// 3 = 52 (0x50277A, galeria de objetos 1..52), 4 = 39 (0x5027AE, galeria "otros": las 7 primeras no son aleatorias).
extern const int kHuntBookPages[HUNT_BOOKS];
struct HuntSave { int8_t page[HUNT_BOOKS][52] = {}; };    // solo se usan las primeras kHuntBookPages[b] de cada libro

// Tablas de hntmod: D_009CB280 (primera pagina global de cada libro) y D_009CB294 (ultima pagina "simple" del libro).
extern const int kHuntBookFirst[HUNT_BOOKS];
extern const int kHuntBookSimpleEnd[HUNT_BOOKS];

struct HuntLogic {
    // HFileAddPageG 0x1D38B0: marca la pagina `idx` del libro; 1 = recien desbloqueada, 0 = ya estaba o libro no valido.
    // El libro 3 usa idx-1 (el numero de objeto empieza en 1).
    static int addPage(HuntSave& s, int book, int idx);
    // HFileAddOtherGallery 0x1D3950: desbloquea al azar una pagina aun sin ver de la galeria 4 (paginas 7..38). `rand` = nlRand().
    static int addOtherGallery(HuntSave& s, int (*rand)());
    // HuntFileTask::CheckBookComp 0x9C55D0: todas las paginas del libro con byte != 0 (libro desconocido = 1).
    static int checkBookComp(const HuntSave& s, unsigned book);
    // HuntFileTask::CheckNoData 0x9C7710 (book = this+0x14, side = 0/1 pagina izquierda/derecha, page = pagina global).
    // Devuelve 1 si la pagina NO tiene datos (se dibuja vacia), 0 si los tiene.
    static int checkNoData(const HuntSave& s, int book, unsigned side, int page);
    // calcLinePos 0x9C5AD0: interpolacion lineal entre a y b en el paso i de n.
    static float calcLinePos(float a, float b, int i, float n);
};
}

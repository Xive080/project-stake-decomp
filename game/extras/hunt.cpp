#include "hunt.h"
namespace vn {
const int kHuntBookPages[HUNT_BOOKS] = {11, 15, 16, 52, 39};
const int kHuntBookFirst[HUNT_BOOKS] = {0, 8, 0x17, 0x24, 0x3E};        // overlay hntmod 0x9CB280
const int kHuntBookSimpleEnd[HUNT_BOOKS] = {4, 0x16, 0x20, 0x3D, 0x44};  // overlay hntmod 0x9CB294

int HuntLogic::addPage(HuntSave& s, int book, int idx) {
    if (book < 0 || book >= HUNT_BOOKS) return 0;
    if (book == 3) idx -= 1;
    // El original no comprueba limites; fuera de rango se descarta aqui para no escribir fuera del guardado.
    if (idx < 0 || idx >= kHuntBookPages[book]) return 0;
    int8_t& b = s.page[book][idx];
    if (b & 1) return 0;
    b = (int8_t)(b | 1);
    return 1;
}

int HuntLogic::addOtherGallery(HuntSave& s, int (*rand)()) {
    int cand[32]; int n = 0;
    for (int i = 0; i < 32; i++) if (!(s.page[4][7 + i] & 1)) cand[n++] = i;
    if (n <= 0) return 0;
    int pick = cand[(rand() & 0x7FFF) % n];
    return addPage(s, 4, pick + 7);
}

int HuntLogic::checkBookComp(const HuntSave& s, unsigned book) {
    if (book >= HUNT_BOOKS) return 1;
    for (int i = 0; i < kHuntBookPages[book]; i++) if (s.page[book][i] == 0) return 0;
    return 1;
}

int HuntLogic::checkNoData(const HuntSave& s, int book, unsigned side, int page) {
    if (book < 0 || book >= HUNT_BOOKS) return 1;
    int simpleEnd = kHuntBookSimpleEnd[book], first = kHuntBookFirst[book];
    auto has = [&](int b, int idx) { return idx >= 0 && idx < kHuntBookPages[b] && s.page[b][idx] != 0; };
    if (simpleEnd < page) {                 // paginas "dobles": dos bytes por hoja (derecha primero)
        if (book == 0 || book == 2 || book == 4) {
            if (side > 1) return 1;
            // base de la zona doble = paginas simples + 1 (0:5, 2:10, 4:7); idx = base + 2*(page-simpleEnd-1) + (side==1 ? 0 : 1)
            static const int base[3] = {5, 10, 7};
            int bi = book == 0 ? 0 : (book == 2 ? 1 : 2);
            int idx = base[bi] + 2 * (page - simpleEnd - 1) + (side == 1 ? 0 : 1);
            return has(book, idx) ? 0 : 1;
        }
        return 1;                           // libros 1 y 3: sin zona doble
    }
    int rel = page - first;                 // zona "simple" (y libro 3, dos bytes por pagina)
    if (book == 3) {
        if (side > 1) return 1;
        return has(3, 2 * rel + (side == 1 ? 0 : 1)) ? 0 : 1;
    }
    return has(book, rel) ? 0 : 1;
}

float HuntLogic::calcLinePos(float a, float b, int i, float n) {
    float fi = (float)i;
    return (fi * b) / n + (a * (n - fi)) / n;
}
}

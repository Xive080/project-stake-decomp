// Fondo de fase (BGCtrlClass, ELF 0x1C8710..0x1C8B40): solo la parte de estado. El dibujo (PutForEntry*, culling, PutPolyMatF) no se reescribe.
#pragma once
#include <cstdint>
#include "scriptmem.h"
namespace vn {
struct BGCtrl {
    int16_t cur = 0;    // +8  fondo actual
    int16_t prev = 0;   // +0xA fondo anterior
    // appointBG (0x1C8B10): BGTB del script [op, fondo]; devuelve el pc siguiente (pc + 8).
    uint32_t appointBG(const ScriptMem& m, uint32_t pc) { prev = cur; cur = (int16_t)m.rd(pc + 4); return pc + 8; }
    // appointBGImm (0x1C8B30)
    void appointBGImm(int16_t bg) { prev = cur; cur = bg; }
};
}

// Memoria de scripts: los scripts de evento (EVTPAL.BIN, base RAM 0x1FC2000) y los registros de camara usan punteros absolutos de RAM
// del PS2 dentro de los propios datos. Esta interfaz permite leer palabras de 32 bits por esa direccion sin depender de la plataforma.
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace vn {
struct ScriptMem {
    virtual ~ScriptMem() = default;
    virtual uint32_t rd(uint32_t addr) const = 0;      // palabra de 32 bits en `addr` (alineada a 4); 0xFFFFFFFF si fuera de rango
};
struct VecMem : ScriptMem {
    uint32_t base = 0; std::vector<uint32_t> w;
    VecMem() = default;
    VecMem(uint32_t b, std::vector<uint32_t> v) : base(b), w(std::move(v)) {}
    uint32_t rd(uint32_t a) const override {
        if (a < base || ((a - base) & 3)) return 0xFFFFFFFFu;
        size_t i = (a - base) / 4; return i < w.size() ? w[i] : 0xFFFFFFFFu;
    }
    uint32_t addrOf(size_t wordIndex) const { return base + (uint32_t)(wordIndex * 4); }
};
}

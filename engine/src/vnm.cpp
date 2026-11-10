#include "vnm.h"
#include <cstdio>
#include <cstring>
namespace vn {
bool loadVNM(const std::string& path, Bank& out) {
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return false;
    char magic[4]; bool v2; uint32_t nm, nt;
    if (fread(magic, 1, 4, f) != 4 || (memcmp(magic, "VNM1", 4) && memcmp(magic, "VNM2", 4)) || fread(&nm, 4, 1, f) != 1 || fread(&nt, 4, 1, f) != 1) { fclose(f); return false; }
    v2 = magic[3] == '2';
    static_assert(sizeof(Vertex) == 28, "Vertex debe medir 28 bytes");
    out.ntex = (int)nt;
    for (uint32_t i = 0; i < nm; i++) {
        Mesh m; uint32_t nv, ni;
        if (fread(&m.block, 4, 1, f) != 1 || fread(&m.tex, 4, 1, f) != 1 || fread(&nv, 4, 1, f) != 1 || fread(&ni, 4, 1, f) != 1 || (v2 && fread(&m.blend, 4, 1, f) != 1)) { fclose(f); return false; }
        m.verts.resize(nv); m.idx.resize(ni);
        if (fread(m.verts.data(), sizeof(Vertex), nv, f) != nv || fread(m.idx.data(), 4, ni, f) != ni) { fclose(f); return false; }
        if ((m.block & 0xFFFFFF) > out.maxBlock) out.maxBlock = m.block & 0xFFFFFF;   // bit 24 = sigue a la camara (flag 0x2 de la entrada)
        out.meshes.push_back(std::move(m));
    }
    fclose(f); return true;
}
}

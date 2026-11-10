// Formato VNM1 (ver tools/vnm_export.py): mallas de un banco de objetos ya trianguladas.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace vn {
struct Vertex { float x, y, z, u, v; uint8_t r, g, b, a; int8_t nx, ny, nz, pad; };
struct Mesh { int32_t block, tex; uint32_t blend = 0;  /* bits0-1: 0 opaco, 1 alfa, 2 aditivo; bit2 As fijo; bits8-15 FIX */ std::vector<Vertex> verts; std::vector<uint32_t> idx; };
struct Bank { std::vector<Mesh> meshes; int ntex = 0; int maxBlock = 0; };
bool loadVNM(const std::string& path, Bank& out);
}

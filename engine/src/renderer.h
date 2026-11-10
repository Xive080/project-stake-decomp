#pragma once
#include "vnm.h"
#include <string>
#include <vector>
namespace vn {
struct Renderer {
    bool init();
    bool loadBank(const std::string& dir);          // dir con bank.vnm y tex_NN.png
    void draw(int w, int h, float yaw, float pitch, float dist, const float center[3], int block, bool textured);
    void drawCam(int w, int h, const float eye[3], const float target[3], float fovDeg, bool textured);   // camara de ruta (Y arriba)
    Bank bank; float bmin[3], bmax[3];
private:
    struct GpuMesh { unsigned vao, vbo, ebo; int n, tex, block; unsigned blend; };
    std::vector<GpuMesh> gm; std::vector<unsigned> tex; unsigned prog = 0, white = 0;
    void drawMeshes(int w, int h, const float* M, int block, bool textured);
    float skyM[16] = {}; bool haveSky = false;
public: float skyPitch = 0.f;
private:
    int uSky = -1, uMVP = -1, uTex = -1, uUseTex = -1, uFix = -1;
};
}

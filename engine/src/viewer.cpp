// Visor de bancos de objetos: arrastrar = orbitar, rueda/+- = zoom, flechas = cambiar bloque, T = texturas, Esc = salir.
// Argumentos: vn_viewer DIR [--cam] [--frame N] [--shot ARCHIVO.bmp]  (--cam reproduce DIR/cam.txt: ruta de camara, ver tools/afs_cam_export.py)
// Antes: vn_viewer DIR [--shot ARCHIVO.bmp]  (DIR con bank.vnm). --shot guarda una captura y sale (prueba sin interaccion).
#include <array>
#include <vector>
#include <string>
#include <cstdlib>
#include "renderer.h"
#include "gl.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "uso: vn_viewer DIR [--cam] [--frame N] [--size WxH] [--shot out.bmp]\n"); return 1; }
    const char* shot = nullptr; bool camMode = false; float skyDeg = 0; int startFrame = 0, winW = 1024, winH = 720;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
        else if (!strcmp(argv[i], "--cam")) camMode = true;
        else if (!strcmp(argv[i], "--skypitch") && i + 1 < argc) skyDeg = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--size") && i + 1 < argc) sscanf(argv[++i], "%dx%d", &winW, &winH);
        else if (!strcmp(argv[i], "--frame") && i + 1 < argc) startFrame = atoi(argv[++i]);
    }
    std::vector<std::array<float, 7>> cam;        // ex ey ez tx ty tz fov por cuadro
    if (camMode) {
        FILE* cf = fopen((std::string(argv[1]) + "/cam.txt").c_str(), "r");
        if (!cf) { fprintf(stderr, "falta %s/cam.txt\n", argv[1]); return 1; }
        std::array<float, 7> q; while (fscanf(cf, "%f %f %f %f %f %f %f", &q[0], &q[1], &q[2], &q[3], &q[4], &q[5], &q[6]) == 7) cam.push_back(q);
        fclose(cf); if (cam.empty()) { fprintf(stderr, "cam.txt vacio\n"); return 1; }
    }
    float camF = (float)startFrame; bool paused = false;
    if (!SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
#if VN_GL_ES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window* win = SDL_CreateWindow("Vampire Night - visor", winW, winH, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!win) { fprintf(stderr, "ventana: %s\n", SDL_GetError()); return 1; }
    SDL_GLContext ctx = SDL_GL_CreateContext(win); if (!ctx) { fprintf(stderr, "GL: %s\n", SDL_GetError()); return 1; }
    SDL_GL_SetSwapInterval(1);
    vn::Renderer r; if (!r.init()) { fprintf(stderr, "init renderer fallo\n"); return 1; }
    if (!r.loadBank(argv[1])) { fprintf(stderr, "no se pudo cargar %s\n", argv[1]); return 1; }
    float c[3]; for (int k = 0; k < 3; k++) c[k] = 0.5f * (r.bmin[k] + r.bmax[k]);
    float ext = std::fmax(r.bmax[0] - r.bmin[0], std::fmax(r.bmax[1] - r.bmin[1], r.bmax[2] - r.bmin[2]));
    float yaw = 0.4f, pitch = 0.2f, dist = ext * 1.6f + 1.f; int block = -1; bool textured = true, run = true; int frames = 0;
    printf("%zu mallas, %d texturas, bloques 0..%d, extension %.1f\n", r.bank.meshes.size(), r.bank.ntex, r.bank.maxBlock, ext);
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) run = false;
            else if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) { yaw += e.motion.xrel * 0.01f; pitch += e.motion.yrel * 0.01f; }
            else if (e.type == SDL_EVENT_MOUSE_WHEEL) dist *= powf(0.9f, e.wheel.y);
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                    case SDLK_ESCAPE: run = false; break;
                    case SDLK_T: textured = !textured; break;
                    case SDLK_SPACE: paused = !paused; break;
                    case SDLK_PERIOD: camF += 1; break; case SDLK_COMMA: camF -= 1; break;
                    case SDLK_RIGHT: if (block < r.bank.maxBlock) block++; break;
                    case SDLK_LEFT: if (block >= 0) block--; break;
                    case SDLK_EQUALS: dist *= 0.9f; break; case SDLK_MINUS: dist /= 0.9f; break;
                }
                if (block >= 0) { SDL_Log("bloque %d", block); }
            }
        }
        int w, h; SDL_GetWindowSizeInPixels(win, &w, &h);
        if (camMode) {
            if (!paused && !(shot)) camF += 0.5f;                 // ~30 cuadros por segundo con vsync a 60 Hz
            int n = (int)cam.size(); int i = ((int)camF % n + n) % n; auto& q = cam[i];
            r.skyPitch = skyDeg * 3.14159265f / 180.f; r.drawCam(w, h, &q[0], &q[3], q[6], textured);
        } else r.draw(w, h, yaw, pitch, dist, c, block, textured);
        if (shot && frames == 3) {
            std::vector<unsigned char> px((size_t)w * h * 4); glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            for (int y = 0; y < h / 2; y++) for (int x = 0; x < w * 4; x++) std::swap(px[(size_t)y * w * 4 + x], px[(size_t)(h - 1 - y) * w * 4 + x]);
            SDL_Surface* s = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px.data(), w * 4); SDL_SaveBMP(s, shot); SDL_DestroySurface(s); run = false;
        }
        SDL_GL_SwapWindow(win); frames++;
    }
    SDL_Quit(); return 0;
}

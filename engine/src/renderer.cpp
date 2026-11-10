#include "renderer.h"
#include "gl.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace vn {
#ifdef VN_NEED_LOADER
#define GLF(ret, name, args) typedef ret (APIENTRY *PFN_##name) args; static PFN_##name name;
// Solo las funciones GL > 1.1 que usa el renderer.
GLF(GLuint, glCreateShader, (GLenum)) GLF(void, glShaderSource, (GLuint, GLsizei, const char* const*, const GLint*)) GLF(void, glCompileShader, (GLuint))
GLF(void, glGetShaderiv, (GLuint, GLenum, GLint*)) GLF(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei*, char*)) GLF(GLuint, glCreateProgram, ())
GLF(void, glAttachShader, (GLuint, GLuint)) GLF(void, glLinkProgram, (GLuint)) GLF(void, glUseProgram, (GLuint)) GLF(GLint, glGetUniformLocation, (GLuint, const char*))
GLF(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat*)) GLF(void, glUniform1i, (GLint, GLint)) GLF(void, glUniform1f, (GLint, GLfloat)) GLF(void, glGenVertexArrays, (GLsizei, GLuint*))
GLF(void, glBindVertexArray, (GLuint)) GLF(void, glGenBuffers, (GLsizei, GLuint*)) GLF(void, glBindBuffer, (GLenum, GLuint)) GLF(void, glBufferData, (GLenum, GLsizeiptr, const void*, GLenum))
GLF(void, glEnableVertexAttribArray, (GLuint)) GLF(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*))
GLF(void, glActiveTexture, (GLenum)) GLF(void, glGenerateMipmap, (GLenum)) GLF(void, glBindAttribLocation, (GLuint, GLuint, const char*))
bool loadGL() {
#define L(n) n = (PFN_##n)SDL_GL_GetProcAddress(#n); if (!n) return false;
    L(glCreateShader) L(glShaderSource) L(glCompileShader) L(glGetShaderiv) L(glGetShaderInfoLog) L(glCreateProgram) L(glAttachShader) L(glLinkProgram) L(glUseProgram)
    L(glGetUniformLocation) L(glUniformMatrix4fv) L(glUniform1i) L(glUniform1f) L(glGenVertexArrays) L(glBindVertexArray) L(glGenBuffers) L(glBindBuffer) L(glBufferData)
    L(glEnableVertexAttribArray) L(glVertexAttribPointer) L(glActiveTexture) L(glGenerateMipmap) L(glBindAttribLocation)
    return true;
}
#else
bool loadGL() { return true; }
#endif

static const char* VS = R"(
layout(location=0) in vec3 aPos; layout(location=1) in vec2 aUV; layout(location=2) in vec4 aCol; layout(location=3) in vec3 aNrm;
uniform mat4 uMVP; out vec2 vUV; out vec4 vCol; out float vL;
void main(){ gl_Position = uMVP * vec4(aPos,1.0); vUV = aUV; vCol = aCol;
  vec3 n = normalize(aNrm); vL = 0.45 + 0.55 * max(dot(n, normalize(vec3(0.4,0.8,0.5))), 0.0); })";
static const char* FS = R"(
precision mediump float; in vec2 vUV; in vec4 vCol; in float vL; uniform sampler2D uTex; uniform int uUseTex; uniform int uSky; uniform float uFix; out vec4 o;
void main(){ vec4 t = uUseTex != 0 ? texture(uTex, uSky != 0 ? vec2(vUV.x, 1.0 - vUV.y) : vUV) : vec4(0.75); if (t.a < 0.02) discard;
  vec3 c = uSky != 0 ? t.rgb : t.rgb * vec3(0.65, 0.64, 0.66);
  o = vec4(c, uFix >= 0.0 ? uFix : t.a); })";   // luz plana 0.65 medida en ST1 (RAM: sin luz direccional ni niebla GS); el cielo sin escalar

static unsigned compile(GLenum type, const std::string& src) {
    unsigned s = glCreateShader(type); const char* p = src.c_str(); glShaderSource(s, 1, &p, nullptr); glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(s, sizeof log, nullptr, log); fprintf(stderr, "shader: %s\n", log); return 0; }
    return s;
}

bool Renderer::init() {
    if (!loadGL()) return false;
    std::string head = VN_GL_ES ? "#version 300 es\n" : "#version 330 core\n";
    unsigned vs = compile(GL_VERTEX_SHADER, head + VS), fs = compile(GL_FRAGMENT_SHADER, head + FS); if (!vs || !fs) return false;
    prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    uMVP = glGetUniformLocation(prog, "uMVP"); uTex = glGetUniformLocation(prog, "uTex"); uUseTex = glGetUniformLocation(prog, "uUseTex"); uSky = glGetUniformLocation(prog, "uSky"); uFix = glGetUniformLocation(prog, "uFix");
    glEnable(GL_DEPTH_TEST); return true;
}

bool Renderer::loadBank(const std::string& dir) {
    if (!loadVNM(dir + "/bank.vnm", bank)) return false;
    for (int i = 0; i < bank.ntex; i++) {
        char name[64]; snprintf(name, sizeof name, "/tex_%02d.png", i); int w, h, c; unsigned id = 0;
        unsigned char* px = stbi_load((dir + name).c_str(), &w, &h, &c, 4);
        if (px) { glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px); glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); stbi_image_free(px); }
        tex.push_back(id);
    }
    for (int k = 0; k < 3; k++) { bmin[k] = 1e30f; bmax[k] = -1e30f; }
    for (auto& m : bank.meshes) {
        GpuMesh g{}; g.n = (int)m.idx.size(); g.tex = m.tex; g.block = m.block; g.blend = m.blend;
        glGenVertexArrays(1, &g.vao); glBindVertexArray(g.vao);
        glGenBuffers(1, &g.vbo); glBindBuffer(GL_ARRAY_BUFFER, g.vbo); glBufferData(GL_ARRAY_BUFFER, m.verts.size() * sizeof(Vertex), m.verts.data(), GL_STATIC_DRAW);
        glGenBuffers(1, &g.ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.idx.size() * 4, m.idx.data(), GL_STATIC_DRAW);
        const GLsizei S = sizeof(Vertex);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, S, (void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, S, (void*)12);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, S, (void*)20);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_BYTE, GL_TRUE, S, (void*)(20 + 4));
        gm.push_back(g);
        for (auto& v : m.verts) { float p[3] = {v.x, v.y, v.z}; for (int k = 0; k < 3; k++) { if (p[k] < bmin[k]) bmin[k] = p[k]; if (p[k] > bmax[k]) bmax[k] = p[k]; } }
    }
    return true;
}

static void mul(const float* a, const float* b, float* o) { for (int c = 0; c < 4; c++) for (int r = 0; r < 4; r++) { float s = 0; for (int k = 0; k < 4; k++) s += a[k * 4 + r] * b[c * 4 + k]; o[c * 4 + r] = s; } }

void Renderer::draw(int w, int h, float yaw, float pitch, float dist, const float c[3], int block, bool textured) {
    float asp = (float)w / (float)h, f = 1.0f / tanf(0.5f * 0.9f), n = 0.1f, fa = 30000.f;
    float P[16] = {f / asp, 0, 0, 0, 0, f, 0, 0, 0, 0, (fa + n) / (n - fa), -1, 0, 0, 2 * fa * n / (n - fa), 0};
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    float Ry[16] = {cy, 0, -sy, 0, 0, 1, 0, 0, sy, 0, cy, 0, 0, 0, 0, 1}, Rx[16] = {1, 0, 0, 0, 0, cp, sp, 0, 0, -sp, cp, 0, 0, 0, 0, 1};
    float T[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -c[0], -c[1], -c[2], 1}, D[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -dist, 1};
    float A[16], B[16], C[16], M[16]; mul(Ry, T, A); mul(Rx, A, B); mul(D, B, C); mul(P, C, M);
    haveSky = false; drawMeshes(w, h, M, block, textured);
}

void Renderer::drawCam(int w, int h, const float e[3], const float t[3], float fovDeg, bool textured) {
    // Vista tipo lookAt con el fov del registro de camara (tomado como vertical, sin confirmar).
    float fz[3] = {t[0] - e[0], t[1] - e[1], t[2] - e[2]}; float l = sqrtf(fz[0] * fz[0] + fz[1] * fz[1] + fz[2] * fz[2]); if (l < 1e-6f) l = 1;
    for (float& v : fz) v /= l;
    float up[3] = {0, 1, 0}, rx[3] = {fz[1] * up[2] - fz[2] * up[1], fz[2] * up[0] - fz[0] * up[2], fz[0] * up[1] - fz[1] * up[0]};
    float lr = sqrtf(rx[0] * rx[0] + rx[1] * rx[1] + rx[2] * rx[2]); if (lr < 1e-6f) { rx[0] = 1; rx[1] = rx[2] = 0; lr = 1; } for (float& v : rx) v /= lr;
    float ry[3] = {rx[1] * fz[2] - rx[2] * fz[1], rx[2] * fz[0] - rx[0] * fz[2], rx[0] * fz[1] - rx[1] * fz[0]};
    float V[16] = {rx[0], ry[0], -fz[0], 0, rx[1], ry[1], -fz[1], 0, rx[2], ry[2], -fz[2], 0,
                   -(rx[0] * e[0] + rx[1] * e[1] + rx[2] * e[2]), -(ry[0] * e[0] + ry[1] * e[1] + ry[2] * e[2]), (fz[0] * e[0] + fz[1] * e[1] + fz[2] * e[2]), 1};
    float fv = fminf(fmaxf(fovDeg, 10.f), 120.f) * 3.14159265f / 180.f, asp = (float)w / (float)h, f = 1.0f / tanf(0.5f * fv), n = 1.f, fa = 30000.f;
    float P[16] = {f / asp, 0, 0, 0, 0, f, 0, 0, 0, 0, (fa + n) / (n - fa), -1, 0, 0, 2 * fa * n / (n - fa), 0}, M[16]; mul(P, V, M);
    // Fondo (PutForEntry_st1, flag 0x2): el juego usa la MISMA vista que el mundo y solo traslada la cupula a (cam.x, 0, cam.z)
    // (cam+0x90/+0x98, y=0); se dibuja antes del mundo y despues se borra el z. skyPitch != 0 fuerza un cabeceo distinto (depuracion).
    { float Tm[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, e[0], 0, e[2], 1};
      if (skyPitch == 0.f) { mul(M, Tm, skyM); }
      else { float hl = sqrtf(fz[0] * fz[0] + fz[2] * fz[2]); if (hl < 1e-6f) hl = 1; float cp = cosf(skyPitch), sp = sinf(skyPitch);
        float sf[3] = {fz[0] / hl * cp, sp, fz[2] / hl * cp}, sr[3] = {-sf[2], 0, sf[0]};
        float lr2 = sqrtf(sr[0] * sr[0] + sr[2] * sr[2]); sr[0] /= lr2; sr[2] /= lr2;
        float su[3] = {sr[1] * sf[2] - sr[2] * sf[1], sr[2] * sf[0] - sr[0] * sf[2], sr[0] * sf[1] - sr[1] * sf[0]};
        float Vs[16] = {sr[0], su[0], -sf[0], 0, sr[1], su[1], -sf[1], 0, sr[2], su[2], -sf[2], 0, 0, 0, 0, 1}; mul(P, Vs, skyM); }
      haveSky = true; }
    drawMeshes(w, h, M, -1, textured);
}

void Renderer::drawMeshes(int w, int h, const float* M, int block, bool textured) {
    glViewport(0, 0, w, h); glClearColor(0.27f, 0.35f, 0.47f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(prog); glUniformMatrix4fv(uMVP, 1, GL_FALSE, M); glUniform1i(uTex, 0);
    // Pases: (cielo si lo hay, luego escena); en cada uno primero opacos y al final los translucidos (sin escribir z).
    for (int pass = 0; pass < 2; pass++) {
      if (pass == 1 && haveSky) glClear(GL_DEPTH_BUFFER_BIT);      // el fondo (cielo) va con su propia camara y detras de todo
      for (int tr = 0; tr < 2; tr++) {
        if (tr) { glEnable(GL_BLEND); glDepthMask(GL_FALSE); }
        for (auto& g : gm) {
          bool sky = (g.block & 0x1000000) != 0, trans = (g.blend & 3) != 0;
          if (trans != (tr == 1)) continue;
          if (block >= 0 && (g.block & 0xFFFFFF) != block) continue;
          if (haveSky ? (sky != (pass == 0)) : pass == 1) continue;
          if (trans) { if ((g.blend & 3) == 1) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); else glBlendFunc(GL_SRC_ALPHA, GL_ONE); }
          glUniform1f(uFix, (g.blend & 4) ? (float)((g.blend >> 8) & 255) / 128.f : -1.f);
          glUniformMatrix4fv(uMVP, 1, GL_FALSE, sky && haveSky ? skyM : M); glUniform1i(uSky, sky && haveSky ? 1 : 0);
          bool use = textured && g.tex >= 0 && g.tex < (int)tex.size() && tex[g.tex];
          glUniform1i(uUseTex, use ? 1 : 0); glActiveTexture(GL_TEXTURE0); if (use) glBindTexture(GL_TEXTURE_2D, tex[g.tex]);
          glBindVertexArray(g.vao); glDrawElements(GL_TRIANGLES, g.n, GL_UNSIGNED_INT, nullptr);
        }
        if (tr) { glDisable(GL_BLEND); glDepthMask(GL_TRUE); }
      }
    }
}
}

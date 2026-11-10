// flow3: camara de juego, rutina por fotograma game_000 (CameraMgrInternalClass, 0x1CA190).
// Hereda de CameraGeo (game/flow_rest/camera.h) sin tocarlo: CH_GAME000, que alli queda "pendiente", se despacha aqui.
// Las partes que no estan reescritas (calcGameCam 0x1CA6C0, calcGameInt) y las matematicas del motor (nlTan, VecCos,
// InterDivCamAng3D) se inyectan como virtuales; gp-0x73CC (branchNo) tambien.
#pragma once
#include "../flow_rest/camera.h"
namespace vn {
struct CameraGeo3 : CameraGeo {
    using CameraGeo::CameraGeo;
    int branchNoV = 0;                                   // gp-0x73CC (s8): elige el sub-registro k = branchNo (0..2)
    int outOfRange = 0;                                  // veces que branchNo no estaba en 0..2 (el original no lo acota)
    // --- inyectados (no reescritos / motor) ---
    virtual void calcGameCam(int s) { (void)s; }         // game_calcGameCam 0x1CA6C0: PENDIENTE
    virtual void calcGameInt(int s) { (void)s; }         // game_calcGameInt: PENDIENTE
    virtual float nlTan(int ang16) = 0;                  // nlTan (angulo de 16 bits)
    virtual float vecCos(const float a[3], const float b[3]) = 0;           // VecCos 0x1C9...
    virtual void interDivCamAng3D(const Anpo& pos, const float tgtLook[3], const Anpo& look, float out[3], int a, int afterSpeed) = 0;
    void game000(int s);                                 // 0x1CA190
    void runHandler(int h, int s) override;
};
}

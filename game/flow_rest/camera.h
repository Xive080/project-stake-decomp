// Matematica de camara reescrita a mano (CameraMgrInternalClass ELF 0x1C9830..0x1CB3F0, CamPathMgr 0x1CC590, CalcFCurve 0x1CC410).
// Complementa game/flow/camjob.* (cola de trabajos). Los campos de las estructuras del original (D_308740 + 0x240*ranura, y la tabla de
// camaras de 0xD0 B en gp-0x7338) se nombran por uso con su offset en comentario. El motor (arctangente, angulos, modo demo, tablas de datos
// de ruta) entra por interfaces; no hay dependencias de PS2 ni de plataforma.
//
// Sin reescribir (pendiente, ver MANIFEST): game_000 / game_calcGameCam / game_calcGameInt (camara de juego: interpolaciones angulares con
// InterDivCamAng3D, VecCos, nlTan...), getObjectPathS (matrices + GetRotZYX), takeOver, CamPathMgr::entryExec_*/freeExec/entryStage (carga de datos).
#pragma once
#include <cstdint>
#include "../flow/camjob.h"
namespace vn {

// ---- CalcFCurve (ELF 0x1CC410): curva CPFRM ----
// Disposicion en memoria: u16 n (+0), u16 iters (+2), luego n claves de 16 B {t, valor, tangente de salida, tangente de entrada}.
struct FCurve {
    int n = 0, iters = 0;
    const float* keys = nullptr;                     // keys[4*i + {0,1,2,3}]
    static FCurve fromBytes(const uint8_t* p);       // lee la cabecera u16 n, u16 iters; las claves van a continuacion (little endian, alineado a 4)
};
// Hermite cubico entre claves, constante tras la ultima. Reproduce el orden de las operaciones del original (mula/madd/adda del EE)
// con floats IEEE: el FPU de PS2 trunca y no maneja NaN/denormales, asi que puede diferir en el ultimo bit.
// Diferencias con el original (el original lee fuera de la curva): el indice de la busqueda binaria se acota a [0, n-1], y si x <= t de la clave 0
// el original interpolaria con memoria anterior a la curva; aqui solo se cubre el caso en que la busqueda cae en la clave 0 (devuelve su valor); si cae en otra, se extrapola. No verificado contra datos reales: comportamiento fuera de rango sin definir.
float calcFCurve(const FCurve& c, float x);

// ---- Datos de rutas de camara (tablas de RAM en tiempo de ejecucion: CAMNOTBL+0x5E0, D_3091C4, D_309098) ----
struct CamCurveSet {
    virtual ~CamCurveSet() = default;
    // curva k (0..2 posicion xyz, 3..5 destino xyz, 6 float extra/fov, 7 entero/bandera) de la camara `camNo`; false si no existe
    virtual bool curve(int camNo, int k, FCurve& out) const = 0;
    virtual int gp730C() const { return 0; }          // gp-0x730C (GETCPMODE); bit 0 activa la curva 7
};
struct Anpo { float x = 0, y = 0, z = 0; int32_t angY = 0, angX = 0, flag = 0; };   // 0x18 B: posicion + angulos (+0xC, +0x10) + bandera (+0x14)

// CamPathMgr::getCameraPath (0x1CC590): posicion = curvas 0..2, destino = 3..5, `extra` = curva 6, `flag` = (int)curva 7 si gp730C & 1, si no 0.
void getCameraPath(const CamCurveSet& s, float frame, int camNo, float pos[3], float look[3], int32_t* flag, float* extra);
// CamPathMgr::getObjectPath (0x1CC590+): curvas 0..5 -> out[6]
void getObjectPath(const CamCurveSet& s, float frame, int camNo, float out[6]);

// ---- Servicios del motor ----
struct CamEngine {
    virtual ~CamEngine() = default;
    virtual int  arcTan2(float y, float x) = 0;                                  // nlArcTan2 (angulo de 16 bits)
    virtual void getAngleYX(float dx, float dy, float dz, int* yaw, int* pitch) = 0;   // GetAngleYX
    virtual bool isDemoPlay() { return false; }                                 // is_DemoPlay()
    virtual int  demoWord500DF8() { return 0; }                                  // s16 en 0x500DF8
};
// VecAng (0x1C9490): angulo entre dos vectores = arcTan2(sqrt(|a|^2|b|^2 - (a.b)^2), a.b), 16 bits con signo.
int vecAng(CamEngine& e, const float a[3], const float b[3]);
// InterDivPosI (0x1C95A0): (cur*b + dst*a) / (a + b)
float interDivPosI(float cur, float dst, int a, int b);

// Tablas de velocidad posterior (D_00266A50: 3 punteros a tablas de s8, 64 entradas cada una; indice = angulo >> 7)
struct AfterSpeedTables { const int8_t* t[3] = {nullptr, nullptr, nullptr}; };

struct CamObj { float p[3] = {0,0,0}; float w = 0; };          // entradas de entryObj: +0x4C + 0x10*i

// Estado geometrico de una ranura (D_308740 + 0x240*i), solo los campos que usan estas rutinas
struct CamSlotGeo {
    float tgtLook[3] = {0,0,0};   // +0x14 (destino de mirada)
    float tgtPos[3]  = {0,0,0};   // +0x2C
    int16_t afterSpeed = 0;       // +0x44 (velocidad tras la ruta, en fotogramas PAL)
    uint8_t flag46 = 0, flag47 = 0;   // +0x46, +0x47 (47 = 1: la ruta se recorre por trazado; 0 en modo 2 de traceCamPath)
    int objCount = 0;             // +0x14C
    CamObj obj[16];               // +0x4C + 0x10*i
    int frameInt = 0;             // +0x150
    int endInt = 0;               // +0x154
    // sub-registros +0x194 + 0x3C*k (k = 0..2): id (-2 libre, -1 listo), posicion (+0x198) y destino (+0x1B0). El id vive en CamSlotState::sub[1+k].
    Anpo subPos[3], subLook[3];
    Anpo gameLook;                // +0x174 (salida de getCameraPath en game())
};
// Estado geometrico de una camara (gp-0x7338 + 0xD0*i)
struct CamGeoState {
    Anpo pos;                     // +0x90
    Anpo look;                    // +0xA8
    int16_t camNo = 0;            // +0xD0
    uint8_t d3 = 0, d4 = 0;       // +0xD3, +0xD4
    int mode = 0;                 // +0xD2 (la copia que usa CamState::mode)
};

// CameraGeo actua tambien como CamHost de la cola: runHandler despacha las rutinas geometricas de la ranura (game, setFinishPos, traceCamPath...).
struct CameraGeo : CamHost {
    CamJobQueue& q; CamEngine& eng; const CamCurveSet& paths;
    CamSlotGeo slot[CamJobQueue::kSlots]; CamGeoState cam[CamJobQueue::kSlots];
    AfterSpeedTables speed;
    float fov[CamJobQueue::kSlots] = {};      // D_502510 + 4*i: salida "extra" de getCameraPath
    CameraGeo(CamJobQueue& jobs, CamEngine& e, const CamCurveSet& p) : q(jobs), eng(e), paths(p) {}
    int pending = 0;                                   // veces que se pidio una rutina no reescrita (game_000/Trace sin datos): ver MANIFEST
    int lastPending = -1;
    void runHandler(int h, int s) override;            // __ptmf_scall de las rutinas geometricas

    uint32_t recWord(int s, int i) const;              // palabra i del registro CCTB del trabajo en curso de la ranura (ring[slot.ring])
    void calcLookAt(Anpo& pos, const Anpo& look, int32_t flag);   // 0x1C9D30
    void setAfterSpeed(int s);                          // 0x1CB370
    void setFinishPos(int s);                           // 0x1CAC80
    void traceCamPath(int s);                           // 0x1CAD50
    void traceCamPath_000(int s);                       // 0x1CAF40
    void game(int s);                                   // 0x1C9FA0 (arranque; game_000 pendiente)
    void entryObj(int s, const float p[3], float w);    // 0x1CB310: añade un objeto a la lista de la ranura
};
}

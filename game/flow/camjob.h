// Cola de trabajos de camara (CameraMgrInternalClass, ELF 0x1C9830..0x1CB3F0) y utilidades de CamPathMgr (0x1CC590), reescritas a mano.
// El opcode CCTB de los scripts de evento mete un registro en una cola circular de 8 entradas; cada una de las 4 camaras ("ranuras",
// 0x240 B en el original) toma de la cola el siguiente registro dirigido a ella (getJob) y ejecuta su rutina hasta que llama a finishJob.
// Registro de CCTB (palabras u32 en el script): [0] opcode, [1] camara/ranura, [2] tipo de trabajo, [3..] 1 parametro por cada 16 del tipo:
//   tipo 0x10 (game), 0x11 (velocidad: float), 0x12 (cuenta de splines: int), 0x13 (indice CAFS: s8) -> 1 parametro;
//   0x30 (posicion final) -> 3 parametros; 0x40 (trazar ruta) -> 4 parametros. Longitud total = 3 + (tipo >> 4) palabras
//   (entryEvent devuelve pc + 0xC + 4*(tipo>>4)). Medido en EVTPAL.BIN: tipos vistos 0x12, 0x30, 0x40, y la palabra siguiente siempre es otro opcode valido.
// Las rutinas que calculan posiciones (game, setFinishPos, traceCamPath, calcCamera...) son matematica de camara y no se reescriben:
// se piden al motor con CamHost::runHandler.
#pragma once
#include <cstdint>
#include "scriptmem.h"
namespace vn {

enum CamJobType : int { CJ_NONE = -1, CJ_GAME = 0x10, CJ_FRAMESPD = 0x11, CJ_SPLCNT = 0x12, CJ_CAFSIDX = 0x13, CJ_FINISHPOS = 0x30, CJ_TRACE = 0x40 };
// Rutina de la ranura (ptmf en +4..+0xF). Direcciones ELF: Nop 0x1C9F90, Game 0x1C9FA0, Game000 0x1CA190, FrameSpd 0x1CAB20, SplCnt 0x1CAB90,
// CafsIdx 0x1CAC40, FinishPos 0x1CAC80, Trace 0x1CAD50, Trace000 0x1CAF40.
enum CamHandler : int { CH_NOP, CH_GAME, CH_GAME000, CH_FRAMESPD, CH_SPLCNT, CH_CAFSIDX, CH_FINISHPOS, CH_TRACE, CH_TRACE000 };

struct CamSlotState {            // 0x240 B por camara en el original (solo lo que usan estas rutinas)
    int job = CJ_NONE;           // +0
    int handler = CH_NOP;        // +4..+0xF (ptmf)
    int ring = 0;                // +0x10 indice de la cola circular del trabajo en curso
    int8_t cafsIdx = 0;          // +0x48
    int16_t sub[4] = {-2,-2,-2,-2};   // +0x158, +0x194, +0x1D0, +0x20C (centinela -2 = libre)
};
struct CamState {                // 0xD0 B por camara (gp-0x7338)
    int mode = 0;                // +0xD2 (1 = modo directo por CAMDR, 2 = modo juego)
    int flagD5 = 0, flagD6 = 0;  // +0xD5, +0xD6 (los consulta WAIT_ITEM/WAIT_ENNUM...)
    float frame = 0;             // +0xC0 fotograma actual de la ruta
    float speed = 0;             // +0xC4 velocidad (fotogramas por fotograma)
    int   endFrame = 0;          // +0xC8
    int   splCnt = 0;            // +0xCC (= +0x10 + 0xBC)
};

struct CamHost {
    virtual ~CamHost() = default;
    // Pide al motor ejecutar la rutina `h` de la ranura `slot` (las geometricas); puede llamar de vuelta a finishJob.
    virtual void runHandler(int h, int slot) { (void)h; (void)slot; }
};

struct CamJobQueue {
    static constexpr int kSlots = 4, kRing = 8;
    int16_t head = 0;            // +0x900 (lectura)
    int16_t tail = 0;            // +0x902 (escritura)
    uint32_t ringRec[kRing] = {};   // +0x904 + 8*i : direccion del registro
    int32_t  ringState[kRing] = {}; // +0x908 + 8*i : 0 libre, 1 en cola, 2 en ejecucion
    CamSlotState slot[kSlots];
    CamState cam[kSlots];
    const ScriptMem* mem = nullptr;

    void flushCamJob();                                  // 0x1CB170
    bool chkRemainCamJob() const { return head != tail; } // 0x1CB150
    // entryEvent (0x1C9CA0): mete el registro si `skip` == 0 y devuelve la direccion del siguiente opcode.
    uint32_t entryEvent(uint32_t rec, int skip);
    uint32_t chgToDirMode(uint32_t pc);                  // 0x1C9D00: cam[pc[1]].mode = 1; devuelve pc + 8
    void getJob(int s);                                  // 0x1C9D90
    void finishJob(int s, CamHost& h);                   // 0x1CB210
    // Rutinas puras de la ranura (las que solo tocan estado): FrameSpd 0x1CAB20, SplCnt 0x1CAB90, CafsIdx 0x1CAC40.
    void runSlot(int s, CamHost& h);                     // __ptmf_scall(this, s): ejecuta la rutina actual de la ranura
    static int recLen(const ScriptMem& m, uint32_t rec) { return 3 + ((int32_t)m.rd(rec + 8) >> 4); }
};

// ---- CamPathMgr (0x1CC590) -------------------------------------------------------------------------------------
// Curva CPFRM: u16 n, u16 iteraciones, luego n claves de 16 B {t f32, valor f32, tangente salida, tangente entrada} (formato de docs/LEVELS.md).
struct CurveKeys { const float* keys; int n; };     // keys[4*i] = t de la clave i
// El resto (camara -> ruta -> curva) usa tablas de RAM de tiempo de ejecucion (CAMNOTBL+0x5E0, 0x3091C4, 0x309098) y no se modela aqui.
struct CamPathMgr {
    // getStartFrame (0x1CCD20): t de la primera clave, truncado a entero (cvt.w.s).
    static int getStartFrame(const CurveKeys& c) { return (int)c.keys[0]; }
    // getEndFrame (0x1CCCB0): (int)(0.5f + t de la ultima clave).
    static int getEndFrame(const CurveKeys& c) { return (int)(0.5f + c.keys[4 * (c.n - 1)]); }
};
}

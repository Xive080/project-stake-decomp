// Gestor de pausa (PauseMgrClass, ELF 0x1F3330..0x1F3800), reescrito a mano.
// Campos del objeto: +4 estado actual (s16), +6 estado pedido, +8 cuenta de la pila de estados, +0xA cuenta de la cola de peticiones,
// +0x10 "pausa por carga" (1 = esperando datos), +0x12 copia previa de +0x10, +0x14[] pila de estados (s8), +0x1C[] cola de peticiones (s8).
// La lista de causas de espera esta en 0x517A00 (8 entradas de 16 B: +0 tipo, +4 argumento) con contador en gp-0x717C.
// Las funciones virtuales (+0xC set, +0x10 chgInit, +0x14 cambio de estado) y EntryDat::chkRemain se inyectan con PauseHooks.
#pragma once
#include <cstdint>
namespace vn {

struct PauseHooks {
    virtual ~PauseHooks() = default;
    virtual bool dataRemaining() { return false; }       // EntryDatClass::chkRemain(0x5161E0) != 0
    virtual void onStateChange(int newState) { (void)newState; }   // funcion virtual +0x14
    virtual void onBadWaitEntry(int index, int kind) { (void)index; (void)kind; }   // printf(0x26E1A0) + break: entrada de tipo inesperado
    virtual void onAssert(const char* what) { (void)what; }       // instruccion `break 0` del original
};

constexpr int kPauseWaitMax = 8;
struct PauseWait { int32_t kind = 0; int32_t arg = 0; };

struct PauseMgr {
    int16_t state = 0, wanted = 0;      // +4, +6
    int16_t stackCount = 0;             // +8
    int16_t queueCount = 0;             // +0xA
    int16_t loadPause = 0, loadPausePrev = 0;   // +0x10, +0x12
    int8_t stack[64] = {};              // +0x14 (el original no comprueba limite; aqui 64)
    int8_t queue[64] = {};              // +0x1C
    PauseWait waits[kPauseWaitMax];     // 0x517A00
    int32_t waitCount = 0;              // gp-0x717C
    int32_t gp7168 = 0;                 // gp-0x7168 (se pone a 0 en cada pasada de exec cuando espera)

    void init();                                // init (0x1F3330) salvo las dos llamadas virtuales (+0xC, +0x10) que hace el motor
    void set(int s) { queue[queueCount++] = (int8_t)s; }          // set (0x1F3390)
    int  get() const { return loadPause != 0 ? 6 : state; }        // get (0x1F33B0): 6 = pausa por carga
    void deleteWaitCauseElem(int kind);        // 0x1F33E0: compacta la lista quitando las entradas de tipo `kind`
    int  chkLoadWaitOvlMod(PauseHooks& h);     // 0x1F3450: siempre 0; `break` si hay una entrada de tipo 5
    void exec(PauseHooks& h);                  // 0x1F34B0
    void synch(PauseHooks& h);                 // 0x1F3610: sincroniza la cola con el estado
    void pushStat();                           // 0x1F36B0
    void pushSetStat(int s) { pushStat(); set(s); }   // 0x1F3700 (virtual +0xC = set)
    void popStat();                            // 0x1F3750
    bool reqPauseUntilAllDataEntry(PauseHooks& h);          // 0x1F3790: false si la lista esta llena (el original hace break)
    bool reqPauseUntilOvlModEntry(PauseHooks& h, int mod);  // 0x1F37D0 (guarda tipo 0, argumento = modulo)
};
}

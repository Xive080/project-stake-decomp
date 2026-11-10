// Resto del modo entrenamiento (overlay trnmod, base 0x9DF000: offset en TRNMOD.BIN = direccion - 0x9DF000):
// flujo del TrainingTask (judge / failed / retry / exit / mcSave / success / start_init), trn_10 y la tabla de estados.
// Cada funcion cita la original. El dibujo (*_put) no se modela; las llamadas a motor salen como eventos.
#pragma once
#include "../stages/training.h"
namespace vn {

// Tabla de estados D_009F4270..D_009F4500 (cada una: {0, -1, puntero a funcion, 0}). Aqui por nombre de la funcion destino.
enum TrnState { TS_NONE = 0, TS_INIT, TS_CAM_APPROACH, TS_SELECT_INIT, TS_SELECT, TS_EXIT, TS_SCROLL, TS_SELECT2, TS_START, TS_GAME,
                TS_JUDGE, TS_RETRY, TS_FAILED, TS_SUCCESS, TS_MCSAVE, TS_SELECT2_INIT };

// Campos del TrainingTask que usa el flujo (offsets en comentario).
struct TrnFlow {
    int   sub = 0;          // +0x68 subestado de la funcion de estado actual
    int   mission = 0;      // +0x60 mision seleccionada (NN-1)
    int   selMission = 0;   // +0x58 mision elegida en el menu
    int   prevMission = 0;  // +0x64 mision anterior (para decidir si se recargan recursos)
    float t78 = 0;          // +0x78 temporizador (fotogramas, baja 1.2/fotograma) de failed/exit/success
    float t7c = 0;          // +0x7C opacidad del fundido de exit (sube hasta 1.0); en success vale 0 al empezar
    int   t70 = 0;          // +0x70 cuenta de retry
    int   c5c = 0;          // +0x5C (start_init lo pone a 0)
    // success (nombres por uso)
    int   anyUncleared = 0; // +0x98 1 si alguna mision (de 17) sigue sin pasar (-1)
    int   newRank = 0;      // +0x9C 1 si esta partida subio el rango de la mision
    int   allPerfect = 0;   // +0xA0 1 si las 17 misiones estan a rango 2
    int   cA4 = 0, cA8 = 0; // +0xA4, +0xA8
    int   cB0 = -1;         // +0xB0
    int   grade = 0;        // +0xB4 codigo de calificacion mostrado
    int   c10F4 = 0;        // +0x10F4
};

struct TrnSys {
    int8_t rank[17] = {};      // D_5027E0 + mision: rango pasado (-1 = no pasada, 0..2)
    int8_t level[17] = {};     // D_502800 + mision: dificultad elegida
    int8_t gp72C4 = 0;         // gp-0x72C4: mision en curso
    int8_t gp72C8 = 0;         // gp-0x72C8: dificultad en curso
    int8_t norm = 0;           // gp-0x72C0
    int    enemyAlive = 0;     // gp-0x7374 (s16)
    int    flagE34 = 0;        // 0x500E34
    int    flagE33 = 0;        // 0x500E33 (judge lo pone a 1)
    int    flagE36 = 0;        // 0x500E36 (!= 0: abandonar -> retry)
    int    modeDF8 = 0;        // 0x500DF8 (s16): 1 = un jugador, 2 = dos
    int    gp73F4 = 0;         // gp-0x73F4: jugador presente (modo 1)
    int    gp7410 = 0, gp740C = 0;   // failed los pone a 0x32
    int    gameState = 0;      // 0x500F16 (s16; exit lo pone a 6)
    int32_t d501310[32] = {};  // D_501310: ranuras con ovacion (mision 3); el original recorre las 32
    bool   paused = false;
};

struct TrnFlowEvent {
    enum Kind { SOUND, SAVE_RECORD, SAVE_COMMIT, CLEANUP, FADE_OUT_START, FADE_NONE, MEMCARD_SAVE_START, START_INIT } kind;
    int a = 0, b = 0;
};
struct TrnFlowOut {
    int nextState = TS_NONE;                 // func_1A3F60(this, funcion, this): cambio de estado (TS_NONE = sigue)
    std::vector<TrnFlowEvent> ev;
    void sound(int id, int arg = 0) { TrnFlowEvent e; e.kind = TrnFlowEvent::SOUND; e.a = id; e.b = arg; ev.push_back(e); }
    void add(TrnFlowEvent::Kind k, int a = 0, int b = 0) { TrnFlowEvent e; e.kind = k; e.a = a; e.b = b; ev.push_back(e); }
};

struct TrnFlowLogic {
    // judge (0x9E4A20): decide el siguiente estado. 17 misiones (jtbl_009F4FF0): ver .cpp. `quit` = 0x500E36 != 0 -> retry.
    // Devuelve TS_FAILED / TS_SUCCESS / TS_RETRY. Pone 0x500E33 = 1 y el subestado a 0.
    static int judge(TrnFlow& f, TrnSys& s, TrnFlowOut& o);
    // failed (0x9E69B0)
    static void failed(TrnFlow& f, TrnSys& s, TrnFlowOut& o);
    // retry (0x9E6AD0): a los 2 fotogramas vuelve a start_init
    static void retry(TrnFlow& f, TrnSys& s, TrnFlowOut& o);
    // exit (0x9E6BE0)
    static void exit(TrnFlow& f, TrnSys& s, TrnFlowOut& o);
    // mcSave (0x9E6E00): `saveDone` = func_1F5010() != 0
    static void mcSave(TrnFlow& f, bool saveDone, TrnFlowOut& o);
    // start_init (0x9E3650), solo la cabecera de estado: devuelve true si hay que recargar recursos (mision nueva != anterior).
    static bool startInit(TrnFlow& f, TrnSys& s);
    // success (0x9E5A90): subestado 0 (progreso de rango) y subestado 1 (codigo de calificacion +0xB4). NO modela el resto (ver MANIFEST).
    static void successInit(TrnFlow& f, TrnSys& s);
    static void successGrade(TrnFlow& f, const TrnSys& s);
};

// trn_10_init (0x9F2200) / trn_10 (0x9F22E0): el gestor Trn10KomonoMgrTask (0x5FB0 B) se crea fuera.
struct Trn10Globals { int mgr6F50 = 0, mgr6F4C = 0; };
void trn10Init(TrnTask& t, Trn10Globals& g, TrnGlobals& tg, int mission);
void trn10Tick(TrnTask& t, Trn10Globals& g, TrnGlobals& tg);
}

// EnEtcThrowObj (GAMEMOD 0x93D2F0..0x93F230): objetos lanzados al jugador. Tipo 1 = proyectil generico que llega al jugador y le hace dano
// (DecPlayerLife_Chk con tipo 7), que el jugador puede derribar disparandole. Tipo 3 = efecto visual con ciclo de modelos y escala.
// Se reescriben las reglas de Ctrl_Pre / Ctrl3_Pre / Ctrl3_Pos y las de Init (1 y 3); dibujo (Ctrl*_Put) y posiciones de matriz no.
#pragma once
#include "e4_common.h"
namespace vn { namespace en4 {

// Estados de EnEtcThrowObj (+0x15C)
enum ThrowState : int { TS_FLY = 0, TS_HIT = 1, TS_FADE = 2, TS_SHOT_DOWN = 5 };

struct ThrowOwner { uint32_t flags2 = 0; uint32_t flags = 0; int16_t pending = 0; };   // tarea madre (+0x144): +0x10, +0xC y +0x14 (s16: objetos lanzados en vuelo)

struct ThrowObjState {
    uint32_t flags = 0;       // +0xC: 0x8 golpeado por disparo, 0x2/0x4 = disparo de jugador 0/1, 0x4000, 0x8000 bloqueo,
                              //       0x01000000 impacto consumado, 0x04000000 madre muerta, 0x20000000 bloquea el ataque global (gp-0x7360)
    float pos[3] = {0, 0, 0};          // +0x2C/+0x30/+0x34
    int   frame = 0;                   // +0x148: modelo/animacion actual
    int   frameStart = 0, frameEnd = 0;// +0x14C / +0x150: rango de la animacion
    int   timer = 0;                   // +0x154
    int   flightFrames = 0;            // +0x158: duracion del vuelo
    int   state = 0;                   // +0x15C
    float alpha = 1.0f;                // +0x160
    float scale = 0.2f;                // +0x164
    float scale2 = 0;                  // +0x16C (tipo 3)
    int   player = 0;                  // +0x170: jugador objetivo
    float start[3] = {0, 0, 0};        // +0x180..: punto de partida
    float end[3] = {0, 0, 0};          // +0x18C..: punto de llegada
    int   coop = 0;                    // +0x198 (= 0x500DFC)
    float view[3] = {0, 0, 0};         // +0x5C/+0x60/+0x64: posicion en espacio de camara (la calcula el motor); view[2] = profundidad
    int   kind = 0;                    // +0x19C (tipo 9: sentido del giro)
    bool  hasGroup = false;            // +0x1BC != 0
    bool  closed = false;
};

struct ThrowLogic {
    // EnEtcThrowObj1Init (0x93DB60), reglas: duracion, escala 0.2, rango de modelos 0x1B008D..0x1B0096, sonido 0xE040F, alfa 1.0, estado 0.
    // `holdsAttackGate` (t1 != 0) -> flags |= 0x20000000 y gp-0x7360 = 1. El owner recibe 0x20000000 en flags2.
    // El punto final (+0x18C) lo calcula el motor con la matriz de camara; aqui solo se calcula el vector LOCAL (endLocal) que se le da:
    // coop == 1 -> (0, 0, -1.5); si no -> (+-240/k, -0.0/k, -1.5) con k = gp-0x7330 (ver `endLocalOffset`).
    static void init1(ThrowObjState& s, ThrowOwner& owner, int flightFrames, bool holdsAttackGate, E4Env& e, E4Events& ev);
    static void endLocalOffset(int coop, bool rightSide, float k, float out[3]);
    // Ctrl_Pre__13EnEtcThrowObjFv (0x93D2F0).
    static void ctrlPre(ThrowObjState& s, ThrowOwner& owner, E4Env& e, E4Events& ev);
    // EnEtcThrowObj3Init (0x93F160): rango de modelos 0x260067..0x260086, flags 0x10001 y escalas a 0.
    static void init3(ThrowObjState& s);
    // Ctrl3_Pre (0x93EEC0): cierra cuando la madre esta muerta (0x04000000).
    static void ctrl3Pre(ThrowObjState& s, const ThrowOwner& owner, E4Events& ev);
    // Ctrl3_Pos (0x93F080): avanza el ciclo de modelos; escala sube hasta el fotograma 24, baja desde el 101 y cierra en el 150.
    static void ctrl3Pos(ThrowObjState& s, E4Events& ev);

    // ---- tipo 9 (objeto que llega en espiral): GAMEMOD 0x941690 Ctrl9_Pre, 0x941E20 Ctrl9_Pos, 0x941FE0 Init9 ----
    // EnEtcThrowObj9Init: punto final aleatorio. rx = nlRand() % 240, ry = nlRand() % 168 (resto con signo C), cada uno negado si otra
    // llamada a nlRand da par; endLocal = (-1.5*rx/k, -1.5*ry/k, -1.5) que el motor lleva al mundo con la matriz de camara.
    // Orden de llamadas a nlRand: rx, ry, signo de rx, signo de ry. Escala 0.5, rango de modelo 0x2900F1 (un solo fotograma), 3.0 en +0xB0.
    static void init9(ThrowObjState& s, ThrowOwner& owner, int kind, int flightFrames, float k, int (*rand)(), float endLocal[3]);
    // Ctrl9_Pre (0x941690): como ctrlPre pero la llegada se decide por profundidad (view[2] < -1.0 sigue volando; el movimiento en espiral es del motor)
    // y el dano va al jugador unico (coop == 1: `solo` = gp-0x73F4) o al de la pantalla (view[0]*k/view[2] < 0 -> jugador 1, si no 0).
    // Derribado: sonido 0x1B0017 + HitNum de los jugadores 0x2/0x4 + owner.pending--, y se cierra.
    static void ctrl9Pre(ThrowObjState& s, ThrowOwner& owner, int solo, float k, E4Env& e, E4Events& ev);
    // Ctrl9_Pos (0x941E20): avanza el modelo si 0x4000 no esta puesto (rango +0x14C..+0x150, comparacion sin signo).
    static void ctrl9Pos(ThrowObjState& s);
};
}}

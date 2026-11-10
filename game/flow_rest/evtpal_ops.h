// Tabla completa opcode -> rutina de los scripts EVTPAL (EventCtrlTask) y longitud de cada instruccion.
// Resuelve los huecos que game/flow/eventctrl_data.cpp dejaba como desconocidos (6, 8, 19, 29..32, 36, 38, 39, 41..52) y corrige 111..114.
//
// ORIGEN (verificado contra el ELF SCES_504.11, no inferido):
//   __sinit_eventctrl.cc (ELF 0x2C5B80) copia 110 plantillas de 16 B (D_0026B530 + 16*t, ptmf: [adj, idx, funcion]) a la tabla
//   D_0026BC20 (12 B por opcode). El orden de las copias SALTA los opcodes 6, 8, 19, 48 y 111: sus entradas se rellenan con el
//   vector cero D_00261608 (ptmf nulo; EventCtrl_000 no comprueba limites, asi que un script con esos opcodes llamaria a NULL).
//   Por tanto  plantilla(opcode) = opcode - (numero de opcodes nulos anteriores)  y los opcodes validos llegan a 115 (no a 114).
//   Los opcodes 111..114 que dejo "extrapolados" game/flow eran en realidad 112..115 (BRANCHITEM, ENTRYSUBGIRL, GAMEFLAGSET,
//   WAIT_FLUSHDATA_NOPAUSE). 29/30 son ITEMSET/BSFT, no FOGCTRL.
// Cada ptmf de plantilla apunta a la rutina EventCtrlTask::<NOMBRE> (direccion en kEvHandlerAddr, comprobada contra el ELF en el test).
#pragma once
#include <cstdint>
#include <vector>
#include "../flow/eventctrl.h"
namespace vn {

constexpr int kEvOpcodeSlots = 116;                 // opcodes 0..115 (el resto hasta 127 tambien es nulo)
extern const uint32_t kEvHandlerAddr[kEvOpcodeSlots];   // direccion ELF de la rutina; 0 = ptmf nulo
// opcode -> rutina (EvOp::Unknown si el slot es nulo o esta fuera de rango)
EvOp evOpFromOpcodeV2(uint32_t opcode);
// indice de plantilla (D_0026B530 + 16*t) de un opcode, -1 si el slot es nulo
int evTemplateOfOpcode(uint32_t opcode);

// ---- longitud de instruccion ----
// Longitudes en palabras de las instrucciones que no saltan, salidas de ejecutar las rutinas del asm (ver MANIFEST: verificado con un
// interprete MIPS sobre el texto de asm/*.s y con las instrucciones alcanzables de EVTPAL.BIN).
// Devuelve 0 para las instrucciones que no avanzan el pc por si mismas (WAIT, SCEFFINOUT, NEXTEV, saltos, esperas con condicion) y -1 si el
// opcode no tiene rutina. `credits`/skipReq no cambian la longitud (SNDONSK/VIBRATION avanzan igual con o sin skip).
struct EvLen { int words; bool linear; };
// `linear` = true cuando la instruccion siempre sigue en pc + words*4 (las esperas WAIT_* ceden el fotograma pero avanzan esa cantidad cuando se cumplen).
EvLen evInsnLength(const ScriptMem& mem, uint32_t pc);

// ---- exploracion estatica de un script ----
struct EvScanOptions {
    bool legacyTable = false;       // usa la tabla de game/flow (evOpFromOpcode) en vez de la V2: sirve para medir la mejora
    bool followPhaseJumps = false;  // BRANCHEV/SETEVNO: seguir tambien a las fases a las que saltan (requiere stage/event)
    int stage = -1, event = -1;
};
enum class EvEnd { NextEv, BranchEv, SetEvNo, WaitForever, UnknownOpcode, BadTarget, OutOfRange, None };
struct EvScanResult {
    bool reachedNextEv = false;     // alguna ruta alcanzable llega a NEXTEV
    EvEnd firstStop = EvEnd::None;  // primer motivo que corto una ruta sin NEXTEV (si reachedNextEv=false: por que no llega)
    int visited = 0;                // instrucciones distintas visitadas
    uint32_t stopPc = 0;
    // Lista de instrucciones visitadas (pc, longitud); sirve a los tests para comparar con un volcado de referencia
    struct Ins { uint32_t pc; uint32_t opcode; int words; };
};
EvScanResult evScanScript(const ScriptMem& mem, uint32_t startPc, const EvScanOptions& opt, std::vector<EvScanResult::Ins>* visitedOut = nullptr,
                          uint32_t evtpalBase = 0x1FC2000u, uint32_t evtpalBytes = 0);
}

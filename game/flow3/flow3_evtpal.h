// Interprete de EVTPAL de solo logica (EventCtrlTask, ELF 0x1E29F0..0x1E5010 + EventStageClear 0x1E2CC0 + InitStage 0x1E28C0 +
// EvCancelTask 0x1E51A0), reescrito a mano a partir del asm. Es una version autonoma y revisada del interprete de game/flow:
//   - Toda salida hacia el motor (enemigos, camara, sonido, carga de datos, luces...) sale como EvtCmd (opcode + palabras del registro) a
//     traves de EvtHost; la longitud de cada instruccion viene de evInsnLength (game/flow_rest, contrastada con la ejecucion de las rutinas).
//   - Los globales del sistema (gp-relativos y 0x500xxx) viven en EvtGlobals con su direccion en comentario: no estan en el ELF (bss).
//   - Correcciones frente a game/flow/eventctrl.cpp, verificadas contra el asm (ver MANIFEST.md): BRANCHEV usa la PRIMERA entrada de la
//     lista como defecto (no la palabra tras la lista); los indices de objeto (ITEMSET/WAIT_ITEM/BRANCHITEM) no se enmascaran a 64
//     (la tabla es de 256 bytes en 0x500E10).
// No hay dependencias de PS2 ni de plataforma. Se puede incluir el codigo existente de game/flow y game/flow_rest (no se modifica).
#pragma once
#include <cstdint>
#include <vector>
#include "../flow/scriptmem.h"
#include "../flow_rest/evtpal_ops.h"
namespace vn {

// ---- Globales del sistema que leen/escriben los scripts (todos fuera del ELF) ----
struct EvtGlobals {
    int8_t  stage = 0;        // gp-0x73D8  etapa (0..5 = fases 1-6; 6 = entrenamiento, la fija GameInit si gp-0x73B4 == 2; 7 = la fija GameInit si gp-0x73B4 == 1; 8 = EventStageClear; 9 = Tutorial_Init; 10 = comparte tabla con 11 y ningun codigo la fija por constante; 11 = Adver_Init)
    int8_t  event = 0;        // gp-0x73D4
    int8_t  phase = 0;        // gp-0x73D0
    int8_t  branchNo = 0;     // gp-0x73CC  numero de rama que eligen BRANCHEV/BRANCHADR/NEXTEV(tipo 1)
    uint32_t savedPc = 0;     // gp-0x7290  pc entre fotogramas (otros modulos, p. ej. TrainingTask, lo reescriben para saltar de script)
    int32_t skipReq = 0;      // gp-0x7298  1 = escena saltada (las esperas se cumplen; sonido/vibracion/camara no se emiten)
    int32_t skipPending = 0;  // gp-0x7294  lo pone EvCancelTask al saltar y lo limpian SKIP/EvCancelTask::skip/__ct
    int32_t skipEnabled = 0;  // gp-0x729C  SKIP 1 lo activa, SKIP 0 lo desactiva (y limpia skipReq)
    int8_t  canAdvance = 0;   // gp-0x7454  lo calcula cada fotograma EventCtrl_000
    int16_t enemyTarget = 0, enemyCount = 0;     // gp-0x736C, gp-0x7374
    int16_t alvTarget = 0, alvCount = 0;         // gp-0x7368, gp-0x7370
    int16_t peopleTarget = 0, peopleCount = 0;   // gp-0x7210, gp-0x7214
    int16_t menmCount = 0;    // gp-0x7358  (MISSENMCLR lo pone a 0; SWITCH_MENM lo lee; lo incrementa el motor de enemigos)
    int8_t  items[256] = {};  // 0x500E10   objetos/banderas de partida (InitStage borra 0x100 bytes)
    int32_t gameFlags = 0;    // gp-0x72D0
    int8_t  b500F05 = 0, b500F07 = 0;  // 0x500F05 (si != 0 NEXTEV conserva branchNo), 0x500F07 (se pone a 0 al cambiar de fase)
    int16_t gameState = 0;    // 0x500F14 (9 = el evento 0 arranca en fase 0 aunque sea != 0; 10 = demo)
    int16_t credits = 0;      // 0x500DFC  (ET1P/ET2P)
    int32_t playMode = 0;     // gp-0x73B4  (ENTRYSUBGIRL: 1)
    int32_t gp730C = 0;       // gp-0x730C  (GETCPMODE)
    uint32_t wlctbTable = 0;  // gp-0x7304  (WLCTB: direccion de la tabla)
    int32_t bsft = 0;         // gp-0x73AC
    int16_t nowLoading = 0;   // gp-0x738C
    int32_t snowDisp = 0;     // gp-0x7340
    uint8_t zmat[4] = {};     // gp-0x7364
    int32_t gp733C = 0;       // gp-0x733C  (0x1DC0 en la etapa 6, 0x2254 en el resto)
    int8_t  stageClearFlag = 0;   // gp-0x73EC (EventStageClear lo pone a 1; InitStage a 0)
    int8_t  mode73F0 = 0;     // gp-0x73F0 (EventStageClear: si gameState==10 y != 8 pasa a 5)
    int8_t  subGirlName[8] = {};  // 0x502690: 2 filas de 4 bytes; ENTRYSUBGIRL busca 0x2E en los bytes +2,+3 de cada fila
};

// ---- Comando que se entrega al motor ----
struct EvtCmd {
    uint32_t opcode = 0;          // opcode del script (palabra 0)
    EvOp op = EvOp::Unknown;      // rutina resuelta (evOpFromOpcodeV2)
    uint32_t pc = 0;              // direccion del registro
    std::vector<uint32_t> w;      // palabras del registro (w[0] = opcode)
    bool skipped = false;         // true si el original NO ejecuta el efecto por skipReq (ENTBSK, SNDONSK, VIBRATION, CCTB...)
    bool cond = false;            // ENTRYSUBGIRL: true si carga el banco; ET1P/ET2P/ENTBSK: true si se entra al enemigo (false = solo salta el registro)
};

// ---- Interfaz con el motor ----
struct EvtHost {
    virtual ~EvtHost() = default;
    virtual void command(const EvtCmd& c) { (void)c; }              // efecto de motor de longitud conocida
    virtual bool dataRemaining() { return false; }                  // EntryDat::chkRemain (D_5161E0)
    virtual void reqPauseAllData() {}                               // PauseMgr::reqPauseUntilAllDataEntry
    virtual int  ovlEntryStat(int mod) { (void)mod; return 2; }     // OverlayMgr::chkEntryStat (2 = cargado)
    virtual void reqPauseOvlMod(int mod) { (void)mod; }             // PauseMgr::reqPauseUntilOvlModEntry
    virtual int  pauseLevel() { return 0; }                         // PauseMgr::get()
    virtual bool demoPlay() { return false; }                       // is_DemoPlay()
    virtual int  playerMode(int p) { (void)p; return 0; }           // 0x500730 + 0x348*p + 4
    virtual int  playerLife(int p) { (void)p; return 0; }           // 0x500730 + 0x348*p + 0x10
    // camara (CameraTask en gp-0x7338)
    virtual bool camJobPending() { return false; }                  // chkRemainCamJob
    virtual float camFrame(int slot) { (void)slot; return 0.f; }    // +0xC0 de la camara
    virtual int  camEndFrame(int slot) { (void)slot; return 0; }    // +0xC8
    virtual bool camFlagD5(int slot) { (void)slot; return true; }   // +0xD5
    virtual void camSetFlagD6() {}                                  // camara 0, +0xD6 = 1 (efecto lateral de las esperas de entidades)
    virtual void camForce(int slot, int kind, uint32_t value) { (void)slot; (void)kind; (void)value; }   // FORCE_CAMPARAM
    virtual void bgAppoint(int bg) { (void)bg; }                    // BGCtrl::appointBG (gp-0x7440)
    virtual void stageClear() {}                                    // EventStageClear: efectos de motor (los globales los toca el interprete)
    virtual void addHitPerBonus() {}                                // AddHitPerBonus (solo en la etapa 5)
};

enum class EvtFault : uint8_t { None, UnknownOpcode, ZmatOverflow, NullBranch, Hang, SceffinoutLoop };

// Transicion de fin de fase: fila de 8 B {tipo, a, b, c} (GAMEMOD 0x9B83D0 -> tabla por etapa). Ver EvTrans en game/flow.
struct EvtTransSource {
    virtual ~EvtTransSource() = default;
    // devuelve false si no hay tabla para la etapa/evento
    virtual bool row(int stage, int event, int16_t out[4]) const = 0;
};
// Implementacion por defecto con kEvTransTables de game/flow (comprobada contra GAMEMOD.BIN en el test)
struct DefaultTrans : EvtTransSource { bool row(int stage, int event, int16_t out[4]) const override; };

struct EvtVm {
    // Campos del objeto EventCtrlTask
    int8_t   wait = 0;        // +4  "ceder el fotograma" (lo ponen WAIT y las esperas; JOBFLAGSET lo escribe con su argumento)
    int8_t   intMode = 0;     // +5  WAIT_INT_MODE: las esperas por entidades se cumplen sin que la camara este lista
    int16_t  stopFlag = 0;    // +6  1 = cortar el bucle del fotograma (datos pendientes)
    uint32_t op = 0;          // +8  opcode actual
    uint32_t pc = 0;          // +0xC
    uint32_t lastPc = 0;      // +0x10
    int32_t  frameCnt = 0;    // +0x14 contador de WAIT_FRM (en fotogramas PAL)

    EvtGlobals& g; const ScriptMem& mem; EvtHost& host; const EvtTransSource& trans;
    uint32_t evtpalBase;
    EvtFault fault = EvtFault::None;
    uint32_t faultPc = 0;
    int maxStepsPerFrame = 100000;   // guarda anti-bucle (el original se colgaria: SCEFFINOUT, VIBRATION de clase desconocida...)
    uint32_t stepsLastFrame = 0;     // instrucciones ejecutadas en el ultimo fotograma
    std::vector<uint32_t>* trace = nullptr;   // si no es nulo, recibe el pc de cada instruccion ejecutada (solo para pruebas)

    EvtVm(EvtGlobals& gl, const ScriptMem& m, EvtHost& h, const EvtTransSource& t, uint32_t evtpal = 0x1FC2000u)
        : g(gl), mem(m), host(h), trans(t), evtpalBase(evtpal) {}

    uint32_t lookupPc(int stage, int event, int phase) const;   // EVTPAL[etapa][evento][fase]
    void construct();                    // __ct__13EventCtrlTaskFv (estado inicial)
    void frame();                        // EventCtrl_000: un fotograma
    void step();                         // ejecuta la instruccion en `pc` (op = palabra en pc)
    void nextEv();                       // NEXTEV (0x1E3BF0)
    bool computeCanAdvance();            // primer tramo de EventCtrl_000
    bool camReady();                     // (cam.d5 != 0 || intMode != 0) con el efecto lateral d6 = 1
    // Salta a otro script como hacen TrainingTask/shpmod: g.savedPc = EVTPAL[stage][event][phase]
    void jumpToScript(int stage, int event, int phase);
    uint32_t arg(int i) const { return mem.rd(pc + 4u * (uint32_t)i); }
private:
    void emit(int words, bool skipped = false, bool cond = false);
    void fixedLen(bool skipped = false);
    void setPcFromTable();
    uint32_t pickFromList(uint32_t first, int n, bool& found) const;
};

// ---- Piezas auxiliares del mismo sistema ----
// InitStage (ELF 0x1E28C0, llamada desde GameInit/trn/acmod): pone a cero los contadores y los 256 objetos. (Los efectos de motor se
// reducen a que el llamante reinicie EnemyMgr y la plata mostrada.)
void evtInitStage(EvtGlobals& g);
// EventStageClear (ELF 0x1E2CC0)
void evtStageClear(EvtGlobals& g);
// EvCancelTask::ctrl (0x1E5320): logica de "saltar la escena". Devuelve que debe hacer el motor.
struct EvCancelState { int8_t buttonFlag = 0; /* gp-0x72A0 */ };
enum class EvCancelAct { DeleteSelf, Nothing, DoSkip };
EvCancelAct evCancelCtrl(EvtGlobals& g, EvCancelState& s);
void evCancelSkip(EvtGlobals& g);                // EvCancelTask::skip (0x1E53D0)
}

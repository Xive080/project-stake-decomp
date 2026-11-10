// Interprete de scripts de evento de fase (EventCtrlTask, ELF 0x1E29F0..0x1E3F00 + __ct), reescrito a mano.
// Los scripts (EVTPAL.BIN, base RAM 0x1FC2000) son palabras u32: opcode seguido de argumentos; EVTPAL[etapa][evento][fase] da el pc.
// Este modulo implementa el flujo (pc, saltos, esperas, cambio de evento/fase, objetos, banderas) y delega al motor en EvHost
// todo lo que carga datos, suena, dibuja o crea enemigos. La tabla opcode -> rutina sale de tools/afs_evtpal.py (comprobada con __sinit para
// parte de los opcodes; los huecos 6, 8, 19, 29..32, 36, 38, 39, 41..52 no estan resueltos y el interprete los marca como desconocidos).
#pragma once
#include <cstdint>
#include "scriptmem.h"
#include "camjob.h"
#include "bgctrl.h"
namespace vn {

enum class EvOp : uint8_t {
    Unknown = 0,
    ENTB, ENTB2, ET1P, ET2P, ENTBSK, ZMAT, WLCTB, BRANCHNO, BRANCH, MISSENMCLR, SWITCH_MENM, BRANCHEV, SETEVNO, BRANCHADR,
    CCTB, CAMDR, GETCPMODE, WAIT_CAMJOB, WAIT_CAMFRM, WAIT_FRM, WAIT_ENNUM, WAIT_ENALV, WAIT_PEOPLE, WAIT_ITEM, WAIT_FLUSHDATA, WAIT,
    ITEMSET, BSFT, WAIT_INT_MODE, FOGCTRL, LIGHTCTRL, MLLIGHT, PLLIGHT, LIGHTIMMEXE, PUSHLIGHT, POPLIGHT, DOFCTRL, SCEFFINOUT, BLURCTRL,
    OVLAP, MOTVOLUME, MOTSHDWMAP, WAIT_BUFFERRAM, BGTB, FLRTB, SKYKD, SKYDSP, CHGSM, SCNINIT, NEXTEV, SKIP, BRANCHNOSET, SNOWDISP,
    DBG_MALLOC_PRINT, DBG_ENTDATA_INFO, JIMAKU, FORCE_CAMPARAM, VIBRATION, ENTRYSTGDATA, ENTRYCPBK, ENTRYBANK, FREEBANK, ENTRYSPR, FREESPR,
    ENTRYMOT, FREEMOT, ENTRYENVTX, FREEENVTX, ENTRYOSK, FREEOSK, ENTRYOSKNAME, FREEOSKNAME, ENTRYOSKBK, FREEOSKBK, ENTRYCLVTX, FREECLVTX,
    ENTRYFANI, FREEFANI, ENTRYFAMOT, FREEFAMOT, ENTRYCOLLI, FREECOLLI, FLUSHDATA, ENTRYTEX, FREETEX, NOWLOADINGSET, ENTRYLANGSPR, FREELANGSPR,
    ENTRYLANGBANK, FREELANGBANK, ENTRYSNDPK, FREESNDPK, BGMPLAY, VOICEPLAY, BGMPAUSE, BGMFADE, SKVOOFF, SNDON, SNDONM, SNDONSK, SNDONMSK,
    LOAD_OVLMOD, WAIT_LOAD_OVLMOD, CHGADXPT, JOBFLAGSET, BRANCHITEM, ENTRYSUBGIRL, GAMEFLAGSET, WAIT_FLUSHDATA_NOPAUSE
};
// opcode del script -> rutina (EvOp::Unknown si no esta resuelto). Fuente: tools/afs_evtpal.py OPS.
EvOp evOpFromOpcode(uint32_t opcode);
const char* evOpName(EvOp op);

// Fila de la tabla de transiciones de fin de fase (GAMEMOD 0x9B83D0 -> tabla por etapa; 8 B por evento).
//   tipo 0: siguiente evento = a;  tipo 1: siguiente evento = (s16)[a + 2*branchNo] (a, b, c = lista);  tipo 2: evento + 1;
//   tipo 3: fase superada (EventStageClear), evento = a, el script pasa a EVTPAL[8].
// En todos los tipos salvo el 3 la fase vuelve a 1.
struct EvTrans { int16_t type, a, b, c; };
struct EvTransTable { const EvTrans* rows; int count; };
extern const EvTransTable kEvTransTables[12];   // indexada por etapa (gp-0x73D8); filas segun los datos de GAMEMOD.BIN (ver MANIFEST)

// Estado global que el original lee/escribe (gp-relativo o 0x500xxx). Todo se inyecta/lee aqui.
struct EvState {
    int stage = 0;          // gp-0x73D8 (s8)
    int event = 0;          // gp-0x73D4 (s8)
    int phase = 0;          // gp-0x73D0 (s8)
    int branchNo = 0;       // gp-0x73CC (s8): numero de rama que eligen los BRANCH*
    uint32_t savedPc = 0;   // gp-0x7290: pc guardado entre fotogramas
    int skipReq = 0;        // gp-0x7298: 1 = saltar la escena (las esperas se cumplen y los sonidos/vibracion no suenan)
    int gp7294 = 0, gp729C = 0;   // SKIP los fija
    int canAdvance = 0;     // gp-0x7454 (lo calcula EventCtrl_000 cada fotograma)
    int enemyTarget = 0, enemyCount = 0;     // gp-0x736C (objetivo), gp-0x7374 (enemigos vivos)
    int alvTarget = 0, alvCount = 0;         // gp-0x7368, gp-0x7370
    int peopleTarget = 0, peopleCount = 0;   // gp-0x7210, gp-0x7214
    int menmCount = 0;      // gp-0x7358 (MISSENMCLR lo pone a 0; SWITCH_MENM lo lee)
    int8_t items[64] = {};  // 0x500E10: objetos/banderas de partida (ITEMSET, WAIT_ITEM, BRANCHITEM)
    int gameFlags = 0;      // gp-0x72D0 (GAMEFLAGSET; bit 1 se limpia al entrar en etapa 5 evento 3)
    int8_t b500F05 = 0, b500F07 = 0;
    int gameState = 0;      // s16 en 0x500F14
    int credits1P2P = 0;    // s16 en 0x500DFC (jugadores/creditos: ET1P/ET2P)
    int gp730C = 0;         // GETCPMODE
    uint32_t wlctbTable = 0;// gp-0x7304 (WLCTB)
    int bsft = 0;           // gp-0x73AC (BSFT)
    int nowLoading = 0;     // gp-0x738C (NOWLOADINGSET)
    int snowDisp = 0;       // gp-0x7340
    uint8_t zmat[4] = {};   // gp-0x7364 (ZMAT)
    int zmatCount = 0;
    int gp733C = 0;         // 0x1DC0 (etapa 6) o 0x2254
    int gp7290init = 0;
};

// Interfaz con el motor.
struct EvHost {
    virtual ~EvHost() = default;
    virtual bool dataRemaining() { return false; }                 // EntryDat::chkRemain
    virtual void reqPauseAllData() {}                              // PauseMgr::reqPauseUntilAllDataEntry
    virtual int  ovlEntryStat(int mod) { (void)mod; return 2; }    // OverlayMgr::chkEntryStat (2 = cargado)
    virtual void reqPauseOvlMod(int mod) { (void)mod; }            // PauseMgr::reqPauseUntilOvlModEntry
    virtual int  pauseLevel() { return 0; }                        // PauseMgr::get()
    virtual bool demoPlay() { return false; }                      // is_DemoPlay(&D_500A78)
    // vida y modo de los dos jugadores (0x500730 + 0x348*i: +4 modo, +0x10 vida)
    virtual int  playerMode(int p) { (void)p; return 0; }
    virtual int  playerLife(int p) { (void)p; return 0; }
    // Enemigos: devuelven el pc siguiente (longitud variable de los registros de entrada)
    virtual uint32_t enemyEntry(uint32_t pc, bool skipOnly) { (void)skipOnly; return pc + 4; }
    virtual uint32_t enemyEntry2(uint32_t pc) { return pc + 4; }
    virtual uint32_t varCmd(EvOp op, uint32_t pc) { (void)op; return pc + 4; }   // FOGCTRL/LIGHTCTRL/DOFCTRL/BLURCTRL
    // Comando de motor de longitud fija: args[0] = opcode, args[1..n-1] = argumentos
    virtual void cmd(EvOp op, const uint32_t* args, int nWords) { (void)op; (void)args; (void)nWords; }
    virtual void stageClear() {}                                   // EventStageClear
    virtual void addHitPerBonus() {}                               // AddHitPerBonus
    virtual void onUnknownOpcode(uint32_t opcode, uint32_t pc) { (void)opcode; (void)pc; }
};

struct EventCtrl {
    // Campos del objeto
    int8_t  wait = 0;       // +4  "ceder el fotograma"
    int8_t  intMode = 0;    // +5  WAIT_INT_MODE
    int16_t stopFlag = 0;   // +6  1 = cortar el bucle (datos pendientes)
    uint32_t op = 0;        // +8  opcode actual
    uint32_t pc = 0;        // +0xC
    uint32_t lastPc = 0;    // +0x10
    int32_t frameCnt = 0;   // +0x14 (WAIT_FRM)

    EvState& g; const ScriptMem& mem; EvHost& host; CamJobQueue* cam;
    BGCtrl bg;              // objeto de fondo (gp-0x7440) que dirige BGTB
    uint32_t evtpalBase;    // 0x1FC2000
    EventCtrl(EvState& st, const ScriptMem& m, EvHost& h, CamJobQueue* c, uint32_t evtpal = 0x1FC2000u) : g(st), mem(m), host(h), cam(c), evtpalBase(evtpal) {}

    uint32_t lookupPc(int stage, int event, int phase) const;      // EVTPAL[stage][event][phase] (tres niveles de punteros)
    void construct();                                               // __ct__13EventCtrlTaskFv (parte de estado)
    void frame();                                                   // EventCtrl_000 (un fotograma)
    void step();                                                    // ejecuta la instruccion en pc segun `op` (op = palabra en pc)
    void exec(EvOp handler);                                        // ejecuta una rutina concreta sobre el pc actual (salta la tabla opcode->rutina)
    void nextEv();                                                  // NEXTEV
    bool canAdvanceNow() const;                                     // gp-0x7454 segun jugadores/demo
    bool camReady();                                                // (cam.d6 = 1, cam.d5 != 0 || intMode != 0): efecto lateral incluido
};
}

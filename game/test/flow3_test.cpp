// Pruebas de game/flow3 (interprete EVTPAL de solo logica). Ver game/flow3/MANIFEST.md.
// Los datos reales salen de disc/TBL/EVTPAL.BIN, disc/TBL/GAMEMOD.BIN y de los .s de asm/; si faltan, esas comprobaciones se omiten con aviso.
// Compilar y ejecutar desde la raiz del repo (comando exacto en el MANIFEST):
//   mkdir -p build/verify_flow3 && c++ -std=c++17 -Wall game/test/flow3_test.cpp game/flow3/*.cpp game/flow_rest/*.cpp game/flow/*.cpp -o build/verify_flow3/flow3_test && build/verify_flow3/flow3_test
#include "../flow3/flow3_evtpal.h"
#include "../flow3/flow3_camera.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace vn;
static int fails = 0, skipped = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)
static void skip(const char* why) { printf("(omitido) %s\n", why); skipped++; }
static std::vector<uint8_t> readFile(const char* p) {
    std::ifstream f(p, std::ios::binary); if (!f) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// ---------------------------------------------------------------- anfitrion de pruebas
struct TestHost : EvtHost {
    std::vector<EvtCmd> cmds; bool remain = false; int pause = 0; bool demo = false;
    int life0 = 100, mode0 = 5, life1 = 0, mode1 = 0; bool camPending = false; float cframe = 1e9f; int cend = 0; bool d5 = true; int d6 = 0;
    int reqPauseAll = 0, clears = 0, hitBonus = 0; int ovlStat = 2; std::vector<int> ovlReq; int bg = -1;
    std::vector<std::array<uint32_t, 3>> forces;
    void command(const EvtCmd& c) override { cmds.push_back(c); }
    bool dataRemaining() override { return remain; }
    void reqPauseAllData() override { reqPauseAll++; }
    int ovlEntryStat(int) override { return ovlStat; }
    void reqPauseOvlMod(int m) override { ovlReq.push_back(m); }
    int pauseLevel() override { return pause; }
    bool demoPlay() override { return demo; }
    int playerMode(int p) override { return p == 0 ? mode0 : mode1; }
    int playerLife(int p) override { return p == 0 ? life0 : life1; }
    bool camJobPending() override { return camPending; }
    float camFrame(int) override { return cframe; }
    int camEndFrame(int) override { return cend; }
    bool camFlagD5(int) override { return d5; }
    void camSetFlagD6() override { d6++; }
    void camForce(int s, int k, uint32_t v) override { forces.push_back({(uint32_t)s, (uint32_t)k, v}); }
    void bgAppoint(int b) override { bg = b; }
    void stageClear() override { clears++; }
    void addHitPerBonus() override { hitBonus++; }
};

// ---------------------------------------------------------------- memoria sintetica
// Un EVTPAL de juguete: la etapa 0 tiene los eventos/fases que se den; la etapa 8 es un script WAIT (como el real). Las palabras 0xBEEF00nn de un
// script se sustituyen por la direccion de la palabra nn del propio script (destinos de salto).
static const uint32_t kBase = 0x1FC2000u;
struct Toy {
    std::vector<std::vector<std::vector<uint32_t>>> ev;     // [evento][fase] -> palabras del script
    VecMem mem; std::vector<std::vector<uint32_t>> addr;    // direccion de cada script
    uint32_t clearAddr = 0;
    void build() {
        std::vector<uint32_t> w(16, 0);                     // cabecera: EVTPAL[0..12]
        auto here = [&]() { return kBase + 4u * (uint32_t)w.size(); };
        // etapa 8: EVTPAL[8] -> lista de eventos {&fases, -1} -> lista de fases {&script, -1} -> script [WAIT, 0]
        uint32_t l1_8 = here();
        w.push_back(l1_8 + 8); w.push_back(0xFFFFFFFFu);            // lista de eventos
        w.push_back(l1_8 + 16); w.push_back(0xFFFFFFFFu);           // lista de fases del evento 0
        w.push_back(28); w.push_back(0);                            // script
        clearAddr = l1_8 + 16;
        w[8] = l1_8;
        // etapa 0
        uint32_t evList = here();
        for (size_t e = 0; e < ev.size(); e++) w.push_back(0);
        w.push_back(0xFFFFFFFFu);
        w[0] = evList;
        std::vector<uint32_t> phList(ev.size());
        for (size_t e = 0; e < ev.size(); e++) {
            phList[e] = here(); w[(evList - kBase) / 4 + e] = phList[e];
            for (size_t p = 0; p < ev[e].size(); p++) w.push_back(0);
            w.push_back(0xFFFFFFFFu);
        }
        addr.assign(ev.size(), {});
        for (size_t e = 0; e < ev.size(); e++)
            for (size_t p = 0; p < ev[e].size(); p++) {
                uint32_t a = here(); addr[e].push_back(a); w[(phList[e] - kBase) / 4 + p] = a;
                for (uint32_t x : ev[e][p]) w.push_back((x & 0xFFFF0000u) == 0xBEEF0000u ? a + 4u * (x & 0xFFFFu) : x);
            }
        w.push_back(0xFFFFFFFFu);
        mem = VecMem(kBase, w);
    }
};
#define REL(n) (0xBEEF0000u | (n))
static const uint32_t NIL = 0xFFFFFFFFu;
// opcodes del script (indice de slot)
enum : uint32_t { O_ENTB = 0, O_ZMAT = 5, O_WLCTB = 7, O_BRANCHNO = 9, O_BRANCH = 10, O_MISSENMCLR = 11, O_SWITCH_MENM = 12, O_BRANCHEV = 13,
    O_SETEVNO = 14, O_BRANCHADR = 15, O_CCTB = 16, O_CAMDR = 17, O_GETCPMODE = 18, O_WAIT_CAMJOB = 20, O_WAIT_CAMFRM = 21, O_WAIT_FRM = 22,
    O_WAIT_ENNUM = 23, O_WAIT_ENALV = 24, O_WAIT_PEOPLE = 25, O_WAIT_ITEM = 26, O_WAIT_FLUSHDATA = 27, O_WAIT = 28, O_ITEMSET = 29, O_BSFT = 30,
    O_WAIT_INT_MODE = 31, O_SCEFFINOUT = 42, O_BGTB = 49, O_NEXTEV = 55, O_SKIP = 56, O_BRANCHNOSET = 57, O_SNOWDISP = 58, O_FORCE_CAMPARAM = 62,
    O_VIBRATION = 63, O_NOWLOADINGSET = 91, O_WAIT_LOAD_OVLMOD = 108, O_JOBFLAGSET = 110, O_BRANCHITEM = 112, O_ENTRYSUBGIRL = 113,
    O_GAMEFLAGSET = 114, O_WAIT_FLUSHDATA_NOPAUSE = 115 };
// (los numeros de arriba se comprueban contra evOpFromOpcodeV2 en testOpcodeNumbers: si algun numero esta mal, esa prueba falla)

struct Rig {
    Toy t; EvtGlobals g; TestHost h; DefaultTrans tr;
    EvtVm* vm = nullptr;
    ~Rig() { delete vm; }
    void start(int ev = 0, int ph = 0) { t.build(); g.stage = 0; g.event = (int8_t)ev; vm = new EvtVm(g, t.mem, h, tr); vm->construct(); if (ph != g.phase) { g.phase = (int8_t)ph; vm->pc = g.savedPc = vm->lookupPc(0, ev, ph); } }
    void frames(int n) { for (int i = 0; i < n; i++) vm->frame(); }
};

static void testOpcodeNumbers() {
    struct { uint32_t n; EvOp op; } t[] = {
        {O_ENTB, EvOp::ENTB}, {O_ZMAT, EvOp::ZMAT}, {O_WLCTB, EvOp::WLCTB}, {O_BRANCHNO, EvOp::BRANCHNO}, {O_BRANCH, EvOp::BRANCH}, {O_MISSENMCLR, EvOp::MISSENMCLR},
        {O_SWITCH_MENM, EvOp::SWITCH_MENM}, {O_BRANCHEV, EvOp::BRANCHEV}, {O_SETEVNO, EvOp::SETEVNO}, {O_BRANCHADR, EvOp::BRANCHADR}, {O_CCTB, EvOp::CCTB},
        {O_CAMDR, EvOp::CAMDR}, {O_GETCPMODE, EvOp::GETCPMODE}, {O_WAIT_CAMJOB, EvOp::WAIT_CAMJOB}, {O_WAIT_CAMFRM, EvOp::WAIT_CAMFRM}, {O_WAIT_FRM, EvOp::WAIT_FRM},
        {O_WAIT_ENNUM, EvOp::WAIT_ENNUM}, {O_WAIT_ENALV, EvOp::WAIT_ENALV}, {O_WAIT_PEOPLE, EvOp::WAIT_PEOPLE}, {O_WAIT_ITEM, EvOp::WAIT_ITEM},
        {O_WAIT_FLUSHDATA, EvOp::WAIT_FLUSHDATA}, {O_WAIT, EvOp::WAIT}, {O_ITEMSET, EvOp::ITEMSET}, {O_BSFT, EvOp::BSFT}, {O_WAIT_INT_MODE, EvOp::WAIT_INT_MODE},
        {O_SCEFFINOUT, EvOp::SCEFFINOUT}, {O_BGTB, EvOp::BGTB}, {O_NEXTEV, EvOp::NEXTEV}, {O_SKIP, EvOp::SKIP}, {O_BRANCHNOSET, EvOp::BRANCHNOSET},
        {O_SNOWDISP, EvOp::SNOWDISP}, {O_FORCE_CAMPARAM, EvOp::FORCE_CAMPARAM}, {O_VIBRATION, EvOp::VIBRATION}, {O_NOWLOADINGSET, EvOp::NOWLOADINGSET},
        {O_WAIT_LOAD_OVLMOD, EvOp::WAIT_LOAD_OVLMOD}, {O_JOBFLAGSET, EvOp::JOBFLAGSET}, {O_BRANCHITEM, EvOp::BRANCHITEM}, {O_ENTRYSUBGIRL, EvOp::ENTRYSUBGIRL},
        {O_GAMEFLAGSET, EvOp::GAMEFLAGSET}, {O_WAIT_FLUSHDATA_NOPAUSE, EvOp::WAIT_FLUSHDATA_NOPAUSE} };
    for (auto& x : t) CHECK(evOpFromOpcodeV2(x.n) == x.op);
}

// ---------------------------------------------------------------- semantica sintetica (valores sacados del asm, ver MANIFEST)
static void testConstructAndFrame() {
    Rig r; r.t.ev = {{{O_WAIT}, {O_WAIT}}, {{O_WAIT}, {O_WAIT}}};
    r.g.event = 0; r.g.gameState = 0; r.start();
    CHECK(r.g.phase == 0 && r.g.gp733C == 0x2254 && r.g.savedPc == r.t.addr[0][0]);
    Rig r2; r2.t.ev = {{{O_WAIT}, {O_WAIT}}, {{O_WAIT}, {O_WAIT}}}; r2.g.event = 1; r2.t.build(); r2.g.stage = 6;
    EvtVm v2(r2.g, r2.t.mem, r2.h, r2.tr); v2.construct();                // evento != 0 y gameState != 9 -> fase 1 (etapa 6 -> 0x1DC0, la tabla real no tiene etapa 6 aqui)
    CHECK(r2.g.phase == 1 && r2.g.gp733C == 0x1DC0);
    r2.g.gameState = 9; v2.construct(); CHECK(r2.g.phase == 0);           // gameState 9: fase 0 siempre
    // canAdvance: vivo en modo 5 (jugador 0 o 1) o demo; con pausa >= 2 el fotograma no ejecuta nada
    Rig r3; r3.t.ev = {{{O_WAIT}}}; r3.start();
    r3.h.life0 = 0; r3.vm->frame(); CHECK(r3.g.canAdvance == 0);
    r3.h.life1 = 5; r3.h.mode1 = 5; r3.vm->frame(); CHECK(r3.g.canAdvance == 1);
    r3.h.life1 = 0; r3.h.demo = true; r3.vm->frame(); CHECK(r3.g.canAdvance == 1);
    r3.h.demo = false; r3.h.life0 = 9; r3.h.mode0 = 4; r3.vm->frame(); CHECK(r3.g.canAdvance == 0);       // vida > 0 pero modo != 5
    Rig r4; r4.t.ev = {{{O_ITEMSET, 3, 1, O_WAIT}}}; r4.start(); r4.h.pause = 2; r4.vm->frame(); CHECK(r4.g.items[3] == 0);
    r4.h.pause = 1; r4.vm->frame(); CHECK(r4.g.items[3] == 1);
    // WAIT: cede el fotograma y no avanza nunca (nadie vuelve a poner wait = 0)
    uint32_t wpc = r4.t.addr[0][0] + 12;
    for (int i = 0; i < 5; i++) { r4.vm->frame(); CHECK(r4.vm->pc == wpc && r4.vm->wait == 1 && r4.g.savedPc == wpc); }
    // otro modulo (TrainingTask) puede reescribir savedPc: el script salta
    r4.g.savedPc = r4.t.addr[0][0]; r4.vm->wait = 0; r4.g.items[3] = 0; r4.vm->frame(); CHECK(r4.g.items[3] == 1);
}
static void testBranches() {
    // BRANCH
    { Rig r; r.t.ev = {{{O_BRANCH, REL(4), O_ITEMSET, 1, 1, O_ITEMSET, 2, 2, 0, O_WAIT}}}; r.t.ev[0][0] = {O_BRANCH, REL(5), O_ITEMSET, 1, 1, O_ITEMSET, 2, 7, O_WAIT}; r.start(); r.frames(1);
      CHECK(r.g.items[2] == 7 && r.g.items[1] == 0); }
    // BRANCHNO + BRANCHADR (defecto 0 si no hay entrada: salto nulo -> fallo registrado) + eleccion por branchNo
    { Rig r; r.t.ev = {{{O_BRANCHNO, 1, O_BRANCHADR, REL(7), REL(9), NIL, O_WAIT, O_ITEMSET, 5, 1, O_WAIT /*9*/ }}};
      r.t.ev[0][0] = {O_BRANCHNO, 1, O_BRANCHADR, REL(7), REL(11), NIL, O_WAIT, O_ITEMSET, 5, 1, O_WAIT, O_ITEMSET, 6, 1, O_WAIT}; r.start(); r.frames(1);
      CHECK(r.g.branchNo == 1 && r.g.items[6] == 1 && r.g.items[5] == 0); }
    { Rig r; r.t.ev = {{{O_BRANCHNO, 5, O_BRANCHADR, REL(5), NIL, O_WAIT}}}; r.start(); r.frames(1);
      CHECK(r.vm->fault == EvtFault::NullBranch && r.vm->pc == 0); }                    // sin entrada: pc = 0 en el original
    // SWITCH_MENM: [op, defecto, dest0, dest1, -1]; elige menmCount; MISSENMCLR pone a 0
    { Rig r; r.t.ev = {{{O_SWITCH_MENM, REL(7), REL(8), REL(11), NIL, O_WAIT, 0, O_ITEMSET, 1, 1, O_WAIT, O_ITEMSET, 2, 1, O_WAIT}}};
      r.t.ev[0][0] = {O_SWITCH_MENM, REL(5), REL(7), REL(10), NIL, O_ITEMSET, 9, 1, O_WAIT, O_ITEMSET, 1, 1, O_WAIT, O_ITEMSET, 2, 1, O_WAIT};
      // nota: REL(5) = defecto en la palabra 5; destinos 7 y 10 son ITEMSET 1 y ITEMSET 2 (palabras 9 y 12 reales) -> se recalculan abajo
      r.t.ev[0][0] = {O_SWITCH_MENM, REL(5), REL(9), REL(13), NIL, O_ITEMSET, 9, 1, O_WAIT, O_ITEMSET, 1, 1, O_WAIT, O_ITEMSET, 2, 1, O_WAIT};
      r.start(); r.g.menmCount = 1; r.frames(1); CHECK(r.g.items[2] == 1 && r.g.items[1] == 0 && r.g.items[9] == 0); }
    { Rig r; r.t.ev = {{{O_SWITCH_MENM, REL(5), REL(9), NIL, O_WAIT, O_ITEMSET, 9, 1, O_WAIT, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.g.menmCount = 3; r.frames(1);
      CHECK(r.g.items[9] == 1 && r.g.items[1] == 0); }                                  // fuera de la lista: defecto
    { Rig r; r.t.ev = {{{O_MISSENMCLR, O_WAIT}}}; r.start(); r.g.menmCount = 4; r.frames(1); CHECK(r.g.menmCount == 0); }
    // BRANCHITEM: [op, objeto, dir0, dir1, -1]; valor del objeto como indice (sin enmascarar: objeto 200)
    { Rig r; r.t.ev = {{{O_BRANCHITEM, 200, REL(6), REL(9), NIL, O_WAIT, O_ITEMSET, 1, 1, O_WAIT, O_ITEMSET, 2, 1, O_WAIT}}};
      r.t.ev[0][0] = {O_BRANCHITEM, 200, REL(7), REL(11), NIL, O_WAIT, O_WAIT, O_ITEMSET, 1, 1, O_WAIT, O_ITEMSET, 2, 1, O_WAIT};
      r.start(); r.g.items[200] = 1; r.vm->intMode = 3; r.frames(1); CHECK(r.g.items[2] == 1 && r.g.items[1] == 0 && r.vm->intMode == 0);
      // a los 64 el interprete antiguo habria leido items[200 & 63 = 8]
      Rig r2; r2.t.ev = r.t.ev; r2.start(); r2.g.items[8] = 1; r2.frames(1); CHECK(r2.g.items[1] == 1 && r2.g.items[2] == 0); }
}
// BRANCHEV / SETEVNO usan la tabla real de tres niveles: el toy tiene eventos con varias fases
static void testBranchEv() {
    // [op, fase0, fase1, ..., -1]: elige la fase fase[branchNo]; defecto = PRIMERA entrada
    auto mk = [](Rig& r) {
        r.t.ev = {{{O_BRANCHEV, 2, 3, NIL}, {O_WAIT}, {O_ITEMSET, 1, 1, O_WAIT}, {O_ITEMSET, 2, 1, O_WAIT}}}; r.start(0, 0); };
    { Rig r; mk(r); r.g.branchNo = 1; r.g.b500F07 = 9; r.vm->intMode = 1; r.frames(1);
      CHECK(r.g.phase == 3 && r.g.items[2] == 1 && r.g.items[1] == 0 && r.g.branchNo == 0 && r.g.b500F07 == 0 && r.vm->intMode == 0); }
    { Rig r; mk(r); r.g.branchNo = 0; r.frames(1); CHECK(r.g.phase == 2 && r.g.items[1] == 1); }
    // sin coincidencia (branchNo 5): defecto = primera entrada (fase 2). game/flow dejaba aqui la palabra -1 (fase -1)
    { Rig r; mk(r); r.g.branchNo = 5; r.frames(1); CHECK(r.g.phase == 2 && r.g.items[1] == 1); }
    // una entrada de valor 0 cuenta como "sin eleccion" y cae al defecto (t == 0 -> defecto) igual que el asm (bnez t0)
    { Rig r; r.t.ev = {{{O_BRANCHEV, 2, 0, NIL}, {O_WAIT}, {O_ITEMSET, 1, 1, O_WAIT}}}; r.start(0, 0); r.g.branchNo = 1; r.frames(1); CHECK(r.g.phase == 2); }
    // SETEVNO
    { Rig r; r.t.ev = {{{O_SETEVNO, 1}, {O_ITEMSET, 3, 3, O_WAIT}}}; r.start(0, 0); r.g.branchNo = 2; r.frames(1); CHECK(r.g.phase == 1 && r.g.items[3] == 3 && r.g.branchNo == 0); }
}
static void testWaits() {
    // WAIT_FRM: n a 60 Hz -> n*50/60 fotogramas PAL; primer fotograma arma la cuenta; se libera al pasar a < 0
    { Rig r; r.t.ev = {{{O_WAIT_FRM, 60, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.vm->frame();
      CHECK(r.vm->frameCnt == 50 && r.vm->wait == 1 && r.g.items[1] == 0);
      int f = 1; while (r.g.items[1] == 0 && f < 200) { r.vm->frame(); f++; }
      CHECK(f == 52 && r.vm->frameCnt == -1 && r.g.items[1] == 1); }
    { Rig r; r.t.ev = {{{O_WAIT_FRM, 7, O_WAIT}}}; r.start(); r.vm->frame(); CHECK(r.vm->frameCnt == 5); }            // 7*50/60 = 5 (hacia cero)
    { Rig r; r.t.ev = {{{O_WAIT_FRM, (uint32_t)-12, O_WAIT}}}; r.start(); r.vm->frame(); CHECK(r.vm->frameCnt == -10); }  // signo: -12*50/60 = -10
    // sin canAdvance la espera de WAIT_FRM no se libera aunque la cuenta termine
    { Rig r; r.t.ev = {{{O_WAIT_FRM, 6, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.h.life0 = 0; r.frames(30); CHECK(r.g.items[1] == 0 && r.vm->frameCnt == -1 && r.vm->wait == 1); }
    // skipReq: las esperas se cumplen al instante y wait = 0
    { Rig r; r.t.ev = {{{O_WAIT_FRM, 600, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.g.skipReq = 1; r.frames(1); CHECK(r.g.items[1] == 1 && r.vm->frameCnt == 0); }
    // WAIT_ITEM: primer paso arma wait; espera objeto != 0 (indice 100: sin enmascarar) + canAdvance + camara lista (d5 o intMode)
    { Rig r; r.t.ev = {{{O_WAIT_ITEM, 100, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.frames(3); CHECK(r.g.items[1] == 0 && r.h.d6 == 0);
      r.g.items[100] = 1; r.h.d5 = false; r.frames(1); CHECK(r.g.items[1] == 0 && r.h.d6 == 1);                  // camara no lista: no pasa (pero d6 = 1)
      r.vm->intMode = 1; r.frames(1); CHECK(r.g.items[1] == 1); }                                                 // intMode lo salta
    // WAIT_ENNUM: objetivo >= recuento. Guarda el objetivo cada fotograma; al liberar lo pone a 0
    { Rig r; r.t.ev = {{{O_WAIT_ENNUM, 2, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.g.enemyCount = 3; r.frames(3); CHECK(r.g.items[1] == 0 && r.g.enemyTarget == 2);
      r.g.enemyCount = 2; r.frames(1); CHECK(r.g.items[1] == 1 && r.g.enemyTarget == 0); }
    { Rig r; r.t.ev = {{{O_WAIT_ENALV, 0, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.g.alvCount = 1; r.frames(3); CHECK(r.g.items[1] == 0); r.g.alvCount = 0; r.frames(1); CHECK(r.g.items[1] == 1 && r.g.alvTarget == 0); }
    { Rig r; r.t.ev = {{{O_WAIT_PEOPLE, 1, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.g.peopleCount = 2; r.frames(3); CHECK(r.g.items[1] == 0 && r.g.peopleTarget == 1); r.g.peopleCount = 1; r.frames(1); CHECK(r.g.items[1] == 1); }
    // WAIT_CAMJOB: skipReq o (sin trabajos y canAdvance); con trabajos cede
    { Rig r; r.t.ev = {{{O_WAIT_CAMJOB, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.h.camPending = true; r.frames(3); CHECK(r.g.items[1] == 0 && r.vm->wait == 1);
      r.h.camPending = false; r.h.life0 = 0; r.frames(1); CHECK(r.g.items[1] == 0);                               // sin canAdvance: sigue esperando
      r.h.life0 = 100; r.frames(1); CHECK(r.g.items[1] == 1); }
    // WAIT_CAMFRM: destino = fotograma - 1 (o endFrame - 1 si 0); libera cuando frame >= destino
    { Rig r; r.t.ev = {{{O_WAIT_CAMFRM, 0, 10, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.h.cframe = 8.f; r.frames(3); CHECK(r.g.items[1] == 0);
      r.h.cframe = 8.99f; r.frames(1); CHECK(r.g.items[1] == 0); r.h.cframe = 9.f; r.frames(1); CHECK(r.g.items[1] == 1); }
    { Rig r; r.t.ev = {{{O_WAIT_CAMFRM, 0, 0, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.h.cend = 100; r.h.cframe = 98.f; r.frames(3); CHECK(r.g.items[1] == 0); r.h.cframe = 99.f; r.frames(1); CHECK(r.g.items[1] == 1); }
    // WAIT_FLUSHDATA: con datos pendientes pide pausa, para el bucle (stopFlag) pero avanza el pc; _NOPAUSE no avanza hasta que no queden
    { Rig r; r.t.ev = {{{O_WAIT_FLUSHDATA, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.h.remain = true; r.frames(1);
      CHECK(r.h.reqPauseAll == 1 && r.vm->stopFlag == 1 && r.g.items[1] == 0 && r.vm->pc == r.t.addr[0][0] + 4); r.frames(1); CHECK(r.g.items[1] == 1); }
    { Rig r; r.t.ev = {{{O_WAIT_FLUSHDATA_NOPAUSE, O_ITEMSET, 1, 1, O_WAIT}}}; r.start(); r.h.remain = true; r.frames(4); CHECK(r.vm->pc == r.t.addr[0][0] && r.h.reqPauseAll == 0);
      r.h.remain = false; r.frames(1); CHECK(r.g.items[1] == 1); }
    // WAIT_LOAD_OVLMOD: si el modulo no esta cargado pide pausa; el pc avanza siempre
    { Rig r; r.t.ev = {{{O_WAIT_LOAD_OVLMOD, 4, O_WAIT}}}; r.start(); r.h.ovlStat = 1; r.frames(1); CHECK(r.h.ovlReq.size() == 1 && r.h.ovlReq[0] == 4); }
    // JOBFLAGSET escribe wait; WAIT_INT_MODE escribe intMode
    { Rig r; r.t.ev = {{{O_JOBFLAGSET, 1, O_WAIT_INT_MODE, 2, O_WAIT}}}; r.start(); r.frames(1); CHECK(r.vm->wait == 1 && r.vm->pc == r.t.addr[0][0] + 8 && r.vm->intMode == 0);
      r.vm->wait = 0; r.frames(1); CHECK(r.vm->intMode == 2); }
}
static void testState() {
    // ITEMSET/GAMEFLAGSET/BSFT/NOWLOADINGSET/SNOWDISP/GETCPMODE/BGTB/SKIP/ZMAT/WLCTB
    { Rig r; r.t.ev = {{{O_ITEMSET, 40, 5, O_ITEMSET, 40, 0, O_ITEMSET, 255, 1, O_GAMEFLAGSET, 1, 0x11, O_GAMEFLAGSET, 0, 0x1, O_BSFT, 7, O_NOWLOADINGSET, 1, O_SNOWDISP, 2,
        O_GETCPMODE, 3, O_BGTB, 9, O_WAIT}}};
      r.start(); r.frames(1); CHECK(r.g.items[40] == 0 && r.g.items[255] == 1 && r.g.gameFlags == 0x10 && r.g.bsft == 7 && r.g.nowLoading == 1 && r.g.snowDisp == 2 && r.g.gp730C == 3 && r.h.bg == 9); }
    { Rig r; r.t.ev = {{{O_SKIP, 1, O_WAIT}}}; r.start(); r.g.skipPending = 5; r.frames(1); CHECK(r.g.skipEnabled == 1 && r.g.skipPending == 0);
      Rig q; q.t.ev = {{{O_SKIP, 0, O_WAIT}}}; q.start(); q.g.skipReq = 1; q.g.skipEnabled = 1; q.frames(1); CHECK(q.g.skipEnabled == 0 && q.g.skipReq == 0); }
    { Rig r; r.t.ev = {{{O_ZMAT, 3, 0x1FF, 7, NIL, O_WAIT}}}; r.start(); r.frames(1); CHECK(r.g.zmat[0] == 3 && r.g.zmat[1] == 0xFF && r.g.zmat[2] == 7 && r.vm->fault == EvtFault::None && r.vm->pc == r.t.addr[0][0] + 20);
      Rig q; q.t.ev = {{{O_ZMAT, 1, 2, 3, 4, NIL, O_WAIT}}}; q.start(); q.frames(1); CHECK(q.vm->fault == EvtFault::ZmatOverflow); }        // 4 elementos: assert (break 0) en el original
    { Rig r; r.t.ev = {{{O_WLCTB, 10, 20, NIL, O_WAIT}}}; r.start(); r.frames(1); CHECK(r.g.wlctbTable == r.t.addr[0][0] + 4 && r.vm->pc == r.t.addr[0][0] + 16); }
    // ENTRYSUBGIRL: gameState == 10, modo 1 y un 0x2E en los bytes +2/+3 de alguna fila
    { Rig r; r.t.ev = {{{O_ENTRYSUBGIRL, O_WAIT}}}; r.start(); r.g.gameState = 10; r.g.playMode = 1; r.g.subGirlName[4 + 3] = 0x2E; r.frames(1);
      CHECK(r.h.cmds.size() == 1 && r.h.cmds[0].cond);
      Rig q; q.t.ev = r.t.ev; q.start(); q.g.gameState = 10; q.g.playMode = 1; q.g.subGirlName[1] = 0x2E; q.frames(1); CHECK(!q.h.cmds[0].cond);   // el byte +1 no cuenta
      Rig p; p.t.ev = r.t.ev; p.start(); p.g.gameState = 9; p.g.playMode = 1; p.g.subGirlName[2] = 0x2E; p.frames(1); CHECK(!p.h.cmds[0].cond); }
    // SCEFFINOUT: el original no avanza ni cede (se cuelga); aqui se registra
    { Rig r; r.t.ev = {{{O_SCEFFINOUT, O_WAIT}}}; r.start(); r.frames(1); CHECK(r.vm->fault == EvtFault::SceffinoutLoop); }
    // FORCE_CAMPARAM y VIBRATION
    { Rig r; r.t.ev = {{{O_FORCE_CAMPARAM, 2, 1, 0x3F800000, O_FORCE_CAMPARAM, 1, 0, 120, O_FORCE_CAMPARAM, 1, 7, 5, O_WAIT}}}; r.start(); r.frames(1);
      CHECK(r.h.forces.size() == 2 && r.h.forces[0][1] == 1 && r.h.forces[0][2] == 0x3F800000 && r.h.forces[1][1] == 0 && r.h.forces[1][2] == 120 && r.vm->pc == r.t.addr[0][0] + 48); }
    { Rig r; r.t.ev = {{{O_VIBRATION, 0, 1, 2, O_VIBRATION, 1, O_VIBRATION, 2, 1, 2, 3, O_WAIT}}}; r.start(); r.g.skipReq = 1; r.frames(1);
      CHECK(r.h.cmds.size() == 3 && r.h.cmds[0].skipped && !r.h.cmds[1].skipped && r.h.cmds[2].skipped && r.vm->pc == r.t.addr[0][0] + 4 * 11); }
    { Rig r; r.t.ev = {{{O_VIBRATION, 9, O_WAIT}}}; r.start(); r.frames(1); CHECK(r.vm->fault == EvtFault::UnknownOpcode); }
    // opcode nulo (6, 8, 19, 48, 111-slot... ver tabla): ptmf nulo
    { Rig r; r.t.ev = {{{6, O_WAIT}}}; r.start(); r.frames(1); CHECK(r.vm->fault == EvtFault::UnknownOpcode); }
}
static void testNextEv() {
    // fin de lista de fases -> tabla de transicion de la etapa 0 (GAMEMOD): {0,1}, {2,0}, {3,0}: evento 0 -> 1; evento 1 -> 2; evento 2 -> fin de fase
    { Rig r; r.t.ev = {{{O_ITEMSET, 1, 1, O_NEXTEV}}, {{O_ITEMSET, 2, 1, O_NEXTEV}}, {{O_ITEMSET, 3, 1, O_NEXTEV}}}; r.start(0, 0);
      r.g.branchNo = 4; r.vm->intMode = 1; r.frames(1);                 // ev0/ph0 -> NEXTEV: fase 1 no existe (-1) -> tipo 0 -> evento 1, fase 1
      CHECK(r.g.event == 1 && r.g.phase == 1 && r.g.branchNo == 0 && r.vm->intMode == 0); }
    // pasar a la fase siguiente dentro del mismo evento
    { Rig r; r.t.ev = {{{O_NEXTEV}, {O_ITEMSET, 1, 1, O_WAIT}}}; r.start(0, 0); r.g.branchNo = 3; r.g.b500F05 = 1; r.frames(1);
      CHECK(r.g.event == 0 && r.g.phase == 1 && r.g.items[1] == 1 && r.g.branchNo == 3); }          // b500F05 != 0 conserva branchNo
    // tipo 2 (evento + 1): fila 1 de la etapa 0 -> {2,0}
    { Rig r; r.t.ev = {{{O_WAIT}}, {{O_WAIT}, {O_NEXTEV}}, {{O_WAIT}, {O_ITEMSET, 5, 5, O_WAIT}}}; r.start(1, 1); r.frames(1); CHECK(r.g.event == 2 && r.g.phase == 1 && r.g.items[5] == 5); }
    // tipo 3 (fase superada): fila 2 -> EventStageClear + pc = EVTPAL[8][0][0], fase 0, evento = a (0); gameState 10 pasa mode73F0 a 5
    { Rig r; r.t.ev = {{{O_WAIT}}, {{O_WAIT}}, {{O_WAIT}, {O_NEXTEV}}}; r.start(2, 1); r.g.gameState = 10; r.g.bsft = 5; r.g.branchNo = 2; r.frames(1);
      CHECK(r.h.clears == 1 && r.g.stageClearFlag == 1 && r.g.bsft == 0 && r.g.mode73F0 == 5 && r.g.event == 0 && r.g.phase == 0 && r.vm->pc == r.t.clearAddr && r.g.branchNo == 2 && r.h.hitBonus == 0);
      r.frames(1); CHECK(r.vm->pc == r.t.clearAddr && r.vm->wait == 1); }
    { Rig r; r.t.ev = {{{O_WAIT}}, {{O_WAIT}}, {{O_WAIT}, {O_NEXTEV}}}; r.start(2, 1); r.g.gameState = 10; r.g.mode73F0 = 8; r.frames(1); CHECK(r.g.mode73F0 == 8); }   // 8 no se toca
    // tipo 1 con branchNo: tabla real de la etapa 1, fila 0 = {1,2,2,-1}; fila 13 = {1,14,31,-1}; fila 38 = {1,39,40,46}
    { DefaultTrans d; int16_t r[4]; CHECK(d.row(1, 38, r) && r[0] == 1 && r[1] == 39 && r[2] == 40 && r[3] == 46); }
}
static void testEvCancelAndInit() {
    EvtGlobals g; EvCancelState s;
    CHECK(evCancelCtrl(g, s) == EvCancelAct::DeleteSelf);                          // skip no habilitado: la tarea se borra
    g.skipEnabled = 1; CHECK(evCancelCtrl(g, s) == EvCancelAct::Nothing && g.skipReq == 0);
    s.buttonFlag = 1; CHECK(evCancelCtrl(g, s) == EvCancelAct::DoSkip && g.skipReq == 1 && g.skipPending == 1 && s.buttonFlag == 0);
    evCancelSkip(g); CHECK(g.skipPending == 0);
    EvtGlobals h; for (int i = 0; i < 256; i++) h.items[i] = (int8_t)(i | 1); h.enemyCount = 3; h.alvCount = 2; h.peopleCount = 1; h.bsft = 4; h.stageClearFlag = 1;
    evtInitStage(h); bool z = true; for (int i = 0; i < 256; i++) z = z && h.items[i] == 0;
    CHECK(z && h.enemyCount == 0 && h.alvCount == 0 && h.peopleCount == 0 && h.bsft == 0 && h.stageClearFlag == 0);
}

// ---------------------------------------------------------------- datos reales
static std::vector<uint32_t> gEvt;
static std::vector<uint8_t> gGame;
static uint32_t gmWord16(uint32_t addr, int i) { size_t o = addr - 0x92B000u + 2u * (size_t)i; int16_t v = 0; if (o + 2 <= gGame.size()) memcpy(&v, &gGame[o], 2); return (uint16_t)v; }
static uint32_t gmWord32(uint32_t addr) { size_t o = addr - 0x92B000u; uint32_t v = 0; if (o + 4 <= gGame.size()) memcpy(&v, &gGame[o], 4); return v; }

static const int kEvCount[12] = {3, 47, 13, 24, 20, 4, 18, 2, 1, 3, 1, 1};     // eventos por etapa en EVTPAL.BIN (medido)
static void testRealStructure(const ScriptMem& mem) {
    const uint32_t B = kBase, end = B + (uint32_t)gEvt.size() * 4;
    for (int s = 0; s < 12; s++) {
        uint32_t l1 = mem.rd(B + 4u * (uint32_t)s); int n = 0;
        while (mem.rd(l1 + 4u * (uint32_t)n) >= B && mem.rd(l1 + 4u * (uint32_t)n) < end) n++;
        CHECK(n == kEvCount[s]);
    }
    CHECK(mem.rd(B + 40) == mem.rd(B + 44));                                         // las etapas 10 y 11 comparten tabla
    CHECK(mem.rd(B + 40) != mem.rd(B + 36));
    // numero de fases por evento de la etapa 10/11: un evento con 23 fases
    uint32_t l2 = mem.rd(mem.rd(B + 40)); int ph = 0; while (mem.rd(l2 + 4u * (uint32_t)ph) >= B && mem.rd(l2 + 4u * (uint32_t)ph) < end) ph++;
    CHECK(ph == 23);
}
static void testTransitionsVsGamemod() {
    if (gGame.empty()) { skip("disc/TBL/GAMEMOD.BIN no disponible: no se comparan las tablas de transicion"); return; }
    // puntero por etapa en 0x9B83D0 (12 punteros)
    uint32_t ptr[12]; for (int s = 0; s < 12; s++) ptr[s] = gmWord32(0x9B83D0u + 4u * (uint32_t)s);
    CHECK(ptr[0] == 0x9B8010 && ptr[1] == 0x9B8030 && ptr[2] == 0x9B81B0 && ptr[3] == 0x9B8220 && ptr[4] == 0x9B82E0 && ptr[5] == 0x9B8380);
    CHECK(ptr[6] == ptr[0] && ptr[7] == 0x9B83C0 && ptr[8] == 0 && ptr[9] == 0x9B83A0 && ptr[10] == 0x9B83B8 && ptr[11] == ptr[10]);
    // cada fila de kEvTransTables == fila del BIN (para las etapas con tabla propia, tantas filas como eventos)
    int compared = 0, bad = 0;
    DefaultTrans d;
    for (int s = 0; s < 12; s++) {
        if (ptr[s] == 0) { int16_t r[4]; CHECK(!d.row(s, 0, r) || true); continue; }
        int rows = kEvCount[s]; if (s == 6) rows = 3;                              // la etapa 6 reutiliza la tabla de la 0 (solo 3 filas utiles)
        for (int e = 0; e < rows; e++) {
            int16_t r[4]; if (!d.row(s, e, r)) { bad++; continue; }
            for (int k = 0; k < 4; k++) { compared++; if ((uint16_t)r[k] != gmWord16(ptr[s] + 8u * (uint32_t)e, k)) bad++; }
        }
    }
    printf("(info) transiciones: %d campos comparados con GAMEMOD.BIN, %d discrepancias\n", compared, bad);
    CHECK(bad == 0 && compared > 400);
    // la ultima fila real de cada etapa 0..5, 7, 9, 10 es de tipo 3 (fase superada), salvo la etapa 3/4 donde el evento final es 23/19 -> {3,0}
    int16_t r[4];
    CHECK(d.row(0, 2, r) && r[0] == 3); CHECK(d.row(1, 46, r) && r[0] == 3); CHECK(d.row(2, 12, r) && r[0] == 3); CHECK(d.row(3, 23, r) && r[0] == 3);
    CHECK(d.row(4, 19, r) && r[0] == 3); CHECK(d.row(5, 3, r) && r[0] == 3); CHECK(d.row(7, 1, r) && r[0] == 3); CHECK(d.row(9, 1, r) && r[0] == 3 && d.row(9, 2, r) && r[0] == 3);
    CHECK(d.row(10, 0, r) && r[0] == 3);
}

// nombre de handler (glabel) -> direccion, leido de los .s del repo
static void testHandlerNamesVsAsm() {
    std::ifstream f("asm/EventCtrlTask_1E2D10.s");
    if (!f) { skip("asm/EventCtrlTask_1E2D10.s no disponible: no se comparan nombres de rutina con el asm"); return; }
    std::map<uint32_t, std::string> byAddr; std::string line, cur;
    while (std::getline(f, line)) {
        if (line.compare(0, 7, "glabel ") == 0) { cur = line.substr(7); size_t p = cur.find("__13EventCtrlTaskFv"); cur = p == std::string::npos ? "" : cur.substr(0, p); continue; }
        if (!cur.empty()) {                         // primera instruccion tras el glabel: /* off 001E3BE0 hex */
            size_t a = line.find("/* "); if (a == std::string::npos) continue;
            unsigned off, addr; char hex[16]; if (sscanf(line.c_str() + a, "/* %x %x %15s", &off, &addr, hex) == 3) { byAddr[addr] = cur; cur.clear(); }
        }
    }
    int matched = 0, mism = 0;
    for (int op = 0; op < kEvOpcodeSlots; op++) {
        if (kEvHandlerAddr[op] == 0) continue;
        auto it = byAddr.find(kEvHandlerAddr[op]);
        if (it == byAddr.end()) { mism++; printf("(info) opcode %d: direccion %x sin glabel EventCtrlTask en el asm\n", op, kEvHandlerAddr[op]); continue; }
        if (it->second == evOpName(evOpFromOpcodeV2((uint32_t)op))) matched++;
        else { mism++; printf("(info) opcode %d: nombre del asm %s != %s\n", op, it->second.c_str(), evOpName(evOpFromOpcodeV2((uint32_t)op))); }
    }
    printf("(info) nombres de las 111 rutinas contra los glabel del asm: %d coinciden, %d no\n", matched, mism);
    CHECK(matched == 111 && mism == 0);
}

struct PlayHost : TestHost {
    int playerLife(int p) override { return p == 0 ? 100 : 0; }
    int playerMode(int p) override { return p == 0 ? 5 : 0; }
};
struct PlayResult { bool reachedClear = false; bool parked = false; int frames = 0; uint32_t parkPc = 0; std::vector<int> path; std::set<uint32_t> executed; int cmds = 0; EvtFault fault = EvtFault::None; };
// Recorrido de una etapa con un "motor" minimo: las esperas por objeto se cumplen solas, los contadores de enemigos estan a 0 y la politica de
// rutas elige branchNo = veces que ya se paso por esa fase (tope 2). Termina al llegar al script de EventStageClear o al aparcar en un WAIT.
static PlayResult playStage(const ScriptMem& mem, int stage, int event, int maxFrames, std::vector<EvtCmd>* log = nullptr) {
    PlayResult R; EvtGlobals g; PlayHost h; DefaultTrans tr; g.stage = (int8_t)stage; g.event = (int8_t)event; g.credits = 1;
    EvtVm vm(g, mem, h, tr); vm.construct(); std::vector<uint32_t> tr_; vm.trace = &tr_;
    uint32_t clearPc = mem.rd(mem.rd(mem.rd(kBase + 0x20)));
    std::map<int, int> visits; int lastEv = -1, lastPh = -1;
    for (R.frames = 0; R.frames < maxFrames; R.frames++) {
        size_t n0 = h.cmds.size();
        vm.frame();
        if (log) for (size_t i = n0; i < h.cmds.size(); i++) log->push_back(h.cmds[i]);
        if (vm.fault != EvtFault::None) { R.fault = vm.fault; break; }
        if (g.event != lastEv || g.phase != lastPh) {
            R.path.push_back(g.event * 100 + g.phase); lastEv = g.event; lastPh = g.phase;
            int k = visits[g.event * 100 + g.phase]++; if (g.b500F05 == 0) g.branchNo = (int8_t)(k < 2 ? k : 2);
        }
        if (vm.pc == clearPc && g.event == 0 && g.phase == 0 && vm.wait) { R.reachedClear = true; break; }
        if (vm.wait && mem.rd(vm.pc) == O_WAIT_ITEM) g.items[mem.rd(vm.pc + 4) & 0xFF] = 1;
        if (vm.wait && mem.rd(vm.pc) == O_WAIT && R.frames > 3) { R.parked = true; R.parkPc = vm.pc; break; }
    }
    R.executed.insert(tr_.begin(), tr_.end()); R.cmds = (int)h.cmds.size();
    return R;
}
static void testPlayStages(const ScriptMem& mem) {
    for (int s : {0, 1, 2, 3, 4, 5, 9, 10, 11}) {
        PlayResult r = playStage(mem, s, 0, 20000);
        printf("(info) etapa %2d: %s en %5d fotogramas, %3zu cambios de fase, %4d ordenes de motor, %5zu pc distintos\n", s, r.reachedClear ? "llega a EventStageClear" : "NO llega", r.frames, r.path.size(), r.cmds, r.executed.size());
        CHECK(r.fault == EvtFault::None); CHECK(r.reachedClear);
    }
    // etapas 6 (entrenamiento), 7 (la fija GameInit con gp-0x73B4 == 1) y 8 (EventStageClear): los scripts aparcan en WAIT hasta que otro modulo los cambia
    PlayResult t6 = playStage(mem, 6, 0, 100); CHECK(t6.parked && t6.fault == EvtFault::None && t6.parkPc == 0x1fedb88);
    PlayResult t7 = playStage(mem, 7, 1, 100); CHECK(t7.parked && t7.fault == EvtFault::None && t7.parkPc == 0x1fed9fc);
}

// Desde cada script de EVTPAL (todas las etapas incluidas la 10 y la 11) ejecuta el VM con esperas satisfechas y comprueba que cada instruccion
// ejecutada fue tambien visitada por el barrido estatico (misma tabla de longitudes), y que ningun script cae en opcode nulo ni bucle.
static void testEverySingleScript(const ScriptMem& mem) {
    const uint32_t B = kBase, end = B + (uint32_t)gEvt.size() * 4;
    int scripts = 0, ended = 0, parked = 0, fault = 0, other = 0, notStatic = 0; std::map<uint32_t, int> lenOf;
    // barrido estatico de toda la tabla (etapas 0..11)
    std::set<uint32_t> staticSet;
    for (int st = 0; st < 12; st++) {
        uint32_t l1 = mem.rd(B + 4u * (uint32_t)st);
        for (int ev = 0; mem.rd(l1 + 4u * (uint32_t)ev) >= B && mem.rd(l1 + 4u * (uint32_t)ev) < end; ev++) {
            uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)ev);
            for (int ph = 0; mem.rd(l2 + 4u * (uint32_t)ph) >= B && mem.rd(l2 + 4u * (uint32_t)ph) < end; ph++) {
                EvScanOptions o; o.stage = st; o.event = ev; o.followPhaseJumps = true; std::vector<EvScanResult::Ins> v;
                evScanScript(mem, mem.rd(l2 + 4u * (uint32_t)ph), o, &v, B, (uint32_t)gEvt.size() * 4);
                for (auto& i : v) staticSet.insert(i.pc);
            }
        }
    }
    for (int st = 0; st < 12; st++) {
        uint32_t l1 = mem.rd(B + 4u * (uint32_t)st);
        for (int ev = 0; mem.rd(l1 + 4u * (uint32_t)ev) >= B && mem.rd(l1 + 4u * (uint32_t)ev) < end; ev++) {
            uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)ev);
            for (int ph = 0; mem.rd(l2 + 4u * (uint32_t)ph) >= B && mem.rd(l2 + 4u * (uint32_t)ph) < end; ph++) {
                EvtGlobals g; PlayHost h; DefaultTrans tr; g.stage = (int8_t)st; g.event = (int8_t)ev; g.phase = (int8_t)ph; g.credits = 1;
                EvtVm vm(g, mem, h, tr); vm.pc = g.savedPc = mem.rd(l2 + 4u * (uint32_t)ph); std::vector<uint32_t> tr_; vm.trace = &tr_;
                scripts++;
                int startEv = ev, startPh = ph; bool done = false;
                for (int f = 0; f < 3000 && !done; f++) {
                    vm.frame();
                    if (vm.fault != EvtFault::None) { fault++; done = true; break; }
                    if (g.event != startEv || g.phase != startPh) { ended++; done = true; break; }       // salio por NEXTEV/BRANCHEV/SETEVNO
                    if (vm.wait && mem.rd(vm.pc) == O_WAIT_ITEM) g.items[mem.rd(vm.pc + 4) & 0xFF] = 1;
                    if (vm.wait && mem.rd(vm.pc) == O_WAIT) { parked++; done = true; break; }
                }
                if (!done) other++;
                for (uint32_t p : tr_) if (!staticSet.count(p)) { notStatic++; break; }
            }
        }
    }
    printf("(info) %d scripts (etapas 0..11, 10 y 11 incluidas): %d salen por NEXTEV/BRANCHEV/SETEVNO, %d aparcan en WAIT, %d fallos, %d sin terminar, %d con pc fuera del barrido estatico\n",
           scripts, ended, parked, fault, other, notStatic);
    CHECK(fault == 0 && other == 0 && notStatic == 0);
    CHECK(parked == 20);            // los 20 scripts que acaban en WAIT sin fin (informe de flow_rest) aparcan tambien al ejecutarlos
}

// Los 20 scripts que acaban en WAIT: clasificacion por lo que hacen antes de aparcar (ver MANIFEST: el modulo que los libera no es EventCtrl).
static void testParkedScripts(const ScriptMem& mem) {
    const uint32_t B = kBase, end = B + (uint32_t)gEvt.size() * 4;
    int n = 0, train = 0, mission = 0, shared = 0; std::set<uint32_t> parkAddr;
    for (int st = 0; st < 12; st++) {
        uint32_t l1 = mem.rd(B + 4u * (uint32_t)st);
        for (int ev = 0; mem.rd(l1 + 4u * (uint32_t)ev) >= B && mem.rd(l1 + 4u * (uint32_t)ev) < end; ev++) {
            uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)ev);
            for (int ph = 0; mem.rd(l2 + 4u * (uint32_t)ph) >= B && mem.rd(l2 + 4u * (uint32_t)ph) < end; ph++) {
                EvtGlobals g; PlayHost h; DefaultTrans tr; g.stage = (int8_t)st; g.event = (int8_t)ev; g.phase = (int8_t)ph;
                EvtVm vm(g, mem, h, tr); vm.pc = g.savedPc = mem.rd(l2 + 4u * (uint32_t)ph); std::vector<uint32_t> t; vm.trace = &t;
                std::vector<uint32_t> waited; bool parked = false;
                for (int f = 0; f < 3000; f++) {
                    vm.frame(); if (g.event != ev || g.phase != ph) break;
                    if (vm.wait && mem.rd(vm.pc) == O_WAIT_ITEM) { uint32_t it = mem.rd(vm.pc + 4); if (waited.empty() || waited.back() != it) waited.push_back(it); g.items[it & 0xFF] = 1; }
                    if (vm.wait && mem.rd(vm.pc) == O_WAIT) { parked = true; break; }
                }
                if (!parked) continue;
                n++; if (!parkAddr.insert(vm.pc).second) shared++;
                if (st == 6) {
                    train++;
                    // entrenamiento: eventos 1..11 esperan los objetos 0x21 (inicio hecho por TrainingTask) y 0x22 (fin); 12..17 esperan 0x23
                    if (ev >= 1 && ev <= 11) { mission++; CHECK(!waited.empty() && waited[0] == 0x21); }
                    if (ev >= 12) CHECK(waited.size() == 1 && waited[0] == 0x23);
                }
            }
        }
    }
    printf("(info) scripts parados: %d (etapa 6: %d, de ellos %d misiones de entrenamiento con espera 0x21; %d comparten direccion de WAIT con otro)\n", n, train, mission, shared);
    CHECK(n == 20 && train == 18 && mission == 11 && shared == 4);
}

// Estadisticas de uso en los scripts reales (alcanzables): objetos usados, ZMAT, opcodes que no aparecen nunca.
static void testUsage(const ScriptMem& mem) {
    const uint32_t B = kBase, end = B + (uint32_t)gEvt.size() * 4;
    std::map<uint32_t, int> hist; int maxItem = 0, maxZ = 0, sceff = 0;
    for (int st = 0; st < 12; st++) {
        uint32_t l1 = mem.rd(B + 4u * (uint32_t)st);
        for (int ev = 0; mem.rd(l1 + 4u * (uint32_t)ev) >= B && mem.rd(l1 + 4u * (uint32_t)ev) < end; ev++) {
            uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)ev);
            for (int ph = 0; mem.rd(l2 + 4u * (uint32_t)ph) >= B && mem.rd(l2 + 4u * (uint32_t)ph) < end; ph++) {
                EvScanOptions o; o.stage = st; o.event = ev; o.followPhaseJumps = true; std::vector<EvScanResult::Ins> v;
                evScanScript(mem, mem.rd(l2 + 4u * (uint32_t)ph), o, &v, B, (uint32_t)gEvt.size() * 4);
                for (auto& i : v) {
                    hist[i.opcode]++;
                    if (i.opcode == O_ITEMSET || i.opcode == O_WAIT_ITEM || i.opcode == O_BRANCHITEM) maxItem = std::max<int>(maxItem, (int)mem.rd(i.pc + 4));
                    if (i.opcode == O_ZMAT) { int c = 0; while (mem.rd(i.pc + 4 + 4u * (uint32_t)c) != NIL) c++; maxZ = std::max(maxZ, c); }
                    if (i.opcode == O_SCEFFINOUT) sceff++;
                }
            }
        }
    }
    std::string unused; int nu = 0;
    for (int op = 0; op < kEvOpcodeSlots; op++) if (kEvHandlerAddr[op] && !hist.count((uint32_t)op)) { unused += " " + std::to_string(op) + ":" + evOpName(evOpFromOpcodeV2((uint32_t)op)); nu++; }
    printf("(info) objeto mayor usado por ITEMSET/WAIT_ITEM/BRANCHITEM: %d; ZMAT mayor: %d elementos; SCEFFINOUT alcanzables: %d; opcodes validos sin uso alcanzable (%d):%s\n", maxItem, maxZ, sceff, nu, unused.c_str());
    CHECK(sceff == 0 && maxZ <= 3 && maxItem < 256);
}


// ---------------------------------------------------------------- camara game_000
namespace {
struct NoCurves : CamCurveSet { bool curve(int, int, FCurve&) const override { return false; } };
struct CEng : CamEngine {
    int arcTan2(float y, float x) override { return (int)std::lround(std::atan2(y, x) * 32768.0 / 3.14159265358979323846); }
    void getAngleYX(float, float, float, int* a, int* b) override { *a = *b = 0; }
};
struct CamT : CameraGeo3 {
    using CameraGeo3::CameraGeo3;
    int gc = 0, gi = 0; float cosV = 0; float lastIn[3] = {};
    void calcGameCam(int) override { gc++; }
    void calcGameInt(int) override { gi++; }
    float nlTan(int a) override { return (float)std::tan(a * 3.14159265358979323846 / 32768.0); }
    float vecCos(const float* a, const float* b) override { (void)a; (void)b; return cosV; }
    void interDivCamAng3D(const Anpo&, const float*, const Anpo& l, float* o, int, int) override { o[0] = l.x; o[1] = l.y; o[2] = l.z; }
};
}
static void testCamera() {
    NoCurves nc; CEng eng; VecMem m(0x1000, {16, 0, 0x30, 0, 0, 0});
    auto mk = [&](CamJobQueue& q) { q.mem = &m; q.ringRec[0] = 0x1000; q.ringState[0] = 2; q.slot[0].ring = 0; q.head = 0; q.tail = 0; };
    // 1) cerca del destino (<= 0.1): sin ajuste de mirada; d4 = 1 y d3 = 1 -> termina el trabajo y libera los sub-registros
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.cosV = 1.0f;
        g.cam[0].d3 = 1; g.cam[0].pos.x = 5; g.slot[0].tgtPos[0] = 5; g.cam[0].look.z = 3; g.slot[0].tgtLook[2] = 3;
        g.slot[0].flag47 = 1; q.slot[0].sub[1] = q.slot[0].sub[2] = q.slot[0].sub[3] = 7;
        g.game000(0);
        CHECK(g.gc == 1 && g.gi == 1);
        CHECK(g.cam[0].d4 == 1 && q.cam[0].flagD5 == 1 && q.cam[0].splCnt == 0 && q.slot[0].cafsIdx == 1);
        CHECK(q.slot[0].sub[1] == -2 && q.slot[0].sub[2] == -2 && q.slot[0].sub[3] == -2 && q.ringState[0] == 0);
        CHECK(g.cam[0].look.z == 3);
    }
    // 2) d4 = 0 con flag47 = 1: no termina
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.cosV = 0.5f;
        g.cam[0].d3 = 1; g.slot[0].flag47 = 1; q.slot[0].sub[1] = 7;
        g.game000(0);
        CHECK(g.cam[0].d4 == 0 && q.slot[0].sub[1] == 7 && q.ringState[0] == 2 && q.cam[0].flagD5 == 0);
    }
    // 3) flag47 = 0 termina aunque d4 = 0; pero solo si d3 == 1
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.cosV = 0.f; g.cam[0].d3 = 0; g.slot[0].flag47 = 0;
        g.game000(0); CHECK(q.ringState[0] == 2 && q.cam[0].flagD5 == 0);
        CamJobQueue q2; mk(q2); CamT g2(q2, eng, nc); g2.cam[0].d3 = 1; g2.slot[0].flag47 = 0;
        g2.game000(0); CHECK(q2.ringState[0] == 0 && q2.cam[0].flagD5 == 1);
    }
    // 4) sub-registro listo (-1) con d3 == 1: pos = (pos*15 + sub)/16; con otro valor no se mueve
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.cam[0].d3 = 1; g.branchNoV = 1; q.slot[0].sub[2] = -1; g.slot[0].flag47 = 1;
        g.slot[0].subPos[1].x = 16; g.slot[0].tgtPos[0] = g.slot[0].tgtPos[1] = g.slot[0].tgtPos[2] = 0;
        g.game000(0); CHECK(std::fabs(g.cam[0].pos.x - 1.0f) < 1e-5f);
        CamJobQueue q2; mk(q2); CamT g2(q2, eng, nc); g2.cam[0].d3 = 1; g2.branchNoV = 1; q2.slot[0].sub[2] = 5; g2.slot[0].flag47 = 1; g2.slot[0].subPos[1].x = 16;
        g2.game000(0); CHECK(g2.cam[0].pos.x == 0.0f);
        CamJobQueue q3; mk(q3); CamT g3(q3, eng, nc); g3.branchNoV = 9; g3.game000(0); CHECK(g3.outOfRange == 1);
    }
    // 5) ajuste de mirada: pos lejos del destino; look = (0,0,10), pos = 0, tgtPos = (0,0,-50), tgtLook = (10,0,10)
    //    B = look + pos - tgtPos = (0,0,60); f20 = (0,0,10), f23 = (0,0,60), f26 = (10,0,10): c1 = c2 = 0 -> no entra en el recorte
    //    (signos no estrictos): la altura de mirada se reajusta con tan(clamp) y look.x/z no cambian.
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.slot[0].flag47 = 1; g.cosV = 0;
        g.cam[0].look.z = 10; g.slot[0].tgtPos[2] = -50; g.slot[0].tgtLook[0] = 10; g.slot[0].tgtLook[2] = 10;
        g.game000(0);
        CHECK(g.cam[0].look.x == 0 && g.cam[0].look.z == 10 && std::fabs(g.cam[0].look.y) < 1e-3f);
    }
    // 6) recorte con signos iguales y estrictos: f20=(1,.,1)->c1 = f20.x*f25 - f22*f23 > 0 y c2 > 0
    //    pos = 0, look = (1,0,1), tgtPos = (0,0,-100): B = (1,0,101); f20=(1,0,1), f23=(1,0,101), f26=tgtLook
    //    tgtLook = (1,0,3): f26=(1,0,3): c1 = 1*101 - 1*1 = 100 > 0; c2 = 1*3 - 1*1 = 2 > 0 -> entra
    //    vecAng(f20,B') = 45 - ~0.57 grados; vecAng(f20,T) = 45 - 18.4 grados: s4 (a B) > v -> look.xz = tgtLook.xz
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.slot[0].flag47 = 1;
        g.cam[0].look.x = 1; g.cam[0].look.z = 1; g.slot[0].tgtPos[2] = -100; g.slot[0].tgtLook[0] = 1; g.slot[0].tgtLook[2] = 3;
        g.game000(0);
        CHECK(g.cam[0].look.x == 1 && g.cam[0].look.z == 3);
        // el otro lado: tgtLook = (1,0,1.01) esta mas cerca de la mirada que B' -> hay s4 < v y look.xz = B
        CamJobQueue q2; mk(q2); CamT g2(q2, eng, nc); g2.slot[0].flag47 = 1;
        g2.cam[0].look.x = 1; g2.cam[0].look.z = 1; g2.slot[0].tgtPos[2] = -3; g2.slot[0].tgtLook[0] = 1; g2.slot[0].tgtLook[2] = 100;
        g2.game000(0);
        CHECK(g2.cam[0].look.x == 1 && g2.cam[0].look.z == 4);   // B = look + pos - tgtPos = (1,0,4)
    }
    // 7) runHandler despacha CH_GAME000 a game000
    {
        CamJobQueue q; mk(q); CamT g(q, eng, nc); g.runHandler(CH_GAME000, 0); CHECK(g.gc == 1 && g.pending == 0);
    }
}

int main() {
    testOpcodeNumbers();
    testConstructAndFrame(); testBranches(); testBranchEv(); testWaits(); testState(); testNextEv(); testEvCancelAndInit(); testCamera();
    std::vector<uint8_t> raw = readFile("disc/TBL/EVTPAL.BIN");
    gGame = readFile("disc/TBL/GAMEMOD.BIN");
    if (raw.empty()) skip("disc/TBL/EVTPAL.BIN no disponible: se omiten las pruebas con scripts reales");
    else {
        gEvt.resize(raw.size() / 4); memcpy(gEvt.data(), raw.data(), gEvt.size() * 4);
        VecMem mem(kBase, gEvt);
        testRealStructure(mem); testTransitionsVsGamemod(); testHandlerNamesVsAsm(); testPlayStages(mem); testEverySingleScript(mem); testParkedScripts(mem); testUsage(mem);
    }
    if (fails) { printf("flow3_test: %d FALLOS (%d comprobaciones, %d omitidas)\n", fails, checks, skipped); return 1; }
    printf("flow3_test: OK (%d comprobaciones, %d omitidas)\n", checks, skipped);
    return 0;
}

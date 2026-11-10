// Pruebas de game/flow (MainModeMgr, SysCfg/Ranking, PauseMgr, SystemTask2, CamJobQueue, EventCtrl).
// Los valores de tablas salen del ELF (baserom/SCES_504.11) y de GAMEMOD.BIN; ver game/flow/MANIFEST.md.
// Compilar: c++ -std=c++17 -Wall game/test/flow_test.cpp game/flow/*.cpp -o build/flow_test && build/flow_test
#include "../flow/mainmode.h"
#include "../flow/syscfg.h"
#include "../flow/pause.h"
#include "../flow/systask.h"
#include "../flow/camjob.h"
#include "../flow/eventctrl.h"
#include <cstdio>
#include <fstream>
#include <vector>
using namespace vn;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

// ---------------- MainModeMgr ----------------
static void testMainMode() {
    MainModeMgr m; m.init(0); CHECK(m.reqMode == 0 && m.blank == 0);
    m.reqMode = -1; m.mode = 0; m.next(); CHECK(m.reqMode == 17);         // kNextMode[0] = 17
    m.reqMode = -1; m.mode = 4; m.next(); CHECK(m.reqMode == 15);
    m.reqMode = 5; m.mode = 4; m.next(); CHECK(m.reqMode == 5);           // ya habia peticion: no cambia
    m.mode = 21; m.reqMode = -1; m.next(); CHECK(m.reqMode == 4);
    // synch: pulsacion en estado 2 (kSkipOnPress[2] == 1) -> pide modo 4 y apaga pantalla 4 fotogramas
    m = MainModeMgr(); m.mode = 3;
    ModeInput in{{true, false}, {false, false}, 2, 0};
    ModeEvents ev = m.synch(in);
    CHECK(ev.clearAbortFlag && ev.screenBitOff && ev.playPressSound && ev.modeChanged && !ev.urawazaCheck && ev.urawazaInit);
    CHECK(m.mode == 4 && m.reqMode == -1 && m.blank == 3);
    in = ModeInput{{false, false}, {false, false}, 2, 0};
    ev = m.synch(in); CHECK(m.blank == 2 && !ev.modeChanged);
    ev = m.synch(in); CHECK(m.blank == 1 && !ev.screenBitOn);
    ev = m.synch(in); CHECK(m.blank == 0 && ev.screenBitOn);
    // pulsacion en estado 0 (tabla = 0): solo limpia la bandera
    m = MainModeMgr(); in = ModeInput{{false, true}, {false, false}, 0, 0};
    ev = m.synch(in); CHECK(ev.clearAbortFlag && !ev.modeChanged && m.reqMode == -1 && m.blank == 0);
    // peticion externa gp-0x73A4
    in = ModeInput{{false, false}, {false, false}, 0, 1}; ev = m.synch(in);
    CHECK(in.gp73A4 == 0 && m.mode == 4 && ev.modeChanged);
    // modo 0xA llama a UrawazaCheck
    m = MainModeMgr(); m.reqMode = 0xA; in = ModeInput{{false, false}, {false, false}, 0, 0}; ev = m.synch(in);
    CHECK(ev.urawazaCheck && !ev.urawazaInit && m.mode == 0xA);
    // synch2
    m = MainModeMgr(); m.reqMode = 7; CHECK(m.synch2() == 7 && m.mode == 7 && m.reqMode == -1);
    // chgInit
    m.mode = 0; ChgInitPlan p = m.chgInit(9); CHECK(!p.fullReset && !p.callA10560);
    m.mode = 6; p = m.chgInit(9); CHECK(p.fullReset && p.callA10560); p = m.chgInit(10); CHECK(p.fullReset && !p.callA10560);
    CHECK(kSkipOnPress[2] == 1 && kSkipOnPress[3] == 1 && kSkipOnPress[9] == 1 && kSkipOnPress[14] == 1 && kSkipOnPress[4] == 0);
}

// ---------------- SysCfg / Ranking ----------------
static void testSysCfg() {
    SysCfgMgr s;
    s.setLang(5); CHECK(s.lang == 5 && s.langFlag43 == 0);
    s.setLang(4); CHECK(s.lang == 4 && s.langFlag43 == 1);
    s.setLang(3); CHECK(s.lang == 6 && s.langFlag43 == 0);
    s.setLang(2); CHECK(s.lang == 3);
    s.setLang(1); CHECK(s.lang == 2);
    s.setLang(0); CHECK(s.lang == 2); s.setLang(77); CHECK(s.lang == 2);
    SysCfgBlock cfg; RankTables rk;
    s.f14 = 1; s.init2(cfg, rk, 4, 0, 0);
    CHECK(cfg.initLife() == 3 && cfg.maxLife() == 5);                    // lo que asume PlayerConfig
    CHECK(cfg.raw[0] == 8 && cfg.raw[32] == 255 && cfg.raw[33] == 230 && cfg.raw[46] == 0);
    CHECK(s.lang == 4 && s.langFlag43 == 1 && s.f4 == 0 && s.f8 == 0 && s.f10 == 0 && s.f14 == 0);   // init2 los pone a 0
    CHECK(rk.a[0].score == 40000 && rk.a[9].score == 4000 && rk.a[0].name[0] == 'T' && rk.b[1].name[0] == 'O' && rk.b[9].stage == 0);
    CHECK(rk.a[2].stage == 2 && rk.a[3].stage == 1);
    // synch de tarjeta
    SysCfgMgr t; t.f14 = 1; CHECK(t.synch(1, 5) && t.f10 == 5 && t.f14 == 0);
    t.f14 = 1; CHECK(!t.synch(1, 0) && t.f14 == 0);
    t.f14 = 1; CHECK(!t.synch(0, 7) && t.f14 == 1);
    // checkRankIn
    CHECK(RankingMgr::checkRankIn(rk, 0, 3999, 5) == -1);                // < 4000 (tabla A, ultimo puesto)
    CHECK(RankingMgr::checkRankIn(rk, 0, 4000, 5) == 1);
    CHECK(RankingMgr::checkRankIn(rk, 0, 99999, 9) == -1);               // modo 9: no entra
    rk.b[9].score = 7000;
    CHECK(RankingMgr::checkRankIn(rk, 1, 6999, 5) == -1 && RankingMgr::checkRankIn(rk, 1, 7000, 5) == 1);   // gameKind 1 = tabla B
    CHECK(RankingMgr::checkRankIn(rk, 2, 4000, 5) == 1);                 // otro = tabla A
}

// ---------------- PauseMgr ----------------
struct PH : PauseHooks {
    bool remain = false; int changes = 0, lastState = -1, asserts = 0, bad = 0;
    bool dataRemaining() override { return remain; }
    void onStateChange(int s) override { changes++; lastState = s; }
    void onAssert(const char*) override { asserts++; }
    void onBadWaitEntry(int, int) override { bad++; }
};
static void testPause() {
    PauseMgr p; PH h; p.init();
    CHECK(p.get() == 0);
    p.set(2); p.synch(h); CHECK(p.state == 2 && p.get() == 2 && h.changes == 1 && p.queueCount == 0);
    p.set(2); p.synch(h); CHECK(h.changes == 1);                          // mismo estado: sin aviso
    p.set(1); p.set(3); p.synch(h); CHECK(p.state == 3 && h.changes == 2); // gana la ultima peticion
    // pila de estados
    p.pushStat(); CHECK(p.stack[0] == 3 && p.stackCount == 1);
    p.set(4); p.pushStat(); CHECK(p.stack[1] == 4);                       // con peticion pendiente apila la pedida
    p.popStat(); CHECK(p.stackCount == 1 && p.queue[p.queueCount - 1] == 4);
    p.queueCount = 0; p.stackCount = 0;
    p.pushSetStat(5); CHECK(p.stack[0] == 3 && p.queue[0] == 5);
    p.queueCount = 0;
    // pausa por carga: un tipo 0 pone loadPause=1 y get() devuelve 6 hasta que no queden datos
    PauseMgr q; q.init(); h.remain = true;
    CHECK(q.reqPauseUntilAllDataEntry(h) && q.waitCount == 1);
    q.synch(h); CHECK(q.loadPause == 1 && q.get() == 6);
    q.synch(h); CHECK(q.loadPause == 1 && q.waitCount == 1);              // sigue habiendo datos
    h.remain = false; q.synch(h); CHECK(q.loadPause == 0 && q.waitCount == 0 && q.get() == 0);
    // la entrada de modulo guarda tipo 0 y el modulo
    CHECK(q.reqPauseUntilOvlModEntry(h, 7) && q.waits[0].kind == 0 && q.waits[0].arg == 7);
    // lista llena: 8 entradas, la 9a falla con aviso
    PauseMgr r; r.init();
    for (int i = 0; i < 8; i++) CHECK(r.reqPauseUntilAllDataEntry(h));
    int a0 = h.asserts; CHECK(!r.reqPauseUntilAllDataEntry(h) && h.asserts == a0 + 1);
    // deleteWaitCauseElem compacta
    r.waits[0].kind = 5; r.waits[1].kind = 0; r.waits[2].kind = 5; r.waitCount = 3; r.waits[1].arg = 11;
    r.deleteWaitCauseElem(5); CHECK(r.waitCount == 1 && r.waits[0].kind == 0 && r.waits[0].arg == 11);
    // entrada con tipo raro mientras se espera: aviso y continua
    PauseMgr w; w.init(); w.loadPause = 1; w.waitCount = 2; w.waits[0].kind = 3; w.waits[1].kind = 0;
    h.remain = false; int b0 = h.bad; w.exec(h); CHECK(h.bad == b0 + 2 && w.loadPause == 0);   // 2 pasadas (la 1a borra el tipo 0 y repite)
    // tipo 5 en chkLoadWaitOvlMod: break + se elimina
    PauseMgr z; z.init(); z.waitCount = 1; z.waits[0].kind = 5; int s0 = h.asserts; CHECK(z.chkLoadWaitOvlMod(h) == 0 && h.asserts == s0 + 1 && z.waitCount == 0);
}

// ---------------- SystemTask2 ----------------
static std::vector<int> trace;
static void fn(void* c) { trace.push_back((int)(intptr_t)c); }
static int cmpId(const void* a, const void* b) { return *(const int*)a == *(const int*)b ? 1 : 0; }
static void testSystask() {
    ExecList l; int idA = 1, idB = 2;
    l.newExec(fn, (void*)10, 5, nullptr, cmpId);
    l.newExec(fn, (void*)11, 3, nullptr, cmpId);
    l.newExec(fn, (void*)12, 5, nullptr, cmpId);     // empate con 10: va detras
    l.newExec(fn, (void*)13, 1, nullptr, cmpId);     // antes de la cabeza
    trace.clear(); l.threadCtrl(); CHECK((trace == std::vector<int>{13, 11, 10, 12}));
    // vrID: mismo id y misma prioridad -> sustituye en el sitio
    l.newExec(fn, (void*)20, 4, &idA, cmpId); CHECK(l.count() == 5);
    l.newExec(fn, (void*)21, 4, &idA, cmpId); CHECK(l.count() == 5);
    trace.clear(); l.threadCtrl(); CHECK((trace == std::vector<int>{13, 11, 21, 10, 12}));
    // mismo id, otra prioridad -> se reubica
    l.newExec(fn, (void*)22, 9, &idA, cmpId); CHECK(l.count() == 5);
    trace.clear(); l.threadCtrl(); CHECK((trace == std::vector<int>{13, 11, 10, 12, 22}));
    l.newExec(fn, (void*)30, 0, &idB, cmpId); CHECK(l.count() == 6);
    trace.clear(); l.threadCtrl(); CHECK(trace.front() == 30);
    // nivel de pausa: solo prio >= nivel
    trace.clear(); l.threadCtrlLevelCheck(5); CHECK((trace == std::vector<int>{10, 12, 22}));
    // threadCtrl2 / LevelCheck2 continuan tras el actual
    l.current = l.head; trace.clear(); l.threadCtrl2(); CHECK(trace.size() == 5 && trace.front() == 13);
    l.current = l.head; trace.clear(); l.threadCtrlLevelCheck2(5); CHECK((trace == std::vector<int>{10, 12, 22}));
    // changeExec / deleteExec
    l.current = l.head; l.changeExec(fn, (void*)31); CHECK(l.nodes[l.head].ctx == (void*)31);
    l.current = l.nodes[l.head].next; CHECK(l.deleteExec() == DelResult::Removed && l.count() == 5 && l.current == l.head);
    l.current = l.head; CHECK(l.deleteExec() == DelResult::Removed && l.count() == 4);
    ExecList one; one.newExec(fn, nullptr, 0, nullptr, cmpId); one.current = one.head;
    CHECK(one.deleteExec() == DelResult::ListEmptyTaskClose);
    // SystemTask base
    TaskGlobals g; SystemTaskState t, u;
    t.construct(g); u.construct(g); CHECK(t.id == 0 && u.id == 1 && g.taskCount == 2 && (t.flags & TF_LINKED) && !(t.flags & TF_STATIC) && (u.flags & TF_STATIC));
    CHECK(t.destruct(g) && g.taskCount == 1 && (t.flags & TF_DEAD) && t.id == 0);
    CHECK(!t.destruct(g) && g.taskCount == 1);                              // doble destruccion: no hace nada
}

// ---------------- CamJobQueue ----------------
static struct CH : CamHost { std::vector<std::pair<int,int>> calls; void runHandler(int h, int s) override { calls.push_back({h, s}); } } camHost;
static uint32_t f2u(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static void testCam() {
    // registros en una memoria sintetica: [op=16, camara, tipo, parametros]
    std::vector<uint32_t> w = {
        16, 0, 0x11, f2u(2.0f),            // @0x1000: FrameSpd camara 0
        16, 0, 0x12, 60,                   // @0x1010: SplCnt = 60*50/60 = 50
        16, 1, 0x13, 3,                    // @0x1020: CafsIdx camara 1 = 3
        16, 0, 0x40, 1, 0, 0, 0,           // @0x1030: Trace camara 0: 4 parametros
        16, 0, 0x12, 1,                    // @0x104C: SplCnt = 1
        16, 2, 0x30, 0, 0, 0,              // @0x105C: FinishPos camara 2: 3 parametros
    };
    VecMem mem(0x1000, w);
    CamJobQueue q; q.mem = &mem; q.flushCamJob();
    CHECK(q.chkRemainCamJob() == false && q.cam[0].flagD5 == 1 && q.slot[3].sub[3] == -2);
    CHECK(CamJobQueue::recLen(mem, 0x1000) == 4 && CamJobQueue::recLen(mem, 0x1030) == 7 && CamJobQueue::recLen(mem, 0x105C) == 6);
    uint32_t n = q.entryEvent(0x1000, 0); CHECK(n == 0x1010 && q.tail == 1 && q.chkRemainCamJob());
    n = q.entryEvent(0x1010, 0); CHECK(n == 0x1020 && q.tail == 2);
    n = q.entryEvent(0x1020, 1); CHECK(n == 0x1030 && q.tail == 2);        // skip: no encola pero avanza
    n = q.entryEvent(0x1020, 0); CHECK(n == 0x1030 && q.tail == 3);
    // camara 0 toma FrameSpd; al acabar toma SplCnt; camara 1 toma CafsIdx
    q.getJob(0); CHECK(q.slot[0].job == CJ_FRAMESPD && q.slot[0].handler == CH_FRAMESPD && q.slot[0].ring == 0 && q.ringState[0] == 2);
    q.getJob(0); CHECK(q.slot[0].ring == 0);                               // ya tiene trabajo: no cambia
    q.getJob(1); CHECK(q.slot[1].job == CJ_CAFSIDX && q.slot[1].ring == 2);
    q.runSlot(1, camHost); CHECK(q.slot[1].cafsIdx == 3 && q.slot[1].job == CJ_NONE && q.ringState[2] == 0);
    q.runSlot(0, camHost);                                                  // FrameSpd -> finishJob -> getJob toma SplCnt y lo ejecuta
    CHECK(q.cam[0].speed == 1.2f * 2.0f);
    CHECK(q.cam[0].splCnt == 50 && q.slot[0].job == CJ_NONE);
    CHECK(q.head == q.tail && !q.chkRemainCamJob());                        // todo consumido
    // el cabezal salta entradas ya terminadas
    for (int i = 0; i < 8; i++) q.ringState[i] = 0;
    q.head = 6; q.tail = 2;                                                 // cola envuelta: 6,7,0,1
    q.ringRec[7] = 0x1030; q.ringState[7] = 1; q.ringRec[0] = 0x105C; q.ringState[0] = 1;
    q.getJob(2); CHECK(q.slot[2].job == CJ_FINISHPOS && q.slot[2].ring == 0);
    q.getJob(0); CHECK(q.slot[0].job == CJ_TRACE && q.slot[0].ring == 7);
    q.runSlot(0, camHost); CHECK(camHost.calls.size() == 1 && camHost.calls[0].first == CH_TRACE && camHost.calls[0].second == 0);
    q.finishJob(0, camHost); CHECK(q.head == 0 && q.ringState[7] == 0);   // salta 6 y 7 (libres) y para en 0 (en ejecucion)
    q.finishJob(2, camHost); CHECK(q.head == q.tail);
    // SplCnt == 1 se queda en 1; chgToDirMode
    q.ringRec[3] = 0x104C; q.ringState[3] = 1; q.head = 3; q.tail = 4; q.slot[0].job = CJ_NONE; q.getJob(0);
    q.runSlot(0, camHost); CHECK(q.cam[0].splCnt == 1);
    VecMem m2(0x2000, {17, 2}); q.mem = &m2; CHECK(q.chgToDirMode(0x2000) == 0x2008 && q.cam[2].mode == 1);
    // curvas
    float keys[8] = {5.f, 0, 0, 0, 100.4f, 0, 0, 0}; CurveKeys c{keys, 2};
    CHECK(CamPathMgr::getStartFrame(c) == 5 && CamPathMgr::getEndFrame(c) == 100);
    keys[4] = 100.5f; CHECK(CamPathMgr::getEndFrame(c) == 101);
}

// ---------------- EventCtrl ----------------
static uint32_t opc(EvOp o) { for (uint32_t i = 0; i < 115; i++) if (evOpFromOpcode(i) == o) return i; printf("FALLA opcode sin resolver en la tabla: %s\n", evOpName(o)); fails++; return 9999; }
struct EH : EvHost {
    int life[2] = {0, 0}, mode[2] = {0, 0}; bool demo = false; int pause = 0; bool remain = false; int cmds = 0; EvOp lastCmd = EvOp::Unknown; int unknown = 0;
    int clears = 0, bonus = 0, reqAll = 0; bool lastSkip = false; int entryCalls = 0; int ovlStat = 2, ovlReq = 0;
    uint32_t enemyEntry(uint32_t pc, bool skipOnly) override { lastSkip = skipOnly; entryCalls++; return pc + 8; }
    int ovlEntryStat(int) override { return ovlStat; }
    void reqPauseOvlMod(int) override { ovlReq++; }
    int playerMode(int p) override { return mode[p]; }
    int playerLife(int p) override { return life[p]; }
    bool demoPlay() override { return demo; }
    int pauseLevel() override { return pause; }
    bool dataRemaining() override { return remain; }
    void reqPauseAllData() override { reqAll++; }
    void cmd(EvOp o, const uint32_t*, int) override { cmds++; lastCmd = o; }
    void stageClear() override { clears++; }
    void addHitPerBonus() override { bonus++; }
    void onUnknownOpcode(uint32_t, uint32_t) override { unknown++; }
};
static void testEventCtrl() {
    // mapeo de opcodes (tools/afs_evtpal.py)
    CHECK(evOpFromOpcode(16) == EvOp::CCTB && evOpFromOpcode(22) == EvOp::WAIT_FRM && evOpFromOpcode(28) == EvOp::WAIT && evOpFromOpcode(6) == EvOp::Unknown && evOpFromOpcode(500) == EvOp::Unknown);
    CHECK(opc(EvOp::NEXTEV) == 0x37 && opc(EvOp::SCNINIT) == 0x36 && opc(EvOp::ENTRYCOLLI) == 0x56 && opc(EvOp::ENTRYBANK) == 0x42);   // 0x37/0x36/0x56/0x42: LEVELS.md
    // Memoria sintetica: EVTPAL en 0x1FC2000: [0..11] punteros por etapa; etapa 0 -> lista de eventos.
    const uint32_t B = 0x1FC2000;
    std::vector<uint32_t> w(256, 0xFFFFFFFFu);
    auto A = [&](int idx) { return B + 4u * idx; };
    for (int s = 0; s < 12; s++) w[s] = A(0x20);                 // todas las etapas -> tabla de eventos en palabra 0x20
    w[8] = A(0x30);                                              // EVTPAL[8] -> [A(0x31)]  (EventStageClear)
    // eventos de la etapa 0: ev0 -> fases en 0x40, ev1 -> 0x44, ev2 -> 0x48
    w[0x20] = A(0x40); w[0x21] = A(0x44); w[0x22] = A(0x48); w[0x23] = 0xFFFFFFFFu;
    w[0x40] = A(0x80); w[0x41] = A(0x90); w[0x42] = 0xFFFFFFFFu;     // ev0: fase0 @0x80, fase1 @0x90
    w[0x44] = A(0xA0); w[0x45] = A(0xA8); w[0x46] = 0xFFFFFFFFu;     // ev1
    w[0x48] = A(0xB0); w[0x49] = 0xFFFFFFFFu;                        // ev2
    w[0x30] = A(0x31); w[0x31] = A(0xC0);                            // EVTPAL[8][0][0] = 0xC0; ver nextEv tipo 3
    w[8] = A(0x30);
    // script ev0 fase0: BRANCHNO 2 ; WAIT_ITEM 5 ; NEXTEV   (ITEMSET no tiene opcode resuelto: se prueba con exec())
    uint32_t i = 0x80;
    w[i++] = opc(EvOp::BRANCHNO); w[i++] = 2;
    w[i++] = opc(EvOp::WAIT_ITEM); w[i++] = 5;
    w[i++] = opc(EvOp::NEXTEV);
    // ev0 fase1 @0x90: GAMEFLAGSET 1 6 ; BRANCHEV [0->phase? ...]
    i = 0x90;
    w[i++] = opc(EvOp::GAMEFLAGSET); w[i++] = 1; w[i++] = 6;
    w[i++] = opc(EvOp::NEXTEV);
    // ev1 fase0 @0xA0: WAIT_FRM 120 ; NEXTEV
    i = 0xA0; w[i++] = opc(EvOp::WAIT_FRM); w[i++] = 120; w[i++] = opc(EvOp::NEXTEV);
    // ev1 fase1 @0xA8: SKIP 1 ; NEXTEV
    i = 0xA8; w[i++] = opc(EvOp::SKIP); w[i++] = 1; w[i++] = opc(EvOp::NEXTEV);
    // ev2 fase0 @0xB0: ENTRYBANK 9 ; NEXTEV
    i = 0xB0; w[i++] = opc(EvOp::ENTRYBANK); w[i++] = 9; w[i++] = opc(EvOp::NEXTEV);
    VecMem mem(B, w);
    EvState g; g.stage = 0; g.event = 0; g.gameState = 10;
    EH h; CamJobQueue cq; cq.mem = &mem; cq.flushCamJob();
    EventCtrl e(g, mem, h, &cq);
    CHECK(e.lookupPc(0, 0, 0) == A(0x80) && e.lookupPc(0, 1, 1) == A(0xA8) && e.lookupPc(0, 0, 2) == 0xFFFFFFFFu);
    e.construct();
    CHECK(e.pc == A(0x80) && g.phase == 0 && g.gp733C == 0x2254 && g.savedPc == A(0x80));
    g.event = 1; g.stage = 6; e.construct(); CHECK(g.phase == 1 && g.gp733C == 0x1DC0 && e.pc == A(0xA8));
    g.gameState = 9; e.construct(); CHECK(g.phase == 0);
    g.stage = 0; g.event = 0; g.gameState = 10; e.construct();
    // sin jugadores ni demo no avanza (canAdvance = 0): ITEMSET y BRANCHNO se ejecutan, WAIT_ITEM cede
    e.frame();
    CHECK(g.branchNo == 2 && e.wait == 1 && g.savedPc == A(0x80 + 2) && g.canAdvance == 0);
    // jugador 1 vivo en modo 5 -> puede avanzar; pero WAIT_ITEM espera a que el objeto 5 sea != 0
    h.life[0] = 1; h.mode[0] = 5;
    e.frame(); CHECK(g.canAdvance == 1 && e.wait == 1 && g.savedPc == A(0x82));
    e.pc = g.savedPc; e.op = mem.rd(e.pc); g.items[5] = 7;
    e.frame();
    CHECK(cq.cam[0].flagD6 == 1);
    // el bucle sigue: NEXTEV -> fase 1 (GAMEFLAGSET 1 6) -> NEXTEV: fin de fases -> tabla etapa 0: ev0 = tipo 0 -> evento 1 fase 1 (SKIP 1, NEXTEV)
    // -> ev1 = tipo 2 -> evento 2 fase 1, que no existe (puntero 0xFFFFFFFF): opcode invalido y cede el fotograma
    CHECK((g.gameFlags & 6) == 6 && g.event == 2 && g.phase == 1 && g.gp729C == 1 && h.unknown == 1 && e.wait == 1);
    // rutinas con opcode sin resolver, probadas directamente
    {   std::vector<uint32_t> t2(8, 0); t2[0] = 0; t2[1] = 9; t2[2] = 0; VecMem m2(0x5000, t2);
        EvState g2; EH h2; EventCtrl x(g2, m2, h2, nullptr); x.pc = 0x5000; t2[1] = 9; t2[2] = 4;
        VecMem m3(0x5000, {0, 9, 4, 0, 1, 33, 2, 0}); EventCtrl y(g2, m3, h2, nullptr);
        y.pc = 0x5000; y.exec(EvOp::ITEMSET); CHECK(g2.items[9] == 4 && y.pc == 0x500C);
        y.pc = 0x5000; y.exec(EvOp::BSFT); CHECK(g2.bsft == 9 && y.pc == 0x5008);
        y.pc = 0x5000; y.exec(EvOp::WAIT_INT_MODE); CHECK(y.intMode == 9 && y.pc == 0x5008);
        y.pc = 0x5000; y.exec(EvOp::NOWLOADINGSET); CHECK(g2.nowLoading == 9);
        y.pc = 0x5000; y.exec(EvOp::SNOWDISP); CHECK(g2.snowDisp == 9);
        y.pc = 0x5000; y.exec(EvOp::GETCPMODE); CHECK(g2.gp730C == 9);
        y.pc = 0x500C; y.exec(EvOp::GAMEFLAGSET); CHECK(g2.gameFlags == 33 && y.pc == 0x5018); g2.gameFlags = 0xFF; y.pc = 0x5000; VecMem m5(0x5000, {0, 0, 4}); EventCtrl y5(g2, m5, h2, nullptr); y5.pc = 0x5000; y5.exec(EvOp::GAMEFLAGSET); CHECK(g2.gameFlags == 0xFB);
        y.pc = 0x5000; y.exec(EvOp::BGTB); CHECK(y.bg.cur == 9 && y.bg.prev == 0 && y.pc == 0x5008); y.pc = 0x5000; y.exec(EvOp::BGTB); CHECK(y.bg.prev == 9);
        y.bg.appointBGImm(3); CHECK(y.bg.cur == 3 && y.bg.prev == 9);
        y.pc = 0x5000; y.exec(EvOp::WAIT); CHECK(y.wait == 1 && y.pc == 0x5000);
        y.wait = 0; y.pc = 0x5000; y.exec(EvOp::JOBFLAGSET); CHECK(y.wait == 9 && y.pc == 0x5008); }
    // pausa >= 2: no ejecuta
    uint32_t pcBefore = g.savedPc; h.pause = 2; e.frame(); CHECK(g.savedPc == pcBefore); h.pause = 0;
    // demo permite avanzar sin jugadores
    h.life[0] = 0; h.demo = true; e.frame(); CHECK(g.canAdvance == 1);
    h.demo = false;
    // jugador 2 cuenta
    h.life[1] = 3; h.mode[1] = 5; EventCtrl e2(g, mem, h, &cq); e2.frame(); CHECK(g.canAdvance == 1);
    h.mode[1] = 4; e2.frame(); CHECK(g.canAdvance == 0);
}
static void testNextEv() {
    const uint32_t B = 0x1FC2000;
    std::vector<uint32_t> w(256, 0xFFFFFFFFu);
    auto A = [&](int idx) { return B + 4u * idx; };
    for (int s = 0; s < 12; s++) w[s] = A(0x20);
    w[8] = A(0x30); w[0x30] = A(0x31); w[0x31] = A(0xC0); w[0xC0] = 0x1234;
    for (int ev = 0; ev < 6; ev++) { w[0x20 + ev] = A(0x40 + ev * 4); w[0x40 + ev * 4] = A(0x80 + ev); w[0x41 + ev * 4] = A(0x90 + ev); w[0x42 + ev * 4] = 0xFFFFFFFFu; }
    VecMem mem(B, w); EH h; EvState g; EventCtrl e(g, mem, h, nullptr);
    // etapa 4 (tabla 0x9B82E0: evento k -> k+1): fin de fase en evento 1 -> evento 2, fase 1
    g.stage = 4; g.event = 1; g.phase = 1; g.branchNo = 3; e.nextEv();
    CHECK(g.event == 2 && g.phase == 1 && e.pc == A(0x90 + 2) && g.branchNo == 0);
    // mientras queden fases: solo avanza la fase
    g.stage = 4; g.event = 1; g.phase = 0; e.nextEv(); CHECK(g.event == 1 && g.phase == 1 && e.pc == A(0x90 + 1));
    // etapa 1 (0x9B8030): fila 0 = {1,2,2,-1}: tipo 1 elige a/b/c segun branchNo; ev0 + branchNo 1 -> 2
    g.stage = 1; g.event = 0; g.phase = 1; g.branchNo = 1; e.nextEv(); CHECK(g.event == 2 && g.phase == 1);
    // fila 5 de la etapa 1 = {1,6,7,-1}: branchNo 1 -> 7; branchNo 2 -> -1 (c)
    g.stage = 1; g.event = 5; g.phase = 1; g.branchNo = 1; e.nextEv(); CHECK(g.event == 4 + 3);   // 7
    // tipo 2 (fila 9 de etapa 1 = {2,0,..}): evento + 1
    g.stage = 1; g.event = 9; g.phase = 1; g.branchNo = 0; e.nextEv(); CHECK(g.event == 4 + 6);   // 10 -> fuera de la tabla simulada, pero event se actualiza
    // tipo 3: etapa 5 evento 3 = {3,0}: fase superada
    g.stage = 5; g.event = 3; g.phase = 1; e.nextEv();
    CHECK(h.clears == 1 && h.bonus == 1 && g.phase == 0 && e.pc == A(0xC0) && g.event == 0);
    g.stage = 7; g.event = 1; g.phase = 1; e.nextEv(); CHECK(h.clears == 2 && h.bonus == 1);          // etapa 7: no hay bono
    // etapa 5 evento 2: {0,3}: pasa al evento 3 -> se limpia el bit 2 de gameFlags
    g.gameFlags = 7; g.stage = 5; g.event = 2; g.phase = 1; e.nextEv(); CHECK(g.event == 3 && (g.gameFlags & 2) == 0 && (g.gameFlags & 5) == 5);
    // b500F05 != 0 conserva branchNo
    g.b500F05 = 1; g.branchNo = 4; g.stage = 4; g.event = 0; g.phase = 1; e.nextEv(); CHECK(g.branchNo == 4);
    // tablas de datos (GAMEMOD.BIN)
    CHECK(kEvTransTables[0].rows == kEvTransTables[6].rows && kEvTransTables[1].count == 48 && kEvTransTables[8].rows == nullptr);
    CHECK(kEvTransTables[1].rows[5].type == 1 && kEvTransTables[1].rows[5].a == 6 && kEvTransTables[1].rows[5].b == 7);
}
static void testBranches() {
    const uint32_t B = 0x1FC2000;
    std::vector<uint32_t> w(512, 0xFFFFFFFFu);
    auto A = [&](int idx) { return B + 4u * idx; };
    for (int s = 0; s < 12; s++) w[s] = A(0x20);
    w[0x20] = A(0x40); w[0x40] = A(0x100); w[0x41] = A(0x110); w[0x42] = A(0x120); w[0x43] = 0xFFFFFFFFu;
    // BRANCHEV [op, 2, 1, -1]: elige la fase numero `branchNo`-esima de la lista
    uint32_t i = 0x100; w[i++] = opc(EvOp::BRANCHEV); w[i++] = 2; w[i++] = 1; w[i++] = 0xFFFFFFFFu;
    // BRANCH 0x110-destino
    i = 0x110; w[i++] = opc(EvOp::BRANCH); w[i++] = A(0x130);
    // SETEVNO 2
    i = 0x120; w[i++] = opc(EvOp::SETEVNO); w[i++] = 2;
    // SWITCH_MENM [def=A(0x150), A(0x140), A(0x144), -1]
    i = 0x130; w[i++] = opc(EvOp::SWITCH_MENM); w[i++] = A(0x150); w[i++] = A(0x140); w[i++] = A(0x144); w[i++] = 0xFFFFFFFFu;
    // BRANCHITEM obj 3: [op, 3, d0, d1, -1]
    i = 0x160; w[i++] = opc(EvOp::BRANCHITEM); w[i++] = 3; w[i++] = A(0x170); w[i++] = A(0x174); w[i++] = 0xFFFFFFFFu;
    // BRANCHADR [op, d0, d1, d2, -1]
    i = 0x180; w[i++] = opc(EvOp::BRANCHADR); w[i++] = A(0x190); w[i++] = A(0x194); w[i++] = A(0x198); w[i++] = 0xFFFFFFFFu;
    // WLCTB + ZMAT + listas
    i = 0x1A0; w[i++] = opc(EvOp::WLCTB); w[i++] = 11; w[i++] = 22; w[i++] = 0xFFFFFFFFu; w[i++] = opc(EvOp::NEXTEV);
    i = 0x1B0; w[i++] = opc(EvOp::ZMAT); w[i++] = 1; w[i++] = 2; w[i++] = 0xFFFFFFFFu; w[i++] = opc(EvOp::NEXTEV);
    i = 0x1C0; w[i++] = opc(EvOp::ENTRYFANI); w[i++] = 4; w[i++] = 5; w[i++] = 6; w[i++] = 0xFFFFFFFFu; w[i++] = opc(EvOp::NEXTEV);
    i = 0x1D0; w[i++] = opc(EvOp::MISSENMCLR); w[i++] = opc(EvOp::SKIP); w[i++] = 1; w[i++] = opc(EvOp::VIBRATION); w[i++] = 2; w[i++] = 1; w[i++] = 2; w[i++] = 3; w[i++] = opc(EvOp::NEXTEV);
    i = 0x1E0; w[i++] = opc(EvOp::FORCE_CAMPARAM); w[i++] = 1; w[i++] = 0; w[i++] = 30; w[i++] = opc(EvOp::FORCE_CAMPARAM); w[i++] = 1; w[i++] = 1; w[i++] = f2u(0.5f); w[i++] = opc(EvOp::NEXTEV);
    VecMem mem(B, w); EH h; EvState g; CamJobQueue cq; cq.mem = &mem; cq.flushCamJob();
    EventCtrl e(g, mem, h, &cq);
    // BRANCHEV: branchNo 0 -> fase 2 ; branchNo 1 -> fase 1
    g.stage = 0; g.event = 0; g.branchNo = 0; e.pc = A(0x100); e.op = w[0x100]; e.step(); CHECK(g.phase == 2 && e.pc == A(0x120) && g.branchNo == 0);
    g.branchNo = 1; e.pc = A(0x100); e.op = w[0x100]; e.step(); CHECK(g.phase == 1 && e.pc == A(0x110));
    // BRANCH
    e.op = w[0x110]; e.step(); CHECK(e.pc == A(0x130));
    // SWITCH_MENM
    g.menmCount = 1; e.op = w[0x130]; e.step(); CHECK(e.pc == A(0x144));
    g.menmCount = 0; e.pc = A(0x130); e.step(); CHECK(e.pc == A(0x140));
    g.menmCount = 9; e.pc = A(0x130); e.step(); CHECK(e.pc == A(0x150));          // fuera de lista -> defecto
    // BRANCHITEM
    g.items[3] = 1; e.pc = A(0x160); e.op = w[0x160]; e.step(); CHECK(e.pc == A(0x174));
    g.items[3] = 0; e.pc = A(0x160); e.step(); CHECK(e.pc == A(0x170));
    g.items[3] = 5; e.pc = A(0x160); e.step(); CHECK(e.pc == 0);                  // sin rama: pc nulo (el original tambien)
    // BRANCHADR
    g.branchNo = 2; e.pc = A(0x180); e.op = w[0x180]; e.step(); CHECK(e.pc == A(0x198));
    // WLCTB
    e.pc = A(0x1A0); e.op = w[0x1A0]; e.step(); CHECK(g.wlctbTable == A(0x1A1) && e.pc == A(0x1A4));
    // ZMAT
    e.pc = A(0x1B0); e.op = w[0x1B0]; e.step(); CHECK(g.zmatCount == 2 && g.zmat[0] == 1 && g.zmat[1] == 2 && e.pc == A(0x1B4));
    // lista de ENTRYFANI: longitud n + 2
    e.pc = A(0x1C0); e.op = w[0x1C0]; h.cmds = 0; e.step(); CHECK(e.pc == A(0x1C5) && h.cmds == 1 && h.lastCmd == EvOp::ENTRYFANI);
    // MISSENMCLR, SKIP, VIBRATION (sin skip suena; con skip no)
    g.menmCount = 5; e.pc = A(0x1D0); e.op = w[0x1D0]; e.step(); CHECK(g.menmCount == 0 && e.pc == A(0x1D1));
    g.skipReq = 1; g.gp7294 = 5; e.op = w[e.pc / 4 * 0 + 0x1D1]; e.step(); CHECK(g.gp7294 == 0 && g.gp729C == 1 && e.pc == A(0x1D3));
    h.cmds = 0; e.op = w[0x1D3]; g.skipReq = 1; e.step(); CHECK(h.cmds == 0 && e.pc == A(0x1D8));
    e.pc = A(0x1D3); g.skipReq = 0; e.step(); CHECK(h.cmds == 1 && e.pc == A(0x1D8));
    // SKIP 0 limpia skipReq
    e.pc = A(0x1D1); w[0x1D2] = 0; VecMem mem2(B, w); EventCtrl e3(g, mem2, h, &cq); e3.pc = A(0x1D1); e3.op = w[0x1D1]; g.skipReq = 1; e3.step(); CHECK(g.skipReq == 0 && g.gp729C == 0);
    // FORCE_CAMPARAM: clase 0 = fotograma (int->float), clase 1 = velocidad (float)
    e.pc = A(0x1E0); e.op = w[0x1E0]; e.step(); CHECK(cq.cam[1].frame == 30.0f && e.pc == A(0x1E4));
    e.op = w[0x1E4]; e.step(); CHECK(cq.cam[1].speed == 0.5f && e.pc == A(0x1E8));
}
static void testWaits() {
    const uint32_t B = 0x1FC2000;
    std::vector<uint32_t> w(256, 0xFFFFFFFFu);
    auto A = [&](int idx) { return B + 4u * idx; };
    uint32_t i = 0x80;
    w[i++] = opc(EvOp::WAIT_FRM); w[i++] = 120;               // 120 * 50 / 60 = 100 cuentas
    w[i++] = opc(EvOp::WAIT_ENNUM); w[i++] = 2;
    w[i++] = opc(EvOp::WAIT_CAMFRM); w[i++] = 1; w[i++] = 10;
    w[i++] = opc(EvOp::WAIT_FLUSHDATA); w[i++] = opc(EvOp::WAIT_FLUSHDATA_NOPAUSE);
    w[i++] = 0 /* WAIT_BUFFERRAM: opcode sin resolver */; w[i++] = opc(EvOp::WAIT_CAMJOB); w[i++] = opc(EvOp::WAIT_LOAD_OVLMOD); w[i++] = 4;
    VecMem mem(B, w); EH h; EvState g; CamJobQueue cq; cq.mem = &mem; cq.flushCamJob();
    EventCtrl e(g, mem, h, &cq);
    g.canAdvance = 1; e.pc = A(0x80); e.op = w[0x80];
    e.step(); CHECK(e.wait == 1 && e.frameCnt == 100 && e.pc == A(0x80));
    int steps = 0; while (e.wait == 1 && steps < 300) { e.step(); steps++; }
    CHECK(steps == 101 && e.wait == 0 && e.pc == A(0x82));                    // 100 -> 0 y luego -1: sale al paso 101
    // skip: sale de inmediato
    e.pc = A(0x80); g.skipReq = 1; e.step(); CHECK(e.wait == 0 && e.pc == A(0x82)); g.skipReq = 0;
    // WAIT_ENNUM: espera hasta enemyCount <= objetivo (2)
    e.op = w[0x82]; g.enemyCount = 5; e.step(); CHECK(e.wait == 1 && g.enemyTarget == 2 && e.pc == A(0x82));
    e.step(); CHECK(e.wait == 1);                                              // 5 > 2
    g.enemyCount = 2; e.step(); CHECK(e.wait == 0 && e.pc == A(0x84) && g.enemyTarget == 0 && cq.cam[0].flagD6 == 1);
    // WAIT_ENNUM no avanza si la camara no esta lista y no hay intMode
    g.enemyCount = 0; cq.cam[0].flagD5 = 0; e.pc = A(0x82); e.wait = 0; e.step(); e.step(); CHECK(e.wait == 1);
    e.intMode = 1; e.step(); CHECK(e.wait == 0);
    // WAIT_CAMFRM: camara 1, fotograma 10; avanza cuando frame >= 9
    e.pc = A(0x84); e.op = w[0x84]; e.wait = 0; cq.cam[1].frame = 3; e.step(); CHECK(e.wait == 1);
    e.step(); CHECK(e.wait == 1 && e.pc == A(0x84));
    cq.cam[1].frame = 9; e.step(); CHECK(e.wait == 0 && e.pc == A(0x87));
    // fotograma 0 = hasta el final de la ruta (endFrame)
    w[0x86] = 0; VecMem mem2(B, w); EventCtrl e2(g, mem2, h, &cq); e2.pc = A(0x84); e2.op = w[0x84]; cq.cam[1].endFrame = 50; cq.cam[1].frame = 48;
    e2.step(); e2.step(); CHECK(e2.wait == 1); cq.cam[1].frame = 49; e2.step(); CHECK(e2.wait == 0);
    // WAIT_FLUSHDATA: con datos pendientes pide pausa y corta el bucle, pero avanza
    e.pc = A(0x87); e.op = w[0x87]; h.remain = true; e.stopFlag = 0; e.step(); CHECK(h.reqAll == 1 && e.stopFlag == 1 && e.pc == A(0x88));
    e.op = w[0x88]; e.stopFlag = 0; e.step(); CHECK(e.stopFlag == 1 && e.pc == A(0x88));    // NOPAUSE: no avanza
    h.remain = false; e.step(); CHECK(e.pc == A(0x89));
    // WAIT_BUFFERRAM
    e.pc = A(0x89); h.remain = true; e.wait = 0; e.exec(EvOp::WAIT_BUFFERRAM); CHECK(e.wait == 1); e.exec(EvOp::WAIT_BUFFERRAM); CHECK(e.wait == 1 && e.pc == A(0x89)); h.remain = false; e.exec(EvOp::WAIT_BUFFERRAM); CHECK(e.wait == 0 && e.pc == A(0x8A));   // opcode sin resolver: exec directo
    // WAIT_CAMJOB: con trabajos pendientes cede
    cq.tail = 1; e.pc = A(0x8A); e.op = w[0x8A]; e.step(); CHECK(e.wait == 1 && e.pc == A(0x8A)); cq.tail = cq.head; e.step(); CHECK(e.wait == 0 && e.pc == A(0x8B));
    // WAIT_ENALV / WAIT_PEOPLE: misma estructura que WAIT_ENNUM con sus propios contadores
    { VecMem m4(0x6000, {0, 2, 0, 0}); EvState g4; EH h4; EventCtrl z(g4, m4, h4, nullptr); g4.canAdvance = 1; z.intMode = 1;
      z.pc = 0x6000; g4.alvCount = 3; z.exec(EvOp::WAIT_ENALV); CHECK(z.wait == 1 && g4.alvTarget == 2); z.exec(EvOp::WAIT_ENALV); CHECK(z.wait == 1 && z.pc == 0x6000);
      g4.alvCount = 2; z.exec(EvOp::WAIT_ENALV); CHECK(z.wait == 0 && z.pc == 0x6008 && g4.alvTarget == 0);
      z.pc = 0x6000; g4.peopleCount = 9; z.exec(EvOp::WAIT_PEOPLE); z.exec(EvOp::WAIT_PEOPLE); CHECK(z.wait == 1);
      g4.peopleCount = 0; z.exec(EvOp::WAIT_PEOPLE); CHECK(z.wait == 0 && z.pc == 0x6008 && g4.peopleTarget == 0);
      // ET1P / ET2P / ENTBSK: solo cambian si se ejecuta la entrada o solo se salta el registro
      g4.credits1P2P = 1; z.pc = 0x6000; z.exec(EvOp::ET1P); CHECK(!h4.lastSkip && z.pc == 0x6008);
      g4.credits1P2P = 2; z.pc = 0x6000; z.exec(EvOp::ET1P); CHECK(h4.lastSkip);
      z.pc = 0x6000; z.exec(EvOp::ET2P); CHECK(!h4.lastSkip);
      g4.credits1P2P = 1; z.pc = 0x6000; z.exec(EvOp::ET2P); CHECK(h4.lastSkip);
      g4.skipReq = 0; z.pc = 0x6000; z.exec(EvOp::ENTBSK); CHECK(!h4.lastSkip); g4.skipReq = 1; z.exec(EvOp::ENTBSK); CHECK(h4.lastSkip);
      z.pc = 0x6000; z.exec(EvOp::ENTB); CHECK(!h4.lastSkip && h4.entryCalls == 7);
      // WAIT_LOAD_OVLMOD: pide pausa solo si el modulo no esta cargado (estado != 2)
      z.pc = 0x6000; h4.ovlStat = 2; z.exec(EvOp::WAIT_LOAD_OVLMOD); CHECK(h4.ovlReq == 0 && z.pc == 0x6008);
      z.pc = 0x6000; h4.ovlStat = 1; z.exec(EvOp::WAIT_LOAD_OVLMOD); CHECK(h4.ovlReq == 1);
      // SNDONSK/SNDONMSK: con skipReq no suenan pero avanzan (5 y 7 palabras)
      g4.skipReq = 1; h4.cmds = 0; z.pc = 0x6000; z.exec(EvOp::SNDONSK); CHECK(h4.cmds == 0 && z.pc == 0x6014);
      z.pc = 0x6000; z.exec(EvOp::SNDONMSK); CHECK(h4.cmds == 0 && z.pc == 0x601C);
      g4.skipReq = 0; z.pc = 0x6000; z.exec(EvOp::SNDONSK); CHECK(h4.cmds == 1 && z.pc == 0x6014); }
    // opcode desconocido: se avisa y se cede
    e.op = 6; e.step(); CHECK(h.unknown == 1 && e.wait == 1);
}

static void sweepRealEvtpal() {
    std::ifstream f("disc/TBL/EVTPAL.BIN", std::ios::binary);
    if (!f) { printf("(info) disc/TBL/EVTPAL.BIN no disponible: se omite el barrido\n"); return; }
    std::vector<char> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<uint32_t> w(raw.size() / 4); memcpy(w.data(), raw.data(), w.size() * 4);
    VecMem mem(0x1FC2000, w);
    int scripts = 0, reachedNext = 0, stoppedUnknown = 0, stoppedEnemy = 0, stoppedOther = 0; long totalSteps = 0;
    struct SH : EH { bool enemy = false; uint32_t enemyEntry(uint32_t pc, bool) override { enemy = true; return pc; } uint32_t enemyEntry2(uint32_t pc) override { enemy = true; return pc; } };
    for (int s = 0; s < 12; s++) {
        uint32_t l1 = mem.rd(0x1FC2000 + 4 * s); if (s == 10 || s == 11) continue;
        for (int ev = 0; mem.rd(l1 + 4 * ev) >= 0x1FC2000 && mem.rd(l1 + 4 * ev) < 0x1FC2000 + w.size() * 4; ev++) {
            uint32_t l2 = mem.rd(l1 + 4 * ev);
            for (int ph = 0; mem.rd(l2 + 4 * ph) >= 0x1FC2000 && mem.rd(l2 + 4 * ph) < 0x1FC2000 + w.size() * 4; ph++) {
                SH h; EvState g; g.canAdvance = 1; g.skipReq = 1;
                EventCtrl e(g, mem, h, nullptr); e.pc = mem.rd(l2 + 4 * ph); scripts++;
                int n = 0; bool done = false;
                while (n < 5000 && !done) {
                    e.op = mem.rd(e.pc); EvOp o = evOpFromOpcode(e.op);
                    if (o == EvOp::NEXTEV) { reachedNext++; done = true; break; }
                    if (o == EvOp::Unknown) { stoppedUnknown++; done = true; break; }
                    if (o == EvOp::ENTB || o == EvOp::ENTB2 || o == EvOp::ET1P || o == EvOp::ET2P || o == EvOp::ENTBSK || o == EvOp::FOGCTRL || o == EvOp::LIGHTCTRL || o == EvOp::DOFCTRL || o == EvOp::BLURCTRL) { stoppedEnemy++; done = true; break; }
                    if (o == EvOp::WAIT || o == EvOp::BRANCH || o == EvOp::BRANCHEV || o == EvOp::SETEVNO || o == EvOp::SWITCH_MENM || o == EvOp::BRANCHITEM || o == EvOp::BRANCHADR) { stoppedOther++; done = true; break; }
                    e.intMode = 1; e.step(); if (e.wait) { e.step(); e.wait = 0; } e.stopFlag = 0; n++;
                }
                totalSteps += n;
            }
        }
    }
    printf("(info) barrido de EVTPAL.BIN real: %d scripts, %d llegan a NEXTEV, %d paran en opcode sin resolver, %d en enemigos/luces/niebla (longitud la decide el motor), %d en saltos; %ld instrucciones\n",
           scripts, reachedNext, stoppedUnknown, stoppedEnemy, stoppedOther, totalSteps);
}

int main() {
    testMainMode(); testSysCfg(); testPause(); testSystask(); testCam(); testEventCtrl(); testNextEv(); testBranches(); testWaits();
    sweepRealEvtpal();
    if (fails == 0) printf("flow_test: OK\n"); else printf("flow_test: %d fallos\n", fails);
    return fails != 0;
}

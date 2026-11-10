// Pruebas de game/flow_rest (opcodes EVTPAL, camara, pantallas continue/game over/entrada de nombre). Ver game/flow_rest/MANIFEST.md.
// Los datos de comparacion salen del ELF baserom/SCES_504.11 y de disc/TBL/*.BIN; si faltan, esas comprobaciones se omiten con un aviso.
// Compilar y ejecutar desde la raiz del repo:
//   mkdir -p build/verify_flow_rest && c++ -std=c++17 -Wall game/test/flow_rest_test.cpp game/flow_rest/*.cpp game/flow/*.cpp -o build/verify_flow_rest/flow_rest_test && build/verify_flow_rest/flow_rest_test
#include "../flow_rest/evtpal_ops.h"
#include "../flow_rest/camera.h"
#include "../flow_rest/screens.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <array>
#include <string>
#include <map>
#include <vector>
using namespace vn;
static int fails = 0, skipped = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static std::vector<uint8_t> readFile(const char* p) {
    std::ifstream f(p, std::ios::binary); if (!f) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static std::vector<uint8_t> gElf;
static bool haveElf() { if (gElf.empty()) gElf = readFile("baserom/SCES_504.11"); return !gElf.empty(); }
// palabra del ELF en la direccion virtual v (offset = v - 0x140000 + 0x800)
static uint32_t elfWord(uint32_t v) { size_t o = (size_t)v - 0x140000 + 0x800; uint32_t w = 0; if (o + 4 <= gElf.size()) memcpy(&w, &gElf[o], 4); return w; }
static void skip(const char* why) { printf("(omitido) %s\n", why); skipped++; }

// ============ (a) opcodes EVTPAL ============
static void testOpcodeTable() {
    // nulos: 6, 8, 19, 48, 111; 111 slots validos con plantillas 0..110
    int valid = 0, maxT = -1;
    for (int op = 0; op < kEvOpcodeSlots; op++) {
        int t = evTemplateOfOpcode((uint32_t)op);
        bool isNull = (op == 6 || op == 8 || op == 19 || op == 48 || op == 111);
        CHECK((t < 0) == isNull); CHECK((kEvHandlerAddr[op] == 0) == isNull); CHECK((evOpFromOpcodeV2((uint32_t)op) == EvOp::Unknown) == isNull);
        if (!isNull) { valid++; if (t > maxT) maxT = t; CHECK(t == (valid - 1)); }       // plantillas consecutivas
    }
    CHECK(valid == 111 && maxT == 110);
    CHECK(evOpFromOpcodeV2(116) == EvOp::Unknown && evOpFromOpcodeV2(0xFFFFFFFFu) == EvOp::Unknown);
    // nombres de los opcodes que game/flow dejaba sin resolver
    CHECK(evOpFromOpcodeV2(29) == EvOp::ITEMSET && evOpFromOpcodeV2(30) == EvOp::BSFT && evOpFromOpcodeV2(31) == EvOp::WAIT_INT_MODE);
    CHECK(evOpFromOpcodeV2(32) == EvOp::FOGCTRL && evOpFromOpcodeV2(33) == EvOp::FOGCTRL && evOpFromOpcodeV2(34) == EvOp::LIGHTCTRL && evOpFromOpcodeV2(35) == EvOp::LIGHTCTRL);
    CHECK(evOpFromOpcodeV2(36) == EvOp::MLLIGHT && evOpFromOpcodeV2(37) == EvOp::PLLIGHT && evOpFromOpcodeV2(38) == EvOp::LIGHTIMMEXE);
    CHECK(evOpFromOpcodeV2(39) == EvOp::PUSHLIGHT && evOpFromOpcodeV2(40) == EvOp::POPLIGHT && evOpFromOpcodeV2(41) == EvOp::DOFCTRL);
    CHECK(evOpFromOpcodeV2(42) == EvOp::SCEFFINOUT && evOpFromOpcodeV2(43) == EvOp::BLURCTRL && evOpFromOpcodeV2(44) == EvOp::OVLAP);
    CHECK(evOpFromOpcodeV2(45) == EvOp::MOTVOLUME && evOpFromOpcodeV2(46) == EvOp::MOTSHDWMAP && evOpFromOpcodeV2(47) == EvOp::WAIT_BUFFERRAM);
    CHECK(evOpFromOpcodeV2(49) == EvOp::BGTB && evOpFromOpcodeV2(50) == EvOp::FLRTB && evOpFromOpcodeV2(51) == EvOp::SKYKD && evOpFromOpcodeV2(52) == EvOp::SKYDSP);
    CHECK(evOpFromOpcodeV2(55) == EvOp::NEXTEV && evOpFromOpcodeV2(110) == EvOp::JOBFLAGSET);
    CHECK(evOpFromOpcodeV2(112) == EvOp::BRANCHITEM && evOpFromOpcodeV2(113) == EvOp::ENTRYSUBGIRL && evOpFromOpcodeV2(114) == EvOp::GAMEFLAGSET && evOpFromOpcodeV2(115) == EvOp::WAIT_FLUSHDATA_NOPAUSE);
    // coincide con la tabla antigua en todo lo que esta ya resolvido, salvo 111..114 (extrapolacion equivocada); cuenta de los huecos
    int oldUnknown = 0, agree = 0, differ = 0;
    for (uint32_t op = 0; op < 111; op++) {
        EvOp a = evOpFromOpcode(op), b = evOpFromOpcodeV2(op);
        if (a == EvOp::Unknown) { oldUnknown++; continue; }
        if (a == b) agree++; else differ++;
    }
    CHECK(differ == 0);
    CHECK(oldUnknown == 22 && agree == 89);                          // 22 huecos en 0..110: 4 slots nulos (6 8 19 48) + 18 rutinas reales
    printf("(info) tabla antigua 0..110: %d resueltos y coincidentes, %d sin resolver (4 son slots nulos reales: 6 8 19 48)\n", agree, oldUnknown);
    CHECK(evOpFromOpcode(111) == EvOp::BRANCHITEM && evOpFromOpcodeV2(111) == EvOp::Unknown);   // el error de la extrapolacion
    // contra el ELF: cada plantilla t esta en 0x26B530 + 16*t con ptmf [0, -1, rutina]; el tercer campo es la rutina
    if (!haveElf()) { skip("baserom/SCES_504.11 no disponible: no se compara la tabla de rutinas con el ELF"); return; }
    int checked = 0;
    for (int op = 0; op < kEvOpcodeSlots; op++) {
        int t = evTemplateOfOpcode((uint32_t)op); if (t < 0) continue;
        uint32_t base = 0x26B530u + 16u * (uint32_t)t;
        CHECK(elfWord(base) == 0 && elfWord(base + 4) == 0xFFFFFFFFu && elfWord(base + 8) == kEvHandlerAddr[op]); checked++;
    }
    CHECK(checked == 111);
    CHECK(elfWord(0x261608) == 0 && elfWord(0x26160C) == 0 && elfWord(0x261610) == 0);     // D_00261608: el ptmf nulo que rellena los slots 6, 8, 19, 48, 111
}

// ---- longitudes con scripts sinteticos ----
struct Syn { VecMem m; Syn(std::vector<uint32_t> w) : m(0x1000, std::move(w)) {} int len(int at = 0) { return evInsnLength(m, 0x1000 + 4u * at).words; } };
static void testLengths() {
    const uint32_t E = 0xFFFFFFFFu;
    CHECK(Syn({0, 0xA0, E}).len() == 3);                           // ENTB con un registro
    CHECK(Syn({0, 0xA0, 0xA4, 0xA8, E}).len() == 5);
    CHECK(Syn({0, E, E}).len() == 3);                              // el original procesa siempre el primer registro y mira el -1 desde pc+8
    CHECK(Syn({4, 1, E}).len() == 3 && Syn({2, 1, 2, E}).len() == 4 && Syn({3, 1, E}).len() == 3);
    CHECK(Syn({1, 0, 5, 6, E}).len() == 5);                        // ENTB2 modo 0 (MENM): lista desde pc+8, mira -1 desde pc+12
    CHECK(Syn({1, 0, E}).len() == 4 && Syn({1, 0, 5, E}).len() == 4);   // el primer registro (pc+8) se procesa siempre
    CHECK(Syn({1, 1, 3, 9, E}).len() == 5 && Syn({1, 1, 3, 9, 9, E}).len() == 6);   // modo 1 (RAND): cuenta en pc+8, -1 desde pc+16
    CHECK(Syn({1, 2, E}).len() == 3 && Syn({1, 9, 0}).len() == 3);                   // otro modo: 3 palabras
    CHECK(Syn({5, E}).len() == 2 && Syn({5, 1, 2, E}).len() == 4);                   // ZMAT
    CHECK(Syn({7, E}).len() == 2 && Syn({7, 1, 2, 3, E}).len() == 5);                // WLCTB
    CHECK(Syn({76, E}).len() == 2 && Syn({82, 1, 2, E}).len() == 4 && Syn({85, 1, E}).len() == 3);
    // CCTB: 3 + (tipo >> 4)
    CHECK(Syn({16, 0, 0x00}).len() == 3 && Syn({16, 0, 0x10}).len() == 4 && Syn({16, 0, 0x40}).len() == 7 && Syn({16, 0, 0x30}).len() == 6);
    CHECK(Syn({16, 0, E}).len() == 2);                             // (s32)-1 >> 4 = -1: el original lo hace asi (nunca aparece en datos reales)
    // VIBRATION
    CHECK(Syn({63, 0}).len() == 4 && Syn({63, 1}).len() == 2 && Syn({63, 2}).len() == 5 && Syn({63, 3}).len() == 0);
    // FOGCTRL set (32) / change (33), LIGHTCTRL set (34) / change (35), DOF (41), BLUR (43)
    const int fs[10] = {3, 3, 3, 3, 3, 5, 4, 3, 3, 3};
    for (int s = 0; s < 10; s++) CHECK(Syn({32, (uint32_t)s, 0, 0, 0}).len() == fs[s]);
    const int fc[8] = {4, 4, 4, 4, 4, 6, 4, 4};
    for (int s = 0; s < 8; s++) CHECK(Syn({33, (uint32_t)s, 0, 0, 0, 0}).len() == fc[s]);
    const int ls[20] = {3, 3, 3, 5, 5, 5, 5, 3, 4, 4, 4, 6, 6, 5, 4, 4, 4, 4, 3, 3};
    for (int s = 0; s < 20; s++) CHECK(Syn({34, (uint32_t)s, 0, 0, 0, 0, 0}).len() == ls[s]);
    const int lc[11] = {4, 4, 4, 6, 6, 4, 6, 4, 5, 4, 4};
    for (int s = 0; s < 11; s++) CHECK(Syn({35, (uint32_t)s, 0, 0, 0, 0, 0}).len() == lc[s]);
    for (int s = 0; s < 10; s++) CHECK(Syn({41, (uint32_t)s, 0, 0}).len() == 3);
    const int bl[10] = {3, 3, 3, 4, 4, 3, 3, 3, 2, 3};
    for (int s = 0; s < 10; s++) CHECK(Syn({43, (uint32_t)s, 0, 0, 0}).len() == bl[s]);
    // fijos
    CHECK(Syn({29, 0, 0}).len() == 3 && Syn({30, 1}).len() == 2 && Syn({36, 0}).len() == 2 && Syn({44, 0, 0, 0, 0}).len() == 5 && Syn({46, 0, 0}).len() == 3);
    CHECK(Syn({49, 0}).len() == 2 && Syn({54}).len() == 1 && Syn({98, 0, 0, 0, 0, 0}).len() == 6 && Syn({104, 0, 0, 0, 0, 0, 0}).len() == 7);
    CHECK(Syn({112, 0}).len() == 0 && Syn({55}).len() == 0 && Syn({28}).len() == 0 && Syn({42}).len() == 0);   // no avanzan por si mismos
    CHECK(Syn({6}).len() == -1 && Syn({8}).len() == -1 && Syn({19}).len() == -1 && Syn({48}).len() == -1 && Syn({111}).len() == -1 && Syn({116}).len() == -1);
}

// ---- barrido de EVTPAL.BIN real ----
struct Sweep { int scripts = 0, nextEv = 0, branchEv = 0, wait = 0, unknown = 0, other = 0; };
static Sweep sweep(const ScriptMem& mem, size_t nWords, bool legacy, bool follow, std::vector<EvScanResult::Ins>* all = nullptr) {
    Sweep s; const uint32_t B = 0x1FC2000, end = B + (uint32_t)nWords * 4;
    for (int st = 0; st < 12; st++) {
        if (st == 10 || st == 11) continue;                                       // igual que el barrido de game/flow_test (comparten tabla con 9)
        uint32_t l1 = mem.rd(B + 4u * (uint32_t)st);
        for (int ev = 0; mem.rd(l1 + 4u * (uint32_t)ev) >= B && mem.rd(l1 + 4u * (uint32_t)ev) < end; ev++) {
            uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)ev);
            for (int ph = 0; mem.rd(l2 + 4u * (uint32_t)ph) >= B && mem.rd(l2 + 4u * (uint32_t)ph) < end; ph++) {
                EvScanOptions o; o.legacyTable = legacy; o.followPhaseJumps = follow; o.stage = st; o.event = ev;
                EvScanResult r = evScanScript(mem, mem.rd(l2 + 4u * (uint32_t)ph), o, all, B, (uint32_t)nWords * 4);
                s.scripts++;
                if (r.reachedNextEv) s.nextEv++;
                else switch (r.firstStop) {
                    case EvEnd::BranchEv: case EvEnd::SetEvNo: s.branchEv++; break;
                    case EvEnd::WaitForever: s.wait++; break;
                    case EvEnd::UnknownOpcode: s.unknown++; break;
                    default: s.other++; }
            }
        }
    }
    return s;
}
static void testRealEvtpal() {
    std::vector<uint8_t> raw = readFile("disc/TBL/EVTPAL.BIN");
    if (raw.empty()) { skip("disc/TBL/EVTPAL.BIN no disponible: se omite el barrido"); return; }
    std::vector<uint32_t> w(raw.size() / 4); memcpy(w.data(), raw.data(), w.size() * 4);
    VecMem mem(0x1FC2000, w);
    Sweep before = sweep(mem, w.size(), true, false), after = sweep(mem, w.size(), false, false), afterF = sweep(mem, w.size(), false, true);
    printf("(info) EVTPAL.BIN real, %d scripts (stages 10/11 excluidas):\n", after.scripts);
    printf("(info)   tabla antigua (+longitudes nuevas): %d llegan a NEXTEV, %d paran en opcode sin resolver, %d BRANCHEV/SETEVNO, %d WAIT sin fin, %d otros\n", before.nextEv, before.unknown, before.branchEv, before.wait, before.other);
    printf("(info)   tabla nueva:                        %d llegan a NEXTEV, %d paran en opcode sin resolver, %d BRANCHEV/SETEVNO, %d WAIT sin fin, %d otros\n", after.nextEv, after.unknown, after.branchEv, after.wait, after.other);
    printf("(info)   tabla nueva siguiendo saltos de fase: %d llegan a NEXTEV, %d WAIT sin fin, %d otros\n", afterF.nextEv, afterF.wait, afterF.other + afterF.unknown + afterF.branchEv);
    CHECK(after.scripts == 402 && before.scripts == 402);
    CHECK(after.unknown == 0 && after.other == 0);                  // ningun script se queda sin poder medirse
    CHECK(after.nextEv > before.nextEv);
    CHECK(after.nextEv + after.branchEv + after.wait == after.scripts);
    // volcado de referencia: longitud de cada instruccion alcanzable obtenida ejecutando las rutinas del asm (build/verify_flow_rest/evtpal_golden.txt)
    std::ifstream g("build/verify_flow_rest/evtpal_golden.txt");
    if (!g) { skip("build/verify_flow_rest/evtpal_golden.txt no disponible: no se compara cada instruccion con la ejecucion del asm"); return; }
    std::map<uint32_t, std::pair<uint32_t, int>> gold; std::string line;
    while (std::getline(g, line)) { if (line.empty() || line[0] == '#') continue; unsigned pc, op; int n; if (sscanf(line.c_str(), "%x %u %d", &pc, &op, &n) == 3) gold[pc] = {op, n}; }
    std::vector<EvScanResult::Ins> all; Sweep full = sweep(mem, w.size(), false, false, &all);
    (void)full;
    // el volcado de referencia incluye tambien las stages 10/11 (mismo contenido que 9): comparamos solo las visitadas aqui
    std::map<uint32_t, std::pair<uint32_t, int>> mine; for (auto& i : all) mine[i.pc] = {i.opcode, i.words};
    int cmp = 0, bad = 0;
    for (auto& kv : mine) { auto it = gold.find(kv.first); if (it == gold.end()) { bad++; continue; } cmp++; if (it->second != kv.second) bad++; }
    printf("(info) instrucciones comparadas con la ejecucion del asm: %d (discrepancias %d)\n", cmp, bad);
    CHECK(bad == 0 && cmp > 10000);
}

// ============ (b) camara ============
struct Cv { std::vector<float> k; FCurve c; Cv(std::vector<std::array<float,4>> keys) { for (auto& a : keys) for (float f : a) k.push_back(f); c.n = (int)keys.size(); c.iters = 1; while ((1 << c.iters) < c.n * 2) c.iters++; c.keys = k.data(); } };
static double hermiteRef(const FCurve& c, double x) {              // referencia en doble con busqueda lineal del segmento
    auto K = [&](int i, int j) { return (double)c.keys[4 * i + j]; };
    if (x > K(c.n - 1, 0)) return K(c.n - 1, 1);
    int i = 1; while (i < c.n - 1 && K(i, 0) < x) i++;
    double t0 = K(i-1,0), d = K(i,0) - t0, u = x - t0, v0 = K(i-1,1), v1 = K(i,1), mo = K(i-1,2), mi = K(i,3), u2 = u*u, u3 = u2*u;
    return (1 - 3*u2/(d*d) + 2*u3/(d*d*d))*v0 + (3*u2/(d*d) - 2*u3/(d*d*d))*v1 + (u - 2*u2/d + u3/(d*d))*mo + (u3/(d*d) - u2/d)*mi;
}
static void testCalcFCurve() {
    Cv lin({{5,10,2,2},{15,30,2,2},{25,50,2,2},{35,70,2,2}});   // 4 claves: con n=3 la busqueda del original no alcanza nunca la ultima (a1=1, a2=0)                    // recta con tangentes = pendiente
    for (float x = 5.5f; x < 35.f; x += 1.7f) CHECK(std::fabs(calcFCurve(lin.c, x) - (10 + 2 * (x - 5))) < 1e-3f);
    CHECK(calcFCurve(lin.c, 100.f) == 70.f);
    Cv s2({{0,0,0,0},{10,100,0,0}});                                 // tangentes nulas: 3u^2-2u^3
    CHECK(std::fabs(calcFCurve(s2.c, 5.f) - 50.f) < 1e-3f && std::fabs(calcFCurve(s2.c, 2.5f) - 15.625f) < 1e-3f);
    CHECK(calcFCurve(FCurve{}, 1.f) == 0.f);
    // contra la referencia en doble con tangentes arbitrarias y 17 claves (varias profundidades de la busqueda binaria)
    for (int n : {2, 3, 5, 8, 9, 17, 33}) {
        std::vector<std::array<float,4>> ks; for (int i = 0; i < n; i++) ks.push_back({(float)(i * 3 + (i % 2)), (float)((i * 37) % 11) - 5.f, (float)(i % 3) - 1.f, (float)((i + 1) % 4) - 1.5f});
        Cv c(ks); double worst = 0;
        for (float x = ks[0][0] + 0.1f; x < ks.back()[0]; x += 0.37f) worst = std::fmax(worst, std::fabs(calcFCurve(c.c, x) - hermiteRef(c.c, x)));
        CHECK(worst < 2e-3);
    }
    alignas(4) uint8_t b[36] = {2,0,2,0}; float kk[8] = {0,0,0,0,10,100,0,0}; memcpy(b + 4, kk, 32);
    FCurve f = FCurve::fromBytes(b); CHECK(f.n == 2 && f.iters == 2 && std::fabs(calcFCurve(f, 5.f) - 50.f) < 1e-3f);
}
struct SynPaths : CamCurveSet {
    std::vector<Cv*> cv; int mode = 0;
    bool curve(int, int k, FCurve& o) const override { if (k >= (int)cv.size()) return false; o = cv[k]->c; return true; }
    int gp730C() const override { return mode; }
};
struct SynEngine : CamEngine {
    bool demo = false; int w = 0;
    int arcTan2(float y, float x) override { return (int)std::lround(std::atan2(y, x) * 32768.0 / 3.14159265358979323846); }
    void getAngleYX(float dx, float dy, float dz, int* yaw, int* pit) override { *yaw = (int)std::lround(std::atan2(dx, dz) * 32768.0 / 3.14159265358979323846); *pit = (int)std::lround(std::atan2(dy, std::sqrt(dx*dx+dz*dz)) * 32768.0 / 3.14159265358979323846); }
    bool isDemoPlay() override { return demo; }
    int demoWord500DF8() override { return w; }
};
static void testCameraUtil() {
    std::vector<Cv*> cv; for (int k = 0; k < 8; k++) cv.push_back(new Cv({{0,(float)k,0,0},{10,(float)(k+10),0,0}}));   // valor k..k+10, tangentes 0
    SynPaths sp; sp.cv = cv; float p[3], l[3], ex = -1; int32_t fl = 99;
    getCameraPath(sp, 100.f, 0, p, l, &fl, &ex);                      // pasada la ultima clave: valor constante de cada curva
    CHECK(p[0] == 10 && p[1] == 11 && p[2] == 12 && l[0] == 13 && l[1] == 14 && l[2] == 15 && ex == 16 && fl == 0);
    sp.mode = 1; getCameraPath(sp, 100.f, 0, p, l, &fl, &ex); CHECK(fl == 17);
    sp.mode = 2; getCameraPath(sp, 100.f, 0, p, l, &fl, &ex); CHECK(fl == 0);
    float o[6]; getObjectPath(sp, 100.f, 0, o); CHECK(o[0] == 10 && o[5] == 15);
    CHECK(interDivPosI(10.f, 20.f, 1, 3) == (10.f * 3 + 20.f) / 4.f && interDivPosI(5.f, 9.f, 0, 4) == 5.f);
    SynEngine e; float a[3] = {1,0,0}, b[3] = {0,1,0}, c[3] = {2,0,0};
    CHECK(vecAng(e, a, b) == 16384 && vecAng(e, a, c) == 0);
    for (auto* c2 : cv) delete c2;
}
struct CapHost : CamHost { int calls = 0; };
static void testCameraGeo() {
    std::vector<Cv*> cv; for (int k = 0; k < 8; k++) cv.push_back(new Cv({{0,(float)(k*10),0,0},{100,(float)(k*10+100),0,0}}));
    SynPaths sp; sp.cv = cv; SynEngine eng;
    // setFinishPos: CCTB 0x30 {fotograma 50, camara 3, sub 1}; con tangentes nulas v(50) = k*10 + 50 (cubica simetrica)
    {
        VecMem m(0x1000, {16, 0, 0x30, 50, 3, 1}); CamJobQueue q; q.mem = &m; q.ringRec[0] = 0x1000; q.ringState[0] = 2; q.slot[0].ring = 0; q.slot[0].job = CJ_FINISHPOS; q.slot[0].handler = CH_FINISHPOS; q.head = 0; q.tail = 1;
        CameraGeo g(q, eng, sp); g.setFinishPos(0);
        CHECK(std::fabs(g.slot[0].subPos[1].x - 50.f) < 1e-3f && std::fabs(g.slot[0].subPos[1].z - 70.f) < 1e-3f && std::fabs(g.slot[0].subLook[1].x - 80.f) < 1e-3f);
        CHECK(q.slot[0].sub[2] == -1 && q.ringState[0] == 0 && q.slot[0].job == CJ_NONE);
    }
    // traceCamPath modo 1: fija ruta sin recorrerla
    {
        VecMem m(0x1000, {16, 0, 0x40, 10, 90, 2, 1}); CamJobQueue q; q.mem = &m; q.ringRec[0] = 0x1000; q.ringState[0] = 2; q.slot[0].ring = 0; q.head = 0; q.tail = 1; q.cam[0].speed = 2.f;
        CameraGeo g(q, eng, sp); g.traceCamPath(0);
        CHECK(g.cam[0].camNo == 2 && q.slot[0].sub[0] == 2 && g.slot[0].frameInt == 8 && q.cam[0].endFrame == 90 && g.slot[0].endInt == 90 && g.slot[0].flag47 == 1 && q.ringState[0] == 0);
    }
    // traceCamPath de un solo fotograma: coloca la camara
    {
        VecMem m(0x1000, {16, 0, 0x40, 50, 50, 1, 0}); CamJobQueue q; q.mem = &m; q.ringRec[0] = 0x1000; q.ringState[0] = 2; q.head = 0; q.tail = 1;
        CameraGeo g(q, eng, sp); g.traceCamPath(0);
        CHECK(std::fabs(g.cam[0].pos.x - 50.f) < 1e-3f && std::fabs(g.cam[0].look.x - 80.f) < 1e-3f && std::fabs(g.fov[0] - 110.f) < 1e-3f && q.cam[0].frame == 50.f);
    }
    // traceCamPath recorrido (modo 0): avanza con traceCamPath_000 en modo demo hasta el final y termina
    {
        VecMem m(0x1000, {16, 0, 0x40, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}); CamJobQueue q; q.mem = &m; q.ringRec[0] = 0x1000; q.ringState[0] = 2; q.head = 0; q.tail = 1; q.cam[0].speed = 1.f;
        // traceCamPath_000 lee el fotograma final en rec + 0x10 (palabra 4) = 20
        eng.demo = true; CameraGeo g(q, eng, sp); g.traceCamPath(0);
        for (int i = 0; i < 100 && q.ringState[0] != 0; i++) g.traceCamPath_000(0);   // una vez por fotograma
        CHECK(q.cam[0].frame == 20.f && q.cam[0].speed == 1.2f && q.slot[0].job == CJ_NONE && q.ringState[0] == 0);
        eng.demo = false;
    }
    // setAfterSpeed: tabla de s8; 50/60 del valor
    {
        static const int8_t t0[64] = {60, 12, -6}, t1[64] = {30}, t2[64] = {0};
        CamJobQueue q; CameraGeo g(q, eng, sp); g.speed.t[0] = t0; g.speed.t[1] = t1; g.speed.t[2] = t2;
        g.cam[0].pos = Anpo{}; g.cam[0].look.x = 1; g.cam[0].look.y = 0; g.cam[0].look.z = 0; g.slot[0].tgtLook[0] = 1; g.slot[0].tgtLook[1] = 0; g.slot[0].tgtLook[2] = 0;
        q.slot[0].cafsIdx = 0; g.cam[0].d3 = 0; g.setAfterSpeed(0); CHECK(g.slot[0].afterSpeed == 50);          // angulo 0 -> idx 0 -> 60*50/60
        g.slot[0].tgtLook[0] = 1; g.slot[0].tgtLook[1] = 1; g.setAfterSpeed(0); CHECK(g.slot[0].afterSpeed == 0);  // 45 grados = 0x2000 -> acotado 0x1FFF -> idx 63 (0)
        q.slot[0].cafsIdx = 7; g.slot[0].tgtLook[1] = 0; g.setAfterSpeed(0); CHECK(g.slot[0].afterSpeed == 25);     // >= 6 -> tabla 1
        g.cam[0].d3 = 1; q.slot[0].cafsIdx = 0; g.setAfterSpeed(0); CHECK(g.slot[0].afterSpeed == 0);               // d3 == 1 -> tabla 2
    }
    // entryObj
    { CamJobQueue q; CameraGeo g(q, eng, sp); float p[3] = {1,2,3}; g.entryObj(0, p, 4.f); g.entryObj(0, p, 5.f); CHECK(g.slot[0].objCount == 2 && g.slot[0].obj[1].w == 5.f && g.slot[0].obj[0].p[2] == 3.f);
      for (int i = 0; i < 30; i++) g.entryObj(0, p, 1.f); CHECK(g.slot[0].objCount == 16); }
    // game(): arranque; game_000 queda pendiente
    {
        CamJobQueue q; VecMem m(0x1000, {16, 0, 0x10, 0}); q.mem = &m; q.ringRec[0] = 0x1000; q.ringState[0] = 2; q.head = 0; q.tail = 1;
        CameraGeo g(q, eng, sp); q.slot[0].sub[0] = 4; g.slot[0].frameInt = 100; g.game(0);
        CHECK(q.cam[0].mode == 2 && g.cam[0].camNo == 4 && q.cam[0].frame == 100.f && std::fabs(g.cam[0].pos.x - 100.f) < 1e-3f && g.pending == 1 && g.lastPending == CH_GAME000);
        CHECK(q.slot[0].sub[1] == -2 && g.slot[0].subPos[0].x == g.cam[0].pos.x);            // -2 libres se rellenan con la camara actual
    }
    for (auto* c : cv) delete c;
}

// ============ (c) pantallas ============
struct FakeScr : ScreenEngine {
    std::vector<std::string> log; CreditSlot cd[2]; bool free = false, enough = false, trig = false, reload = false, start = false, decide = false; int pause = 0;
    bool animDone = false, backDone = false;
    void changeExec(PlExec e, int) override { log.push_back("exec" + std::to_string((int)e)); }
    CreditSlot creditData(int i) override { return cd[i]; }
    bool isFreePlay() override { return free; }
    bool creditEnough(int, bool c) override { log.push_back(c ? "enough1" : "enough0"); return enough; }
    int pauseLevel() override { return pause; }
    bool pullTrigger(int) override { return trig; } bool pressReload(int) override { return reload; } bool pressStart(int) override { return start; } bool pressDecide(int) override { return decide; }
    void drawDamageEffect(int) override { log.push_back("dam"); } void scoreDispCheck(int) override { log.push_back("score"); }
    void maskScreen() override { log.push_back("mask"); } void soundSeq(uint32_t id) override { log.push_back("snd" + std::to_string(id)); }
    void setBookPlGameEnd(int) override { log.push_back("book"); } void clearPlayerBytes(int) override { log.push_back("clr"); }
    void drawNameCursor(int, float x, float y) override { log.push_back("cur" + std::to_string((int)x) + "," + std::to_string((int)y)); }
    void gunExe(int) override { log.push_back("gun"); } void bullet(int) override { log.push_back("bul"); }
    void bookCover(int f) override { log.push_back("cover" + std::to_string(f)); } bool bookAnimDone() override { return animDone; } bool bookAnimBackDone() override { return backDone; }
    void bookPutPages() override { log.push_back("pages"); } void bookPut() override { log.push_back("put"); } void taskLoop() override { log.push_back("task"); }
    bool has(const std::string& s) const { for (auto& l : log) if (l == s) return true; return false; }
};
static void testScreens() {
    // --- contInit ---
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); e.cd[0] = {7, 2}; e.cd[1] = {9, 0};
      ps.contInit(); CHECK(p.contCount == 0x9FFF && p.credSnap0 == 7 && p.credSnap1 == 9 && e.log.size() == 1 && e.log[0] == "exec0");
      s.gameState500F14 = 0xA; s.gp73B4 = 2; e.log.clear(); ps.contInit(); CHECK(p.mode == 1 && e.log.size() == 1 && e.log[0] == "exec3"); }
    // --- contLoop: cuenta atras normal ---
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); s.gp72D0 = 2; e.cd[0] = {1, 2};
      p.credSnap0 = 1; p.credSnap1 = 0; p.contCount = 0x9FFF; ps.contLoop();
      CHECK(p.contCount == 0x9FFF - 0x1E && p.contActive == 1 && (p.flags27FC & 0x12) == 0x12 && e.log.front() == "dam");
      // gatillo con cuenta < 0x8000: salta al inicio del digito + 1 y luego resta 0x1E
      p.contCount = 0x5ABC; e.trig = true; p.flags27FC = 0; ps.contLoop(); CHECK(p.contCount == 0x5000 + 1 - 0x1E);
      // con cuenta >= 0x8000 el gatillo no cambia nada
      p.contCount = 0x9ABC; ps.contLoop(); CHECK(p.contCount == 0x9ABC - 0x1E);
      // cambio de credito reinicia la cuenta (y luego resta)
      e.trig = false; e.cd[0].value = 5; p.contCount = 100; ps.contLoop(); CHECK(p.credSnap0 == 5 && p.contCount == 0x9FFF - 0x1E);
      // pausa o gp73F0 == 4: no cuenta
      p.contCount = 1000; e.pause = 1; ps.contLoop(); CHECK(p.contCount == 1000); e.pause = 0; s.gp73F0 = 4; p.flags27FC = 0; ps.contLoop(); CHECK(p.contCount == 1000 && !(p.flags27FC & 2)); }
    // --- contLoop: expira ---
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); s.gp72D0 = 2; s.word500DFC = 3; e.cd[0] = {1, 2}; p.credSnap0 = 1; p.credSnap1 = 0; p.contCount = 0x10;
      ps.contLoop(); CHECK(p.contCount == 0 && s.word500DFC == 2 && p.mode == 6 && p.contActive == 0 && !(p.flags27FC & 0x12) && e.has("exec3")); }
    // sin credito en curso (ningun estado == 2) y no gratis -> expira; gratis -> no
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); s.gp72D0 = 2; p.contCount = 0x9FFF; ps.contLoop(); CHECK(p.mode == 6);
      PlayerScr p2; ScreenSys s2; FakeScr e2; PlayerScreens ps2(s2, e2, p2); s2.gp72D0 = 2; e2.free = true; p2.contCount = 0x9FFF; ps2.contLoop(); CHECK(p2.mode == 0 && p2.contCount == 0x9FFF - 0x1E); }
    // --- contLoop con credito suficiente + pulsar start en estado 10 ---
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); s.gp72D0 = 2; s.gameState500F14 = 0xA; e.start = true; e.enough = true; e.cd[0] = {1, 2}; p.credSnap0 = 1; p.contCount = 0x9FFF; p.mode = 4;
      ps.contLoop(); CHECK(p.mode == 0xA && p.reqMode94 == 1 && e.has("enough1") && e.has("snd65550") && !(p.flags27FC & 0x12) && p.contActive == 0);   // 0x1000E = 65550
      // con bit 0 de gp72D0: modo pedido (1) en vez de 0xA
      ScreenSys s2; FakeScr e2; PlayerScr p2; PlayerScreens ps2(s2, e2, p2); s2.gp72D0 = 3; s2.gameState500F14 = 0xA; e2.start = true; e2.enough = true; e2.cd[0] = {1, 2}; p2.credSnap0 = 1;
      ps2.contLoop(); CHECK(p2.mode == 1);
      // fuera del estado 10 el credito suficiente pide pasar a 10, pone bit 0 y enmascara
      ScreenSys s3; FakeScr e3; PlayerScr p3; PlayerScreens ps3(s3, e3, p3); e3.enough = true; p3.flag18 = 1; s3.gameState500F14 = 5; ps3.checkCredAndStBtn(1);
      CHECK(s3.nextState500F16 == 0xA && (s3.gp72D0 & 1) && e3.has("mask") && p3.mode == 1 && p3.flag18 == 0 && e3.has("snd65550"));
      // estado 6: no suena
      ScreenSys s4; FakeScr e4; PlayerScr p4; PlayerScreens ps4(s4, e4, p4); e4.enough = true; p4.flag18 = 1; s4.gameState500F14 = 6; s4.gp72D0 = 8; ps4.checkCredAndStBtn(1); CHECK(!e4.has("snd65550") && p4.mode == 1);
      // bit 8 con modo distinto de 1 -> 0xA con reqMode94
      ScreenSys s5; FakeScr e5; PlayerScr p5; PlayerScreens ps5(s5, e5, p5); e5.enough = true; s5.gameState500F14 = 0xA; s5.gp72D0 = 8; CHECK(ps5.checkCredSt(2) && p5.mode == 0xA && p5.reqMode94 == 2);
      // sin credito: nada
      ScreenSys s6; FakeScr e6; PlayerScr p6; PlayerScreens ps6(s6, e6, p6); CHECK(!ps6.checkCredSt(1) && p6.mode == 0 && e6.log.size() == 1); }
    // --- game over ---
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); p.playerNo = 1; s.gp73B4 = 1; s.gameState500F14 = 10; ps.gmoverInitPos[1][0] = 11; ps.gmoverInitPos[1][2] = 33; p.vel[1] = 5;
      ps.gmoverInit(); CHECK(e.has("clr") && (p.flags27FC & 4) && p.pos[0] == 11 && p.pos[2] == 33 && p.vel[1] == 0 && p.gmoverCount == 100 && e.has("exec1"));
      s.word500E00 = 1; ps.gmoverInit(); CHECK(p.pos[0] == 0 && p.pos[2] == 0);
      s.gameState500F14 = 0xB; p.flags27FC = 0; ps.gmoverInit(); CHECK(!(p.flags27FC & 4));
      s.gameState500F14 = 10; p.gmoverCount = 2; p.flags27FC = 0; e.log.clear(); ps.gmoverLoop(); CHECK(p.gmoverCount == 1 && (p.flags27FC & 4) && e.log.empty());
      ps.gmoverLoop(); CHECK(p.gmoverCount == 0 && e.has("book") && p.mode == 9);
      s.gameState500F14 = 11; p.mode = 0; ps.gmoverLoop(); CHECK(p.mode == 0); s.gameState500F14 = 3; ps.gmoverLoop(); CHECK(p.mode == 9); }
    // --- entrada de nombre ---
    { ScreenSys s; FakeScr e; PlayerScr p; PlayerScreens ps(s, e, p); for (int& f : p.nameFields) f = 7; p.unk1CC0 = 5; ps.nameentInit();
      bool z = true; for (int f : p.nameFields) z = z && f == 0; CHECK(z && p.unk1CC0 == 0 && e.has("exec2"));
      NameCursor c; c.x = 10; c.y = -4; c.active = 1; e.log.clear(); ps.nameentLoop(c);
      CHECK(p.unk4C == 8 && e.log.size() == 3 && e.log[0] == "cur330,228" && e.log[1] == "gun" && e.log[2] == "bul");
      c.active = 0; e.log.clear(); ps.nameentLoop(c); CHECK(e.log.size() == 2); s.gp6F90[0] = 1; e.log.clear(); ps.nameentLoop(c); CHECK(e.log.empty()); }
    // --- ranking de final: contador de 0x2B3 fotogramas, pausa >= 5 lo detiene ---
    { ScreenSys s; FakeScr e; RankingScreen r(s, e); s.gp73E4 = 0x2B1; r.loop(); CHECK(s.gp73E4 == 0x2B2 && s.nextState500F16 == 0); r.loop(); CHECK(s.gp73E4 == 0x2B3 && s.nextState500F16 == 0xB && e.has("snd4194305") && e.has("mask"));
      s.nextState500F16 = 0; s.gp73E4 = 5; e.pause = 5; r.loop(); CHECK(s.gp73E4 == 5); }
    // --- libro ---
    { ScreenSys s; FakeScr e; BookScreen b(s, e); s.gp73E4 = 0x63; b.loop(); CHECK(s.gp7114 == 0 && s.gp73E4 == 0x64);
      b.loop(); CHECK(s.gp7114 == 0 && s.gp73E4 == 0x65); b.loop(); CHECK(s.gp7114 == 1 && s.gp73E4 == 0x66); b.loop(); CHECK(s.gp7114 == 1); e.animDone = true; b.loop(); CHECK(s.gp7114 == 2);
      b.bookExit(); CHECK(e.has("snd458771")); b.loop(); CHECK(s.gp7114 == 3); e.backDone = true; b.loop(); CHECK(s.gp7114 == 3);   // gp7118 sigue a 1: el original no lo borra aqui, vuelve a 3
      s.gp7118 = 0; b.loop(); CHECK(s.gp7114 == 0); }
    // --- CalcClearRanking con las dos tablas reales del ELF ---
    if (!haveElf()) { skip("ELF no disponible: no se comprueban las tablas de CalcClearRanking"); return; }
    int32_t tn[8], ta[8]; for (int i = 0; i < 8; i++) { tn[i] = (int32_t)elfWord(0x2C4BD0 + 4 * i); ta[i] = (int32_t)elfWord(0x2C4BF0 + 4 * i); }
    CHECK(tn[1] == 30000 && tn[7] == 72000 && ta[1] == 15000 && ta[7] == 46000);
    for (const int32_t* t : {tn, ta}) {
        CHECK(calcClearRanking(-5, t) == 0 && calcClearRanking(0, t) == 0 && calcClearRanking(t[1] - 1, t) == 0 && calcClearRanking(t[1], t) == 1 && calcClearRanking(t[7], t) == 7 && calcClearRanking(t[7] - 1, t) == 6 && calcClearRanking(2000000000, t) == 7);
        for (int i = 1; i < 8; i++) CHECK(calcClearRanking(t[i], t) == i && calcClearRanking(t[i] - 1, t) == i - 1);
    }
}

int main() {
    testOpcodeTable(); testLengths(); testRealEvtpal();
    testCalcFCurve(); testCameraUtil(); testCameraGeo();
    testScreens();
    if (fails == 0) printf("flow_rest_test: OK (%d comprobaciones omitidas)\n", skipped); else printf("flow_rest_test: %d fallos\n", fails);
    return fails != 0;
}

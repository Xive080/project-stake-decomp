// Tabla opcode -> rutina y longitudes de los scripts EVTPAL (ver evtpal_ops.h). Reescrito a mano; los datos salen de
// __sinit_eventctrl.cc (ELF 0x2C5B80), de las rutinas EventCtrlTask::* (ELF 0x1E2D10..0x1E4FD0), de FogCtrlTask/LightCtrlTask/DofCtrlTask/
// ScrBlurCtrlClass::entryEvent y de EnemyMgrClass::entryEvent* (GAMEMOD 0x935380..0x9356E8).
#include "evtpal_ops.h"
#include <set>
#include <vector>
namespace vn {
using E = EvOp;

// Direccion de la rutina (tercera palabra del ptmf de la plantilla en el ELF 0x26B530 + 16*t). 0 = slot nulo.
const uint32_t kEvHandlerAddr[kEvOpcodeSlots] = {
    0x1E2D10, 0x1E2D40, 0x1E2D70, 0x1E2DD0, 0x1E2E30, 0x1E4920,
    0x0, 0x1E2E80, 0x0, 0x1E49D0, 0x1E49F0, 0x1E4A10,
    0x1E4A30, 0x1E4AB0, 0x1E4B80, 0x1E4C70, 0x1E3060, 0x1E30A0,
    0x1E30D0, 0x0, 0x1E3480, 0x1E34F0, 0x1E35D0, 0x1E36A0,
    0x1E3730, 0x1E37C0, 0x1E3850, 0x1E38E0, 0x1E3BE0, 0x1E3B00,
    0x1E4D80, 0x1E4DA0, 0x1E2F00, 0x1E2F00, 0x1E2F30, 0x1E2F30,
    0x1E2EC0, 0x1E2ED0, 0x1E47F0, 0x1E4830, 0x1E4870, 0x1E48B0,
    0x1E49C0, 0x1E48E0, 0x1E3B70, 0x1E3BB0, 0x1E3BC0, 0x1E3990,
    0x0, 0x1E2FE0, 0x1E2EE0, 0x1E2F60, 0x1E2F80, 0x1E2FA0,
    0x1E3BD0, 0x1E3BF0, 0x1E3010, 0x1E4CE0, 0x1E3460, 0x1E3AE0,
    0x1E3AF0, 0x1E4D40, 0x1E4DC0, 0x1E4E50, 0x1E46C0, 0x1E30F0,
    0x1E3DC0, 0x1E3E00, 0x1E3EC0, 0x1E3F00, 0x1E40C0, 0x1E4100,
    0x1E4140, 0x1E4180, 0x1E41C0, 0x1E4200, 0x1E4240, 0x1E42C0,
    0x1E4340, 0x1E4380, 0x1E43C0, 0x1E4400, 0x1E4440, 0x1E44C0,
    0x1E4540, 0x1E45C0, 0x1E4640, 0x1E4680, 0x1E47B0, 0x1E3E40,
    0x1E3E80, 0x1E3B50, 0x1E3F40, 0x1E3FA0, 0x1E4000, 0x1E4060,
    0x1E3130, 0x1E3180, 0x1E31C0, 0x1E3220, 0x1E3280, 0x1E32C0,
    0x1E3050, 0x1E3300, 0x1E3350, 0x1E33A0, 0x1E3400, 0x1E39F0,
    0x1E3A30, 0x1E3AA0, 0x1E4D20, 0x0, 0x1E4BE0, 0x1E4F10,
    0x1E4FD0, 0x1E3940,
};
static const EvOp kSlotOp[kEvOpcodeSlots] = {
    E::ENTB, E::ENTB2, E::ET1P, E::ET2P, E::ENTBSK,
    E::ZMAT, E::Unknown, E::WLCTB, E::Unknown, E::BRANCHNO,
    E::BRANCH, E::MISSENMCLR, E::SWITCH_MENM, E::BRANCHEV, E::SETEVNO,
    E::BRANCHADR, E::CCTB, E::CAMDR, E::GETCPMODE, E::Unknown,
    E::WAIT_CAMJOB, E::WAIT_CAMFRM, E::WAIT_FRM, E::WAIT_ENNUM, E::WAIT_ENALV,
    E::WAIT_PEOPLE, E::WAIT_ITEM, E::WAIT_FLUSHDATA, E::WAIT, E::ITEMSET,
    E::BSFT, E::WAIT_INT_MODE, E::FOGCTRL, E::FOGCTRL, E::LIGHTCTRL,
    E::LIGHTCTRL, E::MLLIGHT, E::PLLIGHT, E::LIGHTIMMEXE, E::PUSHLIGHT,
    E::POPLIGHT, E::DOFCTRL, E::SCEFFINOUT, E::BLURCTRL, E::OVLAP,
    E::MOTVOLUME, E::MOTSHDWMAP, E::WAIT_BUFFERRAM, E::Unknown, E::BGTB,
    E::FLRTB, E::SKYKD, E::SKYDSP, E::CHGSM, E::SCNINIT,
    E::NEXTEV, E::SKIP, E::BRANCHNOSET, E::SNOWDISP, E::DBG_MALLOC_PRINT,
    E::DBG_ENTDATA_INFO, E::JIMAKU, E::FORCE_CAMPARAM, E::VIBRATION, E::ENTRYSTGDATA,
    E::ENTRYCPBK, E::ENTRYBANK, E::FREEBANK, E::ENTRYSPR, E::FREESPR,
    E::ENTRYMOT, E::FREEMOT, E::ENTRYENVTX, E::FREEENVTX, E::ENTRYOSK,
    E::FREEOSK, E::ENTRYOSKNAME, E::FREEOSKNAME, E::ENTRYOSKBK, E::FREEOSKBK,
    E::ENTRYCLVTX, E::FREECLVTX, E::ENTRYFANI, E::FREEFANI, E::ENTRYFAMOT,
    E::FREEFAMOT, E::ENTRYCOLLI, E::FREECOLLI, E::FLUSHDATA, E::ENTRYTEX,
    E::FREETEX, E::NOWLOADINGSET, E::ENTRYLANGSPR, E::FREELANGSPR, E::ENTRYLANGBANK,
    E::FREELANGBANK, E::ENTRYSNDPK, E::FREESNDPK, E::BGMPLAY, E::VOICEPLAY,
    E::BGMPAUSE, E::BGMFADE, E::SKVOOFF, E::SNDON, E::SNDONM,
    E::SNDONSK, E::SNDONMSK, E::LOAD_OVLMOD, E::WAIT_LOAD_OVLMOD, E::CHGADXPT,
    E::JOBFLAGSET, E::Unknown, E::BRANCHITEM, E::ENTRYSUBGIRL, E::GAMEFLAGSET,
    E::WAIT_FLUSHDATA_NOPAUSE,
};
// Opcodes cuyo slot se rellena con el ptmf nulo D_00261608 (sinit no copia ninguna plantilla)
static const uint8_t kNullSlots[5] = {6, 8, 19, 48, 111};

EvOp evOpFromOpcodeV2(uint32_t opcode) { return opcode < (uint32_t)kEvOpcodeSlots ? kSlotOp[opcode] : EvOp::Unknown; }

int evTemplateOfOpcode(uint32_t opcode) {
    if (opcode >= (uint32_t)kEvOpcodeSlots) return -1;
    int nulls = 0;
    for (uint8_t n : kNullSlots) { if (opcode == n) return -1; if (n < opcode) nulls++; }
    return (int)opcode - nulls;
}

// Longitud fija en palabras (opcode en la primera). -1 nulo; -2 lista terminada en -1; -3 salto; -4 depende de argumentos; -5 no avanza.
static const int8_t kFixed[kEvOpcodeSlots] = {
    -2, -4, -2, -2, -2, -2, -1, -2, -1, 2, -3, 1, -3, -3, -3, -3,
    -4, 2, 2, -1, 1, 3, 2, 2, 2, 2, 2, 1, -5, 3, 2, 2,
    -4, -4, -4, -4, 2, 2, 2, 2, 2, -4, -5, -4, 5, 5, 3, 1,
    -1, 2, 2, 2, 2, 2, 1, -3, 2, 2, 2, 1, 1, 2, 4, -4,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -2, -2, 2, 2,
    2, 2, -2, -2, -2, -2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2,
    4, 2, 6, 6, 2, 4, 1, 5, 7, 5, 7, 2, 2, 2, 2, -1,
    -3, 1, 3, 1,
};
// FOGCTRL/LIGHTCTRL: la longitud la decide el subcodigo (palabra 1). Salen de ejecutar eSETFOG/eCHGFOG/eSETLIGHT/eCHGLIGHT del asm.
static const uint8_t kFogSet[9]    = {3, 3, 3, 3, 3, 5, 4, 3, 3};                     // opcode 32; subcodigo >= 9: 3
static const uint8_t kFogChg[6]    = {4, 4, 4, 4, 4, 6};                               // opcode 33; subcodigo >= 6: 4
static const uint8_t kLightSet[18] = {3, 3, 3, 5, 5, 5, 5, 3, 4, 4, 4, 6, 6, 5, 4, 4, 4, 4};   // opcode 34; subcodigo >= 18: 3
static const uint8_t kLightChg[9]  = {4, 4, 4, 6, 6, 4, 6, 4, 5};                     // opcode 35; subcodigo >= 9: 4
// DOFCTRL (41): todos los subcodigos (0..7 por tabla de saltos, resto = eMTXDOF) miden 3. BLURCTRL (43): subcodigo 3 y 4 -> 4, 8 (eSTOP) -> 2, resto 3.

static const uint32_t kEnd = 0xFFFFFFFFu;
static int scanToEnd(const ScriptMem& m, uint32_t pc, int firstTest) {   // posicion (en palabras) del primer -1 desde `firstTest`; +1 = longitud
    int i = firstTest;
    while (i < 4096 && m.rd(pc + 4u * (uint32_t)i) != kEnd) i++;
    return i + 1;
}

EvLen evInsnLength(const ScriptMem& m, uint32_t pc) {
    uint32_t op = m.rd(pc);
    if (op >= (uint32_t)kEvOpcodeSlots) return {-1, false};
    int f = kFixed[op];
    if (f == -1) return {-1, false};
    if (f >= 0) return {f, true};
    auto arg = [&](int i) { return m.rd(pc + 4u * (uint32_t)i); };
    switch (f) {
    case -3: case -5: return {0, false};
    case -2:
        switch (op) {
        case 0: case 2: case 3: case 4: return {scanToEnd(m, pc, 2), true};     // ENTB/ET1P/ET2P/ENTBSK: EnemyMgr::entryEvent (primer registro sin test)
        case 5: case 7: case 76: case 77: case 82: case 83: case 84: case 85: return {scanToEnd(m, pc, 1), true};   // ZMAT, WLCTB, listas OSKNAME/FANI/FAMOT
        }
        break;
    case -4:
        switch (op) {
        case 1: {                                                // ENTB2 (EnemyMgr::entryEvent2): modo 0 = MENM, 1 = RAND, otro = nada
            uint32_t mode = arg(1);
            if (mode == 0) return {scanToEnd(m, pc, 3), true};
            if (mode == 1) return {scanToEnd(m, pc, 4), true};
            return {3, true}; }
        case 16: return {3 + ((int32_t)arg(2) >> 4), true};      // CCTB: longitud del registro de camara = 3 + (tipo >> 4)
        case 63: {                                               // VIBRATION: clase 0 = 4 palabras, 1 = 2, 2 = 5; otra clase no avanza
            uint32_t k = arg(1); if (k == 0) return {4, true}; if (k == 1) return {2, true}; if (k == 2) return {5, true}; return {0, false}; }
        case 32: { uint32_t s = arg(1); return {s < 9 ? kFogSet[s] : 3, true}; }
        case 33: { uint32_t s = arg(1); return {s < 6 ? kFogChg[s] : 4, true}; }
        case 34: { uint32_t s = arg(1); return {s < 18 ? kLightSet[s] : 3, true}; }
        case 35: { uint32_t s = arg(1); return {s < 9 ? kLightChg[s] : 4, true}; }
        case 41: return {3, true};
        case 43: { uint32_t s = arg(1); return {(s == 3 || s == 4) ? 4 : (s == 8 ? 2 : 3), true}; }
        }
        break;
    }
    return {-1, false};
}

// ---- exploracion ----
static EvOp legacyOp(uint32_t o) { return evOpFromOpcode(o); }

EvScanResult evScanScript(const ScriptMem& m, uint32_t startPc, const EvScanOptions& opt, std::vector<EvScanResult::Ins>* out, uint32_t base, uint32_t bytes) {
    EvScanResult r; std::set<uint32_t> seen; std::vector<uint32_t> stack{startPc};
    auto inRange = [&](uint32_t a) { return a >= base && (bytes == 0 || a < base + bytes) && (a & 3) == 0 && m.rd(a) != kEnd; };
    auto stop = [&](EvEnd e, uint32_t pc) { if (r.firstStop == EvEnd::None) { r.firstStop = e; r.stopPc = pc; } };
    auto lookup = [&](int s, int ev, int ph) -> uint32_t {
        uint32_t l1 = m.rd(base + 4u * (uint32_t)s), l2 = m.rd(l1 + 4u * (uint32_t)ev); return m.rd(l2 + 4u * (uint32_t)ph); };
    while (!stack.empty()) {
        uint32_t pc = stack.back(); stack.pop_back();
        for (;;) {
            if (seen.count(pc)) break;
            seen.insert(pc);
            uint32_t opc = m.rd(pc);
            EvOp o = opt.legacyTable ? legacyOp(opc) : evOpFromOpcodeV2(opc);
            if (o == EvOp::Unknown) { stop(EvEnd::UnknownOpcode, pc); break; }
            r.visited++;
            if (o == EvOp::NEXTEV) { r.reachedNextEv = true; if (out) out->push_back({pc, opc, 0}); break; }
            if (o == EvOp::WAIT || o == EvOp::SCEFFINOUT) { stop(EvEnd::WaitForever, pc); if (out) out->push_back({pc, opc, 0}); break; }
            auto listTargets = [&](uint32_t first, bool addPhase) {       // lista de palabras hasta -1 (la primera entrada siempre cuenta)
                std::vector<uint32_t> t; uint32_t p = first;
                for (int n = 0; n < 4096; n++) { uint32_t w = m.rd(p); t.push_back(w); p += 4; if (m.rd(p) == kEnd) break; }
                (void)addPhase; return t; };
            if (o == EvOp::BRANCH) { uint32_t t = m.rd(pc + 4); if (out) out->push_back({pc, opc, 0}); if (!inRange(t)) { stop(EvEnd::BadTarget, pc); break; } pc = t; continue; }
            if (o == EvOp::BRANCHADR || o == EvOp::BRANCHITEM || o == EvOp::SWITCH_MENM) {
                uint32_t first = pc + (o == EvOp::BRANCHADR ? 4u : 8u);
                if (out) out->push_back({pc, opc, 0});
                std::vector<uint32_t> t = listTargets(first, false), ok;
                for (uint32_t x : t) if (inRange(x)) ok.push_back(x);
                if (ok.empty()) { stop(EvEnd::BadTarget, pc); break; }
                for (size_t i = 1; i < ok.size(); i++) stack.push_back(ok[i]);
                pc = ok[0]; continue; }
            if (o == EvOp::BRANCHEV || o == EvOp::SETEVNO) {
                stop(o == EvOp::BRANCHEV ? EvEnd::BranchEv : EvEnd::SetEvNo, pc);
                if (out) out->push_back({pc, opc, 0});
                if (opt.followPhaseJumps && opt.stage >= 0) {
                    std::vector<uint32_t> phs;
                    if (o == EvOp::SETEVNO) phs.push_back(m.rd(pc + 4));
                    else phs = listTargets(pc + 4, true);
                    for (uint32_t ph : phs) { if (ph > 63) continue; uint32_t x = lookup(opt.stage, opt.event, (int)ph); if (inRange(x)) stack.push_back(x); }
                }
                break; }
            EvLen l = evInsnLength(m, pc);
            if (l.words <= 0) { stop(l.words < 0 ? EvEnd::UnknownOpcode : EvEnd::WaitForever, pc); break; }
            if (out) out->push_back({pc, opc, l.words});
            pc += 4u * (uint32_t)l.words;
            if (!inRange(pc)) { stop(EvEnd::OutOfRange, pc); break; }
        }
    }
    return r;
}
}

#include "eventctrl.h"
#include <cstring>
namespace vn {

uint32_t EventCtrl::lookupPc(int stage, int event, int phase) const {
    uint32_t l1 = mem.rd(evtpalBase + 4u * (uint32_t)stage);        // EVTPAL[etapa]
    uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)event);                // lista de fases del evento
    return mem.rd(l2 + 4u * (uint32_t)phase);
}

void EventCtrl::construct() {                                       // __ct__13EventCtrlTaskFv (estado inicial)
    g.phase = (g.gameState != 9 && g.event != 0) ? 1 : 0;
    pc = lookupPc(g.stage, g.event, g.phase);
    g.savedPc = pc;
    lastPc = 0; wait = 0; intMode = 0;
    g.gp7294 = 0; g.skipReq = 0; g.gp729C = 0;
    g.gp733C = (g.stage == 6) ? 0x1DC0 : 0x2254;
}

bool EventCtrl::canAdvanceNow() const {                             // primer tramo de EventCtrl_000
    bool alive = host.playerLife(0) > 0 && host.playerMode(0) == 5;
    if (!alive) alive = host.playerLife(1) > 0 && host.playerMode(1) == 5;
    if (alive) return true;
    return host.demoPlay();
}

void EventCtrl::frame() {                                           // EventCtrl_000
    g.canAdvance = canAdvanceNow() ? 1 : 0;
    if (host.pauseLevel() < 2) {
        pc = g.savedPc; stopFlag = 0;
        do {
            op = mem.rd(pc);
            if (pc != lastPc) lastPc = pc;
            step();
            g.savedPc = pc;
        } while (wait == 0 && stopFlag == 0);
    }
}

bool EventCtrl::camReady() {
    CamState dummy; dummy.flagD5 = 1;
    CamState& c = cam ? cam->cam[0] : dummy;
    c.flagD6 = 1;                                                   // el original lo fija aqui como efecto lateral
    return c.flagD5 != 0 || intMode != 0;
}

// Saltos con lista terminada en -1: devuelve la palabra elegida (indice == n) o `none` si no hay.
static uint32_t pickFromList(const ScriptMem& m, uint32_t& p, int n, uint32_t none) {
    // p apunta a la primera entrada; sigue el bucle del original: idx = 0.. mientras la palabra != -1
    int idx = -1; uint32_t chosen = none;
    for (;;) {
        uint32_t t = m.rd(p); idx++;
        if (t != 0xFFFFFFFFu && n == idx) { chosen = t; break; }
        p += 4;
        if (m.rd(p) == 0xFFFFFFFFu) break;
    }
    return chosen;
}

void EventCtrl::step() { exec(evOpFromOpcode(op)); }

void EventCtrl::exec(EvOp o) {
    uint32_t a[16] = {};
    auto arg = [&](int i) { return mem.rd(pc + 4u * i); };
    auto fixed = [&](EvOp o, int n) {                               // comando de motor de longitud fija
        for (int i = 0; i < n && i < 16; i++) a[i] = arg(i);
        host.cmd(o, a, n); pc += 4u * n;
    };
    auto listCmd = [&](EvOp o) {                                    // [op, ids..., -1]: longitud n + 2
        int n = 0;
        while (mem.rd(pc + 4 + 4u * n) != 0xFFFFFFFFu) n++;
        for (int i = 0; i < n + 2 && i < 16; i++) a[i] = arg(i);
        host.cmd(o, a, n + 2); pc += 4u * (n + 2);
    };
    auto setPcFromTable = [&]() {
        pc = lookupPc(g.stage, g.event, g.phase);
        g.branchNo = 0; g.b500F07 = 0; intMode = 0;
    };
    switch (o) {
    // ---- enemigos (el motor devuelve el pc siguiente) ----
    case EvOp::ENTB:   pc = host.enemyEntry(pc, false); break;
    case EvOp::ENTB2:  pc = host.enemyEntry2(pc); break;
    case EvOp::ET1P:   pc = host.enemyEntry(pc, g.credits1P2P != 1); break;   // si no es "1 jugador" solo salta el registro
    case EvOp::ET2P:   pc = host.enemyEntry(pc, g.credits1P2P != 2); break;
    case EvOp::ENTBSK: pc = host.enemyEntry(pc, g.skipReq != 0); break;
    case EvOp::ZMAT: {                                              // lista de bytes (max. 3) terminada en -1
        int i = 0; uint32_t p = pc + 4;
        while (mem.rd(p) != 0xFFFFFFFFu) { if (i < 4) g.zmat[i] = (uint8_t)mem.rd(p); i++; p += 4; if (i >= 4) break; }
        g.zmatCount = i; pc = p + 4; break; }
    case EvOp::WLCTB: {                                             // tabla de camino hasta -1; guarda su direccion
        g.wlctbTable = pc + 4;
        do { pc += 4; } while (mem.rd(pc) != 0xFFFFFFFFu);
        pc += 4; break; }
    // ---- ramas / saltos ----
    case EvOp::BRANCHNO:  g.branchNo = (int8_t)arg(1); pc += 8; break;
    case EvOp::BRANCH:    pc = arg(1); break;
    case EvOp::MISSENMCLR: g.menmCount = 0; pc += 4; break;
    case EvOp::SWITCH_MENM: {                                       // [op, defecto, destino0, destino1, ..., -1]
        uint32_t def = arg(1); uint32_t p = pc + 8;
        uint32_t t = pickFromList(mem, p, g.menmCount, 0);
        pc = t ? t : def; break; }
    case EvOp::BRANCHEV: {                                          // [op, evento0, evento1, ..., -1]
        uint32_t p = pc + 4;
        uint32_t t = pickFromList(mem, p, g.branchNo, 0);
        g.phase = (int8_t)(t ? t : mem.rd(p));
        pc = lookupPc(g.stage, g.event, g.phase);
        g.branchNo = 0; g.b500F07 = 0; intMode = 0; break; }
    case EvOp::SETEVNO:   g.phase = (int8_t)arg(1); setPcFromTable(); break;
    case EvOp::BRANCHITEM: {                                        // [op, objeto, dir0, dir1, ..., -1]
        uint32_t item = arg(1); uint32_t p = pc + 8;
        int v = g.items[item & 63];
        pc = pickFromList(mem, p, v, 0); intMode = 0; break; }
    case EvOp::BRANCHADR: {                                         // [op, dir0, dir1, ..., -1]
        uint32_t p = pc + 4;
        pc = pickFromList(mem, p, g.branchNo, 0); intMode = 0; break; }
    case EvOp::BRANCHNOSET: fixed(o, 2); break;
    case EvOp::JOBFLAGSET: wait = (int8_t)arg(1); pc += 8; break;
    case EvOp::WAIT_INT_MODE: intMode = (int8_t)arg(1); pc += 8; break;
    case EvOp::SKIP:
        if (arg(1) != 0) { g.gp7294 = 0; g.gp729C = 1; } else { g.gp729C = 0; g.skipReq = 0; }
        pc += 8; break;
    case EvOp::NEXTEV: nextEv(); break;
    case EvOp::WAIT: wait = 1; break;
    // ---- camara ----
    case EvOp::CCTB: if (cam) pc = cam->entryEvent(pc, g.skipReq); else pc += 4u * (3 + ((int32_t)arg(2) >> 4)); break;
    case EvOp::CAMDR: if (cam) pc = cam->chgToDirMode(pc); else pc += 8; break;
    case EvOp::BGTB: pc = bg.appointBG(mem, pc); break;
    case EvOp::GETCPMODE: g.gp730C = (int)arg(1); pc += 8; break;
    case EvOp::FORCE_CAMPARAM: {                                    // [op, camara, clase, valor]
        uint32_t slot = arg(1) & 3, kind = arg(2), val = arg(3);
        if (cam) {
            if (kind == 0) cam->cam[slot].frame = (float)(int32_t)val;           // cvt.s.w
            else if (kind == 1) std::memcpy(&cam->cam[slot].speed, &val, 4);     // lwc1/swc1: bits del float
        }
        pc += 16; break; }
    // ---- esperas ----
    case EvOp::WAIT_CAMJOB:
        if (g.skipReq != 0) { wait = 0; pc += 4; }
        else if (cam && cam->chkRemainCamJob()) { wait = 1; }
        else if (g.canAdvance != 0) { wait = 0; pc += 4; }
        break;
    case EvOp::WAIT_CAMFRM: {                                       // [op, camara, fotograma (0 = hasta el final)]
        uint32_t slot = arg(1) & 3; int32_t fr = (int32_t)arg(2);
        if (g.skipReq != 0) { wait = 0; pc += 12; break; }
        if (wait != 1) { wait = 1; break; }
        if (g.canAdvance != 0) {
            int32_t tgt = fr != 0 ? fr : (cam ? cam->cam[slot].endFrame : 0);
            float cur = cam ? cam->cam[slot].frame : 0.f;
            if (!(cur < (float)(tgt - 1))) { wait = 0; pc += 12; }
        }
        break; }
    case EvOp::WAIT_FRM: {                                          // [op, fotogramas a 60 Hz]; cuenta en 50 Hz (PAL): n*50/60
        if (g.skipReq != 0) { wait = 0; pc += 8; break; }
        if (wait != 1) { frameCnt = ((int32_t)arg(1) * 50) / 60; wait = 1; break; }
        if (frameCnt >= 0) frameCnt--;
        if (frameCnt < 0 && g.canAdvance != 0) { frameCnt = -1; wait = 0; pc += 8; }
        break; }
    case EvOp::WAIT_ENNUM:
        g.enemyTarget = (int16_t)arg(1);
        if (wait != 0) {
            if (g.enemyTarget >= g.enemyCount && g.canAdvance != 0 && camReady()) { wait = 0; g.enemyTarget = 0; pc += 8; }
            break;
        }
        wait = 1; break;
    case EvOp::WAIT_ENALV:
        g.alvTarget = (int16_t)arg(1);
        if (wait != 0) {
            if (g.alvTarget >= g.alvCount && g.canAdvance != 0 && camReady()) { wait = 0; g.alvTarget = 0; pc += 8; }
            break;
        }
        wait = 1; break;
    case EvOp::WAIT_PEOPLE:
        g.peopleTarget = (int16_t)arg(1);
        if (wait != 0) {
            if (g.peopleTarget >= g.peopleCount && g.canAdvance != 0 && camReady()) { wait = 0; g.peopleTarget = 0; pc += 8; }
            break;
        }
        wait = 1; break;
    case EvOp::WAIT_ITEM:
        if (wait != 1) { wait = 1; break; }
        if (g.items[arg(1) & 63] != 0 && g.canAdvance != 0 && camReady()) { wait = 0; pc += 8; }
        break;
    case EvOp::WAIT_FLUSHDATA:
        if (host.dataRemaining()) { host.reqPauseAllData(); stopFlag = 1; }
        pc += 4; break;
    case EvOp::WAIT_FLUSHDATA_NOPAUSE:
        if (host.dataRemaining()) { stopFlag = 1; break; }
        pc += 4; break;
    case EvOp::WAIT_BUFFERRAM:
        if (wait != 0) { if (!host.dataRemaining()) { wait = 0; pc += 4; } break; }
        wait = 1; break;
    case EvOp::WAIT_LOAD_OVLMOD:
        if (host.ovlEntryStat((int)arg(1)) != 2) host.reqPauseOvlMod((int)arg(1));
        pc += 8; break;
    // ---- estado de partida ----
    case EvOp::ITEMSET: g.items[arg(1) & 63] = (int8_t)arg(2); pc += 12; break;
    case EvOp::GAMEFLAGSET: if (arg(1) != 0) g.gameFlags |= (int)arg(2); else g.gameFlags &= ~(int)arg(2); pc += 12; break;
    case EvOp::NOWLOADINGSET: g.nowLoading = (int)arg(1); pc += 8; break;
    case EvOp::SNOWDISP: g.snowDisp = (int)arg(1); pc += 8; break;
    case EvOp::BSFT: g.bsft = (int)arg(1); pc += 8; break;
    // ---- comandos de motor de longitud fija ----
    case EvOp::MLLIGHT: case EvOp::PLLIGHT: case EvOp::FLRTB: case EvOp::SKYKD: case EvOp::SKYDSP: case EvOp::CHGSM:
    case EvOp::ENTRYCPBK: case EvOp::FREESNDPK: case EvOp::BGMPAUSE: case EvOp::LOAD_OVLMOD: case EvOp::CHGADXPT:
    case EvOp::ENTRYBANK: case EvOp::FREEBANK: case EvOp::ENTRYTEX: case EvOp::FREETEX: case EvOp::ENTRYSPR: case EvOp::FREESPR:
    case EvOp::ENTRYLANGSPR: case EvOp::FREELANGSPR: case EvOp::ENTRYLANGBANK: case EvOp::FREELANGBANK:
    case EvOp::ENTRYMOT: case EvOp::FREEMOT: case EvOp::ENTRYENVTX: case EvOp::FREEENVTX: case EvOp::ENTRYOSK: case EvOp::FREEOSK:
    case EvOp::ENTRYOSKBK: case EvOp::FREEOSKBK: case EvOp::ENTRYCLVTX: case EvOp::FREECLVTX: case EvOp::ENTRYCOLLI: case EvOp::FREECOLLI:
    case EvOp::LIGHTIMMEXE: case EvOp::PUSHLIGHT: case EvOp::POPLIGHT: case EvOp::JIMAKU:
        fixed(o, 2); break;
    case EvOp::SKVOOFF: case EvOp::DBG_MALLOC_PRINT: case EvOp::DBG_ENTDATA_INFO: case EvOp::SCNINIT: case EvOp::ENTRYSTGDATA:
    case EvOp::FLUSHDATA: case EvOp::ENTRYSUBGIRL:
        fixed(o, 1); break;
    case EvOp::MOTSHDWMAP: fixed(o, 3); break;
    case EvOp::ENTRYSNDPK: case EvOp::BGMFADE: fixed(o, 4); break;
    case EvOp::SNDON: fixed(o, 5); break;
    case EvOp::OVLAP: case EvOp::MOTVOLUME: fixed(o, 5); break;
    case EvOp::BGMPLAY: case EvOp::VOICEPLAY: fixed(o, 6); break;
    case EvOp::SNDONM: fixed(o, 7); break;
    case EvOp::SNDONSK: if (g.skipReq == 0) fixed(o, 5); else pc += 20; break;
    case EvOp::SNDONMSK: if (g.skipReq == 0) fixed(o, 7); else pc += 28; break;
    case EvOp::ENTRYOSKNAME: case EvOp::FREEOSKNAME: case EvOp::ENTRYFANI: case EvOp::FREEFANI: case EvOp::ENTRYFAMOT: case EvOp::FREEFAMOT:
        listCmd(o); break;
    case EvOp::FOGCTRL: case EvOp::LIGHTCTRL: case EvOp::DOFCTRL: case EvOp::BLURCTRL: pc = host.varCmd(o, pc); break;
    case EvOp::VIBRATION: {                                         // [op, clase, ...]
        uint32_t k = arg(1);
        if (k == 1) { fixed(o, 2); }
        else if (k == 0) { if (g.skipReq == 0) { for (int i = 0; i < 4; i++) a[i] = arg(i); host.cmd(o, a, 4); } pc += 16; }
        else if (k == 2) { if (g.skipReq == 0) { for (int i = 0; i < 5; i++) a[i] = arg(i); host.cmd(o, a, 5); } pc += 20; }
        break; }                                                    // otra clase: el original no avanza el pc
    case EvOp::SCEFFINOUT: break;                                   // funcion vacia en el original (no avanza el pc)
    default:
        host.onUnknownOpcode(op, pc);
        wait = 1; break;                                            // el original saltaria a un ptmf invalido; aqui se cede el fotograma
    }
}

void EventCtrl::nextEv() {                                          // NEXTEV (0x1E3BF0)
    g.phase = (int8_t)(g.phase + 1);
    if (lookupPc(g.stage, g.event, g.phase) == 0xFFFFFFFFu) {       // se acabo la lista de fases del evento
        const EvTransTable& t = kEvTransTables[g.stage];
        const EvTrans& r = t.rows[g.event];
        switch (r.type) {
        case 0: g.event = (int8_t)r.a; g.phase = 1; break;
        case 1: {                                                   // *(s16*)(fila + 2 + 2*branchNo): puede leer filas contiguas
            const int16_t* base = &r.a;
            long idx = (base - &t.rows[0].a) + g.branchNo;
            g.event = (int8_t)((idx >= 0 && idx < 4L * t.count) ? base[g.branchNo] : -1);
            g.phase = 1; break; }
        case 2: g.event = (int8_t)(g.event + 1); g.phase = 1; break;
        case 3:                                                     // fase superada: el script pasa a EVTPAL[8][0][0]
            host.stageClear();
            g.event = (int8_t)r.a; g.phase = 0;
            pc = mem.rd(mem.rd(mem.rd(evtpalBase + 0x20)));
            if (g.stage == 5) host.addHitPerBonus();
            return;                                                 // no reinicia branchNo/intMode
        default: g.phase = 1; break;
        }
    }
    if (g.stage == 5 && g.event == 3) g.gameFlags &= ~2;
    pc = lookupPc(g.stage, g.event, g.phase);
    if (g.b500F05 == 0) g.branchNo = 0;
    g.b500F07 = 0; intMode = 0;
}
}

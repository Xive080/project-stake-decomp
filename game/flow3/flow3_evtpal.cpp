// Ver flow3_evtpal.h. Cada rutina cita la original (ELF, direccion).
#include "flow3_evtpal.h"
namespace vn {

bool DefaultTrans::row(int stage, int event, int16_t out[4]) const {
    if (stage < 0 || stage >= 12 || event < 0) return false;
    const EvTransTable& t = kEvTransTables[stage];
    if (!t.rows || event >= t.count) return false;
    out[0] = t.rows[event].type; out[1] = t.rows[event].a; out[2] = t.rows[event].b; out[3] = t.rows[event].c;
    return true;
}

static const uint32_t kEnd = 0xFFFFFFFFu;

uint32_t EvtVm::lookupPc(int stage, int event, int phase) const {   // tres niveles de punteros (como NEXTEV/BRANCHEV/__ct)
    uint32_t l1 = mem.rd(evtpalBase + 4u * (uint32_t)(int32_t)stage);
    uint32_t l2 = mem.rd(l1 + 4u * (uint32_t)(int32_t)event);
    return mem.rd(l2 + 4u * (uint32_t)(int32_t)phase);
}

// __ct__13EventCtrlTaskFv (0x1E29F0): fase = (gameState != 9 && evento != 0) ? 1 : 0; pc = EVTPAL[etapa][evento][fase]
void EvtVm::construct() {
    g.phase = (g.gameState != 9 && g.event != 0) ? 1 : 0;
    pc = lookupPc(g.stage, g.event, g.phase);
    g.savedPc = pc;
    lastPc = 0; wait = 0; intMode = 0;
    g.skipPending = 0; g.skipReq = 0; g.skipEnabled = 0;
    g.gp733C = (g.stage == 6) ? 0x1DC0 : 0x2254;
    fault = EvtFault::None;
}

void EvtVm::jumpToScript(int stage, int event, int phase) {
    g.stage = (int8_t)stage; g.event = (int8_t)event; g.phase = (int8_t)phase;
    g.savedPc = lookupPc(stage, event, phase);
}

// Primer tramo de EventCtrl_000 (0x1E2B80): avanza si algun jugador esta vivo en modo 5 (o es demo)
bool EvtVm::computeCanAdvance() {
    bool alive = host.playerLife(0) > 0 && host.playerMode(0) == 5;
    if (!alive) alive = host.playerLife(1) > 0 && host.playerMode(1) == 5;
    if (!alive) alive = host.demoPlay();
    return alive;
}

// EventCtrl_000 (0x1E2B80): ejecuta instrucciones hasta que una cede (wait != 0) o pide parar (stopFlag)
void EvtVm::frame() {
    g.canAdvance = computeCanAdvance() ? 1 : 0;
    stepsLastFrame = 0;
    if (host.pauseLevel() >= 2) return;
    pc = g.savedPc; stopFlag = 0;
    do {
        op = mem.rd(pc);
        if (pc != lastPc) lastPc = pc;
        if (trace) trace->push_back(pc);
        step();
        g.savedPc = pc;
        if (++stepsLastFrame >= (uint32_t)maxStepsPerFrame) { fault = EvtFault::Hang; faultPc = pc; return; }
    } while (wait == 0 && stopFlag == 0);
}

bool EvtVm::camReady() {            // trozo comun de WAIT_ENNUM/ENALV/PEOPLE/ITEM: cam0.d6 = 1; (cam0.d5 != 0 || intMode != 0)
    host.camSetFlagD6();
    return host.camFlagD5(0) || intMode != 0;
}

void EvtVm::emit(int words, bool skipped, bool cond) {
    EvtCmd c; c.opcode = op; c.op = evOpFromOpcodeV2(op); c.pc = pc; c.skipped = skipped; c.cond = cond;
    for (int i = 0; i < words && i < 64; i++) c.w.push_back(arg(i));
    host.command(c);
}
void EvtVm::fixedLen(bool skipped) {      // instruccion de motor: el registro mide lo que dice evInsnLength
    EvLen l = evInsnLength(mem, pc);
    if (l.words <= 0) { fault = EvtFault::UnknownOpcode; faultPc = pc; wait = 1; return; }
    emit(l.words, skipped); pc += 4u * (uint32_t)l.words;
}

void EvtVm::setPcFromTable() {            // cola comun de BRANCHEV/SETEVNO
    pc = lookupPc(g.stage, g.event, g.phase);
    g.branchNo = 0; g.b500F07 = 0; intMode = 0;
}

// Bucle de SWITCH_MENM/BRANCHEV/BRANCHITEM/BRANCHADR: recorre la lista desde `first` (hasta la palabra -1) y devuelve la entrada de indice n.
// Como el original: la entrada elegida solo cuenta si no es -1; si la lista esta vacia (la primera palabra ya es -1) no hay eleccion.
uint32_t EvtVm::pickFromList(uint32_t first, int n, bool& found) const {
    found = false; uint32_t p = first; int idx = -1;
    for (int guard = 0; guard < 4096; guard++) {
        uint32_t t = mem.rd(p); idx++;
        if (t != kEnd && n == idx) { found = true; return t; }
        p += 4;
        if (mem.rd(p) == kEnd) break;
    }
    return 0;
}

void EvtVm::step() {
    EvOp o = evOpFromOpcodeV2(op);
    switch (o) {
    // ---- entrada de enemigos: ENTB 0x1E2D10, ENTB2 0x1E2D40, ET1P 0x1E2D70, ET2P 0x1E2DD0, ENTBSK 0x1E2E30 ----
    case EvOp::ENTB: case EvOp::ENTB2: {
        EvLen l = evInsnLength(mem, pc);
        if (l.words <= 0) { fault = EvtFault::UnknownOpcode; faultPc = pc; wait = 1; break; }
        emit(l.words, false, true); pc += 4u * (uint32_t)l.words; break; }
    case EvOp::ET1P: case EvOp::ET2P: case EvOp::ENTBSK: {
        // ET1P: si credits == 1 -> func_935380 (entra) si no func_9356C0 (solo recorre el registro); ET2P igual con 2; ENTBSK: skipReq != 0 -> solo recorre
        bool enter = (o == EvOp::ET1P) ? (g.credits == 1) : (o == EvOp::ET2P) ? (g.credits == 2) : (g.skipReq == 0);
        EvLen l = evInsnLength(mem, pc);
        if (l.words <= 0) { fault = EvtFault::UnknownOpcode; faultPc = pc; wait = 1; break; }
        emit(l.words, !enter, enter); pc += 4u * (uint32_t)l.words; break; }
    // ---- listas / datos ----
    case EvOp::ZMAT: {                           // 0x1E4920: bytes bajos de las palabras hasta -1 en gp-0x7364; al llegar a 4 el original hace assert (break 0)
        int i = 0; pc += 4;
        while (mem.rd(pc) != kEnd) {
            g.zmat[i & 3] = (uint8_t)mem.rd(pc); i++;
            if (i >= 4) { fault = EvtFault::ZmatOverflow; faultPc = pc; wait = 1; return; }
            pc += 4;
        }
        pc += 4; break; }
    case EvOp::WLCTB:                            // 0x1E2E80: guarda pc+4 en gp-0x7304 y salta hasta despues del -1
        g.wlctbTable = pc + 4;
        do { pc += 4; } while (mem.rd(pc) != kEnd);
        pc += 4; break;
    // ---- saltos ----
    case EvOp::BRANCHNO:   g.branchNo = (int8_t)arg(1); pc += 8; break;                   // 0x1E49D0
    case EvOp::BRANCH:     pc = arg(1); break;                                            // 0x1E49F0
    case EvOp::MISSENMCLR: g.menmCount = 0; pc += 4; break;                               // 0x1E4A10
    case EvOp::SWITCH_MENM: {                                                             // 0x1E4A30: [op, defecto, destino0, ..., -1]
        uint32_t def = arg(1); bool f;
        uint32_t t = pickFromList(pc + 8, g.menmCount, f);
        pc = t ? t : def; break; }
    case EvOp::BRANCHEV: {                                                                // 0x1E4AB0: [op, fase0, fase1, ..., -1]
        uint32_t first = arg(1); bool f;
        uint32_t t = pickFromList(pc + 4, g.branchNo, f);
        g.phase = (int8_t)(t ? t : first);        // defecto = PRIMERA entrada (el interprete de game/flow usaba la palabra tras la lista)
        setPcFromTable(); break; }
    case EvOp::SETEVNO:    g.phase = (int8_t)arg(1); setPcFromTable(); break;             // 0x1E4B80
    case EvOp::BRANCHITEM: {                                                              // 0x1E4BE0 (opcode 112): [op, objeto, dir0, ..., -1]
        uint32_t item = arg(1); bool f;
        int v = g.items[item & 0xFF];
        pc = pickFromList(pc + 8, v, f); intMode = 0;
        if (pc == 0) { fault = EvtFault::NullBranch; faultPc = lastPc; wait = 1; }
        break; }
    case EvOp::BRANCHADR: {                                                               // 0x1E4C70: [op, dir0, ..., -1]
        bool f;
        pc = pickFromList(pc + 4, g.branchNo, f); intMode = 0;
        if (pc == 0) { fault = EvtFault::NullBranch; faultPc = lastPc; wait = 1; }
        break; }
    case EvOp::BRANCHNOSET: emit(2); pc += 8; break;                                      // 0x1E4D40: func_97ED80(gp-0x71F0, arg) (motor)
    case EvOp::JOBFLAGSET: wait = (int8_t)arg(1); pc += 8; break;                         // 0x1E4DC0 (escribe +4 del objeto)
    case EvOp::WAIT_INT_MODE: intMode = (int8_t)arg(1); pc += 8; break;                   // 0x1E4E00
    case EvOp::SKIP:                                                                      // 0x1E2FE0
        if (arg(1) != 0) { g.skipPending = 0; g.skipEnabled = 1; } else { g.skipEnabled = 0; g.skipReq = 0; }
        pc += 8; break;
    case EvOp::NEXTEV: nextEv(); break;
    case EvOp::WAIT: wait = 1; break;                                                     // 0x1E3BE0
    case EvOp::SCEFFINOUT:                        // 0x1E49C0 es `jr ra`: no avanza el pc ni cede -> bucle infinito en la consola
        fault = EvtFault::SceffinoutLoop; faultPc = pc; wait = 1; break;
    // ---- camara / fondo ----
    case EvOp::CCTB: {                            // 0x1E2F00 -> CameraTask::entryEvent(pc, skipReq): registro de 3 + (tipo >> 4) palabras
        EvLen l = evInsnLength(mem, pc);
        emit(l.words, g.skipReq != 0); pc += 4u * (uint32_t)l.words; break; }
    case EvOp::CAMDR: emit(2); pc += 8; break;    // 0x1E2F30 -> chgToDirMode
    case EvOp::BGTB: host.bgAppoint((int16_t)arg(1)); emit(2); pc += 8; break;           // 0x1E2F... -> BGCtrl::appointBG
    case EvOp::GETCPMODE: g.gp730C = (int32_t)arg(1); pc += 8; break;
    case EvOp::FORCE_CAMPARAM: {                  // 0x1E4DA0..: [op, camara, clase, valor] clase 0 = fotograma (int->float), 1 = velocidad (bits float)
        uint32_t cls = arg(2);
        if (cls == 0 || cls == 1) host.camForce((int)(arg(1)), (int)cls, arg(3));
        emit(4); pc += 16; break; }
    // ---- esperas ----
    case EvOp::WAIT_CAMJOB:                       // 0x1E3460
        if (g.skipReq != 0) { wait = 0; pc += 4; }
        else if (host.camJobPending()) wait = 1;
        else if (g.canAdvance != 0) { wait = 0; pc += 4; }
        break;
    case EvOp::WAIT_CAMFRM: {                     // 0x1E34F0: [op, camara, fotograma (0 = hasta el final)]
        uint32_t slot = arg(1); int32_t fr = (int32_t)arg(2);
        if (g.skipReq != 0) { wait = 0; pc += 12; break; }
        if (wait != 1) { wait = 1; break; }
        if (g.canAdvance != 0) {
            int32_t tgt = (fr != 0 ? fr : host.camEndFrame((int)slot)) - 1;
            if (!(host.camFrame((int)slot) < (float)tgt)) { wait = 0; pc += 12; }
        }
        break; }
    case EvOp::WAIT_FRM: {                        // 0x1E35D0: la cuenta es n*50/60 (60 Hz -> 50 Hz PAL), division con signo hacia cero
        if (g.skipReq != 0) { wait = 0; pc += 8; break; }
        if (wait != 1) { frameCnt = (int32_t)((int64_t)(int32_t)arg(1) * 50 / 60); wait = 1; break; }
        if (frameCnt >= 0) frameCnt--;
        if (frameCnt < 0 && g.canAdvance != 0) { frameCnt = -1; wait = 0; pc += 8; }
        break; }
    case EvOp::WAIT_ENNUM:                        // 0x1E36A0
        g.enemyTarget = (int16_t)arg(1);
        if (wait == 0) { wait = 1; break; }
        if (!(g.enemyTarget < g.enemyCount) && g.canAdvance != 0 && camReady()) { wait = 0; g.enemyTarget = 0; pc += 8; }
        break;
    case EvOp::WAIT_ENALV:                        // 0x1E3730
        g.alvTarget = (int16_t)arg(1);
        if (wait == 0) { wait = 1; break; }
        if (!(g.alvTarget < g.alvCount) && g.canAdvance != 0 && camReady()) { wait = 0; g.alvTarget = 0; pc += 8; }
        break;
    case EvOp::WAIT_PEOPLE:                       // 0x1E37C0
        g.peopleTarget = (int16_t)arg(1);
        if (wait == 0) { wait = 1; break; }
        if (!(g.peopleTarget < g.peopleCount) && g.canAdvance != 0 && camReady()) { wait = 0; g.peopleTarget = 0; pc += 8; }
        break;
    case EvOp::WAIT_ITEM:                         // 0x1E3850
        if (wait != 1) { wait = 1; break; }
        if (g.items[arg(1) & 0xFF] != 0 && g.canAdvance != 0 && camReady()) { wait = 0; pc += 8; }
        break;
    case EvOp::WAIT_FLUSHDATA:                    // 0x1E38E0
        if (host.dataRemaining()) { host.reqPauseAllData(); stopFlag = 1; }
        pc += 4; break;
    case EvOp::WAIT_FLUSHDATA_NOPAUSE:            // 0x1E3940
        if (host.dataRemaining()) stopFlag = 1; else pc += 4;
        break;
    case EvOp::WAIT_BUFFERRAM:                    // 0x1E3990
        if (wait == 0) { wait = 1; break; }
        if (!host.dataRemaining()) { wait = 0; pc += 4; }
        break;
    case EvOp::WAIT_LOAD_OVLMOD:                  // 0x1E3A30
        if (host.ovlEntryStat((int)arg(1)) != 2) host.reqPauseOvlMod((int)arg(1));
        pc += 8; break;
    // ---- estado de partida ----
    case EvOp::ITEMSET: g.items[arg(1) & 0xFF] = (int8_t)arg(2); pc += 12; break;         // 0x1E3B00
    case EvOp::GAMEFLAGSET:                                                               // 0x1E4FD0 (opcode 114)
        if (arg(1) != 0) g.gameFlags |= (int32_t)arg(2); else g.gameFlags &= ~(int32_t)arg(2);
        pc += 12; break;
    case EvOp::NOWLOADINGSET: g.nowLoading = (int16_t)arg(1); pc += 8; break;
    case EvOp::SNOWDISP:      g.snowDisp = (int32_t)arg(1); pc += 8; break;
    case EvOp::BSFT:          g.bsft = (int32_t)arg(1); pc += 8; break;
    case EvOp::ENTRYSUBGIRL: {                    // 0x1E4F10 (opcode 113): carga el banco 0xAD / OSK 0x21 si gameState==10, modo 1 y hay un '.' (0x2E) en el nombre
        bool cond = false;
        if (g.gameState == 10 && g.playMode == 1)
            for (int r = 0; r < 2; r++) for (int k = 2; k < 4; k++) if (g.subGirlName[r * 4 + k] == 0x2E) cond = true;
        emit(1, false, cond); pc += 4; break; }
    case EvOp::VIBRATION: {                       // 0x1E4E50: clase 0 = start (4 palabras), 1 = stop (2), 2 = PadActTwin (5); otra clase: no avanza (se cuelga)
        uint32_t k = arg(1);
        if (k == 0) { emit(4, g.skipReq != 0); pc += 16; }
        else if (k == 1) { emit(2); pc += 8; }
        else if (k == 2) { emit(5, g.skipReq != 0); pc += 20; }
        else { fault = EvtFault::UnknownOpcode; faultPc = pc; wait = 1; }
        break; }
    case EvOp::SNDONSK: case EvOp::SNDONMSK:      // sonidos con skip: si skipReq != 0 solo avanzan
        { EvLen l = evInsnLength(mem, pc); emit(l.words, g.skipReq != 0); pc += 4u * (uint32_t)l.words; break; }
    case EvOp::Unknown:
        fault = EvtFault::UnknownOpcode; faultPc = pc; wait = 1; break;       // ptmf nulo: el original llamaria a NULL
    default: fixedLen(); break;                   // resto: orden al motor de longitud fija o medida por evInsnLength
    }
}

// NEXTEV (0x1E3BF0). Ver MANIFEST: la logica coincide con game/flow/eventctrl.cpp salvo que aqui las tablas entran por EvtTransSource.
void EvtVm::nextEv() {
    g.phase = (int8_t)(g.phase + 1);
    if (lookupPc(g.stage, g.event, g.phase) == kEnd) {        // fin de la lista de fases del evento
        int16_t r[4] = {0, 0, -1, -1};
        bool have = trans.row(g.stage, g.event, r);
        int type = have ? r[0] : -1;
        switch (type) {
        case 0: g.event = (int8_t)r[1]; g.phase = 1; break;                                     // evento fijo (byte bajo de a)
        case 1: {                                                                                // lista indexada por branchNo
            // El original lee el byte bajo de la media palabra en fila + 2 + 2*branchNo: con branchNo >= 3 invade las filas siguientes.
            int off = 2 + 2 * (int)g.branchNo;
            int rowAdd = off >= 0 ? off / 8 : -((7 - off) / 8), fld = ((off % 8) + 8) % 8 / 2;
            int16_t r2[4] = {0, 0, -1, -1}; int16_t v = -1;
            if (trans.row(g.stage, g.event + rowAdd, r2)) v = r2[fld];
            g.event = (int8_t)v; g.phase = 1; break; }
        case 2: g.event = (int8_t)(g.event + 1); g.phase = 1; break;
        case 3:                                                                                  // EventStageClear
            host.stageClear(); evtStageClear(g);
            g.event = (int8_t)(have ? r[1] : 0); g.phase = 0;
            pc = mem.rd(mem.rd(mem.rd(evtpalBase + 0x20)));                                      // EVTPAL[8][0][0]
            if (g.stage == 5) host.addHitPerBonus();
            return;                                                                              // no limpia branchNo / intMode
        default: g.phase = 1; break;                                                             // tipo desconocido: solo fase 1
        }
    }
    if (g.stage == 5 && g.event == 3) g.gameFlags &= ~2;
    pc = lookupPc(g.stage, g.event, g.phase);
    if (g.b500F05 == 0) g.branchNo = 0;
    g.b500F07 = 0; intMode = 0;
}

// InitStage (ELF 0x1E28C0). Los contadores de las entidades (gp-0x720C/-0x7210/-0x7214, -0x7368/-0x7370, -0x736C/-0x7374), los 256 bytes de
// objetos y las banderas gp-0x73EC / gp-0x73AC pasan a cero; el resto de la rutina (contadores +0x60/+0x64/+0x68 de los dos jugadores,
// func_9356F0/func_9357F0 de EnemyMgr, InitDispSilver, gp-0x73E0/-0x7414/-0x7420/-0x741C/-0x73DC) es del motor y lo hace el llamante.
void evtInitStage(EvtGlobals& g) {
    g.peopleTarget = 0; g.alvTarget = 0; g.enemyTarget = 0;
    g.peopleCount = 0; g.alvCount = 0; g.enemyCount = 0;
    for (auto& i : g.items) i = 0;
    g.stageClearFlag = 0;
    g.bsft = 0;
}
// EventStageClear (ELF 0x1E2CC0)
void evtStageClear(EvtGlobals& g) {
    g.stageClearFlag = 1; g.bsft = 0;
    if (g.gameState == 10 && g.mode73F0 != 8) g.mode73F0 = 5;
}
EvCancelAct evCancelCtrl(EvtGlobals& g, EvCancelState& s) {
    if (g.skipEnabled == 0) return EvCancelAct::DeleteSelf;
    if (s.buttonFlag != 0) { g.skipReq = 1; g.skipPending = 1; s.buttonFlag = 0; return EvCancelAct::DoSkip; }
    return EvCancelAct::Nothing;
}
void evCancelSkip(EvtGlobals& g) { g.skipPending = 0; }
}

// Boss3Task: interprete de secuencias y auxiliares. Reescrito a mano desde asm/bs3mod/bs3mod_text.s (ver boss3_seq.h).
#include "boss3_seq.h"
#include <cstring>
namespace vn { namespace boss {

static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

void Boss3SeqLogic::allowCameraStart(Bs3SysGlobals& sg) { sg.cameraStartAllowed = 1; }    // AllowCameraStart__9Boss3TaskFv 0x9C9310
void Boss3SeqLogic::forbidCameraStart(Bs3SysGlobals& sg) { sg.cameraStartAllowed = 0; }   // ForbidCameraStart__9Boss3TaskFv 0x9C9320

// ExecCmd__9Boss3TaskFv (0x9C6B50): mientras +0x1DB0 == -1 lee un s16 de +0x1DB4 (avanzando 1 elemento); si vale 1 lo guarda en +0x1DB0.
void Boss3SeqLogic::execCmd(Bs3Ext& x) {
    while (x.cmdRun == -1) {
        if (x.cmdEnd && x.cmdPc >= x.cmdEnd) return;   // guarda propia, el original no la tiene
        int16_t v = *x.cmdPc++;
        if (v == 1) x.cmdRun = v;
    }
}

// GetPathEndFrame__9Boss3TaskFv (0x9C9800)
int Boss3SeqLogic::getPathEndFrame(Bs3Ext& x, Bs3SeqHost& h) {
    x.pathEnd = (x.pathId >= 0) ? h.pathEndFrame(x.pathId) : 1;
    return x.pathEnd;
}
void Boss3SeqLogic::fireAlphaDirectSet(Bs3Ext& x, float v) { x.fireCur = v; x.fireTarget = v; }   // 0x9C9EB0
void Boss3SeqLogic::fireAlphaSupSet(Bs3Ext& x, float v) { x.fireTarget = v; }                      // 0x9C9EC0

// FireAlphaControl__9Boss3TaskFv (0x9C9CF0): suaviza fireCur (+0x1EE0) hacia el objetivo (+0x1EDC) y deriva fireOut (+0x1EE4).
// Solo actua con bit 0x200000 o 0x400000 de +0x1DA4.
void Boss3SeqLogic::fireAlphaControl(const Bs3State& s, Bs3Ext& x) {
    uint32_t st = s.st;
    if (!(st & 0x200000) && !(st & 0x400000)) return;
    float target = x.fireTarget;
    if (st & 0x4000) target = bitsF(0x3CA3D70A);          // 0.02
    else if (st & 0x80) target = bitsF(0x3D4CCCCD);       // 0.05
    float maxCur, maxOut, rate;
    if (st & 0x400000) {
        maxCur = maxOut = bitsF(0x3FE66666);              // 1.8
        rate = (x.fireCur <= target) ? bitsF(0x3CA3D70A) : bitsF(0x3E4CCCCD);   // 0.02 / 0.2
    } else {
        maxCur = bitsF(0x3F000000);                       // 0.5
        maxOut = bitsF(0x3F4CCCCD);                       // 0.8
        rate = bitsF(0x3E19999A);                         // 0.15
    }
    if (target == 0.0f && x.fireCur < bitsF(0x3C23D70A)) x.fireCur = 0.0f;   // 0.01
    else if (target != x.fireCur) x.fireCur = x.fireCur + rate * (target - x.fireCur);
    if (x.fireCur < 0.0f) x.fireCur = 0.0f;
    else if (!(x.fireCur <= maxCur)) x.fireCur = maxCur;
    x.fireOut = 2.0f * x.fireCur;
    if (!(x.fireOut <= maxOut)) x.fireOut = maxOut;
}

// EnableCollision__9Boss3TaskFv (0x9C9330) / DisableCollision (0x9C9390): +0x0C bit 0x8000; si +0x1DA4 no tiene 0x100, bit 0x40 de +0x1A8 de las 4 partes.
void Boss3SeqLogic::enableCollision(Bs3State& s, Bs3Ext& x) {
    s.enemyFlags &= ~0x8000u;
    if (!(s.st & 0x100)) for (int i = 0; i < 4; i++) if (x.partPresent[i]) x.partFlags[i] &= ~0x40u;
}
void Boss3SeqLogic::disableCollision(Bs3State& s, Bs3Ext& x) {
    s.enemyFlags |= 0x8000u;
    if (!(s.st & 0x100)) for (int i = 0; i < 4; i++) if (x.partPresent[i]) x.partFlags[i] |= 0x40u;
}

// LoopSoundOff__9Boss3TaskFv (0x9CA020): si +0x1F04 bit 0: sonido 0x17000C parado y bit a 0.
void Boss3SeqLogic::loopSoundOff(Bs3Ext& x, Bs3SeqHost& h) {
    if (x.f1F04 & 1) { h.loopSoundOffEvent(); x.f1F04 &= ~1u; }
}

// FinishTask0__9Boss3TaskFv (0x9C9220)
void Boss3SeqLogic::finishTask0(Bs3State& s, Bs3Ext& x, Bs3SeqHost& h, Boss3Engine& g) {
    if (s.form == 1) g.cameraEvent(0, 1);
    loopSoundOff(x, h);
    h.releaseTrail();
    for (int i = 0; i < 2; i++) if (x.fx1EA0[i]) { h.freeObject(x.fx1EA0[i]); x.fx1EA0[i] = nullptr; }
}
void Boss3SeqLogic::finishTask(Bs3SeqHost& h) { h.taskEnd(); }   // FinishTask__9Boss3TaskFv (0x9C92E0)

// SetHeight__9Boss3TaskFv (0x9C93F0): sonda el suelo 6 unidades por encima de la posicion.
void Boss3SeqLogic::setHeight(Bs3State& s, Bs3Ext& x, Bs3SeqHost& h) {
    float gnd = h.groundHeight(s.pos.x, s.pos.y + 6.0f, s.pos.z);
    if (s.pos.y <= gnd) { if (!(s.flags2 & 0x40)) s.pos.y = gnd; }
    else if (!(s.flags2 & 0x40)) { s.flags2 |= 0x40; x.b1EB6 = 0; }
}

int Boss3SeqLogic::actExecSeqCmd(Bs3State& s, Bs3Ext& x, Bs3Env& e, Bs3SysGlobals& sg, Boss3Engine& g, Bs3SeqHost& h) { execSeqCmd(s, x, e, sg, g, h); return 0; }
int Boss3SeqLogic::actNop() { return 1; }
int Boss3SeqLogic::actDead() { return 0; }

// actDelete__9Boss3TaskFv (0x9C7FB0): paso 0: marca la tarea como muerta/borrada y descuenta enemigos; paso 1: FinishTask. Devuelve 0.
int Boss3SeqLogic::actDelete(Bs3State& s, Bs3Ext& x, Bs3Env& e, Bs3SeqHost& h) {
    if (x.b1E11 == 0) {
        s.enemyFlags |= 0x04000000;
        Boss3Logic::atcPlFlgClear(s, e);
        s.enemyFlags |= 0x4000;
        s.flags2 |= 0x20;
        // +0x18 = 0 (campo de la tarea base, no modelado)
        Boss3Logic::decNowEnemyAlvNumChk(s, e);
        Boss3Logic::decNowEnemyNumChk(s, e);
        x.b1E11++;
    } else if (x.b1E11 == 1) {
        finishTask(h);
    }
    return 0;
}

// Rotaciones en el orden de +0x1CC8 (ops 18/19). X = +0x24, Y = +0x28, Z = +0x2C.
static void applyEuler(const Bs3State& s, int order, Bs3MatrixOps& m) {
    int ax = s.pos.ax, ay = s.pos.ay, az = s.pos.az;
    switch (order) {
    case 0: m.rotX(ax); m.rotY(ay); m.rotZ(az); break;
    case 1: m.rotX(ax); m.rotZ(az); m.rotY(ay); break;
    case 2: m.rotY(ay); m.rotX(ax); m.rotZ(az); break;
    case 3: m.rotY(ay); m.rotZ(az); m.rotX(ax); break;
    case 4: m.rotZ(az); m.rotX(ax); m.rotY(ay); break;
    default: m.rotZ(az); m.rotY(ay); m.rotX(ax); break;
    }
}

// ExecSeqCmd__9Boss3TaskFv (0x9C8050). pc = +0x1DB8 (puntero a s16: [0] = operacion, [1..] = argumentos). Tabla de saltos jtbl_009D1DE0 (59 entradas,
// extraida del BIN). Bucle: sigue ejecutando operaciones hasta que una pide ceder (s0 = 1). El fotograma de camara se lee una sola vez al entrar.
// Cada `pc += N` del asm es en bytes; aqui en elementos s16 (N/2).
int Boss3SeqLogic::execSeqCmd(Bs3State& s, Bs3Ext& x, Bs3Env& e, Bs3SysGlobals& sg, Boss3Engine& g, Bs3SeqHost& h) {
    const float camF = h.camFrame();
    bool yield = false;
    while (!yield) {
        const int16_t* pc = s.seqPc;
        const int op = pc[0];
        auto arg = [&](int i) { return (int)pc[i]; };
        auto adv = [&](int n) { s.seqPc += n; };
        switch (op) {
        case 0:  // 0x9C80C8: espera a que el fotograma de camara llegue a arg
            if (camF < (float)arg(1)) yield = true; else adv(2); break;
        case 1:  // 0x9C8104: igual, pero con camara aun por llegar: AllowCameraStart; si ya llego: ForbidCameraStart
            if (camF < (float)arg(1)) { allowCameraStart(sg); yield = true; } else { forbidCameraStart(sg); adv(2); } break;
        case 2:  // 0x9C81DC: espera a que el fotograma del movimiento (+0x3C) llegue a arg
            if (s.frame < (float)arg(1)) yield = true; else adv(2); break;
        case 3:  // 0x9C821C: espera al final del movimiento (longitud en la tabla s16 de 0x1FB73E4)
            if (s.frame < (float)g.motionLength(s.motion)) yield = true; else adv(1); break;
        case 4:  // 0x9C8270: espera a arg * longitud del movimiento (producto entero)
            if (s.frame < (float)(arg(1) * g.motionLength(s.motion))) yield = true; else adv(2); break;
        case 5:  // 0x9C82CC: espera a que acabe la ventana de ataque (+0x0C bit 0x10000000)
            if (s.enemyFlags & 0x10000000u) yield = true; else adv(1); break;
        case 7:  // 0x9C8150: byte 0x500F07 = 1
            sg.cameraStartAllowed = 1; adv(1); break;
        case 8:  // 0x9C816C: espera a que 0x500F07 valga 0
            if (sg.cameraStartAllowed != 0) yield = true; else adv(1); break;
        case 9:  s.st |= 0x80000; adv(1); break;                       // 0x9C8198
        case 10: s.st &= ~0x80000u; adv(1); break;                     // 0x9C81B8
        case 11: Boss3Logic::motSupSet2(s, g, arg(1), arg(2), arg(3)); adv(4); break;   // 0x9C8304
        case 12: { // 0x9C8890: movimiento del ataque arg (kBs3Attack[arg].motion), sin mezcla
            Boss3Logic::motSupSet2(s, g, kBs3Attack[arg(1)].motion, 0, 0); adv(2); break; }
        case 13: // 0x9C8328: velocidad = 1.2 * (arg / 256)  (constantes 0x3F99999A y 0x3B800000)
            s.speed = bitsF(0x3F99999A) * (bitsF(0x3B800000) * (float)arg(1)); adv(2); break;
        case 14: s.pos.ay = arg(1); adv(2); break;                     // 0x9C8364: angulo +0x28 = arg
        case 15: s.pos.ay += arg(1); x.lastTurn = arg(1); adv(2); break;   // 0x9C8380
        case 16: { // 0x9C83B0: gira la posicion alrededor del punto (3268.7, 0, -8.03) en el eje Y por arg, y suma arg al angulo +0x28
            int a = arg(1);
            s.pos.ay += a;
            const float cx = bitsF(0x454C4BD6), cz = bitsF(0xC1006FD2);
            float tmp[3] = {s.pos.x - cx, s.pos.y, s.pos.z - cz}, out[3];
            h.push(); h.translate(cx, 0.0f, cz); h.rotY(a); h.transformPoint(tmp, out);
            s.pos.x = out[0]; s.pos.y = out[1]; s.pos.z = out[2];
            h.pop(); adv(1); break; }
        case 17: // 0x9C8474: posicion y angulos a cero
            s.pos.az = 0; s.pos.ay = 0; s.pos.ax = 0; s.pos.z = 0; s.pos.y = 0; s.pos.x = 0; adv(1); break;
        case 18: case 20: case 19: case 21: {
            if (op == 18 || op == 19) {   // 0x9C84B4 / 0x9C86A4: aplica T(pos)*R*S a +-off; 18 -> x,z; 19 -> x,y,z
                float in[3], out[3];
                const float sg3 = (op == 18) ? -1.0f : 1.0f;
                h.push(); h.translate(s.pos.x, s.pos.y, s.pos.z);
                applyEuler(s, x.eulerOrder, h);
                h.scale(x.scale, x.scale, x.scale);
                in[0] = sg3 * x.off[0]; in[1] = sg3 * x.off[1]; in[2] = sg3 * x.off[2];
                h.transformPoint(in, out);
                s.pos.x = out[0]; s.pos.z = out[2]; if (op == 19) s.pos.y = out[1];
                h.pop();
            }
            if (op == 18 || op == 20) x.bodyFlags &= ~0x13u;      // 0x9C8680
            else x.bodyFlags |= 0x12u;                            // 0x9C8870
            adv(1); break; }
        case 22: // 0x9C88C8: lanzar ataque arg (si no hay reaccion de dano completa, +0x1DA4 & 0x4000)
            if (!(s.st & 0x4000)) {
                Boss3Logic::selectAttackTarget(s, e, g);
                Boss3Logic::setBulletNum(s, e, g, (int8_t)pc[1]);
                Boss3Logic::startAttack(s, e, g, (int8_t)pc[1]);
            }
            adv(2); break;
        case 23: s.st |= 1; s.st &= ~2u; adv(1); break;                // 0x9C8918
        case 24: s.st |= 2; s.st &= ~1u; adv(1); break;                // 0x9C8944
        case 25: s.st &= ~3u; adv(1); break;                           // 0x9C8970
        case 26: // 0x9C89A0: modo camara por ruta propia arg (+0x1E34)
            s.st |= 0x200; s.st &= ~0x400u; s.st &= ~0x800u;
            x.pathId = arg(1);
            if (x.pathId == -1 && !(e.gameState == 0xA && e.gameKind == 2)) sg.g500E12 = 1;
            x.pathFrame = 0.0f;
            x.pathEnd = getPathEndFrame(x, h);
            adv(2); break;
        case 27: // 0x9C8A40: modo camara siguiendo la ruta activa de la camara
            s.st |= 0x200; s.st |= 0x400; s.st &= ~0x800u;
            x.pathId = h.camTrackId();
            x.pathFrame = h.camFrame();
            x.pathEnd = getPathEndFrame(x, h);
            adv(2); break;
        case 28: s.st &= ~0x200u; s.st &= ~0x400u; s.st |= 0x800; adv(1); break;   // 0x9C8AD4
        case 29: // 0x9C8B44: espera a que el fotograma de ruta (+0x1E38) llegue a arg
            if (x.pathFrame < (float)arg(1)) yield = true; else adv(2); break;
        case 30: // 0x9C8B10: espera a que +0x1DA4 bit 0x800 este activo
            if (!(s.st & 0x800)) yield = true; else adv(1); break;
        case 31: Boss3Logic::setApproach(s, arg(1)); adv(2); break;    // 0x9C8B84
        case 32: if (s.st & 0x1000) yield = true; else adv(1); break;  // 0x9C8BA0 espera fin de aproximacion
        case 33: g.cameraEvent(0, arg(1)); adv(2); break;              // 0x9C8D08: func_1C9810(camara, 0, arg)
        case 34: // 0x9C8BD4: bits 0x200000/0x400000 de +0x1DA4 segun arg
            if (arg(1) == 0) s.st &= ~0x600000u;
            else if (arg(1) == 1) { s.st |= 0x200000; s.st &= ~0x400000u; }
            else if (arg(1) == 2) { s.st &= ~0x200000u; s.st |= 0x400000; }
            adv(2); break;
        case 35: fireAlphaDirectSet(x, (float)arg(1) / bitsF(0x437F0000)); adv(2); break;   // 0x9C8C88 arg/255
        case 36: fireAlphaSupSet(x, (float)arg(1) / bitsF(0x437F0000)); adv(2); break;      // 0x9C8CC8
        case 37: x.counter = 0; adv(1); break;                         // 0x9C8D28
        case 38: x.counter++; adv(1); break;                           // 0x9C8D3C
        case 39: // 0x9C8EB8: parametros de golpes segun la dificultad (solo forma 1); otra forma: 1 golpe para aturdir
            if (s.form == 1) {
                int d = Boss3Logic::getDifficulty(e);
                s.hitReload = s.hitCtr = kBs3HitRows[s.phase][d][0];
                s.staggerMax = s.staggerHits = kBs3HitRows[s.phase][d][1];
            } else { s.hitCtr = s.hitReload; s.staggerMax = 1; s.staggerHits = 1; }
            adv(1); break;
        case 40: enableCollision(s, x); adv(1); break;                 // 0x9C8F68
        case 41: disableCollision(s, x); adv(1); break;                // 0x9C8F80
        case 42: x.b1E10 = 1; x.b1E11 = 0; adv(1); break;              // 0x9C9000
        case 43: Boss3Logic::seqCmdNextTable(s); break;                // 0x9C901C (sin ceder)
        case 44: adv(2); Boss3Logic::seqCmdSetTable(s, (int16_t)arg(1)); break;   // 0x9C902C (pc ya avanzado antes de la llamada)
        case 45: if (s.st & 0x40000) Boss3Logic::seqCmdSetTable(s, (int16_t)arg(1)); else adv(2); break;   // 0x9C904C
        case 46: if (s.st & 0x4000) Boss3Logic::seqCmdSetTable(s, (int16_t)arg(1)); else adv(2); break;    // 0x9C908C
        case 47: // 0x9C8F98
            if (arg(1) != 0) s.st |= 0x100000 | 0x20000; else s.st &= ~(0x100000u | 0x20000u);
            adv(2); break;
        case 48: yield = true; break;                                  // 0x9C9128: ceder sin avanzar
        case 52: // 0x9C90C8: rand() descartado; seqWait = gp-0x73CC
            (void)g.rand(); s.seqWait = sg.g73CC; adv(3); break;
        case 54: { // 0x9C90E8: salta a la tabla arg1 si seqWait == 0, si no a arg2; cede
            int t = (s.seqWait == 0) ? arg(1) : arg(2);
            adv(3); Boss3Logic::seqCmdSetTable(s, (int16_t)t); yield = true; break; }
        case 56: // 0x9C8D58: modo X-Wing: reproduce el movimiento 0x15E, crea la tarea Boss3XWingTask
            s.st |= 0x2000; s.motionLen = 0x15E; s.motionBlend = 0; x.f1F00 = 0;
            h.spawnXWing(); adv(1); break;
        case 57: if (arg(1) != 0) h.blurInit(); else h.blurEnd(); adv(2); break;   // 0x9C8DD0
        case 58: { // 0x9C8E08: sonido por tipo (arg 0 = ninguno)
            static const uint32_t ids[5] = {0, 0x170211, 0x800099, 0x80009E, 0x80009F};
            int k = arg(1); uint32_t id = (k >= 0 && k <= 4) ? ids[k] : 0;
            if (id) g.sound((int)id, (int)id, k == 1 ? 0 : 1, 0x40);
            adv(3); break; }
        default:  // 6, 49..51, 53, 55 y >= 59: el original salta a 0x9C9130 sin avanzar ni ceder -> bucle infinito
            return RUN_HUNG;
        }
    }
    return RUN_OK;
}

}} // namespace

// Boss2Task: interprete de secuencias y auxiliares. Reescrito a mano desde asm/bs2mod/bs2mod_text.s (ver boss2_seq.h).
#include "boss2_seq.h"
#include <cmath>
#include <cstring>
namespace vn { namespace boss {

static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

void Boss2SeqLogic::allowCameraStart(Bs2SysGlobals& sg) { sg.cameraStartAllowed = 1; }    // AllowCameraStart__9Boss2TaskFv 0x9CA240
void Boss2SeqLogic::forbidCameraStart(Bs2SysGlobals& sg) { sg.cameraStartAllowed = 0; }   // ForbidCameraStart__9Boss2TaskFv 0x9CA250

// ExecCmd__9Boss2TaskFv (0x9C7890): mientras +0x2024 == -1 lee un s16 de +0x2028. 0: lee un s16 (+0x22E2) y un s32 (+0x22D0; 2 s16) y llama a
// InitBunshin; 1: +0x2024 = 1 (fin). Otro valor: sigue leyendo.
void Boss2SeqLogic::execCmd(Bs2Ext& x, Bs2SeqHost& h) {
    while (x.cmdRun == -1) {
        if (x.cmdEnd && x.cmdPc >= x.cmdEnd) return;   // guarda propia
        int16_t c = *x.cmdPc++;
        if (c == 1) { x.cmdRun = 1; }
        else if (c == 0) {
            x.bunshinArgA = *x.cmdPc++;
            int32_t v; std::memcpy(&v, x.cmdPc, 4); x.cmdPc += 2;   // lw sobre dos s16 (little endian)
            x.bunshinArgB = v;
            h.initBunshin(x);
        }
    }
}

// ExecSeqCmd__9Boss2TaskFv (0x9C98C0). pc = +0x2054 (s16*): [0] = operacion, [1..] = argumentos. Tabla de saltos jtbl_009D71E0 (30 entradas del BIN).
// Si ninguna operacion cede, el puntero avanza kBs2OpLen[op] elementos y se pone a 0 el contador +0x2060. El bucle sigue hasta que una cede.
int Boss2SeqLogic::execSeqCmd(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SysGlobals& sg, Boss2Engine& g, Bs2SeqHost& h) {
    (void)x;
    while (true) {
        const int16_t* pc = s.seqPc;
        const int op = pc[0];
        auto arg = [&](int i) { return (int)pc[i]; };
        bool yield = false;
        switch (op) {
        case 0: if (h.camFrame() < (float)arg(1)) yield = true; break;   // 0x9C9910: espera al fotograma de camara arg
        case 1: { // 0x9C995C: ventana [arg, arg+200): Forbid; fuera de ella (antes o despues): Allow y esperar
            float f = h.camFrame(), a = (float)arg(1);
            if (f < a) { allowCameraStart(sg); yield = true; }
            else if (f < bitsF(0x43480000) + a) forbidCameraStart(sg);
            else { allowCameraStart(sg); yield = true; }
            break; }
        case 2: if (s.frame < (float)arg(1)) yield = true; break;                         // 0x9C99E4
        case 3: if (s.frame < (float)h.motionLength(s.motion)) yield = true; break;       // 0x9C9A10
        case 4: if (s.frame < (float)(arg(1) * h.motionLength(s.motion))) yield = true; break;   // 0x9C9A50
        case 5: if (s.st & 1) yield = true; break;                                         // 0x9C9A98
        case 6: if (s.st & 2) yield = true; break;                                         // 0x9C9AB8
        case 7: if (s.enemyFlags & 0x10000000u) yield = true; break;                       // 0x9C9AD8
        case 8: break;                                                                     // 0x9C9D78: sin efecto (avanza 2 elementos)
        case 9: Boss2Logic::motSupSet2(s, g, arg(1), arg(2), arg(3)); break;               // 0x9C9AFC
        case 10: // 0x9C9B18: velocidad base = arg * 0.0046875 (0x3B99999A); efectiva = base * mult
            s.baseSpeed = bitsF(0x3B99999A) * (float)arg(1); s.speed = s.baseSpeed * s.speedMul; break;
        case 11: s.frame = (float)h.motionLength(s.motion); break;                         // 0x9C9B4C: salta al ultimo fotograma
        case 12: Boss2Logic::startWarpOut(s, g, h.warpPoint(s.branchStep)); break;          // 0x9C9B78
        case 13: s.branchStep++; Boss2Logic::startWarpOut(s, g, h.warpPoint(s.branchStep)); break;   // 0x9C9B88 (paso incrementado antes de la llamada)
        case 14: h.startWarpIn(); s.enemyFlags &= ~0x40000000u; s.st &= ~0x40u; break;     // 0x9C9BA4
        case 15: Boss2Logic::selectAttackTarget(s, e, g); Boss2Logic::startAttack(s, e, g, (int8_t)pc[1]); break;   // 0x9C9BD4
        case 16: { // 0x9C9BF4: recarga hitCtr; si +0x2104 (byte bajo) != 0 sube la dificultad; SetDiffParam; marca +0x2104 = 1
            s.hitCtr = s.hitReload;
            if ((int8_t)(s.diffTimer & 0xFF) != 0) Boss2Logic::upDifficulty(s);
            Boss2Logic::setDiffParam(s, e, g);
            s.diffTimer = (int16_t)((s.diffTimer & ~0xFF) | 1);
            break; }
        case 17: s.deadStage = 2; s.deadB5 = 0; s.deadB6 = 0; break;                       // 0x9C9C28
        case 18: Boss2Logic::seqCmdNextTable(s, e); break;                                 // 0x9C9C3C (longitud 0)
        case 19: yield = true; break;                                                      // 0x9C9C4C
        case 20: Boss2Logic::seqCmdPop(s); yield = true; break;                            // 0x9C9C54
        case 21: Boss2Logic::seqCmdPop(s); Boss2Logic::seqCmdCancelWait(s); yield = true; break;   // 0x9C9C64
        case 22: g.enableCollision(); break;                                               // 0x9C9C7C
        case 23: g.disableCollision(); break;                                              // 0x9C9C8C
        case 24: { // 0x9C9C9C: +0x2069 = arg1 + rand() % arg2; gp-0x73CC = mismo valor (modulo con signo, el original rompe con arg2 == 0)
            int r = g.rand(); int m = arg(2);
            if (m == 0) return RUN_HUNG;   // `break 7` del original
            int8_t v = (int8_t)(arg(1) + (r % m));
            s.branchSel = v; sg.g73CC = v; break; }
        case 25: Boss2Logic::branch(s, e); break;                                          // 0x9C9CC8 (longitud 0)
        case 26: h.startWeakDisp(); break;                                                 // 0x9C9CD8
        case 27: if (arg(1) != 0) s.st |= 0x40000; else s.st &= ~0x40000u; break;          // 0x9C9CE8
        case 28: if (arg(1) != 0) s.st |= 0x200000; else s.st &= ~0x200000u; break;        // 0x9C9D20
        case 29: g.sound(0xD001D, 0xD001D, 0, 0x40); break;                                // 0x9C9D58
        default: return RUN_HUNG;   // >= 30: el original salta a 0x9C9D78 con una longitud leida fuera de la tabla
        }
        if (yield) return RUN_OK;
        s.seqPc += kBs2OpLen[op]; s.seqWait = 0;
    }
}
int Boss2SeqLogic::actExecSeqCmd(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SysGlobals& sg, Boss2Engine& g, Bs2SeqHost& h) { execSeqCmd(s, x, e, sg, g, h); return 0; }
int Boss2SeqLogic::actTest() { return 0; }

// ControlPos__9Boss2TaskFv (0x9C7550): forma 0 con +0x2014 & 0x800: gira hacia el punto de camara (+0x90..) con paso (int)(1.0922667 * distancia) (65536/60).
void Boss2SeqLogic::controlPos(Bs2State& s, Bs2SeqHost& h) {
    if (s.form != 0 || !(s.st & 0x800)) return;
    float c[3]; h.camPoint(c);
    float dy = s.py - c[1], dx = s.px - c[0], dz = s.pz - c[2];
    float d = std::sqrt(dx * dx + dy * dy + dz * dz);
    h.turnToward(c, (int)(bitsF(0x3F8BCF65) * d));
}

// AttackPosSet__9Boss2TaskFP11tagNLpoint3 (0x9CB3E0): punto al que apuntar el ataque. Con 1 jugador o sin objetivo: el punto de camara; si no: punto
// a (-1.2*(1-2*objetivo), 0, -1.5) en el sistema local de la camara (T(punto)*RotY(angulo)). En ambos casos gira el jefe (+0x28) hacia el punto.
void Boss2SeqLogic::attackPosSet(Bs2State& s, Bs2SeqHost& h, float out[3]) {
    int pitch;
    float c[3]; h.camPoint(c);
    if (s.playersAtStart == 1 || s.target == -1) {
        h.getAngleYX(c[0] - s.px, c[1] - s.py, c[2] - s.pz, pitch, s.angleY);
        out[0] = c[0]; out[1] = c[1]; out[2] = c[2];
        return;
    }
    float in[3] = {bitsF(0xBF99999A) * (float)(1 - s.target * 2), 0.0f, bitsF(0xBFC00000)};
    h.push(); h.translate(c[0], c[1], c[2]); h.rotY(h.camAngle());
    h.transformPoint(in, out);
    h.pop();
    h.getAngleYX(out[0] - s.px, out[1] - s.py, out[2] - s.pz, pitch, s.angleY);
}

// SetAttackDist__9Boss2TaskFf (0x9CADC0): coloca al jefe en la direccion actual respecto a +0x20CC.. pero a distancia `dist` (func_1AFA7C: v * dist/|v|);
// la y se desplaza 15 antes de reescalar y se resta despues.
void Boss2SeqLogic::setAttackDist(Bs2State& s, const Bs2Ext& x, Bs2SeqHost& h, float dist) {
    float v[3] = {s.px - x.anchor[0], bitsF(0x41700000) + (s.py - x.anchor[1]), s.pz - x.anchor[2]}, r[3];
    h.scaleToLength(v, dist, r);
    s.px = x.anchor[0] + r[0];
    s.py = (x.anchor[1] + r[1]) - bitsF(0x41700000);
    s.pz = x.anchor[2] + r[2];
}

// FinishTask0__9Boss2TaskFv (0x9CA0B0)
void Boss2SeqLogic::finishTask0(Bs2State& s, Bs2Ext& x, Bs2Env& e, Boss2Engine& g, Bs2SeqHost& h) {
    if (x.warpTrack >= 0) { h.releaseTrack(x.warpTrack, x.warpTrackAux); x.warpTrack = -1; }
    h.releaseTrail();
    for (int i = 0; i < 2; i++) if (x.swordFirePresent[i]) x.swordFireDetached[i] = 1;   // *(p+0x2550) = 0
    if (x.warpEffPresent) { h.deleteWarpEff(); x.warpEffPresent = 0; }
    for (int i = 0; i < 2; i++) if (x.fx20E4[i]) { h.freeObject(x.fx20E4[i]); x.fx20E4[i] = nullptr; }
    const bool special = (e.gameState == 0xA && e.gameKind == 2);
    if (special && x.scene == 0x13) g.setAlpha(1.0f);
    if (special) { Boss2Logic::decNowEnemyAlvNumChk(s, e); Boss2Logic::decNowEnemyNumChk(s, e); }
}
void Boss2SeqLogic::finishTask(Bs2SeqHost& h) { h.taskEnd(); }   // FinishTask__9Boss2TaskFv (0x9CA220)

// actDelete__9Boss2TaskFv (0x9C9410): paso (+0x20B5) 0: AtcPlFlgClear, +0x0C |= 0x4000, +0x10 |= 0x20, +0x18 = 0, paso++; 1: FinishTask. Devuelve 0.
int Boss2SeqLogic::actDelete(Bs2State& s, Bs2Ext&, Bs2Env& e, Bs2SeqHost& h) {
    if (s.deadB5 == 0) {
        Boss2Logic::atcPlFlgClear(s, e);
        s.enemyFlags |= 0x4000; s.flags2 |= 0x20;
        s.deadB5++;
    } else if (s.deadB5 == 1) finishTask(h);
    return 0;
}

// actDead__9Boss2TaskFv (0x9C97A0): apaga el contorno (-0.02 por fotograma); si aun no se ha marcado (+0x20B5 == 0) y el fotograma >= 100:
// con la pista de camara 0x199 pasa al estado de muerte 2; si no, gp-0x73CC = 2, gp-0x72A0 = 1 si gp-0x729C != 0 y AllowCameraStart. Devuelve 0.
int Boss2SeqLogic::actDead(Bs2State& s, Bs2Ext&, Bs2SysGlobals& sg, Bs2SeqHost& h) {
    if (s.st & 0x40000) {
        float v = s.outlineAlpha - bitsF(0x3CA3D70A);
        s.outlineAlpha = v;
        if (v <= 0.0f) { s.outlineAlpha = 0.0f; s.st &= ~0x40000u; }
    }
    if (s.deadB5 != 0) return 0;
    if (s.frame < bitsF(0x42C80000)) return 0;
    if (h.camTrackId() == 0x199) { s.deadStage = 2; s.deadB5 = 0; s.deadB6 = 0; }
    else {
        sg.g73CC = 2;
        if (sg.g729C != 0) sg.g72A0 = 1;
        allowCameraStart(sg);
    }
    return 0;
}

// CharInit__9Boss2TaskFv (0x9C7930)
void Boss2SeqLogic::charInit(Bs2State& s, Bs2Ext& x, Bs2Env& e, Bs2SysGlobals& sg, Boss2Engine& g, Bs2SeqHost& h, const Bs2FormConfig* c) {
    (void)sg; (void)g;
    h.charInitClear();
    s.branchStep = 0;
    if (c) {
        s.enemyFlags |= c->orC; s.enemyFlags &= c->andC;
        s.st |= c->orSt; s.st &= c->andSt;
        s.alphaOutThr = c->alphaOutThr; s.alphaInThr = c->alphaInThr;
        s.hitCtr = c->hitReload; s.hitReload = c->hitReload;
        if (c->branch) Boss2Logic::branchInit(s, e, c->branch);
        else {
            if (c->branchParam != 0) s.branchParam = c->branchParam;
            if (c->seqTable) Boss2Logic::seqCmdInit(s, c->seqTable);
        }
        s.deadStage = c->deadStage; s.deadB5 = 0; s.deadB6 = 0;
    }
    switch (s.form) {
    case 0:
    case 1:
        if (s.form == 0) h.setWarpPos();
        x.warpTrack = 0; x.warpTrackAux = 7;
        h.newSwordFire(0, -1); x.swordFirePresent[0] = 1; h.initSwordFire(0, 2, 0x14, 0);
        h.newSwordFire(1, -1); x.swordFirePresent[1] = 1; h.initSwordFire(1, 1, 0x1E, 1);
        h.setTakion(1); h.finalizeSwordFire(1);
        s.swordFireAlpha = 0; x.sf22F8 = 0;
        Boss2Logic::initLife(s, e, s.form == 0 ? (int16_t)h.scaledLife() : (int16_t)-1);
        break;
    case 2:
        e.gp73FC = 0; e.gp73F8 = 0; s.st |= 0x8000;
        h.form2Setup(x);
        Boss2Logic::initLife(s, e, (e.gameState == 0xA && e.gameKind == 2) ? (int16_t)h.scaledLife() : (int16_t)-1);
        break;
    case 3: if (x.owner) s.st = x.owner->st; break;
    case 4: x.bodyFlags &= ~3u; s.deadStage = 5; s.deadB5 = 0; s.deadB6 = 0; break;
    default: break;
    }
}

}} // namespace

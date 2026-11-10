// TBoss4 (overlay bs4mod): reglas del jefe reescritas a mano. Cada funcion cita la original (simbolo y direccion).
#include "boss4.h"
#include <cstring>
namespace vn { namespace boss4 {

static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
static const float kRate = 1.2f;                       // 0x3F99999A: 60 Hz -> 50 Hz (PAL)
static const float kInv256 = 0.00390625f;              // 0x3B800000

// ---------------------------------------------------------------- VM de comandos
// SeqCmdInit (0x9CCxxx): fija la tabla de scripts y empieza por el primero.
void Boss4Logic::seqInit(Boss4State& b, const uint16_t* table) {
    b.seqTab = table; b.tabIdx = 0;
    b.pc = (table && table[0] != 0xFFFF) ? kScript + table[0] : nullptr;
    b.timerSel = 0; b.sp = 0; b.timer[b.sp] = -1;
}
// SeqCmdNextTable: pasa al siguiente script de la tabla.
void Boss4Logic::seqNextTable(Boss4State& b) {
    b.tabIdx++;
    uint16_t e = b.seqTab ? b.seqTab[b.tabIdx] : 0xFFFF;
    b.pc = e != 0xFFFF ? kScript + e : nullptr;
    b.timerSel = 0;
}
// SeqCmdSetTable: salta al script `idx` y vacia la pila.
void Boss4Logic::seqSetTable(Boss4State& b, int idx) {
    b.tabIdx = (int16_t)idx;
    uint16_t e = b.seqTab ? b.seqTab[idx] : 0xFFFF;
    b.pc = e != 0xFFFF ? kScript + e : nullptr;
    b.timerSel = 0; b.sp = 0; b.timer[b.sp] = -1;
}
// SeqCmdPush: guarda el pc actual y abre un contador nuevo.
void Boss4Logic::seqPush(Boss4State& b) {
    b.stack[(int)b.sp] = b.pc; b.sp++; b.timer[(int)b.sp] = -1;
}
// SeqCmdPop: vuelve al pc guardado (no reinicia el contador).
void Boss4Logic::seqPop(Boss4State& b) { b.sp--; b.pc = b.stack[(int)b.sp]; }
// SeqCmdCall: interrumpe el script actual con un script de reaccion (D_009D2CA0[i]).
void Boss4Logic::seqCall(Boss4State& b, int callScript) { seqPush(b); b.pc = kScript + kCallScript[callScript]; }
// SeqCmdCancelWait: si el comando actual es una espera de movimiento (2,3,4) lo salta.
void Boss4Logic::seqCancelWait(Boss4State& b) {
    int op = b.pc[0];
    if (op != 4 && op != 3 && op != 2) return;
    b.pc += kCmdLen[op];
}

// MotSupSet2 (0x9C...): pide un movimiento distinto del actual.
void Boss4Logic::motSupSet2(Boss4State& b, Boss4Env& env, int motion, int a, int bb) {
    if (b.motion != motion) {
        b.motReqMode = 2; b.motReqA = (int16_t)a; b.motReqB = (int16_t)bb;
        env.playMotion(motion, (int16_t)a);
        if (b.typeId == 0x1C) env.hook(H_PLAY_HAND_MOTIONS, (int16_t)a, 0, 0);
    }
}
// MotSupSet3: como MotSupSet2 pero tambien relanza si el fotograma actual no es el pedido.
void Boss4Logic::motSupSet3(Boss4State& b, Boss4Env& env, int motion, int a, int bb) {
    if (b.motion != motion || b.motionFrame != (float)(int16_t)a) {
        b.motReqMode = 2; b.motReqA = (int16_t)a; b.motReqB = (int16_t)bb;
        env.playMotion(motion, (int16_t)a);
        if (b.typeId == 0x1C) env.hook(H_PLAY_HAND_MOTIONS, (int16_t)a, 0, 0);
    }
}

// ExecSeqCmd (0x9CBCB0..0x9CCBF0): interprete de scripts de ataque del jefe. Ejecuta comandos hasta que uno pide esperar.
void Boss4Logic::execSeqCmd(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    for (;;) {
        if (!b.pc) return;
        const int16_t* c = b.pc;
        unsigned op = (uint16_t)c[0];
        if (op >= 67) return;                       // el original lee la tabla de longitudes fuera de rango; no ocurre con datos validos
        int wait = 0;
        switch (op) {
        case 0: {   // espera de camara: tipo 2 solo avanza en la ventana [P, P+10); otros esperan hasta cam >= P
            float p = (float)c[1], cam = env.camFrame();
            if (b.kind == K_BATTLE3) { if (cam < p) wait = 1; else if (!(cam < 10.0f + p)) wait = 1; }
            else if (cam < p) wait = 1;
            break; }
        case 1: {   // puerta de camara: dentro de [P, P+100) prohibe iniciar camara y avanza; fuera permite y espera
            float p = (float)c[1], cam = env.camFrame();
            if (cam < p || !(cam < 100.0f + p)) { g.cameraStartAllowed = 1; wait = 1; }
            else g.cameraStartAllowed = 0;
            break; }
        case 2: if (b.motionFrame <= (float)c[1]) wait = 1; break;                       // espera fotograma > P
        case 3: if (b.motionFrame <= (float)env.motionLen(b.motion)) wait = 1; break;    // espera fin del movimiento
        case 4: if (b.motionFrame < (float)(c[1] * env.motionLen(b.motion))) wait = 1; break;
        case 5: if (b.motionFrame < (float)(b.loopCount * env.motionLen(b.motion))) wait = 1; break;
        case 6: if (b.f0C & F0C_ATTACKING) wait = 1; break;
        case 7: {   // espera P fotogramas NTSC (P*50/60 en PAL; P == 1 -> 1)
            int16_t& t = b.timer[(int)b.sp];
            if (t == -1) { int p = c[1]; t = (int16_t)(p == 1 ? 1 : (p * 50) / 60); }
            int16_t v = t; t = (int16_t)(v - 1);
            if (v != 0) wait = 1;
            break; }
        case 8: {   // espera `loopCount` fotogramas (dependiente de la dificultad)
            int16_t& t = b.timer[(int)b.sp];
            if (t == -1) t = (int16_t)b.loopCount;
            int16_t v = t; t = (int16_t)(v - 1);
            if (v != 0) wait = 1;
            break; }
        case 9: {   // espera a que el bit c[1] de +0x0C valga c[2]
            bool set = (b.f0C & (1u << (c[1] & 31))) != 0;
            if (c[2] ? !set : set) wait = 1;
            break; }
        case 10: {  // idem sobre +0x1E14
            bool set = (b.f1E14 & (1u << (c[1] & 31))) != 0;
            if (c[2] ? !set : set) wait = 1;
            break; }
        case 11: motSupSet2(b, env, c[1], c[2], c[3]); break;
        case 12: motSupSet3(b, env, c[1], c[2], c[3]); break;
        case 13: {  // velocidad base de reproduccion = 1.2 * P/256
            float f = kRate * (kInv256 * (float)c[1]);
            b.rateBase = f; b.rate = f * b.rateMul; break; }
        case 14: selectAttackTarget(b, g, env); startAttack(b, g, env, (int8_t)c[1]); break;
        case 15:    // reinicia el contador de impactos, quita "en dano" y re-arma la dificultad
            b.hitCounter = b.hitsPerCount; b.f0C &= ~(uint32_t)F0C_IN_DAMAGE;
            if (b.diffArmed) upDifficulty(b);
            setDiffParam(b, g); b.diffArmed = 1; break;
        case 16: b.state = TS_FINISHED; b.stateB = 0; b.stateC = 0; break;
        case 17: seqNextTable(b); b.phaseCount++; break;
        case 18: wait = 1; break;
        case 19: seqPop(b); break;
        case 20: seqPop(b); seqCancelWait(b); break;
        case 21: enableCollision(b, env); break;
        case 22: disableCollision(b, env); break;
        case 23: env.hook(H_START_WEAK_DISP, 0, 0, 0); break;
        case 24: if (c[1]) b.f1E14 |= FB_LIFEBAR; else b.f1E14 &= ~(uint32_t)FB_LIFEBAR; break;
        case 25: break;
        case 26: {  // pone posicion y rotacion de la tabla kPose
            const float* p = kPose[c[1]];
            b.pos[0] = p[0]; b.pos[1] = p[1]; b.pos[2] = p[2]; b.rot[0] = p[3]; b.rot[1] = p[4]; b.rot[2] = p[5];
            break; }
        case 27: seqSetTable(b, c[1]); break;
        case 28: setRunKindRand(b, env); break;
        case 29: b.runC = 0; break;
        case 30: motSupSet2(b, env, kRunRowA[(int)b.runA].mot[0], c[1], c[2]); break;
        case 31: if (b.phaseCount < 4) motSupSet2(b, env, kRunRowA[(int)b.runA].mot[1], c[1], c[2]); break;
        case 32: motSupSet2(b, env, kRunRowA[(int)b.runA].mot[3 + b.runC], c[1], c[2]); break;
        case 33: motSupSet2(b, env, kRunRowA[(int)b.runA].mot[2], c[1], c[2]); break;
        case 34: startBunshin(b, env); break;
        case 35: startShusoku(b, env); break;
        case 36: env.setCamSpeed(kRate * (kInv256 * (float)c[1])); break;
        case 37: if (c[2]) b.f0C |= 1u << (c[1] & 31); else b.f0C &= ~(1u << (c[1] & 31)); break;
        case 38: if (c[2]) b.f1E14 |= 1u << (c[1] & 31); else b.f1E14 &= ~(1u << (c[1] & 31)); break;
        case 39: motSupSet2(b, env, kRunRowB[(int)b.runB].mot[0], c[1], c[2]); break;
        case 40: motSupSet2(b, env, kRunRowB[(int)b.runB].mot[1], c[1], c[2]); break;
        case 41: env.hook(H_CLEAR_BODY_MOVE, 0x12, 0, 0); break;
        case 42: env.hook(H_SET_BODY_MOVE, 0x12, 0, 0); break;
        case 43: env.hook(H_CLEAR_BODY_MOVE, 0x10, 0, 0); break;
        case 44: env.hook(H_SET_BODY_MOVE, 0x10, 0, 0); break;
        case 45: if (b.f10 & 0x40) wait = 1; break;
        case 46: env.hook(H_RAISE_GAREKI, c[1], 0, 0); break;
        case 47: env.hook(H_RAISE_GAREKI2, c[1], 0, 0); break;
        case 48: env.hook(H_RAISE_GAREKI3, c[1], 0, 0); break;
        case 49: env.hook(H_FALL_GAREKI, c[1], 0, 0); break;
        case 50:    // solo el tipo 1 cuenta golpes: con >= 6 salta al script 4; en otro caso se salta a si mismo (+1 int16)
            if (b.kind != K_BATTLE2) b.pc += 1;
            else if (b.phaseCount < 6) b.pc += 1;
            else seqSetTable(b, 4);
            break;
        case 51: {  // router de fase del tipo 3 segun el fotograma de camara (ventana 0..10)
            float cam = env.camFrame();
            if (cam < 0.0f || !(cam < 10.0f)) { g.cameraStartAllowed = 1; wait = 1; break; }
            env.hook(H_SET_CAM_PATH_ATTR, 0, 0, 0);
            if (b.b1E4D == 0 && (env.rand() & 1)) {
                b.subPhase = 0; b.b1E4E = (int8_t)(env.rand() & 1); b.b1E4D = 1; seqSetTable(b, 3);
            } else {
                b.subPhase = b.b1E11; b.b1E4D = 0;
                seqSetTable(b, b.subPhase == 1 ? 1 : 5);
            }
            break; }
        case 52: b.b1E4C = (int8_t)c[1]; break;
        case 53: b.b1DB8 = (int8_t)c[1]; break;
        case 54: env.hook(H_SET_FCV, c[1], c[2], c[3]); break;
        case 55: b.camSpeed1E00 = kRate * (kInv256 * (float)c[1]); break;
        case 56:
            if (c[1] > 0) { b.f1E04 = 0.0f; b.f1E08 = kRate * (1.0f / (float)c[1]); }
            else { b.f1E04 = 1.0f; b.f1E08 = 0.0f; }
            break;
        case 57: if (b.f1DFC < b.f1DF8) wait = 1; break;
        case 58: if (c[1] < g.enemiesLeft) wait = 1; break;
        case 59:
            selectAttackTarget(b, g, env);
            if (b.target >= 0) { if (env.damagePlayer(b.target, 1, 7) == 1) downDifficulty(b); else b.diffArmed = 0; }
            break;
        case 60: {  // aparicion del invocado por el lado de la pantalla en que esta
            b.f1E14 |= FB_VISIBLE; env.hook(H_SET_SCREEN_OFFSET, 0, 0, 0);
            float x, y; env.projectToScreen(b.pos, x, y);
            if (x <= 0.0f) motSupSet2(b, env, 0x1A5, 0, 5); else motSupSet2(b, env, 0x1A6, 0, 5);
            break; }
        case 61: wait = 1; break;
        case 62:    // reaparicion escalonada segun el fotograma de camara (kReappearPos)
            if (b.alpha < 1.0f) {
                if (env.camFrame() < (float)kSchedFrame[b.phaseCount - 1]) { wait = 1; break; }
                if (b.phaseCount < 4) {
                    startAlphaAdd(b);
                    const float* p = kReappearPos[b.phaseCount - 1];
                    b.pos[0] = p[0]; b.pos[1] = p[1]; b.pos[2] = p[2]; b.rot[1] = 0;
                    motSupSet3(b, env, kRunRowA[(int)b.runA].mot[0], 0, 0);
                } else {
                    b.alpha = 1.0f;
                    b.pos[0] = bitsF(0xC24DB958); b.pos[1] = bitsF(0xC18B3333); b.pos[2] = bitsF(0x44B2A000);
                    b.rot[1] = 0;
                    motSupSet3(b, env, 0x16B, 0xE1, 0);
                }
            }
            break;
        case 63: if (b.f1E14 & FB_ALPHA_RUN) wait = 1; break;
        case 64: if (!(b.owner->f1E14 & FB_BUNSHIN_ASK)) wait = 1; break;
        case 65: if (!(b.owner->f1E14 & FB_SHUSOKU_ASK)) wait = 1; break;
        case 66:
            if (c[1]) b.f1E14 &= ~(uint32_t)FB_BN_HOLD; else b.f1E14 |= FB_BN_HOLD;
            if (b.kind == K_SHOUKAN) { if (c[1]) b.owner->f1E14 |= 0x04000000; else b.owner->f1E14 &= ~0x04000000u; }
            break;
        }
        if (!wait) { b.pc += kCmdLen[op]; b.timerSel = 0; }
        if (wait) return;
    }
}

// ---------------------------------------------------------------- dificultad
// InitDifficulty (0x9CA300): nivel inicial de dificultad (0..16) tomado del motor (func_96CEC0).
void Boss4Logic::initDifficulty(Boss4State& b, int initial) { b.difficulty = initial; b.diffArmed = 0; }
// SetDiffParam (0x9CA140): carga los parametros del tipo para la dificultad actual.
void Boss4Logic::setDiffParam(Boss4State& b, Boss4Globals& g) {
    int lv = (int16_t)g.gameLevel, d = b.difficulty;
    const DiffRow& r = kDiffRows[b.kind * 2 + lv];
    int8_t hp = b.kind == K_BATTLE3 ? kHpKind2[(int)b.subPhase][lv][d] : r.hp[d];
    b.partLife = hp; b.partLifeMax = hp;
    b.dmgMul = r.dmgMul[d];
    b.speedBase = r.speed16[d] / 16.0f;
    b.diffUp = r.stepUp; b.diffDown = r.stepDown;
    if (b.kind == K_BATTLE2) b.loopCount = kLoopKind1[lv][d];
    else if (b.kind == K_BATTLE3) {
        if (b.subPhase == 2) b.loopCount = kLoopKind2Sub2[lv][d];
        else if (b.subPhase == 0) b.loopCount = kLoopKind2Sub0[lv][d];
    }
    g.diffMirror = d;
}
// UpDifficulty (0x9CA3xx): sube la dificultad (tope 16).
void Boss4Logic::upDifficulty(Boss4State& b) { b.difficulty += b.diffUp; if (b.difficulty >= 0x11) b.difficulty = 0x10; }
// DownDifficulty: baja la dificultad (suelo 0) y desarma la subida.
void Boss4Logic::downDifficulty(Boss4State& b) { b.difficulty -= b.diffDown; if (b.difficulty < 0) b.difficulty = 0; b.diffArmed = 0; }
// NoChgDifficultyReq: deja de pedir cambio de dificultad.
void Boss4Logic::noChgDifficultyReq(Boss4State& b) { b.diffArmed = 0; }

// ---------------------------------------------------------------- vida y dano
// InitLife (0x9C9CB0): fija la vida del jefe; si no hay valor valido usa 400 (0x190).
void Boss4Logic::initLife(Boss4State& b, Boss4Globals& g, int life) {
    int16_t l = (int16_t)life;
    if (l > 0) { g.bossLife = l; g.bossLifeMax = l; }
    if (g.bossLifeMax <= 0 || g.bossLife <= 0) { g.bossLife = 0x190; g.bossLifeMax = 0x190; }
    b.lifeRatio = 1.0f; b.hitRatio = 1.0f;
}
// ControlDispLife (0x9C9D50): razon vida/vida maxima y golpes restantes/golpes de la fase (el dibujo lo hace el motor).
void Boss4Logic::controlDispLife(Boss4State& b, const Boss4Globals& g) {
    b.lifeRatio = (float)g.bossLife / (float)g.bossLifeMax;
    b.hitRatio = (float)b.partLife / (float)b.partLifeMax;
}
// CheckWeakHit: solo los tipos 0, 1 y 2 tienen punto debil.
bool Boss4Logic::checkWeakHit(const Boss4State& b) { return b.kind == K_BATTLE1 || b.kind == K_BATTLE2 || b.kind == K_BATTLE3; }

// CheckDamage (0x9CAE60): procesa los impactos de los 2 jugadores de este fotograma.
void Boss4Logic::checkDamage(Boss4State& b, Boss4Globals& g, Boss4Env& env, const HitInfo hit[2]) {
    if (!(b.f0C & F0C_HIT)) return;
    env.hook(H_HIT_PREPARE, 0, 0, 0);
    if (!(b.f0C & 0x04008100)) {
        for (int i = 0; i < 2; i++) {
            if (hit[i].slot == -1) continue;
            int8_t part = hit[i].partType;
            if (part <= 0) continue;
            b.weakIdx = hit[i].weakIdx;
            if ((b.f0C & F0C_WEAK) || checkWeakHit(b)) {
                weakHit(b, g, env, part, hit[i].slot);
                env.hook(H_HIT_CONSUMED, hit[i].slot, 0, 0);
            } else noWeakHit(b, env, part);
        }
    }
    env.hook(H_CLEAR_WEAK_HIT, 0, 0, 0);
}
// WeakHit (0x9CAFA0): aplica un golpe valido; descuenta vida compartida y golpes de la fase; elige reaccion (1 = flojo, 2 = fase, 3 = muerte).
void Boss4Logic::weakHit(Boss4State& b, Boss4Globals& g, Boss4Env& env, int partType, int hitIdx) {
    if (b.f0C & 0x44008100) return;
    if (b.f1E14 & FB_HIT_STUN) return;
    b.hitCounter--;
    if (b.hitCounter > 0) return;
    int result = 0;
    b.hitCounter = b.hitsPerCount;
    env.registerHit(hitIdx, 10);
    g.bossLife -= (int)((float)b.dmgMul * env.damageScale((int8_t)hitIdx));
    if (g.bossLife <= 0) {
        if (b.kind != K_BATTLE3) g.bossLife = 1;
        else { g.bossLife = 0; result = 3; }
    }
    b.partLife--;
    if (b.partLife <= 0) { if (result < 2) result = 2; }
    else if (result <= 0) result = 1;
    switch (result) {
    case 1: damageUpperBody(b, env); env.hook(H_HIT_MARK2, hitIdx, partType, 0); break;
    case 2: damage(b, g, env); env.hook(H_HIT_MARK1, hitIdx, 0, 0); break;
    case 3: dead(b, g, env, hitIdx); env.hook(H_HIT_MARK2, hitIdx, partType, 0); break;
    }
}
// NoWeakHit (0x9CB1xx): golpe en zona no debil; solo el tipo 3 (invocado) reacciona con un efecto segun la zona.
void Boss4Logic::noWeakHit(const Boss4State& b, Boss4Env& env, int partType) {
    if (b.kind != K_SHOUKAN) return;
    bool fx = true;
    switch (b.b1E51) {
    case 1: if (partType < 3 || partType >= 7) fx = false; break;
    case 2: if (partType < 7 || partType >= 0xB) fx = false; break;
    default: break;
    }
    if (fx) env.hook(H_NO_WEAK_FX, 0, 0, 0);
}
// DamageUpperBody (0x9CB1xx): golpe flojo (sin cambio de fase): sonido, efecto y aturdimiento de 25 fotogramas (tipos 0..2).
void Boss4Logic::damageUpperBody(Boss4State& b, Boss4Env& env) {
    int fx = 0, snd = 0;
    switch (b.kind) {
    case K_BATTLE3: snd = 0x11001E; fx = 0x1AB; break;
    case K_BATTLE2: snd = 0x11000C; fx = 0x17E; break;
    case K_BATTLE1: snd = 0x11000C; fx = 0x19B; break;
    default: break;
    }
    if (snd) {
        env.sound(snd, 0);
        if (!(b.f0C & F0C_ATTACKING)) env.hook(H_SPAWN_EFFECT, fx, 0, 0);
        b.f1E14 |= FB_HIT_STUN; b.stunTimer = 0x19;
    } else b.f1E14 |= FB_HIT_STUN;
}
// Damage (0x9CB3xx): fin de fase de golpes: pasa al siguiente script de la tabla y lanza el script de reaccion del tipo.
void Boss4Logic::damage(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    b.timer[(int)b.sp] = -1;
    switch (b.kind) {
    case K_BATTLE1: env.sound(0x11000D, 0); startAlphaSub(b); seqNextTable(b); b.phaseCount++; seqCall(b, 0); break;
    case K_BATTLE2: env.sound(0x11000D, 0); seqNextTable(b); b.phaseCount++; seqCall(b, 1); break;
    case K_BATTLE3:
        env.sound(0x11001E, 0);
        if (b.subPhase == 2) g.cameraStartAllowed = 0;
        seqNextTable(b); b.phaseCount++; seqCall(b, 2); b.b1E4C = 4; break;
    default: break;
    }
    b.rateBase = kRate; b.rateMul = 1.0f; b.rate = kRate * 1.0f;
    setDamageMotion(b, env);
    b.f0C |= F0C_IN_DAMAGE;
    if (b.f1E14 & FB_BN_CANCEL) cancelBunshin(b);
}
// SetDamageMotion: movimiento de dano segun el tipo (tipo 1 ademas arranca el movimiento corporal 0x10).
void Boss4Logic::setDamageMotion(Boss4State& b, Boss4Env& env) {
    int m = -1;
    switch (b.kind) {
    case K_BATTLE3: m = 0x1AA; break;
    case K_BATTLE2: m = 0x17C; env.hook(H_SET_BODY_MOVE, 0x10, 0, 0); break;
    case K_BATTLE1: m = 0x197; break;
    case K_SHOUKAN:
        switch (b.motion) { case 0x1C4: case 0x1BA: case 0x1B1: case 0x1AE: case 0x1AD: m = 0x1C5; break; default: break; }
        break;
    default: break;
    }
    if (m >= 0) motSupSet3(b, env, m, 0, 5);
}
// SetDeadMotion: movimiento de muerte (tipo 2 y el invocado).
void Boss4Logic::setDeadMotion(Boss4State& b, Boss4Env& env) {
    int m = -1;
    if (b.kind == K_BATTLE3) m = 0x1AA;
    else if (b.kind == K_SHOUKAN) {
        switch (b.motion) { case 0x1C4: case 0x1BA: case 0x1B1: case 0x1AE: case 0x1AD: m = 0x1C5; break; default: break; }
    }
    if (m >= 0) motSupSet3(b, env, m, 0, 5);
}
// Dead (0x9CB5xx): muerte del jefe: puntos 0x5DC, sonido, estado "muerto" (4) y estadisticas de logro.
void Boss4Logic::dead(Boss4State& b, Boss4Globals& g, Boss4Env& env, int hitIdx) {
    b.f1E14 &= 0xFFDFFFFF; b.f2E38 = 0;
    env.registerHit(hitIdx, 0x5DC);
    env.sound(0x11001E, 0);
    b.f1E14 &= ~(uint32_t)FB_LIFEBAR; b.f0C |= F0C_DEAD;
    setDeadMotion(b, env);
    b.state = TS_DEAD; b.stateB = 0; b.stateC = 0; b.b1E4C = 5;
    if (!(g.gameState == 0xA && g.gameKind == 2)) {
        if (env.statFunc(2, 4) == 0 && g.deathCount73FC == 0) env.statFunc(2, 5);
        if (g.deathCount73F8 == 0) env.statFunc(2, 0xC);
    }
}
// actDelete: tarea de borrado en dos pasos (limpia objetivos, marca borrar; luego termina la tarea).
int Boss4Logic::actDelete(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    switch (b.stateB) {
    case 0:
        g.targetBusy[0] = 0; g.targetBusy[1] = 0;
        b.f0C |= F0C_DEAD; b.f0C |= F0C_DELETE; b.f10 |= 0x20; b.f18 = 0; b.stateB++;
        break;
    case 1: env.hook(H_FINISH_TASK, 0, 0, 0); break;
    }
    return 0;
}
// actBunshin: el clon pasa a "terminado" cuando el jefe padre lo esta.
int Boss4Logic::actBunshin(Boss4State& b) {
    if (b.owner->state == TS_FINISHED) { b.state = TS_FINISHED; b.stateB = 0; b.stateC = 0; }
    return 0;
}
// actShoukan: igual para el invocado; si el padre no ha terminado ejecuta su propio script.
int Boss4Logic::actShoukan(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    if (b.owner->state == TS_FINISHED) { b.state = TS_FINISHED; b.stateB = 0; b.stateC = 0; }
    else execSeqCmd(b, g, env);
    return 0;
}
// actDead: secuencia de muerte: espera el fin del movimiento, congela la animacion y espera la ruta de camara 0x136.
int Boss4Logic::actDead(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    switch (b.stateB) {
    case 0:
        if (!(b.motionFrame < (float)env.motionLen(b.motion))) { b.rateBase = 0; b.rate = 0.0f * b.rateMul; b.stateB++; }
        break;
    case 1:
        if (env.camTrackId() == 0x136) { b.state = TS_FINISHED; b.stateB = 0; b.stateC = 0; }
        else {
            g.g73CC = 2;
            if (g.g729C != 0) g.g72A0 = 1;
            g.cameraStartAllowed = 1;
        }
        break;
    }
    return 0;
}

// ---------------------------------------------------------------- ataques
// SelectAttackTarget (0x9CD3xx): elige el jugador al que ataca (nivel 0: el ultimo que golpeo; nivel 1: aleatorio evitando repetir).
bool Boss4Logic::selectAttackTarget(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    switch ((int16_t)g.gameLevel) {
    case 0: b.target = (int8_t)g.lastHitTarget; break;
    case 1: {
        int8_t p = (int8_t)(env.rand() & 1);
        if (g.targetBusy[(int)p] != 0) {
            p = (int8_t)((p + 1) & 1);
            if (g.targetBusy[(int)p] != 0) { g.targetBusy[1] = 0; g.targetBusy[0] = 0; }
        }
        b.target = p; break; }
    default: break;
    }
    if (!env.playerAlive(b.target)) b.target = -1;
    if (b.target == -1) return false;
    g.targetBusy[(int)b.target] = 1;
    return true;
}
// DamageToPlayer (0x9C79xx): el ataque actual hace dano al jugador elegido; si le da sube la presion (baja dificultad).
void Boss4Logic::damageToPlayer(Boss4State& b, Boss4Globals&, Boss4Env& env) {
    if (b.target >= 0) {
        if (env.damagePlayer(b.target, 1, kAttackTable[(int)b.attack].dmg) == 1) downDifficulty(b);
        else b.diffArmed = 0;
    }
}
// AttackDamageFrame (0x9C7980): fotograma de dano del ataque: dano al jugador y sonidos de impacto (tipos 0 y 1).
void Boss4Logic::attackDamageFrame(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    damageToPlayer(b, g, env);
    if (b.kind == K_BATTLE1 || b.kind == K_BATTLE2) { env.sound(0x100010, 0x40); env.sound(0x8000B2, 0x40); }
}
// StartAttack (0x9C7DC0): id 15/16 eligen un ataque por tablas; 17..21 casos especiales; resto: ataque directo de kAttackTable.
void Boss4Logic::startAttack(Boss4State& b, Boss4Globals& g, Boss4Env& env, int id) {
    bool common = true;
    switch (id) {
    case 15: b.attack = kRunRowA[(int)b.runA].atk[(int)b.runB]; break;
    case 16: b.attack = kRunRowB[(int)b.runB].atk[(int)b.runD]; break;
    case 17: b.attack = (int8_t)id; b.atkStep = -1; b.s1F40 = 0; b.b1F26 = (int8_t)g.numPlayersB; common = false; env.hook(H_START_ATTACK_IWA, 0, 0, 0); break;
    case 18: b.attack = (int8_t)id; b.atkStep = 0; b.s1F40 = 0; b.b1F26 = (int8_t)g.numPlayersB; common = false;
             b.f0C |= F0C_ATTACKING; b.f1E14 |= FB_ATTACK; break;
    case 19: common = false; env.hook(H_START_ATTACK_KIRI_PRE, env.rand() & 1, 0, 0); break;
    case 20: common = false; env.hook(H_START_ATTACK_KIRI, 0, 0, 0); break;
    case 21: common = false; env.hook(H_START_ATTACK_KIRI_SHOUKAN, 0, 0, 0); break;
    default: b.attack = (int8_t)id; break;
    }
    if (!common) return;
    b.atkStep = 0; b.s1F40 = 0; b.b1F26 = (int8_t)g.numPlayersB;
    const AttackEntry& e = kAttackTable[(int)b.attack];
    if (b.kind == K_BATTLE3 || b.kind == K_BATTLE2) motSupSet2(b, env, e.motion, e.startFrame, 5);
    else if (b.kind == K_BATTLE1) { motSupSet2(b, env, e.motion, e.startFrame, 2); b.f1E14 |= FB_VISIBLE; }
    b.f0C |= F0C_ATTACKING; b.f1E14 |= FB_ATTACK;
}
// ControlAttack (0x9C7A10): linea de tiempo del ataque por fotogramas del movimiento (tabla de saltos por paso +0x1F42, 0..6).
// Umbrales de kAttackTable: f[0..5] = +6,+8,+A,+C,+E,+0x10 y endFrame = +0x12; un umbral negativo = "no hacer la accion" (el paso avanza igual).
void Boss4Logic::controlAttack(Boss4State& b, Boss4Globals& g, Boss4Env& env) {
    if (!(b.f1E14 & FB_ATTACK)) return;
    const AttackEntry& e = kAttackTable[(int)b.attack];
    if (b.motion != e.motion) { b.f0C &= 0xEFFFFFFF; b.f1E14 &= ~(uint32_t)FB_ATTACK; return; }   // el movimiento se interrumpio: cancela
    unsigned st = (unsigned)(uint8_t)b.atkStep;
    if (st > 6) return;
    // Cada paso espera a que el fotograma alcance su umbral; al pasarlo hace su accion y continua con el siguiente en el mismo fotograma.
    for (;; st++) {
        if (st < 6) {
            if (b.motionFrame < (float)e.f[st]) return;
            if (e.f[st] >= 0) {
                switch (st) {
                case 0: case 3: b.f0C |= F0C_ATTACK_WINDOW; break;
                case 1: case 4: attackDamageFrame(b, g, env); break;
                case 2: b.f0C &= ~(uint32_t)F0C_ATTACK_WINDOW; break;
                case 5: b.f0C &= ~(uint32_t)F0C_ATTACK_WINDOW; break;
                }
            }
            if (st == 5) b.f0C &= 0xEFFFFFFF;           // fin de la fase de dano: siempre quita "atacando"
            b.atkStep++;
        } else {
            if (b.motionFrame + b.rate < (float)e.endFrame) return;
            b.f1E14 &= ~(uint32_t)FB_ATTACK;             // el ataque ha terminado
            return;
        }
    }
}
// SetRunKindRand (0x9CD6xx): sortea variantes de carrera/ataque (2, 3 y 3 opciones).
void Boss4Logic::setRunKindRand(Boss4State& b, Boss4Env& env) {
    b.runA = (int8_t)(env.rand() & 1);
    b.runB = (int8_t)(env.rand() % 3);
    b.runD = (int8_t)(env.rand() % 3);
}
// DecNowEnemyAlvNumChk: descuenta una sola vez de "enemigos vivos" (gp-0x7370).
void Boss4Logic::decNowEnemyAlvNumChk(Boss4State& b, Boss4Globals& g) {
    if (!(b.f10 & 2)) { b.f10 |= 2; g.enemiesAlive--; }
}
// DecNowEnemyNumChk: descuenta una sola vez de "enemigos" (gp-0x7374).
void Boss4Logic::decNowEnemyNumChk(Boss4State& b, Boss4Globals& g) {
    if (!(b.f10 & 4)) { b.f10 |= 4; g.enemiesLeft--; }
}

// ---------------------------------------------------------------- alfa, clones, colisiones
// ControlAlpha (0x9C9xxx): fundido; sumando alphaStep; se detiene en 0 y en 1.
void Boss4Logic::controlAlpha(Boss4State& b) {
    if (b.f1E14 & FB_ALPHA_RUN) {
        float a = b.alpha + b.alphaStep; b.alpha = a;
        if (a < 0.0f) { b.alpha = 0.0f; b.f1E14 &= 0xFFEFFFFF; }
        if (!(b.alpha <= 1.0f)) { b.alpha = 1.0f; b.f1E14 &= 0xFFEFFFFF; }
    }
}
void Boss4Logic::startAlphaAdd(Boss4State& b) { b.f1E14 |= FB_ALPHA_RUN; b.alphaStep = bitsF(0x3CA3D70B); }   // +0.02
void Boss4Logic::startAlphaSub(Boss4State& b) { b.f1E14 |= FB_ALPHA_RUN; b.alphaStep = bitsF(0xBCA3D70B); }   // -0.02
// StartBunshin / StartShusoku / CancelBunshin: orden de dividirse en clones, reunirse, o cancelar.
void Boss4Logic::startBunshin(Boss4State& b, Boss4Env& env) {
    env.sound(0x120016, 0); b.f1E14 |= FB_BUNSHIN_ASK; b.f1E44 = 0x10;
    if (b.kind == K_BATTLE1) b.f1E14 &= ~(uint32_t)FB_VISIBLE;
}
void Boss4Logic::startShusoku(Boss4State& b, Boss4Env& env) { env.sound(0x120015, 0); b.f1E14 |= FB_SHUSOKU_ASK; b.f1E44 = 0x33; }
void Boss4Logic::cancelBunshin(Boss4State& b) {
    b.f1E14 &= 0xFFFF7FFF; b.f1E14 &= 0xFFFEFFFF; b.f1E44 = 0;
    if (b.kind == K_BATTLE1) b.f1E14 |= FB_VISIBLE;
}
// EnableCollision / DisableCollision: bit 0x8000 de +0x0C y, si no es el modo 0x200, el bit 0x40 de +0x1A8 de las 4 partes.
void Boss4Logic::enableCollision(Boss4State& b, Boss4Env& env) {
    b.f0C &= 0xFFFF7FFF;
    if (!(b.f1E14 & 0x200)) env.hook(H_ENABLE_PART_COLLISION, 0, 0, 0);
}
void Boss4Logic::disableCollision(Boss4State& b, Boss4Env& env) {
    b.f0C |= F0C_COLLISION_OFF;
    if (!(b.f1E14 & 0x200)) env.hook(H_DISABLE_PART_COLLISION, 0, 0, 0);
}
// FallCheck (0x9CD120): caida con gravedad -0.0392 hasta el suelo (bit 0x40 de +0x10 activo).
void Boss4Logic::fallCheck(Boss4State& b, float ground) {
    if (!(b.f10 & 0x40)) return;
    switch (b.fallStep) {
    case 0: b.accelY = bitsF(0xBD20902E); b.fallStep++; /* fallthrough */
    case 1:
        b.velY += b.accelY; b.pos[1] += b.velY;
        if (ground < b.pos[1] + b.velY) return;
        b.velY = 0; b.accelY = 0; b.pos[1] = ground; b.fallStep++;
        /* fallthrough */
    case 2:
        b.velY = 0; b.accelY = 0; b.f10 &= ~0x40u; b.fallStep = 0; break;
    default: break;
    }
}
// CharInit (0x9C9xxx): aplica la configuracion del tipo (kKindConfig), arranca el script y fija la vida.
void Boss4Logic::charInit(Boss4State& b, Boss4Globals& g, Boss4Env& env, int scaledLife) {
    env.hook(H_CHAR_INIT_CLEAR, 0, 0, 0);
    const KindConfig* cfg = b.kind < 5 ? &kKindConfig[b.kind] : nullptr;
    if (cfg) {
        b.f0C |= cfg->orC; b.f0C &= cfg->andC; b.f1E14 |= cfg->or1E14; b.f1E14 &= cfg->and1E14;
        if (cfg->seqTab) seqInit(b, cfg->seqTab);
        b.state = cfg->initState; b.stateB = 0; b.stateC = 0;
    }
    b.hitCounter = 1; b.hitsPerCount = 1;
    switch (b.kind) {
    case K_BATTLE1: initLife(b, g, scaledLife); break;
    case K_BATTLE2: initLife(b, g, -1); env.hook(H_INIT_GAREKI, 0, 0, 0); break;
    case K_BATTLE3:
        g.deathCount73FC = 0; g.deathCount73F8 = 0;
        if (g.gameState == 0xA && g.gameKind == 2) initLife(b, g, scaledLife); else initLife(b, g, -1);
        env.hook(H_INIT_KIRIKABE, 0, 0, 0);
        break;
    case K_BUNSHIN: b.f1E14 |= b.owner->f1E14; break;
    default: break;
    }
}

// ---------------------------------------------------------------- utilidades puras
// ETurn (0x9CD700): gira el angulo de 16 bits `cur` hacia `target` por el camino corto con paso `step`; si llega devuelve `target`.
int Boss4Logic::eTurn(int cur, int target, int step) {
    int t = target & 0xFFFF, a = cur & 0xFFFF;
    int d1 = a - t; if (d1 < 0) d1 = -d1;
    int d2 = (t + 0x10000) - a; if (d2 < 0) d2 = -d2;
    int d3 = (a + 0x10000) - t; if (d3 < 0) d3 = -d3;
    if (step >= d1 || step >= d2 || step >= d3) return t;
    if (a >= t) { if (d2 >= d1) step = -step; }
    else if (d3 < d1) step = -step;
    return a + step;
}
// AngChk (0x9CD7B0): 1 si el angulo `ang` esta a distancia <= `range` del centro `center` (ambos de 16 bits, con vuelta).
bool Boss4Logic::angChk(int ang, int center, int range) {
    uint32_t c = (uint32_t)(center & 0xFFFF);
    uint32_t lo = (c - (uint32_t)range) + 0x10000u, hi = c + (uint32_t)range + 0x10000u;
    if (hi < lo) { uint32_t t = lo; lo = hi; hi = t; }
    uint32_t a = (uint32_t)(ang & 0xFFFF);
    while (a < lo) a += 0x10000u;
    return a <= hi;
}

}}  // namespace vn::boss4

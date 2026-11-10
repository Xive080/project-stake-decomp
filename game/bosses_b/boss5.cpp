// TBoss5 (overlay bs5mod): reglas del jefe reescritas a mano. Cada funcion cita la original (simbolo y direccion).
#include "boss5.h"
#include <cstring>
namespace vn { namespace boss5 {

static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
static const float kRate = 1.2f;                       // 0x3F99999A: 60 Hz -> 50 Hz (PAL)
static const float kInv256 = 0.00390625f;              // 0x3B800000

// ---------------------------------------------------------------- VM de comandos
// SeqCmdInit (0x9CA3F0): fija la tabla de scripts y empieza por el primero.
void Boss5Logic::seqInit(Boss5State& b, const uint16_t* table) {
    b.seqTab = table; b.tabIdx = 0;
    b.pc = (table && table[0] != 0xFFFF) ? kScript + table[0] : nullptr;
    b.timerSel = 0; b.sp = 0; b.timer[b.sp] = -1;
}
// SeqCmdNextTable (0x9CA430): pasa al siguiente script de la tabla (el original no comprueba el final).
void Boss5Logic::seqNextTable(Boss5State& b) {
    b.tabIdx++;
    uint16_t e = b.seqTab ? b.seqTab[b.tabIdx] : 0xFFFF;
    b.pc = e != 0xFFFF ? kScript + e : nullptr;
    b.timerSel = 0;
}
// SeqCmdSetTable (0x9CA460): salta al script `idx` y vacia la pila.
void Boss5Logic::seqSetTable(Boss5State& b, int idx) {
    b.tabIdx = (int16_t)idx;
    uint16_t e = b.seqTab ? b.seqTab[(int16_t)idx] : 0xFFFF;
    b.pc = e != 0xFFFF ? kScript + e : nullptr;
    b.timerSel = 0; b.sp = 0; b.timer[b.sp] = -1;
}
// SeqCmdPush (0x9CA4A0): guarda el pc actual y abre un contador nuevo.
void Boss5Logic::seqPush(Boss5State& b) {
    b.stack[(int)b.sp] = b.pc; b.sp++; b.timer[(int)b.sp] = -1;
}
// SeqCmdPop (0x9CA4E0): vuelve al pc guardado (no reinicia el contador).
void Boss5Logic::seqPop(Boss5State& b) { b.sp--; b.pc = b.stack[(int)b.sp]; }
// SeqCmdCall (0x9CA510): interrumpe el script actual con un script de reaccion (gp-0x7930[i]).
void Boss5Logic::seqCall(Boss5State& b, int callScript) { seqPush(b); b.pc = kScript + kCallScript[(int16_t)callScript]; }
// SeqCmdCancelWait (0x9CA560): si el comando actual es 5, 6 o 7 (esperas de movimiento) lo salta con su longitud.
void Boss5Logic::seqCancelWait(Boss5State& b) {
    int op = b.pc[0];
    if (op != 7 && op != 6 && op != 5) return;
    b.pc += kCmdLen[op];
}
// InitBranchParam (0x9CA5E0)
void Boss5Logic::initBranchParam(Boss5State& b) { b.branch[0] = b.branch[1] = b.branch[2] = 0; }
// actExecSeqCmd (0x9CA5C0): ejecuta el interprete y devuelve 0.
int Boss5Logic::actExecSeqCmd(Boss5State& b, Boss5Globals& g, Boss5Env& env) { execSeqCmd(b, g, env); return 0; }

// CheckSwimMotion (0x9CAA70): marca/desmarca el modo "nado" segun el movimiento actual.
void Boss5Logic::checkSwimMotion(Boss5State& b) {
    if (b.motion == 0x1DC || b.motion == 0x1DA || b.motion == 0x1D8) b.f1E40 |= FB_SWIM;
    else b.f1E40 &= ~(uint32_t)FB_SWIM;
}
// MotSupSet2 (0x9CAAD0): pide un movimiento distinto del actual y refresca el modo de nado.
void Boss5Logic::motSupSet2(Boss5State& b, Boss5Env& env, int motion, int start, int blend) {
    if (b.motion != motion) env.playMotion(motion, start, blend);
    checkSwimMotion(b);
}

// ExecSeqCmd (0x9C97D0..0x9CA3E8): interprete de scripts del jefe. Ejecuta comandos hasta que uno pide esperar.
// Tabla de saltos jtbl_009DBD90 (47 comandos). Los comandos que solo llaman al motor van por env.hook (ver enum Hook).
void Boss5Logic::execSeqCmd(Boss5State& b, Boss5Globals& g, Boss5Env& env) {
    for (;;) {
        if (!b.pc) return;
        const int16_t* c = b.pc;
        unsigned op = (uint16_t)c[0];
        if (op >= 47) return;                       // el original avanza con la tabla de longitudes fuera de rango; no ocurre con datos validos
        int wait = 0;
        switch (op) {
        case 0: wait = 1; break;                    // espera eterna
        case 1: b.state = TS_FINISHED; b.stateB = 0; b.stateC = 0; break;
        case 2: wait = 1; break;                    // (9CA39C) igual que 0
        case 3: {   // espera de camara: avanza solo con cam en [P, P+1)
            float p = (float)c[1], cam = env.camFrame();
            if (cam < p) wait = 1; else if (!(cam < 1.0f + p)) wait = 1;
            break; }
        case 4: {   // puerta de camara: dentro de [P, P+1) prohibe iniciar camara y avanza; fuera permite y espera
            float p = (float)c[1], cam = env.camFrame();
            if (cam < p || !(cam < 1.0f + p)) { allowCameraStart(g); wait = 1; }
            else forbidCameraStart(g);
            break; }
        case 5: if (b.motionFrame <= (float)c[1]) wait = 1; break;                          // espera fotograma > P
        case 6: if (b.motionFrame <= (float)env.motionLen(b.motion)) wait = 1; break;       // espera fin del movimiento
        case 7: if (b.motionFrame < (float)(c[1] * env.motionLen(b.motion))) wait = 1; break;
        case 8: if (b.f0C & F0C_ATTACKING) wait = 1; break;
        case 9: {   // espera P fotogramas (sin conversion PAL en este jefe)
            int16_t& t = b.timer[(int)b.sp];
            if (t == -1) t = c[1];
            int16_t v = t; t = (int16_t)(v - 1);
            if (v != 0) wait = 1;
            break; }
        case 10: if (b.f10 & 0x40) wait = 1; break;                                         // espera a que acabe la caida
        case 11: if (b.pathPos < b.pathLimitA) wait = 1; break;                             // 1DE0 < 1DDC
        case 12: if (c[1] < g.enemiesLeft) wait = 1; break;                                 // espera a quedar <= P enemigos
        case 13: if (b.pathPos < b.pathLimitB) wait = 1; break;                             // 1DE0 < 1DE8
        case 14: {  // espera a que el bit c[1] de +0x0C valga (c[2] != 0)
            bool set = (b.f0C & (1u << (c[1] & 31))) != 0;
            if (c[2] ? !set : set) wait = 1;
            break; }
        case 15: {  // idem sobre +0x1E40
            bool set = (b.f1E40 & (1u << (c[1] & 31))) != 0;
            if (c[2] ? !set : set) wait = 1;
            break; }
        case 16:    // espera a una ruta de camara concreta y a que llegue a su fotograma
            if (env.camTrackId() != b.camTrackId) wait = 1;
            else if (env.camFrame() < (float)b.camTrackFrame) wait = 1;
            break;
        case 17: seqSetTable(b, c[1]); break;
        case 18: seqNextTable(b); b.phaseCount++; break;
        case 19: seqPop(b); wait = 1; break;
        case 20: seqPop(b); seqCancelWait(b); wait = 1; break;
        case 21: {  // seleccion aleatoria de rama: branch[0] = c1 + c3 * (rand % c2); luego dos aleatorios mas
            int r = env.rand();
            int m = c[2] ? r % c[2] : 0;            // el original trapa (break 7) con c[2] == 0
            b.branch[0] = (int16_t)(c[1] + c[3] * m);
            b.branch[1] = (int16_t)env.rand();
            b.branch[2] = (int16_t)env.rand();
            seqSetTable(b, b.branch[0]);
            break; }
        case 22: break;
        case 23: motSupSet2(b, env, c[1], c[2], c[3]); break;
        case 24: {  // velocidad base de reproduccion = 1.2 * P/256
            float f = kRate * (kInv256 * (float)c[1]);
            b.rateBase = f; b.rate = f * b.rateMul; break; }
        case 25: env.hook(H_CLEAR_BODY_MOVE, 0x12, 0, 0); break;
        case 26: env.hook(H_SET_BODY_MOVE, 0x12, 0, 0); break;
        case 27: env.hook(H_CLEAR_BODY_MOVE, 0x10, 0, 0); break;
        case 28: env.hook(H_SET_BODY_MOVE, 0x10, 0, 0); break;
        case 29: selectAttackTarget(b, g, env); startAttack(b, g, env, (int8_t)c[1]); break;
        case 30:    // ataque directo al jugador elegido: si le da baja la dificultad; si no, desarma
            selectAttackTarget(b, g, env);
            if (b.target >= 0) {
                if (env.damagePlayer(b.target, 1, 7) == 1) downDifficulty(b); else b.diffArmed = 0;
            }
            break;
        case 31: env.setCamSpeed(kRate * (kInv256 * (float)c[1])); break;
        case 32: b.pathMode = (int8_t)c[1]; break;
        case 33: env.hook(H_SET_PATH, c[1], 0, 0); break;
        case 34: b.pathSpeed = kRate * (kInv256 * (float)c[1]); break;
        case 35:
            if (c[1] > 0) {
                b.f1DF4 = 0.0f; b.f1DF8 = 1.0f / (float)c[1];
                env.hook(H_PATH_CAM_OFFSET, c[1], 0, 0);   // calcula +0x1DFC (matrices del motor)
                b.camRotSave[0] = b.rot[0]; b.camRotSave[1] = b.rot[1]; b.camRotSave[2] = b.rot[2];
            } else { b.f1DF4 = 1.0f; b.f1DF8 = 0.0f; }
            b.pathMode = 1;
            break;
        case 36: b.pathMode = 0; break;
        case 37:    // pone/quita el bit c1 de +0x0C
            if (c[2]) b.f0C |= 1u << (c[1] & 31); else b.f0C &= ~(1u << (c[1] & 31));
            break;
        case 38:    // pone/quita el bit c1 de +0x1E40; 0x13 -> InitWaitPath, 0x12 -> InitSippoPos (solo al ponerlo); bit 0xF suena siempre
            if (c[2]) {
                b.f1E40 |= 1u << (c[1] & 31);
                if (c[1] == 0x13) env.hook(H_INIT_WAIT_PATH, 0, 0, 0);
                else if (c[1] == 0x12) env.hook(H_INIT_SIPPO_POS, 0, 0, 0);
            } else b.f1E40 &= ~(1u << (c[1] & 31));
            if (c[1] == 0xF) env.hook(H_BIT_0F_SOUND, 0, 0, 0);
            break;
        case 39: if (c[1]) b.f1E40 &= ~0x400u; else b.f1E40 |= 0x400u; break;   // (invertido: c1 != 0 quita el bit)
        case 40:    // reinicia el contador de impactos, quita "en dano" y la ventana de ataque, y re-arma la dificultad
            b.hitCounter = b.hitsPerCount; b.f0C &= ~(uint32_t)F0C_IN_DAMAGE; b.f1E40 &= ~(uint32_t)FB_ATK_WINDOW;
            if (b.diffArmed) upDifficulty(b);
            setDiffParam(b, g); b.diffArmed = 1; break;
        case 41: enableCollision(b, env); break;
        case 42: disableCollision(b, env); break;
        case 43: env.hook(H_START_WEAK_DISP, 0, 0, 0); break;
        case 44: if (c[1]) b.f1E40 |= FB_LIFEBAR; else b.f1E40 &= ~(uint32_t)FB_LIFEBAR; break;
        case 45:    // coloca al jefe en una pose fija de kPose (c1 >= 0) o en la de camara (c1 == -1)
            if (c[1] >= 0) {
                const float* p = kPose[c[1] & 1];
                b.pos[0] = p[0]; b.pos[1] = p[1]; b.pos[2] = p[2];
                b.rot[0] = p[3]; b.rot[1] = p[4]; b.rot[2] = p[5];
            } else if (c[1] == -1) {
                const float* p = kPose[1];
                b.rot[0] = p[3]; b.rot[1] = (float)env.camRot(); b.rot[2] = p[5];
                env.hook(H_POSE_ROTATE, 0, 0, 0);          // func_1AFC00(D_009CE868, &pos): rota el vector y lo escribe en pos
            }
            break;
        case 46: b.sel = (int8_t)c[1]; break;
        }
        if (!wait) { b.pc += kCmdLen[op]; b.timerSel = 0; }
        if (wait) return;
    }
}

// ---------------------------------------------------------------- dificultad
// InitDifficulty (0x9C7BE0): la dificultad inicial sale de func_96CEC0().
void Boss5Logic::initDifficulty(Boss5State& b, int initial) { b.difficulty = initial; b.diffArmed = 0; }
// SetDiffParam (0x9C7B20): vida de fase, multiplicador de dano y pasos segun nivel y dificultad.
void Boss5Logic::setDiffParam(Boss5State& b, Boss5Globals& g) {
    int lvl = (int16_t)g.gameLevel, d = b.difficulty;
    int8_t life = kPartLife[b.sel][lvl][d];
    b.partLife = life; b.partLifeMax = life;
    b.dmgMul = kDmgMul[lvl][d];
    b.diffUp = kDiffStep[lvl][0]; b.diffDown = kDiffStep[lvl][1];
    g.diffMirror = b.difficulty;
}
// UpDifficulty (0x9C7C40): sube `diffUp`, tope 16.
void Boss5Logic::upDifficulty(Boss5State& b) { b.difficulty += b.diffUp; if (b.difficulty >= 17) b.difficulty = 16; }
// DownDifficulty (0x9C7C70): baja `diffDown`, suelo 0, y desarma la subida.
void Boss5Logic::downDifficulty(Boss5State& b) { b.difficulty -= b.diffDown; if (b.difficulty < 0) b.difficulty = 0; b.diffArmed = 0; }
// DownDifficultyReq (0x9C7C10): envoltorio de DownDifficulty (llamado por los proyectiles).
void Boss5Logic::downDifficultyReq(Boss5State& b) { downDifficulty(b); }
// NoChgDifficultyReq (0x9C7C30)
void Boss5Logic::noChgDifficultyReq(Boss5State& b) { b.diffArmed = 0; }
// GetStrength (0x9C6710): tira un aleatorio 0..1 contra los dos primeros umbrales de la fila [nivel][dificultad]: 0, 1 o 2.
int Boss5Logic::getStrength(const Boss5State& b, const Boss5Globals& g, Boss5Env& env) {
    float r = env.randFloat();
    const float* row = kStrength[(int16_t)g.gameLevel][b.difficulty];
    float t0 = row[0], t1 = row[0] + row[1];
    if (r < t0) return 0;
    return r < t1 ? 1 : 2;
}

// ---------------------------------------------------------------- vida / dano / muerte
// InitLife (0x9C7860): fija la vida compartida (400 por defecto si no es valida).
void Boss5Logic::initLife(Boss5State& b, Boss5Globals& g, int life) {
    life = (int16_t)life;
    if (life > 0) { g.bossLife = life; g.bossLifeMax = life; }
    if (g.bossLifeMax <= 0 || g.bossLife <= 0) { g.bossLife = 0x190; g.bossLifeMax = 0x190; }
    b.lifeRatio = 1.0f; b.hitRatio = 1.0f;
}
// ControlDispLife (0x9C7900): solo la parte numerica (las dos barras se dibujan con func_948FB0).
void Boss5Logic::controlDispLife(Boss5State& b, const Boss5Globals& g) {
    b.lifeRatio = (float)g.bossLife / (float)g.bossLifeMax;
    b.hitRatio = (float)b.partLife / (float)b.partLifeMax;
}
// CheckWeakHit (0x9C8C60): solo el tipo 1 y zonas < 12.
bool Boss5Logic::checkWeakHit(const Boss5State& b, int partType) { return b.kind == K_BATTLE2 && partType < 12; }
// CheckDamage (0x9C8CE0)
void Boss5Logic::checkDamage(Boss5State& b, Boss5Globals& g, Boss5Env& env, const HitInfo hit[2]) {
    if (!(b.f0C & F0C_HIT)) return;
    env.hook(H_HIT_PREPARE, 0, 0, 0);
    if (!(b.f0C & 0x4008100)) {
        for (int i = 0; i < 2; i++) {
            if (hit[i].slot == -1) continue;
            int part = hit[i].partType;
            if (part <= 0) continue;
            b.weakIdx = hit[i].weakIdx;
            if ((b.f0C & F0C_WEAK) || checkWeakHit(b, part)) {
                weakHit(b, g, env, part, hit[i].slot);
                env.hook(H_HIT_CONSUMED, hit[i].slot, 0, 0);
            } else noWeakHit();
        }
    }
    env.hook(H_CLEAR_WEAK_HIT, 0, 0, 0);
}
// WeakHit (0x9C8E20): descuenta vida compartida y de fase; 0 = nada, 1 = golpe superior, 2 = dano, 3 = muerte.
void Boss5Logic::weakHit(Boss5State& b, Boss5Globals& g, Boss5Env& env, int partType, int hitIdx) {
    if (b.f0C & 0x40008100) return;
    if (b.f1E40 & FB_HIT_STUN) return;
    if (--b.hitCounter > 0) return;
    int result = 0;
    b.hitCounter = b.hitsPerCount;
    env.registerHit(hitIdx, 10);
    g.bossLife -= (int)((float)b.dmgMul * env.damageScale((int8_t)hitIdx));
    if (g.bossLife <= 0) { g.bossLife = 0; result = 3; }
    if (--b.partLife <= 0) { if (result < 2) result = 2; }
    else if (result <= 0) result = 1;
    if (result == 3) { dead(b, g, env, hitIdx); env.hook(H_HIT_MARK2, hitIdx, partType, 0); }
    else if (result == 2) { damage(b, g, env); env.hook(H_HIT_MARK1, hitIdx, 0, 0); }
    else if (result == 1) { damageUpperBody(b, env); env.hook(H_HIT_MARK2, hitIdx, partType, 0); }
}
// NoWeakHit (0x9C8FE0): vacia (solo `jr ra`).
void Boss5Logic::noWeakHit() {}
// DamageUpperBody (0x9C8FF0): golpe que no baja de fase; el tipo 1 suena, saca efecto y queda aturdido 25 fotogramas.
void Boss5Logic::damageUpperBody(Boss5State& b, Boss5Env& env) {
    if (b.kind == K_BATTLE2) {
        env.sound(0x1A0013, 0x40);
        if (b.motion != 0x1DC && !(b.f1E40 & FB_ATK_WINDOW)) {
            env.hook(H_UPPER_HIT_FX, (b.f1E40 & FB_SWIM) ? 1 : 0, 0, 0);   // inchiki_dammot_set + efecto 0x1DE
        }
        b.f1E40 |= FB_HIT_STUN; b.stunTimer = 0x19;
    }
    b.f1E40 |= FB_HIT_STUN;
}
// SetDamageMotion (0x9C9490): tipo 1 -> 0x1DD (si nada) o 0x1E8, mezcla 10; otros tipos no cambian de movimiento.
void Boss5Logic::setDamageMotion(Boss5State& b, Boss5Env& env) {
    if (b.kind == K_BATTLE2) env.playMotion((b.f1E40 & FB_SWIM) ? 0x1DD : 0x1E8, 0, 10);
}
// Damage (0x9C9340): pierde una fase: avanza el script y llama al de reaccion; velocidad de reproduccion a la base 1.2.
void Boss5Logic::damage(Boss5State& b, Boss5Globals&, Boss5Env& env) {
    if (b.kind == K_BATTLE2) {
        if (b.motion == 0x1DC || b.motion == 0x1DA || b.motion == 0x1D8) b.f1E40 |= FB_SWIM;
        else b.f1E40 &= ~(uint32_t)FB_SWIM;
        env.sound(0x1A0011, 0x40);
        seqNextTable(b); b.phaseCount++;
        seqCall(b, (b.f1E40 & FB_SWIM) ? 1 : 0);
    }
    b.rateBase = kRate; b.rateMul = 1.0f; b.rate = b.rateBase * 1.0f;   // (1EDC se escribe dos veces; el resultado es 1.2)
    setDamageMotion(b, env);
    b.f0C |= F0C_IN_DAMAGE;
}
// Dead (0x9C9520)
void Boss5Logic::dead(Boss5State& b, Boss5Globals& g, Boss5Env& env, int hitIdx) {
    if (!(g.gameState == 0xA && g.gameKind == 2)) {
        if (!env.statFunc(2, 6) && !g.deathCount73FC) env.statFunc(2, 7);
        if (!g.deathCount73F8) env.statFunc(2, 0xD);
    }
    b.f1E40 &= 0xFFFEDFFFu;
    env.registerHit(hitIdx, 0x5DC);
    env.sound(0x1A0011, 0x40);
    env.hook(H_OWNER_CLEAR_FLAGS, 0, 0, 0);
    b.f1E40 &= ~0x200u;
    b.f0C |= F0C_DEAD;
    setDamageMotion(b, env);
    b.state = TS_DEAD; b.stateB = 0; b.stateC = 0;
}
// actDelete (0x9C9650)
int Boss5Logic::actDelete(Boss5State& b, Boss5Globals& g, Boss5Env& env) {
    if (b.stateB == 0) {
        g.targetBusy[0] = 0; g.targetBusy[1] = 0;
        b.f0C |= F0C_DEAD; b.f0C |= F0C_DELETE; b.f10 |= 0x20; b.f18 = 0;
        b.stateB++;
    } else if (b.stateB == 1) env.hook(H_FINISH_TASK, 0, 0, 0);
    return 0;
}
// actDead (0x9C96E0)
int Boss5Logic::actDead(Boss5State& b, Boss5Globals& g, Boss5Env& env) {
    if (b.stateB == 0) {
        if (b.motionFrame < (float)env.motionLen(b.motion)) return 0;
        b.rateBase = 0; b.rate = 0.0f * b.rateMul; b.stateB++;
    } else if (b.stateB == 1) {
        if (env.camTrackId() == 0x27F) { b.state = TS_FINISHED; b.stateB = 0; b.stateC = 0; }
        else {
            g.g73CC = 1;                                   // el delay slot lo escribe siempre
            if (g.g729C) g.g72A0 = 1;
            allowCameraStart(g);
        }
    }
    return 0;
}
// CheckDamageEnd (0x9C7250, solo el bloque inicial): cuenta atras del aturdimiento; al llegar a 0 quita el bit y restaura la velocidad.
void Boss5Logic::checkDamageEndTimer(Boss5State& b) {
    if (b.f1E40 & FB_HIT_STUN) {
        b.stunTimer = (int16_t)(b.stunTimer - 1);
        if (b.stunTimer == 0) { b.f1E40 &= ~(uint32_t)FB_HIT_STUN; b.rateMul = 1.0f; b.rate = b.rateBase * 1.0f; }
    }
}

// ---------------------------------------------------------------- ataques
// SelectAttackTarget (0x9CAB30)
bool Boss5Logic::selectAttackTarget(Boss5State& b, Boss5Globals& g, Boss5Env& env) {
    int lvl = (int16_t)g.gameLevel;
    if (lvl == 0) b.target = g.lastHitTarget;
    else if (lvl == 1) {
        int r = env.rand() & 1;
        if (g.targetBusy[r]) { r = (r + 1) & 1; if (g.targetBusy[r]) { g.targetBusy[1] = 0; g.targetBusy[0] = 0; } }
        b.target = (int8_t)r;
    }
    if (!env.playerAlive(b.target)) b.target = -1;
    if (b.target == -1) return false;
    g.targetBusy[(int)b.target] = 1;
    return true;
}
// StartAttack (0x9C64C0)
void Boss5Logic::startAttack(Boss5State& b, Boss5Globals& g, Boss5Env& env, int id) {
    b.attack = (int8_t)id; b.atkStep = 0; b.s1F48 = 0;
    b.numPlayers = g.numPlayersB;
    if (b.kind == K_BATTLE1 || b.kind == K_BATTLE2) motSupSet2(b, env, kAttackTable[b.attack].motion, 0, 5);
    b.f0C |= F0C_ATTACKING; b.f1E40 |= FB_ATTACK;
}
// ControlAttack (0x9C61E0): recorre los hitos de la fila del ataque (jtbl_009DBD50, 7 pasos); cada uno actua cuando motionFrame >= su fotograma.
void Boss5Logic::controlAttack(Boss5State& b, Boss5Globals& g, Boss5Env& env) {
    if (!(b.f1E40 & FB_ATTACK)) return;
    const AttackEntry& e = kAttackTable[(int)b.attack];
    if (b.motion != e.motion) { b.f0C &= ~(uint32_t)F0C_ATTACKING; b.f1E40 &= ~(uint32_t)FB_ATTACK; return; }
    while ((uint8_t)b.atkStep <= 6) {
        int s = b.atkStep;
        if (s == 6) {   // fin: motionFrame + rate >= endFrame
            if (b.motionFrame + b.rate < (float)e.endFrame) return;
            b.f1E40 &= ~(uint32_t)FB_ATTACK;
            return;
        }
        if (b.motionFrame < (float)e.f[s]) return;
        bool use = e.f[s] >= 0;
        switch (s) {
        case 0: if (use) b.f1E40 |= FB_ATK_WINDOW; break;
        case 1: if (use) attackDamageFrame(b, g, env, 0); break;
        case 2: if (use) b.f1E40 &= ~(uint32_t)FB_ATK_WINDOW; break;
        case 3: if (use) b.f1E40 |= FB_ATK_WINDOW; break;
        case 4: if (use) attackDamageFrame(b, g, env, 1); break;
        case 5: if (use) b.f1E40 &= ~(uint32_t)FB_ATK_WINDOW; b.f0C &= ~(uint32_t)F0C_ATTACKING; break;
        }
        b.atkStep++;
    }
}
// AttackDamageFrame (0x9C6180): segun el ataque, lanza la esfera (0), el hielo grande (1, solo en el primer hito) o dano directo.
void Boss5Logic::attackDamageFrame(Boss5State& b, Boss5Globals& g, Boss5Env& env, int which) {
    if (b.attack == 1) { if (which == 0) env.hook(H_SPAWN_XICE, 0, 0, 0); }
    else if (b.attack == 0) env.hook(H_SPAWN_SPHERE, 0, 0, 0);
    else damageToPlayer(b, g, env);
}
// DamageToPlayer (0x9CAC70)
void Boss5Logic::damageToPlayer(Boss5State& b, Boss5Globals&, Boss5Env& env) {
    if (b.target < 0) return;
    if (env.damagePlayer(b.target, 1, kAttackTable[(int)b.attack].dmg) == 1) { downDifficulty(b); env.sound(0x8000C6, 0x40); }
    else b.diffArmed = 0;
}
// DecNowEnemyAlvNumChk (0x9CAC10) / DecNowEnemyNumChk (0x9CAC40): descuentan una sola vez del contador global.
void Boss5Logic::decNowEnemyAlvNumChk(Boss5State& b, Boss5Globals& g) { if (!(b.f10 & 2)) { b.f10 |= 2; g.enemiesAlive--; } }
void Boss5Logic::decNowEnemyNumChk(Boss5State& b, Boss5Globals& g) { if (!(b.f10 & 4)) { b.f10 |= 4; g.enemiesLeft--; } }

// ---------------------------------------------------------------- varios
// ControlAlpha (0x9C6670)
void Boss5Logic::controlAlpha(Boss5State& b) {
    if (b.f1E40 & FB_ALPHA_OUT) {
        if (b.alpha <= 0.0f) return;
        float t = b.alpha - b.alphaStep; b.alpha = t;
        if (t < 0.0f) b.alpha = 0.0f;
    } else {
        if (!(b.alpha < 1.0f)) return;
        float t = b.alpha + b.alphaStep; b.alpha = t;
        if (!(t <= 1.0f)) b.alpha = 1.0f;
    }
}
// EnableCollision (0x9CA7E0) / DisableCollision (0x9CA840)
void Boss5Logic::enableCollision(Boss5State& b, Boss5Env& env) {
    b.f0C &= ~(uint32_t)F0C_COLLISION_OFF;
    if (!(b.f1E40 & FB_NO_PART_COLLISION)) env.hook(H_ENABLE_PART_COLLISION, 0, 0, 0);
}
void Boss5Logic::disableCollision(Boss5State& b, Boss5Env& env) {
    b.f0C |= F0C_COLLISION_OFF;
    if (!(b.f1E40 & FB_NO_PART_COLLISION)) env.hook(H_DISABLE_PART_COLLISION, 0, 0, 0);
}
// SetHeight (0x9CA8A0): pega el jefe al suelo (+heightOffset); con el movimiento 0x17C, si esta por encima empieza a caer.
void Boss5Logic::setHeight(Boss5State& b, Boss5Env& env) {
    float h = b.heightOffset + env.groundY(b.pos);
    if (b.motion == 0x17C) {
        if (b.pos[1] <= h) return;
        if (b.f10 & 0x40) return;
        b.f10 |= 0x40; b.fallStep = 0;
    } else if (!(b.f10 & 0x40)) b.pos[1] = h;
}
// FallCheck (0x9CA960): caida con gravedad -0.0391 (0xBD20902E) hasta el suelo.
void Boss5Logic::fallCheck(Boss5State& b, float ground) {
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
// CharInit (0x9C79D0): aplica la configuracion del tipo (kKindConfig), arranca el script y fija la vida.
void Boss5Logic::charInit(Boss5State& b, Boss5Globals& g, Boss5Env& env, int scaledLife) {
    env.hook(H_CHAR_INIT_CLEAR, 0, 0, 0);
    const KindConfig* cfg = b.kind < 2 ? &kKindConfig[b.kind] : nullptr;
    if (cfg) {
        b.f0C |= cfg->orC; b.f0C &= cfg->andC; b.f1E40 |= cfg->or1E40; b.f1E40 &= cfg->and1E40;
        if (cfg->seqTab) seqInit(b, cfg->seqTab);
        b.state = cfg->initState; b.stateB = 0; b.stateC = 0;
    }
    b.hitCounter = 1; b.hitsPerCount = 1;
    if (b.kind == K_BATTLE1) {
        b.alpha = 0.0f; b.alphaStep = bitsF(0x3CA3D70B);
        initLife(b, g, 0x64); b.kind74 = 0;
    } else if (b.kind == K_BATTLE2) {
        g.deathCount73FC = 0; g.deathCount73F8 = 0;
        initLife(b, g, scaledLife); b.kind74 = 1;
    }
}
// ControlWaterPillar (0x9CC030): maquina de 3 estados de los 6 pilares de agua (el movimiento de cada pilar es de Start__17TBoss5WaterPillar).
void Boss5Logic::controlWaterPillar(Boss5State& b, const Boss5Globals& g, Boss5Env& env) {
    if (b.f1E40 & FB_PILLARS) {
        switch (b.pillarState) {
        case 0:
            b.pillarPhase = 0; b.pillarRise = 0;
            b.pillarBase = kPillarSpeed[(int16_t)g.gameLevel][b.difficulty]; b.pillarTarget = b.pillarBase;
            b.pillarState = 1;
            for (int i = 0; i < 6; i++) env.hook(H_PILLAR_START, i, 0, 0);
            break;
        case 1: b.pillarPhase = (int16_t)(b.pillarPhase + b.pillarRise); break;
        case 2:
            b.pillarState = 0;
            for (int i = 0; i < 6; i++) env.hook(H_PILLAR_START, i, 0, 0);
            break;
        default: break;
        }
        if (b.pillarRise < 0x28F) {
            b.pillarRise = (int16_t)(b.pillarRise + 0x15);
            if (b.pillarRise >= 0x290) b.pillarRise = 0x28F;
        }
    } else {
        switch (b.pillarState) {
        case 0: b.pillarState = 2; break;
        case 1: b.pillarState = 2; break;
        case 2: b.pillarState = 0; b.f1E40 &= 0xFFFEFFFFu; break;
        default: break;
        }
    }
    // muelle amortiguado hacia pillarTarget (constantes 0x3C23D70A = 0.01 y 0x3F7D70A4 = 0.99)
    b.pillarAcc = bitsF(0x3C23D70A) * (b.pillarTarget - b.pillarBase);
    b.pillarVel = (b.pillarVel + b.pillarAcc) * bitsF(0x3F7D70A4);
    b.pillarBase = b.pillarBase + b.pillarVel;
}
// ETurn (0x9CAD50): gira un angulo de 16 bits hacia el objetivo (mismo resultado que la version de TBoss4).
int Boss5Logic::eTurn(int cur, int target, int step) {
    int t = target & 0xFFFF, a = cur & 0xFFFF;
    int d1 = a - t; if (d1 < 0) d1 = -d1;
    int d2 = (t + 0x10000) - a; if (d2 < 0) d2 = -d2;
    int d3 = (a + 0x10000) - t; if (d3 < 0) d3 = -d3;
    if (step >= d1 || step >= d2 || step >= d3) return t;
    if (a >= t) { if (d2 >= d1) step = -step; }
    else if (d3 < d1) step = -step;
    return a + step;
}
// AngChk (0x9CAE00): 1 si `ang` esta a distancia <= `range` de `center` (angulos de 16 bits, con vuelta).
bool Boss5Logic::angChk(int ang, int center, int range) {
    uint32_t c = (uint32_t)(center & 0xFFFF);
    uint32_t lo = (c - (uint32_t)range) + 0x10000u, hi = c + (uint32_t)range + 0x10000u;
    if (hi < lo) { uint32_t t = lo; lo = hi; hi = t; }
    uint32_t a = (uint32_t)(ang & 0xFFFF);
    while (a < lo) a += 0x10000u;
    return a <= hi;
}

// ---------------------------------------------------------------- proyectiles de hielo (TBoss5Ice)
// CheckAttackEnd (0x9CD600): al alcanzar al objetivo (dist < 5 + radio) le hace dano (7) y acaba; mas de 200 de distancia tambien acaba.
void IceLogic::checkAttackEnd(IceState& s, Boss5State& owner, Boss5Env& env, float dist) {
    if (dist < 5.0f + s.homeDist) {
        if (s.target >= 0) {
            if (env.damagePlayer(s.target, 1, 7) == 1) Boss5Logic::downDifficultyReq(owner);
            else Boss5Logic::noChgDifficultyReq(owner);
        }
        s.finished++;
    }
    if (!(dist <= 200.0f)) s.finished++;
}
static void addVel(IceState& s) { for (int i = 0; i < 3; i++) s.pos[i] += s.vel[i]; }
// Attack_sphere (0x9CD6D0) / Attack_tagger: pos += vel y comprobar fin.
void IceLogic::attackSphere(IceState& s, Boss5State& o, Boss5Env& env, float d) { addVel(s); checkAttackEnd(s, o, env, d); }
void IceLogic::attackTagger(IceState& s, Boss5State& o, Boss5Env& env, float d) { addVel(s); checkAttackEnd(s, o, env, d); }
// Attack_chaku (0x9CD7E0): vel += acc; pos += vel.
void IceLogic::attackChaku(IceState& s, Boss5State& o, Boss5Env& env, float d) {
    for (int i = 0; i < 3; i++) s.vel[i] += s.acc[i];
    addVel(s); checkAttackEnd(s, o, env, d);
}
// UpShape (0x9CD8B0): sube hasta baseY y pasa al estado 4.
void IceLogic::upShape(IceState& s) {
    s.pos[1] += s.vel[1];
    if (!(s.pos[1] < s.baseY)) { s.shapeState = 4; s.pos[1] = s.baseY; }
}
// Morph (0x9CD8F0): t = clamp((timer-25)/50, 0, 1); morph = {1-t, t, 1-t, t}.
void IceLogic::morph(IceState& s) {
    float t = (float)(s.morphTimer - 0x19) / 50.0f;
    if (t < 0.0f) t = 0.0f; else if (!(t <= 1.0f)) t = 1.0f;
    s.morph[0] = 1.0f - t; s.morph[2] = 1.0f - t; s.morph[1] = t; s.morph[3] = t;
}

}}  // namespace vn::boss5

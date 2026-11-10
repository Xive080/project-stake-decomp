#include "boss3.h"
#include <cstring>
namespace vn { namespace boss {

// ---------------------------------------------------------------- dificultad
// InitBS3Difficulty__Fv (bs3mod 0x9C4EC0): fila de parametros segun el ajuste (func_96CED0) y dificultad inicial de ambos jugadores (func_96CEC0).
void Boss3Logic::initDifficulty(Bs3Env& e, Boss3Engine& g, int settingIdx) {
    e.phaseParamIdx = settingIdx;
    int d = g.startDifficulty();
    e.diff[0] = d; e.diff[1] = d;
    e.difficultyShown = e.diff[0];
}
// IncBS3Difficulty__9Boss3TaskFiii (0x9C7E30): suma a ambos contadores (el segundo parametro si 0x500DFC != 2, el tercero si es 2); tope 16.
void Boss3Logic::incDifficulty(Bs3Env& e, int normal, int twoPlayer) {
    int a = (e.nPlayersB == 2) ? twoPlayer : normal;
    e.diff[0] += a; e.diff[1] += a;
    if (e.diff[0] >= 0x11) e.diff[0] = 0x10;
    if (e.diff[1] >= 0x11) e.diff[1] = 0x10;
    e.difficultyShown = e.diff[0];
}
// DecBS3Difficulty__9Boss3TaskFiii (0x9C7EB0): idem restando; suelo 0.
void Boss3Logic::decDifficulty(Bs3Env& e, int normal, int twoPlayer) {
    int a = (e.nPlayersB == 2) ? twoPlayer : normal;
    e.diff[0] -= a; e.diff[1] -= a;
    if (e.diff[0] < 0) e.diff[0] = 0;
    if (e.diff[1] < 0) e.diff[1] = 0;
    e.difficultyShown = e.diff[0];
}
// GetBS3Difficulty__9Boss3TaskFi (0x9C7F30): indice 0..33 para las tablas de danio/balas: un jugador = su dificultad; dos = media + 0x11.
int Boss3Logic::getDifficulty(Bs3Env& e) {
    e.difficultyShown = e.diff[0];
    switch (e.nPlayers) {
    case 1: return e.diff[e.activePlayer];
    case 2: return (e.diff[0] + e.diff[1]) / 2 + 0x11;     // el original corrige el redondeo de negativos; aqui siempre >= 0
    default: return 0;
    }
}

// ---------------------------------------------------------------- vida
// InitLife__9Boss3TaskFi (0x9C68A0): la vida global solo se fija la primera vez (gp-0x7074 != 0). (Creacion de las barras del HUD: motor.)
void Boss3Logic::initLife(Bs3State& s, Bs3Env& e, int life) {
    if (e.lifeInitPending != 0) {
        if (life > 0) { e.bossLife = life; e.bossLifeMax = life; }
        e.lifeInitPending = 0;
    }
    s.weakRatio = 1.0f; s.lifeRatio = 1.0f;
    s.weakShowTimer = 0;
}
// ControlDispLife__9Boss3TaskFv (0x9C69F0)
void Boss3Logic::controlDispLife(Bs3State& s, const Bs3Env& e) {
    s.lifeRatio = (float)e.bossLife / (float)e.bossLifeMax;
    if (s.st & B3_SF_WEAK_HIDE) s.weakRatio = 0.0f;
    else {
        float m = (float)s.staggerMax;
        s.weakRatio = ((float)(s.staggerHits - 1) / m) + (((float)s.hitCtr / (float)s.hitReload) / m);
    }
    if (s.lifeRatio < 0.0f) s.lifeRatio = 0.0f;
    if (s.weakRatio < 0.0f) s.weakRatio = 0.0f;
    if (s.st & B3_SF_ATTACKING) s.weakShowTimer = 0x3C;
    if (s.weakShowTimer > 0) { s.st |= B3_SF_LIFE_SHOW; s.weakShowTimer--; }
    else s.st &= ~B3_SF_LIFE_SHOW;
}

// ---------------------------------------------------------------- golpes
// CheckWeakHit__9Boss3TaskFi (0x9C74D0): solo las formas 0 y 1.
bool Boss3Logic::checkWeakHit(const Bs3State& s) { return s.form == 0 || s.form == 1; }

// CheckDamage__9Boss3TaskFv (0x9C7540)
void Boss3Logic::checkDamage(Bs3State& s, Bs3Env& e, Boss3Engine& g, const Bs3Query& q, const Bs3Hit hits[2]) {
    if (!(s.enemyFlags & B3_EF_HIT_THIS_FRAME)) return;
    if (s.enemyFlags & B3_EF_NO_CHECK_MASK) return;
    for (int i = 0; i < 2; i++) {
        if (hits[i].player == -1) continue;
        int kind = (int8_t)hits[i].hitPart;
        if (kind <= 0) continue;
        s.lastHitParam = (int8_t)hits[i].param;                    // +0x1EAC
        if (!(s.enemyFlags & B3_EF_ALWAYS_WEAK) && !checkWeakHit(s)) continue;   // NoWeakHit no hace nada en Boss3
        weakHit(s, e, g, q, kind, hits[i].player);
        g.addHitNum(hits[i].player);
    }
}

// WeakHit__9Boss3TaskFii (0x9C7670): resta vida segun tabla de danio (fase x dificultad) y multiplicador del jugador; gestiona fases y aturdimiento.
void Boss3Logic::weakHit(Bs3State& s, Bs3Env& e, Boss3Engine& g, const Bs3Query& q, int hitPart, int player) {
    if ((s.enemyFlags & B3_EF_NO_HIT_MASK) || (s.st & B3_SF_NO_HIT_MASK)) { g.damageEffect(1); return; }
    if (!(s.st & B3_SF_NO_LIFE_LOSS)) {
        int d = getDifficulty(e);
        float base = (float)((s.st & B3_SF_ATTACKING) ? kBs3DmgBit8[s.phase][d] : kBs3DmgNormal[s.phase][d]);
        e.bossLife -= (int)(g.playerDamageMul((int8_t)player) * base);
    }
    float thr = kBs3PhaseParam[e.phaseParamIdx][3 + s.phase];       // (+0xC + fase*4)
    if (s.phase != 0 && (float)e.bossLife <= thr && (e.gameState != 0xA || e.gameKind != 2 || e.bossLife <= 0)) {
        if (e.bossLife <= 0) e.bossLife = 0;
        switch (s.phase) {
        case 2:
            damage(s, g); s.st |= B3_SF_WEAK_HIDE; seqCmdSetTable(s, 7);
            g.addScore(player, 0x5DC);
            g.hitMark(0, player, hitPart);
            if (!(e.gameState == 0xA && e.gameKind == 2)) {
                if (!q.flag22Set && e.gp73FC == 0) g.unlockFlag(2, 3);
                if (e.gp73F8 == 0) g.unlockFlag(2, 0xB);
            }
            break;
        case 1:
            damage(s, g); s.st |= B3_SF_WEAK_HIDE; seqCmdSetTable(s, 7);
            break;
        }
        return;
    }
    if (s.phase == 0 && (float)e.bossLife <= thr) e.bossLife = (int)thr;   // fase 0: la vida no baja del umbral
    if (s.st & B3_SF_ATTACKING) {
        if (--s.hitCtr <= 0) {
            s.hitCtr = s.hitReload;
            if (--s.staggerHits <= 0) {                             // aturdimiento completo
                g.addScore(player, 10);
                damage(s, g);
                g.hitMark(1, player, hitPart);
                incDifficulty(e, 1, 1);
                return;
            }
            damageUpperBody(s, g, player);
            g.hitMark(0, player, hitPart);
            return;
        }
        if (s.phase == 2) g.sound(0x180014, 0, 0);
    } else {
        if (--s.hitCtr <= 0) { damageUpperBody(s, g, player); g.hitMark(0, player, hitPart); }
    }
}

// DamageUpperBody__9Boss3TaskFUi (0x9C7C80): golpe parcial; +10 puntos; la forma 1 cambia de movimiento y acelera.
void Boss3Logic::damageUpperBody(Bs3State& s, Boss3Engine& g, int player) {
    g.addScore(player, 0xA);
    if (s.form == 1) {
        g.overlayMotion(1, kBs3Motion[9].anim);                          // D_009D08C8 = 0x157
        s.motionLenPrev = s.motionLen;
        s.motionLen = kBs3Motion[9].length;                              // D_009D08CC = 0x166
        s.motionBlend = 5; s.motionMode = 5;
        s.speedMul = 1.0f; s.speed = s.baseSpeed * 1.0f;
        g.sound(0x180014, 0, 0);
    }
    s.st |= B3_SF_UPPER_HIT;
}

// SetDamageMotion__9Boss3TaskFv (0x9C7DF0): forma 1 -> indice 9, forma 0 -> 0x145; otras: el original pasa un registro sin inicializar.
int Boss3Logic::damageMotionId(const Bs3State& s) {
    if (s.form == 1) return 9;
    if (s.form == 0) return 0x145;
    return -1;
}

// Damage__9Boss3TaskFUi (0x9C7D60)
void Boss3Logic::damage(Bs3State& s, Boss3Engine& g) {
    if (s.form == 1) { s.st |= B3_SF_DAMAGE; g.cameraEvent(0, 1); }
    g.sound(0x180016, 0, 0);
    int m = damageMotionId(s);
    if (m >= 0) motSupSet2(s, g, m, 0, 5);
    s.enemyFlags |= B3_EF_DAMAGE_MOTION;
}

// CheckDamageEnd__9Boss3TaskFv (0x9C6750)
void Boss3Logic::checkDamageEnd(Bs3State& s, Boss3Engine& g) {
    if ((s.st & B3_SF_UPPER_HIT) && !(s.frameB < 55.0f)) {
        s.st &= ~B3_SF_UPPER_HIT;
        s.baseSpeed = 1.2f;                                         // 0x3F99999A
        s.speed = 1.2f * s.speedMul;
        s.motionLen = s.motionLenPrev;
        s.motionBlend = (int)(10.0f + s.frame);
        s.motionMode = 0xA;
    }
    if ((s.st & B3_SF_DAMAGE) && !(s.frame < (float)g.motionLength(s.motion))) {
        s.st &= ~B3_SF_DAMAGE;
        if (s.scene == 0x15) motSupSet2(s, g, 3, 0, 0xA);
        if (s.phase != 2) g.cameraEvent(0, 4);
    }
}

// ---------------------------------------------------------------- ataques
// SelectAttackTarget__9Boss3TaskFv (0x9C95D0)
bool Boss3Logic::selectAttackTarget(Bs3State& s, Bs3Env& e, Boss3Engine& g) {
    if (e.nPlayers != 2) {
        if (e.nPlayers != 1) s.target = 0; else s.target = (int8_t)e.activePlayer;
    } else {
        int pl = g.rand() & 1;
        if (e.attackerFlag[pl] != 0) {
            pl = (pl + 1) & 1;
            if (e.attackerFlag[pl] != 0) { e.attackerFlag[0] = 0; e.attackerFlag[1] = 0; }
        }
        s.target = (int8_t)pl;
    }
    if (!g.playerValid(s.target)) return false;
    if (s.target == -1) return false;
    e.attackerFlag[s.target] = 1;
    return true;
}

// SetBulletNum__9Boss3TaskFSc (0x9C96B0): numero de disparos del ataque segun forma y dificultad (ataques 4 y 5 usan la otra tabla).
void Boss3Logic::setBulletNum(Bs3State& s, Bs3Env& e, Boss3Engine& g, int8_t attack) {
    int8_t v = 1;
    switch (s.form) {
    case 0: {
        int idx = g.startDifficulty();
        if (e.nPlayers == 2) idx += 0x11;
        v = (attack == 4 || attack == 5) ? kBs3BulletsB[idx] : kBs3BulletsA[idx];
    }   /* fallthrough */
    case 1: s.fireCount = v; return;
    default: return;                                               // otras formas: no toca nada
    }
}

// AtcPlFlgClear__9Boss3TaskFv (0x9C9770)
void Boss3Logic::atcPlFlgClear(Bs3State& s, Bs3Env& e) { if (s.target != -1) { e.attackerFlag[s.target] = 0; s.target = -1; } }
// DecNowEnemyAlvNumChk / DecNowEnemyNumChk (0x9C97A0 / 0x9C97D0)
void Boss3Logic::decNowEnemyAlvNumChk(Bs3State& s, Bs3Env& e) { if (!(s.flags2 & 2)) { s.flags2 |= 2; e.nowEnemyAlive--; } }
void Boss3Logic::decNowEnemyNumChk(Bs3State& s, Bs3Env& e) { if (!(s.flags2 & 4)) { s.flags2 |= 4; e.nowEnemyNum--; } }

// AttackDamageFrame__9Boss3TaskFv (0x9C5FB0): segun el tipo del ataque (kBs3Attack[].kind): 0 bombas, 1/2 laser, 3 golpe cuerpo a cuerpo.
void Boss3Logic::attackDamageFrame(Bs3State& s, Bs3Env& e, Boss3Engine& g) {
    switch (kBs3Attack[s.attackId].kind) {
    case 0: if (selectAttackTarget(s, e, g)) g.spawnBomb(); break;
    case 1: case 2: g.spawnLaser(); break;
    case 3: g.damagePlayer(s.target, 1, 5); decDifficulty(e, 2, 2); break;
    }
}

// StartAttack__9Boss3TaskFSc (0x9C66D0)
void Boss3Logic::startAttack(Bs3State& s, Bs3Env& e, Boss3Engine& g, int8_t id) {
    s.attackId = id; s.attackSub = 0; s.attackWork = 0;
    s.playersAtStart = (int8_t)e.nPlayersB;
    int m = kBs3Attack[id].motion;
    if (m >= 0) motSupSet2(s, g, m, 0, 0xA);
    s.enemyFlags |= B3_EF_ATTACK_DAMAGING;
    s.st |= B3_SF_ATTACKING;
}

// ControlAttack__9Boss3TaskFv (0x9C60B0). Subestados (+0x1E32): kind 3: 0/10/11/-1; kinds 0..2: 0 (y cualquier otro valor >= 1 = siguiente disparo), 100, -1.
void Boss3Logic::controlAttack(Bs3State& s, Bs3Env& e, Boss3Engine& g) {
    if (!(s.st & B3_SF_ATTACKING)) return;
    const Bs3Attack& a = kBs3Attack[s.attackId];
    auto cancel = [&](bool blurOff) {
        s.recovered = 0; s.enemyFlags &= ~B3_EF_ATTACK_DAMAGING;
        s.st &= ~B3_SF_ATTACKING; s.st &= 0xFFFF7FFFu;
        if (blurOff) g.blur(2, 0);
    };
    int expect = a.motion;
    if (expect > 0) {
        if (s.scene != 0x14 && expect < 10) expect = kBs3Motion[expect].anim;   // expect >= 10: sin verificar
        if (s.motion != expect) { cancel(true); return; }                 // el movimiento cambio
        if (s.st & B3_SF_DAMAGE) { cancel(true); return; }                // lo han aturdido
    }
    s.attackAux = (int16_t)g.attackPathAux(s.attackId);                  // +0x1ED0 (con puntero a trayectoria: +0x18 de su registro; sin el: el tipo)
    if (a.kind == 3) {
        switch (s.attackSub) {
        case -1:
            if (!((s.frame + s.speed) < (float)a.end)) {
                s.st &= ~B3_SF_ATTACKING; s.enemyFlags &= ~B3_EF_ATTACK_DAMAGING; s.st &= 0xFFFF7FFFu;
            }
            break;
        case 0:
            if (a.hitStart == -1) { g.blur(0, 0xA); s.attackSub = 0xA; return; }
            if (!(s.frame < (float)a.hitStart)) {
                s.attackSub = 0xA;
                if (a.hitEnd >= 0) g.blur(0, 0xA);
            }
            break;
        case 10:
            if (a.hitEnd == -1) { s.attackSub = -1; return; }
            g.blur(1, 0xA);
            if ((int)s.frame == (int)((float)a.hitEnd - 20.0f)) { g.sound(0x180013, 0, 0); return; }
            if (!(s.frame < (float)a.hitEnd)) { s.st |= B3_SF_BIT15; s.attackSub = 0xB; }
            break;
        case 11:
            if (!(s.frame < (float)a.hitEnd)) {
                attackDamageFrame(s, e, g);
                s.st &= 0xFFFF7FFFu;
                g.blur(2, 0);
                s.attackSub = -1;
            }
            break;
        }
        return;
    }
    if (a.kind < 0 || a.kind > 2) return;
    switch (s.attackSub) {
    case -1:
        if (!((s.frame + s.speed) < (float)a.end)) { s.recovered = 0; s.st &= ~B3_SF_ATTACKING; s.enemyFlags &= ~B3_EF_ATTACK_DAMAGING; }
        break;
    case 100:
        if (a.post == -1) { s.attackSub = -1; return; }
        if (!(s.frame <= (float)a.post)) {
            s.recovered = 1; s.attackSub = -1;
            if (a.kind == 1) s.postFrame = (int16_t)(a.end - 0x1E);
        }
        break;
    default:                                                          // 0 y subestados de disparo sucesivos
        if (a.hitStart == -1) { s.attackSub = 100; return; }
        if (a.hitStart < a.hitEnd) {                                  // varios disparos repartidos en la ventana
            int n = s.fireCount;                                      // (el original divide por +0x1ED2: no puede ser 0)
            s.fireStep = (s.attackSub * 0xFFFF) / n;
            float thr = (float)a.hitStart + ((float)s.attackSub * ((float)(a.hitEnd - a.hitStart) / (float)(n + 1)));
            if (!(s.frame <= thr)) {
                attackDamageFrame(s, e, g);
                if (a.kind == 0 && s.attackSub == 0) g.sound(0x17000C, 0x17000C, 0);
                if (a.kind == 0 && s.attackSub == 0 && s.attackId != 1) g.sound(0x180013, 0, 0);
                s.attackSub++;
                if (s.attackSub >= s.fireCount) { s.attackSub = 100; return; }
            }
        } else if (!(s.frame <= (float)a.hitStart)) {                 // un solo disparo
            attackDamageFrame(s, e, g);
            s.attackSub = 100;
        }
        break;
    }
}

// ---------------------------------------------------------------- movimiento
// MotSupSet2__9Boss3TaskFiii (0x9C94A0)
void Boss3Logic::motSupSet2(Bs3State& s, Boss3Engine& g, int idx, int blend, int mode) {
    if (s.scene == 0x14) { g.setMotionNow(idx); return; }
    if (s.motionIdx == idx) return;
    s.enemyFlags &= 0xBFFFFFFFu;
    if (idx < 0 || idx >= 10) { g.setMotionNow(idx); s.motionIdx = idx; return; }   // fuera de tabla (p.ej. 324 de kBs3Attack): sin verificar contra el asm
    if (mode == 0) g.setMotionNow(kBs3Motion[idx].anim);
    else g.setMotionBlend(kBs3Motion[idx].anim, blend);
    s.motionIdx = idx;
    s.motionLen = kBs3Motion[idx].length; s.motionLenPrev = s.motionLen;
    s.motionBlend = blend; s.motionMode = mode;
}

// B3InterDivAngF__Fiif (0x9C5CD0): interpola el angulo a0 -> a1 (16 bits) por el camino corto con factor t.
int Boss3Logic::interDivAng(float t, int a0, int a1) {
    int d = (a1 - a0) & 0xFFFF;
    int half = (int16_t)(int)(float)(32767.09f);                    // 0x46FFFE2E -> cvt.w.s = 32767
    float den = t + (1.0f - t);
    if (half < d) return a0 + (int)((-((float)(0x10000 - d)) * t) / den);
    return a0 + (int)(((float)d * t) / den);
}

// SetApproach__9Boss3TaskFi (0x9C5D80): arranca una interpolacion desde la posicion/angulos actuales; duracion = frames * 0.8333333.
void Boss3Logic::setApproach(Bs3State& s, int frames) {
    s.st |= B3_SF_APPROACH_ON;
    s.apX = s.pos.x; s.apY = s.pos.y; s.apZ = s.pos.z;
    s.apAx = s.pos.ax; s.apAy = s.pos.ay; s.apAz = s.pos.az;
    s.apT = 0; s.apRate = 1.2f;                                    // 0x3F99999A
    s.apDur = 0.8333333f * (float)frames;
}

// ApproachANPO__9Boss3TaskFP4ANPO (0x9C5E00): avanza hacia el punto (lineal en posicion, camino corto en angulos); al terminar lo coloca exacto.
void Boss3Logic::approachAnpo(Bs3State& s, const Bs3Anpo& t) {
    if (s.st & B3_SF_APPROACH_ON) {
        if (!(s.apT < s.apDur)) {
            s.pos.x = t.x; s.pos.y = t.y; s.pos.z = t.z; s.pos.ax = t.ax; s.pos.ay = t.ay; s.pos.az = t.az;
            s.st &= ~B3_SF_APPROACH_ON;
            return;
        }
        float f = s.apT / s.apDur;
        s.pos.x = s.apX + f * (t.x - s.apX);
        s.pos.y = s.apY + f * (t.y - s.apY);
        s.pos.z = s.apZ + f * (t.z - s.apZ);
        s.pos.ax = interDivAng(f, s.apAx, t.ax);
        s.pos.ay = interDivAng(f, s.apAy, t.ay);
        s.pos.az = interDivAng(f, s.apAz, t.az);
        return;
    }
    if (s.st & B3_SF_APPROACH_SNAP) {
        s.pos.x = t.x; s.pos.y = t.y; s.pos.z = t.z; s.pos.ax = t.ax; s.pos.ay = t.ay; s.pos.az = t.az;
    }
}

// ---------------------------------------------------------------- secuencias
// SeqCmdInit__9Boss3TaskFPPs (0x9C9160) / NextTable (0x9C9190) / SetTable (0x9C91C0)
void Boss3Logic::seqCmdInit(Bs3State& s, const int16_t* const* table) { s.seqTable = table; s.seqTableIdx = 0; s.seqPc = table[0]; s.seqWait = 0; }
void Boss3Logic::seqCmdNextTable(Bs3State& s) { s.seqTableIdx++; s.seqPc = s.seqTable[s.seqTableIdx]; s.seqWait = 0; }
void Boss3Logic::seqCmdSetTable(Bs3State& s, int16_t idx) { s.seqTableIdx = idx; s.seqPc = s.seqTable[idx]; s.seqWait = 0; }

}} // namespace

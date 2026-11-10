#include "boss2.h"
namespace vn { namespace boss {

// ---------------------------------------------------------------- vida y dificultad
// InitLife__9Boss2TaskFs (bs2mod 0x9C7740): si life > 0 fija vida actual y maxima del jefe; reinicia las dos barras (proporcion 1.0).
void Boss2Logic::initLife(Bs2State& s, Bs2Env& e, int16_t life) {
    if (life > 0) { e.bossLife = life; e.bossLifeMax = life; }
    s.lifeRatio = 1.0f; s.weakRatio = 1.0f;
}

// ControlDispLife__9Boss2TaskFv (0x9C77C0): proporciones de las dos barras (el original ademas las pasa al dibujo de HUD).
void Boss2Logic::controlDispLife(Bs2State& s, const Bs2Env& e) {
    s.lifeRatio = (float)e.bossLife / (float)e.bossLifeMax;
    s.weakRatio = (float)s.staggerHits / (float)s.staggerMax;
}

// SetDiffParam__9Boss2TaskFv (0x9C7D20): copia la fila [forma][nivel] de la tabla de dificultad segun el indice +0x20FC.
void Boss2Logic::setDiffParam(Bs2State& s, Bs2Env& e, Boss2Engine& g) {
    const Bs2Diff& d = kBs2Diff[s.form][g.twoPlayerLevel() & 1];
    int i = s.difficulty;
    s.staggerHits = d.stagger[i]; s.staggerMax = d.stagger[i];
    s.dmgPerHit = d.dmg[i];
    s.diffUp = d.up; s.diffDown = d.down;
    if (d.hasWarpF) { s.alphaOutThr = d.warpF[i]; s.alphaInThr = d.warpF[i]; }
    e.difficultyShown = s.difficulty;
}

// InitDifficulty__9Boss2TaskFv (0x9C7DF0)
void Boss2Logic::initDifficulty(Bs2State& s, Boss2Engine& g) { s.difficulty = g.startDifficulty(); s.diffTimer = 0; }

// UpDifficulty__9Boss2TaskFv (0x9C7E50): tope 16
void Boss2Logic::upDifficulty(Bs2State& s) { s.difficulty += s.diffUp; if (s.difficulty >= 0x11) s.difficulty = 0x10; }

// DownDifficulty__9Boss2TaskFv (0x9C7E80) = DownDifficultyReq (0x9C7E20): suelo 0, reinicia el temporizador
void Boss2Logic::downDifficulty(Bs2State& s) { s.difficulty -= s.diffDown; if (s.difficulty < 0) s.difficulty = 0; s.diffTimer = 0; }

// NoChgDifficultyReq__9Boss2TaskFi (0x9C7E40)
void Boss2Logic::noChgDifficultyReq(Bs2State& s) { s.diffTimer = 0; }

// ---------------------------------------------------------------- golpes y danio
// CheckWeakHit__9Boss2TaskFi (0x9C88B0): formas 0 y 1 siempre; forma 2 solo si la parte golpeada es la 2.
bool Boss2Logic::checkWeakHit(const Bs2State& s, int hitPart) {
    if (s.form == BS2_FORM_BATTLE1 || s.form == BS2_FORM_BATTLE3) return true;
    if (s.form == BS2_FORM_BATTLE4) return (int8_t)hitPart == 2;
    return false;
}

// CheckDamage__9Boss2TaskFv (0x9C8940)
void Boss2Logic::checkDamage(Bs2State& s, Bs2Env& e, Boss2Engine& g, const Bs2Query& q, const Bs2Hit hits[2]) {
    if (!(s.enemyFlags & EF_HIT_THIS_FRAME)) return;
    if (s.enemyFlags & EF_NO_CHECK_MASK) return;
    for (int i = 0; i < 2; i++) {
        int pl = hits[i].player;
        if (pl == -1) continue;
        int kind = (int8_t)hits[i].hitPart;
        if (kind <= 0) continue;
        s.lastHitParam = (int8_t)hits[i].param;                // +0x20EE
        if (!(s.enemyFlags & EF_ALWAYS_WEAK) && !checkWeakHit(s, kind)) { noWeakHit(s, g); continue; }
        weakHit(s, e, g, q, kind, pl);
        g.addHitNum(pl);
    }
}

// WeakHit__9Boss2TaskFii (0x9C8A70)
void Boss2Logic::weakHit(Bs2State& s, Bs2Env& e, Boss2Engine& g, const Bs2Query& q, int hitPart, int player) {
    if ((s.enemyFlags & EF_NO_HIT_MASK) || (s.st & SF_STAGGER)) { g.damageEffect(1); return; }
    if (--s.hitCtr > 0) return;
    s.hitCtr = s.hitReload;
    g.addScore(player, 10);
    e.bossLife -= (int)((float)s.dmgPerHit * g.playerDamageMul((int8_t)player));
    int r = 0;
    if (e.bossLife <= 0) {
        if (s.form != BS2_FORM_BATTLE4) e.bossLife = 1;        // solo la forma final puede morir
        else { e.bossLife = 0; r = 3; }
    }
    if (--s.staggerHits <= 0) { if (r < 2) r = 2; }            // aturdimiento completo
    else if (r <= 0) r = 1;                                    // golpe parcial
    switch (r) {
    case 1: damageUpperBody(s, g, player); g.hitMark(2, player, hitPart); break;
    case 2: damage(s, e, g); g.hitMark(1, player, hitPart); break;
    case 3: dead(s, e, g, q, player); g.hitMark(2, player, hitPart); break;
    }
}

// NoWeakHit__9Boss2TaskFii (0x9C8C40)
void Boss2Logic::noWeakHit(const Bs2State& s, Boss2Engine& g) { if (s.form == BS2_FORM_BATTLE4) g.noWeakHitFeedback(); }

// DamageUpperBody__9Boss2TaskFUi (0x9C8C70): golpe que no aturde del todo; elige movimiento de reaccion y tiempo de aturdimiento.
void Boss2Logic::damageUpperBody(Bs2State& s, Boss2Engine& g, int player) {
    (void)player;
    int react = -1;
    switch (s.form) {
    case BS2_FORM_BATTLE3:
        g.sound(0xD001E, 0, 0);
        if (!(s.enemyFlags & EF_ATTACK_DAMAGING)) react = 0x115;
        s.st |= SF_STAGGER; s.staggerTimer = 0x1D;
        break;
    case BS2_FORM_BATTLE1:
        g.sound(0xD001E, 0, 0);
        if (!(s.enemyFlags & EF_ATTACK_DAMAGING)) {
            switch (s.motion) { case 0xFE: react = 0xFF; break; case 0xFC: react = 0xFD; break; case 0xFA: react = 0xFB; break; case 0xF8: react = 0xF9; break; }
        }
        s.st |= SF_STAGGER; s.staggerTimer = 0x1D;
        break;
    case BS2_FORM_BATTLE4:
        g.damageEffect(0);
        g.sound(0xD001A, 0, 0);
        if (!(s.enemyFlags & EF_ATTACK_DAMAGING)) {
            s.speedMul = 0.3f;                                  // 0x3E99999A
            react = 0x126;
            s.speed = s.baseSpeed * 0.3f;
        }
        s.st |= SF_STAGGER; s.staggerTimer = 0x1E;
        break;
    }
    if (react >= 0) g.playOverlayMotion(1, react);
}

// SetDamageMotion__9Boss2TaskFv (0x9C9230): id del movimiento de dano; -1 si la forma no esta prevista (el original pasaria un registro sin inicializar).
int Boss2Logic::damageMotionId(const Bs2State& s) {
    switch (s.form) { case BS2_FORM_BATTLE4: return 0x125; case BS2_FORM_BATTLE3: case BS2_FORM_BATTLE1: return 0x114; }
    return -1;
}

// Damage__9Boss2TaskFUi (0x9C90C0): aturdimiento completo: avanza la secuencia de la rama y pone el movimiento de dano.
void Boss2Logic::damage(Bs2State& s, Bs2Env& e, Boss2Engine& g) {
    switch (s.form) {
    case BS2_FORM_BATTLE1:
        s.branchStep = (uint8_t)(s.branchStep + 3);
        s.branchStep = (uint8_t)(s.branchStep - (s.branchStep % 3));   // redondea al multiplo de 3 superior...
        s.branchStep = (uint8_t)(s.branchStep - 1);                    // ...menos 1
        /* fallthrough: igual que la forma 1 */
    case BS2_FORM_BATTLE3:
        seqCmdNextTable(s, e);
        g.sound(0xD001D, 0xD001D, 1);
        g.sound(0xD001F, 0, 0);
        break;
    case BS2_FORM_BATTLE4:
        s.baseSpeed = 1.2f; s.speedMul = 1.0f; s.speed = 1.2f;         // 0x2074=1.2, 0x207C=1.0, 0x206C=0x2074*1.0
        seqCmdNextTable(s, e);
        seqCmdCall(s, e, 0);
        g.sound(0xD001B, 0, 0);
        break;
    }
    if (s.form == BS2_FORM_BATTLE4) g.damageEffect(0);                 // StartDamageEffect
    int m = damageMotionId(s);
    if (m >= 0) g.setMotion(m, 0, 0);
    s.enemyFlags |= EF_DAMAGE_MOTION;
    s.st &= ~SF_AFTERIMAGE;
}

// Dead__9Boss2TaskFUi (0x9C92A0): muerte del jefe (solo la forma 2 llega aqui).
void Boss2Logic::dead(Bs2State& s, Bs2Env& e, Boss2Engine& g, const Bs2Query& q, int player) {
    if (s.form == BS2_FORM_BATTLE4) g.damageEffect(0);
    g.addScore(player, 0x5DC);                                         // 1500 puntos
    g.sound(0xD001C, 0, 0);
    if (e.gameState != 0xA || e.gameKind != 2) {
        if (!q.flag20Set && e.gp73FC == 0) g.unlockFlag(2, 1);
        if (e.gp73F8 == 0) g.unlockFlag(2, 0xA);
    }
    s.st &= 0xFFDFFFFFu;
    s.enemyFlags |= EF_DEAD;
    motSupSet2(s, g, 0x11E, 0, 5);                                     // SetDeadMotion (0x9C93D0)
    s.baseSpeed = 1.2f; s.speedMul = 1.0f; s.speed = 1.2f;
    s.deadStage = 3; s.deadB5 = 0; s.deadB6 = 0;
}

// CheckDamageEnd__9Boss2TaskFv (0x9C76D0): cuenta atras del aturdimiento; al llegar a 0 restablece la velocidad.
void Boss2Logic::checkDamageEnd(Bs2State& s) {
    if (!(s.st & SF_STAGGER)) return;
    if (--s.staggerTimer == 0) {
        s.st &= ~SF_STAGGER;
        s.speedMul = 1.0f;
        s.speed = s.baseSpeed * 1.0f;
    }
}

// ---------------------------------------------------------------- ataques
// DamageToPlayer__9Boss2TaskFv (0x9CB370)
void Boss2Logic::damageToPlayer(Bs2State& s, Boss2Engine& g) {
    if (s.target < 0) return;
    if (g.damagePlayer(s.target, 1, kBs2Attack[s.attackId].playerDamage) == 1) downDifficulty(s);
    else s.diffTimer = 0;
}

// AttackDamageFrame__9Boss2TaskFv (0x9C6E20): lo que ocurre en el fotograma de golpe; despacho por tabla de saltos jtbl_009D7180 (24 casos).
void Boss2Logic::attackDamageFrame(Bs2State& s, Bs2Env& e, Boss2Engine& g) {
    (void)e;
    int key = ((int)s.branchIdx << 16) | (uint16_t)s.seqTableIdx;       // (+0x2068 << 16) | +0x205C
    int id = s.attackId;
    if (id >= 0 && id < 24) {
        if (id <= 4) { damageToPlayer(s, g); s.swordFireAlpha = 0; g.resetSwordFire(); }
        else if (id <= 15 || id == 22) { g.spawnSonicWave(); g.sound(0xE0115, 0, 0, 0x40); }
        else if (id == 16 || id == 17 || id == 23) damageToPlayer(s, g);
        else if (id == 18 || id == 19) {
            damageToPlayer(s, g);
            if (key == 0x20002 || key == 0x10002) g.pillarDynamics(1, 3, 0, -1, 0);
            else if (key == 0x10000) g.pillarDynamics(2, 1, 1, -1, 0);
        } else { // 20, 21
            if (key == 0x50000) g.pillarDynamics(2, 3, 5, s.target, s.playersAtStart);
            else if (key == 0x20001) g.pillarDynamics(2, 0, 5, s.target, s.playersAtStart);
        }
    }
    g.sound(0xE010F, 0, 0, 0x40);                                      // comun a todos los casos (incluido id >= 24)
}

// StartAttack__9Boss2TaskFSc (0x9C72B0). id -1 = aleatorio entre 0..3 (nlRand()%4); otros negativos usan registros sin inicializar en el original: no soportado.
void Boss2Logic::startAttack(Bs2State& s, Bs2Env& e, Boss2Engine& g, int8_t id) {
    if (id < 0) s.attackId = (int8_t)(0 + (g.rand() % 4));
    else s.attackId = id;
    s.attackSub = 0; s.attackTimer = 0;
    s.playersAtStart = (int8_t)e.nPlayersDFC;
    const Bs2Attack& a = kBs2Attack[s.attackId];
    switch (s.form) {
    case BS2_FORM_BATTLE1:
        s.st &= ~SF_FACE_PLAYER;
        g.attackPosSet(); g.setAttackDist(a.dist);
        motSupSet2(s, g, a.motion, a.start, 0xA);
        break;
    case BS2_FORM_BATTLE3: motSupSet2(s, g, a.motion, a.start + 5, 5); break;
    case BS2_FORM_BATTLE4: motSupSet2(s, g, a.motion, a.start + 0xA, 0xA); break;
    }
    if (s.attackId >= 0 && s.attackId <= 4) { s.baseSpeed = 0.84f; s.speed = 0.84f * s.speedMul; }   // 0x3F570A3E
    // ataques 16..23: destello de espada y carga de energia (efectos, no se modelan)
    s.enemyFlags |= EF_ATTACK_DAMAGING;
    s.st |= SF_ATTACKING;
}

// ControlAttack__9Boss2TaskFv (0x9C7020): maquina de estados del ataque (subestado +0x20DA): 0/1 primer golpe, 10/11 segundo golpe, 20/21 recuperacion.
void Boss2Logic::controlAttack(Bs2State& s, Bs2Env& e, Boss2Engine& g) {
    if (!(s.st & SF_ATTACKING)) return;
    const Bs2Attack& a = kBs2Attack[s.attackId];
    if (s.motion != a.motion) { s.enemyFlags &= ~EF_ATTACK_DAMAGING; s.st &= ~SF_ATTACKING; return; }   // el movimiento se interrumpio
    switch (s.attackSub) {
    case 0:
        if (!(s.frame < (float)a.hit1)) {
            attackDamageFrame(s, e, g);
            if (s.attackId == 0x17) { s.attackSub = 0xA; s.enemyFlags |= EF_ATTACK_BIT15; return; }
            if (a.hit2 < a.hit1) { s.attackSub++; return; }          // sin segundo golpe (-1)
            s.attackSub = 0xA; return;
        }
        break;
    case 1:
        if (!(s.frame <= (float)(a.hit1 + 5))) { s.enemyFlags &= ~EF_ATTACK_DAMAGING; s.attackSub = 0x14; return; }
        break;
    case 10:
        if (!(s.frame < (float)a.hit2)) { attackDamageFrame(s, e, g); s.attackSub++; return; }
        break;
    case 11:
        if (!(s.frame <= (float)(a.hit2 + 5))) { s.enemyFlags &= ~EF_ATTACK_DAMAGING; s.attackSub = 0x14; return; }
        break;
    case 20:
        if (s.form == BS2_FORM_BATTLE1) {
            s.baseSpeed = 0.12f; s.speed = 0.12f * s.speedMul;       // 0x3DF5C290
            s.st &= ~SF_ATTACKING;
        }
        s.attackSub++;
        /* fallthrough */
    case 21:
        if (!((s.frame + s.speed) < (float)a.end)) s.st &= ~SF_ATTACKING;
        break;
    }
}

// SelectAttackTarget__9Boss2TaskFv (0x9CB200)
bool Boss2Logic::selectAttackTarget(Bs2State& s, Bs2Env& e, Boss2Engine& g) {
    switch (g.twoPlayerLevel()) {
    case 0: s.target = (int8_t)e.activePlayer; break;
    case 1: {
        int pl = g.rand() & 1;
        if (e.attackerFlag[pl] != 0) {
            pl = (pl + 1) & 1;
            if (e.attackerFlag[pl] != 0) { e.attackerFlag[0] = 0; e.attackerFlag[1] = 0; }   // ambos ya atacados: reinicia
        }
        s.target = (int8_t)pl;
        break; }
    }
    if (!g.playerValid(s.target)) s.target = -1;
    if (s.target == -1) return false;
    e.attackerFlag[s.target] = 1;
    return true;
}

// AtcPlFlgClear__9Boss2TaskFv (0x9CB2E0)
void Boss2Logic::atcPlFlgClear(Bs2State& s, Bs2Env& e) {
    if (s.target != -1) { e.attackerFlag[s.target] = 0; s.target = -1; }
}
// DecNowEnemyAlvNumChk__9Boss2TaskFv (0x9CB310): descuenta una sola vez
void Boss2Logic::decNowEnemyAlvNumChk(Bs2State& s, Bs2Env& e) { if (!(s.flags2 & F2_ALV_DEC)) { s.flags2 |= F2_ALV_DEC; e.nowEnemyAlive--; } }
// DecNowEnemyNumChk__9Boss2TaskFv (0x9CB340)
void Boss2Logic::decNowEnemyNumChk(Bs2State& s, Bs2Env& e) { if (!(s.flags2 & F2_NUM_DEC)) { s.flags2 |= F2_NUM_DEC; e.nowEnemyNum--; } }

// ---------------------------------------------------------------- movimiento
// MotSupSet2__9Boss2TaskFiii (0x9CB140): cambia de movimiento si es distinto; compensa el giro por la diferencia de angulo de los dos movimientos.
void Boss2Logic::motSupSet2(Bs2State& s, Boss2Engine& g, int motionId, int a, int b) {
    if (s.motion == motionId) return;
    int cur = g.motionAngle(s.motion);
    int nxt = g.motionAngle(motionId);
    s.angleY -= (nxt - cur);
    s.enemyFlags &= 0xBFFFFFFFu;
    g.setMotion(motionId, a, b);
}

// SetHeight__9Boss2TaskFv (0x9CAF40): pega al suelo si esta a <= 10 de distancia; si esta mas alto empieza la caida.
void Boss2Logic::setHeight(Bs2State& s, Boss2Engine& g) {
    float h = g.groundHeight(s.px, s.py + 6.0f, s.pz);
    float d = s.py - h;
    bool snap = (s.py <= h) || !(__builtin_sqrtf(d * d) > 10.0f);
    if (!snap) {
        if (!(s.flags2 & F2_FALLING)) { s.flags2 |= F2_FALLING; s.fallState = 0; }
    } else if (!(s.flags2 & F2_FALLING)) s.py = h;
}

// FallCheck__9Boss2TaskFv (0x9CB030): caida con gravedad -0.0392; al tocar suelo para y limpia el flag.
void Boss2Logic::fallCheck(Bs2State& s, Boss2Engine& g) {
    if (!(s.flags2 & F2_FALLING)) return;
    switch (s.fallState) {
    case 0:
        s.ay = -0.0392f; s.fallState++;
        /* fallthrough */
    case 1: {
        float ground = g.groundHeight(s.px, s.py, s.pz);
        s.vy += s.ay; s.py += s.vy;
        if (ground < s.py + s.vy) return;                              // sigue cayendo
        s.vy = 0; s.ay = 0; s.py = ground; s.fallState++;
    }
        /* fallthrough */
    case 2:
        s.vy = 0; s.ay = 0; s.flags2 &= ~F2_FALLING; s.fallState = 0;
        break;
    }
}

// ControlSound__9Boss2TaskFv (0x9C6A70): movimiento 0x11D, sonido 0xE0114 al cruzar el fotograma 60.
void Boss2Logic::controlSound(const Bs2State& s, Boss2Engine& g) {
    if (s.motion == 0x11D && s.frameB < 60.0f && !((s.frameB + s.speed) < 60.0f)) g.sound(0xE0114, 0, 0);
}

// ControlOutline__9Boss2TaskFv (0x9C6970): sigue el alfa del jefe con +-0.05 por fotograma.
void Boss2Logic::controlOutline(Bs2State& s) {
    if (!(s.st & SF_OUTLINE)) return;
    if (!(s.alphaRate <= 0.0f)) { s.outlineAlpha = s.alpha; return; }
    if (s.alpha == 0.0f) {
        if (!(s.outlineAlpha <= 0.0f)) { s.outlineAlpha -= 0.05f; if (s.outlineAlpha < 0.0f) s.outlineAlpha = 0.0f; }
    } else if (s.deadStage != 3 && s.deadStage != 2) {
        if (s.outlineAlpha < s.alpha) { s.outlineAlpha += 0.05f; if (!(s.outlineAlpha <= s.alpha)) s.outlineAlpha = s.alpha; }
    }
}

// StartWarpOut__9Boss2TaskFv (0x9C6B00): empieza la salida del teletransporte.
void Boss2Logic::startWarpOut(Bs2State& s, Boss2Engine& g, const Bs2WarpPoint& w) {
    if (w.outMotion >= 0) motSupSet2(s, g, w.outMotion, 0xA, 0xA);
    s.st &= ~8u;
    g.warpEffect(w.outPre, w.outMid, w.outPost);
    s.warpKind = 1; s.warpPhase = 0; s.warpTimer = w.outPre;
    s.st |= SF_WARP_PRE; s.st |= SF_WARP_TIMER;
    if (s.alphaOutThr == 1.0f) g.disableCollision();
    if (s.form == BS2_FORM_BATTLE1 && (s.branchStep % 3) == 2) g.sound(0xD001D, 0xD001D, 0);
}

// ControlWarp__9Boss2TaskFv (0x9C64B0): fases 0/1/2 del teletransporte con sus temporizadores y rampa de alfa.
// NO incluida: la eleccion del movimiento de entrada (kind 2, tablas D_009D4DF0/D_009D4E00/D_9D7800) ni el giro hacia el jugador.
void Boss2Logic::controlWarp(Bs2State& s, Boss2Engine& g, const Bs2WarpPoint& w, bool outlineOn) {
    (void)outlineOn;
    if (s.st & SF_WARP_TIMER) {
        switch (s.warpPhase) {
        case 0:
            if (--s.warpTimer == 0) {
                if (s.warpKind == 1) { s.alphaRate = -s.alpha / (float)w.outMid; s.warpTimer = w.outMid; }
                else if (s.warpKind == 2) { s.alphaRate = (1.0f - s.alpha) / (float)w.inMid; s.warpTimer = w.inMid; }
                s.st |= SF_WARP_ALPHA_BUSY; s.warpPhase++;
            }
            break;
        case 1:
            if (--s.warpTimer == 0) {
                if (s.warpKind == 1) s.warpTimer = w.outPost; else if (s.warpKind == 2) s.warpTimer = w.inPost;
                s.st &= ~SF_WARP_PRE; s.warpPhase++;
            }
            break;
        case 2:
            if (--s.warpTimer == 0) s.st &= ~SF_WARP_TIMER;
            break;
        }
    }
    if (!(s.st & SF_WARP_ALPHA_BUSY)) return;
    float old = s.alpha;
    s.alpha = old + s.alphaRate;
    if (s.alphaRate == 0.0f) s.st &= ~SF_WARP_ALPHA_BUSY;
    else if (!(s.alphaRate <= 0.0f)) {                                  // aparece
        s.st &= ~SF_HIDDEN;
        if (!(s.alpha < s.alphaInThr) && old <= s.alphaInThr) g.enableCollision();
        if (!(s.alpha < 1.0f)) { s.alpha = 1.0f; s.alphaRate = 0.0f; s.st &= ~SF_WARP_ALPHA_BUSY; }
    } else {                                                            // desaparece
        if (s.alpha <= s.alphaOutThr && !(old < s.alphaOutThr)) g.disableCollision();
        if (s.alpha <= 0.0f) { s.alpha = 0.0f; s.alphaRate = 0.0f; s.st &= ~SF_WARP_ALPHA_BUSY; s.st |= SF_HIDDEN; }
    }
    if (s.alpha <= 0.01f && (s.st & SF_OUTLINE) && !(s.outlineAlpha <= 0.0f)) g.setAlpha(0.01f);
    else g.setAlpha(s.alpha);
}

// ETurn__9Boss2TaskFiii (0x9CB620): gira `cur` hacia `target` (angulos de 16 bits) como maximo `step`; si ya esta a <= step devuelve target.
int Boss2Logic::eTurn(int cur, int target, int step) {
    int t = target & 0xFFFF, c = cur & 0xFFFF;
    int direct = c - t; if (direct < 0) direct = -direct;
    int viaUp = (t + 0x10000) - c; if (viaUp < 0) viaUp = -viaUp;
    int viaDown = (c + 0x10000) - t; if (viaDown < 0) viaDown = -viaDown;
    if (step >= direct || step >= viaUp || step >= viaDown) return t;
    if (c >= t) { if (viaUp >= direct) step = -step; }
    else if (viaDown < direct) step = -step;
    return c + step;
}

// AngChk__9Boss2TaskFiii (0x9CB6D0): 1 si `ang` esta dentro de center +- range (modulo 0x10000)
bool Boss2Logic::angChk(int ang, int center, int range) {
    uint32_t c = (uint32_t)center & 0xFFFF;
    uint32_t lo = (c - (uint32_t)range) + 0x10000u, hi = c + (uint32_t)range + 0x10000u;
    if (hi < lo) { uint32_t t = lo; lo = hi; hi = t; }
    uint32_t a = (uint32_t)ang & 0xFFFF;
    if (a < lo) { do { a += 0x10000u; } while (a < lo); }
    return a <= hi;
}

// ---------------------------------------------------------------- secuencias
// SeqCmdInit__9Boss2TaskFPPs (0x9C9DD0)
void Boss2Logic::seqCmdInit(Bs2State& s, const int16_t* const* table) {
    s.seqTable = table; s.seqTableIdx = 0; s.seqPc = table[0]; s.seqWait = 0; s.seqSp = 0;
}
// SeqCmdNextTable__9Boss2TaskFv (0x9C9E00): pasa a la tabla siguiente; si es nula salta a la siguiente rama.
void Boss2Logic::seqCmdNextTable(Bs2State& s, Bs2Env& e) {
    s.seqTableIdx++;
    s.seqPc = s.seqTable[s.seqTableIdx]; s.seqWait = 0;
    if (s.seqPc == nullptr) branch(s, e);
}
// SeqCmdPush__9Boss2TaskFv (0x9C9E60) / Pop (0x9C9E80)
void Boss2Logic::seqCmdPush(Bs2State& s) { s.seqStack[s.seqSp++] = s.seqPc; }
void Boss2Logic::seqCmdPop(Bs2State& s) { s.seqPc = s.seqStack[--s.seqSp]; }
// SeqCmdCall__9Boss2TaskFs (0x9C9EB0): apila y salta al destino idx de la tabla global gp-0x7A90
void Boss2Logic::seqCmdCall(Bs2State& s, const Bs2Env& e, int16_t idx) { seqCmdPush(s); s.seqPc = e.seqCallTable[idx]; }
// SeqCmdCancelWait__9Boss2TaskFv (0x9C9F00): si el comando es 2,3 o 4 (esperas) salta su operando (D_009D6250[op] palabras de 16 bits)
void Boss2Logic::seqCmdCancelWait(Bs2State& s) {
    int16_t op = *s.seqPc;
    if (op != 4 && op != 3 && op != 2) return;
    s.seqPc += kBs2CancelSkip[op];
}
// BranchInit__9Boss2TaskFP12BS2BRANCHPRM (0x9C9F60)
void Boss2Logic::branchInit(Bs2State& s, Bs2Env& e, const Bs2Branch* b) {
    for (int i = 0; i < 8; i++) e.branchVisits[i] = 0;
    s.branches = b; s.branchIdx = 0;
    s.branchParam = b[0].first;
    seqCmdInit(s, b[0].seqFirst);
    s.branchStep = 0;
    e.branchVisits[0]++;
}
// Branch__9Boss2TaskFv (0x9C9FE0): siguiente rama segun branchSel; primera visita usa (first, seqFirst), las demas (repeat, seqRepeat)
void Boss2Logic::branch(Bs2State& s, Bs2Env& e) {
    s.branchIdx = s.branches[(int)s.branchIdx].next[s.branchSel];
    const Bs2Branch& b = s.branches[(int)s.branchIdx];
    int8_t v = e.branchVisits[(int)s.branchIdx];
    e.branchVisits[(int)s.branchIdx] = (int8_t)(v + 1);
    if (v > 0) { s.branchParam = b.repeat; seqCmdInit(s, b.seqRepeat); }
    else { s.branchParam = b.first; seqCmdInit(s, b.seqFirst); }
    s.branchStep = 0;
}

}} // namespace

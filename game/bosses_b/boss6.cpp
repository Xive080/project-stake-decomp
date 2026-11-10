// Boss6Task (overlay bs6mod): reglas del jefe 6 reescritas a mano. Cada funcion cita la original (simbolo y direccion).
// Borrador con m2c, verificado contra el ensamblado (ver MANIFEST_boss6.md para lo no verificado).
#include "boss6.h"
#include <cstring>
namespace vn { namespace boss6 {

static const float kRate = 1.2f;            // 0x3F99999A: 60 Hz -> 50 Hz (PAL)
static const float kInv256 = 0.00390625f;   // 0x3B800000

int16_t vmRead16(uint32_t addr) {
    if (addr >= kOvlImageBase && addr + 2 <= kOvlImageBase + kOvlImageLen * 2) return kOvlImage[(addr - kOvlImageBase) / 2];
    if (addr >= kElfImageBase && addr + 2 <= kElfImageBase + 32) return kElfImage[(addr - kElfImageBase) / 2];
    return 0;
}
uint32_t vmRead32(uint32_t addr) { return (uint16_t)vmRead16(addr) | ((uint32_t)(uint16_t)vmRead16(addr + 2) << 16); }

// ---------------------------------------------------------------- dificultad
// InitBS6Difficulty (0x9C4EE0): fila de vida por nivel de partida y dificultad inicial de ambos jugadores.
void Boss6Logic::initBS6Difficulty(Boss6Globals& g, int level, int initial) {
    g.lifeRowIdx = level;                       // gp-0x7028 = D_009D3490 + level*0x18
    g.diff[1] = (int16_t)initial; g.diff[0] = (int16_t)initial;
    g.diffMirror = g.diff[0];
}
// IncBS6Difficulty (0x9C7D00): suma `d2` (partida de dos jugadores, 0x500DFC == 2) o `d1` a ambas y topa en 16. `slot` no se usa.
int Boss6Logic::incBS6Difficulty(Boss6Globals& g, int, int d1, int d2) {
    int d = g.gameMode == 2 ? d2 : d1;
    g.diff[0] = (int16_t)(g.diff[0] + d); g.diff[1] = (int16_t)(g.diff[1] + d);
    if (g.diff[0] > 16) g.diff[0] = 16;
    if (g.diff[1] > 16) g.diff[1] = 16;
    g.diffMirror = g.diff[0];
    return g.diff[0];
}
// DecBS6Difficulty (0x9C7D80): idem restando; suelo 0.
int Boss6Logic::decBS6Difficulty(Boss6Globals& g, int, int d1, int d2) {
    int d = g.gameMode == 2 ? d2 : d1;
    g.diff[0] = (int16_t)(g.diff[0] - d); g.diff[1] = (int16_t)(g.diff[1] - d);
    if (g.diff[0] < 0) g.diff[0] = 0;
    if (g.diff[1] < 0) g.diff[1] = 0;
    g.diffMirror = g.diff[0];
    return g.diff[0];
}
// GetBS6Difficulty (0x9C7E00): 1 jugador -> su dificultad; 2 jugadores -> media + 0x11; otro -> 0. Copia diff[0] en gp-0x73B8.
int Boss6Logic::getBS6Difficulty(Boss6Globals& g, int) {
    g.diffMirror = g.diff[0];
    if (g.playersMode == 2) {
        int s = (int)g.diff[0] + (int)g.diff[1];
        return (s < 0 ? (s + 1) >> 1 : s >> 1) + 0x11;
    }
    if (g.playersMode == 1) return g.diff[g.lastHitTarget];
    return 0;
}

// ---------------------------------------------------------------- vida
// InitLife (0x9C6740): la vida compartida se fija la primera vez; barras a 1.0.
void Boss6Logic::initLife(Boss6State& b, Boss6Globals& g, Boss6Env& env, int life) {
    if (g.lifeInitFlag != 0) {
        if (life > 0) { g.bossLife = life; g.bossLifeMax = life; }
        g.lifeInitFlag = 0;
    }
    b.hitRatio = 1.0f; b.lifeRatio = 1.0f;
    env.hook(H_ALLOC_GAUGE, 1, 0, 0); b.gauge[0] = true;
    env.hook(H_ALLOC_GAUGE, 0, 0, 0); b.gauge[1] = true;
    b.blinkTimer = 0;
}
// ControlDispLife (0x9C6850): solo la parte numerica; el dibujo de las barras pasa por hook.
void Boss6Logic::controlDispLife(Boss6State& b, const Boss6Globals& g, Boss6Env& env) {
    b.lifeRatio = (float)g.bossLife / (float)g.bossLifeMax;
    if (b.f1DA4 & FA_DEAD) b.hitRatio = 0.0f;
    else {
        float a = (float)b.hitCounter / (float)b.hitCounterMax;
        float bb = (float)b.phaseHitsMax;
        a = a / bb;
        b.hitRatio = (float)(b.phaseHits - 1) / bb + a;
    }
    if (b.lifeRatio < 0.0f) b.lifeRatio = 0.0f;
    if (b.hitRatio < 0.0f) b.hitRatio = 0.0f;
    env.hook(H_DRAW_GAUGE, 0, 0, 0);     // func_948FB0(lifeRatio, gauge0); func_948FB0(hitRatio, gauge1)
    if (b.f1DA4 & FA_ATTACK) b.blinkTimer = 0x32;
    if (b.blinkTimer > 0) { b.f1DA4 |= FA_LIFE_BLINK; b.blinkTimer--; }
    else b.f1DA4 &= ~(uint32_t)FA_LIFE_BLINK;
}

// ---------------------------------------------------------------- impactos y dano
// CheckWeakHit (0x9C6F30): los tipos 0, 1 y 2 siempre cuentan como punto debil.
bool Boss6Logic::checkWeakHit(const Boss6State& b) { return b.kind <= 2; }
// ClearWeakHit: quita 0x40000000 de +0x0C en las 4 partes presentes.
void Boss6Logic::clearWeakHit(Boss6State& b) {
    for (int i = 0; i < 4; i++) if (b.partPresent[i]) b.partF0C[i] &= 0xBFFFFFFFu;
}
// CheckDamage (0x9C6FB0): procesa los impactos de los dos jugadores y los golpes de murcielagos (+0x1F9C).
void Boss6Logic::checkDamage(Boss6State& b, Boss6Globals& g, Boss6Env& env, const HitInfo hit[2]) {
    if (b.f0C & F0C_HIT) {
        env.hook(H_HIT_PREPARE, 0, 0, 0);
        if (!(b.f0C & 0x8100)) {
            for (int i = 0; i < 2; i++) {
                int slot = hit[i].slot;
                if (slot == -1) continue;
                int part = hit[i].partType;
                if (part > 0) {
                    b.weakIdx = hit[i].weakIdx;
                    if ((b.f0C & F0C_WEAK) || checkWeakHit(b)) { weakHit(b, g, env, part, slot); env.hook(H_HIT_CONSUMED, slot, 0, 0); }
                    else noWeakHit(b, env, slot);
                }
            }
        }
        clearWeakHit(b);
    }
    if (b.kind == 0) {
        if (b.hitFlags != 0) {
            if (!(b.f0C & 0x8100)) {
                if (b.hitFlags & 1) weakHit(b, g, env, 0, 0);
                if (b.hitFlags & 2) weakHit(b, g, env, 0, 1);
            }
            clearWeakHit(b);
        }
        b.hitFlags = 0;
    }
}
// WeakHit (0x9C7140): resta vida, decide transicion de fase (umbral por tipo) o cuenta golpes de la barra pequena.
void Boss6Logic::weakHit(Boss6State& b, Boss6Globals& g, Boss6Env& env, int partType, int slot) {
    if (b.f0C & 0x40008100u) return;
    if (b.f1DA4 & 0x4C020u) return;
    float dmgPer = (float)kDmgPerHit[getBS6Difficulty(g, slot)];
    g.bossLife -= (int)(env.damageScale(slot) * dmgPer);
    int life = g.bossLife;
    bool special = (g.gameState == 0xA && g.gameKind == 2);
    bool phaseEnd = (float)life <= kLifeRows_flat[g.lifeRowIdx * 6 + 3 + b.kind];
    if (phaseEnd && (!special || life <= 0)) {
        if (life <= 0) g.bossLife = 0;
        switch (b.kind) {
        case 0:
            damage(b, g, env, slot);
            env.hook(H_HIT_MARK, slot, 0, 0);
            b.f1DA4 |= FA_DEAD;
            seqSetTable(b, 0xC);
            decNowEnemyAlvNumChk(b, g);
            if (!special && g.deathCount73F8 == 0) env.statFunc(2, 0xE);
            break;
        case 1:
            damage(b, g, env, slot);
            env.hook(H_HIT_MARK, slot, 0, 0);
            b.f1DA4 |= FA_DEAD;
            seqSetTable(b, 7);
            decNowEnemyAlvNumChk(b, g);
            break;
        case 2:
            g.g72D0 &= ~2;
            damage(b, g, env, slot);
            env.hook(H_HIT_MARK0, slot, partType, 0);
            env.hook(H_HIT_MARK, slot, 0, 0);
            b.f1DA4 |= FA_DEAD;
            env.registerHit(slot, 0x9C4);
            seqSetTable(b, 4);
            decNowEnemyAlvNumChk(b, g);
            if (!special) {
                if (env.statFunc(2, 8) == 0 && g.deathCount73FC == 0) env.statFunc(2, 9);
                if (g.deathCount73F8 == 0) env.statFunc(2, 0xF);
            }
            break;
        default: break;
        }
        return;
    }
    // golpe que no cambia de fase: cuenta golpes de la barra pequena
    env.hook(H_HIT_MARK0, slot, partType, 0);
    if (b.f1DA4 & FA_ATTACK) {
        b.hitCounter--;
        if (b.hitCounter <= 0) {
            b.hitCounter = b.hitCounterMax;
            b.phaseHits--;
            if (b.phaseHits <= 0) {
                env.registerHit(slot, 0xA);
                damage(b, g, env, slot);
                env.hook(H_HIT_MARK, slot, 0, 0);
                incBS6Difficulty(g, slot, 1, 1);
            } else damageUpperBody(b, env, slot);
        }
    } else {
        b.hitCounter--;
        if (b.hitCounter <= 0) damageUpperBody(b, env, slot);
    }
}
// NoWeakHit (0x9C7750): solo el tipo 2 avisa al motor.
void Boss6Logic::noWeakHit(const Boss6State& b, Boss6Env& env, int) {
    if (b.kind == 2) env.hook(H_NO_WEAK_FX, 0, 0, 0);
}
// DamageUpperBody (0x9C7780): destello de dano parcial.
void Boss6Logic::damageUpperBody(Boss6State& b, Boss6Env& env, int slot) {
    env.registerHit(slot, 0xA);
    int row;
    float f24;
    if (b.typeId == 0x25) {
        if (b.f1DA4 & FA_FORM2) { f24 = 8.125f; row = 0x33; }
        else { f24 = 3.75f; row = 0x30; }
    } else { f24 = 3.75f; row = 0x32; }
    b.f1E24 = f24;
    b.f1E2C = 1.0f;
    b.f1E1C = b.f1E24 * 1.0f;
    env.spawnEffect(b.pos, motionRow(row).id);
    MotionRow m = motionRow(row);
    b.rec[0].prev = b.rec[0].cur; b.rec[0].cur = m.a; b.rec[0].a = 5; b.rec[0].b = 5;
    b.rec[1].prev = b.rec[1].cur; b.rec[1].cur = m.b; b.rec[1].a = 5; b.rec[1].b = 5;
    env.sound(0x130014, 0, 0);
    b.f1DA4 |= FA_HITFLASH;
}
// Damage (0x9C7900): entra en la animacion de dano de la fase.
void Boss6Logic::damage(Boss6State& b, Boss6Globals&, Boss6Env& env, int) {
    b.f1DA4 |= FA_IN_DAMAGE_ANIM;
    b.rate = kRate; b.rate2 = kRate;
    env.setCamMode(1);
    env.sound(0x130016, 0, 0);
    setDamageMotion(b, env);
    b.f0C |= F0C_IN_DAMAGE;
    if (b.kind == 1) setLightColor(b, 5);
}
// SetDamageMotion (0x9C79A0): elige el movimiento de dano; la orientacion hacia la camara (matrices) es motor.
void Boss6Logic::setDamageMotion(Boss6State& b, Boss6Env& env) {
    int m;
    if (b.typeId == 0x25) {
        m = 0x30;
        if (b.f1DA4 & FA_FORM2) {
            b.f1DA4 &= ~(uint32_t)FA_PATH; m = 0x33;
            b.f1DA4 &= ~(uint32_t)FA_CAMPATH; b.f1DA4 |= FA_PATH_DONE;
        }
        env.hook(H_DAMAGE_MOTION_GEOMETRY, 0, 0, 0);
    } else {
        m = 0x34;
        b.f1DA4 &= ~(uint32_t)FA_PATH; b.f1DA4 &= ~(uint32_t)FA_CAMPATH;
        b.f1DA4 &= ~(uint32_t)FA_PATH_1000; b.f1DA4 &= ~(uint32_t)FA_PATH_800;
        b.f1DA4 |= FA_PATH_DONE;
    }
    motSupSet2(b, env, m, 0, 5);
}
// CheckDamageEnd (0x9C6380): fin del destello (0x20) y fin de la animacion de dano (0x4000). Solo la parte de reglas.
void Boss6Logic::checkDamageEnd(Boss6State& b, Boss6Env& env) {
    if (b.f1DA4 & FA_HITFLASH) {
        int lim = 0x1E;
        if (b.typeId == 0x25 && (b.f1DA4 & FA_FORM2)) lim = 0x41;
        if (!(b.f40 < (float)lim)) {
            b.f1DA4 &= ~(uint32_t)FA_HITFLASH;
            for (int i = 0; i < 2; i++) {
                b.rec[i].cur = b.rec[i].prev;
                b.rec[i].a = (int32_t)(10.0f + b.motionFrame);
                b.rec[i].b = 10;
            }
        }
    }
    if ((b.f1DA4 & FA_IN_DAMAGE_ANIM) && !((b.motionFrame + b.rate) < (float)env.motionLen(b.motion))) {
        b.f1DA4 &= ~(uint32_t)FA_IN_DAMAGE_ANIM;
        disableCollision(b);
        if (b.typeId == 0x25) {
            env.hook(H_RESTORE_POSE_AFTER_DAMAGE, 0, 0, 0);   // matrices: pendiente de reescribir
            if (b.f1DA4 & FA_FORM2) {
                motSupSet2(b, env, 0x18, 0, 0);
                if (!(b.f1DA4 & FA_DEAD)) seqSetTable(b, 5);
            } else motSupSet2(b, env, 8, 0, 0);
            b.parentF1C9C |= 2;
            return;
        }
        if (b.kind == 1 && !(b.f1DA4 & FA_DEAD)) b.rate = 2.4f;
    }
}
// batKillAdd (0x9C7B60): acumula bits de golpes de murcielagos si el ataque esta en curso y la bandera 0x200000.
void Boss6Logic::batKillAdd(Boss6State& b, int bits) {
    if ((b.f1DA4 & FA_ATTACK) && (b.f1DA4 & FA_ATK_200000)) b.hitFlags |= bits;
}

// ---------------------------------------------------------------- movimiento
// MotSupSet2 (0x9C9940): pide el movimiento `index` de la tabla (si no es el actual); mode == 0 -> arranque directo, otro -> mezcla.
void Boss6Logic::motSupSet2(Boss6State& b, Boss6Env& env, int index, int a, int mode) {
    if (b.curMotionIdx == index) return;
    b.f0C &= 0xBFFFFFFFu;
    MotionRow m = motionRow(index);
    if (mode == 0) env.startMotion(m.id);
    else env.startMotionBlend(m.id, a, mode);
    b.curMotionIdx = index;
    b.rec[0].prev = b.rec[0].cur = m.a;
    b.rec[0].a = a; b.rec[0].b = mode;
    b.rec[1].prev = b.rec[1].cur = m.b;
    b.rec[1].a = a; b.rec[1].b = mode;
}

// ---------------------------------------------------------------- VM de comandos
// SeqCmdInit (0x9C9690): 1DBC = tabla, 1DC0 = 0, 1DB8 = primer script, 1DC2 = 0, 1DC5 = 0.
void Boss6Logic::seqInit(Boss6State& b, uint32_t table) {
    b.seqTab = table; b.tabIdx = 0;
    b.pc = vmRead32(table);
    b.timer = 0; b.sel1DC5 = 0;
}
// SeqCmdNextTable: siguiente script de la tabla.
void Boss6Logic::seqNextTable(Boss6State& b) {
    b.tabIdx = (int16_t)(b.tabIdx + 1);
    b.pc = vmRead32(b.seqTab + (uint32_t)(int32_t)b.tabIdx * 4);
    b.timer = 0;
}
// SeqCmdSetTable: salta al script `idx`.
void Boss6Logic::seqSetTable(Boss6State& b, int idx) {
    b.tabIdx = (int16_t)idx;
    b.pc = vmRead32(b.seqTab + (uint32_t)(int32_t)b.tabIdx * 4);
    b.timer = 0;
}
// actExecSeqCmd (0x9C9720): ejecuta el interprete y devuelve 0.
int Boss6Logic::actExecSeqCmd(Boss6State& b, Boss6Globals& g, Boss6Env& env) { execSeqCmd(b, g, env); return 0; }

// ExecSeqCmd (0x9C7F10.0x9C9668): interprete de scripts. Ejecuta comandos hasta que uno pide esperar (wait = true).
// Las operaciones con matrices de camara (19,20,43 con P==0) y SetCircleMove (66) pasan por hook.
// Los ops 55,56,57,61,63 y >= 0x44 saltan al final del bucle sin avanzar pc: en el original eso es un bucle infinito; aqui se devuelve.
void Boss6Logic::execSeqCmd(Boss6State& b, Boss6Globals& g, Boss6Env& env) {
    float cam = env.camFrame();
    b.timer = (int16_t)(b.timer - 1);
    if (b.timer >= 0) return;
    bool wait = false;
    for (int guard = 0; !wait && guard < 100000; guard++) {
        uint32_t pc = b.pc;
        auto c = [&](int i) { return (int)vmRead16(pc + 2 * i); };
        auto adv = [&](int shorts) { b.pc = pc + 2 * shorts; };
        unsigned op = (uint16_t)vmRead16(pc);
        if (op >= 0x44) return;
        switch (op) {
        case 0: if (cam < (float)c(1)) wait = true; else adv(2); break;                       // espera camara >= P
        case 1: if (!(cam < (float)c(1))) { forbidCameraStart(g); adv(2); } else { allowCameraStart(g); wait = true; } break;
        case 2: if (!(b.motionFrame < (float)c(1))) adv(2); else wait = true; break;         // espera fotograma >= P
        case 3:
            if (b.motionFrame < (float)c(1)) { if (b.f1DA4 & FA_IN_DAMAGE_ANIM) adv(2); else wait = true; }
            else adv(2);
            break;
        case 4: if (!((b.motionFrame + b.rate) < (float)env.motionLen(b.motion))) adv(1); else wait = true; break;
        case 5: if (!((b.motionFrame + b.rate) < (float)(c(1) * env.motionLen(b.motion)))) adv(2); else wait = true; break;
        case 6: if (!(b.f0C & F0C_ATTACKING)) adv(1); else wait = true; break;
        case 7: wait = true; b.timer = (int16_t)c(1); adv(2); break;                          // espera P fotogramas (via 1DC2)
        case 8: g.cameraStartAllowed = 1; adv(1); break;
        case 9: if (g.cameraStartAllowed == 0) adv(1); else wait = true; break;
        case 10: motSupSet2(b, env, c(1), c(2), c(3)); adv(4); break;
        case 11: motSupSet2(b, env, attackRow(c(1)).motion, 0, 0); adv(2); break;
        case 12: b.rate = kRate * (kInv256 * (float)c(1)); adv(2); break;
        case 13:
            if (!(b.f1DA4 & FA_IN_DAMAGE_ANIM)) {
                setSpeedBaseRate(b);
                b.rate = kRate * (1.0f / (b.speedBase * kSpeedMul[getBS6Difficulty(g, -1)]));
            }
            adv(1);
            break;
        case 14: b.rot[1] = c(1); adv(2); break;
        case 15: b.rot[0] = 0; b.rot[1] += c(1); b.f1F7C = c(1); b.f1F94 = c(1); adv(2); break;
        case 16: b.rot[2] = 0; b.rot[1] = 0; b.rot[0] = 0; b.pos[2] = 0; b.pos[1] = 0.0f; b.pos[0] = 0; adv(1); break;
        case 17: {
            const Pose* p;
            if (b.kind == 2) p = &kPoseD_flat[b.sel1DC4];
            else if (b.kind == 1) p = &kPoseB_flat[b.sel1DC4 * 4 + b.sel1DC5];
            else p = &kPoseA_flat[b.sel1DC4 * 3 + b.sel1DC5];
            b.pos[0] = p->p[0]; b.pos[1] = p->p[1]; b.pos[2] = p->p[2];
            b.rot[0] = p->r[0]; b.rot[1] = p->r[1]; b.rot[2] = p->r[2];
            if (b.kind == 1) setLightColor(b, b.sel1DC5 + 6);
            b.f1E14 = b.rot[1];
            adv(1);
            break; }
        case 18: {
            const Pose& p = kPoseC_flat[b.sel1DC4 * 5 + b.sel1DC5];
            b.pos[0] = p.p[0]; b.pos[1] = p.p[1]; b.pos[2] = p.p[2];
            b.rot[0] = p.r[0]; b.rot[1] = p.r[1]; b.rot[2] = p.r[2];
            b.pos[1] = b.pos[1] - 30.0f;
            adv(1);
            break; }
        case 19: env.hook(H_CAM_TRANSFORM_A, 0, 0, 0); b.parentF1C9C &= ~2u; adv(1); break;
        case 20: env.hook(H_CAM_TRANSFORM_B, 0, 0, 0); b.parentF1C9C |= 2u; adv(1); break;
        case 21: b.parentF1C9C &= ~2u; adv(1); break;
        case 22: b.parentF1C9C |= 2u; adv(1); break;
        case 23: if (c(2) != 0) b.f1DA4 |= (1u << (c(1) & 31)); else b.f1DA4 &= ~(1u << (c(1) & 31)); adv(3); break;
        case 24:
            if (!(b.f1DA4 & FA_IN_DAMAGE_ANIM)) { selectAttackTarget(b, g, env); setBulletNum(b, g, (int8_t)c(1)); startAttack(b, g, env, (int8_t)c(1)); }
            adv(2);
            break;
        case 25: {
            int a = (int8_t)c(2 + b.sel1DC5);
            if (!(b.f1DA4 & FA_IN_DAMAGE_ANIM)) { selectAttackTarget(b, g, env); setBulletNum(b, g, a); startAttack(b, g, env, a); }
            adv(c(1) + 2);
            break; }
        case 26: env.hook(H_SPAWN_ALLBAT, 0, 0, 0); adv(1); break;
        case 27: setLightColor(b, c(1)); adv(2); break;
        case 28:
            b.f1DA4 |= FA_PATH; b.f1DA4 &= ~(uint32_t)FA_CAMPATH; b.f1DA4 &= ~(uint32_t)FA_PATH_DONE;
            if (b.f1DA4 & FA_PATH_800) { for (int i = 0; i < 3; i++) { b.savedPos[i] = b.pos[i]; b.savedRot[i] = b.rot[i]; } }
            b.pathId = c(1); b.pathFrame = 0.0f; b.pathEnd = getPathEndFrame(b, env);
            adv(2);
            break;
        case 29:
            if (!(b.f1DA4 & FA_IN_DAMAGE_ANIM)) {
                b.f1DA4 |= FA_PATH; b.f1DA4 &= ~(uint32_t)FA_CAMPATH; b.f1DA4 &= ~(uint32_t)FA_PATH_DONE;
                b.pathId = c(2 + b.sel1DC5); b.pathFrame = 0.0f; b.pathEnd = getPathEndFrame(b, env);
            }
            adv(c(1) + 2);
            break;
        case 30:
            b.f1DA4 |= FA_PATH; b.f1DA4 |= FA_CAMPATH; b.f1DA4 &= ~(uint32_t)FA_PATH_DONE;
            b.pathId = env.camTrackId(); b.pathFrame = cam; b.pathEnd = getPathEndFrame(b, env);
            adv(2);
            break;
        case 31:
            b.f1DA4 &= ~(uint32_t)FA_PATH; b.f1DA4 &= ~(uint32_t)FA_CAMPATH; b.f1DA4 &= ~(uint32_t)FA_PATH_1000;
            b.f1DA4 &= ~(uint32_t)FA_PATH_800; b.f1DA4 |= FA_PATH_DONE;
            adv(1);
            break;
        case 32: b.rate2 = kRate * ((float)c(1) / 256.0f); adv(2); break;
        case 33:
            if (!(b.f1DA4 & FA_IN_DAMAGE_ANIM)) b.rate2 = kRate * (1.0f / (b.speedBase * kSpeedMul[getBS6Difficulty(g, -1)]));
            adv(1);
            break;
        case 34: if (!(b.pathFrame < (float)c(1))) adv(2); else wait = true; break;
        case 35: if (b.f1DA4 & FA_PATH_DONE) adv(1); else wait = true; break;
        case 36: setApproach(b, c(1)); adv(2); break;
        case 37: if (!(b.f1DA4 & FA_APPROACH)) adv(1); else wait = true; break;
        case 38: env.setCamMode(c(1)); adv(2); break;
        case 39: b.sel1DC4 = 0; adv(1); break;
        case 40: {
            b.sel1DC4 = (int8_t)(b.sel1DC4 + (int8_t)c(1));
            int m = c(2);
            if (m == 0) return;                                   // el original ejecuta `break 7` (division por cero)
            b.sel1DC4 = (int8_t)((int)b.sel1DC4 % m);
            if (b.sel1DC4 < 0) b.sel1DC4 = (int8_t)(b.sel1DC4 + (int8_t)c(2));
            adv(3);
            break; }
        case 41: bodyAlphaSupSet(b, (float)c(1) / 255.0f); adv(2); break;
        case 42: bodyAlphaDirectSet(b, (float)c(1) / 255.0f); adv(2); break;
        case 43:
            if (c(1) == 0) env.hook(H_CAM_POS_TO_SPAWN, 0, 0, 0);
            else { const Pose& p = kPoseC_flat[b.sel1DC4 * 5 + b.sel1DC5]; for (int i = 0; i < 3; i++) b.spawnPos[i] = p.p[i]; }
            env.spawnMask(b.spawnPos, c(2));
            env.sound(0x1E000F, 0x1E000F, 0);
            adv(2);
            break;
        case 44:
            for (int i = 0; i < 5; i++) {
                if (i != b.sel1DC5) {
                    const Pose& p = kPoseC_flat[b.sel1DC4 * 5 + i];
                    for (int k = 0; k < 3; k++) b.spawnPos[k] = p.p[k];
                    env.spawnMask(b.spawnPos, 2);
                }
            }
            adv(1);
            break;
        case 45: {
            const int8_t* row;
            int d = getBS6Difficulty(g, -1);
            if (b.kind == 0) row = kHitCount0_flat + b.sel1DC5 * 68;
            else if (b.kind == 1) row = kHitCount1_flat + b.sel1DC5 * 68;
            else row = kHitCount2_flat + b.kind * 68;
            row += d * 2;
            b.hitCounter = b.hitCounterMax = row[0];
            b.phaseHits = b.phaseHitsMax = row[1];
            b.hitFlags = 0;
            adv(1);
            break; }
        case 46: enableCollision(b); adv(1); break;
        case 47: disableCollision(b); adv(1); break;
        case 48: b.state = 1; b.stateB = 0; adv(1); break;
        case 49: seqNextTable(b); break;
        case 50: seqSetTable(b, c(1)); break;
        case 51: if (b.f1DA4 & FA_DEAD) seqSetTable(b, c(1)); else adv(2); break;
        case 52: if (b.f1DA4 & FA_IN_DAMAGE_ANIM) seqSetTable(b, c(1)); else adv(2); break;
        case 53:
            if (c(1) != 0) b.f1DA4 |= (FA_ALPHA_RUN | FA_LIFE_BLINK);
            else b.f1DA4 &= ~(uint32_t)(FA_ALPHA_RUN | FA_LIFE_BLINK);
            adv(2);
            break;
        case 54: wait = true; break;
        case 58: b.sel1DC5 = (int8_t)c(1); adv(2); break;
        case 59: {
            int m = c(2), base = c(1);
            int r = env.rand();
            if (m == 0) return;                                   // `break 7` en el original
            b.sel1DC5 = (int8_t)(base + r % m);
            adv(3);
            break; }
        case 60: b.sel1DC5 = (int8_t)(c(1) + getRandomBranchRate(b, env, c(2))); adv(3); break;
        case 62: { int k = c(1); int t = c(2 + b.sel1DC5); b.pc = pc + 2 * (k + 2); seqSetTable(b, t); wait = true; break; }
        case 64: if (c(1) != 0) env.hook(H_INIT_BLUR, 0, 0, 0); else env.hook(H_END_BLUR, 0, 0, 0); adv(2); break;
        case 65: {
            int id = kSoundIds[c(1)];
            int flag = (c(2) == 1) ? 0 : 1;
            if (id != 0) env.sound(id, id, flag);
            adv(3);
            break; }
        case 66: env.hook(H_SET_CIRCLE_MOVE, c(1), 0, 0); adv(2); break;
        case 67: if (!(b.f1DA4 & FA_CIRCLE)) adv(1); else wait = true; break;
        default: return;      // 55,56,57,61,63: bucle infinito en el original
        }
    }
}

// ---------------------------------------------------------------- ataques
// SelectAttackTarget (0x9C9A70): elige jugador objetivo (aleatorio entre dos libres) y lo marca ocupado.
bool Boss6Logic::selectAttackTarget(Boss6State& b, Boss6Globals& g, Boss6Env& env) {
    if (g.playersMode == 2) {
        int a = env.rand() & 1;
        if (g.targetBusy[a] != 0) {
            a = (a + 1) & 1;
            if (g.targetBusy[a] != 0) { g.targetBusy[1] = 0; g.targetBusy[0] = 0; }
        }
        b.target = (int8_t)a;
    } else if (g.playersMode == 1) b.target = (int8_t)g.lastHitTarget;
    else b.target = 0;
    if (!env.playerAlive(b.target)) return false;
    if (b.target == -1) return false;
    g.targetBusy[b.target] = 1;
    return true;
}
// SetBulletNum (0x9C9B50): numero de disparos segun el tipo de ataque (fila +0xA) y la dificultad.
void Boss6Logic::setBulletNum(Boss6State& b, Boss6Globals& g, int attack) {
    switch (attackRow((int8_t)attack).kind) {
    case 0: case 4: b.bulletNum = kBulletNum[getBS6Difficulty(g, -1)]; break;
    case 5: b.bulletNum = 6; break;
    case 6: b.bulletNum = 4; break;
    default: b.bulletNum = 1; break;
    }
}
// AtcPlFlgClear (0x9C9C20): libera el jugador objetivo.
void Boss6Logic::atcPlFlgClear(Boss6State& b, Boss6Globals& g) {
    if (b.target != -1) { g.targetBusy[b.target] = 0; b.target = -1; }
}
// StartAttack (0x9C62A0): inicia el ataque `attack` (fila de D_009D5000).
void Boss6Logic::startAttack(Boss6State& b, Boss6Globals& g, Boss6Env& env, int attack) {
    b.attack = (int8_t)attack; b.atkStep = 0; b.s1E80 = 0;
    b.gameMode = (int8_t)g.gameMode;
    b.atkDiff = (int8_t)getBS6Difficulty(g, -1);
    int m = attackRow(b.attack).motion;
    if (m >= 0) {
        if (m == 0x27) motSupSet2(b, env, m, 0, 5);
        else if (m == 0x18) motSupSet2(b, env, m, 0, 0);
        else motSupSet2(b, env, m, 0, 0xA);
    }
    b.atkCount = 0;
    b.f0C |= F0C_ATTACKING;
    b.f1DA4 |= FA_ATTACK;
}
// AttackDamageFrame (0x9C58C0): lo que ocurre en el fotograma de dano segun el tipo de ataque.
void Boss6Logic::attackDamageFrame(Boss6State& b, Boss6Globals& g, Boss6Env& env) {
    int kind = attackRow(b.attack).kind;
    switch (kind) {
    case 0: if (selectAttackTarget(b, g, env)) b.atkFlag = 1; break;
    case 1:
        if (selectAttackTarget(b, g, env)) {
            b.atkFlag = 1;
            env.hook(H_SET_BAT_SPIRAL, 0, 0, 0);
            env.sound(0x1D000F, 0, 0);
            env.sound(0x1D0012, 0x1D0012, 0);
        }
        break;
    case 2:
        b.f1DA4 |= FA_ATK_80000;
        if (env.damagePlayer(b.target, 1, 5) != 0) { decBS6Difficulty(g, b.target, 2, 2); env.sound(0x800114, 0, 0); }
        break;
    case 3: selectAttackTarget(b, g, env); break;
    case 4: if (selectAttackTarget(b, g, env)) env.hook(H_NEW_BULLET, 0, b.atkCount, 0); break;
    case 5: if (selectAttackTarget(b, g, env)) env.hook(H_NEW_BULLET, 1, b.atkCount, 0); break;
    case 6: if (selectAttackTarget(b, g, env)) env.hook(H_NEW_BULLET, 2, b.atkCount, 0); break;
    case 7:
        if (selectAttackTarget(b, g, env)) { env.hook(H_NEW_WATER, 0, 0, 0); env.sound(0x130010, 0, 0); env.hook(H_CLEAR_BULLET_DAMAGE, 0, 0, 0); }
        break;
    case 8: if (env.damagePlayer(b.target, 1, 9) != 0) decBS6Difficulty(g, b.target, 2, 2); break;
    default: break;
    }
}
// ControlAttack (0x9C5B10): avanza el ataque en curso (pasos 0, 100, 10, 11, -1).
void Boss6Logic::controlAttack(Boss6State& b, Boss6Globals& g, Boss6Env& env) {
    if (!(b.f1DA4 & FA_ATTACK)) return;
    auto endAttack = [&](bool sound) {
        b.atkFlag = 0;
        b.f0C &= 0xEFFFFFFFu;
        b.f1DA4 &= ~(uint32_t)FA_ATTACK; b.f1DA4 &= ~(uint32_t)FA_ATK_8000;
        b.f1DA4 &= ~(uint32_t)FA_ATK_200000; b.f1DA4 &= ~(uint32_t)FA_ATK_400000;
        if (sound) loopSoundOff(env);
    };
    b.atkFlag = 0;
    AttackRow r = attackRow(b.attack);
    if (r.motion > 0) {
        if (b.motion != motionRow(r.motion).id) { endAttack(true); return; }
        if (b.f1DA4 & FA_IN_DAMAGE_ANIM) { endAttack(true); return; }
    }
    b.atkKind = (int8_t)r.kind;
    float fr = b.motionFrame;
    switch (r.kind) {
    case 2: case 8:
        switch (b.atkStep) {
        case -1:
            if (!((fr + b.rate) < (float)r.endFrame)) {
                b.f1DA4 &= ~(uint32_t)FA_ATTACK; b.f0C &= 0xEFFFFFFFu;
                b.f1DA4 &= ~(uint32_t)FA_ATK_8000; b.f1DA4 &= ~(uint32_t)FA_ATK_200000; b.f1DA4 &= ~(uint32_t)FA_ATK_400000;
            }
            return;
        case 0:
            if (r.startFrame == -1) { b.atkStep = 10; return; }
            if (!(fr < (float)r.startFrame)) b.atkStep = 10;
            return;
        case 10: {
            if (r.dmgFrame == -1) { b.atkStep = -1; return; }
            float lim = (float)r.dmgFrame - 20.0f;
            if (fr < lim && !((fr + b.rate) < lim)) {
                if (b.atkKind == 2) env.sound(0x130010, 0, 0);
                else if (b.atkKind == 8) env.sound(0x13001A, 0, 0);
                return;
            }
            if (!(fr < (float)r.dmgFrame)) { b.f1DA4 |= FA_ATK_8000; b.atkStep = 11; }
            return; }
        case 11:
            if (!(fr < (float)r.f8)) { attackDamageFrame(b, g, env); b.f1DA4 &= ~(uint32_t)FA_ATK_8000; b.atkStep = -1; }
            return;
        default: return;
        }
    case 0: case 1: case 3: case 4: case 5: case 6: case 7:
        if (!((fr + b.rate) < (float)r.endFrame)) { endAttack(true); }
        switch (b.atkStep) {
        case -1: return;
        case 100:
            if (r.f8 == -1) { b.atkStep = -1; return; }
            if (!(fr <= (float)r.f8)) {
                b.atkFlag = 1; b.atkStep = -1;
                switch (b.atkKind) {
                case 4: env.sound(0x130010, 0, 0); env.sound(0x1E001B, 0, 0); env.hook(H_CLEAR_BULLET_DAMAGE, 0, 0, 0);
                        env.sound(0x130010, 0, 0); env.hook(H_CLEAR_BULLET_DAMAGE, 0, 0, 0); break;   // el caso 4 cae en el 5
                case 5: env.sound(0x130010, 0, 0); env.hook(H_CLEAR_BULLET_DAMAGE, 0, 0, 0); break;
                case 6: env.sound(0x130010, 0, 0); env.sound(0x1E0015, 0, 0); env.hook(H_CLEAR_BULLET_DAMAGE, 0, 0, 0); break;
                case 7: env.hook(H_CLEAR_BULLET_DAMAGE, 0, 0, 0); break;
                default: break;
                }
            } else if (b.atkKind == 1) { env.hook(H_ADD_BAT_SPIRAL_COUNT, 0, 0, 0); b.atkFlag = 1; }
            return;
        default:   // 0 y cualquier otro valor
            if (r.startFrame == -1) { b.atkStep = 100; return; }
            if (r.startFrame < r.dmgFrame) {
                int n = b.bulletNum;
                if (n == 0) return;                                   // `break 7` en el original
                b.dmgMul = (int32_t)(((int)b.atkStep * 0xFFFF) / n);
                float thr = (float)r.startFrame + (float)b.atkStep * ((float)(r.dmgFrame - r.startFrame) / (float)(n + 1));
                if (fr <= thr) return;
                attackDamageFrame(b, g, env);
                if (r.kind == 0) { int v = env.rand() % 100; b.atkCount = (int8_t)v; if (b.atkCount < 0) b.atkCount = (int8_t)(b.atkCount + 100); }
                else b.atkCount = (int8_t)(b.atkCount + 1);
                b.atkStep = (int8_t)(b.atkStep + 1);
                if (b.atkStep >= b.bulletNum) b.atkStep = 100;
            } else {
                if (fr <= (float)r.startFrame) return;
                attackDamageFrame(b, g, env);
                b.atkStep = 100;
            }
            return;
        }
    default: return;
    }
}
// DecNowEnemyAlvNumChk (0x9C9C50): descuenta una vez de los enemigos vivos.
void Boss6Logic::decNowEnemyAlvNumChk(Boss6State& b, Boss6Globals& g) {
    if (!(b.f10 & 2)) { b.f10 |= 2; g.enemiesAlive = (int16_t)(g.enemiesAlive - 1); }
}
// DecNowEnemyNumChk: descuenta una vez del contador de enemigos.
void Boss6Logic::decNowEnemyNumChk(Boss6State& b, Boss6Globals& g) {
    if (!(b.f10 & 4)) { b.f10 |= 4; g.enemiesLeft = (int16_t)(g.enemiesLeft - 1); }
}

// ---------------------------------------------------------------- estados de tarea
// actNop (0x9C7E70): devuelve 1.
int Boss6Logic::actNop() { return 1; }
// actDelete (0x9C7E80): 1E61 == 0 -> marca borrado y pasa a 1; 1E61 == 1 -> FinishTask. Devuelve 0.
int Boss6Logic::actDelete(Boss6State& b, Boss6Globals& g, Boss6Env& env) {
    if (b.stateB == 1) finishTask(env);
    else if (b.stateB == 0) {
        b.f0C |= F0C_DEAD;
        atcPlFlgClear(b, g);
        b.f0C |= 0x4000;
        b.f10 |= 0x20;
        b.f18 = 0;
        decNowEnemyNumChk(b, g);
        b.stateB = (uint8_t)(b.stateB + 1);
    }
    return 0;
}
// actDead (0x9C9740): devuelve 0.
int Boss6Logic::actDead() { return 0; }
// FinishTask (0x9C9840): delega en func_950F80.
void Boss6Logic::finishTask(Boss6Env& env) { env.hook(H_FINISH_TASK, 0, 0, 0); }
// FinishTask0 (0x9C9750): libera las barras y el sonido en bucle; en modo especial descuenta enemigos.
void Boss6Logic::finishTask0(Boss6State& b, Boss6Globals& g, Boss6Env& env) {
    if (b.kind == 2) env.setCamMode(1);
    for (int i = 0; i < 2; i++) if (b.gauge[i]) { env.hook(H_FREE_GAUGE, i, 0, 0); b.gauge[i] = false; }
    loopSoundOff(env);
    if (g.gameState == 0xA && g.gameKind == 2) { decNowEnemyAlvNumChk(b, g); decNowEnemyNumChk(b, g); }
}
// AllowCameraStart (0x9C9860): 0x500F07 = 1.
void Boss6Logic::allowCameraStart(Boss6Globals& g) { g.cameraStartAllowed = 1; }
// ForbidCameraStart: 0x500F07 = 0.
void Boss6Logic::forbidCameraStart(Boss6Globals& g) { g.cameraStartAllowed = 0; }
// EnableCollision (0x9C9880): limpia 0x8000 y, si no esta 0x40, quita 0x40 de +0x1A8 de las partes.
void Boss6Logic::enableCollision(Boss6State& b) {
    b.f0C &= 0xFFFF7FFFu;
    if (!(b.f1DA4 & FA_NO_PART_COLLISION))
        for (int i = 0; i < 4; i++) if (b.partPresent[i]) b.partF1A8[i] &= ~0x40u;
}
// DisableCollision: pone 0x8000 y 0x40 de +0x1A8 de las partes.
void Boss6Logic::disableCollision(Boss6State& b) {
    b.f0C |= F0C_COLLISION_OFF;
    if (!(b.f1DA4 & FA_NO_PART_COLLISION))
        for (int i = 0; i < 4; i++) if (b.partPresent[i]) b.partF1A8[i] |= 0x40u;
}
// LoopSoundOff (0x9CACB0): para el sonido en bucle 0x1D0012.
void Boss6Logic::loopSoundOff(Boss6Env& env) { env.sound(0x1D0012, 0x1D0012, 1); }

// ---------------------------------------------------------------- ruta, alfa, ramas, luz, velocidad
// GetPathEndFrame (0x9C9CB0): fotograma final de la ruta `pathId` (1 si pathId < 0).
int Boss6Logic::getPathEndFrame(Boss6State& b, Boss6Env& env) {
    b.pathEnd = b.pathId >= 0 ? env.pathEndFrame(b.pathId) : 1;
    return b.pathEnd;
}
// SetApproach (0x9C56A0): arranca un acercamiento: guarda pose actual y parametros.
void Boss6Logic::setApproach(Boss6State& b, int param) {
    b.f1DA4 |= FA_APPROACH;
    for (int i = 0; i < 3; i++) { b.apprPos[i] = b.pos[i]; b.apprRot[i] = b.rot[i]; }
    b.f1EC4 = 0; b.f1EC8 = 1.0f; b.f1ECC = (float)param;
}
// BodyAlphaControl (0x9CA2C0): acerca el alfa actual (1F00) al objetivo (1F04) con pasos 0.05 (sube) o 0.03 (baja).
void Boss6Logic::bodyAlphaControl(Boss6State& b) {
    float tgt = b.alphaTarget;
    float cur;
    if (b.f1DA4 & FA_IN_DAMAGE_ANIM) { tgt = 1.0f; cur = b.alphaCur; }
    else {
        if ((b.f1DA4 & FA_HITFLASH) && tgt < 0.5f) tgt = 0.5f;
        cur = b.alphaCur;
    }
    float step = (cur <= tgt) ? 0.05f : 0.03f;
    if (tgt == 1.0f && !(cur <= 0.95f)) b.alphaCur = 1.0f;
    else if (tgt != cur) b.alphaCur = b.alphaCur + step * (tgt - cur);
    if (tgt <= 0.01f && b.alphaCur < 0.01f) b.alphaCur = 0.01f;
    else if (!(b.alphaCur <= 1.0f)) b.alphaCur = 1.0f;
    b.alphaOut2 = b.alphaCur; b.alphaOut1 = b.alphaCur;
}
// BodyAlphaDirectSet (0x9CA420): alfa actual y objetivo.
void Boss6Logic::bodyAlphaDirectSet(Boss6State& b, float a) { b.alphaCur = a; b.alphaTarget = a; }
// BodyAlphaSupSet: solo el objetivo.
void Boss6Logic::bodyAlphaSupSet(Boss6State& b, float a) { b.alphaTarget = a; }
// InitBranchRate (0x9CAAE0): 18 pesos a 1.0.
void Boss6Logic::initBranchRate(Boss6State& b) { for (int i = 0; i < 18; i++) b.branchRate[i] = 1.0f; }
// GetRandomBranchRate (0x9CAB60): elige una rama entre n con probabilidad proporcional a su peso, evitando repetir
// la elegida dos veces seguidas; reduce el peso elegido a 0.33. Sin acierto devuelve 0 (en el original el valor inicial
// es el registro s0 del llamador; ExecSeqCmd lo tiene a un puntero, que es >= n).
int Boss6Logic::getRandomBranchRate(Boss6State& b, Boss6Env& env, int n) {
    int excl = b.b1E10;
    if (excl != b.b1E11) excl = -1;
    float total = 0.0f;
    for (int i = 0; i < n; i++) if (i != excl) total += b.branchRate[i];
    float r = total * env.randFloat();
    float acc = 0.0f;
    int pick = -1;
    for (int i = 0; i < n; i++) {
        if (i != excl) acc += b.branchRate[i];
        if (r < acc) { pick = i; break; }
    }
    if (pick < 0 || pick >= n) pick = 0;
    if (total < 1.0f) initBranchRate(b);
    b.b1E11 = b.b1E10; b.b1E10 = (int8_t)pick;
    b.branchRate[pick] = b.branchRate[pick] * 0.33f;
    return pick;
}
// SetSpeedBaseRate (0x9CB010): multiplicador base de velocidad segun tipo y rama.
void Boss6Logic::setSpeedBaseRate(Boss6State& b) {
    float v = 1.0f;
    switch (b.kind) {
    case 0:
        switch (b.sel1DC5) { case 0: v = 0.95f; break; case 1: v = 1.52f; break; case 2: v = 0.95f; break; default: break; }
        break;
    case 1:
        switch (b.b1E10) { case 0: v = 1.4f; break; case 1: v = 1.35f; break; default: break; }
        break;
    default: break;
    }
    b.speedBase = v;
}
// setLightColor (0x9C7BA0): carga el color de luz `idx` (0..4 -> filas 5..9 de la tabla; 6..9 -> filas 6..9) salvo que
// ya sea el actual (idx 5 -> 0 si no hay luz; idx-5 == luz actual). Guarda el indice final en +0x1FA0.
void Boss6Logic::setLightColor(Boss6State& b, int idx) {
    b.lightCount = 0; b.lightRes[2] = 0; b.lightRes[1] = 0; b.lightRes[0] = 0;
    auto load = [&](int row) {
        const float* k = kLight_flat + row * 9;
        for (int i = 0; i < 3; i++) { b.lightColA[i] = k[i]; b.lightColB[i] = k[3 + i]; b.lightColC[i] = k[6 + i]; }
    };
    if (idx < 5) load(5 + idx);
    else if (idx == 5) { if (b.light == 0) idx = 0; }
    else if (b.light == idx - 5) idx -= 5;
    else load(idx);
    b.light = idx;
}
// CtrlBossLight (0x9CA440): las luces >= 5 caducan a los 0x25 fotogramas.
void Boss6Logic::ctrlBossLight(Boss6State& b) {
    if (b.light >= 5) { b.lightCount++; if (b.lightCount >= 0x25) b.light -= 5; }
}
// B6InterDivAngF (0x9C55F0): interpola un angulo de 16 bits por el camino corto con factor t.
int Boss6Logic::b6InterDivAngF(int a, int bb, float t) {
    int diff = (bb - a) & 0xFFFF;
    float one = 1.0f - t;
    float den = t + one;
    int lim = 32767;                       // (int16_t)(int)0x46FFFE2E: 32767.09 truncado
    float v;
    if (lim >= diff) v = ((float)diff * t) / den;
    else v = -((float)(0x10000 - diff) * t) / den;
    return a + (int)v;
}

}}  // namespace vn::boss6

// Ver enemies3.h. Reglas de las clases En* auxiliares de GAMEMOD, reescritas a mano a partir de asm/gamemod/gamemod_text.s.
#include "enemies3.h"
#include <cstring>
namespace vn { namespace en3 {

static inline int trunc32(float f) { return (int)f; }   // cvt.w.s / func_14C518: trunca hacia cero

// ---------------------------------------------------------------------------------------------------------------------
// EnmIDInit 0x9356F0
void EnmIdTable::init() { for (int i = 0; i < N; i++) slot[i] = 0; }
// EnmIDSet 0x935740: +0x24 = -1; busca el primer hueco libre (0..31); si lo hay: flags2 |= 1, guarda la tarea y su id.
int EnmIdTable::set(uint32_t taskId, uint32_t& flags2, int32_t& idField) {
    idField = -1;
    for (int i = 0; i < N; i++) {
        if (slot[i] == 0) { flags2 |= 1; slot[i] = taskId; idField = i; return i; }
    }
    return -1;
}
// EnmIDClear 0x9357A0: id s8 en 0..31 y flags2 & 1 -> libera el hueco (no toca flags2 ni el id).
void EnmIdTable::clear(uint32_t& flags2, int32_t idField) {
    int8_t id = (int8_t)idField;
    if (id >= 0 && id < N && (flags2 & 1)) slot[id] = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Enemy_Set_ColliBuff 0x92CD90
bool enemySetColliBuff(ColliTask& t, int gameState, ColliBuffer& buf, Ev3s& out) {
    if (gameState == 0xF || !(buf.count < 0xFF)) return false;
    t.hit70 = 0; t.hit71 = 0;
    if (t.flags & F_NOCOLLI) return false;
    if (!(t.flags & F_BIT10) && !(t.viewZ <= 0.0f)) return false;   // sin 0x10 solo se registra si esta delante de la camara (z <= 0)
    if (t.flags & F_BIT10) emit3(out, E3_COLLI_XFORM, (int)t.id);   // el motor transforma la colision (func_1B00E0/1B0138/1AFD54)
    ColliEntry& e = buf.e[buf.count];
    e.task = t.id; e.flags = t.flags; e.pos = t.collPos;
    buf.count++;
    return true;
}

// Enemy_Set_WaitPosBuff 0x936170: solo tareas con bit 1 y sin bit 0x04000000; maximo 14 entradas (gp-0x6FE4 < 0xE sin signo).
bool enemySetWaitPosBuff(uint32_t taskFlags, uint32_t taskId, uint32_t waitPos, float distToCam, WaitPosBuffer& buf) {
    if (!(taskFlags & 1) || (taskFlags & F_DEAD) || !(buf.count < 0xE)) return false;
    WaitPosEntry& e = buf.e[buf.count];
    e.distTimes10 = trunc32(10.0f * distToCam);
    e.task = taskId; e.waitPos = waitPos;
    buf.count++;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// EnSndReq 0x9426E0. Tabla D_009857A0 (de GAMEMOD.BIN): {0x2001B, 0x20018, 0x2001B, 0x2002B, 0x140016, -1}.
const int32_t kEnSndTable[6] = {0x2001B, 0x20018, 0x2001B, 0x2002B, 0x140016, -1};
bool enSndReq(int id, Env3& env, SndSlot* slots, Ev3s& out) {
    for (int i = 0; kEnSndTable[i] != -1; i++) {
        if (kEnSndTable[i] != id) continue;
        int d = slots[i].lastFrame - env.frame739C;
        if (d < 0) d = -d;
        if (d >= 5) {
            slots[i].lastFrame = env.frame739C;
            if (env.soundOn) { emit3(out, E3_SOUND, id); return true; }
        }
        return false;   // encontrado: no sigue buscando
    }
    return false;
}

// EntryRankingBufferSub 0x9708C0: indice de letra del nombre -> ASCII. 0..25 = 'A'..'Z', 26..35 = '0'..'9', 36 '&', 37 '!', 38 '?', 39 '.', 40 '-', otro ' '.
int entryRankingChar(int idx16) {
    int idx = (int16_t)idx16;
    if (idx < 0x1A) return idx + 0x41;
    if (idx < 0x24) return idx + 0x16;
    switch (idx) {
        case 36: return 0x26;
        case 37: return 0x21;
        case 38: return 0x3F;
        case 39: return 0x2E;
        default: return idx == 0x28 ? 0x2D : 0x20;
    }
}

// EnPartsChain 0x93B1E0: ambas llamadas pasan el EnemyTask original (a0 no se toca antes de la primera llamada).
void enPartsChain(Ev3s& out, uint8_t a1, uint8_t a2, int8_t a3, int8_t t0, int8_t t1) {
    // new_EnEtcParts(task, a1, a2, modo, (u32)(s8)a3, t0, t1)
    emit3(out, E3_NEW_PARTS, a1, a2, 2, (int)a3, (int)t0, (int)t1);
    emit3(out, E3_NEW_PARTS, a1, a2, 3, (int)a3, (int)t0, (int)t1);
}

// ---------------------------------------------------------------------------------------------------------------------
// EnemyMgrClass::entryEvent 0x935380
int mgrEntryEvent(const std::vector<int32_t>& s, int idx, Ev3s& out) {
    int p = idx + 1;
    do { emit3(out, E3_ENTRY_SUB, p); p++; } while (s[p] != -1);
    return p + 1;
}
// entryEvent2_MENM 0x935480: una entrada por cada fallo acumulado (gp-0x7358 s16), en orden.
int mgrEntryEvent2Menm(const std::vector<int32_t>& s, int idx, const Env3& env, Ev3s& out) {
    int16_t miss = env.missCount;
    int p = idx;
    do {
        if (miss != 0) { emit3(out, E3_ENTRY_SUB, p); miss--; }
        p++;
    } while (s[p] != -1);
    return p;
}
// entryEvent2_RAND 0x9354F0: s[idx] = cuantas entradas elegir; las entradas siguen hasta -1.
int mgrEntryEvent2Rand(const std::vector<int32_t>& s, int idx, const Env3& env, Ev3s& out) {
    int count = s[idx];
    int n = 1;
    while (s[idx + 1 + n] != -1) n++;   // el original cuenta mirando la palabra siguiente
    if (count < n) {
        std::vector<char> used(n, 0);
        for (int i = 0; i < count; i++) {
            float r = (float)(n - i) * (float)env.rand01();
            int k = trunc32(r);
            while (used[k] != 0) k++;
            used[k] = 1;
            emit3(out, E3_ENTRY_SUB, idx + 1 + k);
        }
    } else {
        for (int i = 0; i < n; i++) emit3(out, E3_ENTRY_SUB, idx + 1 + i);
    }
    return idx + n + 1;
}
// entryEvent2 0x935420: s[idx+1] = modo (0 = MENM, 1 = RAND); devuelve el indice tras el -1.
int mgrEntryEvent2(const std::vector<int32_t>& s, int idx, const Env3& env, Ev3s& out) {
    int mode = s[idx + 1];
    int p = idx + 2;
    if (mode == 0) p = mgrEntryEvent2Menm(s, p, env, out);
    else if (mode == 1) p = mgrEntryEvent2Rand(s, p, env, out);
    return p + 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// EnEtcTask::St4_02TimerBunki 0x9387A0
void etcSt4TimerBunki(EtcTask& t, Env3& env, Ev3s& out) {
    if (t.state == 0) {
        t.a148 = (t.v50 * 0x32) / 60;       // fotogramas de 60 Hz -> de 50 Hz
        env.missCount = 0;
        t.state++;
    } else if (t.state != 1) return;
    t.a148 -= 1;
    if (t.a148 <= 0) {
        if (env.onScreen != 0) env.missCount += 1;
        emit3(out, E3_TASK_CLOSE);
    }
}

// EnEtcTask::EnEtc_St4_Tama 0x9385D0
void etcSt4Tama(EtcTask& t, St4Fade& f, const Env3& env, Ev3s& out) {
    bool frozen = env.freeze > 0;
    int s = t.state;
    if (s == 0) {
        f.alpha = 0.0f; f.u24 = 0; f.u28 = 0;
        t.a148 = t.v50; t.a14C = t.v54;
        t.state = ++s;
    }
    if (s == 1) {
        if (!frozen) f.alpha = f.alpha + 1.2f * (0.9f / (float)t.a148);
        if (f.alpha < 0.9f) goto tail;
        f.alpha = 0.9f;
        t.state = ++s;
    }
    if (s == 2) {
        if (!frozen) f.alpha = f.alpha - 1.2f * (0.9f / (float)t.a14C);
        if (f.alpha <= 0.0f) { f.alpha = 0.0f; emit3(out, E3_TASK_CLOSE); }
    }
tail:
    if (!frozen) {
        f.u24 = trunc32((float)f.u24 + -153.6f);
        f.u28 = trunc32((float)f.u28 + 76.8f);
    }
}

// EnEtcTask::EnEtcST3_SC02_02FireBall 0x938230 (solo la maquina de estados; el dibujo y la sombra son del motor)
void etcFireBall(EtcTask& t, PathFn path, void* ctx, const Env3& env, Ev3s& out) {
    if (env.freeze <= 0) {
        int s = t.state;
        if (s == 0) {
            t.a148 = 0x260057; t.a14C = 0x260057; t.a150 = 0x260066;
            emit3(out, E3_SOUND, 0x17010C);
            t.a154 = 0; t.a158 = 0; t.f170 = 1.0f;
            t.state = ++s;
        }
        if (s == 1) {
            if (t.unk14 == 6 || t.unk14 == 7) t.pos = path(t.unk14, t.a158, ctx);   // func_1CC710(frame, camino, 0x229 / 0x22A)
            t.a158 += 1;
            if (t.a158 >= 0x2D) {
                t.flags |= F_RESPONDED;
                t.a148 = 0x260040; t.a14C = 0x260040; t.a150 = 0x260056;
                t.a154 = t.a150 - t.a14C;
                t.state = ++s;
            } else s = -1;   // break: no cae en el caso 2
        }
        if (s == 2) {
            t.a154 -= 1;
            if (t.a154 <= 0) emit3(out, E3_TASK_CLOSE);
        }
    }
    if (env.freeze <= 0) {
        t.a14C += 1;
        if (t.a14C >= t.a150) t.a14C = t.a148;
    }
}

// EnEtcTask::EnEtc_BranchMark 0x938860
void etcBranchMark(EtcTask& t, BranchEnv& e, Ev3s& out) {
    bool ok = false;
    bool a = e.gameState == 0xA;
    bool kind1 = a && e.gameKind == 1;
    bool f1 = kind1 && (e.flagsTable[t.p14] & 1);
    bool f2 = f1 && (e.flagsTable[t.p14] & 2);
    if (f2 && !(e.flagsTable[t.p14] & 0x20)) ok = true;
    if (!ok) { e.d500F05 = 0; e.d73CC = 0; emit3(out, E3_TASK_CLOSE); }
    if (t.w16C == 0) {
        if (e.camPath == t.a14C && !(e.camFrame < (float)t.a150)) {
            emit3(out, E3_CALL, 0xA10B00, (int8_t)t.p14);   // func_A10B00(id)
            t.w16C = 1;
        }
    }
    if (e.camPath == t.a154 && !(e.camFrame < (float)t.a158)) {
        e.d500F05 = 0; e.d73CC = 0; emit3(out, E3_TASK_CLOSE);
    }
    if (t.flags & F_HIT) {
        t.flags &= ~(uint32_t)F_HIT;
        t.a148 = 0x10088;
        e.d500F05 = 1; e.d73CC = 1;
        if (t.p14 == 0x10) e.d500E11 = 1;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// EnEtcAttach::Ctrl_Pre 0x93CA70
void attachCtrlPre(AttachObj& o, Env3& env, float groundY, Ev3s& out) {
    env.groundHeight = groundY;                       // gp-0x760C
    float g1 = groundY + 1.0f;
    for (int i = 0; i < 3; i++) o.rot[i] += o.rotVel[i];
    o.vel.x += o.acc.x; o.vel.y += o.acc.y; o.vel.z += o.acc.z;
    if (o.pos.y + o.vel.y < g1) {
        o.pos.y = g1;
        o.vel.x *= 0.7f; o.vel.y *= -0.3f; o.vel.z *= 0.7f;
        for (int i = 0; i < 3; i++) o.rotVel[i] = trunc32(0.5f * (float)o.rotVel[i]);
        if (!(o.flags & F_RESPONDED)) { emit3(out, E3_SNDREQ, 0x2001B, -1); o.flags |= F_RESPONDED; }
    }
    o.pos.x += o.vel.x; o.pos.y += o.vel.y; o.pos.z += o.vel.z;
    o.timer -= 1;
    if (o.timer <= 0) {
        o.timer = 0;
        for (int i = 0; i < 3; i++) { o.col[i] -= o.colStep[i]; if (o.col[i] < 0.0f) o.col[i] = 0.0f; }
    }
    if (o.flags & F_RESPONDED) {
        o.alpha -= 0.04f;
        if (o.alpha <= 0.0f) { o.flags |= F_DEAD; emit3(out, E3_TASK_CLOSE); }
    }
}

// EnDamSphere::Ctrl_Pre 0x942A20 (la posicion en mundo la calcula el motor con las matrices de la parte y de la camara)
void damSphereCtrlPre(DamSphere& s, DamOwner& owner, Ev3s& out) {
    if (owner.flags & F_DEAD) emit3(out, E3_TASK_CLOSE);
    if (!(s.flags & F_NOCOLLI) && (s.flags & F_HIT)) {
        if (!(owner.flags & 0x100)) {                 // duenyo no invulnerable: le pasa el impacto
            owner.flags |= F_HIT;
            if (s.flags & F_PLAYER0) { owner.flags2 &= ~(uint32_t)F_GATE; owner.flags |= F_PLAYER0; }
            else { owner.flags2 |= F_GATE; owner.flags |= F_PLAYER1; }
            owner.flags2 |= F_RESPONDED;
        }
        s.flags &= ~(uint32_t)(F_HIT | F_PLAYER0 | F_PLAYER1);
        s.flags |= F_NOCOLLI;
        if (owner.flags & 0x100) s.flags &= ~(uint32_t)F_NOCOLLI;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// WeakTask::Ctrl_Pre 0x9392A0
void weakCtrlPre(WeakTask& t, WeakOwner& owner, const Env3& env, Ev3s& out) {
    float f20;
    if (t.partIdx != 0) {
        const WeakPart& part = owner.part[t.partIdx];
        f20 = part.f_D8;
        if ((owner.flags & F_DEAD) || (part.flags120 & F_HITTED)) {
            if (owner.weakSlot[t.slot] == t.id) owner.weakSlot[t.slot] = 0;
            emit3(out, E3_TASK_CLOSE);
        }
    } else {
        f20 = t.viewZ;
        if (owner.flags & F_DEAD) {
            if (owner.weakSlot[t.slot] == t.id) owner.weakSlot[t.slot] = 0;
            emit3(out, E3_TASK_CLOSE);
        }
    }
    if (t.flags & F_HIT) {
        t.flags &= ~(uint32_t)F_HIT;
        t.flags |= F_RESPONDED;
        owner.flags |= F_HIT;
    }
    uint32_t st = t.state;
    if (st == 0) {
        if ((t.w & 8) && !(t.w & 0x80)) {
            t.counter = (int16_t)(t.counter + 1);
            if (t.counter > 0) {
                t.result = (int8_t)(((t.w & 2) ? 1 : 0) + ((t.w & 4) ? 2 : 0));   // jugador que lo golpeo (bit 0 = j1, bit 1 = j2)
                t.counter = 0;
                t.state = st = 1;
            }
        } else {
            t.counter = (int16_t)(t.counter - 1);
            if (t.counter < 0) t.counter = 0;
        }
    }
    if (st == 1) {                                    // en el original: cae desde el caso 0 o entra directo
        t.counter = (int16_t)(t.counter + 1);
        if (t.counter >= 0x97) { t.counter = 0; t.state = 2; }
    }
    t.w |= 0x10;
    if ((t.w & 0x20) || !(f20 <= 0.0f) || env.mode7218 != 2) t.w &= ~0x10u;
    t.w &= ~8u; t.w &= ~2u; t.w &= ~4u;
    if (t.w & 0x10) {
        t.animCur += 1;
        if (t.animCur >= t.animEnd) t.animCur = t.animStart;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// ArmorTask::ComeOff 0x957040
void armorComeOff(ArmorTask& a, ArmorPart& part, Ev3s& out) {
    emit3(out, E3_NEW_ATTACH, a.armorId);              // new_EnEtcAttach(matriz de la parte, +0x150)
    part.cnt168 = (int16_t)(part.cnt168 + 1);
    emit3(out, E3_TASK_CLOSE);
}
// ArmorTask::PreDisp 0x956F00
void armorPreDisp(ArmorTask& a, ArmorPart& part, Ev3s& out) { if (!(part.flag154 & 0x100)) armorComeOff(a, part, out); }
// ArmorTask::NotDisp 0x956FC0
void armorNotDisp(ArmorTask& a, ArmorPart& part, int32_t tableValue, Ev3s& out) {
    if (!(part.flag154 & 0x100)) { part.a0 = tableValue; armorComeOff(a, part, out); }
}

// ---------------------------------------------------------------------------------------------------------------------
// PetitZombieTask::Ctrl 0x951C40 (solo reglas)
void petitCtrl(PetitZombie& z, const PetitArea& a, const Env3& env, Ev3s& out) {
    if (env.freeze <= 0) {
        if (z.flags & F_HIT) {
            z.flags &= ~1u;
            emit3(out, E3_MOTION, 0x43, 0);
            emit3(out, E3_CALL, C3_PETIT_HIT);
        } else if (env.cam->pathId == a.pathId && !(env.cam->frame < (float)a.frame)) {
            emit3(out, E3_CALL, C3_PETIT_CAM);
        }
        if (z.mode == 1) z.ang += (z.p18 != 0) ? 0x40 : -0x40;
        if (z.mode == 2) {
            bool hit = false;
            if (!(z.x < a.xMax)) {
                z.x = a.xMax; z.ang += (z.ang >= 0xC000) ? 0x100 : -0x100; hit = true;
            } else if (z.x <= a.xMin) {
                z.x = a.xMin; z.ang += (z.ang >= 0x4000) ? 0x100 : -0x100; hit = true;
            }
            if (!(z.z < a.zMax)) {
                z.z = a.zMax;
                if (!hit) z.ang += (z.ang >= 0x8000) ? 0x100 : -0x100;
            } else if (z.z <= a.zMin) {
                z.z = a.zMin;
                if (!hit) z.ang += (z.ang >= 0x8000) ? -0x100 : 0x100;
            }
        }
    }
    if (env.freeze <= 0) { z.f3C += 1.0f; z.flags &= ~(uint32_t)F_HIT; }
}

// ---------------------------------------------------------------------------------------------------------------------
// EnEtcBone::Ctrl_Pre 0x93BF40
void boneCtrlPre(BoneObj& b, bool ownerDead, BoneInput& in, Env3& env, Ev3s& out) {
    (void)ownerDead;
    float g = b.ground;
    env.groundHeight = g;
    if (env.freeze > 0) return;
    int s = b.state;
    if (s == 0) {
        if (in.partDead && in.ownerBit400) { b.state = ++s; }
        else return;
    }
    if (s == 1) {
        // sp44 en el estado 0->1 del mismo fotograma es el valor residual de la pila en el original (el motor lo pasa en partY)
        if (!((g + b.partHeight) < in.partY)) { b.state++; return; }
        return;
    }
    if (s == 2) {
        for (int i = 0; i < 3; i++) b.rot[i] = in.angles[i];           // func_1C9290
        b.rotVel[0] = (env.rand() & 7) * 0x10;
        int sign = 1 - ((env.rand() % 2) * 2);
        b.rotVel[1] = sign * ((env.rand() & 7) * 0x10);
        b.rotVel[2] = 0;
        b.pos = in.p50;
        float k = env.soundOn ? 0.75f : 0.5f;
        b.vel.x = k * (in.p50.x - in.p60.x);
        b.vel.y = k * (in.p50.y - in.p60.y);
        b.vel.z = k * (in.p50.z - in.p60.z);
        b.acc.x = 0.0f; b.acc.y = -0.0392f; b.acc.z = 0.0f;
        b.flags |= 0x200;
        emit3(out, E3_PART_FLAGS, 0x200);                               // part.flags120 |= 0x200 (la parte queda 'suelta')
        b.state = ++s;
    }
    if (s == 3) {
        for (int i = 0; i < 3; i++) b.rot[i] += b.rotVel[i];
        b.vel.x += b.acc.x; b.vel.y += b.acc.y; b.vel.z += b.acc.z;
        if (b.pos.y + b.vel.y <= g) {
            b.pos.y = g;
            if (env.soundOn) { b.vel.x *= 0.5f; b.vel.y *= -0.4f; b.vel.z *= 0.5f; }
            else             { b.vel.x *= 0.2f; b.vel.y *= -0.1f; b.vel.z *= 0.2f; }
            if (!(b.flags & F_RESPONDED)) { emit3(out, E3_SNDREQ, 0x2001B, -1); b.flags |= F_RESPONDED; }
        }
        b.pos.x += b.vel.x; b.pos.y += b.vel.y; b.pos.z += b.vel.z;
        if (b.flags & F_RESPONDED) {
            b.alpha -= 0.04f;
            if (b.alpha <= 0.0f) { b.flags |= F_DEAD; emit3(out, E3_TASK_CLOSE); }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// EnVtxEffCtrl. D_00985430 (GAMEMOD.BIN): claves de color por fraccion de vida.
const float kVtxColorKey[5][4] = {
    {0.07f, 1.0f, 0.8924f, 0.462f}, {0.185f, 1.0f, 0.4219f, 0.246f}, {0.36f, 0.196f, 0.1296f, 0.1123f}, {1.0f, 0.0f, 0.0f, 0.0f},
    {-1.0f, -1.0f, -1.0f, -1.0f}};

// EnVtxEffReqBufInit 0x93BEF0
void vtxReqBufInit(int32_t* kinds, int n) { for (int i = 0; i < n; i++) kinds[i] = 0; }

// EnVtxEffCtrl::Ctrl_Fire 0x93BA50
void vtxCtrlFire(VtxEff& p, VtxCtrl& c, const Env3& env) {
    if (env.freeze <= 0) {
        p.fireFrame += 1;
        if ((uint32_t)p.fireMax < (uint32_t)p.fireFrame || c.active >= 0x18F) { p.kind = 0; c.active -= 1; }
    }
}

// EnVtxEffCtrl::Ctrl_Prtc 0x93B280 (reglas; el dibujo es del motor)
void vtxCtrlPrtc(VtxEff& p, VtxCtrl& c, const Env3& env, uint32_t gp733C, uint32_t gp7380) {
    if ((float)p.life <= 25.0f) {
        if (env.freeze <= 0) p.scale = p.scale - p.scaleStep;
    } else if ((float)p.life == 26.0f) {
        p.scale = 1.0f; p.scaleStep = p.scale / 25.0f;
    }
    if (env.freeze > 0) return;
    if (p.landed == 0) {
        p.vel.x += p.acc.x; p.vel.y += p.acc.y; p.vel.z += p.acc.z;
        if ((p.pos.y + p.vel.y) <= (env.ground - 0.2f)) {
            p.pos.y = env.ground;
            p.vel = V3();
            p.landed = 1;
        }
        p.pos.x += p.vel.x; p.pos.y += p.vel.y; p.pos.z += p.vel.z;
    }
    for (int i = 0; i < 3; i++) { p.col[i] += p.colStep[i]; if (p.col[i] < 0.0f) p.col[i] = 0.0f; }
    if (gp733C < gp7380 || (p.life -= 1, p.life <= 0) || (c.active >= 0x18F && p.life < 0x3C)) {
        p.kind = 0; c.active -= 1;
    }
    if (p.stage < 4 && (float)p.life <= p.stageLife) {
        p.stage += 1;
        const float* k = kVtxColorKey[p.stage];       // OJO: el original indexa D_985430 + stage*0x10 con stage ya incrementado
        p.stageLife = 120.0f - 120.0f * k[0];
        float dt = (float)p.life - p.stageLife;
        p.colStep[0] = (k[1] - p.col[0]) / dt;
        p.colStep[1] = (k[2] - p.col[1]) / dt;
        p.colStep[2] = (k[3] - p.col[2]) / dt;
    }
}

}}  // namespace vn::en3

#include "enemies_rest.h"
#include <cmath>
namespace vn { namespace en { namespace rest {

namespace {
inline int8_t s8(int v) { return static_cast<int8_t>(v); }   // dsll32 24 / dsra32 24 del original
inline int slotOf(int player) { return player & 1; }
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::changeDamageMot (0x954000)
void RestLogic::changeDamageMot(VampireState& v, EventList& ev, int part, int weaponClass, const DamageMotIn& in) {
    if (v.flags & 0x10002000u) return;                                   // VF_BIT2000 | VF_BIT10000000: no cambia la animacion
    int a2, motId, thr;
    if (in.motTypeFlag == 0x12) {                                        // tabla D_009A23D0 -> D_009B5D50 por clase de parte (D_009ACB10)
        int cls = (part >= 0 && part < 18) ? kPeoplePartClass[part] : 0;
        motId = kDamageMotByClass[cls & 15];
        a2 = 1; thr = 11;
    } else {                                                             // registros sin inicializar en el original: se inyectan
        motId = in.staleA3; a2 = in.staleA2; thr = in.staleThreshold;
    }
    if (part < thr) {                                                    // partes bajas: arranque de la capa de dano
        if (v.flags & VF_DAMAGE_MOT) return;
        v.flags |= VF_DAMAGE_MOT;
        v.stateFlags |= VS_DAMMOT_KIND;
        emit(ev, EV_SCRIPT, RE_DAMAGE_LAYER, a2, motId);                 // func_1FAA60(.., 5, 5, 8)
        return;
    }
    if ((v.flags & VF_DAMAGE_MOT) && !(v.stateFlags & VS_DAMMOT_KIND)) return;
    if (weaponClass == 2) return;                                        // gp-0x7348[jugador] == 2: no
    v.flags |= VF_DAMAGE_MOT;
    emit(ev, EV_MOTION, motId, 10, 1);                                   // func_1FA640(.., motId, 10, 10)
    v.motActive = 1;                                                     // +0x1DA0
    v.stateFlags &= ~static_cast<uint32_t>(VS_DAMMOT_KIND);
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::checkDamage_Eknd (0x953870)
void RestLogic::checkDamageEknd(VampireState& v, EnemyEnv& e, const HitInput& h, const EkndIn& in, EkndOut& out, EventList& ev) {
    int s0 = 0;                                                          // contador de partes marcadas y despues el sonido (s0 del original)
    if (v.flags & VF_HITCHECK) {                                         // (CheckHitParts lo hace el motor y rellena HitInput)
        for (int slot = 0; slot < 2; slot++) {
            int pl = h.shooter[slot];
            if (pl == -1) continue;
            int part = s8(h.partOfPlayer[slotOf(pl)]);                   // lb 0x70(*(this)+pl)
            e.lastHitPlayer = pl;                                        // sb -0x6FD0(gp), en el delay slot del blez
            if (part <= 0) continue;
            if (v.flags & VF_DEAD) { emit(ev, EV_HITNUM, pl); continue; }   // .L00953D04
            if (!(v.flags & VF_INVULN)) {
                int8_t list[kVampParts] = {};
                s0 = 0;
                for (int a1 = 1; s8(a1) < in.armorCount && a1 < kVampParts; a1++)   // partes con armadura (flag154 bit 0x100)
                    if (v.parts[a1].flag154 & 0x100) list[s0++] = static_cast<int8_t>(a1);
                VampireLogic::decLife(v, e, 0x2D);                       // 45 de dano base
                int shownPart = part % kVampParts;
                if (v.life > 0) {
                    // Eleccion aleatoria (muestreo sin reposicion) de s1 = 1 parte marcada cuyo flag se limpia. Recorre de la ultima a la primera.
                    int n = s8(s0);
                    if (n > 0) {
                        int s1 = 1, idx = n;
                        while (s0 != 0 || s1 != 0) {
                            int s3 = s8(s0), s5 = s8(s1);
                            bool clear;
                            if (s3 == s5) clear = true;
                            else {
                                if (s3 == 0) break;                      // el original ejecuta break 7 (division por cero); no ocurre por construccion
                                int r = s8(e.rand() % s3);
                                clear = r < s5;
                            }
                            if (clear) { s1 = s8(s1 - 1); v.parts[list[idx - 1]].flag154 &= ~0x100u; }
                            s0 = s8(s0 - 1); idx--;
                        }
                    }
                    out.spark = true;                                    // chispa de armadura en la posicion de la parte (motor: new_EnEff)
                    out.sparkPos[0] = in.body[shownPart][0];
                    out.sparkPos[1] = in.body[shownPart][1];
                    out.sparkPos[2] = in.body[shownPart][2] + in.body154[shownPart] * in.body1CA0;
                    emit(ev, EV_SCRIPT, RE_SPARK, part);
                    s0 = 0x20025;
                    emit(ev, EV_HITMARK, part, pl, 0x3E8);
                } else {
                    for (int k = s8(s0); k > 0; k--) v.parts[list[k - 1]].flag154 &= ~0x100u;   // todas las armaduras marcadas se limpian
                    v.flags |= VF_DEAD;
                    if (!(v.parts[1].flag150 & 0x01000000))              // 0x240 = parte 1 + 0x150: aun no desprendida
                        emit(ev, EV_PARTS_CHAIN, 1, pl, kEkndChainParam[v.type % kVampTypes][0], kEkndChainParam[v.type % kVampTypes][1]);
                    v.stateFlags |= VS_HIT;
                    emit(ev, EV_SCORE, pl, 100);
                    s0 = 0x180011;
                    emit(ev, EV_HITMARK, part, pl, 0x3E9);
                }
                if (e.gameState == 0xA && e.gameKind == 1 && e.pl[slotOf(pl)].weapon == 0xE) {   // trazadora (modo entrenamiento)
                    out.tracer = true;
                    out.tracerLocal[0] = in.partLocal[shownPart][0];
                    out.tracerLocal[1] = in.partLocal[shownPart][1];
                    out.tracerLocal[2] = in.partLocal[shownPart][2] + 1.0f;
                    out.tracerScale = 0.8f;                              // 0x3F4CCCCD
                    emit(ev, EV_EFFECT_TRACER, part, pl);
                }
            }
            emit(ev, EV_SCORE, pl, 10);                                  // tambien con invulnerabilidad (.L00953CF4)
            emit(ev, EV_HITNUM, pl);
        }
    }
    if (s0 != 0) emit(ev, EV_SOUND, s0, 0);                              // func_217C30(D_515F30, s0, 0, 0, 0x40)
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::Wait (0x955A40)
int RestLogic::pickWaitMot(const VampireState& v, const EnemyEnv& e) {
    int idx = kWaitMotBase[v.type % kVampTypes] + (v.subType & 0xFF);   // D_009A22C0[tipo] + subTipo*4 (en el ELF 0x2D8860)
    int r = e.rand();
    int sel = r % 2;                                                     // div con signo del original; nlRand >= 0 (sel = -1 no esta soportado)
    if (idx >= 25 || sel < 0) return 0;                                  // fuera de la tabla extraida: el original leeria otra memoria
    return kWaitMotPairs[idx][sel];
}

void RestLogic::wait(VampireState& v, const EnemyEnv& e, EventList& ev, const int16_t* motLen) {
    if (v.step == 0) {
        v.motId = pickWaitMot(v, e);                                     // +0x1DA8
        v.aiTimer = static_cast<float>(e.rand() % 50);                   // +0x1D84
        v.step = 1;
    } else if (v.step != 1) {
        return;
    }
    if (!(v.flags & VF_DAMAGE_MOT) && v.curMot != v.motId) {             // cambia la animacion de espera
        v.motId = pickWaitMot(v, e);
        int s2 = v.motId;
        int r = e.rand();
        int len = motLen ? motLen[s2] : 0;
        emit(ev, EV_MOTION, s2, 10, 0, len != 0 ? r % len : 0);         // func_1FA640(.., s2, rand % motLen[s2], 10)
    }
    if (!(v.aiTimer <= 100.0f) && v.type == 8) {                         // solo el tipo 8 pasa a atacar (aiMode 2) si hay hueco
        if (e.attackSlot[0] == 0 || e.attackSlot[1] == 0) { v.aiMode = 2; v.step = 0; v.aiTimer = 0; }
    }
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::turnTargetSub (0x954AA0) / turnTarget (0x954BD0)
int RestLogic::turnClamp(int rotY, int angle, int maxStep) {
    int a = static_cast<int16_t>(angle);
    if (maxStep < a) return rotY + maxStep;
    if (a < -maxStep) return rotY - maxStep;
    return rotY + a;
}

float RestLogic::turnSideOffsetX(int atkPlayer) { return atkPlayer == 1 ? 3.5f : (atkPlayer == 0 ? -3.5f : 0.0f); }

void RestLogic::turnTarget(VampireState& v, VampExt& x, const TurnIn& in) {
    int mode = v.motLoop;                                                // +0x1DAC (en vampiros es el selector de objetivo, no un contador)
    if (mode == -1) {
        const float* t = (v.atkPlayer == -1 || (in.coop == 1 && in.gp7374 == 1)) ? in.camPos : in.sideTarget;
        x.target[0] = t[0]; x.target[1] = t[1]; x.target[2] = t[2];
    } else if (mode == -2 || mode == -3) {                               // reflejo respecto a la posicion propia; y = altura de la camara
        x.target[0] = 2.0f * v.posX - in.mirrorSrc[0];
        x.target[1] = in.camPos[1];
        x.target[2] = 2.0f * v.posZ - in.mirrorSrc[2];
    } else if (mode == -4) {                                             // reflejo de la marca (+0x1DC4)
        x.target[0] = 2.0f * v.posX - x.mark1DC4[0];
        x.target[1] = x.mark1DC4[1];
        x.target[2] = 2.0f * v.posZ - x.mark1DC4[2];
    } else {
        x.target[0] = in.explicitTarget[0]; x.target[1] = in.explicitTarget[1]; x.target[2] = in.explicitTarget[2];
    }
    if (v.curMot != 0x314 && v.curMot != 0x313)                          // los saltos no giran
        x.rotY = turnClamp(x.rotY, in.angleToTarget, x.turnStep);
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::FirstCtrl (0x9526D0)
bool RestLogic::firstCtrl(VampireState& v, VampExt& x, EnemyEnv& e, const TurnIn& turn, EventList& ev) {
    bool close = false;
    if (x.closeCountdown != 0) {
        x.closeCountdown--;
        if (x.closeCountdown == 0) close = true;
    } else if (e.cam.pathId == x.spawnPathId && !(e.cam.frame < static_cast<float>(x.spawnFrame))) {
        x.closeCountdown = 1;                                            // se cierra en el siguiente fotograma
    }
    if (close) {
        emit(ev, EV_SCRIPT, RE_PRE_CLOSE);
        if (v.atkPlayer != -1) e.attackSlot[slotOf(v.atkPlayer)] = 0;    // libera el hueco de ataque
        emit(ev, EV_SCRIPT, RE_TASK_CLOSE);
    }
    if (v.stateFlags & VS_AIRBORNE) {                                    // integracion de movimiento en el aire
        v.posX += v.velX; v.posY += v.velY; v.posZ += v.velZ;
        v.velX += x.accelX; v.velY += v.gravity; v.velZ += x.accelZ;
    }
    if (v.motLoop != 0) turnTarget(v, x, turn);
    return close;
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::LastCtrl (0x952B40)
void RestLogic::lastCtrl(VampireState& v, VampExt& x, EnemyEnv& e, const CtrlIn& in, CtrlOut& out, EventList& ev) {
    int mot0 = v.curMot;
    if (e.gameState == 0xA) emit(ev, EV_SCRIPT, RE_WAITPOS_BUFF);
    if (!(v.flags & VF_FINISHED)) {                                      // entrada en camara (el motor calcula el punto: matriz de camara * (0, 0.5, 1.5))
        float d2 = x.vec34C[0] * x.vec34C[0] + x.vec34C[1] * x.vec34C[1] + x.vec34C[2] * x.vec34C[2];
        bool big = (v.flags & VF_BIT10000000) != 0;
        out.camEntry = true;
        out.camEntryScale = (big ? 1000.0f : 100.0f) / d2;
        emit(ev, EV_SCRIPT, RE_CAM_ENTRY, big ? 1 : 0);
    }
    if (v.motActive > 0) {                                               // la animacion se pasa de largo: una repeticion menos
        if (!((v.curFrame + v.speedScale) <= static_cast<float>(in.motLen[v.curMot]))) v.motActive--;
    }
    VampireLogic::chkParam(v, e);
    if (v.paramKind == -1) VampireLogic::getParam(v, e, in.script, in.scriptSize, ev);
    if (v.motActive != 0 && v.curMot == mot0) {                          // avance de fotogramas
        v.frame3C += v.speedScale;
        v.layerEndFrame += v.speedScale;
    }
    if (x.closeCountdown == 1) v.flags2 |= 0x20;
}

// ---------------------------------------------------------------------------------------------------------------
// EnVampireTask::KneadEffect (0x952DA0)
bool RestLogic::kneadEffect(const VampireState& v, const KneadIn& in, KneadOut& out) {
    out = KneadOut();
    if (!(v.stateFlags & VS_DEADSND)) return false;                      // bit 0x10 de +0x1DA4
    if (v.velY < 0.0f) return false;
    float s;
    if (v.curMot == 0x314 || v.curMot == 0x313) s = 0.0f;
    else if (v.curMot == 0x310) s = 1.0f - (v.posY - in.height) / 7.0f;
    else {
        float m = 0;
        for (int i = 1; i < kVampParts; i++) {                           // punto mas bajo (en camara) de todas las partes
            float y = in.viewY[i] - in.radius[i];
            if (i == 1 || !(m <= y)) m = y;
        }
        s = 1.0f - (m - in.height) / 7.0f;
    }
    if (s <= 0.0f) return false;
    int id = 0x2600B8 + (static_cast<int>(v.frame3C) % 5);
    if (!(s <= 1.0f)) s = 1.0f;
    out.draw = true; out.effectId = id; out.scale = 0.7f * s;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// PeopleTask::FirstCtrl (0x205880)
bool RestLogic::peopleFirstCtrl(PeopleTaskState& p, PeopleExt& x, EnemyEnv& e, const PeopleFirstIn& in, EventList& ev) {
    bool close = false;
    if (p.p1DC8 != 0) {                                                  // cuenta atras de cierre
        p.p1DC8--;
        if (p.p1DC8 == 0) close = true;
    } else if (e.cam.pathId == x.spawnPathId && !(e.cam.frame < static_cast<float>(x.spawnFrame))) {
        close = true;                                                    // (a diferencia de EnVampire, aqui cierra ya)
    } else if ((p.stateFlags & 0x100) && in.offScreen) {
        close = true;                                                    // fuera de pantalla
    }
    if (close) {
        emit(ev, EV_SCRIPT, RE_CHILD_ALL_CLOSE);
        if (p.atkPlayer != -1) e.attackSlot[slotOf(p.atkPlayer)] = 0;
        emit(ev, EV_SCRIPT, RE_FUNC_9357A0);
        emit(ev, EV_SCRIPT, RE_TASK_CLOSE);
    }
    if (p.p1DD0 != 0) emit(ev, EV_SCRIPT, RE_TURN_TARGET);               // PeopleTask::turnTarget (pendiente)
    float tx, ty, tz;
    if (p.p1DD0 == -1) { tx = x.target[0]; ty = x.target[1]; tz = x.target[2]; }
    else { tx = in.camPos[0]; ty = in.camPos[1]; tz = in.camPos[2]; }
    float dx = p.posX - tx, dz = p.posZ - tz, dy = p.posY - ty;
    x.dist1DF8 = std::sqrt(dx * dx + dz * dz);                           // +0x1DF8
    x.dist1DFC = std::sqrt(dx * dx + dy * dy + dz * dz);                 // +0x1DFC
    return close;
}

// ---------------------------------------------------------------------------------------------------------------
// PeopleTask::LastCtrl (0x205D70)
void RestLogic::peopleLastCtrl(PeopleTaskState& p, PeopleExt& x, EnemyEnv& e, PeopleCounters& c, VmEnv& vm, const PeopleLastIn& in, CtrlOut& out, EventList& ev) {
    int mot0 = p.curMot;
    if (!(p.flags & PF_FINISHED) && in.gp7210 < c.stage6Alive) {         // entrada en camara solo si hay mas civiles de la fase 6 vivos que gp-0x7210
        float d2 = x.vec34C[0] * x.vec34C[0] + x.vec34C[1] * x.vec34C[1] + x.vec34C[2] * x.vec34C[2];
        bool big = (p.flags & PF_BIT10000000) != 0;
        out.camEntry = true;
        out.camEntryScale = (big ? 1000.0f : 100.0f) / d2;
        emit(ev, EV_SCRIPT, RE_CAM_ENTRY, big ? 1 : 0);
    }
    bool readNext = PeopleLogic::lastCtrlWait(p, e, c, in.wait, in.motLen);
    if (readNext) PeopleLogic::getParam(p, e, c, vm, in.script, in.scriptSize, ev);
    if (p.loop != 0 && p.curMot == mot0) {                               // avance de fotogramas
        p.frame3C += p.speed;
        x.layerEndFrame += p.speed;
    }
}

// ---------------------------------------------------------------------------------------------------------------
// ErsPplTask::FirstCtrl (0x20D100) / LastCtrl (0x20D230)
float RestLogic::ersFirstCtrl(float parentProgress, bool& closed, EventList& ev) {
    closed = (parentProgress == 0.0f);
    if (closed) emit(ev, EV_SCRIPT, RE_TASK_CLOSE);                      // el original sigue ejecutando y pone el alfa 0
    return parentProgress <= 0.5f ? 2.0f * parentProgress : 2.0f - 2.0f * parentProgress;   // triangulo: 0 -> 1 (en 0.5) -> 0
}

void RestLogic::ersLastCtrl(float step, float& frame3C, float& layerEndFrame) {
    frame3C += step;
    layerEndFrame += step;
}
}}}  // namespace vn::en::rest

// Ver bosses56_b6.h. Cada funcion cita la original (direccion en el overlay bs6mod).
#include "bosses56_b6.h"
#include <cstring>
namespace vn { namespace boss56 {

using boss6::Boss6Logic;
using boss6::Boss6State;
using boss6::Boss6Globals;

float b6BitsToFloat(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// ---------------------------------------------------------------- movimiento / rutas

// AddMotionFrame (0x9C5460): si el movimiento no esta detenido (+0x0C & 0x4000) avanza los dos canales (1E18 y 1E1C).
void B6Move::addMotionFrame(B6MoveState& b) {
    if (!(b.f0C & B6F0C_FROZEN)) {
        b.frame3C += b.rate;
        b.frame40 += b.rate1E1C;
    }
}

// AddPathFrame (0x9C5570): con ruta (0x80) avanza pathFrame con rate2; con acercamiento (0x2000) avanza apprFrame hasta apprTotal.
void B6Move::addPathFrame(B6MoveState& b) {
    if (b.f1DA4 & B6A_PATH) b.pathFrame += b.rate2;
    if (b.f1DA4 & B6A_APPROACH) {
        if (!(b.apprFrame < b.apprTotal)) {
            b.apprFrame = b.apprTotal;
            b.apprStep = 0.0f;
            return;
        }
        b.apprFrame += b.apprStep;
    }
}

// ApproachANPO (0x9C5700): con acercamiento (0x2000) interpola pose guardada -> destino con t = apprFrame/apprTotal;
// al llegar copia el destino y apaga el bit. Con ruta (0x80) o movimiento circular (0x01000000) copia el destino tal cual.
void B6Move::approachANPO(B6MoveState& b, const B6Anpo& tgt) {
    if (b.f1DA4 & B6A_APPROACH) {
        if (!(b.apprFrame < b.apprTotal)) {
            for (int i = 0; i < 3; i++) { b.pos[i] = tgt.pos[i]; b.rot[i] = tgt.rot[i]; }
            b.f1DA4 &= ~B6A_APPROACH;
            return;
        }
        float t = b.apprFrame / b.apprTotal;
        for (int i = 0; i < 3; i++) {
            b.pos[i] = b.apprPos[i] + t * (tgt.pos[i] - b.apprPos[i]);
            b.rot[i] = Boss6Logic::b6InterDivAngF(b.apprRot[i], tgt.rot[i], t);
        }
        return;
    }
    if ((b.f1DA4 & B6A_PATH) || (b.f1DA4 & B6A_CIRCLE)) {
        for (int i = 0; i < 3; i++) { b.pos[i] = tgt.pos[i]; b.rot[i] = tgt.rot[i]; }
    }
}

// GetPathParam(float, int, ANPO*) (0x9CA100): muestra de ruta y orientacion segun el tipo (1DAC):
// 0 = mira a la camara (lookAt), 1 = solo giro Y hacia la camara (salvo ruta relativa 0x800), 2 = sin giro y altura minima 8.
void B6Move::getPathParamPoint(B6MoveState& b, B6MoveEnv& env, float frame, int id, B6Anpo& out) {
    float cam[3];
    env.camPos(cam);
    env.pathEval(frame, id, out);
    switch (b.kind) {
    case 0:
        out.rot[2] = 0;
        env.lookAt(cam[0] - out.pos[0], cam[1] - out.pos[1], cam[2] - out.pos[2], out.rot[0], out.rot[1]);
        break;
    case 1:
        out.rot[2] = 0; out.rot[0] = 0;
        if (!(b.f1DA4 & B6A_PATH_800)) out.rot[1] = env.atan2Ang(cam[0] - out.pos[0], cam[2] - out.pos[2]);
        break;
    case 2:
        out.rot[2] = 0; out.rot[0] = 0; out.rot[1] = 0;
        if (out.pos[1] < 8.0f) out.pos[1] = 8.0f;
        break;
    default: break;
    }
}

// GetPathParamCT (0x9CA250): ruta de camara; el motor da la muestra y un punto de mirada, la rotacion apunta a ese punto.
void B6Move::getPathParamCT(B6MoveEnv& env, float frame, int id, B6Anpo& out) {
    float target[3];
    env.pathEvalCT(frame, id, out, target);
    out.rot[2] = 0;
    env.lookAt(target[0] - out.pos[0], target[1] - out.pos[1], target[2] - out.pos[2], out.rot[0], out.rot[1]);
}

// GetPathParam(ANPO*) (0x9C9DC0): acota pathFrame (fin: vuelta si 0x1000, si no marca 0x200 y se queda en el fin),
// muestrea la ruta (id -1 = el propio jefe sin giro; otros negativos no escriben), y aplica 0x800 (ruta relativa a la
// pose guardada, orientada a la camara) o 0x400 (giro en cuartos de vuelta -sel1DC4 << 14).
void B6Move::getPathParam(B6MoveState& b, B6MoveEnv& env, B6Anpo& out) {
    if ((int)b.pathFrame >= b.pathEnd) {
        if (b.f1DA4 & B6A_PATH_LOOP) {
            b.pathFrame -= (float)b.pathEnd;
        } else {
            b.f1DA4 |= B6A_PATH_DONE;
            b.pathFrame = (float)b.pathEnd;
        }
    } else {
        b.f1DA4 &= ~B6A_PATH_DONE;
    }
    int id = b.pathId;
    if (id < 0) {
        if (id == -1) {
            out.pos[0] = b.pos[0]; out.pos[1] = 0.0f; out.pos[2] = b.pos[2];
            out.rot[0] = 0; out.rot[1] = b.rot[1]; out.rot[2] = 0;
        }
    } else if (b.f1DA4 & B6A_CAMPATH) {
        getPathParamCT(env, b.pathFrame, id, out);
    } else {
        getPathParamPoint(b, env, b.pathFrame, id, out);
    }
    if (b.f1DA4 & B6A_PATH_800) {
        float cam[3];
        env.camPos(cam);
        env.transformSaved(b, out);
        out.rot[1] = env.atan2Ang(cam[0] - out.pos[0], cam[2] - out.pos[2]);
        return;
    }
    if (b.f1DA4 & B6A_PATH_400) {
        int ang = (int)((uint32_t)(-(int)b.sel1DC4) << 14);
        env.rotYawVec(ang, out.pos);
        out.rot[1] += ang;
    }
}

// TracePathParam (0x9C9D00): guarda la muestra en 1EAC; al llegar al fin (0x200) apaga los modos de ruta (0x80,0x100,0x400,0x800,0x1000).
void B6Move::tracePathParam(B6MoveState& b, B6MoveEnv& env) {
    B6Anpo o{};
    getPathParam(b, env, o);
    b.pathOut = o;
    if (b.f1DA4 & B6A_PATH_DONE)
        b.f1DA4 &= ~(B6A_PATH | B6A_CAMPATH | B6A_PATH_400 | B6A_PATH_800 | B6A_PATH_LOOP);
}

// UpdatePath (0x9C54A0): con ruta (0x80) o circulo (0x01000000) calcula la muestra (TracePathParam / CircleMove) y la copia a
// 1EE8..1EFC; despues ApproachANPO sobre 1EAC.
void B6Move::updatePath(B6MoveState& b, B6MoveEnv& env) {
    if (b.f1DA4 & B6A_PATH) {
        tracePathParam(b, env);
        b.lastPath = b.pathOut;
    } else if (b.f1DA4 & B6A_CIRCLE) {
        circleMove(b, env);
        b.lastPath = b.pathOut;
    }
    approachANPO(b, b.pathOut);
}

// SetCircleMove (0x9CACF0): prepara un giro circular alrededor del origen desde la posicion actual hasta la pose
// kPoseB[sel1DC4][sel1DC5] (sel1DC4 += dir, modulo 4); da 3 vueltas extra en el sentido de `dir`; 240 fotogramas.
void B6Move::setCircleMove(B6MoveState& b, B6MoveEnv& env, int dir) {
    b.circY0 = b.pos[1];
    b.circAng0 = (-env.atan2Ang(b.pos[0], -b.pos[2])) & 0xFFFF;
    b.circR0 = env.sqrtf_(b.pos[0] * b.pos[0] + b.pos[2] * b.pos[2]);
    b.sel1DC4 = (int8_t)((b.sel1DC4 + dir + 4) % 4);
    const boss6::Pose& p = boss6::kPoseB_flat[b.sel1DC4 * 4 + b.sel1DC5];
    b.circY1 = p.p[1];
    b.circAng1 = (int32_t)((uint32_t)p.r[1] & 0xFFFF);
    b.circR1 = env.sqrtf_(p.p[0] * p.p[0] + p.p[2] * p.p[2]);
    b.circDY = b.circY1 - b.circY0;
    b.circDR = b.circR1 - b.circR0;
    int v = (b.circAng1 - b.circAng0) & 0xFFFF;
    if (v >= 0x8000) v = (v - 0x8000) - 0x8000;
    b.circAngDelta = (dir > 0) ? v - 0x30000 : v + 0x30000;
    b.f1DA4 &= ~B6A_APPROACH;
    b.f1DA4 |= B6A_CIRCLE;
    b.circCnt = 0.0f;
    b.circTotal = b6BitsToFloat(0x43700000u);   // 240.0
    env.spawnCircleCamera(dir);
}

// CircleMove (0x9CAEF0): un paso del giro; t = cnt/total; radio, altura y angulo se interpolan linealmente; al acabar
// (cnt >= total) fija el destino y apaga 0x01000000. El resultado va a 1EAC (posicion girada, rot = (0, ang, 0)).
void B6Move::circleMove(B6MoveState& b, B6MoveEnv& env) {
    float t = b.circCnt / b.circTotal;
    float da = (float)b.circAngDelta * t;
    float dy = b.circDY * t;
    float r = b.circR0 + b.circDR * t;
    int ang = b.circAng0 + (int)da;
    b.circCnt = b.circCnt + 1.0f;
    float y = b.circY0 + dy;
    if (!(b.circCnt < b.circTotal)) {
        ang = b.circAng1;
        y = b.circY1;
        r = b.circR1;
        b.f1DA4 &= ~B6A_CIRCLE;
    }
    env.rotCircle(ang, r, y, b.pathOut.pos);
    b.pathOut.rot[0] = 0; b.pathOut.rot[1] = ang; b.pathOut.rot[2] = 0;
}

// EffectCtrl (0x9CB100): tipo 1 con circulo activo: cada 2 ticks crea una imagen residual.
void B6Move::effectCtrl(B6MoveState& b, B6MoveEnv& env) {
    if (b.kind == 1 && (b.f1DA4 & B6A_CIRCLE) && (b.effectTick % 2) == 0) env.spawnAfterImage();
}

// CtrlPre (0x9C4F30): guarda la pose, y por fotograma: CheckDamageEnd, despacho por estado (1E60), UpdatePath, CheckDamage,
// ControlAttack; tipos 1 y 2: alfa del cuerpo; con el flag 0x100000 la barra de vida.
// (El original copia una vez 4 filas estaticas D_009D2E00.. a D_26B4B0.. -parametros del motor- y no se reescribe.)
void B6Move::ctrlPre(B6MoveState& b, B6StepEnv& st) {
    for (int i = 0; i < 3; i++) {
        b.savedF[i] = b.pos[i]; b.savedF[3 + i] = b.obj0C[i]; b.savedF[6 + i] = b.objScale[i];
        b.savedI[i] = b.rot[i];
    }
    st.step(B6S_CHECK_DAMAGE_END);
    st.step(B6S_DISPATCH_STATE);
    st.step(B6S_UPDATE_PATH);
    st.step(B6S_CHECK_DAMAGE);
    st.step(B6S_CONTROL_ATTACK);
    if (b.kind == 1 || b.kind == 2) {
        st.step(B6S_BODY_ALPHA);
        st.step(B6S_SET_ALPHA);
    }
    if (b.f1DA4 & B6A_LIFEBAR) st.step(B6S_LIFE_DISPLAY);
}

// ---------------------------------------------------------------- murcielagos

// SetApproach (0x9CD480): arranca el acercamiento al destino (1E4..) con paso `step`.
void B6Bat::setApproach(B6BatState& s, float step) {
    s.apprT = 0.0f;
    s.apprStep = step;
    s.flags |= BATF_APPROACH;
}

// Approach (0x9CD4A0): con acercamiento: pos += t*(dest-pos), t += paso (al pasar de 1 se queda en 1 y se apaga); si no, pos = dest.
void B6Bat::approach(B6BatState& s) {
    if (s.flags & BATF_APPROACH) {
        for (int i = 0; i < 3; i++) s.pos[i] = s.pos[i] + s.apprT * (s.dest[i] - s.pos[i]);
        s.apprT = s.apprT + s.apprStep;
        if (!(s.apprT <= 1.0f)) {
            s.apprT = 1.0f;
            s.apprStep = 0.0f;
            s.flags &= (int16_t)~BATF_APPROACH;
        }
    } else {
        for (int i = 0; i < 3; i++) s.pos[i] = s.dest[i];
    }
}

// SetFallParam (0x9CD570): velocidad de caida aleatoria (x, z en [-1.5, 1.5], y en [2, 3]); apaga acercamiento y activo.
void B6Bat::setFallParam(B6BatState& s, B6BatEnv& env) {
    s.fall[0] = 3.0f * env.randFloat() - 1.5f;
    s.fall[1] = 2.0f + env.randFloat();
    s.fall[2] = 3.0f * env.randFloat() - 1.5f;
    s.flags &= (int16_t)~BATF_APPROACH;
    s.flags &= (int16_t)~BATF_ACTIVE;
}

// FallDown (0x9CD620): gravedad (1B0 += -0.0816...) y el destino se desplaza con la velocidad de caida.
void B6Bat::fallDown(B6BatState& s) {
    s.fall[1] = s.fall[1] + b6BitsToFloat(0xBDA740DAu);
    for (int i = 0; i < 3; i++) s.dest[i] += s.fall[i];
}

// ChgDead (0x9CD660): modo 5 (muerto; ExecMode borrara la tarea).
void B6Bat::chgDead(B6BatState& s) { s.mode = BAT_DEAD; }

// SetPathParam (0x9CCB90): elige ruta (kBatPathIds[rand % 17]); paso = 1.2 / (velocidadBase * kBatSpeedA[dificultad del ataque]).
void B6Bat::setPathParam(B6BatState& s, const Boss6State& o, B6BatEnv& env) {
    s.pathId = kBatPathIds[(uint32_t)env.rand() % 17u];
    s.pathT = 0.0f;
    float k = o.speedBase * b6BitsToFloat(kBatSpeedA_bits[o.atkDiff]);
    s.pathStep = b6BitsToFloat(0x3F99999Au) * (1.0f / k);
    s.pathLen = (float)env.pathLen(s.pathId);
    s.flags |= BATF_PATH;
}

// TraceShotPath (0x9CCC50): recorre la ruta; al agotarla, muerto. La Z de la muestra se escala por shotRatio y se gira al destino.
void B6Bat::traceShotPath(B6BatState& s, B6BatEnv& env) {
    if (!(s.pathT < s.pathLen)) { chgDead(s); return; }
    env.pathEval(s.pathT, s.pathId, s.sample);
    s.sample[2] = s.sample[2] * s.shotRatio;
    env.rotMatrixAdd(s.sample, s.dest);
    s.pathT = s.pathT + s.pathStep;
}

// SetSpiralParam (0x9CCEC0): copia objetivo y modo del jefe; recorrido de espiral desde 0 hasta spiralLen + indice.
void B6Bat::setSpiralParam(B6BatState& s, const B6BatGlobals& g, const Boss6State& o) {
    s.gameMode = o.gameMode;
    s.target = o.target;
    s.flags |= BATF_SPIRAL;
    s.pathT = 0.0f;
    s.pathLen = g.spiralLen + (float)s.index;
    s.flags |= BATF_SPIRAL;
}

// TraceSpiralPath (0x9CCF20): mientras spiralStart < pathLen traza la espiral: radio 0.15 + 0.85*(restante/spiralLen),
// angulo (int(spiralStart) - indice) << 10; Z = -pendiente * restante.
void B6Bat::traceSpiralPath(B6BatState& s, B6BatGlobals& g, B6BatEnv& env) {
    float len = s.pathLen, start = g.spiralStart;
    if (!(start < len)) { chgDead(s); return; }
    float rad = b6BitsToFloat(0x3E19999Au) + b6BitsToFloat(0x3F59999Au) * ((len - start) / g.spiralLen);
    int a = ((int)start - s.index) << 10;
    s.sample[0] = 35.0f * (rad * env.sinAng(a));
    s.sample[1] = 35.0f * (rad * env.cosAng(a));
    s.sample[2] = -g.spiralSlope * (s.pathLen - g.spiralStart);
    env.rotMatrixAdd(s.sample, s.dest);
    s.pathT = s.pathT + 1.0f;
}

// RotInit (0x9CC7E0): tres angulos aleatorios (16 bits) y velocidad de giro 1.2*(0x200 + rand&0xFF).
void B6Bat::rotInit(B6BatState& s, B6BatEnv& env) {
    s.rotA = env.rand() & 0xFFFF;
    s.rotB = env.rand() & 0xFFFF;
    s.rotC = env.rand() & 0xFFFF;
    s.rotSpeed = (env.rand() & 0xFF) + 0x200;
    s.rotSpeed = (int)(b6BitsToFloat(0x3F99999Au) * (float)s.rotSpeed);
}

// SetBatSpiral (0x9CCD00), parte escalar: reinicia la espiral global; spiralLen = 50*(200*velBase*kBatSpeedB[dif])/60;
// pendiente = dist/spiralLen. (La direccion y `dist` salen de matrices del motor: se pasan ya calculadas.)
void B6Bat::setBatSpiral(B6BatGlobals& g, const Boss6State& o, int diff, float dist) {
    g.spiralDist = dist;
    g.spiralHitFlag = 0;
    g.spiralStart = 0.0f;
    g.spiralRate = 1.0f;
    float f3 = (b6BitsToFloat(0x43480000u) * o.speedBase) * b6BitsToFloat(kBatSpeedB_bits[diff]);
    g.spiralLen = (b6BitsToFloat(0x42480000u) * f3) / b6BitsToFloat(0x42700000u);
    g.spiralSlope = g.spiralDist / g.spiralLen;
}

// AddBatSpiralCount (0x9CCEA0): avanza la espiral global.
void B6Bat::addBatSpiralCount(B6BatGlobals& g) { g.spiralStart = g.spiralStart + g.spiralRate; }

// ChkMode (0x9CD070): maquina de estados del murcielago.
// Si no esta cayendo/muerto: vida <= 0 -> modo 4 (suma sus bits de muerte al jefe, aviso, parametros de caida, sonido);
// jefe muerto (0x40000) -> modo 4. Despues, por modo: 0 -> 1; 1 -> espera el turno de su ataque (disparo o espiral);
// 3 -> vuelve a 1 al acabar el ataque; 4 -> al caer bajo Y < 0 pasa a 5.
void B6Bat::chkMode(B6BatState& s, B6BatGlobals& g, Boss6State& o, B6BatEnv& env) {
    if (s.mode != BAT_FALL && s.mode != BAT_DEAD) {
        if (s.hp <= 0) {
            s.mode = BAT_FALL;
            Boss6Logic::batKillAdd(o, s.killBits);
            if (s.alertSound != 0) env.alert(0x1A);
            setFallParam(s, env);
            env.sound(0x13001B, 0, 0, 0x40);
        }
        if (o.f1DA4 & boss6::FA_DEAD) {
            s.mode = BAT_FALL;
            setFallParam(s, env);
            return;
        }
    }
    switch (s.mode) {
    case BAT_IDLE:
        s.mode = BAT_WAIT;
        return;
    case BAT_WAIT: {
        if (o.f1DA4 & boss6::FA_ATTACK) {
            if (o.atkFlag == 0) return;
            if (o.atkKind == 0) {
                if (s.index == o.atkCount) {
                    s.flags |= BATF_ACTIVE;
                    s.flags |= BATF_SHOT;
                    env.setShotParam(s);
                    setPathParam(s, o, env);
                    s.mode = BAT_SHOT;
                    env.sound(0x1D000D, 0, 0, 0x40);
                }
            } else if (o.atkKind == 1) {
                float idx = (float)s.index;
                if (!(idx < g.spiralStart) && idx < g.spiralStart + g.spiralRate) {
                    s.flags |= BATF_ACTIVE;
                    setSpiralParam(s, g, o);
                    setApproach(s, b6BitsToFloat(0x3CA3D70Au));   // 0.02
                    s.mode = BAT_SPIRAL;
                }
            }
        } else if (o.f1DA4 & 0x800000) {
            float k = b6BitsToFloat(0x3A83126Fu) * (float)(s.index + 1);   // 0.001
            setApproach(s, b6BitsToFloat(0x38D1B717u) + k * k);            // 0.0001
        }
        return; }
    case BAT_SHOT:
        return;
    case BAT_SPIRAL: {
        uint32_t f = o.f1DA4;
        if (!(f & 0x4000) && (f & boss6::FA_ATTACK)) return;
        if (f & boss6::FA_DEAD) return;
        setApproach(s, b6BitsToFloat(0x3CA3D70Au));
        s.flags &= (int16_t)~BATF_SPIRAL;
        s.flags &= (int16_t)~BATF_ACTIVE;
        s.mode = BAT_WAIT;
        return; }
    case BAT_FALL:
        if (s.pos[1] < 0.0f) {
            s.alertSound = 0;
            chgDead(s);
        }
        return;
    default: break;
    }
}

// ExecMode (0x9CD3A0): ejecuta el modo (0 nada, 1 RotPos, 2 TraceShotPath, 3 TraceSpiralPath [con el global 701C apaga el bit 1],
// 4 FallDown, 5 borra la tarea y, si el jefe no esta muerto, crea otro murcielago con el mismo indice); luego siempre Approach.
void B6Bat::execMode(B6BatState& s, B6BatGlobals& g, const Boss6State& o, B6BatEnv& env) {
    switch (s.mode) {
    case BAT_WAIT: env.rotPos(s); break;
    case BAT_SHOT: traceShotPath(s, env); break;
    case BAT_SPIRAL:
        if (g.spiralHitFlag != 0) s.flags &= (int16_t)~BATF_ACTIVE;
        traceSpiralPath(s, g, env);
        break;
    case BAT_FALL: fallDown(s); break;
    case BAT_DEAD:
        if (!(o.f1DA4 & boss6::FA_DEAD)) env.spawnBat(s.index);
        s.deleted = true;
        break;
    default: break;
    }
    approach(s);
}

// PlHitChk (0x9CC680): con el murcielago activo (bit 1) y vivo, si esta a menos de 5 + radio de la camara: sonido, dano al
// jugador objetivo (func_212D00(t,1,9)) con bajada de dificultad (2,2) y sonido; con espiral (bit 4) marca el global 701C.
bool B6Bat::plHitChk(B6BatState& s, B6BatGlobals& g, Boss6State& o, Boss6Globals& og, B6BatEnv& env) {
    (void)o;
    if (!(s.flags & BATF_ACTIVE)) return false;
    if (s.hp <= 0) return false;
    float d = env.distToCam(s.pos);
    if (!(d < 5.0f + s.radius)) return false;
    env.sound(0x1D0011, 0, 0, 0x40);
    if (s.target >= 0 && env.damagePlayer(s.target, 1, 9) != 0) {
        Boss6Logic::decBS6Difficulty(og, s.target, 2, 2);
        env.sound(0x800114, 0, 0, 0x40);
    }
    if (s.flags & BATF_SPIRAL) g.spiralHitFlag = 1;
    return true;
}

// SetTargetID (0x9CC070): modo = 0x500DFC; objetivo = lastHitTarget (1 jugador) o rand & 1 (2 jugadores); otro valor: no cambia.
void B6Bat::setTargetID(int8_t& target, int8_t& mode, const Boss6Globals& og, B6BatEnv& env) {
    mode = (int8_t)og.gameMode;
    if (og.playersMode == 2) target = (int8_t)(env.rand() & 1);
    else if (og.playersMode == 1) target = (int8_t)og.lastHitTarget;
}

// ---------------------------------------------------------------- efectos pequenos

// TBS6SkyFade::Ctrl (0x9D2240): con el fundido libre: ratio = cnt/total; mode 0 sube el alfa del cielo (1.0 al acabar), otro
// lo baja (0.0 al acabar) y borra; cnt++. Si alguno de los dos globales de salto esta activo, fija el alfa final y borra.
void b6SkyFadeCtrl(B6SkyFade& s, B6SkyFadeEnv& env) {
    if (env.fadeBusy() <= 0) {
        s.ratio = (float)s.cnt / (float)s.total;
        if (s.mode == 0) {
            env.setSkyAlpha(s.ratio);
            if (!(s.cnt < s.total)) { env.setSkyAlpha(1.0f); s.deleted = true; }
        } else {
            env.setSkyAlpha(1.0f - s.ratio);
            if (!(s.cnt < s.total)) { env.setSkyAlpha(0.0f); s.deleted = true; }
        }
        s.cnt = s.cnt + 1;
    }
    if (env.skipFlagA() != 0 || env.skipFlagB() != 0) {
        env.setSkyAlpha(s.mode == 0 ? 1.0f : 0.0f);
        s.deleted = true;
    }
}

// TBS6MaskFeather::CtrlPre (0x9D25E0): desplazamiento de la pluma segun cnt/total: dy = -0.6*(t-0.5), dz = -0.3*(t-0.5).
void b6MaskFeatherPre(B6MaskFeather& s) {
    float t = (float)s.cnt / (float)s.total - 0.5f;
    s.dy = b6BitsToFloat(0xBF19999Au) * t;
    s.dz = b6BitsToFloat(0xBE99999Au) * t;
}
// TBS6MaskFeather::CtrlPost (0x9D26A0): cnt++; al llegar a total borra la tarea.
void b6MaskFeatherPost(B6MaskFeather& s) {
    s.cnt = s.cnt + 1;
    if (!(s.cnt < s.total)) s.deleted = true;
}
// TBoss6AfterImage::CtrlPre (0x9D1780): el alfa baja 0.02 por fotograma; con <= 0 borra la tarea.
void b6AfterImagePre(B6AfterImage& s) {
    s.alpha = s.alpha - b6BitsToFloat(0x3CA3D70Au);
    if (s.alpha <= 0.0f) s.deleted = true;
}

}}  // namespace vn::boss56

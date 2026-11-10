// Ver bosses56_b5.h. Cada funcion cita la original (direccion en el overlay bs5mod).
#include "bosses56_b5.h"
#include <cstring>
namespace vn { namespace boss56 {

namespace {
// Constante float a partir de sus bits exactos leidos del asm (evita redondeos de literales decimales).
float fbits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
const float kStepGauge = 0.083333336f;   // 0x3DAAAAAB: paso del nivel 43E4 por fotograma
}

// GetCamRot (0x9C7160): el id de ruta de camara 0x273..0x27E da un giro de (id-0x273)*0x1555; el resto 0.
int B5Move::getCamRot(B5MoveEnv& env) {
    int id = env.camTrackId();
    if (id >= 0x273 && id < 0x27F) return (id - 0x273) * 0x1555;
    return 0;
}

// GetCamPos (0x9C71C0): gira un vector con el giro de camara (solo eje Y) y devuelve el angulo en el original;
// aqui solo se escribe `out` (el angulo se obtiene con getCamRot).
void B5Move::getCamPos(B5MoveEnv& env, const float in[3], float out[3]) {
    env.rotYaw(getCamRot(env), in, out);
}

// UpdateMotionFrame (0x9C5970): si el movimiento no esta detenido (+0x0C & 0x4000) avanza los dos canales.
void B5Move::updateMotionFrame(B5MoveState& b) {
    if (!(b.f0C & B5F0C_FROZEN)) {
        b.frame3C += b.rate;
        b.frame40 += b.rate2;
    }
}

// ControlByMotion (0x9C59B0): avisos de sonido al cruzar un fotograma del movimiento 0x1C9 (72 y 240).
// Estructura leida de m2c (misma forma que ControlByMotionX, cuya cadena if/else-if si se verifico en el asm).
void B5Move::controlByMotion(B5MoveState& b, B5MoveEnv& env) {
    float prev = b.frame, next = b.frame + b.rate;
    int snd = -1;
    if (b.motion == 0x1C9) {
        if (prev < 72.0f && !(next < 72.0f)) snd = 0x1A0124;
        else if (prev < 240.0f && !(next < 240.0f)) snd = 0x1A0125;
    }
    if (snd >= 0) env.sound(snd, snd, 0);
}

// ControlByMotionX (0x9C5A70): avisos de sonido/efecto por fotograma segun el movimiento (cruce prev < T <= next, la primera
// regla que acierta gana), y sonidos continuos con estado en +0x1E5D.
void B5Move::controlByMotionX(B5MoveState& b, B5MoveEnv& env) {
    if (b.f1E40 & B5F_NO_CUES) return;
    const float prev = b.frame, next = b.frame + b.rate;
    auto cross = [&](float t) { return prev < t && !(next < t); };
    // punto de los efectos: posicion del jefe con Y = -11.1 (0xC1319999)
    float p50[3] = {b.pos[0], fbits(0xC1319999u), b.pos[2]};
    int snd = -1, stop = 0;
    bool wentTo1DD = false;   // 0x1D8 sigue en la comprobacion de 0x1DD
    switch (b.motion) {
    case 0x1D3:
        if (cross(75.0f)) snd = 0x1A010C;
        else if (cross(80.0f)) snd = 0x1B0022;
        else if (cross(220.0f)) snd = 0x1A000E;
        break;
    case 0x1D6:
        if (cross(1.0f)) snd = 0x1A000D;
        else if (cross(75.0f)) snd = 0x1A010C;
        else if (cross(80.0f)) snd = 0x1B0013;
        break;
    case 0x1D4:
        if (cross(14.0f)) snd = 0x1A000F;
        break;
    case 0x1DC:
        if (cross(66.0f)) { snd = 0x1A0114; env.effect(1.0f, 0x39, p50); }
        else if (cross(100.0f)) env.effect(20.0f, 0x2F, p50);
        break;
    case 0x1DA:
        if (cross(29.0f)) env.effect(1.0f, 0x39, p50);
        break;
    case 0x1D8:
        if (cross(30.0f)) snd = 0x1A0010;
        else if (cross(25.0f)) { snd = 0x1A0115; env.effect(1.0f, 0x39, p50); }
        wentTo1DD = true;
        break;
    case 0x1DD:
        wentTo1DD = true;
        break;
    default: break;
    }
    if (wentTo1DD && cross(56.0f)) {
        float p70[3];
        env.posRotY(0.0f, fbits(0xC1333333u), 12.0f, p70);
        env.effect(1.0f, 0x39, p70);
    }
    if (b.motion == 0x1DA && (b.counter % 5u) == 0)
        env.effect(15.0f + 5.0f * env.randFloat(), 0x2E, p50);
    if ((b.f1E40 & B5F_SND_2000) && (b.counter % 15u) == 0)
        env.effect(fbits(0x3FCCCCCDu), 0x31, p50);   // 1.6
    // sonidos continuos: se arrancan al activarse la condicion y se paran (stop = 1) al desactivarse
    if (snd < 0) {
        if (b.motion == 0x1DA) {
            if (!(b.cueBits & 1)) { b.cueBits |= 1; snd = 0x1A0113; }
        } else if (b.cueBits & 1) { snd = 0x1A0113; stop = 1; b.cueBits &= (uint8_t)~1; }
    }
    if (snd < 0) {
        if (b.f1E40 & B5F_PILLARS) {
            if (!(b.cueBits & 2)) { b.cueBits |= 2; snd = 0x1B001C; }
        } else if (b.cueBits & 2) { snd = 0x1B001C; stop = 1; b.cueBits &= (uint8_t)~2; }
    }
    if (snd < 0) {
        if (b.f1E40 & B5F_SND_2000) {
            if (!(b.cueBits & 4)) { b.cueBits |= 4; snd = 0x1C000F; }
        } else if (b.cueBits & 4) { snd = 0x1C000F; stop = 1; b.cueBits &= (uint8_t)~4; }
    }
    if (snd >= 0) env.sound(snd, snd, stop);
}

// InitPath (0x9C6B30): pone a cero el estado de ruta; velocidad 1.2 y mezcla 1.0.
void B5Move::initPath(B5MoveState& b) {
    b.path = B5PathSample{};
    b.pathId = 0; b.f1DD8 = 0; b.pathLimit = 0; b.pathT = 0;
    b.pathSpeed = fbits(0x3F99999Au);   // 1.2
    b.f1DE8 = 0; b.pathKind = 0;
    b.pathBlend = 1.0f; b.pathBlendStep = 0;
}

// ControlPath (0x9C6B80): evalua la ruta en pathT, la gira con el giro de camara, orienta al jefe segun pathKind,
// aplica el desplazamiento opcional, avanza la mezcla y la posicion en la ruta (acotada a pathLimit).
void B5Move::controlPath(B5MoveState& b, B5MoveEnv& env) {
    float cam[3];
    env.camPos(cam);
    env.pathEval(b.pathT, b.pathId, b.path);
    int camRot = getCamRot(env);
    float p[3];
    env.rotYaw(camRot, b.path.pos, p);
    int32_t ry = b.path.rot[1] + camRot;
    switch (b.pathKind) {
    case 0: case 1:   // mira a la camara
        b.pathYaw = (int16_t)env.atan2Ang(cam[0] - p[0], cam[2] - p[2]);
        ry = b.pathYaw;
        break;
    case 2: case 3: {  // mira en el sentido de la ruta (un poco mas adelante)
        float t2 = fbits(0x3A83126Fu) + b.pathT;   // 0.001
        if (t2 <= b.pathLimit) {
            B5PathSample s;
            env.pathEval(t2, b.pathId, s);
            b.pathYaw = camRot + (int16_t)env.atan2Ang(s.pos[0] - b.path.pos[0], s.pos[2] - b.path.pos[2]);
            // (para pathId 0x9D/0xAF el original resta 2.0822 a la Y de `s`, valor que luego no se usa)
        }
        ry = b.pathYaw;
        break; }
    default: break;
    }
    if (b.f1E40 & B5F_OFFSET) { p[0] += b.off[0]; p[1] += b.off[1]; p[2] += b.off[2]; }
    b.pos[0] = p[0]; b.pos[1] = p[1]; b.pos[2] = p[2];
    b.rot[0] = 0; b.rot[1] = ry; b.rot[2] = 0;
    if (b.pathBlend < 1.0f) {
        b.pathBlend += b.pathBlendStep;
        if (!(b.pathBlend <= 1.0f)) { b.pathBlend = 1.0f; b.pathBlendStep = 0; }
    }
    b.pathT += b.pathSpeed;
    if (!(b.pathT < b.pathLimit)) b.pathT = b.pathLimit;
}

// GetWaitPos (0x9C6F90): posicion de la ruta de espera (0xC0..0xC3) girada con la rotacion del jefe. Fuera de ese rango no escribe.
void B5Move::getWaitPos(B5MoveState& b, B5MoveEnv& env, float out[3]) {
    if (b.waitSel < 0xC0 || b.waitSel >= 0xC4) return;
    B5PathSample s;
    env.pathEval(b.waitT, b.waitSel, s);
    env.rotOrdered(b.rotOrder, b.rot, s.pos, out);
}

// InitWaitPath (0x9C6E60): empieza en 0, velocidad 1.2, elige una de las 4 rutas (0xC0 + rand % 4, resto con signo de C)
// y deja el desplazamiento actual = el anterior.
void B5Move::initWaitPath(B5MoveState& b, B5MoveEnv& env) {
    b.waitT = 0;
    b.waitSpeed = fbits(0x3F99999Au);
    b.waitSel = 0xC0 + env.rand() % 4;
    getWaitPos(b, env, b.off);
    b.offPrev[0] = b.off[0]; b.offPrev[1] = b.off[1]; b.offPrev[2] = b.off[2];
}

// ControlWaitPath (0x9C6EE0): con ruta de espera valida, guarda el desplazamiento anterior, calcula el nuevo y avanza waitT
// dando la vuelta a la longitud de la ruta. (El original se colgaria con longitud <= 0; aqui se sale.)
void B5Move::controlWaitPath(B5MoveState& b, B5MoveEnv& env) {
    if (b.waitSel < 0xC0 || b.waitSel >= 0xC4) return;
    for (int i = 0; i < 3; i++) b.offPrev[i] = b.off[i];
    getWaitPos(b, env, b.off);
    float len = (float)env.pathLen(b.waitSel);
    b.waitT += b.waitSpeed;
    if (len <= 0.0f) return;
    while (!(b.waitT <= len)) b.waitT -= len;
}

// ControlPos (0x9C6560): gira hacia la camara (flag 2 o atacando, a 0x400); con ruta (1DB8 == 1) llama a ControlPath,
// si no, con el flag 0x80000 suma al jefe el cambio del desplazamiento de espera.
void B5Move::controlPos(B5MoveState& b, B5MoveEnv& env) {
    if ((b.f1E40 & B5F_TURN_TO_CAM) || (b.f0C & B5F0C_ATTACKING)) {
        float cam[3];
        env.camPos(cam);
        env.turnToCam(cam, 0x400);
    }
    if (b.pathMode != 1) {
        if (b.f1E40 & B5F_OFFSET) {
            b.pos[0] += b.off[0] - b.offPrev[0];
            b.pos[1] += b.off[1] - b.offPrev[1];
            b.pos[2] += b.off[2] - b.offPrev[2];
        }
    } else {
        controlPath(b, env);
    }
}

// CtrlPre_battle1 (0x9C5560): orden por fotograma de la forma inicial.
// (El original copia una vez 4 tablas estaticas D_009CE800.. a D_26B480.. -parametros de luz/niebla del motor- y no se reescribe.)
void B5Move::ctrlPreBattle1(B5StepEnv& st) {
    st.step(B5S_DISPATCH_STATE);
    st.step(B5S_FALL_CHECK);
    st.step(B5S_CHECK_DAMAGE_END);
    st.step(B5S_CHECK_DAMAGE);
    st.step(B5S_CONTROL_ATTACK);
    st.step(B5S_CONTROL_POS);
    st.step(B5S_CONTROL_ALPHA);
    st.step(B5S_BY_MOTION);
}

// CtrlPre_battle2 (0x9C5690): orden por fotograma del combate principal (con ruta de espera y pilares).
void B5Move::ctrlPreBattle2(B5StepEnv& st) {
    st.step(B5S_DISPATCH_STATE);
    st.step(B5S_FALL_CHECK);
    st.step(B5S_CHECK_DAMAGE_END);
    st.step(B5S_CHECK_DAMAGE);
    st.step(B5S_CONTROL_ATTACK);
    st.step(B5S_WAIT_PATH);
    st.step(B5S_CONTROL_POS);
    st.step(B5S_WATER_PILLAR);
    st.step(B5S_BY_MOTION_X);
}

// CtrlPre (0x9C5010): si 1D74 != 0 espera a que la ruta de camara valga 0x272 (entonces lo limpia y lanza un efecto grande);
// si no hay aviso, no hace nada mas. Despues guarda la pose del fotograma, quita el bit 0x40 de 1E40, llama a battle1/2
// segun el tipo (1E48), sube/baja el nivel 43E4 (0..5 a 1/12 por fotograma, flag 0x4000 = subir) y, con el flag 0x200,
// controla la barra de vida.
void B5Move::ctrlPre(B5MoveState& b, B5MoveEnv& env, B5StepEnv& st) {
    if (b.kind74 != 0) {
        if (env.camTrackId() != 0x272) return;
        b.kind74 = 0;
        const float at[3] = {fbits(0xC2C00000u), 0.0f, fbits(0xC2460000u)};   // (-96, 0, -49.5)
        env.effectBig(1.0f, fbits(0x43480000u), 0x46, at);                    // 200.0
    }
    for (int i = 0; i < 3; i++) {
        b.savedPos[i] = b.pos[i]; b.savedF0C[i] = b.objF0C[i];
        b.savedScale[i] = b.objScale[i]; b.savedRot[i] = b.rot[i];
    }
    b.f1E40 &= ~(uint32_t)B5F_CLEAR_AT_PRE;
    if (b.kind == 1) ctrlPreBattle2(st);
    else if (b.kind == 0) ctrlPreBattle1(st);
    if (b.gaugeObj != 0) {
        if (b.f1E40 & B5F_GAUGE_UP) {
            if (b.gaugeLevel < 5.0f) {
                b.gaugeLevel += kStepGauge;
                if (!(b.gaugeLevel <= 5.0f)) b.gaugeLevel = 5.0f;
            }
        } else if (!(b.gaugeLevel <= 0.0f)) {
            b.gaugeLevel -= kStepGauge;
            if (b.gaugeLevel < 0.0f) b.gaugeLevel = 0.0f;
        }
        env.setGaugeLevel(b.gaugeObj, b.gaugeLevel);
    }
    if (b.f1E40 & B5F_LIFEBAR) st.step(B5S_LIFE_DISPLAY);
}

}}  // namespace vn::boss56

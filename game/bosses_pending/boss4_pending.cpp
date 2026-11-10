// Funciones pendientes de TBoss4 reescritas a mano (ver boss4_pending.h y MANIFEST.md). Cada funcion cita la original y su direccion.
#include "boss4_pending.h"
#include <cmath>
#include <cstring>
namespace vn { namespace bosspend {

static float bitsF(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
static double bitsD(uint64_t u) { double d; std::memcpy(&d, &u, 8); return d; }
static float asF(uint32_t u) { return bitsF(u); }
static uint32_t asU(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
static const float kRate = 1.2f;            // 0x3F99999A: 60 Hz -> 50 Hz (PAL)

// Copia (la primera vez en todo el juego, gp-0x7460) de D_009D2B00 a D_26B430 y consulta de estado de la tarea (func_1C6C80);
// el bloque aparece identico al principio de CtrlPre_battle1/2/3 y CtrlPre_bunshin.
static void runState(Boss4State& b, Boss4Ext& x, Boss4PendGlobals& pg, Boss4PendEnv& env) {
    if (pg.stateTableLoaded == 0) { env.loadStateTable(kB4StateRows); pg.stateTableLoaded = 1; }
    x.stateResult = (int16_t)env.stateRun(b.state);
}

// ---------------------------------------------------------------- bucle por fotograma
// CtrlPre__6TBoss4Fv (0x9C5450): mide el desplazamiento del fotograma, despacha por tipo y atiende barra de vida y sonido.
void Boss4Pending::ctrlPre(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env) {
    float d[3] = {b.pos[0] - x.prevPos[0], b.pos[1] - x.prevPos[1], b.pos[2] - x.prevPos[2]};
    for (int i = 0; i < 3; i++) { x.accel[i] = d[i] - x.prevDelta[i]; x.prevDelta[i] = d[i]; }
    float h = env.headingOf(d);
    x.headingDelta = h - x.heading; x.heading = h;
    for (int i = 0; i < 3; i++) x.prevPos[i] = b.pos[i];
    b.f1E14 &= ~0x400u;
    b.motReqMode = 0;
    switch (b.kind) {                                    // el original encadena beq en el orden 4,3,2,1,0; otro valor no hace nada
    case K_BUNSHIN:
        ctrlPreBunshin(b, x, pg, env);
        x.stateResult = (int16_t)env.stateRun(b.state);  // el original repite func_1C6C80 aqui (segunda llamada del fotograma)
        break;
    case K_SHOUKAN:  /* CtrlPre_shoukan (0x9C6050): pendiente, no reescrita */ break;
    case K_BATTLE3: ctrlPreBattle3(b, x, g, pg, env); break;
    case K_BATTLE2: ctrlPreBattle2(b, x, g, pg, env); break;
    case K_BATTLE1: ctrlPreBattle1(b, x, g, pg, env); break;
    default: break;
    }
    if ((b.f1E14 & FB_LIFEBAR) && !(b.f1E14 & 4)) Boss4Logic::controlDispLife(b, g);
    if (b.kind != K_BUNSHIN) env.controlSound();
}

// CtrlPost__6TBoss4Fv (0x9C5B90)
void Boss4Pending::ctrlPost(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    controlEffect();
    updateMotionFrame(b, x);
    env.entryCameraTarget();
    env.entryCollision();
    x.frameCounter++;
}

// CtrlPre_battle1__6TBoss4Fv (0x9C5BE0)
void Boss4Pending::ctrlPreBattle1(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env) {
    runState(b, x, pg, env);
    Boss4Logic::fallCheck(b, env.groundY(b.pos));
    checkDamageEnd(b, x, g, env);
    Boss4Logic::HitInfo hit[2]; env.hitInfo(hit);
    Boss4Logic::checkDamage(b, g, env, hit);
    Boss4Logic::controlAttack(b, g, env);
    controlPos(b, x, env);
    controlBunshinBattle1(b, env);
    controlCamera(b, x, g);
    Boss4Logic::controlAlpha(b);
}
// CtrlPre_battle2__6TBoss4Fv (0x9C5D60)
void Boss4Pending::ctrlPreBattle2(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env) {
    runState(b, x, pg, env);
    Boss4Logic::fallCheck(b, env.groundY(b.pos));
    checkDamageEnd(b, x, g, env);
    Boss4Logic::HitInfo hit[2]; env.hitInfo(hit);
    Boss4Logic::checkDamage(b, g, env, hit);
    Boss4Logic::controlAttack(b, g, env);
    controlPos(b, x, env);
    controlBunshinBattle2(b);
    env.controlGareki();
}
// CtrlPre_battle3__6TBoss4Fv (0x9C5EE0)
void Boss4Pending::ctrlPreBattle3(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env) {
    runState(b, x, pg, env);
    Boss4Logic::fallCheck(b, env.groundY(b.pos));
    checkDamageEnd(b, x, g, env);
    Boss4Logic::HitInfo hit[2]; env.hitInfo(hit);
    Boss4Logic::checkDamage(b, g, env, hit);
    controlAttackBattle3(b, x, g);
    controlPos(b, x, env);
}
// CtrlPre_bunshin__6TBoss4Fv (0x9C68D0): un clon consulta su estado y, salvo que este sea 3, sigue al jefe segun el tipo del padre.
void Boss4Pending::ctrlPreBunshin(Boss4State& b, Boss4Ext& x, Boss4PendGlobals& pg, Boss4PendEnv& env) {
    runState(b, x, pg, env);
    if (b.stateB == 3) return;
    if (!b.owner) return;                                // el original desreferencia el padre sin comprobar
    if (b.owner->kind == K_BATTLE2) ctrlPreBunshinBattle2(b, x, env);
    else if (b.owner->kind == K_BATTLE1) ctrlPreBunshinBattle1(b, x, env);
}

// UpdateMotionFrame__6TBoss4Fv (0x9C7330): avanza los dos contadores de movimiento salvo que la tarea este marcada para borrar (0x4000).
void Boss4Pending::updateMotionFrame(Boss4State& b, Boss4Ext& x) {
    if (b.f0C & F0C_DELETE) return;
    b.motionFrame += b.rate;
    x.frameB += x.rateB;
}

// ControlCamera__6TBoss4Fv (0x9C95D0): en la pelea 1, al cruzar el fotograma 195 de los movimientos 0x188..0x18A se prohibe el cambio de camara.
void Boss4Pending::controlCamera(Boss4State& b, Boss4Ext& x, Boss4Globals& g) {
    float f = x.frameC;
    if (b.kind != K_BATTLE1) return;
    if (b.motion == 0x188 || b.motion == 0x189 || b.motion == 0x18A) {
        if (f < 195.0f && !((f + b.rate) < 195.0f)) g.cameraStartAllowed = 0;   // ForbidCameraStart; el original deja f < 195 y f + rate >= 195
    }
}

// ---------------------------------------------------------------- posicion y altura
// ControlPos__6TBoss4Fv (0x9C8770)
void Boss4Pending::controlPos(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    if (b.f1E14 & FB_VISIBLE) { float p[3]; int32_t a; env.camTrack(p, a); turnPos(env, p, 0x400); }
    if (b.kind == K_BATTLE1) {
        // avance en z segun el movimiento (retroceso lento durante 0x18B, 0x18F y parte de 0x16B)
        float dz = 0.0f; bool scaled = false;
        switch (b.motion) {
        case 0x18B: scaled = true; dz = b.runB == 1 ? bitsF(0xBE051EB8) : bitsF(0xBDF5C28F); break;      // -0.13 / -0.12
        case 0x18F: scaled = true; dz = b.runB == 1 ? bitsF(0xBE0A3D71) : bitsF(0xBE000000); break;      // -0.135 / -0.125
        case 0x16B: if (!(b.motionFrame <= 110.0f) && b.motionFrame < 206.0f) dz = bitsF(0xBE99999A); break;  // -0.3 entre 110 y 206
        default: break;
        }
        if (scaled) {                                    // runB fuera de 0..2: el original deja dz = 0
            if (b.runB < 0 || b.runB > 2) dz = 0.0f;
            switch (b.phaseCount) { case 1: dz *= bitsF(0x3FCCCCCD); break; case 2: case 3: dz *= 2.0f; break; default: break; }
        }
        b.pos[2] += dz;
    }
    switch (b.b1DB8) {
    case POS_CAM_PATH: {
        env.pathSample(b.f1DFC, x.pathId, x.anpo);
        float p[3]; int32_t ax, ay, az;
        float anpoPos[3] = {asF(x.anpo[0]), asF(x.anpo[1]), asF(x.anpo[2])};
        float jv[3] = {0, 0, 0}; bool jit = false;
        const bool jitterNow = (b.camSpeed1E00 == 0.0f) || (b.f1DFC == b.f1DF8);
        auto sampleJitter = [&](bool transform) {
            uint32_t buf[6] = {0, 0, 0, 0, 0, 0};
            env.pathSample(x.jitterT, 0x59, buf);
            float v[3] = {asF(buf[0]), asF(buf[1]), asF(buf[2])};
            if (transform) env.mtxVector(v, jv); else { jv[0] = v[0]; jv[1] = v[1]; jv[2] = v[2]; }
            jit = true;
        };
        if (b.subPhase == 0) {
            if (x.slot >= 0 && x.slot < 7) {
                const B4ShoukanSlot& S = kB4ShoukanSlot[(int)x.slot];
                const float* T = kB4PathShift[(int)x.slot];
                env.mtxPush(); env.mtxTranslate(T[0], T[1], T[2]); env.mtxRotY(S.angle - 0x4000);
                env.mtxPoint(anpoPos, p);
                ax = (int32_t)x.anpo[3]; ay = (int32_t)x.anpo[4] + S.angle + 0x8000; az = (int32_t)x.anpo[5];
                if (jitterNow) sampleJitter(true);
            } else {
                p[0] = anpoPos[0]; p[1] = anpoPos[1]; p[2] = anpoPos[2];
                ax = (int32_t)x.anpo[3]; ay = (int32_t)x.anpo[4]; az = (int32_t)x.anpo[5];
                if (jitterNow) sampleJitter(false);
            }
            if (x.pathId == 0x4C) p[1] += -12.0f;
        } else if (b.subPhase == 1) {
            if (x.slot >= 0 && x.slot < 7) {
                const B4ShoukanSlot& S = kB4ShoukanSlot[(int)x.slot]; const B4ShoukanSlot& S0 = kB4ShoukanSlot[0];
                env.mtxPush(); env.mtxTranslate(S.x, 0.0f, S.z); env.mtxRotY(S.angle - S0.angle); env.mtxTranslate(-S0.x, 0.0f, -S0.z);
                env.mtxPoint(anpoPos, p);
                ax = (int32_t)x.anpo[3]; ay = (int32_t)x.anpo[4] + S.angle - S0.angle + 0x8000; az = (int32_t)x.anpo[5];
                if (jitterNow) sampleJitter(true);
            } else {
                p[0] = anpoPos[0]; p[1] = anpoPos[1]; p[2] = anpoPos[2];
                ax = (int32_t)x.anpo[3]; ay = (int32_t)x.anpo[4]; az = (int32_t)x.anpo[5];
                if (jitterNow) sampleJitter(false);
            }
        } else {                                         // subfase 2: la ruta tal cual, sin temblor
            p[0] = anpoPos[0]; p[1] = anpoPos[1]; p[2] = anpoPos[2];
            ax = (int32_t)x.anpo[3]; ay = (int32_t)x.anpo[4]; az = (int32_t)x.anpo[5];
        }
        if (jit) { p[0] += jv[0]; p[1] += jv[1]; p[2] += jv[2]; }
        float t = b.f1E04;
        if (t < 1.0f) {                                  // mezcla hacia el punto de la ruta
            for (int i = 0; i < 3; i++) b.pos[i] = b.pos[i] + b.f1E04 * (p[i] - b.pos[i]);
            b.rot[0] = (float)ax; b.rot[1] = (float)ay; b.rot[2] = (float)az;
            b.f1E04 = b.f1E04 + b.f1E08;
            if (!(b.f1E04 <= 1.0f)) { b.f1E04 = 1.0f; b.f1E08 = 0.0f; }
        } else {
            for (int i = 0; i < 3; i++) b.pos[i] = p[i];
            b.rot[0] = (float)ax; b.rot[1] = (float)ay; b.rot[2] = (float)az;
        }
        break;
    }
    case POS_FREE:                                       // fisica libre: velocidad += aceleracion; posicion += velocidad
        x.velX += x.acc[0]; b.velY += x.acc[1]; x.velZ += x.acc[2];
        b.pos[0] += x.velX; b.pos[1] += b.velY; b.pos[2] += x.velZ;
        break;
    default: break;
    }
    b.f1DFC = b.f1DFC + b.camSpeed1E00;
    if (!(b.f1DFC < b.f1DF8)) b.f1DFC = b.f1DF8;
    if ((b.camSpeed1E00 == 0.0f) || (b.f1DFC == b.f1DF8)) x.jitterT = x.jitterT + kRate; else x.jitterT = 0.0f;
}

// SetHeight__6TBoss4Fv (0x9CD060): pega el cuerpo al suelo; con el movimiento 0x17C (caida) lo deja caer en cuanto supera la altura del suelo.
void Boss4Pending::setHeight(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    float h = x.groundOffset + env.groundY(b.pos);
    if (b.motion == MOT_FALL) {
        if (!(b.pos[1] <= h)) {
            if (!(b.f10 & 0x40)) { b.f10 |= 0x40; b.fallStep = 0; }
        }
    } else if (!(b.f10 & 0x40)) {
        b.pos[1] = h;
    }
}

// CheckDamageEnd__6TBoss4Fv (0x9C9930): fin del aturdimiento por golpe y fase de "cuerpo lanzado" de la pelea 3 (tipo 2).
void Boss4Pending::checkDamageEnd(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendEnv& env) {
    if (b.f1E14 & FB_HIT_STUN) {
        b.stunTimer = (int16_t)(b.stunTimer - 1);
        if (b.stunTimer == 0) {
            b.f1E14 &= ~(uint32_t)FB_HIT_STUN;
            b.rateMul = 1.0f;
            b.rate = b.rateBase * 1.0f;
        }
    }
    if (b.kind != K_BATTLE3) return;
    if (!(b.f0C & F0C_IN_DAMAGE)) return;
    if (b.subPhase == 0) {
        if ((b.f1E14 & 0x800000) && g.enemiesLeft < 3) b.f1E14 &= 0xFF7FFFFFu;
    }
    if (b.subPhase == 1) {
        if (env.camFrame() < 2.0f) g.cameraStartAllowed = 1;     // AllowCameraStart (byte 0x500E10)
    }
    env.hook(H_SET_SCREEN_OFFSET, 0, 1, 0);                      // func_1ABD40(0, 1, ...)
    switch (b.motion) {
    case MOT_HIT_JUMP:
        if (!(b.motionFrame < (float)env.motionLen(b.motion))) {
            b.f1E14 |= FB_VISIBLE;
            float sx, sy; env.screenPos(b.pos, sx, sy);
            if (!(sx <= 320.0f)) Boss4Logic::motSupSet2(b, env, MOT_HIT_LAND_B, 0, 5);
            else Boss4Logic::motSupSet2(b, env, MOT_HIT_LAND_A, 0, 5);
        }
        break;
    case MOT_HIT_LAND_A: case MOT_HIT_LAND_B:
        if (!(b.motionFrame <= 20.0f) && x.headingDelta < 0.0f && !(b.rate <= 0.0f)) b.rate = 0.0f;
        break;
    default: break;
    }
    if (b.rate == 0.0f) {                                        // congelado: sigue con el ultimo desplazamiento
        b.pos[0] += x.holdDelta[0]; b.pos[1] += x.holdDelta[1]; b.pos[2] += x.holdDelta[2];
    } else {
        x.holdDelta[0] = x.prevDelta[0]; x.holdDelta[1] = x.prevDelta[1]; x.holdDelta[2] = x.prevDelta[2];
    }
    float sx, sy; env.screenPos(b.pos, sx, sy);
    if (sx < -32.0f || !(sx <= 672.0f) || sy < -64.0f || !(sy <= 512.0f)) {   // fuera de pantalla: termina la fase
        b.f0C &= ~(uint32_t)F0C_IN_DAMAGE;
        b.f1E14 &= ~(uint32_t)FB_VISIBLE;
        b.rate = kRate;
    }
}

// ClearWeakHit__6TBoss4Fv (0x9CAE20): quita 0x40000000 de las banderas de las 4 partes del cuerpo (+0x1D58).
void Boss4Pending::clearWeakHit(Boss4Ext& x) {
    for (int i = 0; i < 4; i++) if (x.partFlags[i]) *x.partFlags[i] &= 0xBFFFFFFFu;
}

// TurnPos__6TBoss4FP11tagNLpoint3i (0x9CD6B0): gira el cuerpo hacia el punto (func_951200).
void Boss4Pending::turnPos(Boss4PendEnv& env, const float point[3], int step) { env.turnToward(point, step); }

// ---------------------------------------------------------------- movimiento
// MotSupSet__6TBoss4Fiii (0x9CD230): como MotSupSet2 pero (1) ignora la peticion si la tarea esta en dano (0x40000000) y (2) compara
// con el movimiento del objeto de movimiento (*(this+8)+4) en vez de +0x34. Modo de la peticion: 1.
void Boss4Pending::motSupSet(Boss4State& b, Boss4Ext& x, Boss4Env& env, int motion, int a, int bb) {
    if ((b.f0C & F0C_IN_DAMAGE) || x.motionObjCur == motion) return;
    b.motReqMode = 1; b.motReqA = (int16_t)a; b.motReqB = (int16_t)bb;
    env.playMotion(motion, (int16_t)a);
    if (b.typeId == 0x1C) env.hook(H_PLAY_HAND_MOTIONS, (int16_t)a, 0, 0);
}

// ---------------------------------------------------------------- ataques
// ControlAttack_battle3__6TBoss4Fv (0x9C7CF0): ataque 0x12 (lluvia de rocas). Si quedan menos de 3 enemigos se cancela; si no, cada 40
// fotogramas suelta la primera roca que este esperando (estado 1 -> 2).
void Boss4Pending::controlAttackBattle3(Boss4State& b, Boss4Ext& x, Boss4Globals& g) {
    if (!(b.f1E14 & FB_ATTACK)) return;
    if (b.attack != 0x12) return;
    if (g.enemiesLeft < 3) {
        b.f0C &= ~(uint32_t)F0C_ATTACKING;
        b.f1E14 &= ~(uint32_t)FB_ATTACK;
        return;
    }
    int16_t t = b.s1F40; b.s1F40 = (int16_t)(t + 1);
    if (t % 40 != 0) return;
    for (int i = 0; i < x.iwaCount && i < kMaxIwa; i++) if (x.iwaState[i] == 1) { x.iwaState[i] = 2; break; }
}

static int ownerSlot(const Boss4Ext& x) { return x.ownerExt ? x.ownerExt->slot : x.slot; }

// StartAttackIwa__6TBoss4Fv (0x9C7FD0): crea las rocas (numero y velocidad por nivel de partida y dificultad). Cada una sale de una de
// las 25 posiciones de kB4IwaOrigin (empezando en una aleatoria) pasada por la matriz del punto de aparicion del jefe (o del padre).
// Las dos ramas de gameState == 0xA && gameKind == 2 del original hacen lo mismo (solo cambia una lectura sin efecto).
void Boss4Pending::startAttackIwa(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendEnv& env) {
    int level = g.gameLevel & 1;
    int dif = b.difficulty < 0 ? 0 : (b.difficulty > 16 ? 16 : b.difficulty);   // 0..16; el original no acota
    float speed = kB4IwaSpeed[level][dif];
    x.iwaCount = kB4IwaCount[level][dif];
    if (x.iwaCount > kMaxIwa) x.iwaCount = kMaxIwa;
    int sl = ownerSlot(x);
    if (sl < 0 || sl > 6) return;                        // el original no comprueba el indice; fuera de 0..6 leeria datos ajenos
    const B4ShoukanSlot& S = kB4ShoukanSlot[sl]; const B4ShoukanSlot& S0 = kB4ShoukanSlot[0];
    env.mtxPush();
    env.mtxTranslate(S.x, 0.0f, S.z);
    env.mtxRotY(S.angle - S0.angle);
    env.mtxTranslate(-S0.x, 0.0f, -S0.z);
    int r = env.rand() % 25;
    for (int i = 0; i < x.iwaCount; i++) {
        IwaSpawn sp;
        sp.index = (int8_t)i; sp.speed = speed;
        float out[3];
        env.mtxPoint(kB4IwaOrigin[(i + r) % 25], out);
        for (int k = 0; k < 3; k++) sp.srcPos[k] = out[k];
        sp.spawnPos[0] = out[0]; sp.spawnPos[1] = out[1] + 80.0f; sp.spawnPos[2] = out[2];
        x.iwaHandle[i] = env.spawnIwa(sp);
        x.iwaState[i] = 1;
    }
}

// StartAttackKiriPre__6TBoss4Fi (0x9C8330): coloca al jefe delante de la camara antes del ataque Kiri (arg 0/1) y lanza el movimiento.
void Boss4Pending::startAttackKiriPre(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env, int arg) {
    float tp[3]; int32_t ta; env.camTrack(tp, ta);
    int32_t s1 = ta + kB4KiriPreAngle[(int)x.b1E12 & 1][arg & 1];
    float a, c; env.dirXZ(s1 + kB4KiriPreAngleAdd[arg & 1] + 0x8000, a, c);
    const float k = bitsF(0x42C547AE);                   // 98.64
    b.pos[1] = 26.0f + 20.0f * (env.rand01() - 0.5f);
    b.pos[0] = tp[0] + k * a;
    b.pos[2] = tp[2] + k * c;
    b.rot[0] = 0; b.rot[1] = (float)s1; b.rot[2] = 0;
    Boss4Logic::motSupSet3(b, env, kB4KiriPreMotion[arg & 1], 0, 0);
    b.b1DB8 = 0;
}

// StartAttackKiri__6TBoss4Fv (0x9C8490): ataque Kiri normal: elige 1 de 3 posiciones alrededor de la camara. El original calcula
// x/z con aritmetica de doble precision por software (func_14B220 = suma, func_14B2E0 = producto, 14C6A8/14BAB0 = float<->double).
void Boss4Pending::startAttackKiri(Boss4State& b, Boss4PendEnv& env) {
    int r = env.rand() % 3;
    float tp[3]; int32_t ta; env.camTrack(tp, ta);
    int32_t ang = ta + kB4KiriAngle[r];
    float a, c; env.dirXZ(ang, a, c);
    const double K = bitsD(0x40602FD7DBF487FDull);       // ~129.5
    b.pos[0] = (float)((double)tp[0] + K * (double)a);
    b.pos[1] = kB4KiriY[r] - 8.0f;
    b.pos[2] = (float)((double)tp[2] + K * (double)c);
    b.rot[0] = 0; b.rot[1] = (float)(ang + 0x8000); b.rot[2] = 0;
    Boss4Logic::motSupSet3(b, env, MOT_KIRI, 0, 0);
    b.b1DB8 = 0;
}

// StartAttackKiri_shoukan__6TBoss4Fv (0x9C8660): variante del jefe invocado: a 157 de la camara, y = -2.8 y movimiento a elegir entre 3.
void Boss4Pending::startAttackKiriShoukan(Boss4State& b, Boss4PendEnv& env) {
    int r = env.rand() % 3;
    float tp[3]; int32_t ta; env.camTrack(tp, ta);
    float a, c; env.dirXZ(ta + 0x8000, a, c);
    b.pos[0] = tp[0] + 157.0f * a;
    b.pos[1] = bitsF(0xC0333333);                        // -2.8
    b.pos[2] = tp[2] + 157.0f * c;
    b.rot[0] = 0; b.rot[1] = (float)ta; b.rot[2] = 0;
    Boss4Logic::motSupSet3(b, env, kB4KiriShoukanMotion[r], 0, 0);
    b.b1DB8 = 0;
}

// GetShoukanPos__6TBoss4FP4ANPOP4ANPO (0x9C94B0): pasa un punto ANPO (24 B) al marco del punto de aparicion del jefe (o del padre).
void Boss4Pending::getShoukanPos(const Boss4Ext& x, Boss4PendEnv& env, const uint32_t in6[6], uint32_t out6[6]) {
    int sl = ownerSlot(x);
    if (sl < 0 || sl > 6) sl = 0;                        // el original no comprueba el indice
    const B4ShoukanSlot& S = kB4ShoukanSlot[sl]; const B4ShoukanSlot& S0 = kB4ShoukanSlot[0];
    env.mtxPush();
    env.mtxTranslate(S.x, 0.0f, S.z);
    env.mtxRotY(S.angle - S0.angle);
    env.mtxTranslate(-S0.x, 0.0f, -S0.z);
    float inp[3] = {asF(in6[0]), asF(in6[1]), asF(in6[2])}, out[3];
    env.mtxPoint(inp, out);
    out6[0] = asU(out[0]); out6[1] = asU(out[1]); out6[2] = asU(out[2]);
    out6[3] = in6[3];
    out6[4] = (uint32_t)((int32_t)in6[4] + S.angle - S0.angle);
    out6[5] = in6[5];
}

// DownDifficultyReq__6TBoss4Fi (0x9CA330): la peticion externa baja la dificultad (el argumento no se usa).
void Boss4Pending::downDifficultyReq(Boss4State& b) { Boss4Logic::downDifficulty(b); }

// ---------------------------------------------------------------- clones
// BnStartBunshin__6TBoss4Fv (0x9C6A60): el clon sale: en la pelea 1 se aleja del jefe con velocidad speedBase * D_009D3F10[runB][indice].
void Boss4Pending::bnStartBunshin(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    const Boss4State* o = b.owner;
    if (o && o->kind == K_BATTLE1) {
        x.bnOffset[0] = x.bnOffset[1] = x.bnOffset[2] = 0;
        x.bnVel[0] = o->speedBase * kB4BunshinSpread[(int)o->runB][x.bnIndex]; x.bnVel[1] = 0; x.bnVel[2] = 0;
    }
    x.bnMode = BN_OUT;
    b.f1E14 &= 0xFFFDFFFFu;
    env.fx9443A0(1, 5, 0x1E, 5, 0);
}
// BnStartShusoku__6TBoss4Fv (0x9C6B10): el clon vuelve: velocidad opuesta.
void Boss4Pending::bnStartShusoku(Boss4State& b, Boss4Ext& x) {
    const Boss4State* o = b.owner;
    if (o && o->kind == K_BATTLE1) {
        x.bnVel[0] = (-o->speedBase) * kB4BunshinSpread[(int)o->runB][x.bnIndex]; x.bnVel[1] = 0; x.bnVel[2] = 0;
    }
    x.bnMode = BN_BACK;
    b.f1E14 &= 0xFFFDFFFFu;
}
// BnStartStandby__6TBoss4Fv (0x9C6B90): 3 (volviendo) -> en espera (1) y visible; 2 (saliendo) -> reposo (0).
void Boss4Pending::bnStartStandby(Boss4State& b, Boss4Ext& x) {
    if (x.bnMode == BN_BACK) { x.bnMode = BN_STANDBY; b.f1E14 |= 0x20000; }
    else if (x.bnMode == BN_OUT) x.bnMode = BN_IDLE;
}
// BnCheckCmd__6TBoss4Fv (0x9C6BE0): si la orden del padre (+0x1E3C) cambio, arranca la accion correspondiente.
void Boss4Pending::bnCheckCmd(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    int8_t c = b.owner->bunshinCmd;
    if (x.bnCmd == c) return;
    x.bnCmd = c;
    switch (c) { case 1: bnStartBunshin(b, x, env); break; case 2: bnStartShusoku(b, x); break; case 0: bnStartStandby(b, x); break; default: break; }
}

// ControlBunshin_battle1__6TBoss4Fv (0x9C9660): lo ejecuta el JEFE (pelea 1): mientras pide clones (0x8000) o su reunion (0x10000)
// descuenta el contador +0x1E44 (s16) y, durante la salida, se desplaza segun runB (0 atras, 2 adelante, 1 quieto).
void Boss4Pending::controlBunshinBattle1(Boss4State& b, Boss4PendEnv& env) {
    uint32_t f = b.f1E14;
    if (f & FB_BUNSHIN_ASK) {
        b.f1E14 = f | FB_BN_CANCEL; b.bunshinCmd = 1;
        int16_t t = (int16_t)b.f1E44; b.f1E44 = (int16_t)(t - 1);
        if (t == 0) { b.f1E14 &= 0xFFFF7FFFu; b.bunshinCmd = 0; return; }
        float s2, s1; env.dirXZ((int32_t)b.rot[1], s2, s1);   // +0x28 del objeto de posicion: angulo Y; el 1er resultado va a z, el 2o a x
        s1 *= b.speedBase; s2 *= b.speedBase;
        switch (b.runB) {
        case 0: b.pos[0] -= s1; b.pos[2] -= s2; break;
        case 2: b.pos[0] += s1; b.pos[2] += s2; break;
        default: break;
        }
    } else if (f & FB_SHUSOKU_ASK) {
        b.f1E14 = f | FB_BN_CANCEL; b.bunshinCmd = 2;
        int16_t t = (int16_t)b.f1E44; b.f1E44 = (int16_t)(t - 1);
        if (t == 0) { b.f1E14 &= ~(uint32_t)FB_BN_CANCEL; b.f1E14 &= 0xFFFEFFFFu; b.bunshinCmd = 0; }
    }
}

// ControlBunshin_battle2__6TBoss4Fv (0x9C97E0): (pelea 2) segun el movimiento 0x16C..0x17E: pone o quita 0x4000 de +0x1E14
// (tabla de saltos jtbl_009DA760 en kB4BunshinMotAction).
void Boss4Pending::controlBunshinBattle2(Boss4State& b) {
    unsigned idx = (unsigned)(b.motion - 0x16C);
    if (idx >= 0x13) return;
    switch (kB4BunshinMotAction[idx]) { case 0: b.f1E14 |= FB_BN_CANCEL; break; case 1: b.f1E14 &= ~(uint32_t)FB_BN_CANCEL; break; default: break; }
}

// CtrlPre_bunshin_battle1__6TBoss4Fv (0x9C6C60): el clon de la pelea 1 copia el movimiento del jefe, se desvanece mientras el jefe esta
// en dano y se mueve con su desplazamiento propio respecto al jefe.
void Boss4Pending::ctrlPreBunshinBattle1(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    bnCheckCmd(b, x, env);
    Boss4State* o = b.owner;
    switch (o->motReqMode) {
    case 1: motSupSet(b, x, env, o->motion, o->motReqA, o->motReqB); break;
    case 2: Boss4Logic::motSupSet2(b, env, o->motion, o->motReqA, o->motReqB); break;
    case 3: Boss4Logic::motSupSet3(b, env, o->motion, o->motReqA, o->motReqB); break;
    default: break;
    }
    if (o->f0C & F0C_IN_DAMAGE) {
        b.alpha = b.alpha - bitsF(0x3D088889);           // 0.033333335 = 1/30
        if (b.alpha < 0.0f) b.alpha = 0.0f;
        x.bnMode = BN_STANDBY;
        b.f1E14 |= 0x20000;
    } else b.alpha = 1.0f;
    switch (x.bnMode) {
    case BN_BACK:
        for (int i = 0; i < 3; i++) x.bnOffset[i] += x.bnVel[i];
        if (std::sqrt(x.bnOffset[0] * x.bnOffset[0] + x.bnOffset[1] * x.bnOffset[1] + x.bnOffset[2] * x.bnOffset[2]) < bitsF(0x3DCCCCCD)) {   // < 0.1
            x.bnMode = BN_STANDBY; b.f1E14 |= 0x20000;
        }
        break;
    case BN_OUT:
        for (int i = 0; i < 3; i++) x.bnOffset[i] += x.bnVel[i];
        break;
    default: break;
    }
    for (int i = 0; i < 3; i++) b.pos[i] = o->pos[i] + x.bnOffset[i];
}

// CtrlPre_bunshin_battle2__6TBoss4Fv (0x9C6E90): el clon de la pelea 2 pide el movimiento que toca a su indice segun el movimiento del
// padre (jtbl_009DA690 en kB4BunshinB2Class, filas de kB4BunshinMotRows). Sin movimiento (-1) queda oculto/en espera.
void Boss4Pending::ctrlPreBunshinBattle2(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env) {
    bnCheckCmd(b, x, env);
    Boss4State* o = b.owner;
    const int32_t* row = kB4BunshinMotRows[(int)o->runB][x.bnIndex];
    int32_t mot = -1;
    unsigned idx = (unsigned)(o->motion - 0x16C);
    if (idx < 0x13) {
        switch (kB4BunshinB2Class[idx]) {
        case 0: mot = row[2 + o->runD]; break;
        case 1: mot = row[0]; break;
        case 2: mot = row[1]; break;
        case 3:                                          // 0x178: copia posicion y angulos del padre
            for (int i = 0; i < 3; i++) { b.pos[i] = o->pos[i]; b.rot[i] = o->rot[i]; }
            break;
        case 5: mot = 0x17E; break;
        default: break;
        }
    }
    if (mot == -1) { b.f1E14 |= 0x20000; return; }
    b.f1E14 &= 0xFFFDFFFFu;
    if (x.bnLastMot == mot) return;
    switch (o->motReqMode) {
    case 1: motSupSet(b, x, env, mot, o->motReqA, o->motReqB); break;
    case 2: Boss4Logic::motSupSet2(b, env, mot, o->motReqA, o->motReqB); break;
    default: Boss4Logic::motSupSet3(b, env, mot, o->motReqA, o->motReqB); break;   // 3 y cualquier otro valor
    }
    if (mot == 0x179 || mot == 0x17A || mot == 0x17B) env.fx9443A0(2, 1, 0x19, 0x19, 0);
    x.bnLastMot = (int16_t)mot;
}

}}  // namespace vn::bosspend

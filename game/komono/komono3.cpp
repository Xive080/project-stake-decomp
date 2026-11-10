// Komono3Task (objetos de coleccion de misiones "Org"): ver komono.h. Cada funcion cita la original (asm/aomod/aomod_text.s).
#pragma STDC FP_CONTRACT OFF   // la PS2 no tiene FMA: sin contraccion de a*b+c
#include "komono.h"
#include <cstring>
namespace vn { namespace komono {

static void sound(KomonoHost& h, int id, int a2 = 0) { h.sound(id, a2, 0); }

// setBranchNo__11Komono3TaskFv (0x9ED560)
void Komono3Logic::setBranchNo(KomonoEnv& env) { env.branchFlag = 1; env.branchNo = 1; }
// clrBranchNo__11Komono3TaskFv
void Komono3Logic::clrBranchNo(KomonoEnv& env) { env.branchFlag = 0; env.branchNo = 0; }

// checkCamPath__11Komono3TaskFii: la camara actual es `path` y su frame ya llego a `frame`.
bool Komono3Logic::checkCamPath(const KomonoEnv& env, int path, int frame) {
    if ((int16_t)env.camPathId == path) return !(env.camFrame < (float)frame);
    return false;
}

// checkHit__11Komono3TaskFPi: bit 8 = golpe; elige jugador (ver komonoHitPlayer); consume el golpe.
int Komono3Logic::checkHit(Komono3State& s, KomonoHost& h, int* player) {
    if (!(s.flags & 8)) return 0;
    int p = komonoHitPlayer(s.flags, h, 0, true);
    s.flags &= ~8u;
    *player = p;
    return 1;
}

// calcEffPos__11Komono3TaskFv: posicion del efecto = transformacion de (colli.x, colli.y + 1, colli.z + 2) por la camara; el resto son
// las palabras +0x9C/+0xA0/+0xA4 de la entrada de camara.
void Komono3Logic::calcEffPos(const Komono3State& s, const KomonoEnv& env, KomonoHost& h, float out[6]) {
    float in[3] = {s.colliPos[0], 1.0f + s.colliPos[1], 2.0f + s.colliPos[2]};
    h.camTransform(in, out);
    for (int k = 0; k < 3; k++) std::memcpy(&out[3 + k], &env.camVec[k], 4);
}

// setColli__11Komono3TaskF11tagNLpoint3
static void setColli(Komono3State& s, const float p[3], KomonoHost& h) { h.registerCollider(p, s.colliPos); }

// checkColli_setBranchNo__11Komono3TaskFv (0x9ED5F0)
void Komono3Logic::checkColli_setBranchNo(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    setColli(s, s.pos, h);
    int pl = 0;
    switch (s.state) {
        case 1:
            if (checkHit(s, h, &pl)) {
                float eff[6]; calcEffPos(s, env, h, eff);
                h.orgHitAgain(pl, eff);
                sound(h, 0x20025, 0x1000);
            }
            break;
        case 0:
            if (checkHit(s, h, &pl)) {
                h.orgHitCollect(pl, s.pos);
                setBranchNo(env);
                s.state = 1;
                sound(h, 0xB012B);
            }
            break;
        default: break;
    }
}

// checkMissionEnd_setTrnsl__11Komono3TaskFv
void Komono3Logic::checkMissionEnd_setTrnsl(Komono3State& s, const KomonoEnv& env) {
    switch (s.endState) {
        case 2:
            if (env.pauseLevel <= 0) {
                s.alpha = s.alpha - 0.033333335f;                // 1/30 por fotograma
                if (s.alpha < 0.0f) s.alpha = 0.0f;
            }
            break;
        case 1:
            if (env.msgPending == 0) s.endState = 2;
            break;
        default: break;
    }
}

// checkGameMode__11Komono3TaskFv (0x9ED7A0): el objeto solo vive en gameKind 1; sin recoger exige arcade + mision abierta (bit0) +
// activa (bit1) + no completada (bit5); si no, se retira y limpia la rama.
void Komono3Logic::checkGameMode(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    if (env.gameKind != 1) h.closeTask();
    if (s.state == 0) {
        uint8_t f = (s.mission >= 0 && s.mission < 16) ? env.missionFlags[s.mission] : 0;
        bool ok = env.gameState == 0xA && env.gameKind == 1 && (f & 1) && (f & 2) && !(f & 0x20);
        if (!ok) { clrBranchNo(env); h.closeTask(); }
    }
}

// checkMissionCompleteMessageDispTiming__11Komono3TaskFv (0x9ED8C0)
void Komono3Logic::checkMissionCompleteMessageDispTiming(Komono3State& s, const KomonoEnv& env, KomonoHost& h) {
    if (s.phase == 0 && checkCamPath(env, s.msgPath, s.msgFrame)) {
        h.missionMessage((int8_t)s.mission);                     // func_A10B00(lb 0x5A0)
        s.phase = 1; s.endState = 1; s.state = 2;
    }
}

// checkCloseTiming_clrBranchNo__11Komono3TaskFv (0x9ED920)
void Komono3Logic::checkCloseTiming_clrBranchNo(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    if (checkCamPath(env, s.closePath, s.closeFrame)) { clrBranchNo(env); h.closeTask(); }
}

// lightCtrl__11Komono3TaskFv: copia parametros de luz al motor (omitido) y gira la luz 0x200 por fotograma con el juego en marcha.
void Komono3Logic::lightCtrl(Komono3State& s, const KomonoEnv& env) {
    if (env.pauseLevel <= 0) s.lightAngle += 0x200;
}

// orgCtrl__11Komono3TaskFv (0x9ED970)
void Komono3Logic::orgCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    checkColli_setBranchNo(s, env, h);
    lightCtrl(s, env);                                           // (func_1F0990/orgPut/func_1F0AD0: dibujo omitido)
}
void Komono3Logic::broochCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) { orgCtrl(s, env, h); }

// silverGrassCtrl__11Komono3TaskFv
void Komono3Logic::silverGrassCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    int skip = 0;                                                // el golpe ya no cuenta al final del recorrido (camara 0x228, frame >= 550)
    if (s.state == 1 && env.camPathId == 0x228 && !(env.camFrame < 550.0f)) skip = 1;
    if (!skip) setColli(s, s.pos, h);
    int pl = 0;
    switch (s.state) {
        case 0:
            if (checkHit(s, h, &pl)) { h.orgHitCollect(pl, s.pos); setBranchNo(env); s.state = 1; sound(h, 0xB012B); }
            break;
        case 1:
            if (!skip && checkHit(s, h, &pl)) {
                float eff[6]; calcEffPos(s, env, h, eff);
                h.orgHitAgain(pl, eff);
                sound(h, 0x20025, 0x1000);
            }
            break;
        default: break;
    }
    lightCtrl(s, env);
}

// goldenBornCtrl__11Komono3TaskFv
void Komono3Logic::goldenBornCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    setColli(s, s.pos, h);
    int pl = 0;
    switch (s.state) {
        case 0:
            if (checkHit(s, h, &pl)) {
                h.orgHitCollect(pl, s.pos);
                float in[3] = {s.colliPos[0], 1.0f + s.colliPos[1], 2.0f + s.colliPos[2]};
                float e[3]; h.camTransform(in, e);
                // tres rafagas de chispas (escala 1.0 codigo 5 y 0.8 codigo 0x12), desplazadas en x/y
                h.effect(1.0f, 5, e); h.effect(0.8f, 0x12, e);
                e[0] += 2.0f; e[1] -= 0.5f;
                h.effect(1.0f, 5, e); h.effect(0.8f, 0x12, e);
                e[0] -= 3.0f; e[1] += 1.0f;
                h.effect(1.0f, 5, e); h.effect(0.8f, 0x12, e);
                setBranchNo(env);
                s.state = 1;
                sound(h, 0xB012B);
                sound(h, 0xF020C);
            }
            break;
        case 1:
            if (checkHit(s, h, &pl)) {
                float eff[6]; calcEffPos(s, env, h, eff);
                h.orgHitAgain(pl, eff);
                sound(h, 0x20025, 0x1000);
            }
            break;
        default: break;
    }
    if (s.state != 0) lightCtrl(s, env);
    if (s.state == 0) h.goggleEffect(s.pos);                      // OrgDispGoggleEffect
}

// goldenRatCtrl__11Komono3TaskFv (0x9EE040)
void Komono3Logic::goldenRatCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    checkColli_setBranchNo(s, env, h);
    float p[3]; int32_t r[3];
    h.animSample(s.animTime, 0x216, p, r);                       // func_1CC710(t, gp-0x7310, 0x216, out)
    s.animPos[0] = -10.3f + p[0];                                // 0xC124CCCD
    s.animPos[1] = -149.0f + p[1];                               // 0xC3150000
    s.animPos[2] = -1919.0f + p[2];                              // 0xC4EFE000
    s.animRot[0] = r[0];
    s.animRot[1] = r[1] + 0x7FFF + 1;
    s.animRot[2] = r[2];
    for (int k = 0; k < 3; k++) s.pos[k] = s.animPos[k];
    switch (s.animState) {
        case 2: break;
        case 0:
            if (env.pauseLevel <= 0) {
                s.animTime = s.animTime + 1.0f;
                if (((int)s.animTime % 90) == 0x3C) sound(h, 0x190110);
                if (!(s.animTime <= 340.0f)) s.animTime = 0;     // 0x43AA0000
                s.animFrame = (s.animFrame + 1) % 15;
            }
            if (s.state == 1) s.animState = 1;
            break;
        case 1:
            s.animTime = s.animTime + 1.0f;
            s.animFrame = (s.animFrame + 1) % 15;
            if (!(s.animTime <= 340.0f)) { s.animTime = 0; s.animState = 2; s.animFrame = 0; }
            if (env.camPathId == 0x23F && !(env.camFrame < 65.0f)) { s.animTime = 0; s.animState = 2; s.animFrame = 0; }
            break;
        default: break;
    }
    if (s.state == 0) h.goggleEffect(s.animPos);
}

// goldenFishCtrl__11Komono3TaskFv
void Komono3Logic::goldenFishCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    if (checkCamPath(env, s.closePath, s.closeFrame)) { clrBranchNo(env); h.closeTask(); }
    checkColli_setBranchNo(s, env, h);
    if (s.animState == 0) {
        float p[3]; int32_t r[3];
        h.animSample(s.animTime, 0x280, p, r);
        s.animPos[0] = s.basePos[0] + p[0];
        s.animPos[1] = s.basePos[1] + p[1];
        s.animPos[2] = s.basePos[2] + p[2];
        s.animRot[0] = s.baseRot[0] + r[0];
        s.animRot[1] = s.baseRot[1] + r[1];
        s.animRot[2] = s.baseRot[2] + r[2];
        if (env.pauseLevel <= 0) {
            s.animTime = s.animTime + 1.0f;
            if (s.animTime == 72.0f || s.animTime == 261.0f) sound(h, 0x1C010C);
            if (s.animTime == 213.0f || s.animTime == 403.0f) sound(h, 0x1C010D);
            if (!(s.animTime <= 420.0f)) s.animTime = 0;         // 0x43D20000
        }
    }
    for (int k = 0; k < 3; k++) s.pos[k] = s.animPos[k];
    s.animFrame = (s.animFrame + 1) % 37;
    if (s.animState == 0 && s.state == 1 && s.animPos[1] <= -12.7f) { s.animState = 1; s.animTime = 0; }
    if (s.state == 0) h.goggleEffect(s.animPos);
}

// silverKinokoCtrl__11Komono3TaskFv
void Komono3Logic::silverKinokoCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    switch (s.endState) {
        case 1: if (env.msgPending == 0) s.endState = 2; break;
        case 2:
            if (env.pauseLevel <= 0) {
                s.alpha = s.alpha - 0.033333335f;
                if (s.alpha < 0.0f) { s.alpha = 0.0f; clrBranchNo(env); h.closeTask(); }
            }
            break;
        default: break;
    }
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    checkColli_setBranchNo(s, env, h);
    if (s.state == 0) h.goggleEffect(s.pos);
    if (env.pauseLevel <= 0) s.pulse = (s.pulse + 0x200) & 0xFFFF;
}
}}  // namespace vn::komono

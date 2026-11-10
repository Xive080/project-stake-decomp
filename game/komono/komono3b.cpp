// Komono3Task: objetos Org adicionales y objetos de fase (ver komono.h). Cada funcion cita la original (asm/aomod/aomod_text.s).
#pragma STDC FP_CONTRACT OFF   // la PS2 no tiene FMA: sin contraccion de a*b+c
#include "komono.h"
namespace vn { namespace komono {

static void snd(KomonoHost& h, int id, int a2 = 0) { h.sound(id, a2, 0); }
// setColli__11Komono3TaskF11tagNLpoint3 (0x9ED510)
static void setColli(Komono3State& s, const float p[3], KomonoHost& h) { h.registerCollider(p, s.colliPos); }

// makimonoCtrl__11Komono3TaskFv: pergamino; al recogerlo avanza su animacion 0x257 hasta el frame 120.
void Komono3Logic::makimonoCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    if (checkCamPath(env, s.closePath, s.closeFrame)) { clrBranchNo(env); h.closeTask(); }
    checkColli_setBranchNo(s, env, h);
    if (s.animState == 0) {
        float p[3]; int32_t r[3];
        h.animSample(s.animTime, 0x257, p, r);
        for (int k = 0; k < 3; k++) { s.animPos[k] = p[k]; s.animRot[k] = r[k]; }
    }
    if (s.state != 0 && env.pauseLevel <= 0) {
        s.animTime = s.animTime + 1.0f;
        if (!(s.animTime <= 120.0f)) { s.animTime = 120.0f; s.animState = 1; }
    }
    for (int k = 0; k < 3; k++) s.pos[k] = s.animPos[k];
    if (s.state == 0) h.goggleEffect(s.pos);
    lightCtrl(s, env);
}

// urokoCtrlA__11Komono3TaskFv: escama "A". Se cierra en su camara (campos +0x1CC/+0x1D0 reutilizados como camara/frame), cuenta la recogida.
void Komono3Logic::urokoCtrlA(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    if (checkCamPath(env, s.phase, s.msgPath)) h.closeTask();
    setColli(s, s.pos, h);
    int pl = 0;
    switch (s.state) {
        case 0:
            if (checkHit(s, h, &pl)) {
                h.orgHitCollect(pl, s.pos);
                s.state = 1;
                env.urokoCount += 1;                              // gp-0x6F00
                snd(h, 0xB012B);
            }
            break;
        case 1:
            if (env.pauseLevel <= 0) {
                s.alpha = s.alpha - 0.033333335f;
                if (s.alpha < 0.0f) { s.alpha = 0.0f; h.closeTask(); }
            }
            break;
        default: break;
    }
    if (s.state == 0) h.goggleEffect(s.pos);
    lightCtrl(s, env);
}

// urokoCtrlB__11Komono3TaskFv: escama "B" (la ultima); a la quinta recogida activa la rama.
void Komono3Logic::urokoCtrlB(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    setColli(s, s.pos, h);
    int pl = 0;
    switch (s.state) {
        case 0:
            if (checkHit(s, h, &pl)) {
                env.urokoCount += 1;
                if (env.urokoCount == 5) setBranchNo(env);
                h.orgHitCollect(pl, s.pos);
                s.state = 1;
                snd(h, 0xB012B);
            }
            break;
        case 1:
            if (checkHit(s, h, &pl)) {
                float eff[6]; calcEffPos(s, env, h, eff);
                h.orgHitAgain(pl, eff);
                snd(h, 0x20025, 0x1000);
            }
            break;
        default: break;
    }
    if (s.state == 0) h.goggleEffect(s.pos);
    lightCtrl(s, env);
}

// hitotsuCtrl__11Komono3TaskFv: colisionador a pos.y + 17.5; el temporizador (+0x274, +1.2 por fotograma) dispara sonidos.
void Komono3Logic::hitotsuCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    float p[3] = {s.pos[0], s.pos[1] + 17.5f, s.pos[2]};
    setColli(s, p, h);
    if (s.state == 0) h.goggleEffect(p);
    int pl = 0;
    switch (s.state) {
        case 0:
            if (checkHit(s, h, &pl)) { h.orgHitCollect(pl, p); setBranchNo(env); s.state = 1; snd(h, 0xB012B); }
            break;
        case 1:
            if (checkHit(s, h, &pl)) {
                float eff[6]; calcEffPos(s, env, h, eff);
                h.orgHitAgain(pl, eff);
                snd(h, 0x20025, 0x1000);
            }
            break;
        default: break;
    }
    switch (s.sub204) {
        case 0:
            if (checkCamPath(env, 0x28D, 0)) s.sub204 = 1;
            break;
        case 1:
            if (env.pauseLevel <= 0) {
                float t = s.animTime + 1.2f;
                s.animTime = t;
                // los limites superiores 561.2/261.2/461.2 no son constantes del original: se calculan como A + 1.2f (en coma flotante)
                bool a = (t >= 1.0f && t < 2.2f) || (t >= 560.0f && t < 560.0f + 1.2f);
                if (a) snd(h, 0x1C010E);
                t = s.animTime;
                bool b = (t >= 260.0f && t < 260.0f + 1.2f) || (t >= 460.0f && t < 460.0f + 1.2f);
                if (b) snd(h, 0x1C010F);
                if (!(s.animTime <= 745.0f)) s.animTime = 745.0f;
            }
            break;
        default: break;
    }
    if (s.sub204 > 0) lightCtrl(s, env);
}

// kinokoCtrl__11Komono3TaskFv: cadena Org comun + gafas en posicion fija (D_00A09DB4 = 62.6, 3.7, -4121.3).
void Komono3Logic::kinokoCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    checkCloseTiming_clrBranchNo(s, env, h);
    checkColli_setBranchNo(s, env, h);
    if (s.state == 0) { const float g[3] = {62.599998474121094f, 3.700000047683716f, -4121.2998046875f}; h.goggleEffect(g); }
    lightCtrl(s, env);
}

// nutsCtrl__11Komono3TaskFv: nueces; al golpe recoge en la posicion global de la nuez; sonidos a los frames 56 y 61.
void Komono3Logic::nutsCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) {
    checkGameMode(s, env, h);
    checkMissionEnd_setTrnsl(s, env);
    checkMissionCompleteMessageDispTiming(s, env, h);
    if (checkCamPath(env, s.closePath, s.closeFrame)) { clrBranchNo(env); h.closeTask(); }
    h.registerColliderAlt();
    int pl = 0;
    if (s.state == 0) {
        if (checkHit(s, h, &pl)) {
            float p[3] = {env.nutsPos[0], env.nutsPos[1], env.nutsPos[2]};
            h.orgHitCollect(pl, p);
            setBranchNo(env);
            s.state = 1;
            snd(h, 0xC000E);
            snd(h, 0xB012B);
        }
    }
    if (s.animState == 0) {
        float p[3]; int32_t r[3];
        h.animSample(s.animTime, 0x164, p, r);                    // -> +0x2E8
        h.animSample(s.animTime, 0x165, p, r);                    // -> +0x300
    }
    if (env.pauseLevel <= 0 && s.state == 1) {
        s.animTime = s.animTime + 1.0f;
        float t = s.animTime;
        if (t >= 56.0f && t < 57.2f) snd(h, 0xC000F);
        t = s.animTime;
        if (t >= 61.0f && t < 62.2f) snd(h, 0xC000F);
        if (!(s.animTime <= 90.0f)) { s.animTime = 90.0f; s.animState = 1; }
    }
    if (s.state == 0) h.goggleEffect(s.animPos);
}

// maboroshiCtrl__11Komono3TaskFv: igual que orgCtrl; despues dibuja dos objetos fijos (omitido).
void Komono3Logic::maboroshiCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h) { orgCtrl(s, env, h); }

// setDynaParam__11Komono3TaskFii (0x9EA4B0) / setDynaParam__11Komono2TaskFii: handle[idx] = dynaFind(id); frames[idx] = (s16)tabla[id].
void DynaLogic::setDynaParam(DynaSlots& d, int idx, int id, const KomonoEnv& env, KomonoHost& h) {
    d.handle[idx] = h.dynaFind(id);
    d.frames[idx] = env.dynaFrames ? (int)env.dynaFrames[id] : 0;
}

// checkCloseCamPath__11Komono2TaskFif
void DynaLogic::checkCloseCamPath(const KomonoEnv& env, int path, float frame, KomonoHost& h) {
    if (env.camPathId == path && !(env.camFrame < frame)) h.closeTask();
}

// ---------------------------------------------------------------------------------------------------------------------
// st2NakaDoorCtrl__11Komono3TaskFv: cierra la tarea en las camaras 0x1A8, 0x1AF y 0x187 (el resto es dibujo de la animacion).
void StageObjLogic::nakaDoorCtrl(const KomonoEnv& env, KomonoHost& h) {
    int p = (int16_t)env.camPathId;
    if (p == 0x1A8 || p == 0x1AF || p == 0x187) h.closeTask();
}

// st2NakaDoorCtrlPost__11Komono3TaskFv
void StageObjLogic::nakaDoorCtrlPost(NakaDoorState& s, KomonoEnv& env, KomonoHost& h) {
    if ((int16_t)env.camPathId == 0x1D0 && s.sub == 1) { env.branchNo = 1; s.sub = 2; }
    switch (s.phase) {
        case 0:
            if (s.flags & 8) {
                s.flags &= ~8u;
                s.phase = 1; s.sub = 1;
                h.sound(0xD0012, 0, 0);
            }
            {
                float p[3] = {s.pos[0] + s.off[0], s.pos[1] + s.off[1], s.pos[2] + s.off[2]};
                h.registerCollider(p, s.colliPos);
            }
            s.tA = s.tA + 0.2f;
            if (!(s.tA <= (float)s.lenA)) s.tA = 0.0f;
            break;
        case 1:
            s.tB = s.tB + 0.2f;
            if (!(s.tB <= ((float)s.lenB - 6.0f)) && s.soundDone == 0) { h.sound(0xD0016, 0, 0); s.soundDone = 1; }
            if (!(s.tB <= (float)s.lenB)) s.tB = (float)s.lenB;
            break;
        default: break;
    }
}

// st3bs01KnightBunkiCtrlPre__11Komono3TaskFv: en las camaras 0x230/0x20A cierra los dos colisionadores y la tarea.
void StageObjLogic::knightCtrlPre(KnightState& s, const KomonoEnv& env, KomonoHost& h) {
    int p = (int16_t)env.camPathId;
    if (p == 0x230 || p == 0x20A) {
        for (int i = 0; i < 2; i++) if (s.colli[i]) ColliLogic::close(*s.colli[i]);
        h.closeTask();
    }
}

// st3bs01KnightBunkiCtrl__11Komono3TaskFv: un disparo a cada colisionador da 10 puntos, lo destruye con pedazos (pieza 0x11) y marca la fase.
void StageObjLogic::knightCtrl(KnightState& s, KomonoEnv& env, KomonoHost& h) {
    for (int i = 0; i < 2; i++) {
        if (s.colli[i] && ColliLogic::chkColli(*s.colli[i])) {
            int pl = ColliLogic::getHitPl(*s.colli[i], h);
            h.addScore(pl, 10);
            h.addHitNum(pl);
            h.sound(0x17021C, 0, 0);
            ColliLogic::clrColli(*s.colli[i]);
            ColliLogic::close(*s.colli[i]);
            s.colli[i] = nullptr;
            if (s.phase == 0 && i == 0) s.first = 1;
            s.phase = 1;
            h.breakEffectA(s.pos[i], s.rot[i][1], 0x11);
        }
    }
    for (int i = 0; i < 2; i++) if (s.colli[i]) ColliLogic::setPos(*s.colli[i], s.pos[i][0], s.pos[i][1], s.pos[i][2]);
    if (s.phase == 1 && s.sub == 0 && (int16_t)env.camPathId == 0x208) { s.sub = 1; env.branchNo = (int8_t)s.first; }
}
}}  // namespace vn::komono

// EnEtcThrowObj (GAMEMOD 0x93D2F0..0x941xxx): objetos lanzados al jugador. Reglas reescritas a mano; ver enemies3.h.
#include "enemies3.h"
namespace vn { namespace en3 {

static const float kNearZ = -1.5f;   // 0xBFC00000: plano de destino en camara

// Borra el objeto sin marcarlo muerto (Ctrl2_Pre): libera su sitio en el duenyo y cierra la tarea.
static void deleteObj(ThrowObj& o, ThrowOwner& owner, Ev3s& out, bool markDead) {
    if (!(o.flags & F_DEAD)) owner.live = (int16_t)(owner.live - 1);
    if (markDead) o.flags |= F_DEAD;
    emit3(out, E3_TASK_CLOSE);
}
// Destino aleatorio en pantalla (Init2/6/9): x en [-239, 239], y en [-167, 167] (pixeles de la pantalla de 640x448).
static void randomTarget(ThrowObj& o, Env3& env) {
    float x = (float)(env.rand() % 240);
    float y = (float)(env.rand() % 168);
    if ((env.rand() % 2) == 0) x *= -1.0f;
    if ((env.rand() % 2) == 0) y *= -1.0f;
    V3 v; v.x = (kNearZ * x) / env.focal; v.y = (kNearZ * y) / env.focal; v.z = kNearZ;
    o.target = env.cam->viewToWorld(v);
}

// ---------------------------------------------------------------------------------------------------------------------
// EnEtcThrowObj1Init 0x93DB60: tipo 1 (cuchillo que va al jugador `side`).
void throwInit1(ThrowObj& o, uint32_t side, int32_t timer, ThrowOwner& owner, bool gate, Env3& env, Ev3s& out) {
    o.coop = env.coop;
    V3 v;
    if (o.coop == 1) { v.x = 0; v.y = 0; v.z = kNearZ; }
    else {
        float px = (side == 0) ? 160.0f : -160.0f;
        v.x = (kNearZ * px) / env.focal; v.y = -0.0f / env.focal; v.z = kNearZ;
    }
    o.target = env.cam->viewToWorld(v);
    o.timer = timer; o.total = o.timer;
    o.scale = 0.2f;                                   // 0x3E4CCCCD
    o.animStart = 0x1B008D; o.animEnd = 0x1B0096;
    emit3(out, E3_SOUND, 0xE040F);
    o.radius = 0.5f;
    owner.flags2 |= 0x20000000;
    o.animCur = o.animStart;
    o.flags = 0x10001;
    if (gate) { o.flags |= F_GATE; env.attackGate = 1; }
    o.alpha = 1.0f; o.state = 0;
    emit3(out, E3_ALLOC_BUF, 0x1000, 0x11);           // buffer de 17 partes (0xF0 B)
    o.hasBuf = true;
    o.animT = 0;
}
// EnEtcThrowObj2Init 0x93EBC0: objetos 2 (por tipo +0x19C: defecto / 5 / 8 / 9).
void throwInit2(ThrowObj& o, int32_t timer, Env3& env) {
    randomTarget(o, env);
    o.timer = timer; o.total = o.timer;
    switch (o.type) {
        case 5: o.scale = 0.7f;  o.animStart = 0x290000; o.animEnd = 0x290000; o.radius = 3.0f; break;
        case 8: o.scale = 0.1f;  o.animStart = 0x29001E; o.animEnd = 0x29001E; o.radius = 2.0f; break;
        case 9: o.scale = 0.5f;  o.animStart = 0x2900F1; o.animEnd = 0x2900F1; o.radius = 3.0f; break;
        default: o.scale = 0.1f; o.animStart = 0x260000; o.animEnd = 0x26001F; o.radius = 1.2f; break;
    }
    o.animCur = o.animStart;
    o.flags = 0x10001; o.alpha = 1.0f; o.state = 0;
}
// EnEtcThrowObj3Init 0x93F160
void throwInit3(ThrowObj& o) {
    o.animStart = 0x260067; o.animEnd = 0x260086;
    o.timer = 0;
    o.animCur = o.animStart;
    o.flags = 0x10001;
    o.alpha = 0.0f; o.w16C = 0; o.state = 0;
}
// EnEtcThrowObj5Init 0x93FDC0 / 7Init 0x941480: igual salvo la base del temporizador (0x50 / 0xA4); la 7 ademas suena 0x100019.
static void init57(ThrowObj& o, uint32_t n, int32_t total, uint32_t base, Ev3s& out) {
    o.timer = total; o.total = o.timer;
    o.scale = 0.0f;
    o.animStart = 0x260000; o.animEnd = 0x26001F;
    o.radius = 1.7f;                                   // 0x3FD9999A
    o.timer = (int32_t)((((uint32_t)(n * 5u) + base) * 0x32u) / 60u);   // multu 0x88888889 >> 5: division SIN signo
    o.scaleStep = 0.5f / (float)o.timer;
    o.animCur = o.animStart;
    o.flags = 0x10001; o.alpha = 1.0f; o.state = 0;
    emit3(out, E3_ALLOC_BUF, 0x790, 8);               // 8 partes; la parte 3 usa la animacion 0x280015
    o.hasBuf = true;
    o.animT = 0;
}
void throwInit5(ThrowObj& o, uint32_t n, int32_t total, Ev3s& out) { init57(o, n, total, 0x50, out); }
void throwInit7(ThrowObj& o, uint32_t n, int32_t total, Ev3s& out) { init57(o, n, total, 0xA4, out); emit3(out, E3_SOUND, 0x100019); }
// EnEtcThrowObj6Init 0x9408A0
void throwInit6(ThrowObj& o, int32_t timer, Env3& env) {
    randomTarget(o, env);
    o.timer = timer; o.total = o.timer;
    o.scale = 1.0f;
    o.animStart = 0x800002; o.animEnd = 0x800002;
    o.radius = 1.8f;                                   // 0x3FE66666
    o.rotY = 0;
    o.rotSpeed = (o.type != 1) ? 0x1000 : -0x1000;
    o.animCur = o.animStart;
    o.flags = 0x10001; o.alpha = 1.0f; o.state = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Ctrl_Pre 0x93D2F0: tipo 1 (vuela en linea recta hasta el jugador; golpea al llegar).
void throwCtrlPre1(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out) {
    if (!(o.flags & F_DEAD) && (owner.flags2 & 0x20)) o.flags |= F_DEAD;
    if (!(o.flags & F_NOCOLLI) && (o.flags & F_HIT)) {   // derribado a tiros
        emit3(out, E3_SOUND, 0xE0411);
        o.flags |= F_NOCOLLI; o.flags |= F_FROZEN_ANIM;
        if (o.flags & F_PLAYER0) emit3(out, E3_HITNUM, 0);
        if (o.flags & F_PLAYER1) emit3(out, E3_HITNUM, 1);
        o.alpha = 0.5f; o.state = 5;
    }
    int s = o.state;
    if (s == 0) {
        o.pos.x += (o.target.x - o.startPos.x) / (float)o.total;
        o.pos.y += (o.target.y - o.startPos.y) / (float)o.total;
        o.pos.z += (o.target.z - o.startPos.z) / (float)o.total;
        o.timer -= 1;
        if (o.timer < 0) { o.state = ++s; } else return;
    }
    if (s == 1) {                                          // llega: hiere al jugador
        emit3(out, E3_DAMAGE_PLAYER, o.player, 1, 7);
        o.flags |= F_HITTED; o.flags |= F_NOCOLLI;
        o.timer = 0xF;
        o.state = ++s;
    }
    if (s == 2) {                                          // se desvanece
        o.alpha -= 0.04f;
        o.timer -= 1;
        if (o.timer <= 0) { o.flags |= F_FROZEN_ANIM; o.timer = 0; }
        if (o.alpha <= 0.0f) {
            env.attackSlot[o.player] = 0;
            if (!(o.flags & F_DEAD)) owner.flags2 &= ~0x20000000u;
            if (o.flags & F_GATE) env.attackGate = 0;
            if (o.hasBuf) emit3(out, E3_FREE_BUF);
            emit3(out, E3_TASK_CLOSE);
        }
    } else if (s == 5) {                                   // derribado: se encoge
        if (!(o.flags & F_DEAD)) owner.flags2 &= ~0x20000000u;
        o.alpha -= 0.033333335f;
        o.scale += 0.06666667f;
        if (o.alpha <= 0.0f) {
            env.attackSlot[o.player] = 0;
            if (o.flags & F_GATE) env.attackGate = 0;
            if (o.hasBuf) emit3(out, E3_FREE_BUF);
            emit3(out, E3_TASK_CLOSE);
        }
    }
}

// Cuerpo comun de las fases 0..2 de Ctrl2_Pre y Ctrl6_Pre. `missInc` = Ctrl6 (suma un fallo al golpear), `markDead` = Ctrl6.
static void flyAndHit(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out, bool missInc, bool markDead) {
    int s = o.state;
    float sx2 = 0.0f;
    if (s == 0) {
        const Camera3& cam = *env.cam;
        V3 a = cam.worldToView(o.pos);                      // posicion ANTES de moverse (sp4A0/sp480)
        V3 d; d.x = o.target.x - o.pos.x; d.y = o.target.y - o.pos.y; d.z = o.target.z - o.pos.z;
        d = cam.normalize(d);
        float k = (float)o.total / 10.0f;
        o.vel.x = d.x * k; o.vel.y = d.y * k; o.vel.z = d.z * k;
        o.pos.x += o.vel.x; o.pos.y += o.vel.y; o.pos.z += o.vel.z;
        V3 b = cam.worldToView(o.pos);                      // posicion DESPUES (sp4C0/sp4A0)
        float fx = a.x * env.focal, fy = a.y * env.focal;
        float sx = fx / a.z, sy = fy / a.z;
        if (!(sx >= -320.0f) || sx > 320.0f || !(sy >= -224.0f) || sy > 224.0f) { deleteObj(o, owner, out, markDead); return; }
        float thr = kNearZ - (o.radius * o.scale);
        if (b.z < thr) return;                              // aun lejos
        sx2 = fx / thr; float sy2 = fy / thr;
        if (!(sx2 >= -240.0f) || sx2 > 240.0f || !(sy2 >= -168.0f) || sy2 > 168.0f) { deleteObj(o, owner, out, markDead); return; }
        o.state = ++s;
    }
    if (s == 1) {                                           // llega: hiere al jugador (lado segun la x en pantalla)
        if (env.coop == 1) emit3(out, E3_DAMAGE_PLAYER, env.solo, 1, 7);
        else if (!(sx2 < 0.0f)) emit3(out, E3_DAMAGE_PLAYER, 0, 1, 7);
        else emit3(out, E3_DAMAGE_PLAYER, 1, 1, 7);
        o.flags |= F_HITTED; o.flags |= F_NOCOLLI;
        if (missInc) env.missCount = (int16_t)(env.missCount + 1);
        o.timer = 0xF;
        o.state = ++s;
    }
    if (s == 2) {
        o.alpha -= 0.04f;
        o.timer -= 1;
        if (o.timer <= 0) { o.flags |= F_FROZEN_ANIM; o.timer = 0; }
        if (o.alpha <= 0.0f) deleteObj(o, owner, out, markDead);
    }
}

// Ctrl2_Pre 0x93DE60
void throwCtrlPre2(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out) {
    if (!(o.flags & F_DEAD) && (owner.flags2 & 0x20)) o.flags |= F_DEAD;
    if (!(o.flags & F_NOCOLLI) && (o.flags & F_HIT)) {
        if (o.type == 8) emit3(out, E3_SOUND, 0x1B000E);
        else if (o.type != 5) emit3(out, E3_SOUND, 0x18020F);
        else emit3(out, E3_SOUND, 0x20025);
        o.flags |= F_NOCOLLI; o.flags |= F_FROZEN_ANIM;
        o.animCur = 0x260040; o.animEnd = 0x260056;
        if (o.flags & F_PLAYER0) emit3(out, E3_HITNUM, 0);
        if (o.flags & F_PLAYER1) emit3(out, E3_HITNUM, 1);
        o.state = 5;
        if (o.type == 5) { V3 p = o.pos; emit3(out, E3_EFFECT, 1, 0x30, 0, 0, 0, 0, p); }
        else if (o.type == 8) emit3(out, E3_SPAWN_HIT, 0x39, env.cam->yaw, env.cam->pitch, 0, 0, 0, o.pos);
    }
    if (o.state == 5) {
        if (o.type != 8 && o.type != 5) {
            o.animCur += 1;
            if (o.animCur >= o.animEnd + 1) deleteObj(o, owner, out, false);
        } else deleteObj(o, owner, out, false);
        return;
    }
    flyAndHit(o, owner, env, out, false, false);
}

// Ctrl6_Pre 0x93FFB0
void throwCtrlPre6(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out) {
    if (!(o.flags & F_DEAD) && (owner.flags2 & 0x20)) o.flags |= F_DEAD;
    if (!(o.flags & F_NOCOLLI) && (o.flags & F_HIT)) {
        emit3(out, E3_SPAWN_HIT, 0x3A, env.cam->yaw, env.cam->pitch, 0, 0, 0, o.pos);
        emit3(out, E3_SOUND, 0x100014);
        o.flags |= F_NOCOLLI; o.flags |= F_FROZEN_ANIM;
        o.animCur = 0x800002; o.animEnd = 0x800002;
        if (o.flags & F_PLAYER0) emit3(out, E3_HITNUM, 0);
        if (o.flags & F_PLAYER1) emit3(out, E3_HITNUM, 1);
        o.state = 5;
    }
    if (o.state == 5) { deleteObj(o, owner, out, true); return; }
    flyAndHit(o, owner, env, out, true, true);
}

// ---------------------------------------------------------------------------------------------------------------------
// Ctrl_Pos: animacion, posicion en camara y entrada en la camara de seguimiento.
static void camEntry(ThrowObj& o, const Env3& env, Ev3s& out) {
    o.view = env.cam->worldToView(o.pos);
    if (!(o.flags & F_HITTED)) emit3(out, E3_CAM_ENTRY, 0, 0, 0, 0, 0, 0, o.pos);
    else { V3 n; n.z = kNearZ; emit3(out, E3_CAM_ENTRY, 0, 0, 0, 0, 0, 0, env.cam->viewToWorld(n)); }
}
// Ctrl_Pos 0x93D950: ademas marca el hueco de ataque del jugador y lleva el contador animT (0..10).
bool throwCtrlPos1(ThrowObj& o, Env3& env, Ev3s& out) {
    env.attackSlot[o.player] = 1;                      // gp-0x7420[player] = 1 (el hueco queda ocupado mientras vuela)
    o.animT = o.animT + 1.0f;
    if (!(o.animT <= 10.0f)) o.animT = 0.0f;
    if (!(o.flags & F_FROZEN_ANIM)) { o.animCur += 1; if (o.animCur >= o.animEnd) o.animCur = o.animStart; }
    camEntry(o, env, out);
    return !(o.flags & F_RESPONDED);
}
// Ctrl2_Pos 0x93EA00
bool throwCtrlPos2(ThrowObj& o, const Env3& env, Ev3s& out) {
    if (!(o.flags & F_FROZEN_ANIM)) { o.animCur += 1; if (o.animCur >= o.animEnd) o.animCur = o.animStart; }
    camEntry(o, env, out);
    return !(o.flags & F_RESPONDED);
}
// Ctrl6_Pos 0x9406D0: igual que 2 pero gira (rotY += rotSpeed) y registra la colision si no esta marcado 0x8000.
bool throwCtrlPos6(ThrowObj& o, const Env3& env, Ev3s& out) {
    if (!(o.flags & F_FROZEN_ANIM)) {
        o.animCur += 1; if (o.animCur >= o.animEnd) o.animCur = o.animStart;
        o.rotY += o.rotSpeed;
    }
    camEntry(o, env, out);
    return !(o.flags & F_NOCOLLI);
}

}}  // namespace vn::en3

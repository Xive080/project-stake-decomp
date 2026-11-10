// Clases TrnKomono* del overlay trnmod (misiones de entrenamiento 05, 08 y 10): bolas, fichas-letra y gestores.
// Direcciones virtuales del overlay (base 0x9DF000). Dibujo y colision de motor no se modelan: salen como eventos.
#include "trnkomono.h"
#include <cstring>
namespace vn {

// Constante float a partir de su patron de bits (los inmediatos del asm).
static float bf(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }


// ---------------------------------------------------------------------------------------------------------------------
// CTrn05Komono
// ---------------------------------------------------------------------------------------------------------------------
// init__12CTrn05KomonoFif (0x9EDFB0). Orden de llamadas al azar: [entero si idx == -1], x, y, [z si idx == -1], vy, entero, entero.
void Trn05Komono::init(float speed, int idx, const TrnRand& r, TrnBallOut& o) {
    const bool rnd = (idx == -1);
    if (rnd) idx = r.i() % 5 + 1;
    b.obj = (int)kTrn05Obj[idx];
    b.scale = kTrn05Scale[idx];
    b.pos[0] = 40.0f * r.f() - 20.0f;
    b.pos[1] = 120.0f + 10.0f * r.f();
    if (!rnd) b.pos[2] = 35.5f;                         // 0x420E0000
    else      b.pos[2] = 36.0f + 2.0f * r.f();           // 0x42100000 + 2.0 * rand
    vel[0] = 0;
    vel[1] = (bf(0xBC23D70Au) - bf(0x3D4CCCCDu) * r.f()) - speed;   // 0xBC23D70A - 0.05 * rand - f20
    vel[2] = 0;
    // 0x20 -> 0
    grav = bf(0xBC20902Eu) * kTrn05GravMul[idx];
    frame = 0;
    if (!rnd) { rotSpd[0] = r.i() & 0x10; rotSpd[1] = r.i() & 0x10; }
    else      { rotSpd[0] = r.i() & 0x80; rotSpd[1] = r.i() & 0x200; }
    b.life = 1;
    o.add(TrnBallEvent::COLLI_CLEAR);
    o.add(TrnBallEvent::COLLI_RADIUS, 0, 0, kTrn05Radius[idx]);
}

// ctrl__12CTrn05KomonoFv (0x9EE210)
void Trn05Komono::ctrl(TrnBallSys& s, const TrnRand& r, const TrnTrig& t, TrnBallOut& o) {
    // (dibujo: traslacion, rotaciones, escala b.scale; si obj == 0x10077 se dibuja tambien el modelo 0x10078 + frame)
    o.add(TrnBallEvent::COLLI_POS, 0, 0, b.pos[0], b.pos[1], b.pos[2]);
    if (!s.paused) {
        b.pos[0] += vel[0]; b.pos[1] += vel[1]; b.pos[2] += vel[2];
        vel[1] += grav;
        rot[0] += rotSpd[0]; rot[1] += rotSpd[1];
        frame = (frame + 1) % 15;
    }
    if (b.pos[1] < 50.0f) b.life = 0;                                   // cae por debajo de Y = 50
    if (b.pos[0] < -30.0f || !(b.pos[0] <= 30.0f)) b.life = 0;          // sale por los lados (|x| > 30)
    if (colliHit) {
        colliHit = false;                                                // func_21B4C0 (limpia la colision)
        o.add(TrnBallEvent::COLLI_CLEAR);
        if ((uint32_t)b.obj == 0x69000Cu) {
            // objetivo acertado: efecto en la posicion, un objetivo menos, sonido 0x6000D y desaparece
            o.add(TrnBallEvent::HIT_EFFECT, colliPl, 0, b.pos[0], b.pos[1], b.pos[2]);
            s.norm -= 1;
            o.add(TrnBallEvent::SOUND, 0x6000D, 0x40);
            b.life = 0;
            return;
        }
        // senuelo: rebota con un angulo al azar y cambia su giro
        int ang;
        if (r.f() < 0.5f) ang = (r.i() & 0x3FF) + 0x7E01;
        else              ang = (r.i() & 0x3FF) - 0x1FF;
        vel[0] = vel[0] + 1.5f * t.axisX(ang);
        vel[1] = vel[1] + 1.5f * t.axisY(ang);
        rotSpd[0] = (r.i() & 0xFF) - 0x7F;
        rotSpd[1] = (r.i() & 0x3FF) - 0x1FF;
        s.gp6F5C = 1;                                                   // dispara el temblor del gestor
        o.add(TrnBallEvent::SOUND, 0x6000C, 0x40);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Trn05KomonoMgrTask
// ---------------------------------------------------------------------------------------------------------------------
// Velocidad de caida segun el fotograma (mismo calculo en el bucle de senuelos y en el de objetivos): fotograma >= 0x259 -> 600.0.
static float trn05SpeedParam(int tick) { return tick >= 0x259 ? 600.0f : (float)tick; }
static void trn05HardAdjust(Trn05Komono& k, float f20, const TrnBallSys& s) {
    if (s.level[s.mission] == 2) {                                      // dificultad maxima: desplaza y empuja hacia un lado
        k.addVel(bf(0x3AC49BA6u) * (-f20 / 2.0f), 0.0f, 0.0f);                   // 0x3AC49BA6
        k.addPos(13.0f * (f20 / 600.0f), 0.0f, 0.0f);                    // 0x41500000 / 0x44160000
    }
}

// __ct__18Trn05KomonoMgrTaskFv (0x9EE5A0)
void Trn05Mgr::ctor(const TrnRand& r, TrnBallSys& s, TrnBallOut& o) {
    for (int i = 0; i < 32; i++) {
        if (decoy[i].b.getLife() == 0) {
            if (r.i() % 100 < 5) decoy[i].init(0.0f, -1, r, o);
        }
    }
    for (int i = 0; i < 15; i++) sched[i] = (int16_t)(kTrn05Sched[i] + r.i() % 10);
    tick = 0; next = 0;
    (void)s;
}

// ctrl__18Trn05KomonoMgrTaskFv (0x9EE730)
void Trn05Mgr::ctrl(TrnBallSys& s, const TrnRand& r, const TrnTrig& t, TrnBallOut& o) {
    if (s.gp6F58 == 0) return;
    if (s.missionOver == 1) o.add(TrnBallEvent::CLOSE_TASK);             // func_1A2630
    for (int i = 0; i < 32; i++) {
        Trn05Komono& k = decoy[i];
        if (k.b.getLife() != 0) { k.ctrl(s, r, t, o); continue; }
        if (tick < 0x2BD && r.i() % 100 < 5) {
            float f20 = trn05SpeedParam(tick);
            k.init(bf(0x3AC49BA6u) * f20, -1, r, o);
            trn05HardAdjust(k, f20, s);
        }
    }
    if (tick < 0x2BD) {
        if (!s.paused && next < 15 && sched[next] == tick) {
            for (int i = 0; i < 8; i++) {
                if (target[i].b.getLife() != 0) continue;
                float f20 = trn05SpeedParam(tick);
                target[i].init(bf(0x3AC49BA6u) * f20, 0, r, o);
                trn05HardAdjust(target[i], f20, s);
                next++;
                break;
            }
        }
    }
    for (int i = 0; i < 8; i++) if (target[i].b.getLife() != 0) target[i].ctrl(s, r, t, o);
    if (!s.paused) tick++;
    // temblor de camara
    if (s.gp6F5C != 0) {
        s.gp6F60 = 48.0f;                                               // 0x42400000
        s.gp6F5C = 0;
        o.add(TrnBallEvent::QUAKE, 1, 0x80);                             // func_21AC60(1, 0x80, 0xC)
    }
    if (!s.paused) { s.gp6F60 = s.gp6F60 - 1.0f; if (s.gp6F60 <= 0.0f) s.gp6F60 = 0.0f; }
    else s.gp6F60 = 0.0f;
    if (!(s.gp6F60 <= 0.0f)) {
        s.gp7408 = 0;
        int ang = (int)(1536.0f * s.gp6F60);                            // 0x45C00000 = 1536.0; func_14C518 = fptosi
        float v = t.axisX(ang);                                          // func_1B09B4
        s.gp7404 = (int)(((1.0f / 48.0f) * (24.0f * s.gp6F60)) * v);    // 0x3CAAAAAB, 0x41C00000
    } else {
        o.add(TrnBallEvent::CAM_ATTR, 0xB);                              // atributo +0x9C de la ruta de camara actual = 0xB
        s.gp7404 = 0; s.gp7408 = 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// CTrn08Komono
// ---------------------------------------------------------------------------------------------------------------------
void Trn08Komono::setInterpolate(float x, float y, float z, float sx, float sy, float sz, float dur) {
    b.pos[0] = sx; b.pos[1] = sy; b.pos[2] = sz;
    fin[0] = x; fin[1] = y; fin[2] = z;
    if (!(dur <= 0.0f)) {
        xs = sx; xe = x; xv = 0;
        xa = (x - sx) / ((dur * dur) / 4.0f);
        xdur = dur; xt = dur;
    }
    if (!(dur <= 0.0f)) {
        ys = sy; ye = y; yv = 0;
        ya = (y - sy) / ((dur * dur) / 4.0f);
        ydur = dur; yt = dur;
    }
    interp = 1;           // se pone a 1 aunque dur <= 0 (el delay slot lo fija en ambas ramas)
    elapsed = 0;
    total = dur;
}
void Trn08Komono::init(int obj, float x, float y, float z, float sx, float sy, float sz, float dur, TrnBallOut& o) {
    b.obj = obj;
    b.pos[0] = sx; b.pos[1] = sy; b.pos[2] = sz;
    o.add(TrnBallEvent::COLLI_CLEAR);
    o.add(TrnBallEvent::COLLI_RADIUS, 0, 0, 1.8f);                       // 0x3FE66666
    b.life = 1;
    setInterpolate(x, y, z, sx, sy, sz, dur);
}
bool Trn08Komono::checkColli(TrnBallOut& o) {
    hitPl = -1;
    if (!colliHit) return false;
    colliHit = false;
    hitPl = colliPl;
    o.add(TrnBallEvent::COLLI_CLEAR);
    rotSpd = 0x3200;
    return true;
}
void Trn08Komono::ctrl(const TrnBallSys& s, TrnBallOut& o) {
    o.add(TrnBallEvent::COLLI_POS, 0, 0, b.pos[0], b.pos[1], b.pos[2]);
    if (!s.paused && rotSpd > 0) {
        rot += rotSpd;
        rotSpd -= 0x140;
        if (!(rotSpd > 0)) { rotSpd = 0; rot = 0; }
    }
    if (s.paused || interp == 0) return;
    xt -= 1.0f;
    if (xt < xdur / 2.0f) xv -= xa * 1.0f; else xv += xa * 1.0f;
    xs += xv * 1.0f;
    yt -= 1.0f;
    if (yt < ydur / 2.0f) yv -= ya * 1.0f; else yv += ya * 1.0f;
    ys += yv * 1.0f;
    b.pos[0] = xs; b.pos[1] = ys;
    elapsed += 1.0f;
    if (elapsed < total) return;
    b.pos[0] = fin[0]; b.pos[1] = fin[1]; b.pos[2] = fin[2];
    elapsed = 0; interp = 0;
}

// ---------------------------------------------------------------------------------------------------------------------
// Trn08KomonoMgrTask
// ---------------------------------------------------------------------------------------------------------------------
// __ct__18Trn08KomonoMgrTaskFv (0x9EF9E0)
void Trn08Mgr::ctor(int level, const TrnRand& r, TrnBallOut& o) {
    for (int i = 0; i < 29; i++) {
        const Trn08TileHome& h = kTrn08Home[i];
        float sx = h.x + (float)(r.i() % 60) - 30.0f;                    // 0x41F00000 = 30.0
        float sy = h.y + (float)(r.i() % 60) - 30.0f;
        tile[i].init((int)h.obj, h.x, h.y, h.z, sx, sy, h.z, (float)(0x32 + (i * 4) % 20), o);
    }
    typedCount = 0;
    for (int i = 0; i < 16; i++) typed[i] = 0;
    wordIdx = 0; state = 2; counter = 0; timer = 30.0f; frame = 0; spinAngle = 0; spinVel = 0;
    for (int w = 0; w < 5; w++) {
        int len = kTrn08WordLen[level][w];
        wordLen[w] = len;
        for (int i = 0; i < len; i++) word[w][i] = kTrn08Home[r.i() % 29].obj;
        if (TrainingLogic::checkBadWord(word[w], len)) {
            // sustituto fijo para una palabra prohibida
            if (len == 3)      { word[w][1] = 0x6C002C; word[w][2] = 0x6C0029; }
            else if (len == 2) { word[w][1] = 0x6C002C; }
            else if (len == 1) { word[w][0] = 0x6C000A; }
        }
    }
    for (int w = 0; w < 5; w++) {
        // las 3 primeras letras parten de (-6.0, Y, Z) = (0xC0C00000, 0xC0E051CA, 0x3E04FF86)
        for (int j = 0; j < 3; j++) { wordPos[w][j][0] = bf(0xC0C00000u); wordPos[w][j][1] = bf(0xC0E051CAu); wordPos[w][j][2] = bf(0x3E04FF86u); }
        int len = wordLen[w];
        if (len == 3)      { wordPos[w][0][0] = bf(0x41C9999Au); wordPos[w][1][0] = bf(0x41F9999Au); wordPos[w][2][0] = bf(0x4214CCCDu); }
        else if (len == 2) { wordPos[w][0][0] = bf(0x41E1999Au); wordPos[w][1][0] = bf(0x4208CCCDu); }
        else if (len == 1) { wordPos[w][0][0] = bf(0x41F9999Au); }
    }
}

// ctrl__18Trn08KomonoMgrTaskFv (0x9F00D0), parte logica.
void Trn08Mgr::ctrl(TrnBallSys& s, const TrnRand& r, TrnBallOut& o) {
    for (int i = 0; i < 29; i++) tile[i].ctrl(s, o);
    if (s.gp6F54 == 0) return;
    if (state == 0 && wordIdx < 5) {
        for (int i = 0; i < 29; i++) {
            if (!tile[i].checkColli(o)) continue;
            if (wordIdx >= 5) continue;       // el original leeria fuera de la tabla de palabras; aqui se ignora
            uint32_t expect = word[wordIdx][typedCount];
            if ((uint32_t)tile[i].b.getObj() != expect) {              // letra equivocada
                o.add(TrnBallEvent::QUAKE, 1, 0x80);
                o.add(TrnBallEvent::SOUND, 0x6000C, 0x40);
                continue;
            }
            typed[typedCount] = (uint32_t)tile[i].b.getObj();
            typedCount++;
            int pl = tile[i].getHitPl();
            const float* lp = wordPos[wordIdx][typedCount - 1];
            if (typedCount < wordLen[wordIdx]) {                       // letra correcta, falta la palabra
                o.add(TrnBallEvent::LETTER_EFFECT, pl, 0, lp[0], lp[1], lp[2]);                       // efecto en el hueco de la letra
                o.add(TrnBallEvent::LETTER_EFFECT, pl, 2, tile[i].b.pos[0], tile[i].b.pos[1], tile[i].b.pos[2]);   // y en la ficha
                o.add(TrnBallEvent::SOUND, 0x2002E, 0x40);
            } else if (typedCount == wordLen[wordIdx]) {               // palabra completa
                o.add(TrnBallEvent::LETTER_EFFECT, pl, 1, lp[0], lp[1], lp[2]);
                o.add(TrnBallEvent::LETTER_EFFECT, pl, 2, tile[i].b.pos[0], tile[i].b.pos[1], tile[i].b.pos[2]);
                wordIdx++; typedCount = 0; state = 3; spinAngle = 0; spinVel = 0x3200;
                s.norm -= 1;
                if (!(s.norm > 0)) s.missionOver = 1;                  // 0x500E32
                o.add(TrnBallEvent::SOUND, 0x1F0014, 0x40);
                if (s.norm == 0) {
                    // todo hecho: las fichas se dispersan (de su sitio a un punto al azar en 30 fotogramas)
                    for (int j = 0; j < 29; j++) {
                        const Trn08TileHome& h = kTrn08Home[j];
                        float tx = h.x + (float)(r.i() % 60) - 30.0f;
                        float ty = h.y + (float)(r.i() % 60) - 30.0f;
                        tile[j].setInterpolate(tx, ty, h.z, h.x, h.y, h.z, 30.0f);
                    }
                }
            }
        }
    }
    if (!s.paused) {
        if (state == 3) {
            if (spinVel > 0) {
                spinAngle += spinVel;
                spinVel = (int)((float)spinVel - 384.0f);              // 0x43C00000; func_14C518 = fptosi
                if (!(spinVel > 0)) { spinVel = 0; spinAngle = 0; }
            }
            counter++;
            if (counter >= 9) { state = 2; counter = 0; spinVel = 0; spinAngle = 0; }
        } else if (state == 2) {
            timer = 30.0f; state = 1;
            o.add(TrnBallEvent::SOUND, 0x20027, 0x40);
        } else if (state == 1) {
            if (wordIdx < 5) {
                for (int j = 0; j < 3; j++) wordPos[wordIdx][j][0] -= bf(0x4019999Au);      // 0x4019999A
            }
            if (wordIdx > 0) {
                for (int j = 0; j < 3; j++) wordPos[wordIdx - 1][j][0] -= bf(0x4019999Au);
            }
            timer -= bf(0x4019999Au);
            if (timer <= 0.0f) {
                state = 0; timer = 0.0f;
                for (int i = 0; i < 29; i++) tile[i].clrColli(o);
            }
        }
    }
    frame++;
}

// ---------------------------------------------------------------------------------------------------------------------
// CTrn10Komono / Trn10KomonoMgrTask
// ---------------------------------------------------------------------------------------------------------------------
void Trn10KomonoFull::init(int objId, float x, float y, float z, TrnBallOut& o) {
    obj = objId; k.x = x; k.y = y; k.z = z;
    o.add(TrnBallEvent::COLLI_CLEAR);
    o.add(TrnBallEvent::COLLI_RADIUS, 0, 0, 15.0f);                      // 0x41700000
    life = 1;
}
bool Trn10KomonoFull::checkColli(const int* playerAttr, TrnBallOut& o) {
    k.hitPl = -1;
    if (hasCollider && colliHit) {
        colliHit = false;
        k.hitPl = colliPl;
        hitAttr = playerAttr[colliPl];
        o.add(TrnBallEvent::COLLI_CLEAR);
        return true;
    }
    hitAttr = 0;
    return false;
}
void Trn10KomonoFull::ctrl(bool paused, TrnBallOut& o) {
    if (k.mode == 0) {
        // ctrl1: solo coloca el colisionador en (x + offX, y, z + offZ) y dibuja
        if (hasCollider) o.add(TrnBallEvent::COLLI_POS, 0, 0, k.x + k.offX, k.y, k.z + k.offZ);
    } else if (k.mode == 1) {
        Trn10Komono::ctrl2(k, paused);
    }
}
void Trn10KomonoFull::setHitMark(int attr) {
    markOn[markIdx] = 1;
    markAttr[markIdx] = attr;
    markIdx++;
    if (markIdx == 16) markIdx = 0;
}

Trn10MgrInit Trn10MgrInit::make(int level, const TrnRand& r) {
    Trn10MgrInit m;
    m.count = (level == 2) ? 0x12 : 0x10;
    int n = m.count;
    for (int i = 0; i < 18; i++) m.flag[i] = 0;
    m.flag[0] = 1;
    m.flag[r.i() % n] = 1;
    m.flag[r.i() % n] = 1;
    int ones = 0;
    for (int i = 0; i < n; i++) if (m.flag[i] == 1) ones++;
    if (ones < 3) {
        for (int i = 0; i < n; i++) {
            if (m.flag[i] == 0) { ones++; m.flag[i] = 1; if (ones == 3) break; }
        }
    }
    for (int i = 0; i < n; i++) if (m.flag[i] == 0) m.objectives++;
    m.topY = kTrn10TopY[level == 2 ? 1 : 0] - 9.0f;
    return m;
}
}

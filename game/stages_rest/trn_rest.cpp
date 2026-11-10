#include "trn_rest.h"
namespace vn {

int TrnFlowLogic::judge(TrnFlow& f, TrnSys& s, TrnFlowOut& o) {
    f.sub = 0;
    s.flagE33 = 1;
    o.add(TrnFlowEvent::CLEANUP);                 // varias llamadas de camara/sonido (func_1C97B0, func_1EFCF0, func_217D80...)
    if (s.flagE36 != 0) { o.nextState = TS_RETRY; return o.nextState; }    // D_009F4360
    bool fail;
    switch (f.mission) {                          // jtbl_009F4FF0: bloques 0x9E4C08.. (un destino failed/success por bloque)
        case 0: case 1: case 2: fail = s.norm > 0; break;                     // 0x9E4C08
        case 3:                                                                // 0x9E4C50: ademas suena la ovacion por cada ranura de D_501310 != 0
            for (int i = 0; i < 32; i++)
                if (s.d501310[i] != 0) { o.sound(0x150015, i * 0x1000); o.sound(0x150016, i * 0x1000); o.sound(0x15001F, i * 0x1000); }
            fail = s.flagE34 == 1; break;
        case 4: case 5: fail = s.norm > 0; break;                              // 0x9E4D34 / 0x9E4D7C
        case 6: fail = s.enemyAlive > 0; break;                                // 0x9E4DC4
        case 7: case 8: case 9: fail = s.norm > 0; break;                      // 0x9E4E0C / 0x9E4E54 / 0x9E4E9C
        case 10: fail = s.flagE34 == 1; break;                                 // 0x9E4EE4
        default: fail = s.enemyAlive > 0; break;                               // 0x9E4F34 (11..16)
    }
    o.nextState = fail ? TS_FAILED : TS_SUCCESS;
    return o.nextState;
}

void TrnFlowLogic::failed(TrnFlow& f, TrnSys& s, TrnFlowOut& o) {
    if (f.sub == 0) {
        f.t78 = 180.0f;                           // 0x43340000
        o.sound(0x1F000E, 0);
        o.sound(0x800109, -10);
        s.gp7410 = 0x32; s.gp740C = 0x32;
        f.sub++;
    } else if (f.sub != 1) return;
    if (!s.paused) f.t78 -= 1.2f;                 // 0x3F99999A
    if (f.t78 <= 0.0f) {
        o.add(TrnFlowEvent::CLEANUP);             // func_1E28C0
        f.sub = 0;
        s.flagE34 = 1;
        o.nextState = TS_SELECT2_INIT;            // D_009F44F0
    }
}

void TrnFlowLogic::retry(TrnFlow& f, TrnSys& s, TrnFlowOut& o) {
    if (f.sub == 0) { f.t70 = 2; f.sub++; }
    else if (f.sub != 1) return;
    if (!s.paused) f.t70--;
    if (f.t70 <= 0) {
        o.add(TrnFlowEvent::CLEANUP);             // func_1E28C0
        f.sub = 0;
        o.add(TrnFlowEvent::START_INIT);          // start_init(this)
    }
}

void TrnFlowLogic::exit(TrnFlow& f, TrnSys& s, TrnFlowOut& o) {
    if (f.sub == 0) {
        o.add(TrnFlowEvent::FADE_OUT_START);
        f.t78 = 30.0f;                            // 0x41F00000
        f.t7c = 0.0f;
        f.sub++;
    }
    if (f.sub == 1) {
        if (!s.paused) f.t7c += 0.04296875f;      // 0x3D2F8AFA
        if (!(f.t7c <= 1.0f)) f.t7c = 1.0f;
        if (!s.paused) f.t78 -= 1.2f;
        if (f.t78 <= 0.0f) f.sub++;
        else return;
    }
    if (f.sub == 2) {                             // 0x9E6D3C: guarda records y vuelve al menu
        if (s.modeDF8 == 2) { o.add(TrnFlowEvent::SAVE_RECORD, 0); o.add(TrnFlowEvent::SAVE_RECORD, 1); }
        else if (s.modeDF8 == 1) o.add(TrnFlowEvent::SAVE_RECORD, s.gp73F4 != 0 ? 1 : 0);
        o.add(TrnFlowEvent::SAVE_COMMIT);         // func_1D35E0(&D_5028C0)
        o.sound(0x400001, -10);
        s.gameState = 6;
    }
}

void TrnFlowLogic::mcSave(TrnFlow& f, bool saveDone, TrnFlowOut& o) {
    if (f.sub == 0) { f.sub = 1; }
    if (f.sub == 1) { o.add(TrnFlowEvent::MEMCARD_SAVE_START); f.sub = 2; }
    if (f.sub == 2) {
        if (saveDone) {
            o.add(TrnFlowEvent::CLEANUP);
            o.add(TrnFlowEvent::FADE_NONE);
            f.sub = 0;
            o.nextState = TS_SELECT_INIT;         // D_009F4500
        }
    }
}

bool TrnFlowLogic::startInit(TrnFlow& f, TrnSys& s) {
    f.c5c = 0;
    f.mission = f.selMission;                     // +0x60 = +0x58
    s.gp72C4 = (int8_t)f.mission;
    s.gp72C8 = (f.mission >= 0 && f.mission < 17) ? s.level[f.mission] : 0;
    return f.mission != f.prevMission;            // si es igual, el original se salta la carga de recursos
}

void TrnFlowLogic::successInit(TrnFlow& f, TrnSys& s) {
    f.t78 = 360.0f;                               // 0x43B40000
    f.anyUncleared = 0;
    for (int i = 0; i < 17; i++) if (s.rank[i] == -1) f.anyUncleared = 1;
    f.newRank = 0; f.allPerfect = 0; f.cA4 = 0; f.cA8 = 0;
    f.cB0 = -1; f.grade = 0; f.c10F4 = 0; f.t7c = 0;
    int m = f.mission;
    int cur = s.rank[m];
    // el rango sube un escalon solo si la dificultad elegida es exactamente rango+1 (y el rango es < 2)
    if (m >= 0 && m < 17 && s.level[m] == cur + 1 && cur < 2) {
        s.rank[m] = (int8_t)(cur + 1);
        f.newRank = 1;
        for (int i = 0; i < 17; i++) {            // allPerfect = todas a 2 (se corta en la primera que no lo es)
            if (s.rank[i] == 2) f.allPerfect = 1; else { f.allPerfect = 0; break; }
        }
    }
}

void TrnFlowLogic::successGrade(TrnFlow& f, const TrnSys& s) {
    int m = f.mission;
    int r = s.rank[m];
    if (r == 2) {
        f.grade = 3;
        if (f.newRank == 1) f.cA8 = 1;
    } else if (r == 1) {
        f.grade = 2;
    } else if (r == 0) {
        f.grade = 1;
        // vecinos: si ninguno de los dos esta superado (-1) la calificacion es 0 (misiones 1, 4 y 5)
        if (m == 1 && !(s.rank[0] >= 0) && s.rank[2] < 0) f.grade = 0;
        if (m == 4 && !(s.rank[3] >= 0) && s.rank[5] < 0) f.grade = 0;
        if (m == 5 && !(s.rank[4] >= 0) && !(s.rank[6] >= 0)) f.grade = 0;
        if (m >= 12) f.grade = 4;
    }
}

void trn10Init(TrnTask& t, Trn10Globals& g, TrnGlobals& tg, int mission) {
    g.mgr6F50 = 0; g.mgr6F4C = 0;
    t.c98 = TrainingLogic::frames(mission, tg.level[mission]);     // lee la mision de +0x60 (no de gp-0x72C4)
}

void trn10Tick(TrnTask& t, Trn10Globals& g, TrnGlobals& tg) {
    g.mgr6F50 = 1;
    if (g.mgr6F4C == 0 && !tg.paused) {
        t.c98--;
        if (!(t.c98 > 0)) tg.missionOver = 1;
    }
}
}

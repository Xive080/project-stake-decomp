// Ver boss3_motion.h.
#include "boss3_motion.h"
namespace vn { namespace bosspend {

bool Boss3Motion::lookup(int idx, B3MotionEntry& out) {
    if (idx < 0 || idx >= kBs3MotionCount) return false;
    out = kB3MotionFull[idx];
    return true;
}

// ControlAttack__9Boss3TaskFv (0x9C60B0), tramo "expect": lh a2,0(registro del ataque); blez -> sin comprobacion; escena 0x14 -> a2 tal cual;
// otra escena -> lw a2, D_009D0880 + a2*8.
int Boss3Motion::expectedAnim(int scene, int attackMotion, bool& ok) {
    ok = true;
    if (scene == kBs3SceneFight) return attackMotion;
    B3MotionEntry e;
    if (!lookup(attackMotion, e)) { ok = false; return attackMotion; }
    return e.anim;
}

// MotSupSet2__9Boss3TaskFiii (0x9C94A0). Escena 0x14: func_1FA540(obj, pos, idx) sin mas. Otra: si idx == +0x1EE8 no hace nada; borra
// 0x40000000 de +0xC; modo 0 -> func_1FA540(.., tabla[idx].anim), si no func_1FA640(.., anim, blend, modo); guarda idx, longitud (dos veces:
// +0x1EEC y +0x1EF0), blend (+0x1EF4) y modo (+0x1EF8).
bool Boss3Motion::motSupSet2(Bs3State& s, Boss3Engine& g, int idx, int blend, int mode) {
    if (s.scene == kBs3SceneFight) { g.setMotionNow(idx); return true; }
    if (s.motionIdx == idx) return true;
    B3MotionEntry e;
    if (!lookup(idx, e)) return false;
    s.enemyFlags &= 0xBFFFFFFFu;
    if (mode == 0) g.setMotionNow(e.anim); else g.setMotionBlend(e.anim, blend);
    s.motionIdx = idx;
    s.motionLen = e.length; s.motionLenPrev = e.length;
    s.motionBlend = blend; s.motionMode = mode;
    return true;
}

}}

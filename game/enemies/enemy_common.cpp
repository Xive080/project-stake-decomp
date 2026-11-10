#include "enemy_common.h"
namespace vn { namespace en { namespace rules {

// ORGMOD D_00A12500 (file offset 0x2D00): 16 floats, indice = objeto de arma (D_502690 + pl*4).
const float kWeaponDamage[16] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.5f, 2.0f, 2.0f, 2.0f, 0.6f, 0.4f, 1.7f, 0.8f, 3.0f, 1.0f};

float weaponDamageScale(const EnemyEnv& e, int player) {
    float v = 1.0f;
    if (e.gameState == 0xA && e.gameKind == 1) {              // OrgPlWeaponDamageGet_core
        int item = e.pl[player & 1].weapon;
        if (item < 0 || item > 15) return 1.0f;               // (el original no acota; fuera de tabla es lectura de otros datos)
        v = kWeaponDamage[item];
        if (item == 8 && e.pl[player & 1].oneHit == 0) v = 0.5f;
    }
    return v;
}

float enemyLifeScale(const EnemyEnv&) { return 1.0f; }

int scaledDamage(const EnemyEnv& e, int baseDamage, int player) {
    return static_cast<int>(static_cast<float>(baseDamage) * weaponDamageScale(e, player));
}

int weightedPlayerPick(const EnemyEnv& e) {
    int s0 = 1, s1 = 0;
    if (e.pl[1].life < e.pl[0].life) { s0 = 0; s1 = 1; }      // s1 = jugador con menos vida (empate: el 0)
    int lowLife = e.pl[s1].life;
    float total = static_cast<float>(e.pl[s0].life + lowLife);
    bool pickLow = (total * static_cast<float>(e.rand01())) < static_cast<float>(lowLife);
    return pickLow ? s1 : s0;
}

bool playerCanBeHurt(const PlayerView& p) {
    return (p.mode == 5 || p.mode == 4 || p.mode == 1) && p.life > 0;
}
}}}  // namespace

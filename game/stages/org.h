// Logica "Original" (overlay orgmod, 0xA0F880): objetos equipados, tienda, plata, misiones, union de segundo jugador.
// Cada funcion cita la original. Sin dibujo ni sonido: las llamadas a motor salen como eventos.
#pragma once
#include "stage_common.h"
namespace vn {

struct SilverEvent { bool create = false; bool recharge = false; int from = 0, to = 0; };   // TDispSilver (new / reCharge)

struct OrgLogic {
    // OrgPlWeaponDamageGet_core (0xA10C20): multiplicador de dano del arma equipada en la ranura 0 del jugador.
    static float weaponDamage(const StageSys& s, int pl);
    // OrgEnemyLifeScale (0xA10C70): vida enemiga = min(1, 0.66666, 0.5) segun objetos 38/39 en ranuras 2..3 de jugadores en modo 0,1,3,4,5.
    static float enemyLifeScale(const StageSys& s);
    // OrgAddItem (0xA11380): inventario +1 (tope 99) para ids 1..0x34. Devuelve true si el original avisa a la pantalla (HFileAddPageG(3,id)).
    static bool addItem(StageSys& s, int id);
    // OrgShopLevel (0xA113D0): desbloquea en tienda los objetos cuyo nivel == level (nota: m2c decia que se excluia el nivel 4; el asm no lo excluye).
    static void shopLevel(StageSys& s, int level);
    // OrgAddSilver (0xA11270): suma plata (acumulado siempre; plata actual con tope 999999 solo en estado 0xA y gameKind 1).
    static SilverEvent addSilver(StageSys& s, int delta, bool displayExists);
    // new_mission_complete (0xA10B10), parte de estado: marca mision n como completada y reinicia banderas de otras misiones.
    static void missionComplete(StageSys& s, int n);
    // SetOrgMissionOver (0xA11CE0): 0 = no, 1/2/3 = mision especial cumplida (14: 30000 pts; 24: 1000 de plata; 31: 140000 pts).
    static int  missionOver(const StageSys& s);
    // checkGetAnotherPlayerEquip (0xA11FE0): numero de ranuras del otro jugador con ese objeto (solo si el otro esta en modo 7, 9 o >10).
    static int  anotherPlayerHas(const StageSys& s, int pl, int item);
    // unequipAnotherPlayerEquip (0xA12080): quita (pone a 0) la primera ranura del otro jugador con ese objeto, mismas condiciones.
    static void unequipAnotherPlayer(StageSys& s, int pl, int item);
    // unequipJoinIn (0xA12120): al unirse un jugador, devuelve al inventario sus objetos de tipo 2 cuyo grupo choca con uno del otro.
    static void unequipJoinIn(StageSys& s, int pl);
    // setDispScoreJoinIn (0xA12270): devuelve true si hay que activar gp-0x737C (marcador de union); `stage` = gp-0x73D8, `slot1` = D_502691[pl*4].
    static bool setDispScoreJoinIn(int stage, int slot1);
};

// Tabla de dano por objeto (D_00A12500 de ORGMOD.BIN, 16 floats) y niveles de tienda (D_00A12B70, 0x36 bytes).
extern const float kOrgWeaponDamage[16];
extern const int8_t kOrgShopLevel[0x36];
// Tabla ELF 0x2C2A10: 2 bytes por objeto (grupo, tipo). tipo 2 = unico por pareja.
extern const int8_t kItemInfo[0x35][2];
}

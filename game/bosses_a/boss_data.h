// Tablas de datos de los jefes 2 (bs2mod) y 3 (bs3mod), volcadas de los overlays/ELF (ver boss_data.cpp, generado).
#pragma once
#include <cstdint>
namespace vn { namespace boss {

// Boss2Task D_009D7000: una fila por ataque (16 B). Offsets entre parentesis.
struct Bs2Attack {
    int16_t motion;       // +0  id de movimiento que debe estar activo (ControlAttack lo compara con +0x34 del jefe)
    int16_t start;        // +2  fotograma inicial al arrancar el movimiento (StartAttack -> MotSupSet2)
    int16_t end;          // +4  fotograma final (la ventana de ataque acaba cuando fotograma + velocidad >= end)
    uint8_t b6;           // +6  sin uso visto
    uint8_t playerDamage; // +7  danio al jugador (DamageToPlayer)
    int16_t hit1;         // +8  primer fotograma de golpe
    int16_t hit2;         // +0xA segundo fotograma de golpe (-1 = no hay)
    float   dist;         // +0xC distancia de ataque (SetAttackDist)
};
extern const Bs2Attack kBs2Attack[24];

// Boss2Task D_009D6080 (5 formas x 2 niveles; nivel = 0 un jugador, 1 dos jugadores segun func_943AC0).
struct Bs2Diff {
    int8_t  stagger[17];  // golpes necesarios para aturdir (-> +0x20F2/+0x20F4), indexado por dificultad 0..16
    int32_t dmg[17];      // danio por golpe valido (-> +0x20F8)
    int16_t up, down;     // pasos de subida/bajada de dificultad (+0x2100/+0x2102); tabla ELF 0x2D8680..
    bool    hasWarpF;     // solo la forma 0 tiene la tabla float
    float   warpF[17];    // umbrales de alfa para activar/desactivar colision (-> +0x2120/+0x2124)
};
extern const Bs2Diff kBs2Diff[5][2];
extern const int8_t kBs2CancelSkip[5];   // D_009D6250

// Boss3Task D_009D1C80 (20 ataques x 16 B; +0xC puntero a trayectoria no se vuelca)
struct Bs3Attack {
    int16_t motion;   // +0  indice de movimiento (>0: se traduce con kBs3Motion salvo en la escena 0x14)
    int16_t end;      // +2  fotograma final
    int16_t hitStart; // +4  inicio de ventana de disparo/golpe (-1: sin ventana)
    int16_t hitEnd;   // +6
    int16_t post;     // +8  fotograma de fin de la fase de recuperacion (-1 = terminar ya)
    int16_t kind;     // +0xA 0 = bombas, 1/2 = laser, 3 = melee
};
extern const Bs3Attack kBs3Attack[20];
struct Bs3Motion { int32_t anim, length; };   // D_009D0880 (8 B)
extern const Bs3Motion kBs3Motion[10];   // el overlay tiene al menos 12 entradas; solo se usan 0..9 (ver damageMotionId)
extern const int16_t kBs3DmgNormal[3][34];    // D_009D0AF0 [fase][dificultad]
extern const int16_t kBs3DmgBit8[3][34];      // D_009D0BC0
extern const int8_t  kBs3BulletsA[34];        // D_009D09C0
extern const int8_t  kBs3BulletsB[34];        // D_009D09F0
extern const float   kBs3PhaseParam[17][6];   // D_009D0C90 (24 B por entrada; [3+fase] = umbral de vida)

}} // namespace

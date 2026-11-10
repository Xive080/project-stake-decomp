// Logica del jugador de Vampire Night, reescrita a mano a partir de PlayerTask (ELF main, 0x20D990..0x213870).
// Cada funcion cita la funcion original. Los nombres de campos son inferidos del uso (offset en la estructura del jugador, 0x348 B por jugador).
// Sin dependencias de plataforma ni del hardware PS2: el motor alimenta `PlayerConfig` y llama a `PlayerLogic`.
#pragma once
#include <cstdint>
namespace vn {

// Modos del jugador (PlayerTask::changePlayerMode; campo +4). Solo los valores vistos en el codigo; sin nombres oficiales.
enum PlayerMode : int { PM_START_FIRST = 5, PM_JOIN_WAIT = 10, PM_GAMEOVER = 4 };

// Entradas de configuracion que el original lee de globales de sistema (menu de servicio/arcade).
struct PlayerConfig {
    int  gameState;          // global 0x500F14; 0xA = modo "juego arcade normal" en los chequeos de vidas/balas
    int  gameKind;           // gp-0x73B4: 1 = modo entrenamiento/tienda, 2 = otro modo especial; otro = normal
    int8_t cfgInitLife;      // 0x502901 (vidas iniciales de servicio)
    int8_t cfgMaxLife;       // 0x502902 (vidas maximas de servicio)
    int8_t itemSlot[4];      // D_502690 + jugador*4: 3 bytes de objetos equipados (valores 30/31/32 suman vida extra)
    int16_t bulletTable[256]; // D_A12300 (overlay): balas maximas por objeto
};

struct PlayerState {
    int mode = 0;            // +4
    uint32_t flags = 0;      // +8 (bit0 = jugando)
    int life = 0;            // +0x10
    int lifePrev = 0;        // +0x14
    int maxLife = 0;         // +0x40
    int initLife = 0;        // +0x44
    int maxBullet = 0;       // +0x48
    int bullet = 0;          // +0x4C
    float sightX = 0, sightY = 0; // +0x1C, +0x20 (mira, ya normalizada por el motor)
    int shotNow = 0;         // +0x24 (1 = dispara este fotograma)
    int emptyFlag = 0;       // +0x50 (cargador vacio)
    int bulletFlag54 = 0;    // +0x54 (1 -> reinicia el dibujo de balas)
    int flag58 = 0;          // +0x58
    int reloadReq = 0;       // +0x5C (1 = hay que recargar)
    int shotsFired = 0;      // +0x64
    int shotsTotal = 0;      // +0x6C
    int score = 0;           // +0xC
    int hits = 0;            // +0x68 (aciertos, para el bonus de punteria)
    int hitsTotal = 0;       // +0x70
    int damageFlag = 0;      // +0xBC (pide efecto de dano)
    int damageKind = 0;      // +0xC0
};

// Globales de dano (gp-0x7410[jugador], gp-0x73F8, gp-0x73F4, gp-0x73BC, estado de camara 0xD2).
struct DamageGlobals { int invuln = 0; int lifeLostFlag = 0; int mode73F4 = 0; int counter73BC = 0; int camState = 2; };
struct ReloadEvent { int sound = 0; };         // 0 = sin sonido; el motor reproduce seqRequest(sound)

// Entrada de la pistola del jugador (GunMgr, 0x64 B por jugador en 0x500580).
struct GunInput { int16_t sightX, sightY; bool active; bool trigger; };   // +8,+0xA, +0xE != 0, +0xC != 0
// Globales que gunExe consulta (gp-0x72D0 & 0x10, gp-0x7418, gp-0x738C).
struct GunGlobals { bool noAmmoCost; bool fireEnabled; bool reloadCancel; };
struct ShotEvent { bool fired = false; bool dryFire = false; };

// Fila de la tabla ELF 0x274AF0 (12 B): que reinicia startInit en cada tipo de arranque.
struct StartInfo { uint32_t flags; int mode; int value; };
extern const StartInfo kStartInfo[4];   // 0 = primera partida, 1 = continuar, 2 = fase nueva, 3 = unirse
enum StartFlags : uint32_t { SF_SET_LIFE = 1, SF_CLEAR_C = 4, SF_UNK8 = 8, SF_CREDIT_A = 0x10, SF_CREDIT_B = 0x20 };

struct PlayerLogic {
    static void setMaxLife(PlayerState& p, const PlayerConfig& c);          // PlayerTask::setMaxLife
    static void setInitLife(PlayerState& p, const PlayerConfig& c, int pl); // PlayerTask::setInitLife
    static void setMaxBullet(PlayerState& p, const PlayerConfig& c, int pl);// PlayerTask::setMaxBullet
    static void setLifeStart(PlayerState& p);                                // PlayerTask::setLifeStart
    static void changePlayerMode(PlayerState& p, int mode);                  // PlayerTask::changePlayerMode (sin el cambio de rutina, eso lo hace el planificador)
    // Parte de startInit que solo toca el estado del jugador. Devuelve el modo siguiente. `creditsA/B` = contadores globales 0x500DF8/0x500DFC.
    static int startInit(PlayerState& p, const PlayerConfig& c, int pl, int kind, int16_t& creditsA, int16_t& creditsB);
    // Tramo de gameLoop que fija el limite de vida; devuelve true si el jugador acaba de morir (life <= 0 -> modo GAMEOVER).
    // PlayerTask::getInput + gunExe (estado): consume la entrada y dice si hay disparo. El motor hace el trazado del rayo (Ent_ShotRay),
    // el sonido (0x2000C/0x2000D segun jugador) y los cartuchos; aqui solo el estado.
    static ShotEvent gunExe(PlayerState& p, const GunInput& g, const GunGlobals& gl, int mode);
    static ReloadEvent reloadExe(PlayerState& p, const PlayerConfig& c, const GunGlobals& gl, int pl, int pauseLevel);   // PlayerTask::reloadExe
    static void addScore(PlayerState& p, const PlayerConfig& c, int pl, int16_t delta);   // AddPlayerScore (objetos 47 = x2, 48 = x4)
    static void addHitNum(PlayerState& p);                                                // AddPlayerHitNum
    static int  hitBonus(const PlayerState& p);                                           // AddHitPerBonus (puntos a sumar; 0 si no hay disparos)
    static int  canDecLife(const PlayerConfig& c, const PlayerState& p, const DamageGlobals& g, int pl);  // can_DecPlayerLife: 0 = se puede
    // DecPlayerLife: devuelve 1 si se aplico dano (o lo salvo el objeto 0x1B). `rand` = nlRand().
    static int  decLife(PlayerState& p, const PlayerConfig& c, DamageGlobals& g, int pl, bool hasKind, int kind, int (*rand)());
    static int  incLife(PlayerState& p, const PlayerConfig& c, int pl);                   // IncPlayerLife: 1 = sumo vida, 0 = ya al maximo (+1000 puntos)
    static bool lifeTick(PlayerState& p, const PlayerConfig& c);
};
}

// Reglas comunes de enemigos comunes de Vampire Night, reescritas a mano (EnVampireTask, PeopleTask).
// Los globales de sistema (0x500xxx, gp-0x7xxx) viven en bss y NO estan en el ELF: se modelan como `EnemyEnv` inyectado.
// Las llamadas a motor (sonido, efectos, camara, movimiento) no se implementan: salen como `Event` (cola de eventos).
// Sin rutas absolutas ni dependencias de plataforma.
#pragma once
#include <cstdint>
#include <vector>
namespace vn { namespace en {

// Eventos que el original resuelve llamando al motor. El motor los ejecuta en el orden en que salen.
enum EvType : int {
    EV_SOUND,          // SoundMgrClass::seqRequest(a = id, b = emisor<<12 [campo +0x24 del enemigo]); 0x217C30 / 0x...seqRequest
    EV_SCORE,          // AddPlayerScore(a = jugador, b = puntos)           (0x212FA0)
    EV_HITNUM,         // AddPlayerHitNum(a = jugador)                      (0x213070 / AddPlayerHitNum__Fi)
    EV_HITMARK,        // new_EnemyHitMark(a = parte, b = jugador, c = codigo 0x3E8 golpe / 0x3E9 muerte)
    EV_PARTS_CHAIN,    // EnPartsChain(a = parte, b = jugador): la parte se desprende
    EV_DAMAGE_PLAYER,  // DecPlayerLife(a = jugador, b = 1 (hasKind), c = tipo de dano)   (0x212D00)
    EV_DAMAGE_MOT,     // changeDamageMot(a = jugador)
    EV_EFFECT_HIT,     // new_EnEff(parte a) efecto de impacto
    EV_EFFECT_TRACER,  // TyakudanLauncherMgrBorn(parte a, jugador b) (modo entrenamiento, arma 0xE)
    EV_CAMERA_CHG,     // CameraTask::chgCafsIdx(0, 1)
    EV_RESCUE,         // PeopleTask::addRescuePeopleNum
    EV_GALLERY,        // HFileAddOtherGallery (galeria de rescatados)
    EV_MOTION,         // cambio de animacion: a = id de animacion, b = mezcla (frames), c = 0 normal / 1 capa de dano
    EV_ADJ_CHEST,      // adjChest(a, b): ajuste del pecho
    EV_LIFEUP_ITEM,    // func_933510(a = jugador): aparece el item de vida (si no esta al maximo)
    EV_SCRIPT,         // orden del script de comportamiento que ejecuta el motor: a = opcode, b/c/d = argumentos (ver vampire.h / people.h)
    EV_BEAT_RECORD     // ZM_EnemyBeatRecord(a = tipo)
};
struct Event { EvType type; int a, b, c, d; };
typedef std::vector<Event> EventList;
inline void emit(EventList& v, EvType t, int a = 0, int b = 0, int c = 0, int d = 0) { Event e = {t, a, b, c, d}; v.push_back(e); }

// Vista de un jugador (struct de 0x348 B en 0x500730: +4 modo, +0x10 vida, +0x346 = flag de entrenamiento D_500A76).
struct PlayerView {
    int  mode = 0;            // +4  (0x500734): solo 5, 4 y 1 pueden recibir dano de enemigos
    int  life = 0;            // +0x10 (0x500740)
    int  maxLife = 0;         // +0x40 (0x500770)
    int8_t weapon = 0;        // D_502690 + pl*4: objeto de arma equipado (0xE = lanzagranadas/trazadora, 8 = especial)
    int8_t oneHit = 0;        // 0x500A76 + pl*0x348: flag de entrenamiento (mata de un golpe / quita el 0.5 del arma 8)
    bool invuln = false;      // gp-0x7410[pl]: invulnerabilidad (pl 0 = gp-0x7410, pl 1 = gp-0x740C)
};
// Entrada de camara actual (gp-0x7338 -> tabla de 0xD0 B por camara, indice en +4).
struct CameraView { int state = 2; /* +0xD2 */ int pathId = 0; /* +0xD0 */ float frame = 0; /* +0xC0 */ };

struct EnemyEnv {
    int     gameState = 0;      // 0x500F14 (0xA = arcade normal)
    int     gameKind = 0;       // gp-0x73B4 (1 = entrenamiento)
    int16_t playerCount = 1;    // 0x500DF8: 1 jugador, 2 jugadores
    int16_t coop = 0;           // 0x500DFC
    int     solo = 0;           // gp-0x73F4: jugador activo cuando playerCount == 1 (2 = dos jugadores en tablas de vida)
    int     stage = 0;          // gp-0x73D8: fase 0..6 (6 = igual que 0 en las tablas de salto; 5 = sin sonidos)
    int     levelA = 0;         // gp-0x73C8
    int     levelB = 0;         // gp-0x73C6
    int     attackSlot[2] = {0, 0};  // gp-0x7420 / gp-0x741C: el jugador i ya tiene un enemigo asignado
    PlayerView pl[2];
    CameraView cam;
    int    (*rand)() = nullptr;      // nlRand()
    double (*rand01)() = nullptr;    // nlRandom(): [0,1)
    int     lastHitPlayer = 0;       // gp-0x6FD0 (EnVampire) / gp-0x7148 (People)
};

// Entradas de impacto del fotograma: gp-0x7350[2] = jugador que disparo (o -1), por jugador: parte golpeada (byte +0x70 de la tabla) y clase de arma gp-0x7348[pl].
struct HitInput {
    int shooter[2] = {-1, -1};
    int partOfPlayer[2] = {0, 0};
    int weaponClass[2] = {0, 0};
};

namespace rules {
// OrgPlWeaponDamageGet (0x942F00 -> OrgPlWeaponDamageGet_core 0xA10BF0 en ORGMOD): multiplicador de dano del arma.
// Solo en arcade (0x500F14 == 0xA) con gp-0x73B4 == 1 se consulta la tabla (D_A12500, 16 floats); si no, 1.0.
extern const float kWeaponDamage[16];
float weaponDamageScale(const EnemyEnv& e, int player);
// OrgEnemyLifeScale (0xA10C60): escala de vida por objetos 0x27/0x26. El bucle del original compara con c.lt.s contra 1.0
// y solo sustituye si 1.0 < candidato; como los candidatos (0.5, 0.6667) son menores, SIEMPRE devuelve 1.0.
float enemyLifeScale(const EnemyEnv& e);
// Dano efectivo = trunc(dano base * multiplicador del arma) (fptosi 0x14C518 trunca hacia cero).
int scaledDamage(const EnemyEnv& e, int baseDamage, int player);
// setAtcPlID: eleccion ponderada por vida cuando hay 2 jugadores (ambos enemigos). `lifeA/lifeB` = vida de jugador 0/1.
// Devuelve el jugador elegido: el de menos vida se elige con probabilidad vidaMenor/(suma).
int weightedPlayerPick(const EnemyEnv& e);
// Un jugador solo recibe dano de enemigos si su modo es 5, 4 o 1 y tiene vida > 0.
bool playerCanBeHurt(const PlayerView& p);
// Eleccion entre dos sonidos por paridad de nlRand: par -> `even`, impar -> `odd` (movz del original).
inline int pickByParity(int r, int odd, int even) { return (r & 1) ? odd : even; }
}  // namespace rules
}}  // namespace vn::en

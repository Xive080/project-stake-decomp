// Logica de EnVampireTask (gamemod, 0x952170..0x956D40) reescrita a mano. Cada funcion cita la original.
// Campos del enemigo nombrados por uso, con el offset en comentario. Las llamadas a motor salen como eventos.
#pragma once
#include "enemy_common.h"
namespace vn { namespace en {

enum { kVampTypes = 18, kVampParts = 18 };
// Tablas extraidas del binario (vampire_tables.cpp, generado).
extern const int16_t kVampDamage[kVampTypes][kVampParts][6];   // D_009AF570: dano por [tipo][parte][nivel de golpe]
extern const int16_t kVampLifeByType[kVampTypes];              // D_009B6BE0
extern const int16_t kVampLifeByLevel[17];                     // D_009B6B50 (indice gp-0x73C8)
extern const int16_t kVampLifeByDiff1P[17];                    // D_009B6B80 (indice gp-0x73C6)
extern const int16_t kVampLifeByDiff2P[17];                    // D_009B6BB0 (si gp-0x73F4 == 2)

// Bits de +0xC (flags).
enum VampFlags : uint32_t {
    VF_HITCHECK = 0x8,          // hay colision/impacto que procesar
    VF_INVULN = 0x100,
    VF_BIT2000 = 0x2000,
    VF_FINISHED = 0x20000,      // muerte terminada (Dead, paso 3)
    VF_AIRBORNE_DEAD = 0x40000, // muerte en el aire
    VF_DEAD = 0x04000000,
    VF_DAMAGE_MOT = 0x40000000, // reproduciendo animacion de dano
    VF_BIT10000000 = 0x10000000
};
// Bits de +0x1DA4 (estado de animacion/caida).
enum VampState : uint32_t { VS_HIT = 1, VS_DEADHIT = 2, VS_DAMMOT_KIND = 4, VS_AIRBORNE = 8, VS_DEADSND = 0x10 };

struct VampPart {           // 0xF0 B por parte, desde +0 de la parte 0 (partes 1..17 usadas)
    uint32_t flag150 = 0;   // +0x150: bit 0x01000000 = la parte ya esta desprendida (no se encadena)
    uint32_t flag154 = 0;   // +0x154: bit 0x100 = marcada (sonido 0x2002C al golpearla)
    int16_t  hitLevel = 0;  // +0x168: nivel de golpe (indice en la fila de dano)
};

struct VampireState {
    uint32_t flags = 0;            // +0xC
    uint32_t flags2 = 0;           // +0x10 (bit 2 = muriendo, bit 4 = desaparecido)
    int life = 0;                  // +0x18
    int lifeMax = 0;               // +0x1C
    int lifePrev = 0;              // +0x20
    int emitter = 0;               // +0x24 (emisor de sonido; id<<12)
    int type = 0;                  // +0x5C tipo (0..17; 8 = especial)
    int curMot = 0;                // +0x34 animacion actual
    int layerMot = 0;              // +0x38 animacion de la capa de dano
    float frame3C = 0;             // +0x3C fotograma (chkParam tipo 1, Dead paso 1)
    float layerEndFrame = 0;       // +0x40 (se fuerza al final de la animacion)
    float curFrame = 0;            // +0x44 fotograma de la animacion actual
    float layerFrame = 0;          // +0x48 fotograma de la capa
    int8_t layerBusy = 0;          // +0x6C (!=0 mientras la capa de animacion sigue activa)
    int8_t layerFlag6B = 0;        // +0x6B (bit 4: no forzar el final)
    int8_t atkPlayer = -1;         // +0x1D7C jugador al que ataca (-1 = ninguno)
    uint8_t subType = 0;           // +0x1D7D
    uint8_t step = 0;              // +0x1D82 paso de la maquina de estados de Dead/Wait/Attack
    int paramKind = -1;            // +0x1D8C (chkParam: 0 fin de anim, 1 fotograma, 2 camara, -1 = nada)
    int paramA = 0;                // +0x1D90
    int paramB = 0;                // +0x1D94
    float speedScale = 0;          // +0x1D9C
    int motActive = 0;             // +0x1DA0
    uint32_t stateFlags = 0;       // +0x1DA4
    int motId = 0;                 // +0x1DA8
    int motLoop = 0;               // +0x1DAC
    float reachLimit = 0;          // +0x1DC8 (limite para poder danar al jugador; tipo 8 lo fija a -20.0)
    float screenX = 0;             // +0x2E0 (signo -> jugador izquierdo en damagePlayer)
    float posX = 0;                // +0x30->+0x0
    float posZ = 0;                // +0x30->+0x8
    float velX = 0, velY = 0, velZ = 0, gravity = 0;   // +0x30->+0xC/+0x10/+0x14/+0x1C (velocidad y gravedad)
    float posY = 0;                // +0x30->+0x4
    // --- "script" de comportamiento (getParam): flujo de ordenes de 16 bits en +0x1D88
    int scriptPos = 0;             // +0x1D88 (aqui: indice en bytes dentro del flujo)
    int scriptMark1DC4 = 0;        // +0x1DC4
    int scriptMark1DD8 = 0;        // +0x1DD8
    int aiMode = 0;                // +0x1D7E
    float aiTimer = 0;             // +0x1D84
    float f88 = 0, f90 = 0;        // +0x88, +0x90
    int posExtra[3] = {0, 0, 0};   // +0x30->+0x24/+0x28/+0x2C
    float dist1DD0 = 0;            // +0x1DD0 (tiempo/distancia hasta el jugador; menor = mas cerca)
    float dist1DC0 = 0;            // +0x1DC0 (tipo 8: 150.0 en charInit; en ataque: distancia camara-marca)
    bool hasMark1DC4 = false;      // +0x1DC4 != 0 (posicion marcada por la orden 17)
    bool hasMark1DD8 = false;      // +0x1DD8 != 0 (orden 15)
    int16_t h1DD4 = -1;            // +0x1DD4 (movimiento de ataque; 0x313/0x314 = saltos con marca; -1 = por defecto 0x325)
    uint32_t flags1CCC = 0;        // +0x1CCC (bit 2)
    VampPart parts[kVampParts];
};

struct VampDamageResult { int soundId = 0; };

struct VampireLogic {
    // EnVampireTask::charInit (parte de vida): vida = tipo + nivel + dificultad (2 tablas segun gp-0x73F4==2); entrenamiento * OrgEnemyLifeScale.
    static void initLife(VampireState& v, const EnemyEnv& e);
    // EnVampireTask::decLife(base): resta trunc(base*arma); en entrenamiento con flag por jugador, resta toda la vida.
    static void decLife(VampireState& v, const EnemyEnv& e, int baseDamage);
    // EnVampireTask::sufferDamage(player): devuelve el sonido de impacto por clase de arma (4 = ninguno, 2 = 0x20025, resto 0x180010) y emite efecto/animacion.
    static int  sufferDamage(VampireState& v, const EnemyEnv& e, EventList& ev, int player, int part, int weaponClass);
    // EnVampireTask::checkDamage: procesa los impactos del fotograma. Puntuacion: +10 por golpe, +100 al matar.
    static void checkDamage(VampireState& v, EnemyEnv& e, const HitInput& h, EventList& ev);
    // EnVampireTask::damagePlayer(kind): el enemigo ataca al jugador si la camara esta en estado 2 y esta al alcance.
    // `reach` = valor de +0x30->+0x38 (distancia); el dano se aplica si !(reach <= reachLimit). Devuelve true si emitio dano.
    static bool damagePlayer(const VampireState& v, const EnemyEnv& e, EventList& ev, float reach, int kind);
    // EnVampireTask::chkParam: fin de la espera del "script" de parametros; devuelve true si se resetea a -1.
    static bool chkParam(VampireState& v, const EnemyEnv& e);
    // EnVampireTask::setAtcPlID: asigna el jugador al que atacara (con reservas attackSlot). Devuelve el elegido (-1 si no hay hueco).
    static int  setAtcPlID(VampireState& v, EnemyEnv& e);
    // EnVampireTask::checkDamageMotEnd. `motLen` = tabla de duraciones por animacion (halfwords en 0x1FB73E4, RAM de ejecucion: inyectada).
    static void checkDamageMotEnd(VampireState& v, const int16_t* motLen);
    // EnVampireTask::Dead (maquina de estados de muerte), un fotograma. Entradas de motor en `DeadIn`; contadores de enemigos vivos en `counters`.
    // Entradas de motor: aboveGround = !(pos.y <= getHight()) (paso 0); landed = pos.y <= getHight() (paso 2);
    // kickX/kickZ = (0,0,-1.1) girado por la direccion de la camara (paso 1; el eje Y lo fija la regla a -1.5).
    struct DeadIn { bool aboveGround = false; bool landed = false; float kickX = 0, kickZ = 0; };
    struct Counters { int alive = 0; /* gp-0x7370 */ int onScreen = 0; /* gp-0x7374 */ };
    // EnVampireTask::changeDeadMot (0x954370): sonido de muerte (si VS_DEADSND), animacion 0x49 con mezcla 10; fija motActive=1, paramA=25, paramKind=0x14.
    static void changeDeadMot(VampireState& v, EventList& ev);
    static void dead(VampireState& v, EnemyEnv& e, Counters& c, const DeadIn& in, EventList& ev);
    // EnVampireTask::Attack_Eknd_Sub: salto parabolico hacia el objetivo. T = duracion/1.44 (`jumpFrames` = +0x1DD0); gravedad -0.0392; el eje Y apunta
    // a (objetivo.y - 5) respecto a la posicion. Fija velX/velY/velZ/gravity, speedScale = 1.15 (0x3F99999A), step = 3, stateFlags |= VS_AIRBORNE.
    // EnVampireTask::getParam (0x954500): interprete del script. Lee ordenes hasta que una fija una espera (paramKind != -1).
    // Ordenes (tabla de saltos 0x9B56C0, 21 entradas; >20 se ignoran):
    //  0 motActive=s16 (si >0 espera fin de anim)  1 espera frame3C >= s16  2 espera camara (s16 id, s16 frame, +2 relleno)
    //  3 motSupplement(s16,s16,s16)  4 motion(s16,s16,s16)+f88=f32,f90=f32  5 motion(s16)  6 adjChest+motSupplement  7 adjPos+motSupplement
    //  8 speedScale=1.2*f32  9 flags|=u32  10 flags&=~u32  11 stateFlags|=u32 (+sonido 0x17030D si bit 0x10 y anim 0x312)  12 stateFlags&=~u32
    //  13 aiMode=s8, step=0, aiTimer=0, motActive=-1  14 pos=(f32,f32,f32)  15 pos extra (3 x s16)  16 h1DD4=s16  17 mark1DC4, saltar 14 B
    //  18 flags1CCC bit2 = (s16 != 0)  19 sonido u32  20 espera infinita (paramKind=20).
    // `script` = bytes del flujo; devuelve el numero de ordenes ejecutadas (maxOps evita bucles infinitos si el flujo no tiene esperas).
    static int getParam(VampireState& v, EnemyEnv& e, const uint8_t* script, int scriptSize, EventList& ev, int maxOps = 4096);
    // EnVampireTask::Attack_Eknd (0x955E00): maquina de estados del ataque (7 pasos en +0x1D82), un fotograma. Las funciones de motor/geometria entran por `AttackIn`.
    // 0 elegir jugador (setAtcPlID) y pasar al 1; 1 preparar animacion 0x312 y esperar fotograma 45; 2 ventanas de sonido y salto (Attack_Eknd_Sub) cuando dist1DD0 < 50 (u 80 con anim 0x313/0x314
    // en los fotogramas 3..11); 3 con dist1DD0 < 16 golpea al jugador (damagePlayer kind 7) -> 4; 4 tras 10 fotogramas rebota (vel *= -1.2) y libera el hueco -> 5;
    // 5 aterriza (suelo+5) o rebota (x0.75) -> 6; 6 al terminar la animacion vuelve a aiMode 1, paso 0.
    struct AttackIn {
        float height = 0;           // getHight()
        float speedXZ = 0;          // hypot(velX, velZ)
        float markX = 0, markZ = 0; // posicion de la marca (orden 17)
        float camToMark = 0;        // distancia camara -> marca
        const int16_t* motLen = nullptr;
        float reach = 0;            // alcance para damagePlayer (+0x30->+0x38)
    };
    static void attackEknd(VampireState& v, EnemyEnv& e, const AttackIn& in, EventList& ev);
    static void attackJump(VampireState& v, float jumpFrames, float tx, float ty, float tz, EventList& ev);
};
}}  // namespace vn::en

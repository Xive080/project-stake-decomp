// game/enemies4: PetitZombieTask, ArmorTask, EnEtc* y otras tareas de objeto/enemigo de GAMEMOD (0x938xxx..0x95xxxx), reescritas a mano.
// Cada funcion cita la original con su direccion (asm/gamemod/gamemod_text.s, base de carga 0x92B000).
// Los globales de sistema (0x500xxx, 0x502xxx, gp-0x7xxx) viven en bss y NO estan en el ELF: se modelan como `E4Env` inyectado.
// Las llamadas a motor (sonido, dibujo, camara, colisiones, creacion de tareas) NO se implementan: salen como `E4Event`.
// Sin rutas absolutas ni dependencias de plataforma.
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace vn { namespace en4 {

// ---------------------------------------------------------------------------------------------------------------------
// Eventos de motor (el motor los ejecuta en el orden en que salen).
enum E4EvType : int {
    E4_SOUND_GLOBAL,   // func_217C30(&D_515F30, a = id, 0, 0, 0x40): sonido sin emisor
    E4_TASK_CLOSE,     // func_1A2630(): la tarea se cierra (el original SIGUE ejecutando tras la llamada)
    E4_TASK_CLOSE_MOT, // func_1A3F80(&base): cierra el enemigo (PetitZombie al salir de camara)
    E4_SET_HANDLER,    // func_1A3DC0 / func_1A3F60: cambia la "funcion de la tarea": a = id de manejador (ver cada modulo), b = prioridad
    E4_MOTION,         // func_1FA640(a = animacion, b = mezcla)  cambio de animacion
    E4_MOTION_STEP,    // func_1F8D50: avanza la animacion del cuerpo
    E4_ITEM_APPLY,     // func_A10B00(a = objeto) en ORGMOD: aplica el efecto/consumo de un objeto
    E4_SNOW_PARTICLE,  // new_SnowParticle(pos f[3], a = hueco, b = reinicio)
    E4_SPAWN_ATTACH,   // new_EnEtcAttach(pos, a = id de modelo): trozo de armadura que sale volando
    E4_SPAWN_PARTS,    // new_EnEtcParts(parent a, b, c, d = 2/3 ...)
    E4_SOUND_EMITTER,  // seqRequest sobre un emisor
    E4_DRAW,           // (informativo) llamada de dibujo que el motor resuelve
    E4_CAMERA_FX,      // efecto de camara / transformacion de matriz (informativo)
    E4_DAMAGE_PLAYER,  // DecPlayerLife_Chk(a = jugador, b = 1, c = tipo de dano)
    E4_HITNUM,         // func_213070(a = jugador): AddPlayerHitNum
    E4_CLOSE_GROUP     // func_1A1F00(+0x1BC - 0x10): cierra el grupo de partes que la tarea lleva en +0x1BC
};
struct E4Event { E4EvType type; int a, b, c, d; float f[3]; };
typedef std::vector<E4Event> E4Events;
inline void e4emit(E4Events& v, E4EvType t, int a = 0, int b = 0, int c = 0, int d = 0, float x = 0, float y = 0, float z = 0) {
    E4Event e = {t, a, b, c, d, {x, y, z}}; v.push_back(e);
}

// Camara actual (gp-0x7338 -> entrada de 0xD0 B por camara, indice en +4): +0xD0 camino (s16), +0xC0 fotograma (float),
// +0x90/+0x94/+0x98 posicion de la camara.
struct E4Camera { int pathId = 0; float frame = 0; float pos[3] = {0, 0, 0}; };

// Entorno global inyectado.
struct E4Env {
    int16_t gameState = 0;      // 0x500F14 (0xA = arcade; 0xF = el PetitZombie cambia su escala por tipo)
    int     gameKind = 0;       // gp-0x73B4 (1 = entrenamiento/arcade normal para los objetos, 2 = el EnEtc usa otra tarea madre)
    int     stage = 0;          // gp-0x73D8 (s8): fase; 0 = sin sonido de EnSndReq
    int8_t  itemFlag[64] = {0}; // D_502708 + objeto: bit 0x1 = poseido, 0x2 = activo, 0x20 = bloqueado
    int     frameCounter = 0;   // gp-0x739C: contador global de fotogramas (limite de repeticion de sonido)
    bool    paused = false;     // func_1F33B0(&D_500F20) > 0: pausa de demo/sistema
    E4Camera cam;
    int  (*rand)() = nullptr;   // nlRand()
    // globales que las funciones ESCRIBEN
    int8_t  g500F03 = 0, g500F05 = 0, g500E11 = 0;
    int8_t  gp73CC = 0;
    int8_t  gp73D0 = 0, gp73D4 = 0;   // gp-0x73D0 / gp-0x73D4 (bytes que el constructor de EnEtc copia a +0x168/+0x164)
    int     gp7340 = 0;         // gp-0x7340: la nieve esta activa
    int8_t  gp6FDC = 0;         // gp-0x6FDC: contador de reinicios de nieve
    int16_t gp7200 = 0;         // gp-0x7200
    int8_t  gp7400 = 0;         // gp-0x7400
    int16_t gp7358 = 0;         // gp-0x7358: ataques perdidos (missCount)
    int16_t gp7374 = 0;         // gp-0x7374: enemigos en pantalla
    int     attackSlot[2] = {0, 0};   // gp-0x7420 / gp-0x741C: el jugador i ya tiene un ataque reservado
    int8_t  attackGate = 0;     // gp-0x7360
    int16_t coop = 0;           // 0x500DFC
    int8_t  snowFlags[10] = {0};// D_9C3990: hueco de nieve ocupado
    // pantalla de nieve/tinte del nivel 4 (gp-0x7440): alfa (+0x18), desplazamiento (+0x24, +0x28)
    float   tintAlpha = 0; int tintScrollX = 0, tintScrollY = 0;
};

// Constante float a partir de sus bits (las del asm: lui/ori + mtc1).
inline float fbits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// Conversion de fotogramas NTSC (60 Hz) a PAL (50 Hz) que repiten St4_02TimerBunki y EnEtcSnd: n*50/60 con division entera
// (el original usa la multiplicacion magica 0x88888889 con signo / sin signo; para n >= 0 ambas dan n*50/60 truncado).
inline int palFrames(int n) { return (int)(((int64_t)n * 50) / 60); }
}}  // namespace vn::en4

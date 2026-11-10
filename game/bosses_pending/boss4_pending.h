// Funciones PENDIENTES de TBoss4 (overlay bs4mod, carga 0x9C4800), reescritas a mano a partir de asm/bs4mod/bs4mod_text.s.
// Completa game/bosses_b/boss4.* (que no se modifica): usa Boss4State / Boss4Globals / Boss4Env / Boss4Logic de alli.
// Lo que Boss4State no tiene (campos del objeto TBoss4 que solo usan estas funciones) vive en Boss4Ext, con su offset.
// Convenio heredado de boss4.h: los angulos de 16 bits se guardan como float en Boss4State::rot[] (son valores enteros).
// Las llamadas al motor (matrices, rutas de camara, spawn de rocas, sonido) NO se implementan: salen por Boss4PendEnv.
#pragma once
#include <cstdint>
#include "../bosses_b/boss4.h"
#include "boss_pending_data.h"
namespace vn { namespace bosspend {
using namespace vn::boss4;

// Ids de movimiento que citan estas funciones (valores del asm).
enum B4Motion : int { MOT_HIT_JUMP = 0x1AA, MOT_HIT_LAND_A = 0x1A5, MOT_HIT_LAND_B = 0x1A6, MOT_FALL = 0x17C, MOT_KIRI = 0x1A0 };
// Estados del subobjeto de ruta (+0x1DB8): 1 = sigue la ruta de camara, 3 = fisica libre (velocidad/aceleracion), otro = nada.
enum B4PosMode : int8_t { POS_NONE = 0, POS_CAM_PATH = 1, POS_FREE = 3 };
// Modo de pareja de MotSupSet* (+0x1E46): 0 = ninguno, 1 = MotSupSet, 2 = MotSupSet2, 3 = MotSupSet3.
// Modo de clon (+0x2120): 0 reposo, 1 en espera (listo), 2 saliendo (BnStartBunshin), 3 volviendo (BnStartShusoku).
enum B4BunMode : int32_t { BN_IDLE = 0, BN_STANDBY = 1, BN_OUT = 2, BN_BACK = 3 };
constexpr int kMaxIwa = 16;     // el original no limita: la tabla de rocas llega a 12 y +0x2E40.. tiene sitio de sobra; aqui se acota.

// Campos del TBoss4 que usan estas funciones y no estan en Boss4State.
struct Boss4Ext {
    // velocidad por fotograma (CtrlPre)
    float prevPos[3] = {0, 0, 0};      // +0x1EB4 posicion del fotograma anterior
    float prevDelta[3] = {0, 0, 0};    // +0x1EC0 desplazamiento del fotograma anterior
    float accel[3] = {0, 0, 0};        // +0x1ECC variacion del desplazamiento
    float heading = 0, headingDelta = 0;   // +0x1EE4, +0x1EE8 rumbo del desplazamiento (func_1AF9C0) y su variacion
    float holdDelta[3] = {0, 0, 0};    // +0x1EF8 desplazamiento congelado (CheckDamageEnd con ritmo 0)
    // ruta de camara (ControlPos)
    uint32_t anpo[6] = {0, 0, 0, 0, 0, 0};   // +0x1DBC punto ANPO: x,y,z (float) + 3 angulos (int)  (bits crudos)
    int32_t pathId = 0;                // +0x1DEC id de la ruta de camara
    float jitterT = 0;                 // +0x1E0C tiempo de la ruta de temblor (id 0x59)
    int8_t slot = 0;                   // +0x1E10 punto de aparicion (kB4ShoukanSlot); <0 = sin punto
    int8_t b1E12 = 0;                  // +0x1E12 fila de kB4KiriPreAngle
    float groundOffset = 0;            // +0x1D78 altura del cuerpo sobre el suelo
    int32_t frameCounter = 0;          // +0x1E20 fotogramas vividos (CtrlPost)
    int16_t stateResult = 0;           // +0x1F22 resultado de func_1C6C80 (estado de la tarea)
    float frameB = 0, frameC = 0;      // +0x40, +0x44 contadores de movimiento secundarios
    float rateB = 0;                   // +0x1EA0 ritmo del contador +0x40
    float velX = 0, velZ = 0, acc[3] = {0, 0, 0};   // objeto de posicion: +0xC/+0x14 y +0x18,+0x1C,+0x20 (la velocidad Y es Boss4State::velY)
    int motionObjCur = 0;              // *(this+8)+4: movimiento actual del objeto de movimiento (MotSupSet)
    // rocas (TBoss4Iwa)
    int8_t iwaCount = 0;               // +0x2EA4
    int32_t iwaHandle[kMaxIwa] = {};   // +0x2E40 punteros a las rocas (aqui: manejadores que devuelve el motor)
    int8_t iwaState[kMaxIwa] = {};     // +0x2E90 0 = no existe, 1 = esperando, 2 = lanzada
    // clones (K_BUNSHIN)
    int32_t bnIndex = 0;               // +0x2104 indice de clon (0..1)
    float bnOffset[3] = {0, 0, 0};     // +0x2108 desplazamiento respecto al jefe
    float bnVel[3] = {0, 0, 0};        // +0x2114 velocidad del desplazamiento
    int32_t bnMode = 0;                // +0x2120 B4BunMode
    int8_t bnCmd = 0;                  // +0x2124 ultima orden vista del jefe (+0x1E3C del padre)
    int16_t bnLastMot = 0;             // +0x2126 ultimo movimiento pedido por CtrlPre_bunshin_battle2
    Boss4Ext* ownerExt = nullptr;      // campos extra del jefe padre (+0x2100), solo para +0x1E10 (punto de aparicion)
    // partes del cuerpo (+0x1D58): banderas +0x0C de cada una (null = no existe)
    uint32_t* partFlags[4] = {nullptr, nullptr, nullptr, nullptr};
};

// Datos que se pasan a la rocas al crearlas (ENEMY_TABLE de TBoss4Iwa, 0x20 B en la pila de StartAttackIwa).
struct IwaSpawn {
    int8_t index = 0;                  // +0x00 numero de roca
    int16_t angle[3] = {0, 0, 0};      // +0x02,+0x04,+0x06 (el original deja s6 SIN inicializar: se pasa 0; no verificado)
    float srcPos[3] = {0, 0, 0};       // +0x08 punto de origen ya transformado
    float spawnPos[3] = {0, 0, 0};     // +0x20 igual con y + 80
    float scale = 6.0f;                // +0x18 (0x40C00000)
    int16_t one = 1;                   // +0x1C
    float speed = 0;                   // +0x2C kB4IwaSpeed[nivel][dificultad]
    int owner = 0;                     // +0x30 el jefe (aqui: indice del jefe que da el motor)
};

// Interfaz con el motor de estas funciones. Extiende Boss4Env (las funciones ya reescritas siguen usando la base).
struct Boss4PendEnv : Boss4Env {
    // ---- camara / aleatorios / geometria
    virtual void camTrack(float pos[3], int32_t& angle) = 0;      // *(G + G[1]*0xD0 + 0x90..0x98), +0xA0 (pista de camara actual)
    virtual float rand01() = 0;                                   // func_1A7F10: float en [0,1)
    virtual void dirXZ(int32_t angle, float& a, float& b) = 0;    // func_1B08B0(angulo 16 bits, &a, &b): a = seno, b = coseno (por uso; no verificado)
    virtual float headingOf(const float d[3]) = 0;                // func_1AF9C0(&d): rumbo del vector de desplazamiento
    virtual float groundY(const float pos[3]) = 0;                // func_951300
    virtual void turnToward(const float point[3], int step) = 0;  // TurnPos -> func_951200(pos, point, step)
    virtual void screenPos(const float pos[3], float& x, float& y) { projectToScreen(pos, x, y); }   // func_1AC3A0
    // ---- pila de matrices (func_1B0834/1B0808/1B03EC/1AFC00/1AFCAC; identificadas por uso, no se ha leido su cuerpo)
    virtual void mtxPush() = 0;                                   // func_1B0834
    virtual void mtxTranslate(float x, float y, float z) = 0;     // func_1B0808
    virtual void mtxRotY(int32_t angle) = 0;                      // func_1B03EC
    virtual void mtxPoint(const float in[3], float out[3]) = 0;   // func_1AFC00 (de un punto ANPO solo se transforman x,y,z)
    virtual void mtxVector(const float in[3], float out[3]) = 0;  // func_1AFCAC
    // ---- rutas de camara
    virtual void pathSample(float frame, int pathId, uint32_t out6[6]) = 0;   // func_1CC710(frame, gp-0x7310, id, out)
    // ---- estado de tarea
    virtual void loadStateTable(const uint32_t rows[6][3]) = 0;   // copia D_009D2B00 -> D_26B430 (solo la primera vez: gp-0x7460)
    virtual int stateRun(int stateIdx) = 0;                       // func_1C6C80(this, &D_26B430[estado]) -> +0x1F22
    // ---- partes
    virtual void hitInfo(Boss4Logic::HitInfo out[2]) = 0;         // *(gp-0x7350 + 4i), byte +0x70 de la zona, *(gp-0x7348 + slot*4)
    virtual void controlSound() = 0;                              // ControlSound (pendiente)
    virtual void controlGareki() = 0;                             // ControlGareki (pendiente)
    virtual void entryCameraTarget() = 0;                         // EntryCameraTarget (pendiente: da de alta el objetivo de camara)
    virtual void entryCollision() = 0;                            // EntryCollision (pendiente: da de alta las colisiones)
    // ---- rocas
    virtual int spawnIwa(const IwaSpawn& s) = 0;                  // func_1A4100(0x210, ...) + __ct__9TBoss4Iwa; devuelve el manejador
    // ---- efectos
    virtual void fx9443A0(int a1, int a2, int a3, int t0, int t1) = 0;   // func_9443A0(this, a1, a2, a3, t0, t1) (efecto de los clones)
};

// Globales del sistema que estas funciones leen/escriben y que Boss4Globals no recoge.
struct Boss4PendGlobals {
    int8_t stateTableLoaded = 0;       // gp-0x7460 (la copia de D_009D2B00 se hace una sola vez en todo el juego)
};

struct Boss4Pending {
    // ---- bucle por fotograma ----
    static void ctrlPre(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env);          // CtrlPre (0x9C5450)
    static void ctrlPreBunshin(Boss4State& b, Boss4Ext& x, Boss4PendGlobals& pg, Boss4PendEnv& env);                   // CtrlPre_bunshin (0x9C68D0)
    static void ctrlPost(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                            // CtrlPost (0x9C5B90)
    static void ctrlPreBattle1(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env);  // CtrlPre_battle1 (0x9C5BE0)
    static void ctrlPreBattle2(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env);  // CtrlPre_battle2 (0x9C5D60)
    static void ctrlPreBattle3(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendGlobals& pg, Boss4PendEnv& env);  // CtrlPre_battle3 (0x9C5EE0)
    static void updateMotionFrame(Boss4State& b, Boss4Ext& x);                                                                      // UpdateMotionFrame (0x9C7330)
    static void controlCamera(Boss4State& b, Boss4Ext& x, Boss4Globals& g);                                                         // ControlCamera (0x9C95D0)
    static void controlEffect() {}                                                                                      // ControlEffect (0x9C9920): vacia
    // ---- posicion y altura ----
    static void controlPos(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                                             // ControlPos (0x9C8770)
    static void setHeight(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                                              // SetHeight (0x9CD060)
    static void checkDamageEnd(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendEnv& env);                        // CheckDamageEnd (0x9C9930)
    static void clearWeakHit(Boss4Ext& x);                                                                             // ClearWeakHit (0x9CAE20)
    static void turnPos(Boss4PendEnv& env, const float point[3], int step);                                            // TurnPos (0x9CD6B0)
    // ---- movimiento ----
    static void motSupSet(Boss4State& b, Boss4Ext& x, Boss4Env& env, int motion, int a, int bb);                       // MotSupSet (0x9CD230)
    // ---- ataques ----
    static void controlAttackBattle3(Boss4State& b, Boss4Ext& x, Boss4Globals& g);                                     // ControlAttack_battle3 (0x9C7CF0)
    static void startAttackIwa(Boss4State& b, Boss4Ext& x, Boss4Globals& g, Boss4PendEnv& env);             // StartAttackIwa (0x9C7FD0)
    static void startAttackKiriPre(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env, int arg);                            // StartAttackKiriPre (0x9C8330)
    static void startAttackKiri(Boss4State& b, Boss4PendEnv& env);                                                     // StartAttackKiri (0x9C8490)
    static void startAttackKiriShoukan(Boss4State& b, Boss4PendEnv& env);                                              // StartAttackKiri_shoukan (0x9C8660)
    static void getShoukanPos(const Boss4Ext& x, Boss4PendEnv& env, const uint32_t in6[6], uint32_t out6[6]);  // GetShoukanPos (0x9C94B0)
    // ---- dificultad ----
    static void downDifficultyReq(Boss4State& b);                                                                      // DownDifficultyReq (0x9CA330)
    // ---- clones ----
    static void bnStartBunshin(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                                         // BnStartBunshin (0x9C6A60)
    static void bnStartShusoku(Boss4State& b, Boss4Ext& x);                                                            // BnStartShusoku (0x9C6B10)
    static void bnStartStandby(Boss4State& b, Boss4Ext& x);                                                            // BnStartStandby (0x9C6B90)
    static void bnCheckCmd(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                                             // BnCheckCmd (0x9C6BE0)
    static void controlBunshinBattle1(Boss4State& b, Boss4PendEnv& env);                                               // ControlBunshin_battle1 (0x9C9660)
    static void controlBunshinBattle2(Boss4State& b);                                                                  // ControlBunshin_battle2 (0x9C97E0)
    static void ctrlPreBunshinBattle1(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                                  // CtrlPre_bunshin_battle1 (0x9C6C60)
    static void ctrlPreBunshinBattle2(Boss4State& b, Boss4Ext& x, Boss4PendEnv& env);                                  // CtrlPre_bunshin_battle2 (0x9C6E90)
};

}}  // namespace vn::bosspend

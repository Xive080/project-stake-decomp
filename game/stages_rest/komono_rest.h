// Resto de la logica de objetos de escenario (overlay aomod, base 0x9DF080): Komono3Task::*Ctrl por objeto, checkHit,
// KomonoTumiageColliTask, KusariTask/KomonoKusariHahenTask y CKomonoBase::bornItem.
// Sin dibujo ni colision de motor: las llamadas al motor salen como eventos (K3Out). Se apoya en KomonoLogic (game/stages/komono.h).
#pragma once
#include "../stages/komono.h"
#include <vector>
namespace vn {

// Komono3Task::checkHit (0x9ED2F0): flags en +0xC (0x2 = disparo de P1, 0x4 = disparo de P2, 0x8 = impacto).
// Devuelve true si hubo impacto; `pl` = jugador al que se atribuye (0/1; aleatorio con rand()%2 si dispararon los dos; 0 si no hay ninguno de los dos bits).
// Limpia el bit 0x8 y el bit del jugador elegido (si solo uno de los dos).
bool k3CheckHit(uint32_t& flags, RandFn rand, int& pl);

// KomonoTumiageColliTask (ELF aomod 0x9E73C0...): esfera de colision de objetos apilables. Mismos flags +0xC.
struct TumiageColli {
    uint32_t flags = 0;          // +0xC
    int closeFlag = 0;           // +0x144 (close() lo pone a 1; ctrlPre cierra la tarea)
    float pos[3] = {0, 0, 0};    // +0x2C..+0x34
    float radius = 0;            // +0xB0
    bool chkColli() const { return (flags & 8) != 0; }            // 0x9E75F0
    int  getHitPl(RandFn rand);                                    // 0x9E7630 (no limpia 0x8; limpia el bit del jugador unico)
    void clrColli() { flags &= ~8u; }                              // 0x9E76F0
    void close() { closeFlag = 1; }                                // 0x9E7710
    void setPos(float x, float y, float z) { pos[0] = x; pos[1] = y; pos[2] = z; }   // 0x9E7720
    void setRadius(float r) { radius = r; }                        // 0x9E7730
    bool ctrlPre() const { return closeFlag != 0; }                // 0x9E7740: true = func_1A2630 (cerrar tarea)
};

// Salida comun de los controles de Komono3Task: sonidos y efectos en el orden en que los emite el original.
struct K3Out {
    std::vector<int> sounds;          // func_217C30(&D_515F30, id, arg, 0, 0x40)
    std::vector<int> soundArgs;
    bool closeTask = false;           // func_1A2630
    bool showMissionComplete = false; int missionNo = -1;
    int  hitEffectPl = -1;            // func_9330D0 / func_9332F0 (efecto de impacto atribuido a este jugador)
    int  quake = 0;                   // func_21AD80(1, 0x10) (temblor) = numero de llamadas
    std::vector<int> debris;          // indice de escombro generado (secretDoor)
    bool goggleEffect = false;        // OrgDispGoggleEffect (rama sin tocar)
    void snd(int id, int arg = 0) { sounds.push_back(id); soundArgs.push_back(arg); }
};

// Contexto de un fotograma de un control de Komono3.
struct K3Ctx {
    StageSys* sys = nullptr;
    KomonoLogic::K3Globals* g = nullptr;
    CamPathNow cam;               // ruta actual de la camara (gp-0x7338)
    bool paused = false;          // PauseMgr::get(&D_500F20) > 0
    RandFn rand = nullptr;
};

// Estado propio de cada objeto sobre el K3 comun. frame = +0x274 (f32), animState = +0x1E8, cnt59C = +0x59C.
struct K3Anim {
    KomonoLogic::K3 k;
    float frame = 0;
    int animState = 0;
    int cnt59C = 0;
    int debrisCount = 0;          // +0x204 (secretDoor)
    int doorAngle = 0;            // +0x1EC (secretDoor)
    uint32_t hitFlags = 0;        // +0xC
};

struct K3Objects {
    // Secuencia comun checkGameMode / checkMissionEnd_setTrnsl / checkMissionCompleteMessageDispTiming (+ checkCloseTiming_clrBranchNo
    // + checkColli_setBranchNo en los objetos que las llaman). Devuelve true si algun paso pidio cerrar la tarea.
    static void common3(K3Anim& a, K3Ctx& c, K3Out& o);               // las tres primeras
    static void commonFull(K3Anim& a, K3Ctx& c, K3Out& o);            // + checkCloseTiming + checkColli_setBranchNo
    // nutsCtrl (0x9EE6D0): cierre por ruta de camara (+0x1D8/+0x1DC = k.closePath/closeFrame); golpe -> sonidos 0xC000E, 0xB012B;
    // tras el golpe avanza el fotograma (+1/fotograma sin pausa); suena 0xC000F en [56,57.2) y [61,62.2); a los 90 queda terminado (animState=1).
    static void nuts(K3Anim& a, K3Ctx& c, K3Out& o);
    // goldenRatCtrl (0x9EE0D0): animState 0 corre (fotograma 0..340, sonido 0x190110 cuando (int)fotograma % 90 == 60), golpeado -> 1; estado 1 sigue
    // y pasa a 2 al terminar el ciclo (>340) o cuando la ruta 0x23F llega a 65; estado 2 reposo.
    static void goldenRat(K3Anim& a, K3Ctx& c, K3Out& o);
    // secretDoorCtrl (0x9EEAF0): puerta secreta, dos golpes (el 2o = efecto + sonido 0x20025), escombros, giro por camara.
    static void secretDoor(K3Anim& a, K3Ctx& c, K3Out& o);
    // silverKinokoCtrl (0x9EF0F0), parte logica: checkGameMode + estado de mision 1->2 + cuenta atras de trnsl que cierra al llegar a 0.
    static void silverKinoko(K3Anim& a, K3Ctx& c, K3Out& o);
};

// KomonoKusariHahenTask::loop (0x9F20F0): 3 trozos de cadena que caen. Por trozo (0x40 B desde +0x40): pos +0..8, vel +0xC..0x14, gravedad +0x1C.
struct KusariPiece { float pos[3] = {0, 0, 0}; float vel[3] = {0, 0, 0}; float grav = 0; int rotA = 0, rotB = 0, rotSpdA = 0, rotSpdB = 0; };
struct KusariHahen {
    KusariPiece p[3];
    int   age = 0;             // +0x100
    float floorY = 0;          // +0x104
    float alpha = 1.0f;        // +0x108 (baja 1/30 por fotograma desde la edad 21)
};
// Devuelve true si debe cerrarse (edad >= 51).
bool kusariHahenLoop(KusariHahen& h, bool paused);

// KusariTask::ctrl (0x9F2680), parte de decision. Una cadena de n eslabones (n = +0x6C, 12 en init); un TumiageColliTask por eslabon intermedio.
struct KusariLinkEvent { int link = 0; int pl = 0; bool spawnDebris = false; int sound = 0; };
struct KusariTick {
    bool closeAll = false;             // checkEndCP: cierra los colli y la tarea
    std::vector<KusariLinkEvent> broken;
};
// `colli[i]` puede ser nullptr (eslabon ya roto). Rompe (close + puntero a null) los eslabones tocados. Sonido 0xD0018 por rotura.
KusariTick kusariCtrl(TumiageColli* colli[], int n, bool endCP, bool paused, RandFn rand);

// CKomonoBase::bornItem (0x9DF140): genera un ItemTask con el botin de esta pieza. Solo la construccion de la tabla.
// Origen: +0 = n, +4 + 4*i = objeto (int), +0x18 + 4*i = peso (float, se trunca a int).
struct BornSrc { int n = 0; int item[16] = {}; float weight[16] = {}; };
struct BornEntry { int item = 0, weight = 0; };
struct BornTable {
    int16_t kindA = 0, kindB = 0, kindC = 0;       // +0x72, +0x74, +0x76 de la tabla temporal (a2, a3, t0 de la llamada)
    float pos[3] = {0, 0, 0};                      // +0x78..+0x80
    std::vector<BornEntry> e;                      // desde +0x84, 8 B por entrada
    int tail = 0;                                  // 4º argumento (t1/ ctor arg3): ItemTask(tabla, 1, n, arg3, 0)
};
// Devuelve true si el original llegaria a construir la tarea (asignacion correcta, `allocOk`).
bool bornItem(const BornSrc& src, const float pos[3], int a2, int a3, int t0, int arg3, bool allocOk, BornTable& out);
}

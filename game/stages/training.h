// Logica del modo entrenamiento (overlay trnmod, 0x9DF080): reglas de las 12 misiones, murcielagos, texto prohibido, tiempos.
// Sin dibujo: lo que el original dibuja/suena/crea sale como TrnEvent.
#pragma once
#include "stage_common.h"
#include <vector>
#include <cstring>
namespace vn {
typedef float (*RandFFn)();   // func_1A7F10: numero en [0,1)

// Tablas de TRNMOD.BIN (extraidas por direccion virtual).
extern const int8_t kTrnSeconds[16][3];    // D_009F41A0: segundos por mision (indice mision = NN-1) y nivel (x50 = fotogramas PAL); -1 = sin tiempo
extern const int32_t kTrnNormTrn01[3];     // D_009F4BA0
extern const int32_t kTrnNormTrn02[3];     // D_009F4D60
extern const int32_t kTrnNormTrn03_04[3];  // D_009F4D70
extern const int32_t kTrnNormTrn09[3];     // D_009F4E58
extern const int32_t kTrn09Table[10];      // D_009F4560
extern const char* const kTrnBadWords[50]; // D_009F4940 -> cadenas del ELF 0x2D88EC..
extern const uint32_t kTrnLetterObj[26];   // D_009F4710 (paso 0x10): objeto de dibujo por letra A..Z
extern const int kTrnTimeDigitTbl[5];      // D_009F4510 = {0,1,3,6,8}

struct TrnEvent {
    enum Kind { SPAWN_GENERAL, SOUND, EFFECT, SPAWN_ENEMY } kind;
    int a = 0, b = 0, c = 0;       // SPAWN_GENERAL: a=codigo, b/c = parametros; SOUND: a = codigo; EFFECT: a = id
    float x = 0, y = 0, z = 0;
};

// Globales de sistema que tocan las misiones.
struct TrnGlobals {
    int  level[12] = {};        // D_502800 + mision (dificultad 0..2)
    int  levelGp72C8 = 0;       // gp-0x72C8 (nivel usado por trn_02_init)
    int  norm = 0;              // gp-0x72C0 (objetivo restante, s8)
    int  missionOver = 0;       // 0x500E32
    int  flagE34 = 0;           // 0x500E34 (trn_04: fin por tiempo)
    int  flagE37 = 0;           // 0x500E37
    int  countDF8 = 0;          // 0x500DF8 (s16)
    int  enemyAlive = 0;        // gp-0x7374 (enemigos vivos; trn_09 y trn_12)
    int  enemyAlvNum = 0;       // gp-0x7370
    int  paused = 0;            // func_1F33B0(&D_500F20) > 0
    int  targets[4] = {};       // gp-0x6F68..: secuencia objetivo de trn_02
    int  mgrFlag6F58 = 0, mgrFlag6F54 = 0, mgr6F5C = 0, mgr6F60 = 0;   // banderas de los gestores trn_05 / trn_08
};

// TrainingTask (campos por uso): +0x60 mision, +0x98.. contadores.
struct TrnTask {
    int mission = 0;            // +0x60 (NN-1)
    int c98 = 0, c9c = 0, cA0 = 0, cA4 = 0, cA8 = 0, cAC = 0;
    float f7c = 0, f80 = 0, f84[4] = {};  // trn_06: +0x84..0x90 escalas
    int  dir[4] = {};                     // trn_06: +0xA0..0xAC sentido de pulso
};

struct TrainingLogic {
    static int frames(int mission, int level) { return kTrnSeconds[mission][level] * 50; }   // x0x32; el asm usa lb -> -1 da -50
    // Inicios: trn_01_init, trn_02_init, trn_03_init, trn_04_init, trn_05_init, trn_06_init, trn_07_init, trn_09_init, trn_11_init, trn_12_init.
    static void init01(TrnTask& t, TrnGlobals& g);
    static void init02(TrnTask& t, TrnGlobals& g, RandFn rand);
    static void init03(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev);
    static void init04(TrnTask& t, TrnGlobals& g);
    static void init05(TrnTask& t, TrnGlobals& g, int level502804);
    static void init06(TrnTask& t, TrnGlobals& g, int sel502805);
    static void init07(TrnTask& t, TrnGlobals& g);
    static void init09(TrnTask& t, TrnGlobals& g);
    static void init11(TrnTask& t, TrnGlobals& g);
    static void init12(TrnTask& t, TrnGlobals& g);
    // Ticks por fotograma.
    static void tick01(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev);
    static void tick02(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev);
    static void tick03(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev);
    static void tick04(TrnTask& t, TrnGlobals& g);
    static void tick05(TrnTask& t, TrnGlobals& g);
    static void tick06(TrnTask& t, TrnGlobals& g);
    static void tick07(TrnTask& t, TrnGlobals& g, float camY, std::vector<TrnEvent>& ev);
    static void tick08(TrnTask& t, TrnGlobals& g);
    static void tick09(TrnTask& t, TrnGlobals& g, RandFn rand, std::vector<TrnEvent>& ev);
    static void tick11(TrnTask& t, TrnGlobals& g, float camY);
    static void tick12(TrnTask& t, TrnGlobals& g, std::vector<TrnEvent>& ev);
    // trn_boss: fin cuando los dos jugadores estan en modo 4, 6 o 9 (tabla ELF 0x27496C, paso 0x1C).
    static bool bossOver(int modeP1, int modeP2);
    // trn_01..: DecideBatNum (0x9...): murcielagos segun nivel de la mision 12 (D_50280B): 2 -> 45, 1 -> 30, otro -> 15.
    static int decideBatNum(int level50280B);
    // time_change: convierte fotogramas (50/s) al numero que muestra el marcador (decenas de minuto/min/seg/...); ver .cpp
    static int timeChange(uint32_t frames);
    // Trn08KomonoMgrTask::moji2obj / checkBadWord
    static uint32_t moji2obj(char c);
    static bool checkBadWord(const uint32_t* obj, int n);
    // trn_free (0x9...): efectos sobre banderas de desbloqueo (la funcion hace llamadas a motor 0x2192D0 / 0x219650: set/clear de ids).
    struct FreeOp { bool set; int id; };
    static std::vector<FreeOp> free1(int mission);
};

// Murcielago (TrainingBat): modo 0 inicio, 1 vivo (RotPos), 2 cayendo (FallDown), 3 muerto.
struct BatState {
    int life = 1;                // +0x18
    int mode = 0;                // +0x1C6
    uint32_t flags = 0;          // +0x10 (bit1 = contado vivo, bit2 = contado)
    float y = 0;                 // +0x30
    float fallVel[3] = {};       // +0x1B4..0x1BC
    float rotA = 0, rotB = 0;    // +0x1E0 (suave hacia +0x1E4 con 0.01), +0x1E4
    int   rotC = 0, rotCStep = 0;// +0x1D8, +0x1DC
    unsigned hitMask = 0;        // +0x18C
};
struct BatLogic {
    static void setFallParam(BatState& b, RandFFn randf);            // SetFallParam
    static void fallDown(BatState& b);                               // FallDown: la velocidad vertical baja 0.08166666 por fotograma (la posicion la integra el motor)
    static void rotPos(BatState& b);                                 // RotPos (parte de estado)
    static void chkMode(BatState& b, TrnGlobals& g, RandFFn randf, std::vector<TrnEvent>& ev);   // ChkMode
};

// Komono del ejercicio 10 (CTrn10Komono): bola que cae; se golpea y rota.
struct Trn10Komono {
    float x = 0, y = 0, z = 0;   // +8, +0xC, +0x10
    float vx = 0, vy = 0, vz = 0;// +0x14, +0x18, +0x1C
    int   rotZ = 0, rotX = 0;    // +0x44, +0x40
    int   rotVZ = 0, rotVX = 0;  // +0x50, +0x4C
    float offX = 0, offZ = 0;    // +0x64, +0x6C
    int   mode = 0;              // +0x58
    int   hitPl = -1;            // +0x60
    static void setMode(Trn10Komono& k, int mode, RandFn rand);      // setMode
    static void ctrl2(Trn10Komono& k, bool paused);                  // ctrl2 (mov balistico)
    static void hitMove(Trn10Komono& k, float aimX, RandFn rand, RandFFn randf);   // hitMove: aimX = D_502600[hitPl*0x1C]
};
}

// Clases TrnKomono* del modo entrenamiento (objetos de escenario y bolas/fichas de las misiones 05, 08 y 10), reescritas a mano.
// Dos orígenes:
//  - ELF principal (asm/TrnKomono*_*.s, CTrnKomonoBase_*.s, CTrnBoxPoint_*.s): TrnKomonoTask, TrnKomonoTumiageColliTask, CTrnKomonoBase, CTrnBoxPoint.
//  - Overlay trnmod (asm/trnmod/trnmod_text.s, base 0x9DF000): CTrnKomonoBase2, CTrn05Komono, Trn05KomonoMgrTask, CTrn08Komono,
//    Trn08KomonoMgrTask, CTrn10Komono, Trn10KomonoMgrTask.
// Sin dibujo ni colision de motor: lo que el original dibuja/suena/crea sale en las estructuras de salida (eventos). Los globales de
// sistema (0x500xxx, gp-0x7xxx) se reciben como estructuras inyectadas. Cada funcion cita la original (nombre y direccion).
#pragma once
#include "../stages/stage_common.h"
#include "../stages/komono.h"
#include "../stages/training.h"
#include "../stages_rest/komono_rest.h"
#include <cstdint>
#include <vector>
namespace vn {

// Generadores de azar: nlRand (func_1A7EE0, entero) y func_1A7F10 (flotante en [0,1)). Se inyectan.
struct TrnRand {
    RandFn  i = nullptr;
    RandFFn f = nullptr;
};
// func_1B09B4 / func_1B0938 (rutinas de VU0 escritas a mano): reciben un angulo de 16 bits (0x10000 = vuelta completa) y devuelven una
// componente. Cual es seno y cual coseno NO esta verificado (no hay constantes de VU0 en el ELF): se nombran por el eje que alimentan.
struct TrnTrig {
    float (*axisX)(int angle) = nullptr;   // func_1B09B4
    float (*axisY)(int angle) = nullptr;   // func_1B0938
};

// ---------------------------------------------------------------------------------------------------------------------
// ELF principal
// ---------------------------------------------------------------------------------------------------------------------

// CTrnBoxPoint (0x21AE40): punto de caja con enlaces a otros puntos (maximo 0x20: los punteros viven en +0xA4.., las distancias en +0x24..).
struct TrnBoxPoint {
    float pos[3] = {};        // +0, +4, +8
    float pos0[3] = {};       // +0xC, +0x10, +0x14 copia inicial
    float v18[3] = {};        // +0x18, +0x1C, +0x20 (se ponen a 0 en init; uso desconocido)
    float linkDist[32] = {};  // +0x24 + 4*i distancia al punto enlazado
    TrnBoxPoint* link[32] = {}; // +0xA4 + 4*i
    int linkCount = 0;        // +0x124
    void init();                                                  // init__12CTrnBoxPointFv (0x21AE40)
    void initPos(float x, float y, float z);                      // initPos__12CTrnBoxPointFfff (0x21AE70)
    // setLink__12CTrnBoxPointFP12CTrnBoxPoint (0x21AEA0): enlace en ambos sentidos con la distancia 3D (sqrt = func_1B0AA0 inyectado).
    void setLink(TrnBoxPoint& other, float (*sqrtFn)(float));
};

// CTrnKomonoBase (0x21AF60): tabla de puntos de fin = {n, ids, fotogramas}. Capacidad 5 (el layout es +4..+0x14 ids y +0x18..+0x28 fotogramas).
struct TrnKomonoBase {
    int   n = 0;              // +0
    int   id[5] = {};         // +4
    float frame[5] = {};      // +0x18
    // initEndCP__14CTrnKomonoBaseFPii (0x21AF60): `src` = n pares {id, fotograma (int)}; el original copia n pares sin acotar a 5 (aqui se acota).
    void initEndCP(const int* src, int count);
    // checkEndCP__14CTrnKomonoBaseFv (0x21B170): igual que KomonoLogic::checkEndCP (ruta actual == id y fotograma >= tabla).
    bool checkEndCP(const CamPathNow& cam) const;
};

// TrnKomonoTumiageColliTask (0x21B1F0): esfera de colision. Reusa TumiageColli (mismos flags +0xC, mismos metodos que KomonoTumiageColliTask).
struct TrnTumiageColli {
    TumiageColli c;           // flags +0xC, closeFlag +0x144, pos +0x2C, radio +0xB0
    int   enClName = 0;       // +0xD8 nombre de la clase de colision de enemigo
    int   enClActive = 0;     // +0x14C 1 tras setEnClName (se actualiza la colision en ctrlPre)
    // __ct__ (0x21B1F0): flags = 1 | 0x10000, radio = 8.0, cerrar = 0, enClActive = 0, pos/rot a 0.
    void ctor();
    // setEnClName__25TrnKomonoTumiageColliTaskFUi (0x21B510): flags |= 0x51, &= ~0x10000; guarda el nombre; activa la colision de enemigo.
    void setEnClName(int name);
    // ctrlPre (0x21B5E0): devuelve true si hay que cerrar la tarea (TaskClose); el original sigue y actualiza la colision (evento del llamador).
    bool ctrlPre() const { return c.closeFlag != 0; }
};

// TrnKomonoTask (0x21F240): pieza rompible del entrenamiento; disparo -> 10 puntos + impacto -> se rompe (escombros/efecto/sonido por tabla).
struct TrnKomonoTask {
    uint32_t flags = 0;       // +0xC (0x2 = disparo P1, 0x4 = disparo P2, 0x8 = impacto, 0x10000)
    float pos[3] = {};        // +0x2C, +0x30, +0x34
    int   rot[3] = {};        // +0x50, +0x54, +0x58
    float radius = 0;         // +0xB0
    int   objId = 0;          // +0x16C identificador del objeto (grupo<<16 | indice)
    int   objId2 = 0;         // +0x170 segundo objeto a dibujar (0 = ninguno)
    float yOffset = 0;        // +0x178 desfase Y de la esfera de colision
    int   hasCollision = 0;   // +0x17C bandera de la tabla (1 = actualizar colision en ctrlPost)
    TrnKomonoBase endCP;      // +0x140
};
// Entradas de tabla (ver trnkomono_tables.cpp).
struct TrnKomonoRadius { uint32_t id; float radius; float yOff; int collide; };
struct TrnKomonoBreak  { uint32_t id; int16_t kind; int16_t model; int16_t fx; float fxScale; int32_t sound; };
constexpr int kTrnKomonoRadiusN = 45;
constexpr int kTrnKomonoBreakN = 43;
extern const TrnKomonoRadius kTrnKomonoRadius[kTrnKomonoRadiusN];
extern const TrnKomonoBreak  kTrnKomonoBreak[kTrnKomonoBreakN];

// Globales de sistema que usa TrnKomonoTask (modelo del modo entrenamiento).
struct TrnKomonoSys {
    int gameState = 0;        // 0x500F14 (s16); 0xA = juego normal
    int gameKind = 0;         // gp-0x73B4; 2 = entrenamiento
    int missionKind = 0;      // gp-0x73D8 (s8); 6 = (entrenamiento con contador de objetivos)
    int mission = 0;          // gp-0x72C4 (s8)
    int norm = 0;             // gp-0x72C0 (s8): objetivos restantes
};
// Eventos de TrnKomonoTask.
struct TrnKomonoEvent {
    enum Kind { CLOSE_TASK, ADD_SCORE, ADD_HIT, SPAWN_DYNAMICS, SPAWN_DISH, SPAWN_FCV, SPAWN_FCV_AT, SOUND, COLLISION_UPDATE } kind;
    int a = 0, b = 0;
    float f = 0;
};
struct TrnKomonoOut {
    std::vector<TrnKomonoEvent> ev;
    void add(TrnKomonoEvent::Kind k, int a = 0, int b = 0, float f = 0) { TrnKomonoEvent e; e.kind = k; e.a = a; e.b = b; e.f = f; ev.push_back(e); }
};
struct TrnKomonoLogic {
    // Segundo objeto asociado (init__13TrnKomonoTaskFP11ENEMY_TABLE, 0x21F4B0): 0 si no tiene.
    static int secondObj(int objId);
    // Fila de la tabla de radios para el objeto (o nullptr: el original deja radio = D_002C3264 = 8.0, desfase = D_002C3268 = 4.0, bandera 0).
    static const TrnKomonoRadius* findRadius(int objId);
    // Fila de la tabla de rotura (o nullptr).
    static const TrnKomonoBreak* findBreak(int objId);
    // init (0x21F4B0): flags = 1|0x10000, objId2, radio/desfase/bandera de tabla.
    static void init(TrnKomonoTask& t);
    // __ct__ (0x21F240) / __dt__ (0x21F3D0): el contador de objetivos sube/baja con el objeto 5 del entrenamiento tipo 6 (gameKind 2, missionKind 6, mision 5).
    static void ctorCounter(const TrnKomonoSys& s, int8_t& norm);
    static void dtorCounter(const TrnKomonoSys& s, int8_t& norm);
    // ctrl (0x21F6F0): cierra si se llego al punto de fin de camara.
    static void ctrl(const TrnKomonoTask& t, const CamPathNow& cam, TrnKomonoOut& o);
    // ctrlPost (0x21F790): si hay impacto: puntos (10) al jugador atribuido, +1 impacto, limpia el bit 8 y se rompe (close).
    static void ctrlPost(TrnKomonoTask& t, const TrnKomonoSys& s, RandFn rand, TrnKomonoOut& o);
    // close (0x21F8E0): efectos de rotura segun tabla y efectos especiales por objeto; cierra la tarea.
    static void close(const TrnKomonoTask& t, const TrnKomonoSys& s, TrnKomonoOut& o);
};

// ---------------------------------------------------------------------------------------------------------------------
// Overlay trnmod
// ---------------------------------------------------------------------------------------------------------------------

// CTrnKomonoBase2 (0x9EDC90..0x9EDD70): base de las bolas/fichas; +0x30 objeto, +0x34 vida (0 = muerta), +0x3C escala.
struct TrnKomonoBase2 {
    int   obj = 0;            // +0x30
    int   life = 0;           // +0x34
    float pos[3] = {};        // +8, +0xC, +0x10
    float scale = 1.0f;       // +0x3C
    int   getObj() const { return obj; }            // getObj__15CTrnKomonoBase2Fv (0x9EDC90)
    void  getPos(float out[3]) const { out[0] = pos[0]; out[1] = pos[1]; out[2] = pos[2]; }   // getPos (0x9EDCA0)
    int   getLife() const { return life; }          // getLife (0x9EDCC0)
    // __ct__ (0x9EDD70): vida 0, escala 1.0, resto 0. El colisionador se crea en el motor (func_21B1F0).
    void ctor() { *this = TrnKomonoBase2(); }
};

// Globales del modo entrenamiento que tocan las bolas de la mision 05/08/10.
struct TrnBallSys {
    int  paused = 0;          // func_1F33B0(&D_500F20) > 0
    int  mission = 0;         // gp-0x72C4 (s8): indice de mision (NN-1)
    int  level[12] = {};      // D_502800 + mision: dificultad 0..2
    int  norm = 0;            // gp-0x72C0 (s8): objetivos restantes
    int  missionOver = 0;     // 0x500E32
    int  flagE37 = 0;         // 0x500E37
    int  gp6F58 = 0;          // gp-0x6F58: gestor 05 activo
    int  gp6F5C = 0;          // gp-0x6F5C: disparo de temblor
    float gp6F60 = 0;         // gp-0x6F60: intensidad de temblor (fotogramas, 48 al empezar)
    int  gp7408 = 0, gp7404 = 0;   // gp-0x7408 / gp-0x7404: desplazamiento de camara por temblor
    int  gp6F54 = 0;         // gp-0x6F54: gestor 08 activo
    int  gp6F4C = 0;         // gp-0x6F4C: estado del gestor 10 (0 = en curso, 1 = tiempo casi agotado, 2 = terminado)
    int  gp6F50 = 0;         // gp-0x6F50: gestor 10 en marcha
};
struct TrnBallEvent {
    enum Kind { SOUND, HIT_EFFECT, QUAKE, CAM_ATTR, COLLI_POS, COLLI_CLEAR, COLLI_RADIUS, CLOSE_TASK, LETTER_EFFECT } kind;
    int a = 0, b = 0;
    float x = 0, y = 0, z = 0;
};
struct TrnBallOut {
    std::vector<TrnBallEvent> ev;
    void add(TrnBallEvent::Kind k, int a = 0, int b = 0, float x = 0, float y = 0, float z = 0) {
        TrnBallEvent e; e.kind = k; e.a = a; e.b = b; e.x = x; e.y = y; e.z = z; ev.push_back(e);
    }
    int count(TrnBallEvent::Kind k) const { int n = 0; for (auto& e : ev) n += e.kind == k; return n; }
};

// CTrn05Komono (0x9EDF70..0x9EE50C, 0x58 B): bola/blanco que cae. idx 0 = objetivo (0x69000C); 1..5 = senuelos.
struct Trn05Komono {
    TrnKomonoBase2 b;
    float vel[3] = {};        // +0x14, +0x18, +0x1C
    float grav = 0;           // +0x24 aceleracion Y por fotograma
    int   frame = 0;          // +0x38 fotograma de animacion (0..14)
    int   rot[2] = {};        // +0x40, +0x44
    int   rotSpd[2] = {};     // +0x4C, +0x50
    // Estado del colisionador (func_21B3D0 / 21B400 / 21B4C0): el motor lo rellena antes de ctrl.
    bool  colliHit = false;   // hubo impacto
    int   colliPl = 0;        // jugador que acerto (func_21B400)
    // init__12CTrn05KomonoFif (0x9EDFB0). idx = -1 -> senuelo al azar (1..5). speed = argumento f12.
    void init(float speed, int idx, const TrnRand& r, TrnBallOut& o);
    void addVel(float x, float y, float z) { vel[0] += x; vel[1] += y; vel[2] += z; }    // 0x9EE1B0
    void addPos(float x, float y, float z) { b.pos[0] += x; b.pos[1] += y; b.pos[2] += z; }   // 0x9EE1E0
    // ctrl__12CTrn05KomonoFv (0x9EE210)
    void ctrl(TrnBallSys& s, const TrnRand& r, const TrnTrig& t, TrnBallOut& o);
};

// Trn05KomonoMgrTask (0x9EE5A0 / 0x9EE730): 8 objetivos (+0x40) y 32 senuelos (+0x300, 0x58 B cada uno).
struct Trn05Mgr {
    Trn05Komono target[8];
    Trn05Komono decoy[32];
    int tick = 0;             // +0xE00
    int next = 0;             // +0xE04 siguiente fila de calendario
    int16_t sched[15] = {};   // +0xE08
    // __ct__ (0x9EE5A0): senuelos iniciales al azar (5 %) y calendario kTrn05Sched[i] + rand()%10.
    void ctor(const TrnRand& r, TrnBallSys& s, TrnBallOut& o);
    // ctrl (0x9EE730)
    void ctrl(TrnBallSys& s, const TrnRand& r, const TrnTrig& t, TrnBallOut& o);
};

// CTrn08Komono (0x9EF340..0x9EF7E4, 0xA4 B): ficha-letra que vuela a su sitio (interpolacion con aceleracion) y gira al ser golpeada.
struct Trn08Komono {
    TrnKomonoBase2 b;
    int   rot = 0;            // +0x44
    int   rotSpd = 0;         // +0x50
    float xs = 0, xe = 0, xt = 0, xdur = 0, xv = 0, xa = 0;   // +0x58, +0x5C, +0x60, +0x64, +0x68, +0x6C (X: inicio, fin, tiempo, duracion, velocidad, aceleracion)
    float ys = 0, ye = 0, yt = 0, ydur = 0, yv = 0, ya = 0;   // +0x70, +0x74, +0x78, +0x7C, +0x80, +0x84
    int   interp = 0;         // +0x88
    float elapsed = 0, total = 0;   // +0x8C, +0x90
    float fin[3] = {};        // +0x94, +0x98, +0x9C posicion final
    int   hitPl = -1;         // +0xA0
    bool  colliHit = false; int colliPl = 0;
    // init__12CTrn08KomonoFifffffff (0x9EF380): objeto, destino (x,y,z), inicio (sx,sy,sz), duracion.
    void init(int obj, float x, float y, float z, float sx, float sy, float sz, float dur, TrnBallOut& o);
    // setInterpolate (0x9EF450)
    void setInterpolate(float x, float y, float z, float sx, float sy, float sz, float dur);
    // checkColli (0x9EF510): true si hubo impacto (guarda jugador, limpia la colision, giro 0x3200).
    bool checkColli(TrnBallOut& o);
    int  getHitPl() const { return hitPl; }       // 0x9EF570
    void clrColli(TrnBallOut& o) { o.add(TrnBallEvent::COLLI_CLEAR); }      // 0x9EF580
    // ctrl (0x9EF5B0)
    void ctrl(const TrnBallSys& s, TrnBallOut& o);
};

// Trn08KomonoMgrTask (0x9EF9E0 / 0x9F00D0): 29 fichas, 5 palabras de 1..3 letras a escribir en orden.
struct Trn08Mgr {
    Trn08Komono tile[29];
    int   wordLen[5] = {};            // +0x17E4
    uint32_t word[5][16] = {};        // +0x12E4 + 0x40*w : objetos de las letras de cada palabra
    uint32_t typed[16] = {};          // +0x17F8 letras acertadas de la palabra actual
    int   typedCount = 0;             // +0x18F8
    int   wordIdx = 0;                // +0x18FC
    int   state = 0;                  // +0x1900: 0 jugando, 1 deslizando palabras, 2 anuncio, 3 giro tras palabra completa
    int   counter = 0;                // +0x1904
    float timer = 0;                  // +0x1908
    int   frame = 0;                  // +0x1914
    int   spinAngle = 0, spinVel = 0; // +0x1928, +0x192C
    float wordPos[5][16][3] = {};     // +0x1424 + 0xC0*w + 0xC*j: posicion (x,y,z) de cada letra mostrada
    // __ct__ (0x9EF9E0): coloca las fichas, genera las palabras (sin palabras prohibidas) y su disposicion. level = D_502800[mision].
    void ctor(int level, const TrnRand& r, TrnBallOut& o);
    // ctrl (0x9F00D0): sin dibujo (putBg y los bucles de dibujo no se modelan).
    void ctrl(TrnBallSys& s, const TrnRand& r, TrnBallOut& o);
};
struct Trn08TileHome { uint32_t obj; float x, y, z; };
extern const Trn08TileHome kTrn08Home[29];
extern const int16_t kTrn08WordLen[3][5];

// CTrn10Komono (0x9F0B40..0x9F15B4, 0x540 B): bola del ejercicio 10. Extiende Trn10Komono (game/stages/training.h) con el resto de campos.
struct Trn10KomonoFull {
    Trn10Komono k;                    // x,y,z, vel, rot, modo (setMode / ctrl2 / hitMove ya reescritas en stages/training)
    int   obj = 0;                    // +0x30
    int   life = 0;                   // +0x34
    int   hitAttr = 0;                // +0x5C atributo del ultimo impacto (D_50260C + jugador*0x1C)
    float shake = 0;                  // +0x68 (a 0 en ctor)
    int   markIdx = 0;                // +0xB0 indice del anillo de marcas
    int   markOn[16] = {};            // +0x4C0 marca activa
    int   markAttr[16] = {};          // +0x500 atributo de cada marca
    bool  hasCollider = true;         // +0x4 distinto de null
    bool  colliHit = false; int colliPl = 0;
    // init__12CTrn10KomonoFifff (0x9F0BF0): objeto, posicion, radio 15.0, vida 1.
    void init(int objId, float x, float y, float z, TrnBallOut& o);
    // checkColli (0x9F0C40): hitPl = -1; si hay impacto: jugador, atributo (de playerAttr[pl]) y limpia. `playerAttr` = D_50260C + pl*0x1C.
    bool checkColli(const int* playerAttr, TrnBallOut& o);
    int  getHitColliAttr() const { return hitAttr; }     // 0x9F0CD0
    int  getHitPl() const { return k.hitPl; }            // 0x9F0CE0
    void addPos(float x, float y, float z) { k.x += x; k.y += y; k.z += z; }   // 0x9F0CF0
    float getPosY() const { return k.y; }                // 0x9F0D20
    void setVel(float x, float y, float z) { k.vx = x; k.vy = y; k.vz = z; }   // 0x9F1080
    // ctrl (0x9F1330): modo 0 -> ctrl1 (solo dibujo y colision), modo 1 -> ctrl2 (fisica balistica ya reescrita).
    void ctrl(bool paused, TrnBallOut& o);
    // closeColli (0x9F1580): libera el colisionador.
    void closeColli(TrnBallOut& o) { hasCollider = false; o.add(TrnBallEvent::CLOSE_TASK); }
    // setHitMark (0x9F0D30), parte de estado: ring de 16 marcas.
    void setHitMark(int attr);
};

// Trn10KomonoMgrTask (0x9F1630): seleccion de las bolas "de adorno" y las que hay que tirar. Solo el __ct__ (ctrl 0x9F1730.. pendiente).
struct Trn10MgrInit {
    int count = 0;                    // +0x5FA4 (0x10 para dificultad 0/1, 0x12 para 2)
    int flag[18] = {};                // 1 = bola que NO cuenta (0x6E001F), 0 = bola objetivo (0x6E0020)
    int objectives = 0;               // numero de bolas objetivo (se suma a gp-0x72C0)
    float topY = 0;                   // +0x5F98 = kTrn10TopY - 9.0
    static Trn10MgrInit make(int level, const TrnRand& r);
};
extern const float kTrn10Pos[18][3];
extern const float kTrn10TopY[2];
extern const uint32_t kTrn05Obj[6];
extern const float kTrn05Scale[6], kTrn05GravMul[6], kTrn05Radius[6];
extern const int16_t kTrn05Sched[15];
}

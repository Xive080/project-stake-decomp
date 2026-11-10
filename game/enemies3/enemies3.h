// Clases En* auxiliares de GAMEMOD (EnEtcThrowObj, EnEtcTask, EnEtcBone, EnEtcAttach, EnDamSphere, WeakTask, ArmorTask,
// PetitZombieTask, EnVtxEffCtrl, EnemyMgrClass, utilidades EnmID*/EnSndReq/...) reescritas a mano. Cada funcion cita la original
// con su direccion (GAMEMOD, vram). Campos nombrados por uso con su offset en comentario.
// Globales de sistema (gp-0x7xxx, 0x500xxx) viven en bss y NO estan en el ELF: entran como `Env3` inyectado.
// Las llamadas a motor (sonido, dibujo, matrices de camara, colisiones, alta/baja de tareas) NO se implementan: salen como
// eventos `Ev3` o pasan por la interfaz abstracta `Camera3`. Sin rutas absolutas ni dependencias de plataforma.
#pragma once
#include <cstdint>
#include <vector>
namespace vn { namespace en3 {

struct V3 { float x = 0, y = 0, z = 0; };

// ---- Eventos de motor (el motor los ejecuta en orden) ----------------------------------------------------------------
enum Ev3Type : int {
    E3_SOUND,         // func_217C30(&D_515F30, a = id, 0, 0, 0x40): sonido global
    E3_SNDREQ,        // EnSndReq(a = id, b = emisor -1): sonido con limitador (ver enSndReq)
    E3_HITNUM,        // func_213070(a = jugador): AddPlayerHitNum
    E3_DAMAGE_PLAYER, // DecPlayerLife_Chk(a = jugador, b = 1, c = tipo de dano)
    E3_TASK_CLOSE,    // func_1A2630(): cierra la tarea
    E3_FREE_BUF,      // func_1A1F00(buffer de partes +0x1BC - 0x10): libera el buffer
    E3_ALLOC_BUF,     // func_1A1E30(a = bytes) + func_1C6CC0 (+ func_1E7890 por parte): reserva buffer de partes (a = bytes, b = n partes)
    E3_EFFECT,        // new_EnEff(a = codigo, b = parametro, pos v)
    E3_SPAWN_HIT,     // func_1E12B0(struct {0, 5, yaw, pitch, 0, pos, a = codigo}): efecto de impacto (b = yaw, c = pitch)
    E3_NEW_PARTS,     // new_EnEtcParts(task, a, b, c = modo, d = t0, e = t1, f = t2)
    E3_NEW_ATTACH,    // new_EnEtcAttach(matriz de la parte, a = id de armadura)
    E3_CAM_ENTRY,     // func_1C97D0(100000.0, camara, 0, pos v): entrada en camara
    E3_COLLI_XFORM,   // func_1B00E0/1B0138/1AFD54: el motor transforma la colision de la tarea con la matriz de camara
    E3_ENTRY_SUB,     // entryEventSub(a = indice de palabra del guion)
    E3_SNOW,          // new_SnowParticle(v, a = indice, b = bandera)
    E3_MOTION,        // func_1FA640(a = animacion 0x43, b = 0)
    E3_CALL,          // llamada de motor sin reglas propias: a = identificador (ver enum Call3)
    E3_MOTION_SET,    // cambio de animacion (a = id)
    E3_PART_FLAGS     // part.flags120 |= a (parte del enemigo duenyo)
};
enum Call3 : int { C3_PETIT_HIT = 0x1A3F60, C3_PETIT_CAM = 0x1A3F80 };
struct Ev3 { Ev3Type type; int a, b, c, d, e, f; V3 v; };
typedef std::vector<Ev3> Ev3s;
inline void emit3(Ev3s& q, Ev3Type t, int a = 0, int b = 0, int c = 0, int d = 0, int e = 0, int f = 0, V3 v = V3()) {
    Ev3 x = {t, a, b, c, d, e, f, v}; q.push_back(x);
}

// Camara activa (gp-0x7338 -> tabla de 0xD0 B por camara, indice en +4). Las dos matrices las aplica el motor:
//   worldToView = func_1B00E0(cam + 4*0xD0 + 0x10) + func_1AFC00;  viewToWorld = (cam + 0x50) + func_1AFC00.
struct Camera3 {
    virtual ~Camera3() {}
    virtual V3 worldToView(const V3& w) const = 0;
    virtual V3 viewToWorld(const V3& v) const = 0;
    virtual V3 normalize(const V3& v) const = 0;   // func_1AF9FC (aparece en nlFastInvertMatrix: normalizar un vector; inferido)
    int16_t yaw = 0;       // +0x9C
    int16_t pitch = 0;     // +0xA0
    int     pathId = 0;    // +0xD0
    float   frame = 0;     // +0xC0
};

// Estado de limitador de EnSndReq: 12 B por entrada (D_9C39A0), solo se usa +4 (ultimo fotograma).
struct SndSlot { int lastFrame = 0; };

// Entorno global inyectado.
struct Env3 {
    int16_t coop = 0;        // 0x500DFC: 1 = coop (un solo objetivo central)
    int     solo = 0;        // gp-0x73F4: jugador del modo de un jugador
    float   focal = 1;       // gp-0x7330: distancia focal
    int     attackSlot[2] = {0, 0};  // gp-0x7420 / gp-0x741C: el jugador i tiene un objeto lanzado en vuelo
    int8_t  attackGate = 0;  // gp-0x7360
    int16_t missCount = 0;   // gp-0x7358
    int16_t mode7218 = 2;    // gp-0x7218 -> +4
    int     frame739C = 0;   // gp-0x739C: contador de fotogramas (limitador de EnSndReq)
    int8_t  soundOn = 1;     // gp-0x73D8: !=0 -> suena (EnSndReq) / constantes de rebote del hueso
    int     freeze = 0;      // func_1F33B0(&D_500F20): >0 = pausa/congelado
    int16_t onScreen = 0;    // gp-0x7374 (enemigos en pantalla)
    int     gameState = 0;   // 0x500F14 (0xA arcade, 0xF = sin colisiones)
    int     gameKind = 0;    // gp-0x73B4
    float   ground = 0;      // gp-0x73A8: valor "sin suelo"
    float   groundHeight = 0;// gp-0x760C: resultado de EN_HighGet (suelo bajo la tarea)
    int    (*rand)() = nullptr;      // nlRand() = func_1A7EE0
    double (*rand01)() = nullptr;    // nlRandom() = func_1A7F10 en [0,1)
    const Camera3* cam = nullptr;
};

// Bits de +0xC (flags) de las tareas En*.
enum F3 : uint32_t {
    F_PLAYER0 = 0x2, F_PLAYER1 = 0x4, F_HIT = 0x8, F_NOCOLLI = 0x8000, F_FROZEN_ANIM = 0x4000, F_HITTED = 0x01000000,
    F_DEAD = 0x04000000, F_BIT10 = 0x10, F_RESPONDED = 0x40000000, F_GATE = 0x20000000
};

// ======================================================================================================================
// Utilidades
// ======================================================================================================================
// EnmIDInit/EnmIDSet/EnmIDClear (0x93xxxx): tabla de 32 punteros D_501310.
struct EnmIdTable {
    static const int N = 32;
    uint32_t slot[N];           // D_501310: id de la tarea o 0
    EnmIdTable() { init(); }
    void init();                                  // EnmIDInit
    // EnmIDSet: tarea con +0x10 (flags2) y +0x24 (id, s8/u8). Devuelve el id o 0xFF (-1) si esta llena.
    int set(uint32_t taskId, uint32_t& flags2, int32_t& idField);
    void clear(uint32_t& flags2, int32_t idField);  // EnmIDClear
};

// Enemy_Set_ColliBuff (0x92CD90): registra la colision de una tarea en el buffer del fotograma.
struct ColliTask {
    uint32_t id = 0;
    uint32_t flags = 0;       // +0xC
    float    viewZ = 0;       // +4 -> +0x38 (= +0x64 de la tarea): z en camara de la posicion
    V3       collPos;         // coll(+0)->+8..+0x10 (posicion de la esfera de colision)
    int8_t   hit70 = 0, hit71 = 0;  // coll->+0x70/+0x71 (se ponen a 0)
};
struct ColliEntry { uint32_t task; uint32_t flags; V3 pos; };      // 0x14 B en D_9B9000
struct ColliBuffer { ColliEntry e[0xFF]; int count = 0; /* gp-0x7004 */ };
bool enemySetColliBuff(ColliTask& t, int gameState, ColliBuffer& buf, Ev3s& out);

// Enemy_Set_WaitPosBuff (0x936170): registra una posicion de espera (D_9C0C00, 12 B: dist*10 como int, tarea, wait pos).
struct WaitPosEntry { int32_t distTimes10; uint32_t task; uint32_t waitPos; };
struct WaitPosBuffer { WaitPosEntry e[14]; uint32_t count = 0; /* gp-0x6FE4 */ };
// `distToCam` = func_1B0AA0(dx*dx + dz*dz) (raiz), calculada por el motor a partir de la pos de la tarea y la de la camara (+0x90/+0x98).
bool enemySetWaitPosBuff(uint32_t taskFlags, uint32_t taskId, uint32_t waitPos, float distToCam, WaitPosBuffer& buf);

// EnSndReq (0x9426E0): limitador de sonidos. Tabla D_009857A0 (ids, -1 final). Si el id esta, solo suena si han pasado >= 5 fotogramas.
extern const int32_t kEnSndTable[6];
bool enSndReq(int id, Env3& env, SndSlot* slots /* >= 5 */, Ev3s& out);

// EntryRankingBufferSub (0x9xxxxx): codigo de caracter del nombre del ranking (indice 0..40 -> ASCII).
int entryRankingChar(int idx);

// EnPartsChain (0x93B1E0): la parte se desprende en cadena: crea dos EnEtcParts (modo 2 y modo 3).
void enPartsChain(Ev3s& out, uint8_t a1, uint8_t a2, int8_t a3, int8_t t0, int8_t t1);

// EnemyMgrClass::entryEvent* (guion de aparicion). El guion es un vector de palabras; entryEventSub lo resuelve el motor.
// entryEvent: recorre desde script[idx+1] hasta -1 llamando a entryEventSub; devuelve el indice de la palabra tras el -1 + 1.
int mgrEntryEvent(const std::vector<int32_t>& script, int idx, Ev3s& out);
// entryEvent2: script[idx+1] = modo (0 = MENM, 1 = RAND); devuelve indice siguiente.
int mgrEntryEvent2(const std::vector<int32_t>& script, int idx, const Env3& env, Ev3s& out);
int mgrEntryEvent2Menm(const std::vector<int32_t>& script, int idx, const Env3& env, Ev3s& out);
int mgrEntryEvent2Rand(const std::vector<int32_t>& script, int idx, const Env3& env, Ev3s& out);

// ======================================================================================================================
// EnEtcTask (tareas de escenario): rampa del tiempo, bolas de fuego, marca de rama
// ======================================================================================================================
struct EtcTask {
    uint32_t flags = 0;       // +0xC
    int32_t  p14 = 0;         // +0x14 (id de la marca)
    int32_t  v50 = 0, v54 = 0;// +0x50/+0x54 (parametros del constructor)
    int32_t  a148 = 0, a14C = 0, a150 = 0, a154 = 0, a158 = 0;  // +0x148..+0x158
    int8_t   state = 0;       // +0x160
    int32_t  w16C = 0;        // +0x16C
    float    f170 = 0;        // +0x170
    V3       pos;             // +0x2C
    int32_t  unk14 = 0;       // +0x14
};
// St4_02TimerBunki (0x9xxxxx): cuenta atras de `v50*50/60` fotogramas; al acabar cuenta (missCount) si hay enemigos en pantalla.
void etcSt4TimerBunki(EtcTask& t, Env3& env, Ev3s& out);
// EnEtc_St4_Tama: fundido de la esfera (alpha +0x18 del objeto de pantalla). `fade` = estructura de gp-0x7440.
struct St4Fade { float alpha = 0; int32_t u24 = 0, u28 = 0; };
void etcSt4Tama(EtcTask& t, St4Fade& fade, const Env3& env, Ev3s& out);
// EnEtcST3_SC02_02FireBall: solo la maquina de estados (movimiento por camino `pathPos(frame)` = func_1CC710, entrada en `fireballPos`).
// Para otros tipos que no sean 6/7 el original deja la posicion sin inicializar; aqui se conserva la anterior.
typedef V3 (*PathFn)(int type, int frame, void* ctx);   // func_1CC710(frame, camino, 0x229 (tipo 6) / 0x22A (tipo 7))
void etcFireBall(EtcTask& t, PathFn path, void* ctx, const Env3& env, Ev3s& out);
// EnEtc_BranchMark (0x9xxxxx): marca de bifurcacion del modo practica. `flagsTable` = D_502708[id] (bytes por marca).
struct BranchEnv { int gameState = 0, gameKind = 0; uint8_t flagsTable[32] = {0}; int camPath = 0; float camFrame = 0;
                   int8_t d500F05 = 0; int16_t d73CC = 0; int8_t d500E11 = 0; };
// a14C/a150 = (camino, fotograma) en que se activa; a154/a158 = (camino, fotograma) en que se cierra.
void etcBranchMark(EtcTask& t, BranchEnv& e, Ev3s& out);

// ======================================================================================================================
// Objetos pequenos
// ======================================================================================================================
// EnEtcAttach::Ctrl_Pre (0x9xxxxx): pieza de armadura suelta que cae y rebota.
struct AttachObj {
    uint32_t flags = 0;       // +0xC
    V3 pos, vel, acc;         // +0x2C, +0x38, +0x44
    int32_t rot[3] = {0,0,0}; // +0x50..+0x58
    int32_t rotVel[3] = {0,0,0}; // +0xBC..+0xC4
    int32_t timer = 0;        // +0xAC
    float alpha = 1;          // +0xB4
    float col[3] = {0,0,0};   // +0xC8..+0xD0
    float colStep[3] = {0,0,0};// +0xD4..+0xDC
};
void attachCtrlPre(AttachObj& o, Env3& env, float groundY, Ev3s& out);

// EnDamSphere::Ctrl_Pre: esfera de dano ligada a una parte; traspasa los impactos al enemigo duenyo.
struct DamOwner { uint32_t flags = 0; uint32_t flags2 = 0; };    // +0xC, +0x10
struct DamSphere { uint32_t flags = 0; };
void damSphereCtrlPre(DamSphere& s, DamOwner& owner, Ev3s& out);

// WeakTask::Ctrl_Pre: punto debil.
struct WeakPart { float f_D8 = 0; uint32_t flags120 = 0; };      // parte (0xF0 B): +0xD8, +0x120
struct WeakOwner { uint32_t flags = 0; WeakPart part[18]; uint32_t weakSlot[32]; /* owner+0 tabla de tareas debiles */ };
struct WeakTask {
    uint32_t id = 0;
    uint32_t flags = 0;       // +0xC
    int32_t slot = 0;         // +0x144
    int32_t partIdx = 0;      // +0x14C
    float viewZ = 0;          // +0x64
    uint32_t state = 0;       // +0x164
    int16_t counter = 0;      // +0x198
    uint32_t animCur = 0, animStart = 0, animEnd = 0; // +0x19C, +0x1A0, +0x1A4
    uint32_t w = 0;           // +0x1A8
    int8_t result = 0;        // +0x1AC
};
void weakCtrlPre(WeakTask& t, WeakOwner& owner, const Env3& env, Ev3s& out);

// ArmorTask: armadura que se suelta de una parte.
struct ArmorPart { uint32_t flag154 = 0; int16_t cnt168 = 0; int32_t a0 = 0; int32_t c8 = 0; };
struct ArmorTask { int32_t armorId = 0; /* +0x150 */ };
// ComeOff (0x9xxxxx): se desprende: pide la pieza suelta y cuenta una menos, cierra.
void armorComeOff(ArmorTask& a, ArmorPart& part, Ev3s& out);
// PreDisp: solo si la parte no esta marcada con 0x100.
void armorPreDisp(ArmorTask& a, ArmorPart& part, Ev3s& out);
// NotDisp: `tableValue` = D_009AE640[tipo][parte*0x18 + c8*4] (inyectado).
void armorNotDisp(ArmorTask& a, ArmorPart& part, int32_t tableValue, Ev3s& out);

// PetitZombieTask::Ctrl (parte de reglas): limites del area y giro.
struct PetitArea { int pathId = 0; int frame = 0; float xMin = 0, xMax = 0, zMin = 0, zMax = 0; };  // area(+0x28): +2,+4,+8,+0xC,+0x10,+0x14
struct PetitZombie {
    uint32_t flags = 0;       // +0xC
    int32_t  p18 = 0;         // +0x18
    float    f3C = 0;         // +0x3C (tiempo)
    int32_t  mode = 0;        // +0x1D78
    float    x = 0, z = 0;    // pos (+0x30 -> +0 y +8)
    int32_t  ang = 0;         // +0x30 -> +0x28 (angulo 0..0xFFFF)
};
void petitCtrl(PetitZombie& z, const PetitArea& area, const Env3& env, Ev3s& out);

// EnEtcBone::Ctrl_Pre: hueso que se suelta de un muerto y cae.
struct BoneObj {
    uint32_t flags = 0;       // +0xC
    V3 pos, vel, acc;         // +0x2C, +0x38, +0x44
    int32_t rot[3] = {0,0,0}; // +0x50..+0x58
    int32_t rotVel[3] = {0,0,0}; // +0x118, +0x11C, +0x120
    int32_t state = 0;        // +0x10C
    float alpha = 1;          // +0x110
    float ground = 0;         // +0x124
    float partHeight = 0;     // part+0x154 (altura de la parte)
};
struct BoneInput {            // lo que calcula el motor con las matrices
    bool partDead = false;    // part.flags120 & 0x04000000
    bool ownerBit400 = false; // owner.flags2 & 0x400
    float partY = 0;          // sp44: y de la parte en camara (estado 1) / pos.y + 10 (otros)
    V3 p50, p60;              // posicion de la parte y del centro del duenyo (estado 2)
    int32_t angles[3] = {0,0,0}; // func_1C9290: angulos desde la matriz de la parte
};
void boneCtrlPre(BoneObj& b, bool ownerDead, BoneInput& in, Env3& env, Ev3s& out);

// EnVtxEffCtrl: efectos de vertices (particulas D_9C0D70, 0x1C B por peticion, 0x190 como maximo).
struct VtxEff {               // 0x84 B (ENETCVTXEFF)
    V3 pos;                   // +4, +8, +0xC
    V3 vel;                   // +0x10..+0x18
    V3 acc;                   // +0x1C..+0x24
    int32_t landed = 0;       // +0x3C
    int32_t life = 0;         // +0x40
    float col[3] = {0,0,0};   // +0x48, +0x4C, +0x50
    float colStep[3] = {0,0,0}; // +0x58, +0x5C, +0x60
    float scale = 0, scaleStep = 0; // +0x64, +0x68
    int32_t stage = 0;        // +0x6C
    float stageLife = 0;      // +0x70
    int32_t fireFrame = 0;    // +0x74
    int32_t fireMax = 0;      // +0x78
    int32_t kind = 0;         // +0x80 (0 libre, 1 particula, 2 fuego)
};
extern const float kVtxColorKey[5][4];   // D_00985430: {fraccion de vida, r, g, b}; la quinta fila es el terminador -1
struct VtxCtrl { int32_t active = 0; /* +0x34 */ };
// Ctrl_Prtc (0x93B2xx): particula (solo reglas; el dibujo es del motor).
void vtxCtrlPrtc(VtxEff& p, VtxCtrl& c, const Env3& env, uint32_t gp733C, uint32_t gp7380);
// Ctrl_Fire: fuego (animacion por fotogramas).
void vtxCtrlFire(VtxEff& p, VtxCtrl& c, const Env3& env);
// EnVtxEffReqBufInit: pone a 0 el tipo de las 0x190 peticiones.
void vtxReqBufInit(int32_t* kinds, int n);

// ======================================================================================================================
// EnEtcThrowObj (objetos lanzados al jugador: cuchillos, hachas, bolas, ...)
// ======================================================================================================================
struct ThrowOwner { uint32_t flags2 = 0; /* +0x10 */ int16_t live = 0; /* +0x14 */ };
struct ThrowObj {
    uint32_t flags = 0;       // +0xC
    V3 pos;                   // +0x2C
    V3 vel;                   // +0x38
    V3 view;                  // +0x5C (pos en camara)
    int32_t rotY = 0;         // +0x54
    int32_t rotZ = 0;         // +0x58
    float   radius = 0;       // +0xB0
    uint32_t animCur = 0, animStart = 0, animEnd = 0;   // +0x148, +0x14C, +0x150
    int32_t timer = 0;        // +0x154
    int32_t total = 0;        // +0x158
    int32_t state = 0;        // +0x15C
    float   alpha = 1;        // +0x160
    float   scale = 0;        // +0x164
    float   scaleStep = 0;    // +0x168
    int32_t w16C = 0;         // +0x16C
    int32_t player = 0;       // +0x170
    int32_t rotSpeed = 0;     // +0x178
    V3 startPos;              // +0x180
    V3 target;                // +0x18C
    int32_t coop = 0;         // +0x198
    int32_t type = 0;         // +0x19C
    bool    hasBuf = false;   // +0x1BC != 0
    float   animT = 0;        // +0x1C0
};
// Inits: reglas (valores de campos, destino aleatorio) del constructor de cada variante.
void throwInit1(ThrowObj& o, uint32_t side, int32_t timer, ThrowOwner& owner, bool gate, Env3& env, Ev3s& out);
void throwInit2(ThrowObj& o, int32_t timer, Env3& env);
void throwInit3(ThrowObj& o);
void throwInit5(ThrowObj& o, uint32_t n, int32_t total, Ev3s& out);
void throwInit6(ThrowObj& o, int32_t timer, Env3& env);
void throwInit7(ThrowObj& o, uint32_t n, int32_t total, Ev3s& out);
// Ctrl_Pre de cada variante (1 = Ctrl_Pre, 2 = Ctrl2_Pre, 6 = Ctrl6_Pre). 5, 7 y 9 quedan pendientes (ver MANIFEST).
void throwCtrlPre1(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out);
void throwCtrlPre2(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out);
void throwCtrlPre6(ThrowObj& o, ThrowOwner& owner, Env3& env, Ev3s& out);
// Ctrl_Pos de 2 y 6 (animacion + posicion en camara + entrada en camara); devuelve true si debe registrar la colision.
bool throwCtrlPos2(ThrowObj& o, const Env3& env, Ev3s& out);
bool throwCtrlPos6(ThrowObj& o, const Env3& env, Ev3s& out);
bool throwCtrlPos1(ThrowObj& o, Env3& env, Ev3s& out);

}}  // namespace vn::en3

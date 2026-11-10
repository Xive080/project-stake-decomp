// Logica de las clases Komono* de Vampire Night (overlay aomod, 0x9DF080..), reescrita a mano.
// "Komono" = objetos pequenos de escenario: destructibles que dan puntos y sueltan objetos (KomonoTask), cadena de explosion de
// colisionadores (KomonoTumiage*) y los objetos de coleccion de las misiones "Org" (Komono3Task: broche, hierba plateada, ...).
// Cada funcion cita la original (glabel de asm/aomod/aomod_text.s). Campos nombrados por uso, con su offset en comentario.
// Los globales de sistema (0x500xxx, gp-0x7xxx) viven en bss y NO estan en el ELF: se modelan como `KomonoEnv` inyectado.
// Las llamadas a motor (sonido, efectos, dibujo, colisiones, tareas) NO se implementan: salen por la interfaz abstracta `KomonoHost`.
// Sin rutas absolutas ni dependencias de plataforma.
#pragma once
#include <cstdint>
namespace vn { namespace komono {
struct ItemSpawn;

// ---------------------------------------------------------------------------------------------------------------------
// Entorno global inyectado.
struct KomonoEnv {
    int gameState = 0;        // s16 0x500F14 (0xA = arcade normal)
    int gameKind = 1;         // gp-0x73B4 (1 = modo "misiones"/entrenamiento; otro valor cierra los objetos Org)
    int camPathId = 0;        // camara actual: s16 +0xD0 de la entrada (gp-0x7338 + indice(+4) * 0xD0)
    float camFrame = 0;       // camara actual: float +0xC0 de la entrada
    int camVec[3] = {0, 0, 0};// camara actual: +0x9C/+0xA0/+0xA4 (palabras copiadas tal cual a la posicion de efecto)
    int pauseLevel = 0;       // func_1F33B0(&D_500F20): > 0 = congelado/pausado
    int msgPending = 0;       // gp-0x72E0: != 0 mientras el mensaje de mision sigue en pantalla
    uint8_t missionFlags[16] = {0}; // D_502708[mision]: bit0 = abierta, bit1 = activa, bit5 (0x20) = ya completada
    int urokoCount = 0;       // gp-0x6F00: escamas (uroko) recogidas; la quinta activa la rama
    const int16_t* dynaFrames = nullptr; // tabla de s16 en 0x1FB73E4 (fuera del ELF y de AOMOD.BIN: inyectada); indexada por id de animacion
    float nutsPos[3] = {0, 0, 0}; // gp-0x7608, gp-0x760C, gp-0x7610 (posicion del hit de la nuez; orden de direcciones decreciente como en el asm)
    int8_t branchFlag = 0;    // 0x500F05 (salida de setBranchNo/clrBranchNo)
    int8_t branchNo = 0;      // gp-0x73CC (salida de setBranchNo/clrBranchNo/Knight/NakaDoor)
};

// Interfaz con el motor. Cada metodo cita la funcion del juego que sustituye.
class KomonoHost {
public:
    virtual ~KomonoHost() {}
    virtual int  rand() = 0;                                              // func_1A7EE0 (nlRand)
    virtual void sound(int id, int a2 = 0, int a3 = 0) = 0;              // func_217C30(&D_515F30, id, a2, a3, 0x40): sonido global, volumen 0x40
    virtual void addScore(int player, int pts) = 0;                       // func_212FA0 (AddPlayerScore)
    virtual void addHitNum(int player) = 0;                               // func_213070 (AddPlayerHitNum)
    virtual void closeTask() = 0;                                         // func_1A2630 (cierra la tarea actual)
    virtual void spawnBreakTask(int typeId, const float pos[3]) = 0;      // new KomonoDishKowareTask(typeId, pos) (0x8C B)
    virtual void breakEffectA(const float pos[3], int rotY, int piece) = 0; // func_1E12B0(&{tipo 5, rotY, pos, pieza}): pedazos; (su 2o argumento es el contador sobrante del bucle, sin uso conocido)
    virtual void effect(float scale, int code, const float pos[3]) = 0;  // func_1E1620(scale, code, pos)
    virtual void groundEffect(int code, float x, float z) = 0;           // func_1E18D0(code, x, z)
    virtual void spawnItem(const ItemSpawn& s) = 0;               // new ItemTask(table, 1, itemKind, count, 0) (0x250 B)
    virtual void counterInc() = 0;                                        // KomEtcMgrInc (contador de objetos destruidos)
    virtual void registerCollider(const float pos[3], float out[3]) = 0;     // func_1AFC00(pos, this+0x5C) + func_92CD90(this): out = +0x5C..+0x64 de la tarea
    virtual void registerColliderAlt() = 0;                              // func_1AFD54(this+0xE0) + func_92CD90(this) (nueces)
    virtual void goggleEffect(const float pos[3]) = 0;                   // OrgDispSenriganGoggleEffect / OrgDispGoggleEffect
    virtual void orgHitCollect(int player, const float pos[3]) = 0;      // func_9330D0(jugador, pos): objeto Org recogido
    virtual void orgHitAgain(int player, const float eff[6]) = 0;        // func_9332F0(jugador, efecto): golpe a un Org ya recogido
    virtual void camTransform(const float in[3], float out[3]) = 0;      // func_1B00E0(camara+0x50) + func_1AFC00(in, out)
    virtual void missionMessage(int mission) = 0;                        // func_A10B00(mision): mensaje "mision cumplida"
    virtual int  dynaFind(int id) = 0;                                   // func_1FC370(gp-0x7234, id): manejador de animacion dinamica
    virtual float angA(int ang) = 0;                                     // func_1B0938 (rutina VU0 de angulo de 16 bits; cual es seno/coseno no verificado)
    virtual float angB(int ang) = 0;                                     // func_1B09B4 (idem)
    virtual void animSample(float t, int animId, float outPos[3], int32_t outRot[3]) = 0; // func_1CC710(t, gp-0x7310, id, out)
};

// ---------------------------------------------------------------------------------------------------------------------
// Tablas (komono_tables.cpp, copiadas de AOMOD.BIN).
struct KomonoTypeRow { int32_t id; float radius; float yOffset; int32_t collide; };          // D_00A093F0, 16 B, fin con id = -1
struct KomonoBreakRow { int32_t id; int16_t isItem; int16_t piece; int16_t effect; float scale; int32_t sound; }; // D_00A096D0, 0x14 B
extern const KomonoTypeRow kKomonoTypeTable[45];
extern const KomonoBreakRow kKomonoBreakTable[43];

// ---------------------------------------------------------------------------------------------------------------------
// CKomonoBase: lista de "camaras de fin" (CP) + generacion de objetos. 0x140 en las tareas que la contienen.
struct KomonoEndCP {
    // Palabras crudas: w[0] = n, w[1+k] = id de camara (s16 en la camara), w[6+k] = limite (float de initEndCP). Capacidad real 5;
    // para n > 5 el original solapa ids y limites en el mismo hueco (se replica).
    uint32_t w[24] = {0};
    int   count() const { return (int)w[0]; }
    int   id(int k) const { return (int)w[1 + k]; }
    float limit(int k) const;
};
struct ItemSpawn {            // ENEMY_TABLE sintetica de bornItem (sp+0x70, 0x14 + 8*n B) + parametros del constructor de ItemTask
    int16_t rot[3];           // +2,+4,+6
    float   pos[3];           // +8,+0xC,+0x10
    int32_t cpId[8];          // +0x14 + 8*k : ids copiados de la lista CP
    int32_t cpLimit[8];       // +0x18 + 8*k : limites truncados a entero (fptosi)
    int     count;            // numero de pares copiados (limitado a 8 aqui; en KomonoTask el objeto solo tiene hueco para 4)
    int     itemKind;         // t1 -> a3 del constructor (tipo de objeto)
};
struct CKomonoBase {
    // CKomonoBase::initEndCP(int* tabla, int n): copia n pares (id, limite->float) a la lista.
    static void initEndCP(KomonoEndCP& cp, const int32_t* pairs, int n);
    // CKomonoBase::checkEndCP: 1 si la camara actual coincide con alguna de las camaras de fin y su frame >= limite.
    static int checkEndCP(const KomonoEndCP& cp, const KomonoEnv& env);
    // CKomonoBase::bornItem(pos, rotX, rotY, rotZ, itemKind): construye la tabla y pide crear el objeto.
    static void bornItem(const KomonoEndCP& cp, const float pos[3], int rotX, int rotY, int rotZ, int itemKind, KomonoHost& h);
};

// Decodificacion del impacto comun (KomonoTumiageColliTask::getHitPl, Komono3Task::checkHit, KomonoTask::ctrlPost):
// bit 2 = disparo del jugador 0, bit 4 = jugador 1; ambos -> nlRand() % 2 (con signo de C); ninguno -> `dflt`.
// Si `consume` se limpia el bit usado (solo en las ramas de un unico jugador, como el original).
int komonoHitPlayer(uint32_t& flags, KomonoHost& h, int dflt, bool consume);

// ---------------------------------------------------------------------------------------------------------------------
// KomonoTumiageColliTask (colisionador esfera de la cadena de derribo). Campo flags +0xC: bit1 = vivo (1), 2/4 = disparo J1/J2, 8 = golpeado.
struct ColliState {
    uint32_t flags = 1;       // +0xC
    float pos[3] = {0, 0, 0}; // +0x2C,+0x30,+0x34
    float radius = 8.0f;      // +0xB0 (0x41000000)
    int   closeReq = 0;       // +0x144 (1 = cerrar en ctrlPre)
    int   zeros[6] = {0};     // +0x38..+0x4C puestos a 0 por el constructor
    int   rot[3] = {0, 0, 0}; // +0x50,+0x54,+0x58 (constructor los pone a 0 al final)
    float colliPos[3] = {0, 0, 0}; // +0x5C..+0x64 (salida de registerCollider)
};
struct ColliLogic {
    // __ct__22KomonoTumiageColliTask: parte de estado (flags = 1 | 0x10000, radio 8.0, pos desde la tabla ENEMY_TABLE +8/+C/+10).
    static void init(ColliState& s, const float tablePos[3]);
    static int  chkColli(const ColliState& s);                         // chkColli: bit 8 -> 1
    static int  getHitPl(ColliState& s, KomonoHost& h);                // getHitPl (sin bit 8; si ni bit 2 ni 4 devuelve basura del original -> 0)
    static void clrColli(ColliState& s);                               // clrColli: flags &= ~8
    static void close(ColliState& s);                                  // close: closeReq = 1
    static void setPos(ColliState& s, float x, float y, float z);      // setPos (tres floats +0x2C..)
    static void setRadius(ColliState& s, float r);                     // setRadius (+0xB0)
    static void ctrlPre(const ColliState& s, KomonoHost& h);           // ctrlPre: cierra si closeReq; registra el colisionador (+0x5C)
};

// ---------------------------------------------------------------------------------------------------------------------
// KomonoTask: objeto destructible simple.
struct KomonoState {
    uint32_t flags = 1;       // +0xC (bit 8 = golpeado, 2/4 = disparo J1/J2, 0x10000 se activa en init)
    float pos[3] = {0, 0, 0}; // +0x2C..+0x34
    int   rot[3] = {0, 0, 0}; // +0x50,+0x54,+0x58 (u16 de la tabla)
    int   typeId = 0;         // +0x16C (id de tipo, p. ej. 0x3C001C)
    int   typeAlt = 0;        // +0x170 (id "segundo estado" derivado de typeId; 0 = ninguno)
    int   itemKind = 0;       // +0x180 (objeto que suelta al romperse; 0 = ninguno; >= 0x15 muestra el efecto de gafas en entrenamiento)
    float radius = 8.0f;      // +0xB0
    float yOffset = 4.0f;     // +0x178
    int   collide = 0;        // +0x17C (1 = registra colisionador cada fotograma)
    KomonoEndCP endCp;        // +0x140
    float colliPos[3] = {0, 0, 0}; // +0x5C..+0x64 (salida de registerCollider)
};
struct KomonoLogic {
    static int  altType(int typeId);                                   // tabla de init: id -> id alternativo (0 si no hay)
    // __ct__10KomonoTaskFP11ENEMY_TABLEiii + init__10KomonoTaskFP11ENEMY_TABLE (parte de estado).
    static void init(KomonoState& s, const float tablePos[3], const uint16_t tableRot[3], int typeId, const int32_t* cpPairs, int cpCount, int itemKind);
    static void ctrl(KomonoState& s, const KomonoEnv& env, KomonoHost& h);       // ctrl__10KomonoTask
    static void ctrlPost(KomonoState& s, KomonoHost& h);                          // ctrlPost__10KomonoTask
    static void close(KomonoState& s, KomonoHost& h);                             // close__10KomonoTask
};

// ---------------------------------------------------------------------------------------------------------------------
// Komono3Task: objetos de coleccion de misiones "Org".
struct Komono3State {
    uint32_t flags = 0;       // +0xC
    float pos[3] = {0, 0, 0}; // +0x2C..+0x34
    int   modelA = 0;         // +0x144
    int   phase = 0;          // +0x1CC (0 = esperando el mensaje de mision cumplida)
    int   msgPath = 0;        // +0x1D0 (camara del aviso)
    int   msgFrame = 0;       // +0x1D4 (frame del aviso)
    int   closePath = 0;      // +0x1D8 (camara de cierre)
    int   closeFrame = 0;     // +0x1DC
    int   animState = 0;      // +0x1E8 (0 normal, 1 recogido animando, 2 terminado)
    float animTime = 0;       // +0x274
    float animPos[3] = {0};   // +0x2E8,+0x2EC,+0x2F0
    int32_t animRot[3] = {0}; // +0x2F4,+0x2F8,+0x2FC
    float basePos[3] = {0};   // +0x300,+0x304,+0x308 (desplazamiento base del pez)
    int32_t baseRot[3] = {0}; // +0x30C,+0x310,+0x314
    int   endState = 0;       // +0x590 (0 nada, 1 mensaje en curso, 2 desvaneciendo)
    float alpha = 1.0f;       // +0x594 (opacidad; baja 1/30 por fotograma en endState 2)
    int   state = 0;          // +0x598 (0 sin recoger, 1 recogido, 2 mision cumplida)
    int   animFrame = 0;      // +0x59C (fotograma de modelo cicla modulo 15 / 37)
    int   mission = 0;        // +0x5A0 (s8, indice en D_502708)
    int   pulse = 0;          // +0x5DC (fase del pulso de la seta plateada, u16)
    float colliPos[3] = {0, 0, 0}; // +0x5C..+0x64 (salida de registerCollider)
    int   sub204 = 0;         // +0x204 (hitotsu: 0 espera la camara 0x28D, 1 temporizando)
    int   lightAngle = 0;     // +0x5F8 (angulo de la luz, +0x200 por fotograma con el juego en marcha)
};
struct Komono3Logic {
    static void setBranchNo(KomonoEnv& env);                           // setBranchNo: 0x500F05 = 1, gp-0x73CC = 1
    static void clrBranchNo(KomonoEnv& env);                           // clrBranchNo: ambos a 0
    static bool checkCamPath(const KomonoEnv& env, int path, int frame); // checkCamPath: camara == path y camFrame >= frame
    static int  checkHit(Komono3State& s, KomonoHost& h, int* player); // checkHit: 1 si hay golpe (consume bit 8 y bit de jugador)
    static void calcEffPos(const Komono3State& s, const KomonoEnv& env, KomonoHost& h, float out[6]); // calcEffPos
    static void checkColli_setBranchNo(Komono3State& s, KomonoEnv& env, KomonoHost& h); // checkColli_setBranchNo
    static void checkMissionEnd_setTrnsl(Komono3State& s, const KomonoEnv& env);        // checkMissionEnd_setTrnsl
    static void checkGameMode(Komono3State& s, KomonoEnv& env, KomonoHost& h);          // checkGameMode
    static void checkMissionCompleteMessageDispTiming(Komono3State& s, const KomonoEnv& env, KomonoHost& h); // idem
    static void checkCloseTiming_clrBranchNo(Komono3State& s, KomonoEnv& env, KomonoHost& h);
    static void lightCtrl(Komono3State& s, const KomonoEnv& env);                       // lightCtrl (solo el avance del angulo)
    static void orgCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);                // orgCtrl (cadena comun; dibujo omitido)
    static void silverGrassCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);        // silverGrassCtrl
    static void goldenBornCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);         // goldenBornCtrl
    static void goldenRatCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);          // goldenRatCtrl
    static void goldenFishCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);         // goldenFishCtrl
    static void silverKinokoCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);       // silverKinokoCtrl
    static void makimonoCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);           // makimonoCtrl
    static void urokoCtrlA(Komono3State& s, KomonoEnv& env, KomonoHost& h);             // urokoCtrlA
    static void urokoCtrlB(Komono3State& s, KomonoEnv& env, KomonoHost& h);             // urokoCtrlB
    static void hitotsuCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);            // hitotsuCtrl
    static void kinokoCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);             // kinokoCtrl
    static void nutsCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);               // nutsCtrl
    static void maboroshiCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);          // maboroshiCtrl (= cadena orgCtrl + 2 dibujos fijos, omitidos)
    static void broochCtrl(Komono3State& s, KomonoEnv& env, KomonoHost& h);             // broochCtrl (= orgCtrl + 3 dibujos fijos, omitidos)
};

// Parametros de animacion dinamica (Komono3Task::setDynaParam, offsets 0x15C/0x23C; Komono2Task::setDynaParam, 0x1C/0x2C: misma logica).
struct DynaSlots { int handle[8] = {0}; int frames[8] = {0}; };
struct DynaLogic {
    static void setDynaParam(DynaSlots& d, int idx, int id, const KomonoEnv& env, KomonoHost& h);
    // Komono2Task::checkCloseCamPath(path, frame): cierra la tarea cuando la camara es `path` y su frame >= frame.
    static void checkCloseCamPath(const KomonoEnv& env, int path, float frame, KomonoHost& h);
};

// Puertas/jefe de fase 2 y 3 (Komono3Task).
struct NakaDoorState {          // st2NakaDoor*
    uint32_t flags = 0;         // +0xC (bit 8 = golpe)
    float pos[3] = {0};         // +0x2C
    float off[3] = {0};         // +0x150,+0x154,+0x158 (desplazamiento del colisionador)
    int   phase = 0;            // +0x1CC (0 cerrada, 1 abriendo)
    int   sub = 0;              // +0x1D0 (1 al abrir, 2 cuando la camara 0x1D0 la ve)
    int   soundDone = 0;        // +0x1D4
    int   lenA = 0, lenB = 0;   // +0x23C, +0x240 (duraciones de la animacion)
    float tA = 0, tB = 0;       // +0x274, +0x278
    float colliPos[3] = {0};    // +0x5C
};
struct KnightState {            // st3bs01KnightBunki*
    ColliState* colli[2] = {nullptr, nullptr}; // +0x2B0,+0x2B4
    float pos[2][3] = {{0}};    // +0x2E8 + 0x18*i
    int16_t rot[2][3] = {{0}};  // +0x2F4 + 0x18*i (s16 leidos con lh)
    int   phase = 0;            // +0x1CC
    int   sub = 0;              // +0x1D0
    int   first = 0;            // +0x1D4 (1 si el primero en romperse fue el 0)
};
struct StageObjLogic {
    static void nakaDoorCtrl(const KomonoEnv& env, KomonoHost& h);                  // st2NakaDoorCtrl (logica: cierra en las camaras 0x1A8/0x1AF/0x187)
    static void nakaDoorCtrlPost(NakaDoorState& s, KomonoEnv& env, KomonoHost& h);  // st2NakaDoorCtrlPost
    static void knightCtrlPre(KnightState& s, const KomonoEnv& env, KomonoHost& h); // st3bs01KnightBunkiCtrlPre
    static void knightCtrl(KnightState& s, KomonoEnv& env, KomonoHost& h);          // st3bs01KnightBunkiCtrl
};

// ---------------------------------------------------------------------------------------------------------------------
// Fragmentos de rotura (KomonoDish/Box/CrosetKowareTask y KomonoKusariHahenTask): trozos de 0x40 B con fisica simple.
struct Debris {
    float pos[3] = {0, 0, 0};     // +0,+4,+8
    float vel[3] = {0, 0, 0};     // +0xC,+0x10,+0x14
    float grav = 0;               // +0x1C (aceleracion Y por fotograma)
    int   rot[3] = {0, 0, 0};     // +0x24,+0x28,+0x2C (u16)
    int   rotVel[3] = {0, 0, 0};  // +0x30,+0x34,+0x38
    float floorY = 0;             // +0x3C (solo Croset: suelo propio de cada trozo)
};
struct DebrisLogic {
    // Un paso de los trozos de Dish/Box/Croset: rebote en el suelo (vy -= 0.9*vy, vx y vz *= 0.6, y = suelo, giros a 0), gravedad, mover, girar.
    static void stepKoware(Debris& d, float floorY);
    // Un paso de KomonoKusariHahenTask::loop (amortigua con 0.5; solo anula los giros X/Y; gira antes de mover).
    static void stepKusari(Debris& d, float floorY);
};
struct DishState {              // KomonoDishKowareTask (0x8C B)
    int   count = 0;            // +0x40 (numero de trozos)
    int   typeId = 0;           // +0x48 (0x1B0079 vive 60 fotogramas; el resto 40)
    int   frame = 0;            // +0x78
    float alpha = 1.0f;         // +0x80
    Debris* pieces = nullptr;   // +0x74
};
struct BoxState {               // KomonoBoxKowareTask
    int   count = 0;            // +0x40
    int   frame = 0;            // +0xC0
    float floorY = 0;           // +0xC4
    float alpha = 1.0f;         // +0xC8
    Debris* pieces = nullptr;   // +0x78
};
struct CrosetState {            // KomonoCrosetKowareTask
    int   count = 0;            // +0x64 (trozos que se simulan)
    int   frame = 0;            // +0x2894
    float alpha = 1.0f;         // +0x2898
    Debris* pieces = nullptr;   // +0x94
};
struct KusariState {            // KomonoKusariHahenTask: 3 trozos
    Debris pieces[3];           // +0x40 + 0x40*i
    int   frame = 0;            // +0x100
    float floorY = 0;           // +0x104
    float alpha = 1.0f;         // +0x108
};
struct DebrisTasks {
    static void dishCtrl(DishState& s, const KomonoEnv& env, KomonoHost& h);       // ctrl__20KomonoDishKowareTask
    static void boxCtrl(BoxState& s, const KomonoEnv& env, KomonoHost& h);         // ctrl__19KomonoBoxKowareTask
    static void crosetCtrl(CrosetState& s, const KomonoEnv& env, KomonoHost& h);   // ctrl__22KomonoCrosetKowareTask
    // __ct__21KomonoKusariHahenTaskFffff: lanza 3 trozos al azar desde (x, y, z); `floorY` = suelo (+0x104).
    static void kusariInit(KusariState& s, float x, float y, float z, float floorY, KomonoHost& h);
    static void kusariLoop(KusariState& s, const KomonoEnv& env, KomonoHost& h);   // loop__21KomonoKusariHahenTask
};

// ---------------------------------------------------------------------------------------------------------------------
// KomonoTumiageTask: pila de cajas hecha de 8 puntos de masa por caja (bloques de 0x12F0 B desde el inicio de la tarea).
struct TumiageBox {
    float p[8][3];                // puntos 0..7 (CBoxPoint en +0x60 + 0x128*k; solo sus 3 primeras palabras = posicion)
    float center8[3] = {0};       // +0x1320 (media de los 8 puntos)
    float center4[3] = {0};       // +0x132C (media de los puntos 4..7: posicion de dibujo)
    ColliState* colli = nullptr;  // +0x1338
    int   active = 0;             // +0x133C
    int   cooldown = 0;           // +0x1340 (baja 1 por fotograma en ctrlPost mientras > 0)
    TumiageBox() { for (int i = 0; i < 8; i++) for (int k = 0; k < 3; k++) p[i][k] = 0; }
};
struct TumiageState {
    int itemKind = 0;             // +0x40 (>= 0x15: gafas en entrenamiento)
    int count = 0;                // +0x44 (numero de cajas)
    TumiageBox* boxes = nullptr;
};
struct TumiageLogic {
    static void ctrl(TumiageState& s, const KomonoEnv& env, KomonoHost& h);   // ctrl__17KomonoTumiageTask (centros + colisionador + gafas; orientacion/dibujo omitidos)
    static void ctrlPost(TumiageState& s);                                     // ctrlPost__17KomonoTumiageTask
};
}}  // namespace vn::komono

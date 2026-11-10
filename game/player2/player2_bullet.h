// Fogonazo y estela de las balas del jugador (PlayerTask::bulletConst / PlayerTask::bullet, ELF main 0x20FA90 / 0x20FDF0), reescrito a mano.
// Cada jugador tiene dos anillos de 6 posiciones, de 0x34 B cada una, dentro de su estructura (0x348 B):
//   anillo A "fogonazo" en +0x208 + i*0x34, anillo B "estela" en +0xD0 + i*0x34; el indice del anillo esta en PlayerTask+0x1CC0 (s16).
// La parte de dibujo (PutPolyMatCE, nlSetScaleFactor, colores de fundido) no se reescribe: llega al motor como evento (BulletHost::drawMuzzle/drawTracer).
// Lo que el original lee de globales de sistema (gp-relativos, 0x500F14, 0x502704...) entra por BulletEnv. Sin dependencias de plataforma.
#pragma once
#include <cstdint>
namespace vn {

constexpr int kBulletRing = 6;             // 6 posiciones por anillo (bucle de 6 en bullet, indice vuelve a 0 al llegar a 6)
constexpr int kMuzzleLife = 10;            // el fogonazo se apaga con edad >= 10 (slti 0xA)
constexpr int kTracerLife = 50;            // la estela se apaga con edad >= 50 (slti 0x32)
constexpr int32_t kTracerSpin = 0x1000;    // giro de la estela por fotograma (+0x18 += 0x1000)
constexpr float kTracerSpeed = 20.0f;      // 0x41A00000: velocidad = direccion normalizada * 20
constexpr float kTracerFirstFrameBoost = 30.0f;  // 0x41F00000: con edad == 1 la estela avanza velocidad*30
constexpr float kMuzzleDepth = -1.0f;      // 0xBF800000
constexpr uint32_t kMuzzleDepthScaleBits = 0xBD4D35A8;   // factor (-0.0501f) que multiplica la mira normalizada del fogonazo

// Estilo de dibujo del fogonazo. El valor es la direccion del bloque del original al que salta la tabla jtbl_002C2E20 (ELF 0x2C2E20, 8 entradas
// para el arma 8..15); las armas fuera de 8..15 usan el bloque por defecto.
enum MuzzleStyle : uint32_t {
    MS_DEFAULT = 0x20FEB0,   // un solo sprite, escala 0.042 (armas 11, 13 y las que no son 8..15)
    MS_W10     = 0x20FF0C,   // un solo sprite, escala 0.021 (arma 10)
    MS_W8_9    = 0x20FF68,   // arma 8 y 9: sprite + 4 capas con color de fundido (tabla D_002C2DD0)
    MS_W12     = 0x210148,   // arma 12: giro con (fase<<12 + edad<<11)
    MS_W14     = 0x210214,   // arma 14: brillo cuya escala depende de la edad
    MS_W15     = 0x210374    // arma 15: color segun fase (0..3)
};
MuzzleStyle muzzleStyleFor(int weapon);   // jtbl_002C2E20: (arma - 8) sin signo < 8 -> tabla; si no, MS_DEFAULT

// Posicion del anillo A (fogonazo), 0x34 B.
struct MuzzleSlot {
    int16_t active = 0;          // +0
    float x = 0, y = 0, z = 0;   // +4, +8, +0xC  (direccion de la mira * factor)
    int32_t angA = 0, angB = 0;  // +0x10, +0x14 (angulos de GetAngleYX; angA negado, angB + 0x8000)
    int32_t age = 0;             // +0x28
    int32_t weapon = 0;          // +0x2C (arma equipada, 0 si no aplica)
    int32_t phase = 0;           // +0x30 (gp-0x739C % 4)
};

// Posicion del anillo B (estela), 0x34 B.
struct TracerSlot {
    int16_t active = 0;          // +0
    float x = 0, y = 0, z = 0;   // +4, +8, +0xC
    int32_t spin = 0;            // +0x18 (no se inicializa al crear: acumula 0x1000 por fotograma)
    float vx = 0, vy = 0, vz = 0;// +0x1C, +0x20, +0x24
    int32_t age = 0;             // +0x28
    int32_t weapon = 0;          // +0x2C
    int32_t phase = 0;           // +0x30
};

struct BulletPool {
    MuzzleSlot muzzle[kBulletRing];
    TracerSlot tracer[kBulletRing];
    int16_t ring = 0;            // PlayerTask+0x1CC0
};

struct BulletEnv {
    int playerNo = 0;            // PlayerTask+0x1CB8
    int g739C = 0;               // gp-0x739C (contador de fotogramas; fase = g739C % 4, con el resto de C)
    int gameState = 0;           // 0x500F14 (lh)
    int gameKind = 0;            // gp-0x73B4
    uint32_t flags72D0 = 0;      // gp-0x72D0 (bit 0x10 = balas infinitas)
    int8_t itemSlot0 = 0;        // D_502690[jugador*4] (arma equipada, lb)
    int word502704 = 0;          // 0x502704 (arma 8 sin esta carga (<5) cuenta como 0)
    uint16_t g738C = 0;          // gp-0x738C (distinto de 0 = detener efectos)
    int g72F0[2] = {0, 0};       // gp-0x72F0 + 4*jugador (distinto de 0 = la estela muere en su primer fotograma)
    float sightScale = 1.0f;     // gp-0x7330 (divisor de la mira en pixeles)
    float sightX = 0, sightY = 0;// PlayerData+0x1C / +0x20 (mira)
};

// Llamadas al motor.
struct BulletHost {
    virtual ~BulletHost() {}
    virtual void getAngleYX(float x, float y, float z, int32_t& a, int32_t& b) = 0;          // GetAngleYX(x,y,z,&a,&b)
    // func_001B00E0 (carga la matriz de la camara activa) + func_001AFC00 (transforma el vector) + posicion de la camara (cam+0x90..0x98).
    virtual void cameraRay(const float in[3], float outWorld[3], float camPos[3]) = 0;
    virtual void drawMuzzle(MuzzleStyle style, int playerNo, int index, const MuzzleSlot& s) {(void)style;(void)playerNo;(void)index;(void)s;}
    virtual void drawTracer(int playerNo, int index, const TracerSlot& t) {(void)playerNo;(void)index;(void)t;}
};

struct BulletLogic {
    // Arma efectiva para el efecto: solo en estado 0xA, gameKind 1 y sin balas infinitas; arma 8 con word502704 < 5 -> 0.
    static int effectiveWeapon(const BulletEnv& e);
    static void bulletConst(BulletPool& b, const BulletEnv& e, BulletHost& h);   // PlayerTask::bulletConst (0x20FA90)
    static void bullet(BulletPool& b, const BulletEnv& e, BulletHost& h);        // PlayerTask::bullet (0x20FDF0): vida, movimiento y envio al dibujo
    static void clear(BulletPool& b);                                            // lo que hace startInit con los 12 flags y el indice
};
}

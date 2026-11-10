// Disparo por arma del modo Original (overlay orgmod, base 0xA0F800): PlayerTask::gunOrgExe (0xA11490, 0xA10 B),
// PlayerTask::gunOrgBulletSet (0xA11440), gunOrgSoundOff (0xA11430, vacia), DecLife (gamemod 0x9501C0),
// OrgPlWeaponDamageGet (gamemod 0x942F00) y Ray_VS_BallColli (ELF 0x1D3B0C...).
// Sin dibujo ni sonido: las llamadas a motor salen por la interfaz GunOrgHost (en los tests, un mock que las registra).
#pragma once
#include "../stages/org.h"
namespace vn {

// Estado de arma del PlayerTask (apuntado por this+0x1CBC; offsets en comentario). Nombres por uso.
struct GunOrgWork {
    int   playerMode = 0;     // +0x04 (8 = modo en el que no se gasta municion)
    float sightX = 0, sightY = 0;      // +0x1C, +0x20 mira de salida (la fija la propia funcion para el disparo)
    int   firing = 0;         // +0x24 1 si este fotograma se dispara (0 = se suprime)
    float lastX = 0, lastY = 0;        // +0x28, +0x2C mira del ultimo disparo
    int   lastValid = 0;      // +0x30 mira valida del ultimo disparo (G.sightValid)
    float prevX = 0, prevY = 0;        // +0x34, +0x38 mira del disparo anterior (para la interpolacion de rafaga)
    int   prevValid = 0;      // +0x3C
    int   ammoPrev = 0;       // +0x48 municion al empezar (comparada al final)
    int   ammo = 0;           // +0x4C municion actual
    int   emptyFlag = 0;      // +0x50 1 = cargador vacio
    int   reloadFxFlag = 0;   // +0x54 1 = pendiente efecto de recarga
    int   w58 = 0;            // +0x58 (se limpia al recargar/vaciar; uso desconocido)
    int   reloadReq = 0;      // +0x5C 1 = pedir recarga (se pone a 1 cuando el gatillo es efectivo y no hay mira valida y cambio la municion)
    int   shotsA = 0;         // +0x64 contador de disparos
    int   shotsB = 0;         // +0x6C contador de disparos (identico a +0x64 en este codigo)
    int8_t cool = 0;          // +0x340 fotogramas de espera hasta el siguiente disparo (decrece 1 por fotograma, minimo 0)
    int8_t phase = 0;         // +0x341 fase dentro de la rafaga / patron del arma
    int8_t burst = 0;         // +0x342 1 = rafaga activa
    int8_t spreadDone = 0;    // +0x343 (objeto 10: 1 tras la primera dispersion; en 13/11 no se toca)
    int8_t cnt344 = 0;        // +0x344 contador mod 4 (armas 13/11)
    int8_t flag346 = 0;       // +0x346 marca "bala especial" (objetos 8 y 9); la lee OrgPlWeaponDamageGet_core / DecLife
};

// Una entrada de GunMgr por jugador (D_500580 + 100*pl): solo lo que lee gunOrgExe.
struct GunOrgGun { int sightValid = 0; /* +0xC */ int8_t trigger = 0; /* +0xE */ };

// Globales que consulta gunOrgExe.
struct GunOrgSys {
    int  pl = 0;                 // this+0x1CB8
    int  flags72D0 = 0;          // gp-0x72D0 (bit 0x10 = balas infinitas)
    int  active7418 = 0;         // gp-0x7418 (0 = juego parado: se anula la rafaga)
    int  gp7218_4 = 0;           // *(gp-0x7218)+4 (byte): 2 = rama de casquillo/animacion de disparo
    int  gp738C = 0;            // gp-0x738C (u16)
    int  stage = 0;              // gp-0x73D8 (s8; 7 = no se pide recarga)
    uint8_t* d50064C = nullptr;  // 0x50064C: mascara |= 1 << (6-pl) (puede ser nullptr)
    int8_t equip0 = 0;           // D_502690[pl*4]: arma equipada
    int32_t* silver = nullptr;      // 0x502704
    int32_t* silverTotal = nullptr; // gp-0x72DC
};

// Interfaz al motor (llamadas del original; cada una indicada con su direccion).
struct GunOrgHost {
    virtual ~GunOrgHost() {}
    virtual int  rand() = 0;                               // nlRand 0x1A7EE0
    virtual void preUpdate() = 0;                          // func_20FA30(this)
    virtual void reloadFx() = 0;                           // func_210C60(this)
    virtual void setSight(int pl, float x, float y) = 0;   // func_1D39F0(pl, x, y)
    virtual void postUpdate() = 0;                         // func_20FA90(this)
    virtual void playSound(int id) = 0;                    // func_217C30(&D_515F30, id, 0, 0, 0x40)
    virtual void shellEject(int pl, int ammoIdx) = 0;      // func_2106D0(this, vec D_51F350[pl][min(ammo,8)])
    virtual void shotEnd() = 0;                            // func_210CE0(this)
    virtual bool canContinueBurst(int pl) = 0;             // func_202170(pl) != 0
    virtual void ammoDisplay() = 0;                        // func_20F590(this)
};

// Tablas de ORGMOD.BIN: D_00A12300 (16 pares {municion inicial, espera}), jtbl 0xA12BB0 (sonido por objeto 8..15).
extern const int8_t kGunOrgTable[16][2];

struct GunOrg {
    // gunOrgBulletSet (0xA11440): reinicia el patron de disparo.
    static void bulletSet(GunOrgWork& w, GunOrgHost& h);
    // gunOrgExe (0xA11490). Devuelve nada; todo el estado sale por `w`, `sys` y las llamadas a `h`.
    static void exe(GunOrgWork& w, const GunOrgGun& g, GunOrgSys& sys, GunOrgHost& h);
    // Id de sonido de disparo (switch de 0xA12BB0 + rama por defecto). 0 = ninguno. `itemFirst` = caso del objeto 10 (suena inmediato).
    static int shotSound(int item, int pl);
};

// OrgPlWeaponDamageGet (gamemod 0x942F00): 1.0 salvo en juego arcade (0xA) en modo Original, donde vale OrgLogic::weaponDamage.
float orgPlWeaponDamageGet(const StageSys& s, int pl);

// Estado de vida del enemigo que toca DecLife (gp-0x6FD8): +0x18 vida, +0x20 vida previa, +0xC banderas (0x1000 = invulnerable).
struct DecLifeTarget { int life = 0; int prevLife = 0; uint32_t flags = 0; };
// DecLife (gamemod 0x9501C0). `pl` = gp-0x6FD4 (jugador que dispara). Si la marca 0x346 del jugador esta puesta,
// el dano es la vida entera (bala de plata/estaca).
void decLife(DecLifeTarget& e, const StageSys& s, int pl, int dmg);

// Ray_VS_BallColli (ELF 0x1D3B0C): una esfera (x,y,z) de radio r es tocada por el rayo del jugador.
// sin/cos de los angulos del rayo = D_502C40 + 0x68*pl + 0x38..0x44 (los calcula Ent_ShotRay con el motor: no se reescribe).
struct ShotRay { float sinA = 0, cosA = 1, sinB = 0, cosB = 1; };   // +0x38, +0x3C, +0x40, +0x44
bool rayVsBall(const ShotRay& r, float x, float y, float z, float radius);
}

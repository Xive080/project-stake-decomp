// Ciclo de vida de la tarea del jugador (PlayerTask, ELF main 0x20D990..0x212230) que no cubre game/src/player.cpp ni game/flow_rest:
// arranque completo (startInit con la tabla de 6 filas), *GameInit, gameLoop, espera/union/demo, y los efectos que desencadena un disparo
// (bulletConst, cartucho, iconos de balas) y una recarga. Reescrito a mano, sin dependencias de plataforma ni del hardware PS2.
// El planificador de tareas (changeExec/newExec/__ptmf_scall), el dibujo, el sonido y las consultas de creditos entran por CycleHost.
// Los globales de sistema (gp-relativos, 0x500xxx) entran por CycleSys.
#pragma once
#include <cstdint>
#include "../src/player.h"
#include "player2_bullet.h"
#include "player2_hud.h"
namespace vn {

// Rutinas a las que PlayerTask pasa el control (changeExec; direccion del campo +8 de cada ptmf, ELF 0x274B48..0x274C58).
enum class P2Exec : uint32_t {
    GameLoop    = 0x20DF60,   // D_274B98 / 274BA8 / 274BB8 / 274BC8 / 274C48
    JoinWaitLoop= 0x20EA10,   // D_274C28
    WaitLoop    = 0x20EB30,   // D_274C38
    Init        = 0x20DCC0,   // D_274B58 / D_274C58 (changePlayerMode con exec)
    CommonLoop  = 0x2121F0    // D_274B48 (slot 0 de newExec)
};

// Fila de la tabla ELF 0x274AF0 (12 B): flags, modo siguiente, valor (invulnerabilidad inicial en fotogramas). Las filas 0..5 son las que usa startInit(0..5).
struct StartRow { uint32_t flags; int mode; int value; };
constexpr int kStartRows = 6;
extern const StartRow kStartRows6[kStartRows];
enum StartKind { SK_FIRST = 0, SK_CONT = 1, SK_STAGE = 2, SK_JOIN = 3, SK_DEMO = 4 };   // 4 y 5 solo las usa demoInit (4); 5 no se ha visto usada

// Estado del jugador que leen/escriben estas rutinas: PlayerState (game/src/player.h) mas los campos que ese no tiene.
struct PlayerRound {
    int playerNo = 0;                 // PlayerTask+0x1CB8
    PlayerState p;                    // estructura de 0x348 B en 0x500730 + pl*0x348
    uint8_t flagsC8 = 0;              // +0xC8 (bit 0: el jugador esta fuera / sin efecto de dano; sin confirmar)
    uint8_t joinReqC9 = 0;            // +0xC9 (1 = pide unirse; lo pone joinWaitLoop_main)
    int reqMode94 = 0;                // +0x94 (modo al que ir tras esperar)
    int field60 = 0;                  // +0x60 (se borra en joinGameInit; significado no visto)
    int fieldB4 = 0;                  // +0xB4 (se borra en startInit)
    uint8_t shopFlags[7] = {0,0,0,0,0,0,0};   // +0x340..+0x346 (se borran en startInit si gameKind == 1)
    int dmgEff[3] = {0, 0, 0};        // D_500DC0 / D_500DC4 / D_500DCC + pl*0x1C (estado del efecto de dano; startInit los pone a 0)
    uint32_t taskFlags27FC = 0;       // PlayerTask+0x27FC (bit0 = parpadeo espera, bit3 = dibujo HUD, bit4 = union pedida)
    BulletPool bullets;               // +0xD0 / +0x208 / +0x1CC0
    BulletHud hud;                    // D_51F270 / D_51F350
    CartridgePool cartridges;         // +0x24E0
    LifeParticlePool lifeParticles;   // +0x1CC4
};

// Globales de sistema (lectura y escritura).
struct CycleSys {
    PlayerConfig cfg;                 // gameState = 0x500F14, gameKind = gp-0x73B4, objetos, vidas de servicio, tabla de balas
    int16_t credits500DF8 = 0;        // 0x500DF8 (contador de creditos/vidas de la partida; startInit lo incrementa, gameLoop lo decrementa al morir)
    int16_t credits500DFC = 0;        // 0x500DFC
    int16_t g73C6 = 0;                // gp-0x73C6 (contContinuar: -3 con piso 0)
    uint32_t flags72D0 = 0;           // gp-0x72D0 (bit0 inhibir disparo, bit1 continuar activo, bit4 = 0x10 balas infinitas)
    int g7418 = 0;                    // gp-0x7418 (disparo permitido)
    int g729C = 0;                    // gp-0x729C
    uint8_t g72A0 = 0;                // gp-0x72A0 (byte que se pone a 1)
    uint8_t g7400 = 0, g73FC = 0;     // gp-0x7400, gp-0x73FC (bytes que se ponen a 1 al morir)
    int8_t g73D8 = 0, g73D4 = 0, g73F0 = 0;   // gp-0x73D8/-0x73D4/-0x73F0 (lb)
    int g739C = 0;                    // gp-0x739C (contador de fotogramas)
    uint16_t g738C = 0;               // gp-0x738C
    int pauseLevel = 0;               // PauseMgr::get(&0x500F20)
    int screenMode = 0;               // *(s8*)(gp-0x7218 + 4)
    int32_t invuln[2] = {0, 0};       // gp-0x7410 + 4*jugador (invulnerabilidad)
    int g72F0[2] = {0, 0};            // gp-0x72F0 + 4*jugador
    int word502704 = 0;               // 0x502704
    float sightScale = 1.0f;          // gp-0x7330 (divisor de la mira en pixeles)
    bool demoPlay = false;            // is_DemoPlay()
    int gunDevice = 0;                // GetGunDevice(pl): 2/3 = GunCon
    bool gunTrigger = false;          // D_500580 + 0x64*pl + 0xE
    bool gunSightValid = false;       // +0xC
};

// Llamadas al motor / planificador.
struct CycleHost {
    virtual ~CycleHost() {}
    virtual BulletHost& bulletHost() = 0;
    virtual void changeExec(P2Exec target, int playerNo) = 0;      // SystemTask2::changeExec
    virtual void dispatchModeInit(int mode, int playerNo) = 0;     // PlayerTask::init: __ptmf_scall(D_274960 + 0x1C*modo); la tabla esta a 0 en el ELF (se rellena en ejecucion)
    virtual void setBookPlGameStart(int pl) = 0;                   // SysCfgMgr
    virtual void setBookPlGameEnd(int pl) = 0;
    virtual void setBookPlGameCont(int pl) = 0;
    virtual void resetSightPosPl(int pl) = 0;                      // GunMgr::reset_SightPosPl
    virtual void orgJoinIn(int pl, int which) = 0;                 // func_A12120 (which=0) y func_A12270 (which=1) del overlay ORGMOD
    virtual bool checkCredAndStBtn(int pl, int mode) = 0;          // game/flow_rest PlayerScreens::checkCredAndStBtn
    virtual void clrDamEff(int pl) = 0;                            // ClrPlDamEff
    virtual void dispDamEff(int pl) = 0;                           // DispPlDamEff
    virtual void dispDamEffReq(int pl) = 0;                        // DispPlDamEffReq
    virtual void dispSight(int pl) = 0;                            // dispSight_core
    virtual void gunExe(int pl) = 0;                               // PlayerLogic::gunExe + efectos (ver PlayerCycle::onShot)
    virtual void gunOrgExe(int pl) = 0;                            // func_A11490 (ORGMOD)
    virtual void gunOrgSoundOff(int pl) = 0;                       // func_A11430
    virtual void calcLifeVelX(PlayerRound& r) = 0;                 // dispLifePtclCalcVelX (usa la camara)
    virtual void drawBullets(int pl, const HudBulletOut& o) {(void)pl;(void)o;}
    virtual void drawCartridges(int pl, const CartridgePool& c) {(void)pl;(void)c;}
    virtual void drawLife(PlayerRound& r) {(void)r;}               // dispLife (solo dibujo y nacimiento de particulas)
    virtual void drawLifeParticles(int pl, const LifeParticlePool& p) {(void)pl;(void)p;}
    virtual void scoreDispCheck(int pl) = 0;                       // PlScoreDispChk
    virtual void sound(int id, int param) = 0;                     // SoundMgr::seqRequest(id, 0, 0, param)
    virtual void entryPut(int pl) = 0;                             // CPutEntryControl::Entry (commonLoop)
};

struct PlayerCycle {
    // PlayerTask::dispSight (0x20F4C0): decide si se dibuja la mira (no GunCon, sin congelar, con vida, disparo permitido y pistola activa).
    static bool sightVisible(const CycleSys& s, const PlayerRound& r);
    // PlayerTask::startInit (0x20EBE0): version completa. El de game/src/player.cpp solo cubre las filas 0..3 y no hace el resto de la inicializacion.
    // Devuelve false si `kind` no es 0..5.
    static bool startInit(PlayerRound& r, CycleSys& s, CycleHost& h, int kind);
    static void init(PlayerRound& r, CycleHost& h);                                        // PlayerTask::init (0x20DCC0)
    static void firstGameInit(PlayerRound& r, CycleSys& s, CycleHost& h);                  // 0x20DD00
    static void contGameInit(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void stageGameInit(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void joinGameInit(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void gameLoop(PlayerRound& r, CycleSys& s, CycleHost& h);                       // PlayerTask::gameLoop (0x20DF60)
    static void joinWaitInit(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void joinWaitLoopMain(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void waitInit(PlayerRound& r, CycleHost& h);
    static void waitLoop(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void demoInit(PlayerRound& r, CycleSys& s, CycleHost& h);
    static void commonLoop(PlayerRound& r, CycleHost& h);                                  // PlayerTask::commonLoop (0x2121F0)
    // Efectos que gunExe (0x20F770) desencadena al disparar: bulletConst, sonido (0x2000C / 0x2000D, parametro 0x40), cartucho y movimiento de iconos.
    // `refill` = balas infinitas (gp-0x72D0 & 0x10 o modo 8).
    static void onShot(PlayerRound& r, CycleSys& s, CycleHost& h, bool refill, RandFn rand);
    // Efecto de dispBulletInit que reloadExe y gunExe (bulletFlag54 == 1) piden al motor.
    static void onReload(PlayerRound& r);
    static BulletEnv bulletEnv(const PlayerRound& r, const CycleSys& s);
};
}

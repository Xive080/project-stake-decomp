// Pantallas de continuar, fin de partida (game over) y entrada de nombre de PlayerTask, y la pantalla de ranking de final de juego (ClearRanking*),
// reescritas a mano como maquinas de estados. Cada funcion cita el original con su direccion ELF.
// Todo lo que el original lee de globales del sistema (gp-relativos y direcciones fijas de RAM) entra por la estructura `ScreenSys`;
// todo lo que hace al motor (dibujo, sonido, cambio de rutina de la tarea, creditos) sale por la interfaz `ScreenEngine`. Sin dependencias de PS2.
// Los nombres de globales siguen el offset (gp-0x73F0 -> gp73F0); el significado de varios no esta confirmado y se indica como "(sin confirmar)".
// Se reescribe la logica de decision; lo que dibuja (DispPlDamEff, SprPut con su estructura SPR_ARG, gunExe, bullet) queda como evento al motor.
#pragma once
#include <cstdint>
namespace vn {

// Rutina a la que PlayerTask::changeExec pasa el control (tablas D_00274BD8 / 0x274BF0.. / 0x274C08 / 0x274C18 / 0x274C58, campo +8 de cada ptmf).
enum class PlExec { ContLoop, GmoverLoop, NameentLoop, ModeDispatch };

struct CreditSlot { int32_t value = 0; int32_t state = 0; };   // getCreditData(&D_5004D0, i): +0 valor, +4 estado (2 = credito suficiente en curso; sin confirmar)

// Entradas globales (lecturas) y salidas a globales (escrituras) de estas pantallas.
struct ScreenSys {
    int16_t gameState500F14 = 0;     // s16 en 0x500F14 (10 = en juego, 11 = fin; 6 = ... usado por checkCredSt; resto sin confirmar)
    int16_t nextState500F16 = 0;     // s16 en 0x500F16 (se escribe 0xA al pedir continuar, 0xB al acabar el ranking)
    int32_t gp73B4 = 0;              // gp-0x73B4 (contInit: 2 -> ir directo al modo 1; gmoverInit: 1 -> limpia bytes de jugador). Sin confirmar nombre
    int32_t gp73F0 = 0;              // gp-0x73F0 (4 = se suspende la cuenta atras; sin confirmar: parece el estado de pausa/demostracion)
    uint32_t gp72D0 = 0;             // gp-0x72D0 flags (bit 0 = continuar pedido, bit 1 = pantalla de continuar activa, bit 3 = 8, bit 4 = 0x10)
    int16_t word500DFC = 0;          // s16 en 0x500DFC (se decrementa al agotarse la cuenta atras del continuar; sin confirmar: contador de continuaciones)
    int16_t word500E00 = 0;          // s16 en 0x500E00 (gmoverInit: 1 = posicion a cero)
    int32_t gp6F90[2] = {0, 0};      // gp-0x6F90 + 4*jugador (nameentLoop: distinto de 0 = no hacer nada; sin confirmar)
    uint32_t gp73E4 = 0;             // gp-0x73E4 (contador de fotogramas de la pantalla de ranking / libro)
    int32_t gp738C = 0;              // gp-0x738C
    int32_t gp7114 = 0;              // gp-0x7114 (estado del libro IraiList: 0..3)
    int32_t gp7118 = 0;              // gp-0x7118 (1 = salir del libro, lo pone BookExit)
    int32_t gp711C = 0;              // gp-0x711C (libro abierto)
};

// Datos por jugador que lee/escribe el original (PlayerTask + su estructura en +0x1CBC)
struct PlayerScr {
    int  playerNo = 0;               // PlayerTask+0x1CB8
    uint32_t flags27FC = 0;          // PlayerTask+0x27FC (bits 2, 4 y 0x10 los pone esta logica; los consume el resto de PlayerTask)
    int32_t mode = 0;                // *(+0x1CBC)+4 modo del jugador (changePlayerMode lo escribe)
    int32_t contCount = 0;           // +0xB4 cuenta atras del continuar en unidades de 0x1000 por segundo (0x9FFF al empezar; se resta 0x1E por fotograma)
    int32_t credSnap0 = 0, credSnap1 = 0;   // +0x78, +0x7C copia de los valores de credito de los dos jugadores (si cambian, reinicia la cuenta)
    int32_t flag18 = 0;              // +0x18 (se consulta y se pone a 0 en checkCredAndStBtn)
    int32_t reqMode94 = 0;           // +0x94
    int32_t gmoverCount = 0;         // +0x74 (100 al empezar el game over)
    int32_t unk4C = 0;               // +0x4C (nameentLoop lo pone a 8)
    int32_t contActive = 0;          // +0xC9
    int32_t nameFields[12] = {};     // +0xD0.. 0x34 B entre si (12 campos puestos a cero en nameentInit: D0 104 138 16C 1A0 1D4 208 23C 270 2A4 2D8 30C)
    int32_t unk1CC0 = 0;             // PlayerTask+0x1CC0 (nameentInit: 0)
    int32_t pos[3] = {};             // PlayerTask->unk0 +0,+4,+8 (gmoverInit)
    int32_t vel[3] = {};             // PlayerTask->unk0 +0x24,+0x28,+0x2C
};
struct NameCursor { int16_t x = 0, y = 0, active = 0; };      // D_500580 + 0x64*jugador: +8 x, +0xA y, +0xC activo

struct ScreenEngine {
    virtual ~ScreenEngine() = default;
    virtual void changeExec(PlExec e, int player) = 0;           // SystemTask2::changeExec(+0x1CB4, ptmf+8, this)
    virtual CreditSlot creditData(int i) = 0;                    // CreditClass::getCreditData(&D_5004D0, i)
    virtual bool isFreePlay() = 0;                               // CreditClass::IsFreePlay
    virtual bool creditEnough(int player, bool continuing) = 0;  // nlaCheckCreditEnough(player, continuing)
    virtual int  pauseLevel() = 0;                               // PauseMgrClass::get(&D_500F20)
    virtual bool pullTrigger(int player) = 0;                    // is_PullTrigger
    virtual bool pressReload(int player) = 0;                    // is_PressReload
    virtual bool pressStart(int player) = 0;                     // is_PressStart
    virtual bool pressDecide(int player) = 0;                    // is_PressDecide
    virtual void drawDamageEffect(int player) = 0;               // DispPlDamEff dentro de nlPush/PopMatrix y Perspective (0x3FB6DB6E, 0x477DE800, 0x2AAB)
    virtual void scoreDispCheck(int player) = 0;                 // PlScoreDispChk
    virtual void maskScreen() = 0;                               // MaskScreen
    virtual void soundSeq(uint32_t id) = 0;                      // SoundMgrClass::seqRequest(id, 0, 0)
    virtual void setBookPlGameEnd(int player) = 0;               // SysCfgMgrClass::setBookPlGameEnd
    virtual void clearPlayerBytes(int player) = 0;               // D_502692[4*p] = D_502693[4*p] = 0
    virtual void drawNameCursor(int player, float x, float y) = 0;   // SprPut con SPR_ARG (id 0x61C + jugador, textura 0x20A, escala 1)
    virtual void gunExe(int player) = 0;
    virtual void bullet(int player) = 0;
    // libro (IraiList) y ranking
    virtual void bookCover(int fade) = 0;                        // SprPut de la tapa (fade = ver MANIFEST: formula de transparencia sin trasladar)
    virtual bool bookAnimDone() = 0;                             // IraiNote::anim(..) == -1
    virtual bool bookAnimBackDone() = 0;                         // IraiNote::animback(..) == -1
    virtual void bookPutPages() = 0;                             // IraiNote::PutPageAll
    virtual void bookPut() = 0;                                  // IraiNote::putbook
    virtual void taskLoop() = 0;                                 // TaskLoop(gp-0x73A0)
};

// PlayerTask (ELF 0x20D990..)
struct PlayerScreens {
    ScreenSys& sys; ScreenEngine& eng; PlayerScr& pl;
    PlayerScreens(ScreenSys& s, ScreenEngine& e, PlayerScr& p) : sys(s), eng(e), pl(p) {}
    // Posiciones iniciales de game over por jugador (ELF 0x274BF0 + 0xC*jugador; palabras de 32 bits copiadas tal cual).
    int32_t gmoverInitPos[2][3] = {};

    void changePlayerMode(int mode, bool exec);          // 0x20F020 changePlayerMode__10PlayerTaskFii
    bool checkCredSt(int mode);                          // 0x20EED0 checkCredSt__10PlayerTaskFi
    bool checkCredAndStBtn(int mode);                    // 0x20EE30 checkCredAndStBtn__10PlayerTaskFi
    void contInit();                                     // 0x20E2F0 contInit__10PlayerTaskFv
    void contLoop();                                     // 0x20E3C0 contLoop__10PlayerTaskFv
    void gmoverInit();                                   // 0x20E670 gmoverInit__10PlayerTaskFv
    void gmoverLoop();                                   // 0x20E780 gmoverLoop__10PlayerTaskFv
    void nameentInit();                                  // 0x20E820 nameentInit__10PlayerTaskFv
    void nameentLoop(const NameCursor& cur);             // 0x20E890 nameentLoop__10PlayerTaskFv (`cur` = D_500580 + 0x64*jugador)
};

// ---- Ranking de final de juego ----
// CalcClearRanking (ELF 0x2266D0): posicion (0..7) = mayor i en 1..7 con puntos >= tabla[i]; 0 si ninguna. La tabla (8 s32) es D_002C4BD0 (normal)
// o D_002C4BF0 si los s32 de 0x500738 y 0x500A80 son ambos distintos de cero (sin confirmar que son: parece modo de dos jugadores).
int calcClearRanking(int32_t score, const int32_t table[8]);
inline const int32_t* clearRankingTable(bool alt, const int32_t* normal, const int32_t* altTable) { return alt ? altTable : normal; }

struct RankingScreen {
    ScreenSys& sys; ScreenEngine& eng;
    RankingScreen(ScreenSys& s, ScreenEngine& e) : sys(s), eng(e) {}
    static constexpr uint32_t kFrames = 0x2B3;           // 691 fotogramas
    void loop();                                         // ClearRankingLoop (0x226EF0)
};
// IraiList_Loop (0x226420): maquina de estados del libro (0 portada, 1 abriendo, 2 paginas, 3 cerrando)
struct BookScreen {
    ScreenSys& sys; ScreenEngine& eng;
    BookScreen(ScreenSys& s, ScreenEngine& e) : sys(s), eng(e) {}
    void bookExit();                                     // BookExit (0x2262C0): pide sonido 0x70013 y gp7118 = 1
    void loop();                                         // IraiList_Loop
};
}

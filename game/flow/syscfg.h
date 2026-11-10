// Configuracion de sistema (SysCfgMgrClass, ELF 0x1D3500) y tabla de records (RankingMgrClass, ELF 0x215EA0), reescritos a mano.
#pragma once
#include <cstdint>
#include <cstring>
namespace vn {

// ---- RankingMgrClass ------------------------------------------------------------------------------------------
// Entrada de la tabla de records (12 B): iniciales ASCII (3), relleno 0, s32 +4 (0..3 en la tabla de fabrica; segun el uso, "fase alcanzada"/nivel),
// s32 +8 puntuacion. La tabla A vive en 0x502AFC y la B en 0x502B74 (10 entradas cada una).
struct RankEntry { char name[3]; uint8_t pad; int32_t stage; int32_t score; };
static_assert(sizeof(RankEntry) == 12, "RankEntry");
constexpr int kRankSlots = 10;
extern const RankEntry kRankInitA[kRankSlots];   // ELF 0x28C180
extern const RankEntry kRankInitB[kRankSlots];   // ELF 0x28C200
struct RankTables { RankEntry a[kRankSlots]; RankEntry b[kRankSlots]; };

struct RankingMgr {
    // RankingMgrClass::setInitTbl (0x215EA0): copia las dos tablas de fabrica a las de trabajo.
    static void setInitTbl(RankTables& t);
    // RankingMgrClass::checkRankIn (0x215F60): 1 = el jugador entra en la tabla, -1 = no.
    // `gameKind` = gp-0x73B4 (1 = tabla B, otro = tabla A); `score` = jugador+0xC; `playerMode` = jugador+4 (9 = no puede entrar).
    static int checkRankIn(const RankTables& t, int gameKind, int32_t score, int playerMode);
};

// ---- SysCfgMgrClass -------------------------------------------------------------------------------------------
// Bloque de configuracion de servicio (0x502900, 0x2F B) copiado desde la tabla ELF 0x266BA0 por clrAll.
constexpr int kSysCfgSize = 0x2F;
extern const uint8_t kSysCfgDefault[kSysCfgSize];   // ELF 0x266BA0
struct SysCfgBlock {
    uint8_t raw[kSysCfgSize];
    int initLife() const { return (int8_t)raw[1]; }   // 0x502901 (lo lee PlayerTask::setInitLife)
    int maxLife() const { return (int8_t)raw[2]; }    // 0x502902 (lo lee PlayerTask::setMaxLife)
};

struct SysCfgMgr {
    int32_t f4 = 0, f8 = 0, fC = 0, f10 = 0, f14 = 0;   // +4 +8 +0xC +0x10 +0x14 (estado de la sincronizacion de tarjeta de memoria)
    int8_t lang = 0;          // +0x45: idioma interno del juego
    int8_t langFlag43 = 0;    // +0x43: 1 si el idioma interno es 4 (aleman en la correspondencia de abajo)
    // setLang (0x1D3620). `scfLanguage` = sceScfGetLanguage() (0 jp, 1 ingles, 2 frances, 3 espanol, 4 aleman, 5 italiano, ...).
    // Correspondencia del original: 5->5, 4->4, 3->6, 2->3, resto (incl. 1)->2.
    void setLang(int scfLanguage);
    // clrAll (0x1D36D0): copia la configuracion por defecto, fija idioma y rellena los records. Devuelve el bloque.
    void clrAll(SysCfgBlock& cfg, RankTables& rank, int scfLanguage);
    // init2: clrAll + a cero de +8,+4,+0x10,+0x14 + synch. (sceMcInit lo hace el motor)
    void init2(SysCfgBlock& cfg, RankTables& rank, int scfLanguage, int mcSyncRet, int mcSyncOut);
    // synch (0x1D3560): `mcSyncRet` = sceMcSync(1,0,&+8); `mcSyncOut` = valor que deja en +8. Devuelve true si imprimio el codigo de resultado.
    bool synch(int mcSyncRet, int mcSyncOut);
};
}

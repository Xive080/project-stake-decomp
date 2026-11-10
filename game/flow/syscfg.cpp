#include "syscfg.h"
namespace vn {
// ELF 0x28C180 / 0x28C200 (10 x 12 B)
const RankEntry kRankInitA[kRankSlots] = {
    {{'T','F','F'},0,3,40000},{{'J','I','N'},0,3,36000},{{'Y','K','Y'},0,2,32000},{{'H','O','R'},0,1,28000},{{'A','K','Z'},0,1,24000},
    {{'O','G','A'},0,0,20000},{{'E','F','I'},0,0,16000},{{'W','A','T'},0,0,12000},{{'T','A','G'},0,0,8000},{{'K','A','T'},0,0,4000}};
const RankEntry kRankInitB[kRankSlots] = {
    {{'H','A','T'},0,3,40000},{{'O','M','O'},0,3,36000},{{'H','R','D'},0,2,32000},{{'N','M','Y'},0,1,28000},{{'T','O','M'},0,1,24000},
    {{'I','Z','I'},0,0,20000},{{'K','R','O'},0,0,16000},{{'T','A','K'},0,0,12000},{{'F','U','J'},0,0,8000},{{'S','U','Z'},0,0,4000}};

void RankingMgr::setInitTbl(RankTables& t) {                                      // setInitTbl__15RankingMgrClassFv
    for (int i = 0; i < kRankSlots; i++) { t.a[i] = kRankInitA[i]; t.b[i] = kRankInitB[i]; }
}

int RankingMgr::checkRankIn(const RankTables& t, int gameKind, int32_t score, int playerMode) {   // checkRankIn__15RankingMgrClassFi
    int32_t floor10 = (gameKind == 1) ? t.b[kRankSlots - 1].score : t.a[kRankSlots - 1].score;   // 0x502BE8 / 0x502B70
    if (score < floor10) return -1;
    return playerMode == 9 ? -1 : 1;
}

// ELF 0x266BA0 (0x2F B)
const uint8_t kSysCfgDefault[kSysCfgSize] = {8,3,5,0,5,0,0,0,1,0,5,3,0,192,0,56,0,3,6,4,1,0,5,3,0,192,0,56,0,3,6,4,255,230,230,0,0,0,0,0,0,0,0,0,0,0,0};

void SysCfgMgr::setLang(int l) {                                                  // setLang__14SysCfgMgrClassFv
    if (l == 5) lang = 5;
    else if (l == 4) lang = 4;
    else if (l == 3) lang = 6;
    else if (l == 2) lang = 3;
    else lang = 2;                                                                 // 1 y cualquier otro
    langFlag43 = (lang == 4) ? 1 : 0;
}

void SysCfgMgr::clrAll(SysCfgBlock& cfg, RankTables& rank, int scfLanguage) {      // clrAll__14SysCfgMgrClassFv
    std::memcpy(cfg.raw, kSysCfgDefault, kSysCfgSize);
    setLang(scfLanguage);
    RankingMgr::setInitTbl(rank);
}

bool SysCfgMgr::synch(int ret, int out) {                                          // synch__14SysCfgMgrClassFv
    fC = ret; f8 = out;
    if (fC != 0 && f14 != 0) {
        f14 = 0;
        if (f8 != 0) { f10 = f8; return true; }                                    // printf("...%d", +0x10)
    }
    return false;
}

void SysCfgMgr::init2(SysCfgBlock& cfg, RankTables& rank, int scf, int ret, int out) {   // init2__14SysCfgMgrClassFv
    clrAll(cfg, rank, scf);
    f8 = 0; f4 = 0; f10 = 0; f14 = 0;
    synch(ret, out);
}
}

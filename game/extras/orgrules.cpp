#include "orgrules.h"
#include <algorithm>

int OrgRules::missionOver(const OrgMissionCtx& c) {
    if (c.gameMode != 1) return 0;
    int best = std::max(0, std::max(c.playerScore[0], c.playerScore[1]));
    auto ok = [](int8_t f) { return (f & 1) && (f & 2) && !(f & 0x20); };
    if (c.stageMode == 10 && ok(c.flagsA)) return best < 30000 ? 0 : 1;
    if (c.stageMode == 10 && ok(c.flagsB)) return c.counter < 1000 ? 0 : 2;
    if (c.stageMode == 10 && ok(c.flagsC)) return best < 140000 ? 0 : 3;
    return 0;
}

bool OrgRules::dispScoreJoinIn(int8_t modeByte, int8_t slot1Item) {
    return modeByte != 7 && slot1Item == 0x12;
}

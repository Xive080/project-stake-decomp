#include "komono.h"
namespace vn {

bool KomonoLogic::checkEquipItem(const StageSys& s, int slot, int item) {
    if (s.gameState != 0xA || s.gameKind != 1) return false;
    for (int pl = 0; pl < 2; pl++) {
        int m = s.pl[pl].mode;
        if (m >= 7 || m == 2) continue;                  // tabla jtbl 0x A0B590: solo el modo 2 salta el test; >= 7 sale
        if (s.equip[pl][slot] == item) return true;       // el original indexa D_502690 + slot + pl*4
    }
    return false;
}

int KomonoLogic::calcItemNo(const StageSys& s, const ItemTbl& t, RandFn rand) {
    int n = t.count, res = 0;
    if (checkEquipItem(s, 1, 0x13)) res = 14;
    else if (checkEquipItem(s, 1, 0x14)) res = 16;
    else if (n > 0) {
        if (t.kind == 0) {
            int item[16], w[16], hasHigh = 0;
            int roll = rand() % 100;
            for (int i = 0; i < n; i++) { item[i] = t.e[i].item; w[i] = t.e[i].weight; if (item[i] >= 0x15) hasHigh = 1; }
            if (hasHigh == 1 && (checkEquipItem(s, 1, 0x16) || checkEquipItem(s, 1, 0x17))) {
                int bonus = checkEquipItem(s, 1, 0x16) ? 10 : 25;
                for (int i = 0; i < n; i++) if (item[i] >= 0x15) w[i] += bonus;
                int sum = 0;
                for (int i = 0; i < n; i++) sum += w[i];
                if (sum > 100) {
                    int excess = sum - 100;
                    for (int i = 0; i < n && excess; i++) {
                        if (item[i] >= 8 && item[i] < 0x15) {
                            if (w[i] >= excess) { w[i] -= excess; excess = 0; }
                            else { excess -= w[i]; w[i] = 0; }
                        }
                    }
                    if (excess != 0) {
                        int k = 0;
                        for (int i = 0; i < n; i++) if (item[i] >= 0x15) k++;
                        int sub = (int)((float)excess / (float)k + 0.5f);   // fptosi (0x14C518): truncar
                        for (int i = 0; i < n; i++) if (item[i] >= 0x15) w[i] -= sub;
                    }
                }
            }
            int prev = 0, cum = 0;
            for (int i = 0; i < n; i++) {
                cum += w[i];
                if (!(roll < prev)) { prev = cum; if (roll < cum) { res = item[i]; break; } }
                else prev = cum;
            }
        } else return -1;     // kind 1/2: break 0 en el original
    }
    if (res >= 8) {
        if (res < 0x14 && checkEquipItem(s, 1, 0x15) && (rand() % 200) < 0x19) res = 0x14;
    }
    return res;
}

bool KomonoLogic::checkEndCP(const EndCP& cp, const CamPathNow& cam) {
    for (int i = 0; i < cp.n && i < 5; i++)
        if (cam.pathId == cp.id[i] && !(cam.frame < cp.frame[i])) return true;
    return false;
}
bool KomonoLogic::checkCamPath(const CamPathNow& cam, int id, int frame) { return cam.pathId == id && !(cam.frame < (float)frame); }
bool KomonoLogic::checkCloseCamPath(const CamPathNow& cam, int id, float frame) { return cam.pathId == id && !(cam.frame < frame); }

void KomonoLogic::checkMissionEnd_setTrnsl(K3& k, const K3Globals& g, bool paused) {
    if (k.missionState == 2) {
        if (!paused) { k.trnsl -= 0.033333335f; if (k.trnsl < 0.0f) k.trnsl = 0.0f; }
    } else if (k.missionState == 1) {
        if (g.nowSettingGp72E0 == 0) k.missionState = 2;
    }
}

KomonoLogic::K3Event KomonoLogic::checkMissionCompleteMessageDispTiming(K3& k, const CamPathNow& cam) {
    K3Event ev;
    if (k.doneFlag == 0 && checkCamPath(cam, k.msgPath, k.msgFrame)) {
        ev.showMissionComplete = true; ev.missionNo = k.missionNo;
        k.doneFlag = 1; k.missionState = 1; k.branchState = 2;
    }
    return ev;
}

KomonoLogic::K3Event KomonoLogic::checkCloseTiming_clrBranchNo(const K3& k, K3Globals& g, const CamPathNow& cam) {
    K3Event ev;
    if (checkCamPath(cam, k.closePath, k.closeFrame)) { clrBranchNo(g); ev.closeTask = true; }
    return ev;
}

KomonoLogic::K3Event KomonoLogic::checkGameMode(const K3& k, const StageSys& s, K3Globals& g) {
    K3Event ev;
    if (s.gameKind != 1) ev.closeTask = true;                       // func_1A2630 (cerrar tarea) cuando gameKind != 1
    if (k.branchState == 0) {
        bool open = s.gameState == 0xA && s.gameKind == 1;
        int8_t f = (k.missionNo >= 0 && k.missionNo < 0x28) ? s.missionFlags[k.missionNo] : 0;
        open = open && (f & 1) && (f & 2) && !(f & 0x20);
        if (!open) { clrBranchNo(g); ev.closeTask = true; }
    }
    return ev;
}

KomonoLogic::K3Event KomonoLogic::onHit(K3& k, K3Globals& g, bool hit) {
    K3Event ev;
    if (!hit) return ev;
    if (k.branchState == 0) { setBranchNo(g); k.branchState = 1; ev.sound = 0xB012B; }
    else if (k.branchState == 1) { ev.sound = 0x20025; ev.soundArg = 0x1000; ev.hitEffect = true; }
    return ev;
}

KomonoLogic::Pickup KomonoLogic::itemCtrlPost(ItemTaskState& it, const int life[2], const int maxLife[2], RandFn rand) {
    Pickup p;
    if (!(it.flags & 8) || it.picked != 0) return p;
    int pl = 0;
    bool p1 = (it.flags & 2) != 0, p2 = (it.flags & 4) != 0;
    if (p1 && p2) pl = rand() % 2;                      // ambos: al azar (bits no se limpian)
    else if (p1) { pl = 0; it.flags &= ~2u; }
    else if (p2) { pl = 1; it.flags &= ~4u; }
    it.flags &= ~8u; it.picked = 1;
    p.player = pl;
    p.showGetDisp = it.itemNo >= 8;
    if (it.kind == 0x100BC) {
        p.lifeItem = true;
        if (life[pl] < maxLife[pl]) p.heal = true; else p.giveItem = true;
        p.sound = 0x2001A;
    } else {
        p.giveItem = true;
        if (it.itemNo >= 8 && it.itemNo < 0x14) p.sound = 0xB011E;
        else if (it.itemNo == 0x14) p.sound = 0xB011F;
        else if (it.itemNo >= 0x15) p.sound = 0xB010C;
        else p.sound = 0x2002E;
    }
    return p;
}
}

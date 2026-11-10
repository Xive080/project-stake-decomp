// Logica de objetos de escenario y objetos recogibles (overlay aomod, 0x9DF080): Komono*, ItemTask, CKomonoBase.
// Sin dibujo ni colision: las llamadas a motor salen como eventos (sonido, destruir tarea, etc.).
#pragma once
#include "stage_common.h"
namespace vn {

// Camara por rutas: estructura en gp-0x7338; la ruta actual esta en +4 (indice, 0xD0 B por ruta); en la ruta actual: id (s16, +0xD0) y fotograma (f32, +0xC0).
struct CamPathNow { int pathId = 0; float frame = 0; };

// Una fila de _ITEM_TBL (tabla de botin de un Komono): +0 tipo (s16), +2 cantidad (s16), luego pares {objeto s16, peso s16}.
struct ItemTblEntry { int16_t item, weight; };
struct ItemTbl { int16_t kind; int16_t count; ItemTblEntry e[16]; };

struct KomonoLogic {
    // checkEquipItem (0x9F40E0): alguno de los jugadores en juego (modo < 7, salvo 2) lleva `item` en la ranura `slot` (solo gameState 0xA y gameKind 1).
    static bool checkEquipItem(const StageSys& s, int slot, int item);
    // calcItemNo (0x9F4170): elige el objeto de botin. rand = nlRand. Devuelve el id (0 si ninguno).
    //  - objeto 0x13 equipado (ranura 1): siempre 14; 0x14 equipado: siempre 16.
    //  - kind 0: tirada rand()%100 contra pesos acumulados; pesos de objetos >= 21 suben 10 (equipado 0x16) o 25 (0x17); si suman > 100 se recorta
    //    primero de los objetos 8..20 y luego el resto se reparte entre los >= 21.
    //  - kind 1/2: el original ejecuta `break 0` (trampa); aqui devuelve -1.
    //  - al final, si el resultado >= 8 y < 20 y hay objeto 0x15 equipado: 25/200 de cambiarlo por 20.
    static int calcItemNo(const StageSys& s, const ItemTbl& t, RandFn rand);

    // CKomonoBase::checkEndCP (0x9DF0E0): tabla de puntos de fin = {n, ids[], fotogramas[]}; true si la ruta actual es una de ellas y ya paso el fotograma.
    struct EndCP { int n = 0; int id[5] = {}; float frame[5] = {}; };
    static bool checkEndCP(const EndCP& cp, const CamPathNow& cam);
    // Komono3Task::checkCamPath (0x9E...): ruta == id y camara >= fotograma.
    static bool checkCamPath(const CamPathNow& cam, int id, int frame);
    // Komono2Task::checkCloseCamPath (mismo test; el original cierra la tarea). Devuelve true si debe cerrarse.
    static bool checkCloseCamPath(const CamPathNow& cam, int id, float frame);

    // Estado de un Komono3Task (offsets en comentario).
    struct K3 {
        int missionState = 0;      // +0x590: 0 nada, 1 mensaje de mision completa en curso, 2 fade
        float trnsl = 0;           // +0x594 (se baja 1/30 por fotograma sin pausa hasta 0)
        int branchState = 0;       // +0x598: 0 sin tocar, 1 golpeado, 2 cerrado
        int missionNo = 0;         // +0x5A0
        int doneFlag = 0;          // +0x1CC
        int msgPath = 0, msgFrame = 0;     // +0x1D0, +0x1D4: ruta/fotograma del mensaje de mision completa
        int closePath = 0, closeFrame = 0; // +0x1D8, +0x1DC: ruta/fotograma de cierre
    };
    struct K3Globals { int branchNo500F05 = 0; int branchNoGp73CC = 0; int nowSettingGp72E0 = 0; };
    struct K3Event { bool closeTask = false; bool showMissionComplete = false; int missionNo = -1; int sound = 0; int soundArg = 0; bool hitEffect = false; };
    // setBranchNo / clrBranchNo (0x...): 0x500F05 y gp-0x73CC
    static void setBranchNo(K3Globals& g) { g.branchNo500F05 = 1; g.branchNoGp73CC = 1; }
    static void clrBranchNo(K3Globals& g) { g.branchNo500F05 = 0; g.branchNoGp73CC = 0; }
    // checkMissionEnd_setTrnsl: `paused` = func_1F33B0(&D_500F20) > 0.
    static void checkMissionEnd_setTrnsl(K3& k, const K3Globals& g, bool paused);
    // checkMissionCompleteMessageDispTiming: dispara el mensaje (func_A10B00 = new_mission_complete(missionNo)).
    static K3Event checkMissionCompleteMessageDispTiming(K3& k, const CamPathNow& cam);
    // checkCloseTiming_clrBranchNo
    static K3Event checkCloseTiming_clrBranchNo(const K3& k, K3Globals& g, const CamPathNow& cam);
    // checkGameMode: si la mision no esta "abierta" para este modo, limpia la rama y cierra la tarea.
    static K3Event checkGameMode(const K3& k, const StageSys& s, K3Globals& g);
    // checkColli_setBranchNo / Hit: `hit` = checkHit != 0. Primer golpe: sonido 0xB012B; golpes siguientes: 0x20025 (arg 0x1000) + efecto.
    static K3Event onHit(K3& k, K3Globals& g, bool hit);

    // ItemTask::ctrlPost (0x9E...): recogida de objeto por disparo.
    struct Pickup {
        int  player = -1;          // jugador que lo recoge (-1 = nadie)
        bool showGetDisp = false;  // ItemGetDispTask (objetos >= 8)
        bool lifeItem = false;     // objeto 0x100BC (vida): cura si vida < maxima (heal), si no pasa al inventario
        bool heal = false, giveItem = false;
        int  sound = 0;
    };
    struct ItemTaskState { uint32_t flags = 0; int picked = 0; int itemNo = 0; int kind = 0; };   // +0xC (bit1 P1, bit2 P2, bit3 impacto), +0x1A0, +0x1A8, +0x16C
    static Pickup itemCtrlPost(ItemTaskState& it, const int life[2], const int maxLife[2], RandFn rand);
};
}

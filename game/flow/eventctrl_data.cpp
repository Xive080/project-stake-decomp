// Datos de EventCtrl extraidos del ELF/GAMEMOD (ver MANIFEST.md). Generado a mano desde tools/afs_evtpal.py (opcodes) y GAMEMOD.BIN (transiciones).
#include "eventctrl.h"
#include <cstring>
namespace vn {
static const char* const kEvOpNames[] = {"Unknown","ENTB","ENTB2","ET1P","ET2P","ENTBSK","ZMAT","WLCTB","BRANCHNO","BRANCH","MISSENMCLR","SWITCH_MENM","BRANCHEV","SETEVNO","BRANCHADR","CCTB","CAMDR","GETCPMODE","WAIT_CAMJOB","WAIT_CAMFRM","WAIT_FRM","WAIT_ENNUM","WAIT_ENALV","WAIT_PEOPLE","WAIT_ITEM","WAIT_FLUSHDATA","WAIT","ITEMSET","BSFT","WAIT_INT_MODE","FOGCTRL","LIGHTCTRL","MLLIGHT","PLLIGHT","LIGHTIMMEXE","PUSHLIGHT","POPLIGHT","DOFCTRL","SCEFFINOUT","BLURCTRL","OVLAP","MOTVOLUME","MOTSHDWMAP","WAIT_BUFFERRAM","BGTB","FLRTB","SKYKD","SKYDSP","CHGSM","SCNINIT","NEXTEV","SKIP","BRANCHNOSET","SNOWDISP","DBG_MALLOC_PRINT","DBG_ENTDATA_INFO","JIMAKU","FORCE_CAMPARAM","VIBRATION","ENTRYSTGDATA","ENTRYCPBK","ENTRYBANK","FREEBANK","ENTRYSPR","FREESPR","ENTRYMOT","FREEMOT","ENTRYENVTX","FREEENVTX","ENTRYOSK","FREEOSK","ENTRYOSKNAME","FREEOSKNAME","ENTRYOSKBK","FREEOSKBK","ENTRYCLVTX","FREECLVTX","ENTRYFANI","FREEFANI","ENTRYFAMOT","FREEFAMOT","ENTRYCOLLI","FREECOLLI","FLUSHDATA","ENTRYTEX","FREETEX","NOWLOADINGSET","ENTRYLANGSPR","FREELANGSPR","ENTRYLANGBANK","FREELANGBANK","ENTRYSNDPK","FREESNDPK","BGMPLAY","VOICEPLAY","BGMPAUSE","BGMFADE","SKVOOFF","SNDON","SNDONM","SNDONSK","SNDONMSK","LOAD_OVLMOD","WAIT_LOAD_OVLMOD","CHGADXPT","JOBFLAGSET","BRANCHITEM","ENTRYSUBGIRL","GAMEFLAGSET","WAIT_FLUSHDATA_NOPAUSE"};
// 111..114 = plantilla+4 extrapolada (tools/afs_evtpal.py solo llega a 110): NO verificada.
// opcode -> nombre de rutina ("-" = sin resolver). tools/afs_evtpal.py OPS (53..110 = plantilla+4; 0..5,7,9..18,20..28,33..35,37,40 por emulacion de __sinit).
static const char* const kOpcodeNames[115] = {"ENTB","ENTB2","ET1P","ET2P","ENTBSK","ZMAT","-","WLCTB","-","BRANCHNO","BRANCH","MISSENMCLR","SWITCH_MENM","BRANCHEV","SETEVNO","BRANCHADR","CCTB","CAMDR","GETCPMODE","-","WAIT_CAMJOB","WAIT_CAMFRM","WAIT_FRM","WAIT_ENNUM","WAIT_ENALV","WAIT_PEOPLE","WAIT_ITEM","WAIT_FLUSHDATA","WAIT","-","-","-","-","FOGCTRL","LIGHTCTRL","LIGHTCTRL","-","PLLIGHT","-","-","POPLIGHT","-","-","-","-","-","-","-","-","-","-","-","-","CHGSM","SCNINIT","NEXTEV","SKIP","BRANCHNOSET","SNOWDISP","DBG_MALLOC_PRINT","DBG_ENTDATA_INFO","JIMAKU","FORCE_CAMPARAM","VIBRATION","ENTRYSTGDATA","ENTRYCPBK","ENTRYBANK","FREEBANK","ENTRYSPR","FREESPR","ENTRYMOT","FREEMOT","ENTRYENVTX","FREEENVTX","ENTRYOSK","FREEOSK","ENTRYOSKNAME","FREEOSKNAME","ENTRYOSKBK","FREEOSKBK","ENTRYCLVTX","FREECLVTX","ENTRYFANI","FREEFANI","ENTRYFAMOT","FREEFAMOT","ENTRYCOLLI","FREECOLLI","FLUSHDATA","ENTRYTEX","FREETEX","NOWLOADINGSET","ENTRYLANGSPR","FREELANGSPR","ENTRYLANGBANK","FREELANGBANK","ENTRYSNDPK","FREESNDPK","BGMPLAY","VOICEPLAY","BGMPAUSE","BGMFADE","SKVOOFF","SNDON","SNDONM","SNDONSK","SNDONMSK","LOAD_OVLMOD","WAIT_LOAD_OVLMOD","CHGADXPT","JOBFLAGSET","BRANCHITEM","ENTRYSUBGIRL","GAMEFLAGSET","WAIT_FLUSHDATA_NOPAUSE"};
// Transiciones de fin de fase: GAMEMOD 0x9B83D0 apunta a una tabla por etapa (filas de 8 B). Las longitudes salen de la separacion entre tablas.
static const EvTrans kEvTrans_9B8010[] = {{0,1,-1,-1}, {2,0,-1,-1}, {3,0,-1,-1}, {0,0,0,0},};
static const EvTrans kEvTrans_9B8030[] = {{1,2,2,-1}, {1,4,4,-1}, {1,4,4,-1}, {0,5,-1,-1}, {0,5,-1,-1}, {1,6,7,-1}, {0,8,-1,-1}, {0,8,-1,-1}, {1,9,24,-1}, {2,0,-1,-1}, {1,11,12,-1}, {0,13,-1,-1}, {0,13,-1,-1}, {1,14,31,-1}, {2,0,-1,-1}, {1,29,16,-1}, {2,0,-1,-1}, {1,18,19,-1}, {0,30,-1,-1}, {0,30,-1,-1}, {0,32,-1,-1}, {2,0,-1,-1}, {2,0,-1,-1}, {0,33,-1,-1}, {1,25,26,-1}, {0,22,-1,-1}, {1,27,28,-1}, {0,34,-1,-1}, {0,16,-1,-1}, {0,34,-1,-1}, {0,20,-1,-1}, {0,21,-1,-1}, {0,36,-1,-1}, {0,36,-1,-1}, {0,22,-1,-1}, {0,37,-1,-1}, {0,37,-1,-1}, {0,38,-1,-1}, {1,39,40,46}, {1,41,42,46}, {1,41,42,46}, {1,43,43,46}, {1,43,43,46}, {1,44,45,46}, {1,39,40,46}, {1,39,40,46}, {3,0,-1,-1}, {0,0,0,0},};
static const EvTrans kEvTrans_9B81B0[] = {{1,3,6,-1}, {0,4,-1,-1}, {0,7,-1,-1}, {0,1,-1,-1}, {0,5,-1,-1}, {0,9,-1,-1}, {0,2,-1,-1}, {0,8,-1,-1}, {0,10,-1,-1}, {0,10,-1,-1}, {0,11,-1,-1}, {0,12,-1,-1}, {3,0,-1,-1}, {0,0,0,0},};
static const EvTrans kEvTrans_9B8220[] = {{0,4,-1,-1}, {1,2,3,-1}, {0,5,-1,-1}, {0,5,-1,-1}, {0,1,-1,-1}, {2,0,-1,-1}, {0,7,-1,-1}, {1,8,9,23}, {1,13,20,23}, {1,10,17,23}, {1,11,18,23}, {1,12,19,23}, {1,7,16,23}, {1,14,21,23}, {1,15,22,23}, {1,7,16,23}, {1,7,-1,23}, {1,10,-1,23}, {1,11,-1,23}, {1,12,-1,23}, {1,13,-1,23}, {1,14,-1,23}, {1,15,-1,23}, {3,0,-1,-1},};
static const EvTrans kEvTrans_9B82E0[] = {{0,1,-1,-1}, {0,2,-1,-1}, {0,3,-1,-1}, {0,4,-1,-1}, {0,5,-1,-1}, {0,6,-1,-1}, {0,7,-1,-1}, {1,8,19,-1}, {1,9,19,-1}, {1,10,19,-1}, {1,11,19,-1}, {1,12,19,-1}, {1,13,19,-1}, {1,14,19,-1}, {1,15,19,-1}, {1,16,19,-1}, {1,17,19,-1}, {1,18,19,-1}, {1,7,19,-1}, {3,0,-1,-1},};
static const EvTrans kEvTrans_9B8380[] = {{0,1,-1,-1}, {0,2,-1,-1}, {0,3,-1,-1}, {3,0,-1,-1},};
static const EvTrans kEvTrans_9B83A0[] = {{1,1,2,-1}, {3,0,-1,-1}, {3,0,-1,-1},};
static const EvTrans kEvTrans_9B83B8[] = {{3,0,-1,-1},};
static const EvTrans kEvTrans_9B83C0[] = {{0,1,-1,-1}, {3,0,-1,-1},};
const EvTransTable kEvTransTables[12] = {{kEvTrans_9B8010,4}, {kEvTrans_9B8030,48}, {kEvTrans_9B81B0,14}, {kEvTrans_9B8220,24}, {kEvTrans_9B82E0,20}, {kEvTrans_9B8380,4}, {kEvTrans_9B8010,4}, {kEvTrans_9B83C0,2}, {nullptr,0}, {kEvTrans_9B83A0,3}, {kEvTrans_9B83B8,1}, {kEvTrans_9B83B8,1}};

const char* evOpName(EvOp op) { return kEvOpNames[(int)op]; }
EvOp evOpFromOpcode(uint32_t opcode) {
    if (opcode >= 115) return EvOp::Unknown;
    const char* n = kOpcodeNames[opcode];
    if (n[0] == '-') return EvOp::Unknown;
    for (int i = 1; i < (int)(sizeof(kEvOpNames) / sizeof(kEvOpNames[0])); i++)
        if (std::strcmp(kEvOpNames[i], n) == 0) return (EvOp)i;
    return EvOp::Unknown;
}
}

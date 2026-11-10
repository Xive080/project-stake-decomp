// Menus de OriginalTask (overlay shpmod): shop_buy (0x9C7390) y mode_select (shpmod_text.s). Solo la decision de estados;
// el dibujo (ShopScriptDisp, shop_put_icon) y el sonido salen como eventos. shop_buy_open_book/2 y shop_melt* NO estan reescritos.
#pragma once
#include <vector>
namespace vn {

enum ShopNext { SN_NONE = 0, SN_SHOP, SN_BOOK, SN_EXIT2MAINMENU, SN_UNSOLVED_LIST, SN_SHOP_INIT, SN_EQUIP_INIT_INIT, SN_GAMESTART };

struct ShopEvent { enum Kind { SOUND, SCRIPT_DISP, MARK_SEEN } kind; int a; };
struct ShopOut {
    int next = SN_NONE;                 // func_1A3F60(this+0x1CAC, funcion D_009D....+8, this)
    std::vector<ShopEvent> ev;
};

struct ShopTask {
    int state = 0;        // +0x1D10
    int cursor = 0;       // +0x1CEC
    int sel1D18 = 0;      // +0x1D18
    int flag1D1C = 0;     // +0x1D1C
    float f2510 = 0;      // 0x502510
    int   gp704C = 0;     // gp-0x704C (rand & 3)
    int   gp7060 = 0;     // gp-0x7060
    int8_t seen[16] = {}; // gp-0x7046 + indice (bit 1 = visto)
};

struct ShopLogic {
    // shop_buy (0x9C7390). `hover` = mode_select_colli(D_009D07C0), `sw` = shotsw_numcheck (< 0 = nada).
    static void buy(ShopTask& t, int hover, int sw, ShopOut& o);
    // mode_select (shpmod): `hover` = mode_select_colli(D_009D06C0), `sw` = shotsw_numcheck, `rand` = nlRand, `st1D18` ya en t.sel1D18.
    static void modeSelect(ShopTask& t, int hover, int sw, int rand, ShopOut& o);
};
}

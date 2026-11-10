#include "shop_rest.h"
namespace vn {

void ShopLogic::buy(ShopTask& t, int hover, int sw, ShopOut& o) {
    if (t.state == 0) t.state = 1;                      // 0x9C73B0: el estado 0 pasa a 1 y sigue
    if (t.state < 0) return;                            // 0x9C73C8 -> solo shop_put_icon
    int script = 0x48;                                  // 0x9C7464
    if (hover == 12) script = 0x43; else if (hover == 11) script = 0x42; else if (hover == 10) script = 0x41; else if (hover == 9) script = 0x45;
    o.ev.push_back({ShopEvent::SCRIPT_DISP, script});
    if (sw < 0) return;                                 // 0x9C7480
    t.state = 0;                                        // 0x9C748C (delay slot de beq: siempre)
    if (sw == 12 || sw == 11 || sw == 10) {             // 0x9C74F8
        o.ev.push_back({ShopEvent::SOUND, 0x70011});
        t.cursor = sw;
        if (sw >= 0 && sw < 16) t.seen[sw] |= 1;        // sb en gp-0x7046+sw
        o.ev.push_back({ShopEvent::MARK_SEEN, sw});
        o.next = SN_BOOK;                               // D_009D0F88+8 = shop_buy_open_book
    } else if (sw == 9) {                               // 0x9C74BC
        o.ev.push_back({ShopEvent::SOUND, 0x70013});
        o.next = SN_SHOP;                               // D_009D0F78+8 = shop
    }
}

void ShopLogic::modeSelect(ShopTask& t, int hover, int sw, int rand, ShopOut& o) {
    (void)hover;                                        // el original solo lo pasa a mode_select_colli (efecto visual)
    switch (t.state) {
    case 0: t.f2510 = 40.0f; t.gp704C = rand & 3; t.state = 1; break;
    case 1: if (t.sel1D18 >= 3) t.state = 2; break;
    case 2:
        t.cursor = sw;
        if (sw >= 0) { if (sw != 0) o.ev.push_back({ShopEvent::SOUND, 0x70014}); t.state = 3; t.flag1D1C = 1; }
        break;
    case 3:
        if (t.sel1D18 == 5) {
            switch (t.cursor) {
            case 0: o.next = SN_EXIT2MAINMENU; break;
            case 1: t.gp7060 = 1; o.next = SN_UNSOLVED_LIST; break;   // D_009D0E28
            case 2: t.gp7060 = 2; o.next = SN_SHOP_INIT; break;       // D_009D0E38
            case 3: t.gp7060 = 3; o.next = SN_EQUIP_INIT_INIT; break; // D_009D0E48
            case 4: o.next = SN_GAMESTART; break;                     // D_009D0E58
            default: t.sel1D18 = 0; break;
            }
        }
        break;
    }
}
}

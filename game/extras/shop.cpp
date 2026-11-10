#include "shop.h"
namespace vn {

// shpmod 0x9D3560 (file 0xED60), 52 enteros.
const int kItemPrice[52] = {0, 300, 700, 3600, 10000, 450, 2500, 15000, 12000, 50000, 1200, 3800, 4200, 22000, 25000, 35000,
    500, 2500, 500, 2000, 12000, 20000, 2500, 6000, 3000, 5000, 3500, 45000, 18000, 3000, 100, 300, 900, 600, 1100, 1500,
    2500, 3000, 2000, 4800, 300, 500, 750, 1000, 2000, 900, 1200, 1500, 9000, 500, 800, 1500};

// ELF 0x2C2A10, pares {grupo, tipo}; objetos 0..55.
const uint8_t kItemAttr[56][2] = {
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {1,2},{1,2},{2,2},{3,2},{3,2},{3,2},{4,2},{4,2},{5,2},{5,2},{5,2},{0,0},{7,0},{7,0},
    {8,1},{8,1},{8,1},{9,2},{9,2},{9,2},{9,2},{9,2},{10,2},{10,2},{11,2},{11,2},{11,2},{11,2},{11,2},
    {12,2},{12,2},{13,1},{13,1},{14,1},{14,1},{14,1},{14,1},{0,0},{0,0},{0,0}};

// ---------- plata ----------
// El original usa 25 fotogramas de animacion para importes >= 30 y salta directo para importes pequenos.
void ShopLogic::silverSub(ShopCtx& c, int amount) {
    if (amount < 30) {
        c.silverAnimTimer = 0;
        c.save.silver -= amount;
        c.silverAnimA = c.save.silver; c.silverAnimB = c.save.silver;
    } else {
        c.silverAnimTimer = 25;
        c.silverAnimA = c.save.silver; c.silverAnimB = c.save.silver;
        c.save.silver -= amount;
    }
}
void ShopLogic::silverAdd(ShopCtx& c, int amount) {
    const int kMax = 999999;                          // 0xF423F
    if (c.save.silver + amount > kMax) amount = kMax - c.save.silver;
    if (amount < 30) {
        c.silverAnimTimer = 0;
        c.save.silver += amount;
        c.silverAnimA = c.save.silver; c.silverAnimB = c.save.silver;
    } else {
        c.silverAnimTimer = 25;
        c.silverAnimA = c.save.silver; c.silverAnimB = c.save.silver;
        c.save.silver += amount;
    }
}

// ---------- precios ----------
// Compra: x1, objeto 0x19 equipado = 80 %, objeto 0x18 = 90 % (0x19 tiene prioridad). Venta: 60 %, con objeto 0x1A = 75 %.
// Los objetos se miran en la ranura 1 de los jugadores presentes. El original no comprueba el rango de `item`.
int ShopLogic::calcPrice(const ShopCtx& c, int item, int qty, bool sell) {
    bool t1 = false, t2 = false, t3 = false;
    for (int pl = 0; pl < 2; pl++) {
        if (!c.active[pl]) continue;
        int it = c.save.equipped[pl][1];
        if (it == 0x1A) t3 = true; else if (it == 0x19) t2 = true; else if (it == 0x18) t1 = true;
    }
    int base = (item >= 0 && item < 52) ? kItemPrice[item] * qty : 0;
    if (sell) return t3 ? (base * 15) / 20 : (base * 6) / 10;
    if (t2) return (base * 8) / 10;
    if (t1) return (base * 9) / 10;
    return base;
}

// ---------- equipo ----------
// El original cuenta lo que lleva el OTRO jugador (1 - pl) y solo si esta jugando (modos 7, 9 o >= 11).
int ShopLogic::otherEquipCount(const ShopCtx& c, int pl, int item) {
    int o = 1 - pl;
    int m = c.playerMode[o];
    bool counts = (m == 7 || m == 9 || m > 10);
    if (!counts) return 0;
    int n = 0;
    for (int s = 0; s < ITEM_SLOTS; s++) if (c.save.equipped[o][s] == item) n++;
    return n;
}
void ShopLogic::otherUnequip(ShopCtx& c, int pl, int item) {
    int o = 1 - pl;
    int m = c.playerMode[o];
    bool counts = (m == 7 || m == 9 || m > 10);
    if (!counts) return;
    for (int s = 0; s < ITEM_SLOTS; s++) if (c.save.equipped[o][s] == item) { c.save.equipped[o][s] = 0; return; }
}

static bool slotRange(int slot, int& lo, int& hi) {
    switch (slot) {
    case 0: lo = 1; hi = 0xF; return true;
    case 1: lo = 0x10; hi = 0x1D; return true;
    case 2: case 3: lo = 0x1E; hi = 0x34; return true;
    }
    return false;
}

int ShopLogic::checkExistItem(const ShopCtx& c, const int8_t* inv, int slot) {
    int lo, hi;
    if (!slotRange(slot, lo, hi)) return 1;          // ranura desconocida: el original devuelve 1
    for (int i = lo; i <= hi; i++)
        if (inv[i] + otherEquipCount(c, 0, i) + otherEquipCount(c, 1, i) != 0) return 1;
    return 0;
}

int ShopLogic::trnsIconToEquipType(int icon) { return icon == 0xC ? 2 : icon == 0xB ? 1 : icon == 0xA ? 0 : -1; }

// pl >= 0: se puede equipar `item` en la ranura `slot` del jugador. pl == -1: hay unidades en `inv`. pl == -2: ademas cuenta lo equipado.
int ShopLogic::checkItemNum(const ShopCtx& c, const int8_t* inv, int pl, int slot, int item) {
    if (item == 0) return 1;
    if (pl < 0) {
        int n = inv[item];
        if (pl == -2) n += otherEquipCount(c, 0, item) + otherEquipCount(c, 1, item);
        return n > 0 ? 1 : 0;
    }
    if (!(inv[item] > 0)) {
        if (c.save.equipped[pl][slot] != item && otherEquipCount(c, pl, item) == 0) return 0;
    }
    int ok = 1;
    int type = kItemAttr[item][1], group = kItemAttr[item][0];
    for (int a = 0; a < 2; a++) {
        for (int b = 0; b < ITEM_SLOTS; b++) {
            if (a == pl && b == slot) continue;
            if (a != pl && !c.active[a]) continue;
            if (type == 2 || (type == 1 && a == pl)) {
                int eq = c.save.equipped[a][b];
                if (group == kItemAttr[eq][0]) ok = 0;
            }
        }
    }
    return ok;
}

// Busca el siguiente objeto valido a partir de `cur` en direccion `step` (+1/-1) dentro del rango de la ranura.
// Con step == 0 solo revalida `cur` (devuelve cur o 0). Para pl >= 0 no hay vuelta al principio: devuelve 0 al salirse del rango.
int ShopLogic::decideItemNum(const ShopCtx& c, const int8_t* inv, int pl, int slot, int cur, int step) {
    int lo = 0, hi = 0;
    slotRange(slot, lo, hi);                           // ranura invalida: el original usa valores sin inicializar
    bool hasAny = false;
    if (pl < 0) {
        for (int i = lo; i <= hi; i++) {
            int n = inv[i];
            if (pl == -2) n += otherEquipCount(c, 1, i) + otherEquipCount(c, 0, i);
            if (n != 0) { hasAny = true; break; }
        }
    }
    if (step == 0) return checkItemNum(c, inv, pl, slot, cur) ? cur : 0;
    int s4;
    if (cur != 0) s4 = cur; else s4 = (step > 0) ? lo - 1 : hi + 1;
    for (int guard = 0; guard < 400; guard++) {      // (el original no tiene tope; solo se alcanza con inventarios negativos)
        int s1 = s4 + step;
        if (s1 < lo || s1 > hi) {
            if (!hasAny) return 0;
            s4 = (step >= 0) ? lo - step : hi - step;  // da la vuelta
            continue;
        }
        if (checkItemNum(c, inv, pl, slot, s1)) return s1;
        s4 += step;
    }
    return 0;
}

void ShopLogic::unEquipTool(ShopCtx& c, int pl) {
    for (int s = 0; s < ITEM_SLOTS; s++) {
        int it = c.save.equipped[pl][s];
        if (it == 0) continue;
        if (c.save.stock[it] < 0x63) c.save.stock[it]++;
        c.save.equipped[pl][s] = 0;
    }
}

ShopEvents ShopLogic::setEquipmentItem(ShopCtx& c, int pl) {
    ShopEvents ev;
    int sel = c.selItem[pl];
    int avail = c.save.stock[sel] + otherEquipCount(c, pl, sel);
    if (!(avail > 0) && sel != 0) return ev;
    int slot = c.selSlot[pl];
    int cur = c.save.equipped[pl][slot];
    if (sel != cur) {
        if (cur != 0) orgAddItemG(c, cur);            // lo que llevaba vuelve al inventario
        c.save.equipped[pl][slot] = (int8_t)sel;
        if (sel != 0) {
            ev.sound = 0x7000F;
            if (c.save.stock[sel] != 0) c.save.stock[sel]--;
            else otherUnequip(c, pl, sel);             // se lo quita al otro jugador
        } else ev.sound = 0x7000D;
    }
    int o = 1 - pl;
    if (c.active[o]) {
        int a0 = c.selSlot[pl], a3 = c.selSlot[o];
        if (a0 == a3 || (a0 == 2 && a3 == 3) || (a0 == 3 && a3 == 2))
            c.selItem[o] = (int8_t)decideItemNum(c, c.save.stock, o, a3, c.selItem[o], 0);
    }
    return ev;
}

// ---------- objetos conseguidos ----------
void ShopLogic::orgAddItemG(ShopCtx& c, int item) {
    if (item <= 0 || item >= 0x35) return;
    int v = c.save.stock[item] + 1;
    if (v < 0x64) c.save.stock[item] = (int8_t)v;
    HuntLogic::addPage(c.hunt, 3, item);              // HFileAddPageG(3, item)
}
void ShopLogic::orgAddShopItemG(ShopCtx& c, int item) {
    if (c.save.shopAvail[item] == 0) { c.save.shopAvail[item] = 1; c.save.shopNewFlag = 1; }
}

// ---------- efectos al empezar ----------
// Ranuras 2 y 3 de los jugadores presentes: 0x21..0x24 suman 2/4/6/10 creditos, 0x25 = juego libre (bonus 0).
CreditResult ShopLogic::setCredit(const ShopCtx& c) {
    bool freePlay = false; int bonus = 0;
    for (int pl = 0; pl < 2; pl++) {
        if (!c.active[pl]) continue;
        for (int s = 2; s < 4; s++) {
            switch (c.save.equipped[pl][s]) {
            case 0x25: freePlay = true; break;
            case 0x24: bonus += 10; break;
            case 0x23: bonus += 6; break;
            case 0x22: bonus += 4; break;
            case 0x21: bonus += 2; break;
            default: break;
            }
        }
    }
    return freePlay ? CreditResult{true, 0} : CreditResult{false, bonus};
}

// Base 8; 0x31 = -2, 0x32 = -5, 0x33 = -8, 0x34 = +8; limitado a 0..16.
int ShopLogic::setDifficulty(const ShopCtx& c) {
    int d = 8;
    for (int pl = 0; pl < 2; pl++) {
        if (!c.active[pl]) continue;
        for (int s = 2; s < 4; s++) {
            switch (c.save.equipped[pl][s]) {
            case 0x34: d += 8; break;
            case 0x31: d -= 2; break;
            case 0x32: d -= 5; break;
            case 0x33: d -= 8; break;
            default: break;
            }
        }
    }
    if (d < 0) d = 0; else if (d > 16) d = 16;
    return d;
}

int ShopLogic::setDispScore(const ShopCtx& c) {
    int r = 0;
    for (int pl = 0; pl < 2; pl++) if (c.active[pl] && c.save.equipped[pl][1] == 0x12) r = 1;
    return r;
}

int ShopLogic::countUnsolvedReq(const ShopSave& s) { int n = 0; for (int i = 0; i < REQ_COUNT; i++) if (s.reqFlags[i] & 2) n++; return n; }
int ShopLogic::countRemainReq(const ShopSave& s) { int n = 0; for (int i = 0; i < REQ_COUNT; i++) if (!(s.reqFlags[i] & 1)) n++; return n; }

int ShopLogic::shotswPlCheck(const ShopCtx& c, int pl, bool trigger) { return (c.active[pl] && trigger) ? 1 : 0; }
int ShopLogic::dirswCheck() { return 0; }
}

// Tienda de objetos / equipamiento (overlay shpmod, OriginalTask) y objetos de la partida (lib 0x1D3720, orgmod), reescrito a mano.
// Solo reglas de juego (precios, inventario, compatibilidad de equipo, creditos y dificultad por objetos); sin dibujo, sin sonido, sin hardware.
// Los efectos de motor (sonido, paginas del cuaderno) se devuelven como valores/eventos.
//
// Mapa de objetos (identificadores 1..52; tabla de precios en shpmod 0x9D3560, atributos en ELF 0x2C2A10):
//   ranura 0 = armas 1..15 (el 8..15 cambia balas y sonido de recarga en player.cpp; 11 y 13 son de rafaga en GunMgr)
//   ranura 1 = 16..29 (0x18/0x19/0x1A = descuentos de tienda, 0x1B = salva del dano, 0x12 = marcador)
//   ranuras 2 y 3 = 30..52 (30/31/32 vidas extra, 33..37 creditos, 47/48 puntos x2/x4, 49..52 dificultad)
#pragma once
#include <cstdint>
#include "hunt.h"
namespace vn {

enum { ITEM_MAX = 0x34, ITEM_SLOTS = 4, REQ_COUNT = 33 };

// Bloque de guardado de objetos (0x502690..0x502728).
struct ShopSave {
    int8_t  equipped[2][ITEM_SLOTS] = {};   // 0x502690 + 4*pl + ranura (0 = vacia)
    int8_t  shopAvail[56] = {};             // 0x502698 + objeto: 1 = ya a la venta (OrgAddShopItemG)
    int8_t  stock[56] = {};                 // 0x5026CE + objeto: unidades en el inventario (max 99)
    int32_t silver = 0;                     // 0x502704 plata (max 999999)
    int8_t  reqFlags[REQ_COUNT] = {};       // 0x502708 + n: peticiones; bit0 = ?, bit1 = ? (ver CountRemainReq/CountUnsolvedReq)
    int8_t  shopNewFlag = 0;                // 0x50274C: hay objetos nuevos en la tienda
};

// Estado de la tienda (OriginalTask, this+...) y de los jugadores que necesita la logica.
struct ShopCtx {
    ShopSave save;
    HuntSave hunt;
    bool active[2] = {false, false};        // this+0x1CF0 + 12*pl: jugador presente
    int  playerMode[2] = {0, 0};            // PlayerTask+4 (0x500734 + 0x348*pl)
    int8_t selSlot[2] = {0, 0};             // this+0x1D45 + pl: ranura seleccionada
    int8_t selItem[2] = {0, 0};             // this+0x1D47 + pl: objeto seleccionado
    // Animacion del contador de plata (this+0x1D34/0x1D38/0x1D3C)
    int silverAnimTimer = 0, silverAnimA = 0, silverAnimB = 0;
};

struct ShopEvents { int sound = 0; };       // identificador de sonido (0 = ninguno): 0x7000F equipar, 0x7000D quitar

struct CreditResult { bool freePlay; int bonus; };   // argumentos de func_1C7880(flag, bonus)

extern const int kItemPrice[52];            // shpmod 0x9D3560 (indice = objeto; el 0 vale 0)
extern const uint8_t kItemAttr[56][2];      // ELF 0x2C2A10: {grupo, exclusividad (0 = ninguna, 1 = por jugador, 2 = entre jugadores)}

struct ShopLogic {
    // --- plata ---
    static void silverSub(ShopCtx& c, int amount);          // OrgShopSubSilver 0x9C6880
    static void silverAdd(ShopCtx& c, int amount);          // OrgShopAddSilver 0x9C6900 (tope 999999)
    // --- precios ---
    static int calcPrice(const ShopCtx& c, int item, int qty, bool sell);   // OriginalTask::calcPrice 0x9CC0C0
    // --- inventario y equipo ---
    static int otherEquipCount(const ShopCtx& c, int pl, int item);    // checkGetAnotherPlayerEquip (orgmod 0xA11FE0)
    static void otherUnequip(ShopCtx& c, int pl, int item);            // unequipAnotherPlayerEquip (orgmod 0xA12080)
    static int checkExistItem(const ShopCtx& c, const int8_t* inv, int slot);   // 0x9C7F40
    static int trnsIconToEquipType(int icon);                         // 0x9C8020
    static int checkItemNum(const ShopCtx& c, const int8_t* inv, int pl, int slot, int item);       // 0x9CAB90
    static int decideItemNum(const ShopCtx& c, const int8_t* inv, int pl, int slot, int cur, int step);   // 0x9CAD50
    static void unEquipTool(ShopCtx& c, int pl);                      // 0x9CAF50
    static ShopEvents setEquipmentItem(ShopCtx& c, int pl);           // 0x9CAFC0
    // --- partida: objetos conseguidos (lib 0x1D3720) ---
    static void orgAddItemG(ShopCtx& c, int item);                    // OrgAddItemG 0x1D3820 (inventario +1, max 99, pagina del cuaderno)
    static void orgAddShopItemG(ShopCtx& c, int item);                // OrgAddShopItemG 0x1D3880
    // --- efectos de los objetos equipados al empezar la partida ---
    static CreditResult setCredit(const ShopCtx& c);                  // OriginalTask::OrgSetCredit 0x9CC4F0
    static int setDifficulty(const ShopCtx& c);                       // OrgSetDifficulty 0x9CC600 (0..16)
    static int setDispScore(const ShopCtx& c);                        // OrgSetDispScore 0x9CC700
    // --- peticiones ---
    static int countUnsolvedReq(const ShopSave& s);                   // CountUnsolvedReq 0x9CDD80 (bit1 activo)
    static int countRemainReq(const ShopSave& s);                     // CountRemainReq 0x9CDDC0 (bit0 inactivo)
    // --- seleccion con la pistola ---
    static int shotswPlCheck(const ShopCtx& c, int pl, bool trigger); // shotsw_plcheck 0x9CB9E0
    static int dirswCheck();                                          // dirsw_check 0x9CBAE0 (siempre 0: dirsw_plcheck devuelve 0)
};
}

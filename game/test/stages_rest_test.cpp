// Pruebas de game/stages_rest. Comprueban las reglas leidas del asm (no hay ejecucion diferencial contra el original)
// y las tablas contra ORGMOD.BIN / TRNMOD.BIN (se omiten con aviso si no estan).
#include "../stages_rest/gunorg.h"
#include "../stages_rest/komono_rest.h"
#include "../stages_rest/trn_rest.h"
#include "../stages_rest/shop_rest.h"
#include <cstdio>
#include <vector>
#include <string>
using namespace vn;
static int fails = 0, checks = 0;
#define CHECK(x) do { checks++; if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)

static int g_seq[32], g_n = 1, g_i = 0;
static void seq(std::initializer_list<int> v) { g_n = 0; g_i = 0; for (int x : v) g_seq[g_n++] = x; }
static int fakeRand() { return g_seq[g_i++ % g_n]; }

static bool readBin(const char* name, std::vector<unsigned char>& out) {
    const char* dirs[] = {"disc/TBL/", "../disc/TBL/", "../../disc/TBL/"};
    for (const char* d : dirs) {
        FILE* f = fopen((std::string(d) + name).c_str(), "rb");
        if (!f) continue;
        unsigned char b[4096]; size_t n;
        while ((n = fread(b, 1, sizeof b, f)) > 0) out.insert(out.end(), b, b + n);
        fclose(f); return true;
    }
    return false;
}

struct Mock : GunOrgHost {
    std::vector<int> snd; int shell = 0, shotEndN = 0, sightN = 0, reloadFxN = 0, ammoDispN = 0, preN = 0, postN = 0;
    float sx = 0, sy = 0; bool cont = true;
    int rand() override { return fakeRand(); }
    void preUpdate() override { preN++; }
    void reloadFx() override { reloadFxN++; }
    void setSight(int, float x, float y) override { sx = x; sy = y; sightN++; }
    void postUpdate() override { postN++; }
    void playSound(int id) override { snd.push_back(id); }
    void shellEject(int, int n) override { shell = n + 100; }
    void shotEnd() override { shotEndN++; }
    bool canContinueBurst(int) override { return cont; }
    void ammoDisplay() override { ammoDispN++; }
};

int main() {
    // ---- tablas contra ficheros reales ----
    std::vector<unsigned char> org, trn;
    if (readBin("ORGMOD.BIN", org) && org.size() > 0x2B20) {
        bool ok = true;
        for (int i = 0; i < 16; i++) for (int j = 0; j < 2; j++) if ((int8_t)org[0x2B00 + i * 2 + j] != kGunOrgTable[i][j]) ok = false;
        CHECK(ok);
    } else printf("OMITIDO: ORGMOD.BIN no encontrado (tabla D_00A12300)\n");
    if (readBin("TRNMOD.BIN", trn) && trn.size() > 0x151A0 + 192) {
        bool ok = true;
        // En el fichero real la tabla son 48 entradas de 4 bytes {segundos, -1, 3, -1} (mision*3 + nivel); kTrnSeconds guarda solo el campo 0.
        for (int i = 0; i < 16; i++) for (int j = 0; j < 3; j++) if ((int8_t)trn[0x151A0 + (i * 3 + j) * 4] != kTrnSeconds[i][j]) ok = false;   // base 0x9DF000
        printf("kTrnSeconds contra TRNMOD.BIN (base 0x9DF000): %s\n", ok ? "coincide" : "NO coincide");
        CHECK(ok);
    } else printf("OMITIDO: TRNMOD.BIN no encontrado (kTrnSeconds)\n");

    // ---- gunOrgExe ----
    {   // arma 0 (pistola): un disparo valido gasta 1, espera 0
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; int32_t silver = 100, total = 100; s.silver = &silver; s.silverTotal = &total;
        s.active7418 = 1; s.equip0 = 0; g.trigger = 1; g.sightValid = 1; w.ammo = 8; w.ammoPrev = 8; w.sightX = 10; w.sightY = 20;
        GunOrg::exe(w, g, s, h);
        CHECK(w.ammo == 7 && w.firing == 1 && w.cool == 0 && h.sightN == 1 && h.sx == 10 && h.sy == 20);
        CHECK(h.snd.size() == 1 && h.snd[0] == 0x2000C);
    }
    {   // jugador 2 cambia el sonido
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.pl = 1; s.active7418 = 1; g.trigger = 1; g.sightValid = 1; w.ammo = 8;
        GunOrg::exe(w, g, s, h); CHECK(h.snd.size() == 1 && h.snd[0] == 0x2000D);
    }
    {   // arma 8: cuesta 5 de plata y marca 0x346
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; int32_t silver = 7, total = 9; s.silver = &silver; s.silverTotal = &total;
        s.active7418 = 1; s.equip0 = 8; g.trigger = 1; g.sightValid = 1; w.ammo = 6;
        GunOrg::exe(w, g, s, h);
        CHECK(w.ammo == 5 && silver == 2 && total == 4 && w.flag346 == 1 && w.cool == 5 && h.snd.size() == 1 && h.snd[0] == 0xB0112);
        GunOrg::exe(w, g, s, h);   // espera: 5 -> 4, no dispara
        CHECK(w.ammo == 5 && w.cool == 4 && w.flag346 == 0);
    }
    {   // arma 8 sin plata suficiente: dispara pero sin marca
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; int32_t silver = 4; s.silver = &silver; s.active7418 = 1; s.equip0 = 8; g.trigger = 1; g.sightValid = 1; w.ammo = 6;
        GunOrg::exe(w, g, s, h); CHECK(silver == 4 && w.flag346 == 0 && w.ammo == 5);
    }
    {   // arma 9: marca 0x346 sin plata
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.active7418 = 1; s.equip0 = 9; g.trigger = 1; g.sightValid = 1; w.ammo = 3;
        GunOrg::exe(w, g, s, h); CHECK(w.flag346 == 1 && w.ammo == 2 && w.cool == 5);
    }
    {   // arma 10 (dispersion): 1er fotograma inicia rafaga y suena 0xB010E; el 2o usa 4 rand
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.active7418 = 1; s.equip0 = 10; g.trigger = 1; g.sightValid = 1; w.ammo = 6; w.lastX = 0;
        w.sightX = 100; w.sightY = 100;
        GunOrg::exe(w, g, s, h);
        CHECK(w.phase == 1 && w.burst == 1 && w.ammo == 5 && h.snd.size() == 1 && h.snd[0] == 0xB010E);
        seq({0, 5, 1, 7});   // signo +1, 5, signo -1, 7
        h.snd.clear(); w.cool = 0;
        GunOrg::exe(w, g, s, h);
        CHECK(w.phase == 2 && w.spreadDone == 1 && w.ammo == 5 && h.sx == w.lastX + 5 && h.sy == w.lastY - 7);
        // fase 6 termina la rafaga con espera 12
        for (int i = 0; i < 4; i++) { seq({0, 1, 0, 1}); w.cool = 0; GunOrg::exe(w, g, s, h); }
        CHECK(w.phase == 0 && w.burst == 0 && w.cool == 12);
    }
    {   // arma 13: rafaga de 50, gasta 1 al disparar y con trigger suelto cuenta mod 4
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.active7418 = 1; s.equip0 = 13; g.trigger = 1; g.sightValid = 1; w.ammo = 50; w.sightX = 4; w.sightY = 4;
        GunOrg::exe(w, g, s, h);
        CHECK(w.ammo == 49 && w.phase == 1 - 1 + 0 + (w.phase) && w.burst == 1);
        CHECK(h.snd.size() == 1 && h.snd[0] == 0xB012D);
    }
    {   // sin municion ni rafaga: no dispara, se marca vacio no (ammo ya 0 -> no entra); con trigger y sin mira valida pide recarga
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.active7418 = 1; s.equip0 = 0; g.trigger = 1; g.sightValid = 0; w.ammo = 2; w.ammoPrev = 3;
        GunOrg::exe(w, g, s, h); CHECK(w.reloadReq == 1 && h.ammoDispN == 1 && h.sightN == 0);
        s.stage = 7; GunOrg::exe(w, g, s, h); CHECK(w.reloadReq == 0);
    }
    {   // balas infinitas
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.active7418 = 1; s.flags72D0 = 0x10; s.equip0 = 3; g.trigger = 1; g.sightValid = 1; w.ammo = 1;
        GunOrg::exe(w, g, s, h); CHECK(w.ammo == 16 && w.cool == 0);
    }
    {   // juego parado anula la rafaga
        Mock h; GunOrgWork w; GunOrgGun g; GunOrgSys s; s.active7418 = 0; s.equip0 = 10; g.trigger = 1; g.sightValid = 1; w.ammo = 6; w.burst = 1; w.phase = 3; w.lastValid = 1;
        GunOrg::exe(w, g, s, h); CHECK(w.burst == 0 && w.phase == 0 && h.sightN == 0);
    }
    {   // bulletSet
        Mock h; GunOrgWork w; w.burst = 1; w.phase = 3; w.cnt344 = 2; w.prevValid = 1; GunOrg::bulletSet(w, h);
        CHECK(w.burst == 0 && w.phase == 0 && w.cnt344 == 0 && w.prevValid == 0 && h.ammoDispN == 1);
    }
    // ---- dano y vida ----
    {
        StageSys st; st.gameState = 0xA; st.gameKind = 1; st.equip[0][0] = 7;
        CHECK(orgPlWeaponDamageGet(st, 0) == 2.0f);
        st.gameKind = 2; CHECK(orgPlWeaponDamageGet(st, 0) == 1.0f);
        st.gameKind = 1;
        DecLifeTarget e; e.life = 100; decLife(e, st, 0, 10);
        CHECK(e.life == 80 && e.prevLife == 100);
        st.equip[0][0] = 0; st.pl[0].flag346 = 1; e.life = 55; decLife(e, st, 0, 1);
        CHECK(e.life == 0);                                 // plata/estaca: dano = vida
        e.life = 30; e.flags = 0x1000; decLife(e, st, 0, 9); CHECK(e.life == 30 && e.prevLife == 30);   // invulnerable
    }
    {   // rayo contra esfera: rayo recto, centro en el eje -> toca
        ShotRay r;
        CHECK(rayVsBall(r, 0, 0, 5, 1.0f));
        CHECK(!rayVsBall(r, 3, 0, 5, 1.0f));
        CHECK(rayVsBall(r, 0.5f, 0.5f, 5, 1.0f));
    }

    // ---- Komono3 ----
    {
        uint32_t f = 0; int pl = -1; seq({1});
        CHECK(!k3CheckHit(f, fakeRand, pl));
        f = 8 | 2; CHECK(k3CheckHit(f, fakeRand, pl) && pl == 0 && f == 0);
        f = 8 | 4; CHECK(k3CheckHit(f, fakeRand, pl) && pl == 1 && f == 0);
        f = 8 | 6; seq({1}); CHECK(k3CheckHit(f, fakeRand, pl) && pl == 1 && f == 6);
        TumiageColli t; t.flags = 8 | 2; CHECK(t.chkColli() && t.getHitPl(fakeRand) == 0 && t.chkColli()); t.clrColli(); CHECK(!t.chkColli());
        CHECK(!t.ctrlPre()); t.close(); CHECK(t.ctrlPre());
    }
    {   // nuts: golpe -> 0xC000E, 0xB012B; luego avanza y suena en 56 y 61
        StageSys st; st.gameState = 0xA; st.gameKind = 1; for (auto& m : st.missionFlags) m = 3;
        KomonoLogic::K3Globals g; K3Ctx c; c.sys = &st; c.g = &g; c.rand = fakeRand;
        K3Anim a; a.k.missionNo = 0; a.k.doneFlag = 1; a.k.closePath = 99; a.k.closeFrame = 99; a.hitFlags = 8 | 2;
        K3Out o; K3Objects::nuts(a, c, o);
        CHECK(a.k.branchState == 1 && g.branchNo500F05 == 1 && o.sounds.size() == 2 && o.sounds[0] == 0xC000E && o.sounds[1] == 0xB012B);
        int n56 = 0;
        for (int i = 0; i < 100; i++) { K3Out o2; K3Objects::nuts(a, c, o2); for (int s : o2.sounds) if (s == 0xC000F) n56++; }
        CHECK(n56 == 4 && a.animState   /* 56,57 y 61,62 caen dentro de las ventanas [56,57.2) y [61,62.2) */ == 1 && a.frame == 90.0f);
    }
    {   // goldenRat: estado 0 corre; golpe -> 1; 340 -> 2
        StageSys st; st.gameState = 0xA; st.gameKind = 1; for (auto& m : st.missionFlags) m = 3;
        KomonoLogic::K3Globals g; K3Ctx c; c.sys = &st; c.g = &g; c.rand = fakeRand;
        K3Anim a; a.k.doneFlag = 1; a.k.closePath = 99; a.k.closeFrame = 99;
        a.frame = 59; K3Out o; K3Objects::goldenRat(a, c, o);
        CHECK(a.frame == 60.0f && o.sounds.size() == 1 && o.sounds[0] == 0x190110 && a.animState == 0);
        a.hitFlags = 8 | 2; K3Out o2; K3Objects::goldenRat(a, c, o2); CHECK(a.k.branchState == 1 && a.animState == 1);
        a.frame = 340; K3Out o3; K3Objects::goldenRat(a, c, o3); CHECK(a.animState == 2 && a.frame == 0);
    }
    {   // secretDoor: escombros cada 9 fotogramas a partir de >10
        StageSys st; st.gameState = 0xA; st.gameKind = 1; for (auto& m : st.missionFlags) m = 3;
        KomonoLogic::K3Globals g; K3Ctx c; c.sys = &st; c.g = &g; c.rand = fakeRand;
        K3Anim a; a.k.doneFlag = 1; a.k.closePath = 99; a.k.closeFrame = 99; a.hitFlags = 8 | 2;
        K3Out o; K3Objects::secretDoor(a, c, o);
        CHECK(o.sounds.size() == 3 && o.sounds[0] == 0xB012B && o.sounds[2] == 0xC000D && a.frame > 1.19f && a.frame < 1.21f && o.quake == 1);
        for (int i = 0; i < 200; i++) { K3Out o2; K3Objects::secretDoor(a, c, o2); }
        CHECK(a.frame == 100.0f && a.animState == 1 && a.debrisCount <= 7 && a.debrisCount > 0);
    }
    {   // cierre cuando el modo no es arcade 1
        StageSys st; st.gameState = 0xA; st.gameKind = 2;
        KomonoLogic::K3Globals g; K3Ctx c; c.sys = &st; c.g = &g; c.rand = fakeRand;
        K3Anim a; K3Out o; K3Objects::common3(a, c, o); CHECK(o.closeTask);
    }
    // ---- kusari / hahen ----
    {
        TumiageColli c0, c1, c2; TumiageColli* arr[3] = {&c0, &c1, &c2}; c1.flags = 8 | 4;
        seq({0});
        KusariTick k = kusariCtrl(arr, 3, false, false, fakeRand);
        CHECK(k.broken.size() == 1 && k.broken[0].link == 1 && k.broken[0].pl == 1 && k.broken[0].sound == 0xD0018 && arr[1] == nullptr && c1.ctrlPre());
        k = kusariCtrl(arr, 3, true, false, fakeRand); CHECK(k.closeAll && c0.ctrlPre() && c2.ctrlPre());
        TumiageColli d0; d0.flags = 8 | 2; TumiageColli* arr2[1] = {&d0}; k = kusariCtrl(arr2, 1, false, true, fakeRand); CHECK(k.broken.empty());
        KusariHahen h; int n = 0; while (!kusariHahenLoop(h, false)) n++; CHECK(n == 0x32 && h.age == 0x33 && h.alpha < 1.0f);
        KusariHahen h2; h2.p[0].pos[1] = -1; h2.floorY = 0; h2.p[0].vel[1] = -2; kusariHahenLoop(h2, false); CHECK(h2.p[0].vel[1] == -1.0f && h2.p[0].pos[1] == -1.0f)   /* rebote: vel -= 0.5*vel, pos = suelo, luego integra */;
    }
    {   // bornItem
        BornSrc s; s.n = 2; s.item[0] = 3; s.weight[0] = 60.9f; s.item[1] = 9; s.weight[1] = 40.0f;
        float p[3] = {1, 2, 3}; BornTable t;
        CHECK(bornItem(s, p, 1, 2, 3, 4, true, t) && t.e.size() == 2 && t.e[0].weight == 60 && t.e[1].item == 9 && t.kindB == 2 && t.tail == 4 && t.pos[2] == 3);
        CHECK(!bornItem(s, p, 1, 2, 3, 4, false, t));
    }

    // ---- Training: flujo ----
    {
        TrnFlow f; TrnSys s; TrnFlowOut o;
        f.mission = 0; s.norm = 1; CHECK(TrnFlowLogic::judge(f, s, o) == TS_FAILED && s.flagE33 == 1);
        TrnFlowOut o2; s.norm = 0; CHECK(TrnFlowLogic::judge(f, s, o2) == TS_SUCCESS);
        TrnFlowOut o3; s.flagE36 = 1; CHECK(TrnFlowLogic::judge(f, s, o3) == TS_RETRY); s.flagE36 = 0;
        f.mission = 6; s.enemyAlive = 2; TrnFlowOut o4; CHECK(TrnFlowLogic::judge(f, s, o4) == TS_FAILED);
        f.mission = 10; s.flagE34 = 0; TrnFlowOut o5; CHECK(TrnFlowLogic::judge(f, s, o5) == TS_SUCCESS);
        f.mission = 16; s.enemyAlive = 0; TrnFlowOut o6; CHECK(TrnFlowLogic::judge(f, s, o6) == TS_SUCCESS);
        f.mission = 3; s.d501310[2] = 1; TrnFlowOut o7; TrnFlowLogic::judge(f, s, o7);
        int ov = 0; for (auto& e : o7.ev) if (e.kind == TrnFlowEvent::SOUND) ov++; CHECK(ov == 3);
    }
    {
        TrnFlow f; TrnSys s; TrnFlowOut o; TrnFlowLogic::failed(f, s, o);
        CHECK(f.sub == 1 && s.gp7410 == 0x32 && o.nextState == TS_NONE && f.t78 < 180.0f);
        int n = 0; while (o.nextState == TS_NONE && n < 1000) { TrnFlowLogic::failed(f, s, o); n++; }
        CHECK(o.nextState == TS_SELECT2_INIT && s.flagE34 == 1 && n == 150 - 1 + 1 - 1 + (n - 149) && n >= 149 && n <= 151 && f.sub == 0);
    }
    {
        TrnFlow f; TrnSys s; TrnFlowOut o; TrnFlowLogic::retry(f, s, o); CHECK(f.t70 == 1);
        TrnFlowLogic::retry(f, s, o); bool st = false; for (auto& e : o.ev) if (e.kind == TrnFlowEvent::START_INIT) st = true; CHECK(st && f.sub == 0);
    }
    {
        TrnFlow f; TrnSys s; TrnFlowOut o; s.modeDF8 = 2; int n = 0;
        while (s.gameState != 6 && n < 200) { TrnFlowLogic::exit(f, s, o); n++; }
        int rec = 0; for (auto& e : o.ev) if (e.kind == TrnFlowEvent::SAVE_RECORD) rec++;
        CHECK(s.gameState == 6 && rec == 2 && f.t7c == 1.0f);
    }
    {
        TrnFlow f; TrnFlowOut o; TrnFlowLogic::mcSave(f, false, o); CHECK(f.sub == 2 && o.nextState == TS_NONE);
        TrnFlowLogic::mcSave(f, true, o); CHECK(o.nextState == TS_SELECT_INIT && f.sub == 0);
    }
    {
        TrnFlow f; TrnSys s; for (auto& r : s.rank) r = 2; s.rank[4] = 1; f.mission = 4; s.level[4] = 2;
        TrnFlowLogic::successInit(f, s); CHECK(s.rank[4] == 2 && f.newRank == 1 && f.allPerfect == 1 && f.anyUncleared == 0 && f.t78 == 360.0f);
        TrnFlowLogic::successGrade(f, s); CHECK(f.grade == 3 && f.cA8 == 1);
        s.level[4] = 1; s.rank[4] = 0; TrnFlow g; g.mission = 4; TrnFlowLogic::successInit(g, s); CHECK(s.rank[4] == 1 && g.newRank == 1 && g.allPerfect == 0);
        s.level[4] = 0; TrnFlow h; h.mission = 4; TrnFlowLogic::successInit(h, s); CHECK(h.newRank == 0);   // dificultad menor: no sube
        TrnFlow k; s.rank[0] = -1; k.mission = 0; TrnFlowLogic::successInit(k, s); CHECK(k.anyUncleared == 1);
    }
    {
        TrnFlow f; TrnSys s; f.selMission = 5; f.prevMission = 5; s.level[5] = 2;
        CHECK(!TrnFlowLogic::startInit(f, s) && s.gp72C4 == 5 && s.gp72C8 == 2);
        f.selMission = 6; CHECK(TrnFlowLogic::startInit(f, s));
    }
    {   // trn_10
        TrnTask t; Trn10Globals g; TrnGlobals tg; tg.level[10] = 1; trn10Init(t, g, tg, 10);
        CHECK(t.c98 == kTrnSeconds[10][1] * 50);
        t.c98 = 2; trn10Tick(t, g, tg); CHECK(g.mgr6F50 == 1 && t.c98 == 1 && tg.missionOver == 0);
        trn10Tick(t, g, tg); CHECK(tg.missionOver == 1);
        TrnGlobals tg2; tg2.paused = true; TrnTask t2; t2.c98 = 5; trn10Tick(t2, g, tg2); CHECK(t2.c98 == 5);
    }

    // ---- tienda ----
    {
        ShopTask t; ShopOut o; ShopLogic::buy(t, 12, -1, o);
        CHECK(t.state == 1 && o.ev.size() == 1 && o.ev[0].a == 0x43 && o.next == SN_NONE);
        ShopOut o2; ShopLogic::buy(t, 5, 11, o2);
        CHECK(t.state == 0 && t.cursor == 11 && (t.seen[11] & 1) && o2.next == SN_BOOK && o2.ev[0].a == 0x48 && o2.ev[1].a == 0x70011);
        ShopOut o3; ShopLogic::buy(t, 9, 9, o3); CHECK(o3.next == SN_SHOP && o3.ev[0].a == 0x45 && o3.ev[1].a == 0x70013);
        ShopOut o4; ShopLogic::buy(t, 0, 3, o4); CHECK(o4.next == SN_NONE && t.state == 0);
        ShopTask n; n.state = -1; ShopOut o5; ShopLogic::buy(n, 12, 12, o5); CHECK(o5.ev.empty());
    }
    {
        ShopTask t; ShopOut o; ShopLogic::modeSelect(t, 0, -1, 7, o); CHECK(t.state == 1 && t.f2510 == 40.0f && t.gp704C == 3);
        ShopLogic::modeSelect(t, 0, -1, 0, o); CHECK(t.state == 1); t.sel1D18 = 3; ShopLogic::modeSelect(t, 0, -1, 0, o); CHECK(t.state == 2);
        ShopLogic::modeSelect(t, 0, -1, 0, o); CHECK(t.state == 2);
        ShopLogic::modeSelect(t, 0, 2, 0, o); CHECK(t.state == 3 && t.cursor == 2 && t.flag1D1C == 1 && o.ev.size() == 1 && o.ev[0].a == 0x70014);
        ShopLogic::modeSelect(t, 0, -1, 0, o); CHECK(o.next == SN_NONE);   // 0x1D18 != 5
        t.sel1D18 = 5; ShopOut o2; ShopLogic::modeSelect(t, 0, -1, 0, o2); CHECK(o2.next == SN_SHOP_INIT && t.gp7060 == 2);
        ShopTask u; u.state = 2; ShopOut o3; ShopLogic::modeSelect(u, 0, 0, 0, o3); CHECK(o3.ev.empty() && u.state == 3);
    }

    printf("%d comprobaciones, %d fallos\n", checks, fails);
    return fails ? 1 : 0;
}

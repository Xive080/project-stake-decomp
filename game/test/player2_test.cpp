// Pruebas de game/player2 (fogonazo/estela, HUD, cartuchos, particulas, dano, ciclo del jugador).
// Valores de tablas y constantes sacados del asm/ELF (ver comentarios); se omiten las comparaciones con el ELF si no esta baserom/SCES_504.11.
#include "../player2/player2_bullet.h"
#include "../player2/player2_hud.h"
#include "../player2/player2_cycle.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace vn;
static int fails = 0;
#define CHECK(x) do { if (!(x)) { printf("FALLA %s:%d %s\n", __FILE__, __LINE__, #x); fails++; } } while (0)
#define NEAR(a, b, e) do { double _a = (a), _b = (b); if (std::fabs(_a - _b) > (e)) { printf("FALLA %s:%d %s=%g esperado %g\n", __FILE__, __LINE__, #a, _a, _b); fails++; } } while (0)

static float fb(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// ---- ELF ----
static std::vector<uint8_t> elf;
static void loadElf() {
    FILE* f = fopen("baserom/SCES_504.11", "rb");
    if (!f) return;
    uint8_t buf[4096]; size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) elf.insert(elf.end(), buf, buf + n);
    fclose(f);
}
static uint32_t elfWord(uint32_t v) { size_t o = v - 0x140000 + 0x800; uint32_t w; std::memcpy(&w, &elf[o], 4); return w; }

// ---- Host de fogonazo ----
struct BHost : BulletHost {
    int muzzleDraws = 0, tracerDraws = 0; MuzzleStyle lastStyle = MS_DEFAULT; int lastIndex = -1;
    void getAngleYX(float x, float y, float z, int32_t& a, int32_t& b) override { a = (int32_t)(x * 100000); b = (int32_t)(y * 100000); (void)z; }
    void cameraRay(const float in[3], float w[3], float cam[3]) override { w[0] = in[0] * 2; w[1] = in[1] * 2; w[2] = 3.0f; cam[0] = 0; cam[1] = 0; cam[2] = 0; }
    void drawMuzzle(MuzzleStyle s, int, int i, const MuzzleSlot&) override { muzzleDraws++; lastStyle = s; lastIndex = i; }
    void drawTracer(int, int, const TracerSlot&) override { tracerDraws++; }
};

static void testConstants() {
    // inmediatos leidos del asm de bulletConst/bullet/dispCartridge*/dispLifePtcl*
    NEAR(fb(0xBD4D35A8), -0.0501, 1e-6);
    NEAR(fb(0x41A00000), kTracerSpeed, 0);
    NEAR(fb(0x41F00000), kTracerFirstFrameBoost, 0);
    NEAR(fb(0x3727C5AC), 1e-5, 1e-9);
    NEAR(fb(0xB7A7C5AC), -2e-5, 1e-9);
    CHECK(kBulletRing == 6 && kMuzzleLife == 10 && kTracerLife == 50 && kTracerSpin == 0x1000);
    CHECK(sizeof(kStartRows6) / sizeof(kStartRows6[0]) == 6);
}

static void testJtbl() {
    // jtbl_002C2E20 (ELF 0x2C2E20): arma 8..15 -> bloque; el valor del enum es la direccion del bloque
    static const uint32_t expect[8] = {0x20FF68, 0x20FF68, 0x20FF0C, 0x20FEB0, 0x210148, 0x20FEB0, 0x210214, 0x210374};
    for (int i = 0; i < 8; i++) CHECK((uint32_t)muzzleStyleFor(8 + i) == expect[i]);
    CHECK(muzzleStyleFor(7) == MS_DEFAULT && muzzleStyleFor(16) == MS_DEFAULT && muzzleStyleFor(-1) == MS_DEFAULT && muzzleStyleFor(0) == MS_DEFAULT);
    if (!elf.empty()) {
        for (int i = 0; i < 8; i++) CHECK(elfWord(0x2C2E20 + 4 * i) == expect[i]);
        // tabla de colores D_002C2DD0 y D_002C2E00 (no usadas por la logica; solo comprobar que el ELF coincide con lo leido)
        CHECK(elfWord(0x2C2DD0) == 0x100D6 && elfWord(0x2C2E00) == 0x25000F);
    }
}

static void testEffectiveWeapon() {
    BulletEnv e; e.gameState = 0xA; e.gameKind = 1; e.itemSlot0 = 12;
    CHECK(BulletLogic::effectiveWeapon(e) == 12);
    e.flags72D0 = 0x10; CHECK(BulletLogic::effectiveWeapon(e) == 0);           // balas infinitas
    e.flags72D0 = 0; e.gameKind = 0; CHECK(BulletLogic::effectiveWeapon(e) == 0);
    e.gameKind = 1; e.itemSlot0 = 8; e.word502704 = 4; CHECK(BulletLogic::effectiveWeapon(e) == 0);
    e.word502704 = 5; CHECK(BulletLogic::effectiveWeapon(e) == 8);
    e.gameState = 9; CHECK(BulletLogic::effectiveWeapon(e) == 0);
}

static void testBullet() {
    BHost h; BulletPool b; BulletEnv e; e.playerNo = 0; e.g739C = 7; e.sightScale = 100.0f; e.sightX = 50; e.sightY = -20;
    e.gameState = 0xA; e.gameKind = 1; e.itemSlot0 = 12;
    BulletLogic::bulletConst(b, e, h);
    const MuzzleSlot& a = b.muzzle[0]; const TracerSlot& t = b.tracer[0];
    CHECK(a.active == 1 && a.age == 0 && a.weapon == 12 && a.phase == 3 && b.ring == 1);
    float c = fb(0xBD4D35A8);
    NEAR(a.x, -(0.5f) * c, 1e-7); NEAR(a.y, -(-0.2f) * c, 1e-7); NEAR(a.z, c, 0);
    CHECK(a.angA == -(int32_t)(a.x * 100000) && a.angB == (int32_t)(a.y * 100000) + 0x8000);
    // estela: mundo = 2*(x,y), 3; camara en el origen -> velocidad = normalizado * 20
    float wx = 1.0f, wy = -0.4f, wz = 3.0f, len = std::sqrt(wx * wx + wy * wy + wz * wz);
    NEAR(t.x, wx, 1e-6); NEAR(t.vx, wx / len * 20, 1e-4); NEAR(t.vy, wy / len * 20, 1e-4); NEAR(t.vz, wz / len * 20, 1e-4);
    CHECK(t.weapon == 12 && t.phase == 3 && t.age == 0 && t.active == 1);
    // simulacion: fogonazo vive edades 0..9 (10 dibujos); estela se dibuja con edad 1..49 (49 dibujos), desaparece con 50
    float x0 = t.x, vx = t.vx;
    for (int i = 0; i < 60; i++) BulletLogic::bullet(b, e, h);
    CHECK(h.muzzleDraws == 10 && h.lastStyle == MS_W12 && h.lastIndex == 0);
    CHECK(h.tracerDraws == 49);
    CHECK(b.muzzle[0].active == 0 && b.tracer[0].active == 0);
    // posicion: edad 0 -> +v; edad 1 -> +30v; edades 2..49 -> +v (48 veces) => 1 + 30 + 48 = 79 v
    NEAR(b.tracer[0].x, x0 + 79 * vx, 5e-3);
    CHECK(b.tracer[0].spin == 50 * kTracerSpin);
    // anillo: 6 llamadas dan la vuelta
    BulletPool b2; for (int i = 0; i < 6; i++) { e.g739C = i; BulletLogic::bulletConst(b2, e, h); CHECK(b2.ring == (i + 1) % 6); CHECK(b2.muzzle[i].phase == i % 4); }
    // resto de C con contador negativo: -5 % 4 == -1
    BulletPool b3; e.g739C = -5; BulletLogic::bulletConst(b3, e, h); CHECK(b3.muzzle[0].phase == -1);
    // g72F0: estela muere con edad 1; g738C mata ambos
    BulletPool b4; BHost h4; e.g739C = 0; e.g72F0[0] = 1; BulletLogic::bulletConst(b4, e, h4);
    BulletLogic::bullet(b4, e, h4); BulletLogic::bullet(b4, e, h4);
    CHECK(h4.tracerDraws == 0 && b4.tracer[0].active == 0 && b4.muzzle[0].active == 1);
    BulletPool b5; BHost h5; e.g72F0[0] = 0; BulletLogic::bulletConst(b5, e, h5); e.g738C = 1; BulletLogic::bullet(b5, e, h5);
    CHECK(b5.muzzle[0].active == 0 && b5.tracer[0].active == 0 && h5.muzzleDraws == 0);
    // jugador 1 consulta g72F0[1]
    BulletPool b6; BHost h6; e.g738C = 0; e.playerNo = 1; e.g72F0[1] = 1; BulletLogic::bulletConst(b6, e, h6);
    BulletLogic::bullet(b6, e, h6); BulletLogic::bullet(b6, e, h6); CHECK(b6.tracer[0].active == 0);
    BulletLogic::clear(b2); CHECK(b2.ring == 0 && b2.muzzle[3].active == 0 && b2.tracer[3].active == 0);
}

static void testBulletHud() {
    BulletHud hud; HudLogic::layoutInit(hud, 0);
    NEAR(hud.layout[0].x, -0.0145, 1e-6); NEAR(hud.layout[0].y, -0.0076, 1e-6); NEAR(hud.layout[8].y, 0.0004, 1e-6); NEAR(hud.layout[3].z, -0.02, 1e-6);
    BulletHud h1; HudLogic::layoutInit(h1, 1); NEAR(h1.layout[0].x, 0.0145, 1e-6);
    HudLogic::bulletInit(hud);
    for (int i = 0; i < 9; i++) CHECK(hud.icon[i].y == hud.layout[i].y && hud.icon[i].moving == 0);
    // 8 balas maximas, quedan 5 -> desplazamiento 3: cada icono baja hacia la fila i+3 en 4 pasos
    HudLogic::bulletMoveOn(hud, 8, 5);
    NEAR(hud.icon[0].vy, (hud.layout[3].y - hud.layout[0].y) / 4, 1e-9); CHECK(hud.icon[0].moving == 1 && hud.icon[0].step == 0);
    HudBulletIn in; in.maxBullet = 8; in.bullet = 5; in.screenMode = 2;
    for (int f = 0; f < 3; f++) HudLogic::bulletLoop(hud, in);
    CHECK(hud.icon[0].moving == 1 && hud.icon[0].step == 3);
    HudBulletOut o = HudLogic::bulletLoop(hud, in);
    CHECK(hud.icon[0].moving == 0 && hud.icon[0].step == 0);
    NEAR(hud.icon[0].y, hud.layout[3].y, 1e-6);
    CHECK(hud.icon[8].moving == 1);                       // solo se mueven los maxBullet (8) primeros iconos
    CHECK(o.drawIcons && o.iconCount == 5 && !o.drawCount);
    // bullet >= 10: numero y recorte a 9 iconos
    in.bullet = 12; in.maxBullet = 12; o = HudLogic::bulletLoop(hud, in); CHECK(o.drawCount && o.tens == 1 && o.ones == 2 && o.iconCount == 9);
    in.screenMode = 1; o = HudLogic::bulletLoop(hud, in); CHECK(!o.drawIcons && !o.drawCount);
    // aviso de recarga: solo con cargador vacio y gp-0x7418; parpadea 45 de cada 60; >=100 capa fuerte; sonido 1..99 con pistola activa
    HudBulletIn r; r.screenMode = 2; r.emptyFlag = 1; r.active7418 = 1; r.gunTrigger = true; r.gunSightValid = true; r.flag58 = 0;
    o = HudLogic::bulletLoop(hud, r); CHECK(o.reloadBlink && !o.sound && r.flag58 == 1);
    o = HudLogic::bulletLoop(hud, r); CHECK(o.sound == 0x20017 && o.soundParam == 0x40 && r.flag58 == 2);
    r.flag58 = 44; o = HudLogic::bulletLoop(hud, r); CHECK(o.reloadBlink);
    r.flag58 = 45; o = HudLogic::bulletLoop(hud, r); CHECK(!o.reloadBlink);
    r.flag58 = 105; o = HudLogic::bulletLoop(hud, r); CHECK(!o.reloadBlink);       // 105 % 60 = 45
    r.flag58 = 100; o = HudLogic::bulletLoop(hud, r); CHECK(o.reloadBlink && o.reloadStrong && !o.sound);   // 100 % 60 = 40
    r.flag58 = 30; r.pauseLevel = 1; o = HudLogic::bulletLoop(hud, r); CHECK(r.flag58 == 30);
    r.pauseLevel = 0; r.gunSightValid = false; o = HudLogic::bulletLoop(hud, r); CHECK(!o.sound && r.flag58 == 31);
    r.emptyFlag = 0; o = HudLogic::bulletLoop(hud, r); CHECK(!o.reloadBlink && r.flag58 == 31);
    r.emptyFlag = 1; r.active7418 = 0; o = HudLogic::bulletLoop(hud, r); CHECK(!o.reloadBlink && r.flag58 == 31);
    r.active7418 = 1; r.screenMode = 0; o = HudLogic::bulletLoop(hud, r); CHECK(!o.reloadBlink && r.flag58 == 31);
}

static int g_seq[32]; static int g_pos = 0;
static int seqRand() { return g_seq[g_pos++]; }

static void testCartridge() {
    CartridgePool pool; CartridgeLogic::init(pool);
    CartridgeIn in; in.playerNo = 0;
    int s1[] = {5, 7, 3, 4}; std::memcpy(g_seq, s1, sizeof s1); g_pos = 0;
    CartridgeLogic::born(pool, in, 1.0f, 2.0f, 3.0f, seqRand);
    const Cartridge& c = pool.c[0];
    CHECK(pool.next == 1 && c.active == 1 && c.kind == 9 && c.age == 0 && g_pos == 4);
    CHECK(c.angle == 5 && c.angleVel == ((7 + 0x100) & 0x3FF));
    NEAR(c.x, 1, 0); NEAR(c.y, 2, 0); NEAR(c.z, 3, 0);
    NEAR(c.vy, 2e-4 + 1e-5 * 3, 1e-9); NEAR(c.vx, -1e-5 - 1e-5 * 4, 1e-9); NEAR(c.vz, 0, 0);
    // jugador 1: vx positivo
    CartridgePool p2; in.playerNo = 1; g_pos = 0; CartridgeLogic::born(p2, in, 0, 0, 0, seqRand); NEAR(p2.c[0].vx, 1e-5 + 1e-5 * 4, 1e-9);
    // tienda con arma 13: vx*6, edad inicial 90 (jugador 0)
    CartridgePool p3; in.playerNo = 0; in.gameState = 0xA; in.gameKind = 1; in.itemSlot0 = 13; g_pos = 0; CartridgeLogic::born(p3, in, 0, 0, 0, seqRand);
    NEAR(p3.c[0].vx, 6 * (-1e-5 - 4e-5), 1e-8); CHECK(p3.c[0].age == 90);
    in.itemSlot0 = 11; CartridgePool p4; g_pos = 0; CartridgeLogic::born(p4, in, 0, 0, 0, seqRand); CHECK(p4.c[0].age == 90);
    in.itemSlot0 = 12; CartridgePool p5; g_pos = 0; CartridgeLogic::born(p5, in, 0, 0, 0, seqRand); CHECK(p5.c[0].age == 0);
    // pausa: no nace
    CartridgePool p6; in.pauseLevel = 1; g_pos = 0; CartridgeLogic::born(p6, in, 0, 0, 0, seqRand); CHECK(g_pos == 0 && p6.next == 0 && p6.c[0].active == 0);
    // movimiento: gravedad -2e-5 en y, vida 0x79 fotogramas
    float y = c.y, vy = c.vy; int ang = c.angle;
    CartridgeLogic::loop(pool, 0);
    NEAR(pool.c[0].x, 1 + c.vx, 1e-7); NEAR(pool.c[0].y, y + vy, 1e-7); NEAR(pool.c[0].vy, vy - 2e-5, 1e-9); CHECK(pool.c[0].angle == ang + c.angleVel && pool.c[0].age == 1);
    CartridgeLogic::loop(pool, 1); CHECK(pool.c[0].age == 1);
    for (int i = 0; i < 119; i++) CartridgeLogic::loop(pool, 0);
    CHECK(pool.c[0].active == 1 && pool.c[0].age == 120);
    CartridgeLogic::loop(pool, 0); CHECK(pool.c[0].active == 0 && pool.c[0].age == 0);
    // anillo de 18
    CartridgePool ring; in = CartridgeIn();
    for (int i = 0; i < 18; i++) { g_pos = 0; CartridgeLogic::born(ring, in, (float)i, 0, 0, seqRand); }
    CHECK(ring.next == 0); NEAR(ring.c[17].x, 17, 0);
    g_pos = 0; CartridgeLogic::born(ring, in, 99, 0, 0, seqRand); NEAR(ring.c[0].x, 99, 0); CHECK(ring.next == 1);
}

static void testLifeParticles() {
    LifeParticlePool pool; LifeParticleLogic::init(pool); pool.velX = 0.5f;
    // r1 % 10 < 5 -> x se mueve con (r2 % 5 - 2); r3 % 3 -> vy; &0xFFF dos veces; r6 % 4 -> escala
    int s[] = {3, 4, 1, 0x1ABC, 0x2DEF, 3}; std::memcpy(g_seq, s, sizeof s); g_pos = 0;
    CHECK(LifeParticleLogic::born(pool, 0, 1.0f, 2.0f, 3.0f, seqRand));
    const LifeParticle& q = pool.p[0];
    NEAR(q.x, 1.0 + 2e-4 * (4 % 5 - 2), 1e-7); NEAR(q.y, 2.0, 0); NEAR(q.z, 3.0, 0); NEAR(q.vx, 0.5, 0);
    NEAR(q.vy, 1e-5 * -((1 % 3) + 2), 1e-10); CHECK(q.active == 1 && q.age == 0);
    CHECK(q.spinAVel == 0xABC && q.spinBVel == 0xDEF); NEAR(q.scale, 0.00132, 1e-8); NEAR(q.scaleVel, 2e-5 * -(3 + 1), 1e-9);
    CHECK(g_pos == 6);
    // segunda: r1 % 10 >= 5 -> mueve y
    int s2[] = {7, 0, 2, 1, 2, 0}; std::memcpy(g_seq, s2, sizeof s2); g_pos = 0;
    CHECK(LifeParticleLogic::born(pool, 0, 1.0f, 2.0f, 3.0f, seqRand));
    NEAR(pool.p[1].x, 1.0, 0); NEAR(pool.p[1].y, 2.0 + 2e-4 * (0 - 2), 1e-7);
    // pausa: nada
    g_pos = 0; CHECK(!LifeParticleLogic::born(pool, 1, 0, 0, 0, seqRand) && g_pos == 0);
    // escala 0.00132 con -8e-5 por fotograma: <= 0 tras 17 pasos
    int n = 0; while (pool.p[0].active && n < 200) { LifeParticleLogic::loop(pool, 0); n++; }
    CHECK(n == 17); CHECK(pool.p[0].age == 17);
    // la segunda: escala -2e-5*(0+1): 66 o 67 pasos (<81), nunca llega a 81
    int m = 17; while (pool.p[1].active && m < 300) { LifeParticleLogic::loop(pool, 0); m++; }
    CHECK(m >= 66 && m <= 67);
    // sin velocidad de escala negativa: vive 81 fotogramas (age >= 0x51)
    LifeParticlePool pl; pl.p[0].active = 1; pl.p[0].scale = 1.0f; pl.p[0].scaleVel = 0;
    for (int i = 0; i < 80; i++) LifeParticleLogic::loop(pl, 0);
    CHECK(pl.p[0].active == 1 && pl.p[0].age == 80);
    LifeParticleLogic::loop(pl, 0); CHECK(pl.p[0].active == 0 && pl.p[0].age == 81);
    // 32 particulas: la 33 no nace
    LifeParticlePool full; for (int i = 0; i < 32; i++) full.p[i].active = 1;
    g_pos = 0; CHECK(!LifeParticleLogic::born(full, 0, 0, 0, 0, seqRand) && g_pos == 0);
    // nace en el primer hueco
    full.p[5].active = 0; int s3[] = {0, 2, 0, 0, 0, 0}; std::memcpy(g_seq, s3, sizeof s3); g_pos = 0;
    CHECK(LifeParticleLogic::born(full, 0, 0, 0, 0, seqRand) && full.p[5].active == 1);
    LifeParticleLogic::init(pool); CHECK(pool.p[0].active == 0 && pool.velX == 0);
}

static float sinTurn(int a) { return (float)std::sin((a & 0xFFFF) * 6.283185307179586 / 65536.0); }

static void testDamage() {
    DmgShakeSys s;
    // sin golpe: todo a 0
    DmgLogic::dmgEftPlayer(s, sinTurn); CHECK(s.timer == 0 && s.shakeX == 0 && s.shakeY == 0);
    // golpe del jugador 0: 40 -> 39 el mismo fotograma; el aviso se consume
    s.hitP0 = 1; DmgLogic::dmgEftPlayer(s, sinTurn);
    CHECK(s.hitP0 == 0 && s.timer == 39);
    double ex = 0.025 * (48.0 * 39) * std::sin(39 * 0x1800 * 6.283185307179586 / 65536.0);
    CHECK(std::abs(s.shakeX - (int)ex) <= 1); CHECK(s.shakeY == 0);
    for (int i = 0; i < 38; i++) DmgLogic::dmgEftPlayer(s, sinTurn);
    CHECK(s.timer == 1);
    DmgLogic::dmgEftPlayer(s, sinTurn); CHECK(s.timer == 0 && s.shakeX == 0);
    DmgLogic::dmgEftPlayer(s, sinTurn); CHECK(s.timer == 0);
    // jugador 1 y bit 0x10 anula el temblor pero cuenta
    s.hitP1 = 1; s.flags72D0 = 0x10; DmgLogic::dmgEftPlayer(s, sinTurn); CHECK(s.hitP1 == 0 && s.timer == 39 && s.shakeX == 0 && s.shakeY == 0);
    // los dos golpes a la vez: sigue en 40 -> 39
    DmgShakeSys t; t.hitP0 = 1; t.hitP1 = 1; DmgLogic::dmgEftPlayer(t, sinTurn); CHECK(t.timer == 39 && t.hitP0 == 0 && t.hitP1 == 0);
    // la funcion de seno entra con angulo & 0xFFFF y el argumento es timer*0x1800
    CHECK(0x1800 == 6144);
    // PostPlayerTask::exec
    DmgShakeSys u; int32_t g = 99;
    CHECK(DmgLogic::postExec(u, sinTurn, 2, 5, g) && g == 2);
    CHECK(DmgLogic::postExec(u, sinTurn, 1, 5, g) && g == 0);
    CHECK(DmgLogic::postExec(u, sinTurn, 1, 7, g) && g == 0);
    CHECK(DmgLogic::postExec(u, sinTurn, 1, 6, g) && g == 1);
    CHECK(DmgLogic::postExec(u, sinTurn, 0, 6, g) && g == -1);
    g = 77; CHECK(!DmgLogic::postExec(u, sinTurn, 3, 6, g) && g == 77);
    CHECK(!DmgLogic::postExec(u, sinTurn, -1, 6, g) && g == 77);
    // PrePlayerTask::exec
    uint32_t f = 0xDB;
    DmgLogic::preExec(f, 1, 0, 0xA); CHECK(f == 0xDA);                 // sin bits 0x24: se apaga el bit 0, el resto igual
    f = 0xFF; DmgLogic::preExec(f, 1, 0, 0xA); CHECK(f == 0xFF);       // con bits 0x24: se pone el bit 0
    f = 0; DmgLogic::preExec(f, 1, 0, 0xA); CHECK((f & 1) == 0);
    f = 0x4; DmgLogic::preExec(f, 1, 0, 0xA); CHECK(f == 0x5);         // bit 2 activo -> se pone el bit 0
    f = 0x20; DmgLogic::preExec(f, 1, 0, 0xA); CHECK(f == 0x21);
    f = 0; DmgLogic::preExec(f, 2, 0, 0xA); CHECK(f == 1);             // camara distinta de 1
    f = 0; DmgLogic::preExec(f, 1, 7, 0xA); CHECK(f == 1);
    f = 0; DmgLogic::preExec(f, 1, 0, 9); CHECK(f == 1);
    f = 1; DmgLogic::preExec(f, 1, 6, 0xA); CHECK(f == 0);
}

// ---- Host del ciclo ----
struct CHost : CycleHost {
    BHost bh; std::vector<int> calls; std::vector<P2Exec> execs; int lastDispatchMode = -1;
    std::vector<int> sounds; int credBtnMode = -1; bool credResult = false; int orgIn[2] = {0, 0};
    int drawBullets_n = 0, drawCart_n = 0, drawLife_n = 0, drawPtcl_n = 0, entry_n = 0;
    enum { GUN = 1, ORG, ORGOFF, SIGHT, DAMREQ, DAM, CLR, SCORE, BOOKSTART, BOOKEND, BOOKCONT, RESET, VELX };
    BulletHost& bulletHost() override { return bh; }
    void changeExec(P2Exec t, int) override { execs.push_back(t); }
    void dispatchModeInit(int m, int) override { lastDispatchMode = m; }
    void setBookPlGameStart(int) override { calls.push_back(BOOKSTART); }
    void setBookPlGameEnd(int) override { calls.push_back(BOOKEND); }
    void setBookPlGameCont(int) override { calls.push_back(BOOKCONT); }
    void resetSightPosPl(int) override { calls.push_back(RESET); }
    void orgJoinIn(int, int w) override { orgIn[w]++; }
    bool checkCredAndStBtn(int, int m) override { credBtnMode = m; return credResult; }
    void clrDamEff(int) override { calls.push_back(CLR); }
    void dispDamEff(int) override { calls.push_back(DAM); }
    void dispDamEffReq(int) override { calls.push_back(DAMREQ); }
    void dispSight(int) override { calls.push_back(SIGHT); }
    void gunExe(int) override { calls.push_back(GUN); }
    void gunOrgExe(int) override { calls.push_back(ORG); }
    void gunOrgSoundOff(int) override { calls.push_back(ORGOFF); }
    void calcLifeVelX(PlayerRound&) override { calls.push_back(VELX); }
    void drawBullets(int, const HudBulletOut&) override { drawBullets_n++; }
    void drawCartridges(int, const CartridgePool&) override { drawCart_n++; }
    void drawLife(PlayerRound&) override { drawLife_n++; }
    void drawLifeParticles(int, const LifeParticlePool&) override { drawPtcl_n++; }
    void scoreDispCheck(int) override { calls.push_back(SCORE); }
    void sound(int id, int) override { sounds.push_back(id); }
    void entryPut(int) override { entry_n++; }
    bool has(int c) const { for (int x : calls) if (x == c) return true; return false; }
};

static void testStartTable() {
    CHECK(kStartRows6[0].flags == 61 && kStartRows6[0].mode == 5 && kStartRows6[0].value == 75);
    CHECK(kStartRows6[1].flags == 25 && kStartRows6[1].mode == 5 && kStartRows6[1].value == 150);
    CHECK(kStartRows6[2].flags == 8 && kStartRows6[2].mode == 5 && kStartRows6[2].value == 75);
    CHECK(kStartRows6[3].flags == 61 && kStartRows6[3].mode == 5 && kStartRows6[3].value == 150);
    CHECK(kStartRows6[4].flags == 60 && kStartRows6[4].mode == 9 && kStartRows6[4].value == 75);
    CHECK(kStartRows6[5].flags == 61 && kStartRows6[5].mode == 9 && kStartRows6[5].value == 75);
    if (!elf.empty()) {
        for (int i = 0; i < 6; i++) {
            CHECK(elfWord(0x274AF0 + 12 * i) == kStartRows6[i].flags);
            CHECK((int)elfWord(0x274AF4 + 12 * i) == kStartRows6[i].mode);
            CHECK((int)elfWord(0x274AF8 + 12 * i) == kStartRows6[i].value);
        }
        // changeExec: campo +8 de cada ptmf
        CHECK(elfWord(0x274B98 + 8) == (uint32_t)P2Exec::GameLoop && elfWord(0x274BA8 + 8) == (uint32_t)P2Exec::GameLoop);
        CHECK(elfWord(0x274BB8 + 8) == (uint32_t)P2Exec::GameLoop && elfWord(0x274BC8 + 8) == (uint32_t)P2Exec::GameLoop);
        CHECK(elfWord(0x274C48 + 8) == (uint32_t)P2Exec::GameLoop);
        CHECK(elfWord(0x274C28 + 8) == (uint32_t)P2Exec::JoinWaitLoop && elfWord(0x274C38 + 8) == (uint32_t)P2Exec::WaitLoop);
        CHECK(elfWord(0x274C58 + 8) == (uint32_t)P2Exec::Init && elfWord(0x274B58 + 8) == (uint32_t)P2Exec::Init);
        CHECK(elfWord(0x274B48 + 8) == (uint32_t)P2Exec::CommonLoop);
    }
}

static void initRound(PlayerRound& r, CycleSys& s, int pl = 0) {
    s.cfg.gameState = 1; s.cfg.gameKind = 0; s.cfg.cfgInitLife = 3; s.cfg.cfgMaxLife = 5;
    r.playerNo = pl; HudLogic::layoutInit(r.hud, pl);
}

static void testStartInit() {
    CycleSys s; PlayerRound r; CHost h; initRound(r, s);
    r.p.score = 500; r.p.life = 1; r.p.bullet = 0; r.p.emptyFlag = 1; r.p.reloadReq = 1; r.p.flag58 = 9; r.p.bulletFlag54 = 1; r.p.sightX = 4; r.p.sightY = 5;
    r.fieldB4 = 1; r.dmgEff[0] = r.dmgEff[1] = r.dmgEff[2] = 1; r.bullets.ring = 3; r.bullets.muzzle[2].active = 1; r.bullets.tracer[5].active = 1;
    for (int i = 0; i < 7; i++) r.shopFlags[i] = 1;
    CHECK(PlayerCycle::startInit(r, s, h, 0));
    CHECK(r.p.life == 3 && r.p.lifePrev == 3 && r.p.score == 0 && s.credits500DF8 == 1 && s.credits500DFC == 1);
    CHECK(s.invuln[0] == 75 && s.invuln[1] == 0 && r.fieldB4 == 0 && r.dmgEff[0] == 0 && r.dmgEff[2] == 0);
    CHECK(r.p.maxBullet == 8 && r.p.bullet == 8 && r.p.emptyFlag == 0 && r.p.reloadReq == 0 && r.p.flag58 == 0 && r.p.bulletFlag54 == 0);
    CHECK(r.p.sightX == 0 && r.p.sightY == 0 && r.p.mode == 5);
    CHECK(r.bullets.ring == 0 && r.bullets.muzzle[2].active == 0 && r.bullets.tracer[5].active == 0);
    CHECK(r.shopFlags[0] == 1 && r.shopFlags[6] == 1);                  // gameKind != 1: no se borran
    // continuar (fila 1, flags 0x19): vida a inicial, sin borrar la puntuacion, credito A, invulnerabilidad 150
    r.p.score = 700; r.p.life = 0; PlayerCycle::startInit(r, s, h, SK_CONT);
    CHECK(r.p.life == 3 && r.p.score == 700 && s.credits500DF8 == 2 && s.credits500DFC == 1 && s.invuln[0] == 150 && r.p.mode == 5);
    // fase nueva (fila 2, flags 8): no toca vida ni creditos
    r.p.life = 2; PlayerCycle::startInit(r, s, h, SK_STAGE);
    CHECK(r.p.life == 2 && s.credits500DF8 == 2 && s.credits500DFC == 1 && s.invuln[0] == 75);
    // unirse (fila 3): flags 61, 150
    r.p.score = 10; PlayerCycle::startInit(r, s, h, SK_JOIN);
    CHECK(r.p.score == 0 && s.invuln[0] == 150 && s.credits500DF8 == 3 && s.credits500DFC == 2);
    // demo (fila 4, flags 60 = sin recuperar vida): modo 9
    r.p.life = 1; PlayerCycle::startInit(r, s, h, SK_DEMO);
    CHECK(r.p.mode == 9 && r.p.life == 1 && s.credits500DF8 == 4 && s.credits500DFC == 3 && s.invuln[0] == 75);
    // fila 5: flags 61, modo 9
    PlayerCycle::startInit(r, s, h, 5); CHECK(r.p.mode == 9 && r.p.life == 3);
    // jugador 1 escribe su invulnerabilidad
    CycleSys s2; PlayerRound r2; initRound(r2, s2, 1); PlayerCycle::startInit(r2, s2, h, 0); CHECK(s2.invuln[1] == 75 && s2.invuln[0] == 0);
    // kind fuera de rango
    CHECK(!PlayerCycle::startInit(r, s, h, 6) && !PlayerCycle::startInit(r, s, h, -1));
    // gameKind == 1 borra los 7 bytes de tienda
    s2.cfg.gameState = 0xA; s2.cfg.gameKind = 1; for (int i = 0; i < 7; i++) r2.shopFlags[i] = 1;
    s2.cfg.bulletTable[0] = 6; s2.cfg.itemSlot[0] = 0;
    PlayerCycle::startInit(r2, s2, h, 0); for (int i = 0; i < 7; i++) CHECK(r2.shopFlags[i] == 0);
    CHECK(r2.p.maxBullet == 6 && r2.p.bullet == 6);
    // dispBulletInit: iconos vuelven a su sitio
    r2.hud.icon[2].y = 9; r2.hud.icon[2].moving = 1; PlayerCycle::startInit(r2, s2, h, 0); CHECK(r2.hud.icon[2].y == r2.hud.layout[2].y && r2.hud.icon[2].moving == 0);
}

static void testInits() {
    CycleSys s; PlayerRound r; CHost h; initRound(r, s);
    PlayerCycle::firstGameInit(r, s, h);
    CHECK((r.p.flags & 1) && h.has(CHost::BOOKSTART) && h.execs.size() == 1 && h.execs[0] == P2Exec::GameLoop && r.p.mode == 5 && s.credits500DF8 == 1);
    // contGameInit: g73C6 -3 con piso 0, borra efecto de dano si lo hay
    CHost h2; s.g73C6 = 7; r.dmgEff[0] = 1; r.p.score = 10;
    PlayerCycle::contGameInit(r, s, h2);
    CHECK(s.g73C6 == 4 && h2.has(CHost::CLR) && h2.has(CHost::BOOKEND) && h2.has(CHost::BOOKCONT) && h2.has(CHost::RESET) && r.p.score == 11);
    CHECK(h2.execs.size() == 1 && h2.execs[0] == P2Exec::GameLoop);
    // score: addScore(+1) luego startInit(1) no borra (flags 0x19 sin 4): 10 + 1
    CHost h3; CycleSys s3; PlayerRound r3; initRound(r3, s3); r3.p.score = 10; s3.g73C6 = 2; PlayerCycle::contGameInit(r3, s3, h3);
    CHECK(r3.p.score == 11 && s3.g73C6 == 0 && !h3.has(CHost::CLR));
    CHost h4; PlayerCycle::stageGameInit(r3, s3, h4); CHECK(h4.execs.size() == 1 && h4.execs[0] == P2Exec::GameLoop && r3.p.mode == 5);
    // joinGameInit: pone a 0 los contadores; org solo en 0xA/kind 1
    CHost h5; r3.p.shotsFired = 4; r3.p.hits = 3; r3.p.shotsTotal = 9; r3.p.hitsTotal = 8; r3.field60 = 5;
    PlayerCycle::joinGameInit(r3, s3, h5);
    CHECK(r3.p.shotsFired == 0 && r3.p.hits == 0 && r3.p.shotsTotal == 0 && r3.p.hitsTotal == 0 && r3.field60 == 0 && h5.orgIn[0] == 0 && h5.orgIn[1] == 0);
    CHECK(h5.has(CHost::RESET) && h5.has(CHost::BOOKSTART) && h5.execs[0] == P2Exec::GameLoop && (r3.p.flags & 1));
    s3.cfg.gameState = 0xA; s3.cfg.gameKind = 1; CHost h6; PlayerCycle::joinGameInit(r3, s3, h6); CHECK(h6.orgIn[0] == 1 && h6.orgIn[1] == 1);
    s3.cfg.gameKind = 0; CHost h7; PlayerCycle::joinGameInit(r3, s3, h7); CHECK(h7.orgIn[0] == 0);
    // demoInit
    CHost h8; s3.cfg.gameState = 9; PlayerCycle::demoInit(r3, s3, h8); CHECK(r3.p.mode == 9 && h8.execs[0] == P2Exec::GameLoop);
    // init: despacha por modo
    CHost h9; r3.p.mode = 10; PlayerCycle::init(r3, h9); CHECK(h9.lastDispatchMode == 10);
    // commonLoop
    r3.taskFlags27FC = 0xFF; PlayerCycle::commonLoop(r3, h9); CHECK(r3.taskFlags27FC == 0 && h9.entry_n == 1);
}

static void testWait() {
    CycleSys s; PlayerRound r; CHost h; initRound(r, s);
    // waitInit
    PlayerCycle::waitInit(r, h); CHECK(h.execs.size() == 1 && h.execs[0] == P2Exec::WaitLoop);
    // waitLoop: bit 0 de gp-0x72D0 -> modo = +0x94 y init
    s.flags72D0 = 1; r.reqMode94 = 6; PlayerCycle::waitLoop(r, s, h); CHECK(r.p.mode == 6 && h.lastDispatchMode == 6 && r.taskFlags27FC == 0);
    // sin bit 0: parpadeo con bit 0x20 del contador y sin congelar
    s.flags72D0 = 0; s.g739C = 0x20; h.lastDispatchMode = -1; PlayerCycle::waitLoop(r, s, h); CHECK(r.taskFlags27FC == 1 && h.lastDispatchMode == -1);
    r.taskFlags27FC = 0; s.g739C = 0x1F; PlayerCycle::waitLoop(r, s, h); CHECK(r.taskFlags27FC == 0);
    s.g739C = 0x20; s.g738C = 1; PlayerCycle::waitLoop(r, s, h); CHECK(r.taskFlags27FC == 0);
    // joinWaitInit: estado != 9 -> JoinWaitLoop y una pasada de joinWaitLoop_main; estado 9 -> demoInit
    CHost h2; s.g738C = 0; s.cfg.gameState = 0xA; r.p.mode = 9; s.flags72D0 = 2; h2.credResult = true;
    PlayerCycle::joinWaitInit(r, s, h2);
    CHECK(h2.execs.size() == 1 && h2.execs[0] == P2Exec::JoinWaitLoop && h2.credBtnMode == 3 && r.joinReqC9 == 1 && (r.taskFlags27FC & 0x10));
    CHost h3; s.cfg.gameState = 9; PlayerCycle::joinWaitInit(r, s, h3); CHECK(h3.execs.size() == 1 && h3.execs[0] == P2Exec::GameLoop && r.p.mode == 9);
    // joinWaitLoopMain: tabla de condiciones. modo de comprobacion de creditos: 3 en 0xA, 0 en otro
    struct Case { int state; int mode; uint32_t f; int expectC9; int expectBtn; };
    Case cs[] = { {0xA, 9, 2, 1, 3}, {0xA, 9, 0, 0, 3}, {0xA, 5, 2, 0, 3}, {1, 9, 2, 0, 0}, {1, 9, 0, 0, 0}, {0xA, 10, 3, 0, 3} };
    for (auto& c : cs) {
        CycleSys ss; PlayerRound rr; CHost hh; initRound(rr, ss); ss.cfg.gameState = c.state; rr.p.mode = c.mode; ss.flags72D0 = c.f; rr.joinReqC9 = 1;
        PlayerCycle::joinWaitLoopMain(rr, ss, hh);
        CHECK(rr.joinReqC9 == c.expectC9); CHECK(hh.credBtnMode == c.expectBtn); CHECK(((rr.taskFlags27FC & 0x10) != 0) == (c.expectC9 == 1));
    }
}

static void testGameLoop() {
    // a) vida > 0, estado normal: gunExe, bullet, dibujo y lifePrev
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 1; r.p.life = 3; r.p.lifePrev = 0; s.screenMode = 2; s.g7418 = 1; s.gunDevice = 1; s.gunSightValid = true;
        PlayerCycle::gameLoop(r, s, h);
        CHECK(h.has(CHost::SIGHT) && h.has(CHost::GUN) && !h.has(CHost::ORG) && h.drawBullets_n == 1 && h.drawCart_n == 1 && h.drawLife_n == 1 && h.drawPtcl_n == 1);
        CHECK(r.p.lifePrev == 3 && h.has(CHost::DAM) && h.has(CHost::SCORE) && (r.taskFlags27FC & 8) && h.has(CHost::VELX) && r.p.mode == 0 && h.execs.empty());
        // orden: mira primero, puntuacion al final
        CHECK(h.calls.front() == CHost::SIGHT && h.calls.back() == CHost::SCORE);
    }
    // b) la mira no se dibuja con GunCon (2/3), congelado, sin vida, sin permiso o sin pistola activa
    {
        for (int k = 0; k < 6; k++) {
            CycleSys s; PlayerRound r; CHost h; initRound(r, s); r.p.life = 2; s.g7418 = 1; s.gunSightValid = true; s.gunDevice = 1;
            if (k == 0) s.gunDevice = 2; if (k == 1) s.gunDevice = 3; if (k == 2) s.g738C = 1; if (k == 3) r.p.life = 0; if (k == 4) s.g7418 = 0; if (k == 5) s.gunSightValid = false;
            CHECK(!PlayerCycle::sightVisible(s, r));
        }
        CycleSys s; PlayerRound r; initRound(r, s); r.p.life = 1; s.g7418 = 1; s.gunSightValid = true; CHECK(PlayerCycle::sightVisible(s, r));
    }
    // c) vida 0 en modo normal: modo 4 con exec, creditos -1, bytes de muerte, sin dibujo
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 1; r.p.life = 0; s.credits500DF8 = 3;
        PlayerCycle::gameLoop(r, s, h);
        CHECK(r.p.mode == 4 && s.credits500DF8 == 2 && s.g7400 == 1 && s.g73FC == 1 && h.execs.size() == 1 && h.execs[0] == P2Exec::Init);
        CHECK(h.drawBullets_n == 0 && !h.has(CHost::GUN) && !h.has(CHost::ORGOFF) && h.has(CHost::SCORE));
    }
    // d) vida 0 en estado 0xA / kind 1: tambien silencia el arma org
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 0xA; s.cfg.gameKind = 1; r.p.life = 0;
        PlayerCycle::gameLoop(r, s, h);
        CHECK(h.has(CHost::ORGOFF) && r.p.mode == 4);
    }
    // e) estado 0xA / kind 1 con vida: arma org, salvo bit 0x10 o modo 8
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 0xA; s.cfg.gameKind = 1; r.p.life = 2; r.p.mode = 5;
        PlayerCycle::gameLoop(r, s, h); CHECK(h.has(CHost::ORG) && !h.has(CHost::GUN));
        CHost h2; s.flags72D0 = 0x10; PlayerCycle::gameLoop(r, s, h2); CHECK(h2.has(CHost::GUN) && !h2.has(CHost::ORG));
        CHost h3; s.flags72D0 = 0; r.p.mode = 8; PlayerCycle::gameLoop(r, s, h3); CHECK(h3.has(CHost::GUN) && !h3.has(CHost::ORG));
    }
    // f) pausa >= 4: no hay disparo; en 0xA / kind 1 solo silencia
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 1; r.p.life = 2; s.pauseLevel = 4;
        PlayerCycle::gameLoop(r, s, h); CHECK(!h.has(CHost::GUN) && !h.has(CHost::ORGOFF));
        CHost h2; s.cfg.gameState = 0xA; s.cfg.gameKind = 1; PlayerCycle::gameLoop(r, s, h2); CHECK(!h2.has(CHost::ORG) && h2.has(CHost::ORGOFF));
        CHost h3; s.pauseLevel = 3; PlayerCycle::gameLoop(r, s, h3); CHECK(h3.has(CHost::ORG));
    }
    // g) gp-0x72A0: gp-0x7418 == 0, gp-0x729C != 0 y pistola activa con mira
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); r.p.life = 2; s.g7418 = 0; s.g729C = 1; s.gunTrigger = true; s.gunSightValid = true;
        PlayerCycle::gameLoop(r, s, h); CHECK(s.g72A0 == 1);
        CycleSys s2; PlayerRound r2; CHost h2; initRound(r2, s2); r2.p.life = 2; s2.g7418 = 0; s2.g729C = 1; s2.gunTrigger = true; s2.gunSightValid = false;
        PlayerCycle::gameLoop(r2, s2, h2); CHECK(s2.g72A0 == 0);
        // con vida 0 no llega a evaluarse
        CycleSys s3; PlayerRound r3; CHost h3; initRound(r3, s3); r3.p.life = 0; s3.g7418 = 0; s3.g729C = 1; s3.gunTrigger = true; s3.gunSightValid = true;
        PlayerCycle::gameLoop(r3, s3, h3); CHECK(s3.g72A0 == 0);
    }
    // h) estado 9: vida fija a 1, no pasa por el bloque de muerte ni por el dibujo
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 9; r.p.life = 0;
        PlayerCycle::gameLoop(r, s, h);
        CHECK(r.p.life == 1 && r.p.mode == 0 && h.drawBullets_n == 0 && h.execs.empty() && h.has(CHost::GUN));
        CycleSys s2; PlayerRound r2; CHost h2; initRound(r2, s2); s2.cfg.gameState = 0xE; r2.p.life = 0; PlayerCycle::gameLoop(r2, s2, h2);
        CHECK(r2.p.mode == 0 && h2.execs.empty() && s2.g7400 == 0);
    }
    // i) saltos de dibujo en 0xA: g73D8 == 5 con g73D4 == 3 o g73F0 en {5, 6, 15, 16}
    {
        struct C { int8_t d8, d4, f0; bool draw; };
        C cs[] = { {5, 3, 0, false}, {5, 0, 5, false}, {5, 0, 6, false}, {5, 0, 15, false}, {5, 0, 16, false}, {5, 0, 17, true}, {5, 0, 14, true}, {5, 0, 4, true}, {4, 3, 5, true}, {0, 0, 0, true} };
        for (auto& c : cs) {
            CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 0xA; r.p.life = 2; r.p.lifePrev = 0; s.g73D8 = c.d8; s.g73D4 = c.d4; s.g73F0 = c.f0;
            PlayerCycle::gameLoop(r, s, h);
            CHECK((h.drawBullets_n == 1) == c.draw); CHECK((r.p.lifePrev == 2) == c.draw);
        }
        // fuera de 0xA siempre se dibuja
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 1; r.p.life = 2; s.g73D8 = 5; s.g73D4 = 3; PlayerCycle::gameLoop(r, s, h); CHECK(h.drawBullets_n == 1);
    }
    // j) dano: efecto pedido si flagsC8 bit 0 == 0 y damageFlag; si el bit esta puesto se borra el efecto activo en lugar de dibujarlo
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); r.p.life = 2; r.p.damageFlag = 1;
        PlayerCycle::gameLoop(r, s, h); CHECK(h.has(CHost::DAMREQ) && h.has(CHost::DAM) && !h.has(CHost::CLR));
        CycleSys s2; PlayerRound r2; CHost h2; initRound(r2, s2); r2.p.life = 2; r2.p.damageFlag = 1; r2.flagsC8 = 1; r2.dmgEff[0] = 1;
        PlayerCycle::gameLoop(r2, s2, h2); CHECK(!h2.has(CHost::DAMREQ) && !h2.has(CHost::DAM) && h2.has(CHost::CLR));
        CycleSys s3; PlayerRound r3; CHost h3; initRound(r3, s3); r3.p.life = 2; r3.flagsC8 = 1; PlayerCycle::gameLoop(r3, s3, h3); CHECK(!h3.has(CHost::CLR) && !h3.has(CHost::DAM));
    }
    // k) demo: joinWaitLoopMain tras el bucle
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 0xA; r.p.life = 2; r.p.mode = 9; s.flags72D0 = 2; s.demoPlay = true;
        PlayerCycle::gameLoop(r, s, h); CHECK(h.credBtnMode == 3 && r.joinReqC9 == 1);
        CHost h2; s.demoPlay = false; r.joinReqC9 = 0; h2.credBtnMode = -1; PlayerCycle::gameLoop(r, s, h2); CHECK(h2.credBtnMode == -1);
    }
    // l) setMaxBullet al inicio: recorta las balas al maximo
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.cfg.gameState = 1; r.p.life = 2; r.p.bullet = 20; PlayerCycle::gameLoop(r, s, h); CHECK(r.p.maxBullet == 8 && r.p.bullet == 8);
    }
    // m) aviso de recarga a traves del bucle: el contador avanza dentro del bucle de dibujo
    {
        CycleSys s; PlayerRound r; CHost h; initRound(r, s); r.p.life = 2; r.p.emptyFlag = 1; r.p.bullet = 3; s.screenMode = 2; s.g7418 = 1; s.gunTrigger = true; s.gunSightValid = true;
        r.p.flag58 = 5; PlayerCycle::gameLoop(r, s, h); CHECK(r.p.flag58 == 6 && h.sounds.size() == 1 && h.sounds[0] == 0x20017);
    }
}

static void testOnShot() {
    // disparo normal con iconos visibles: bulletConst, sonido del jugador, cartucho en el icono recien gastado y movimiento de iconos
    CycleSys s; PlayerRound r; CHost h; initRound(r, s); s.screenMode = 2; s.sightScale = 100.0f;
    r.p.maxBullet = 8; r.p.bullet = 3; r.p.sightX = 50; r.p.sightY = 20; HudLogic::bulletInit(r.hud);
    g_pos = 0; for (int i = 0; i < 32; i++) g_seq[i] = 0;
    PlayerCycle::onShot(r, s, h, false, seqRand);
    CHECK(r.bullets.muzzle[0].active == 1 && r.bullets.tracer[0].active == 1 && r.bullets.ring == 1);
    CHECK(h.sounds.size() == 1 && h.sounds[0] == 0x2000C);
    CHECK(r.cartridges.c[0].active == 1 && r.cartridges.next == 1);
    NEAR(r.cartridges.c[0].x, r.hud.icon[3].x, 0); NEAR(r.cartridges.c[0].y, r.hud.icon[3].y, 0);
    CHECK(r.hud.icon[0].moving == 1);
    // jugador 1: sonido 0x2000D
    CycleSys s1; PlayerRound r1; CHost h1; initRound(r1, s1, 1); s1.screenMode = 2; r1.p.maxBullet = 8; r1.p.bullet = 1; HudLogic::bulletInit(r1.hud);
    PlayerCycle::onShot(r1, s1, h1, false, seqRand); CHECK(h1.sounds.size() == 1 && h1.sounds[0] == 0x2000D);
    // balas infinitas: sin cartucho, pero los iconos se mueven igual
    CycleSys s2; PlayerRound r2; CHost h2; initRound(r2, s2); s2.screenMode = 2; r2.p.maxBullet = 8; r2.p.bullet = 8; HudLogic::bulletInit(r2.hud);
    PlayerCycle::onShot(r2, s2, h2, true, seqRand); CHECK(r2.cartridges.c[0].active == 0 && r2.hud.icon[0].moving == 1);
    // pantalla reducida: sin cartucho
    CycleSys s3; PlayerRound r3; CHost h3; initRound(r3, s3); s3.screenMode = 1; r3.p.maxBullet = 8; r3.p.bullet = 3; HudLogic::bulletInit(r3.hud);
    PlayerCycle::onShot(r3, s3, h3, false, seqRand); CHECK(r3.cartridges.c[0].active == 0 && r3.hud.icon[0].moving == 1);
    // con mas de 8 balas el icono es el 8
    CycleSys s4; PlayerRound r4; CHost h4; initRound(r4, s4); s4.screenMode = 2; r4.p.maxBullet = 12; r4.p.bullet = 11; HudLogic::bulletInit(r4.hud);
    r4.hud.icon[8].x = 7.0f; PlayerCycle::onShot(r4, s4, h4, false, seqRand); NEAR(r4.cartridges.c[0].x, 7.0, 0);
    // recarga: dispBulletInit
    r4.hud.icon[1].y = 5; r4.hud.icon[1].moving = 1; PlayerCycle::onReload(r4); CHECK(r4.hud.icon[1].y == r4.hud.layout[1].y && r4.hud.icon[1].moving == 0);
}

int main() {
    loadElf();
    testConstants(); testJtbl(); testEffectiveWeapon(); testBullet(); testBulletHud(); testCartridge(); testLifeParticles(); testDamage();
    testStartTable(); testStartInit(); testInits(); testWait(); testGameLoop(); testOnShot();
    if (fails == 0) printf("player2_test: OK%s\n", elf.empty() ? " (sin comparacion con el ELF)" : " (con ELF)");
    return fails ? 1 : 0;
}

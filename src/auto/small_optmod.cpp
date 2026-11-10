// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.
int c_001A2460(...);
int c_001AB090(...);
int c_001C9730(...);
int c_001F0D40(...);
int c_001F2FF0(...);
int c_001F33B0(...);
int c_002030D0(...);
int c_0092B0C0(...);
int c_0092B130(...);
int c_0092CB20(...);
int c_0092CD50(...);
extern char g_002C8760_char[64];
extern char g_00500F10_char[64];
extern char g_00500F20_char[64];
extern char g_00502900_char[64];
extern char g_00502901_char[64];
extern char g_00502902_char[64];
extern char g_00502908_char[64];
extern char g_00502914_char[64];
extern signed char g_0093DB28_signedchar[64];
extern char g_0093F800_char[64];
extern char g_0093F801_char[64];
extern signed char g_0093F801_signedchar[64];
extern char g_0093F802_char[64];
extern signed char g_0093F802_signedchar[64];
extern char g_0093F803_char[64];
extern signed char g_0093F803_signedchar[64];
extern signed char g_0093F804_signedchar[64];
extern signed char g_0093F805_signedchar[64];
extern char g_0093F810_char[64];
extern int g_0093F810_int[64];
extern int g_0093F814_int[64];
extern int g_0093F818_int[64];
extern int g_0093F81C_int[64];
extern int g_0093F820_int[64];
extern int g_0093F824_int[64];
extern char g_0093F830_char[64];
extern signed char g_gp_8C10_signedchar;
extern signed char g_gp_8C14_signedchar;
extern char g_gp_8C18_char;
extern int g_gp_8C1C_int;
extern int g_gp_8C60_int;
extern int g_gp_8CC8_int;
extern int g_gp_8DC0_int;

// JvsConnect__Fv @ 0x92B080
int f_0092B080_r(void) { int m0; signed char t0 = g_gp_8C10_signedchar; if (!(t0 == 1)) { int t1 = c_0092B0C0(); m0 = t1; } else { int t2 = c_0092B130(); m0 = t2; } return m0; }

// setScrAttr__10CCsoptItemFi @ 0x92BBD0
void f_0092BBD0_r(int a0, int a1) { (*(int *)(a0 + 224)) = a1; }

// getFlag__10CCsoptItemFv @ 0x92BD80
int f_0092BD80_r(int a0) { int t0 = (*(int *)(a0 + 228)); return t0; }

// add__11CCsoptPanelFP10CCsoptItem @ 0x92BE80
void f_0092BE80_r(void) {  }

// remove__11CCsoptPanelFP10CCsoptItem @ 0x92BE90
void f_0092BE90_r(void) {  }

// actvOff__11CCsoptPanelFv @ 0x92C0E0
void f_0092C0E0_r(int a0) { (*(int *)(a0 + 256)) = 0; }

// actvOn__11CCsoptPanelFv @ 0x92C0F0
void f_0092C0F0_r(int a0) { (*(int *)(a0 + 256)) = 1; }

// remove__19CCompositeCsoptItemFP10CCsoptItem @ 0x92C4F0
void f_0092C4F0_r(void) {  }

// CsOption__Fv @ 0x92C920
int f_0092C920_r(void) { int m0; signed char t0 = g_gp_8C10_signedchar; if (!(t0 == 1)) { int t1 = c_0092CB20(); m0 = t1; } else { int t2 = c_0092CD50(); m0 = t2; } return m0; }

// setFade__5CMenuFi @ 0x933B00
void f_00933B00_r(void) {  }

// immediateChangeMenu__12CsOptionTaskFi @ 0x933B10
void f_00933B10_r(int a0, int a1) { (*(int *)(a0 + 16)) = a1; }

// setNextMenu__12CsOptionTaskFi @ 0x933B20
void f_00933B20_r(int a0, int a1) { (*(int *)(a0 + 20)) = a1; }

// updateMenu__12CsOptionTaskFv @ 0x933B30
void f_00933B30_r(int a0) { int t0 = (*(int *)(a0 + 20)); (*(int *)(a0 + 16)) = t0; }

// initFocus__12CsOptionTaskFv @ 0x9345C0
void f_009345C0_r(int a0) { (*(int *)(a0 + 40)) = -1; (*(int *)(a0 + 44)) = -1; (*(int *)(a0 + 36)) = -1; (*(int *)(a0 + 64)) = -1; (*(int *)(a0 + 68)) = -1; (*(int *)(a0 + 48)) = -1; (*(int *)(a0 + 72)) = 0; }

// getColli__10CCsoptItemFPfPf @ 0x934A80
void f_00934A80_r(void) {  }

// loop__18CsOptionCameraTaskFv @ 0x93B450
int f_0093B450_r(void) { int t0 = g_gp_8CC8_int; (*(int *)(t0 + 144)) = 0; int t1 = g_gp_8CC8_int; (*(int *)(t1 + 148)) = 0; int t2 = g_gp_8CC8_int; (*(int *)(t2 + 152)) = 0; int t3 = g_gp_8CC8_int; (*(int *)(t3 + 156)) = 0; int t4 = g_gp_8CC8_int; (*(int *)(t4 + 160)) = 0; int t5 = g_gp_8CC8_int; (*(int *)(t5 + 164)) = 0; int t6 = c_001C9730(); return t6; }

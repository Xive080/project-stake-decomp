// Byte-match verificado contra SCES_504.11 con mwcps2-2.4-001213, flags: -lang c++ -O4,p
// Modulo gamemod, grupo free. Generado por tools/vn_small2.py --module gamemod --max-instr 60 --permute 40;
// cada funcion lleva su nombre real y direccion en el comentario.
int c_001A2460(...);
int c_001A2630(...);
int c_001C9730(...);
int c_001F0AD0(...);
int c_001F3390(...);
int c_001F33B0(...);
int c_00214A70(...);
int c_0092C2D0(...);
int c_0092C8A0(...);
int c_0092CD90(...);
int c_0092D340(...);
int c_0092ECD0(...);
int c_00932C00(...);
int c_00943E80(...);
int c_00945550(...);
int c_009495B0(...);
int c_00950E20(...);
int c_00957040(...);
int c_0095DA60(...);
int c_0096C510(...);
int c_0096C7B0(...);
int c_0096D0B0(...);
int c_00978280(...);
int c_00978560(...);
int c_009785C0(...);
int c_0097AB90(...);
int c_0097F420(...);
int c_0097F9F0(...);
extern char g_0026B4E0_char[64];
extern char g_0027496C_char[64];
extern char g_002C9528_char[64];
extern char g_004A2238_char[64];
extern int g_00500734_int[64];
extern char g_00500794_char[64];
extern int g_00500A7C_int[64];
extern char g_00500E36_char[64];
extern short g_00500F14_short[64];
extern char g_00500F20_char[64];
extern int g_00502664_int[64];
extern char g_00502909_char[64];
extern char g_00502C40_char[64];
extern char g_00515EC0_char[64];
extern char g_00515EE0_char[64];
extern char g_009857A0_char[64];
extern long long g_009A16A0_longlong[64];
extern char g_009A25B0_char[64];
extern char g_009AE640_char[64];
extern char g_009B5580_char[64];
extern char g_009B6B50_char[64];
extern char g_009B6B80_char[64];
extern char g_009B6BB0_char[64];
extern char g_009B6F90_char[64];
extern char g_009B7430_char[64];
extern char g_009B7450_char[64];
extern int g_009BB7E4_int[64];
extern char g_009BC7E0_char[64];
extern char g_009BC804_char[64];
extern char g_009BC80C_char[64];
extern char g_009C0400_char[64];
extern char g_009C0404_char[64];
extern char g_009C39A0_char[64];
extern long long g_12001000_longlong[64];
extern int g_gp_8C0C_int;
extern char g_gp_8C10_char;
extern signed char g_gp_8C10_signedchar;
extern signed char g_gp_8C28_signedchar;
extern signed char g_gp_8C2C_signedchar;
extern signed char g_gp_8C30_signedchar;
extern short g_gp_8C38_short;
extern short g_gp_8C3A_short;
extern int g_gp_8C4C_int;
extern int g_gp_8C60_int;
extern short g_gp_8C74_short;
extern short g_gp_8CA8_short;
extern int g_gp_8CB0_int;
extern int g_gp_8CB4_int;
extern int g_gp_8CC8_int;
extern int g_gp_8D30_int;
extern int g_gp_8DC0_int;
extern int g_gp_9004_int;
extern int g_gp_9010_int;
extern int g_gp_9014_int;
extern int g_gp_9018_int;
extern int g_gp_9028_int;
extern int g_gp_9040_int;
extern int g_gp_9058_int;
extern int g_gp_9060_int;
extern int g_gp_9064_int;
extern int g_gp_9068_int;


// Vector_Hit_FixBox__Fv @ 0x92C850
int f_0092C850_r(void) { int t0 = g_gp_9004_int; (*(int *)((((int)g_009BC80C_char) + (((t0 << 4) - t0) << 2)) + 0)) = 0; (*(int *)((((int)g_009BC804_char) + (((t0 << 4) - t0) << 2)) + 0)) = 0; int t1 = c_0092C8A0((((t0 << 4) - t0) << 2), t0); return t1; }
// Vector_Hit_Box_Sub2__FP9EnemyTask @ 0x92CA10
void f_0092CA10_r(int a0) { g_00502664_int[0] = a0; }
// SpecularErase_strip_modify__FP19tagNL_PF_StripParamiPv @ 0x92E340
void f_0092E340_r(void) {  }
// SpecularErase_mat_modify__FP19tagNL_PF_GloblParamiPv @ 0x92E350
void f_0092E350_r(int a0) { int t0 = (*(int *)(a0 + 8)); (*(int *)(a0 + 8)) = (t0 & -29); int t1 = (*(int *)(a0 + 100)); (*(int *)(a0 + 100)) = (t1 & -25); }
// TyakudanMgrInit__Fv @ 0x92F240
void f_0092F240_r(void) { g_gp_9014_int = 0; }
// TyakudanMgrSet__FP16TyakudanCtrlTask @ 0x92F250
void f_0092F250_r(int a0) { g_gp_9014_int = a0; }
// TyakudanMgrKillAll__Fv @ 0x92F2A0
int f_0092F2A0_r(void) { int t0 = g_gp_9014_int; int t1 = c_0092ECD0(t0); return t1; }
// TyakudanLauncherMgrInit__Fv @ 0x92F720
void f_0092F720_r(void) { g_gp_9018_int = 0; }
// WeakInit__FP12ColliObjTask @ 0x939280
void f_00939280_r(int a0) { (*(int *)(a0 + 120)) = 0; (*(int *)(a0 + 124)) = 0; (*(int *)(a0 + 128)) = 0; (*(int *)(a0 + 132)) = 0; }
// EnSndInit__Fv @ 0x942690
void f_00942690_rg(void) { int l0; int l1; int t0; l0 = ((int)g_009857A0_char); l1 = ((int)g_009C39A0_char); goto Lc2; Lt2: ; (*(int *)(l1 + 0)) = t0; (*(int *)(l1 + 4)) = 0; (*(int *)(l1 + 8)) = -1; l0 = (l0 + 4); l1 = (l1 + 12); Lc2: ; t0 = (*(int *)(l0 + 0)); if (t0 != -1) goto Lt2; }
// new_EnEtc2__FP11ENEMY_TABLE @ 0x9435A0
void f_009435A0_r(void) {  }
// prism_callback_func__FPv @ 0x945270
int f_00945270_r(void) { int t0 = c_00945550(); return t0; }
// TaskCloseX__FP9EnemyTask @ 0x950F80
int f_00950F80_r(void) { int t0 = c_00950E20(); int t1 = c_001A2630(); return t1; }
// GetDifficulty__Fv @ 0x96CEC0
int f_0096CEC0_r(void) { short t0 = g_gp_8C3A_short; return t0; }
// GetFirstDifficulty__Fv @ 0x96CED0
int f_0096CED0_r(void) { short t0 = g_gp_8C38_short; return t0; }

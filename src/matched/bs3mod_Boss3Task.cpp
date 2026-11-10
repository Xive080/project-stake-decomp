// Byte-match verificado contra SCES_504.11 con mwcps2-2.4-001213, flags: -lang c++ -O4,p
// Modulo bs3mod, grupo Boss3Task. Generado por tools/vn_small2.py --module bs3mod --max-instr 60 --permute 40;
// cada funcion lleva su nombre real y direccion en el comentario.
int c_001A2630(...);
int c_00215220(...);
int c_0096CEC0(...);
int c_0096CED0(...);
int c_009CAD90(...);
int c_009CB840(...);
int c_009CC500(...);
extern int g_004A13E0_int[64];
extern short g_00500DFC_short[64];
extern char g_00500F07_char[64];
extern char g_00515F00_char[64];
extern char g_009D0C90_char[64];
extern char g_009D17B0_char[64];
extern int g_gp_8C48_int;
extern short g_gp_8F88_short;
extern short g_gp_8F8A_short;
extern int g_gp_8F90_int;


// actNop__9Boss3TaskFv @ 0x9C7FA0
int f_009C7FA0_r(void) {  return 1; }
// SeqCmdInit__9Boss3TaskFPPs @ 0x9C9160
void f_009C9160_r(int a0, int a1) { (*(int *)(a0 + 7612)) = a1; (*(short *)(a0 + 7616)) = 0; short t0 = (*(short *)(a0 + 7616)); int t1 = (*(int *)(a0 + 7612)); int t2 = (*(int *)((t1 + (t0 << 2)) + 0)); (*(int *)(a0 + 7608)) = t2; (*(char *)(a0 + 7621)) = 0; }
// SeqCmdSetTable__9Boss3TaskFs @ 0x9C91C0
void f_009C91C0_r(int a0, int a1) { (*(short *)(a0 + 7616)) = a1; short t0 = (*(short *)(a0 + 7616)); int t1 = (*(int *)(a0 + 7612)); int t2 = (*(int *)((t1 + (t0 << 2)) + 0)); (*(int *)(a0 + 7608)) = t2; (*(char *)(a0 + 7621)) = 0; }
// AllowCameraStart__9Boss3TaskFv @ 0x9C9310
void f_009C9310_r(void) { g_00500F07_char[0] = 1; }
// ForbidCameraStart__9Boss3TaskFv @ 0x9C9320
void f_009C9320_r(void) { g_00500F07_char[0] = 0; }
// EffectCtrl__9Boss3TaskFv @ 0x9CA090
void f_009CA090_r(void) {  }
// endBlur__9Boss3TaskFv @ 0x9CA290
int f_009CA290_r(void) { int t0 = c_00215220(((int)g_00515F00_char), ((int)g_009D17B0_char)); return t0; }

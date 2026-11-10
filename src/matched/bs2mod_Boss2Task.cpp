// Byte-match verificado contra SCES_504.11 con mwcps2-2.4-001213, flags: -lang c++ -O4,p
// Modulo bs2mod, grupo Boss2Task. Generado por tools/vn_small2.py --module bs2mod --max-instr 60 --permute 40;
// cada funcion lleva su nombre real y direccion en el comentario.
int c_001A2630(...);
int c_00950F80(...);
int c_009C7E80(...);
int c_009CBDF0(...);
extern int g_004A13E0_int[64];
extern char g_00500E17_char[64];
extern char g_009D78EA_char[64];
extern int g_gp_8D6C_int;


// PutEffect__9Boss2TaskFv @ 0x9C7690
void f_009C7690_r(void) {  }
// DownDifficultyReq__9Boss2TaskFi @ 0x9C7E20
int f_009C7E20_r(void) { int t0 = c_009C7E80(); return t0; }
// NoChgDifficultyReq__9Boss2TaskFi @ 0x9C7E40
void f_009C7E40_r(int a0) { (*(char *)(a0 + 8452)) = 0; }
// actNop__9Boss2TaskFv @ 0x9C9400
int f_009C9400_r(void) {  return 1; }
// SeqCmdInit__9Boss2TaskFPPs @ 0x9C9DD0
void f_009C9DD0_r(int a0, int a1) { (*(int *)(a0 + 8280)) = a1; (*(short *)(a0 + 8284)) = 0; short t0 = (*(short *)(a0 + 8284)); int t1 = (*(int *)(a0 + 8280)); int t2 = (*(int *)((t1 + (t0 << 2)) + 0)); (*(int *)(a0 + 8276)) = t2; (*(char *)(a0 + 8288)) = 0; (*(char *)(a0 + 8289)) = 0; }
// FinishTask__9Boss2TaskFv @ 0x9CA220
int f_009CA220_r(void) { int t0 = c_00950F80(); return t0; }
// AllowCameraStart__9Boss2TaskFv @ 0x9CA240
void f_009CA240_r(void) { g_00500E17_char[0] = 1; }
// ForbidCameraStart__9Boss2TaskFv @ 0x9CA250
void f_009CA250_r(void) { g_00500E17_char[0] = 0; }

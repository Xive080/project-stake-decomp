// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.
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

// CtrlPut__14TBoss2WarpEff2Fv @ 0x9CC310
int f_009CC310_r(int a0) { int t0 = c_009CBDF0(a0, 1); return t0; }

// CtrlPost__14TBoss2WarpEff2Fv @ 0x9CC330
void f_009CC330_r(void) {  }

// CtrlPre__9TBoss2EffFv @ 0x9CCB40
void f_009CCB40_v(void) { int t0 = g_gp_8D6C_int; if (!(t0 == 0)) { int t1 = c_001A2630(); } }

// CtrlPost__12TZanzoEffectFv @ 0x9CE270
void f_009CE270_r(void) {  }

// CtrlPost__10TSwordFireFv @ 0x9CF0B0
void f_009CF0B0_r(void) {  }

// CtrlPre__13TSonicWaveEffFv @ 0x9D0C40
void f_009D0C40_r(void) {  }

// CtrlPre__15TBoss2DamageEffFv @ 0x9D1950
void f_009D1950_r(void) {  }

// KillAllChildren__18TFlyingPillarBlockFv @ 0x9D4340
void f_009D4340_r(void) { g_009D78EA_char[0] = 1; }

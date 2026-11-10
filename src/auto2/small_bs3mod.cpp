// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.
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

// set_xpipe_strip__FP19tagNL_PF_StripParamiPv @ 0x9C6FD0
void f_009C6FD0_r(void) {  }

// set_xpipe_material__FP19tagNL_PF_GloblParamiPv @ 0x9C6FE0
void f_009C6FE0_r(int a0) { int t0 = (*(int *)(a0 + 8)); (*(int *)(a0 + 8)) = (t0 | 4096); }

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

// prtsDispFunc__14Boss3XWingTaskFP7_BODYPT @ 0x9CAE30
int f_009CAE30_r(void) { int t0 = c_009CAD90(); return t0; }

// CtrlPost__14TBS3XWingLightFv @ 0x9CB280
void f_009CB280_v(int a0) { int t0 = (*(int *)(a0 + 44)); if (!(t0 == 0)) { int t1 = c_001A2630(); } }

// prtsDispFunc__15Boss3SubchrTaskFP7_BODYPT @ 0x9CB8E0
int f_009CB8E0_r(void) { int t0 = c_009CB840(); return t0; }

// new_Boss3DamageEff__FP9EnemyTaski @ 0x9CB900
void f_009CB900_r(void) {  }

// CtrlPre_Fall__10TBoss3BombFv @ 0x9CC970
int f_009CC970_r(void) { int t0 = c_009CC500(); return t0; }

// CtrlPre_Appear__10TBoss3BombFv @ 0x9CCAD0
void f_009CCAD0_r(void) {  }

// CtrlPre_Explosion__10TBoss3BombFv @ 0x9CCBB0
void f_009CCBB0_r(void) {  }

// ResetAcc__10TBoss3BombFv @ 0x9CCBC0
void f_009CCBC0_r(int a0) { (*(int *)(a0 + 536)) = 0; (*(int *)(a0 + 540)) = -1121939410; (*(int *)(a0 + 544)) = 0; }

// CtrlPost__13TBoss3BombEyeFv @ 0x9CE1A0
void f_009CE1A0_r(int a0) { (*(char *)(a0 + 173)) = 0; }

// CtrlPost__11TBoss3xHandFv @ 0x9D00B0
void f_009D00B0_v(int a0) { int t0 = (*(int *)(a0 + 56)); (*(int *)(a0 + 56)) = (t0 + 1); if (!((t0 < 10) != 0)) { int t1 = c_001A2630(a0, t0); } }

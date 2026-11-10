// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.
int c_001A2630(...);
int c_00215220(...);
int c_0092CD90(...);
int c_00950F80(...);
int c_0096CEC0(...);
int c_0096CED0(...);
int c_009C6DE0(...);
int c_009CBD60(...);
int c_009CEF00(...);
extern int g_004A13E0_int[64];
extern short g_00500DFC_short[64];
extern char g_00500F07_char[64];
extern char g_00515F00_char[64];
extern char g_009D3490_char[64];
extern char g_009D4520_char[64];
extern int g_gp_8C48_int;
extern short g_gp_8FD0_short;
extern short g_gp_8FD2_short;
extern int g_gp_8FD8_int;
extern int g_gp_8FF8_int;

// BodyInit__9Boss6TaskFv @ 0x9C6CC0
void f_009C6CC0_r(int a0) { (*(char *)(a0 + 7597)) = 18; }

// prtsDispFunc__9Boss6TaskFP7_BODYPT @ 0x9C6F00
int f_009C6F00_r(void) { int t0 = c_009C6DE0(); return t0; }

// colliFunc__9Boss6TaskFv @ 0x9C6F20
void f_009C6F20_r(void) {  }

// actNop__9Boss6TaskFv @ 0x9C7E70
int f_009C7E70_r(void) {  return 1; }

// SeqCmdInit__9Boss6TaskFPPs @ 0x9C9690
void f_009C9690_r(int a0, int a1) { (*(int *)(a0 + 7612)) = a1; (*(short *)(a0 + 7616)) = 0; short t0 = (*(short *)(a0 + 7616)); int t1 = (*(int *)(a0 + 7612)); int t2 = (*(int *)((t1 + (t0 << 2)) + 0)); (*(int *)(a0 + 7608)) = t2; (*(short *)(a0 + 7618)) = 0; (*(char *)(a0 + 7621)) = 0; }

// SeqCmdSetTable__9Boss6TaskFs @ 0x9C96F0
void f_009C96F0_r(int a0, int a1) { (*(short *)(a0 + 7616)) = a1; short t0 = (*(short *)(a0 + 7616)); int t1 = (*(int *)(a0 + 7612)); int t2 = (*(int *)((t1 + (t0 << 2)) + 0)); (*(int *)(a0 + 7608)) = t2; (*(short *)(a0 + 7618)) = 0; }

// FinishTask__9Boss6TaskFv @ 0x9C9840
int f_009C9840_r(void) { int t0 = c_00950F80(); return t0; }

// AllowCameraStart__9Boss6TaskFv @ 0x9C9860
void f_009C9860_r(void) { g_00500F07_char[0] = 1; }

// ForbidCameraStart__9Boss6TaskFv @ 0x9C9870
void f_009C9870_r(void) { g_00500F07_char[0] = 0; }

// CtrlBossLight__9Boss6TaskFv @ 0x9CA440
void f_009CA440_r(int a0) { int t0 = (*(int *)(a0 + 8096)); if (!((t0 < 5) != 0)) { int t1 = (*(int *)(a0 + 8100)); (*(int *)(a0 + 8100)) = (t1 + 1); int t2 = (*(int *)(a0 + 8100)); if (!((t2 < 37) != 0)) { int t3 = (*(int *)(a0 + 8096)); (*(int *)(a0 + 8096)) = (t3 + -5); } } }

// endBlur__9Boss6TaskFv @ 0x9CB2C0
int f_009CB2C0_r(void) { int t0 = c_00215220(((int)g_00515F00_char), ((int)g_009D4520_char)); return t0; }

// prtsDispFunc__14Boss6XWingTaskFP7_BODYPT @ 0x9CBE00
int f_009CBE00_r(void) { int t0 = c_009CBD60(); return t0; }

// EntryCollision__14Boss6XWingTaskFv @ 0x9CBE20
void f_009CBE20_r(void) {  }

// ChgDead__10TBoss6XBatFv @ 0x9CD660
void f_009CD660_r(int a0) { (*(short *)(a0 + 446)) = 5; }

// ClearTBoss6XBulletDamage__Fv @ 0x9CE190
void f_009CE190_r(void) { g_gp_8FF8_int = 0; }

// CtrlPut__13TBoss6XBulletFv @ 0x9CED70
int f_009CED70_r(void) { int t0 = c_009CEF00(); return t0; }

// CtrlPost__15TBS6MaskFeatherFv @ 0x9D26A0
void f_009D26A0_v(int a0) { int t0 = (*(int *)(a0 + 4)); (*(int *)(a0 + 4)) = (t0 + 1); int t1 = (*(int *)(a0 + 4)); int t2 = (*(int *)(a0 + 8)); if (!((t1 < t2) != 0)) { int t3 = c_001A2630(a0, t1); } }

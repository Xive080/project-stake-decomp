// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.
int c_001A2630(...);
int c_00950F80(...);
int c_009CA390(...);
int c_009CF050(...);
extern char g_00500E10_char[64];
extern int g_gp_8CE8_int;
extern int g_gp_8D68_int;
extern int g_gp_8D6C_int;

// StartAlphaAdd__6TBoss4Fv @ 0x9C98E0
void f_009C98E0_r(int a0) { int t0 = (*(int *)(a0 + 7700)); (*(int *)(a0 + 7700)) = (t0 | 1048576); (*(int *)(a0 + 7720)) = 1017370379; }

// StartAlphaSub__6TBoss4Fv @ 0x9C9900
void f_009C9900_r(int a0) { int t0 = (*(int *)(a0 + 7700)); (*(int *)(a0 + 7700)) = (t0 | 1048576); (*(int *)(a0 + 7720)) = -1130113269; }

// ControlEffect__6TBoss4Fv @ 0x9C9920
void f_009C9920_r(void) {  }

// CancelBunshin__6TBoss4Fv @ 0x9C9EF0
void f_009C9EF0_r(int a0) { int t0 = (*(int *)(a0 + 7700)); (*(int *)(a0 + 7700)) = (t0 & -32769); int t1 = (*(int *)(a0 + 7700)); (*(int *)(a0 + 7700)) = (t1 & -65537); (*(short *)(a0 + 7748)) = 0; unsigned char t2 = (*(unsigned char *)(a0 + 7708)); if (!(t2 != 0)) { int t3 = (*(int *)(a0 + 7700)); (*(int *)(a0 + 7700)) = (t3 | 32); } }

// DownDifficultyReq__6TBoss4Fi @ 0x9CA330
int f_009CA330_r(void) { int t0 = c_009CA390(); return t0; }

// NoChgDifficultyReq__6TBoss4Fi @ 0x9CA350
void f_009CA350_r(int a0) { (*(char *)(a0 + 8044)) = 0; }

// actNop__6TBoss4Fv @ 0x9CBA80
int f_009CBA80_r(void) {  return 1; }

// FinishTask__6TBoss4Fv @ 0x9CCF60
int f_009CCF60_r(void) { int t0 = c_00950F80(); return t0; }

// AllowCameraStart__6TBoss4Fv @ 0x9CCF80
void f_009CCF80_r(void) { g_00500E10_char[0] = 1; }

// ForbidCameraStart__6TBoss4Fv @ 0x9CCF90
void f_009CCF90_r(void) { g_00500E10_char[0] = 0; }

// InitGareki__6TBoss4Fv @ 0x9CE0F0
void f_009CE0F0_r(int a0) { (*(int *)(a0 + 8488)) = 0; (*(int *)(a0 + 8492)) = 0; (*(int *)(a0 + 8496)) = 0; (*(int *)(a0 + 8500)) = 0; (*(int *)(a0 + 8504)) = 0; (*(char *)(a0 + 8508)) = 0; (*(int *)(a0 + 8512)) = 0; (*(char *)(a0 + 8544)) = 0; }

// InitBackEff__6TBoss4Fv @ 0x9CEBC0
void f_009CEBC0_r(int a0) { (*(int *)(a0 + 10476)) = 0; }

// ControlMaskModel__6TBoss4Fv @ 0x9CF030
int f_009CF030_r(void) { int t0 = c_009CF050(); return t0; }

// opmp_genzan__FP19tagNL_PF_GloblParamiPv @ 0x9CF760
void f_009CF760_r(int a0) { (*(int *)(a0 + 112)) = 66; }

// opmp_strip_no_alphablend__FP19tagNL_PF_StripParamiPv @ 0x9D0150
void f_009D0150_r(int a0) { int t0 = (*(int *)(a0 + 12)); (*(int *)(a0 + 12)) = (t0 & -2097153); int t1 = (*(int *)(a0 + 16)); (*(int *)(a0 + 16)) = (t1 & -2097153); }

// PrePut__11TBoss4EgureFv @ 0x9D1E50
void f_009D1E50_v(int a0) { short t0 = (*(short *)(a0 + 28)); (*(short *)(a0 + 28)) = (t0 + -1); if (!(t0 != 0)) { int t1 = c_001A2630(a0, t0); } }

// PostPut__15TBoss4HensinEffFv @ 0x9D2700
void f_009D2700_r(int a0) { int t0 = (*(int *)(a0 + 4)); (*(int *)(a0 + 4)) = (t0 + 1); }

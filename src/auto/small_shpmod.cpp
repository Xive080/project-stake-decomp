// AUTO: funciones con byte-match (tools/vn_small.py). Nombres reales en comentario.
int c_001A2630(...);
int c_001F0D40(...);
int c_00202F90(...);
int c_009CEC40(...);
int c_009CECB0(...);
int c_009CEE30(...);
extern char g_002C2098_char[64];
extern char g_002C20A0_char[64];
extern int g_00502704_int[64];
extern char g_00503850_char[64];
extern int g_00503850_int[64];
extern int g_00503854_int[64];
extern int g_00503858_int[64];
extern int g_0050385C_int[64];
extern int g_00503860_int[64];
extern int g_00503864_int[64];
extern int g_009D4000_int[64];
extern unsigned short g_gp_8C74_unsignedshort;
extern int g_gp_8DC0_int;

// disp__18TOrgNowLoadingDispFv @ 0x9C5730
int f_009C5730_r(void) { int m0; unsigned short t0 = g_gp_8C74_unsignedshort; if (!(t0 == 0)) { int t1 = c_00202F90(); m0 = t1; } else { int t2 = c_001A2630(); m0 = t2; } return m0; }

// OrgShopInitDispSilver__12OriginalTaskFv @ 0x9C6520
void f_009C6520_r(int a0) { (*(int *)(a0 + 7476)) = 0; int t0 = g_00502704_int[0]; (*(int *)(a0 + 7480)) = t0; int t1 = g_00502704_int[0]; (*(int *)(a0 + 7484)) = t1; }

// guncolli_reset__12OriginalTaskFv @ 0x9CBB60
void f_009CBB60_r(int a0) { (*(int *)(a0 + 7416)) = -1; (*(int *)(a0 + 7412)) = -1; (*(int *)(a0 + 7428)) = -1; (*(int *)(a0 + 7424)) = -1; (*(int *)(a0 + 7432)) = -1; (*(int *)(a0 + 7436)) = 0; }

// finish__18TOrgRequestDispReqFv @ 0x9CD9C0
int f_009CD9C0_r(void) { int t0 = c_001A2630(); return t0; }

// EquipBook_Loop__Fv @ 0x9D0350
int f_009D0350_r(void) { int t0 = g_009D4000_int[0]; int t1 = c_009CECB0(t0); int t2 = g_009D4000_int[0]; int t3 = c_009CEE30(t2); return t3; }

// EquipBook_Delete__Fv @ 0x9D0380
int f_009D0380_r(void) { int t0 = g_009D4000_int[0]; int t1 = c_009CEC40(t0, 1); g_009D4000_int[0] = 0; return t1; }

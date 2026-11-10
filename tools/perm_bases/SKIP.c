extern int g_7294, g_729c, g_7298;
struct EventCtrlTask { char pad[0xc]; int * volatile pc; };
void SKIP(struct EventCtrlTask *self) {
    if (self->pc[1]) { g_7294 = 0; g_729c = 1; }
    else { g_729c = 0; g_7298 = 0; }
    self->pc += 2;
}

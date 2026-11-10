extern int g_7304;
struct EventCtrlTask { char pad[0xc]; int * volatile pc; };
void WLCTB(struct EventCtrlTask *self) {
    g_7304 = (int)(self->pc + 1);
    do { self->pc++; } while (*self->pc != -1);
    self->pc++;
}

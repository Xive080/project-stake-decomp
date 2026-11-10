struct EventCtrlTask { char pad[0xc]; int * volatile pc; };
void ITEMSET(struct EventCtrlTask *self) {
    if (self->pc[2] == 0) *(char *)(0x500e10 + self->pc[1]) = 0;
    else *(char *)(0x500e10 + self->pc[1]) = self->pc[2];
    self->pc += 3;
}

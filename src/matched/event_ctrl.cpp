// Byte-match verificado contra SCES_504.11 con mwcps2-2.4-001213, flags: -lang c++ -O4,p
// EventCtrlTask: handlers de opcodes del script de eventos (pc en el offset 0xC del objeto).
// Verificar: CC_ONLY=mwcps2 python3 tools/probe_compilers.py src/matched/event_ctrl.cpp -O4,p

extern int g_7304;     // gp-0x7304
extern char g500e10[]; // 0x500e10 (direccion de simbolo: el original usa lui+addiu, no ori)

struct EventCtrlTask {
    char pad[0xc];
    int *pc;
    void WLCTB();      // @ 0x1e2e80
    void ITEMSET();    // @ 0x1e3b00
};

void EventCtrlTask::WLCTB() {
    g_7304 = (int)(pc + 1);
    do { pc++; } while (*pc != -1);
    pc++;
}

void EventCtrlTask::ITEMSET() {
    if (pc[2] == 0) g500e10[pc[1]] = 0;
    else g500e10[pc[1]] = pc[2];
    pc += 3;
}

// SKIP__13EventCtrlTaskFv @ 0x1e3010: PENDIENTE (d9-d11: el original rellena el delay slot del beqz con `li v1,1`).

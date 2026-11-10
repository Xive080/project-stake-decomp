// Byte-match verificado contra SCES_504.11 con mwcps2-2.4-001213, flags: -lang c++ -O4,p
// (tools/probe_compilers.py <fuente> -O4,p con CC_ONLY=mwcps2)

struct BGCtrlClass {                  // appointBG__11BGCtrlClassFPUi @ 0x1c8b10
    char pad0[8];
    short s8;
    short sA;
    unsigned int *appointBG(unsigned int *p);
};
unsigned int *BGCtrlClass::appointBG(unsigned int *p) {
    sA = s8;
    s8 = *(unsigned short *)(p + 1);
    return p + 2;
}

struct CameraMgrInternalClass {       // chkRemainCamJob__22CameraMgrInternalClassFv @ 0x1cb150
    char pad[0x900];
    short s900;
    short s902;
    int chkRemainCamJob();
};
int CameraMgrInternalClass::chkRemainCamJob() {
    return (s900 == s902) ^ 1;
}

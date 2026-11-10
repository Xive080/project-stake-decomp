# Manifiesto de game/bosses_rest (segunda tanda: pendiente de bosses_a / bosses_b)

Estados: reescrita+test / reescrita sin test / pendiente. Direcciones = primera instruccion del simbolo en el overlay (base 0x9C4800), de bs2mod (Boss2) y bs3mod (Boss3).
"reescrita+test" = alguna asercion de game/test/bosses_rest_test.cpp ejecuta la funcion. Los tests comprueban coherencia con el asm leido a mano y con los datos del BIN;
NO se ha comparado contra ejecucion real del juego. m2c se uso solo de borrador (fallo con tablas de saltos y se corrigio a mano contra el asm).

Compilar y ejecutar (desde la raiz del repo):

    mkdir -p build/verify_boss_rest && c++ -std=c++17 -Wall game/test/bosses_rest_test.cpp game/bosses_rest/boss2_seq.cpp game/bosses_rest/boss3_seq.cpp game/bosses_rest/boss3_bomb.cpp game/bosses_rest/boss_rest_data.cpp game/bosses_a/boss2.cpp game/bosses_a/boss3.cpp game/bosses_a/boss_data.cpp -o build/verify_boss_rest/bosses_rest_test && build/verify_boss_rest/bosses_rest_test

Resultado de la ultima ejecucion: `273 comprobaciones, 0 fallos`.

## Datos
`boss_rest_data.cpp` lo genera `python3 game/bosses_rest/tools/gen_tables.py` (desde la raiz) leyendo disc/TBL/BS3MOD.BIN y BS2MOD.BIN:
- `kBs3HitRows[3][34][2]` = D_009D0A20 (golpes por impacto / para aturdir segun fase y dificultad), usada por la op 39 de Boss3.
- `kBs2OpLen[30]` = D_009D6250 (cuanto avanza cada operacion de Boss2), tabla de longitudes de ExecSeqCmd.
El test compara ambas con los BIN y verifica la tabla de saltos jtbl_009D1DE0 (Boss3, 59 entradas) y jtbl_009D71E0 (Boss2, 30) contra las operaciones implementadas (si faltan los BIN lo avisa y omite).

## Boss3 (boss3_seq.h/.cpp)

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| ExecSeqCmd | 0x9C8050 | reescrita+test | 59 entradas de la tabla; 53 implementadas; ver "Sin verificar" |
| ExecCmd | 0x9C6B50 | reescrita+test | el original no tiene limite de lectura; se anade una guarda (`cmdEnd`) |
| actExecSeqCmd | 0x9C91F0 | reescrita+test | |
| actNop / actDead | 0x9C7FA0 / 0x9C9210 | reescrita+test | constantes de retorno |
| actDelete | 0x9C7FB0 | reescrita+test | el campo +0x18 de la tarea base no se modela |
| FinishTask0 | 0x9C9220 | reescrita+test | |
| FinishTask | 0x9C92E0 | reescrita+test | func_1A2A10 + func_950F80 delegados en `taskEnd()` |
| AllowCameraStart / ForbidCameraStart | 0x9C9310 / 0x9C9320 | reescrita+test | byte 0x500F07 |
| EnableCollision / DisableCollision | 0x9C9330 / 0x9C9390 | reescrita+test | |
| SetHeight | 0x9C93F0 | reescrita+test | func_951300 inyectado |
| GetPathEndFrame | 0x9C9800 | reescrita+test | func_1CCCB0 inyectado |
| FireAlphaControl | 0x9C9CF0 | reescrita+test | leida contra el asm (constantes 0.02/0.05/1.8/0.5/0.8/0.15/0.2/0.01) |
| FireAlphaDirectSet / FireAlphaSupSet | 0x9C9EB0 / 0x9C9EC0 | reescrita+test | |
| LoopSoundOff | 0x9CA020 | reescrita+test | solo la parte de estado; el evento de sonido sale por `loopSoundOffEvent()` |

Operaciones de ExecSeqCmd no implementadas: 6, 49, 50, 51, 53, 55 y >= 59 saltan a 0x9C9130 sin avanzar ni ceder (bucle infinito en el original); aqui devuelven `RUN_HUNG`.

## Boss2 (boss2_seq.h/.cpp)

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| ExecSeqCmd | 0x9C98C0 | reescrita+test | 30 operaciones; la tabla de longitudes sale del BIN |
| ExecCmd | 0x9C7890 | reescrita+test | InitBunshin (pendiente) delegado en el host; guarda de limite propia |
| actExecSeqCmd | 0x9CA090 | reescrita+test | |
| actTest | 0x9C98B0 | reescrita+test | devuelve 0 |
| actDelete | 0x9C9410 | reescrita+test | |
| actDead | 0x9C97A0 | reescrita+test | |
| ControlPos | 0x9C7550 | reescrita+test | TurnPos (0x9CB5D0 = una llamada a func_951200) delegado en `turnToward()` |
| AttackPosSet | 0x9CB3E0 | reescrita+test | GetAngleYX y la pila de matrices inyectados; salida de pitch ignorada |
| SetAttackDist | 0x9CADC0 | reescrita+test | func_1AFA7C interpretada como "v * len/|v|" (leida en asm/lib_1A43D0.s); el test la modela asi |
| FinishTask0 | 0x9CA0B0 | reescrita+test | |
| FinishTask | 0x9CA220 | reescrita+test | |
| AllowCameraStart / ForbidCameraStart | 0x9CA240 / 0x9CA250 | reescrita+test | byte 0x500E17 |
| CharInit | 0x9C7930 | reescrita+test | configuracion por forma (D_009D5D20) + despacho; las rutas de BranchInit/SeqCmdInit probadas solo con configuracion sintetica; los objetos (SwordFire, WarpEff, func_939C80/9499C0) son llamadas al host |

## Boss3Bomb (boss3_bomb.h/.cpp)

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| ResetAcc | 0x9CCBC0 | reescrita+test | |
| AdjustLocalPos | 0x9CC110 | reescrita+test | |
| SetTargetLocal | 0x9CBF80 | reescrita+test | rama no simetrica deducida de mula.s/msub.s: x = 4(r-0.5) - 2(1-2*kindA); sin ejecucion real |
| CtrlPre_Float | 0x9CCAE0 | reescrita+test | SetTargetLocalFloat delegado |
| CtrlPre_Fall / CtrlPre_Bound / CtrlPre_Fly | 0x9CC970 / 0x9CC990 / 0x9CCB80 | reescrita+test | solo la secuencia de llamadas |
| CtrlPre_Appear / CtrlPre_Explosion | 0x9CCAD0 / 0x9CCBB0 | reescrita+test | vacias en el asm |

Total: Boss3 18, Boss2 14, Bomb 9 = 41 funciones C++ (todas con test directo), contadas de las tablas anteriores.

## Sin verificar / inferido
- La semantica de las llamadas inyectadas (matriz, camara, sonido, spawn) es interpretacion mia de los argumentos del asm; los mocks solo registran las llamadas. En particular: func_1B0834/1B0808/1B027C/1B03EC/1B055C/1B06CC/1AFC00 = push/translate/rotX/rotY/rotZ/scale/transformPoint (por patron de uso; no se ha leido su cuerpo).
- Convenio de angulos y signo de las rotaciones (ops 16, 18, 19 de Boss3; AttackPosSet de Boss2): no modelado, queda en el motor.
- Boss3 op 18/19: la entrada de `transformPoint` es (-off) en la 18 y (+off) en la 19; la 18 solo escribe x y z. Las ops 18 y 19 siguen a 20 y 21 respectivamente (comparten el cierre de banderas de *(this+8)+0x1C9C), segun el asm.
- Boss3 op 26: el valor -1 con gameState 0xA y gameKind 2 NO toca el byte 0x500E12; el significado de ese byte no esta identificado.
- Boss3 op 52: la llamada a rand() se descarta en el original (resultado no usado); se conserva por si tiene efecto lateral en el motor.
- Boss3 op 58: el orden de argumentos de func_217C30 (id repetido, a3 = (arg != 1), vol 0x40) esta leido del asm pero no validado.
- Boss2 op 16: el byte +0x2104 se lee como byte aunque `Bs2State::diffTimer` es s16 (se usa su byte bajo).
- CharInit Boss2: los punteros de D_009D5D20 y los de BranchInit/SeqCmdInit apuntan a datos del overlay; el test solo lee los campos escalares (flags, umbrales, hitReload, deadStage) y comprueba el ida y vuelta.
- Boss2 CharInit form 0/1: las dos llamadas a new_SwordFire pasan -1 como segundo argumento segun el asm.

## Pendiente (no reescrito)
- Boss2 (bs2mod): CtrlPre 0x9C5461.. / CtrlPut / CtrlPost del Boss2Task y de TBoss2Eff/TZanzoEffect/TSwordFire/TSonicWave*/TKangeiPillar/TFlyingPillar*, CtrlPre_battle1/3/4, TurnPos (0x9CB5D0, trivial: func_951200), InitBunshin (0x9C96C0), StartWarpIn (0x9C6C20), StartWeakDisp, MotSupSet (primera variante), eleccion del movimiento de entrada del teletransporte, ControlSound ya en bosses_a.
- Boss3 (bs3mod): CtrlPre/CtrlPut/CtrlPost de Boss3Task (0x9C5xxx), CharInit (0x9C6BA0..), EntryCameraTarget, EntryCollision, UpdateMotionFrame, ControlTransform, UpdatePath, AddPathFrame, TracePathParam, GetPathParam*, HitMarkEff*, SetBossLight, blur (init/setStart/On/Off/end), EffectCtrl.
- Boss3Bomb: SetTarget2DForce, SetAngle3DForce, SetTargetLocalFloat, SetTargetWorld, CtrlPre_Target2D/Target3D/Ground/Bound0/Linear/ModeChg, CtrlPre (dispatcher), CtrlPut, CtrlPost, new_Boss3Bomb. TBoss3BombEye completo.
- Boss3Laser (TBoss3Laser, TBoss3LaserLocus): todo (Prep/Run/Slash/Vanish, CalcBeam*, DrawBeam, MallocBeamLocus).
- Boss3XWingTask, TBS3XWingLight, Boss3SubchrTask: todo.
- Boss4 (bs4mod, game/bosses_b): CtrlPre/CtrlPut/CtrlPost y variantes, ControlPos, BnStart*, StartAttack{Iwa,Kiri*}, ControlSound, FinishTask*, etc. NO tocados en esta tanda.

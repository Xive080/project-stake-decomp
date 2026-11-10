# Manifiesto de game/bosses_b: jefe 5 (TBoss5, overlay bs5mod)

Estados: reescrita+test / reescrita sin test / pendiente. Direcciones = primera instruccion del simbolo en el overlay (base 0x9C4800, asm/bs5mod/bs5mod_text.s).

Una fila "reescrita+test" significa que alguna asercion de game/test/boss5_test.cpp ejecuta la funcion. Los tests comprueban coherencia con el asm leido a mano y con los datos extraidos de los binarios; NO se ha comparado contra ejecucion real del juego ni contra el binario compilado. Ficheros: boss5.h, boss5.cpp, boss5_data.cpp (generado por build/verify_boss5/gen5.py), game/test/boss5_test.cpp.

Compilar y ejecutar (desde la raiz del repo):

    c++ -std=c++17 -Wall game/test/boss5_test.cpp game/bosses_b/boss5.cpp game/bosses_b/boss5_data.cpp -o build/verify_boss5/boss5_test && build/verify_boss5/boss5_test

Tipos de combatiente (+0x1E48): 0 = battle1, 1 = battle2 (nombres de CtrlPre_battle1/2). `m2c` tuvo fallos en ExecSeqCmd/ControlAttack/SetPath (tablas de salto); ExecSeqCmd y ControlAttack se han leido directamente del asm y los jtbl de asm/bs5mod/data.

## Funciones

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| SeqCmdInit | 0x9CA3F0 | reescrita+test |  |
| SeqCmdNextTable | 0x9CA430 | reescrita+test | el original no comprueba el final de la tabla; aqui se usa 0xFFFF como fin |
| SeqCmdSetTable | 0x9CA460 | reescrita+test |  |
| SeqCmdPush | 0x9CA4A0 | reescrita+test | a traves de SeqCmdCall |
| SeqCmdPop | 0x9CA4E0 | reescrita+test |  |
| SeqCmdCall | 0x9CA510 | reescrita+test |  |
| SeqCmdCancelWait | 0x9CA560 | reescrita+test | salta ops 5,6,7 |
| ExecSeqCmd | 0x9C97D0 | reescrita+test | 47 ops transcritos del asm (jtbl_009DBD90). Ops 25-28, 33, 35 (calculo 3D), 38 (InitWaitPath/InitSippoPos), 41-43 y 45 con c1=-1 delegan en env.hook: la parte escalar esta probada, lo que hace el motor no. Op 22 es no-op con longitud 0 (colgaria el original; no aparece en ningun script). El orden exacto de la rotacion de op 35 (func_1B0834/1B0808/1B06CC/1AFC00) no esta reescrito |
| actExecSeqCmd | 0x9CA5C0 | reescrita+test |  |
| InitBranchParam | 0x9CA5E0 | reescrita+test |  |
| MotSupSet2 | 0x9CAAD0 | reescrita+test | playMotion(motion, inicio, mezcla) es func_1FA640 |
| CheckSwimMotion | 0x9CAA70 | reescrita+test |  |
| InitDifficulty | 0x9C7BE0 | reescrita+test | el valor inicial es func_96CEC0() (inyectado) |
| SetDiffParam | 0x9C7B20 | reescrita+test | nivel = func_943AC0() inyectado; fila [sel 1F75] con 5 selectores (extension inferida del hueco hasta D_009CF730) |
| UpDifficulty | 0x9C7C40 | reescrita+test |  |
| DownDifficulty | 0x9C7C70 | reescrita+test |  |
| DownDifficultyReq | 0x9C7C10 | reescrita+test | solo llama a DownDifficulty |
| NoChgDifficultyReq | 0x9C7C30 | reescrita+test |  |
| GetStrength | 0x9C6710 | reescrita+test | func_1A7F10 se asume aleatorio float [0,1); no verificado; el tercer float de la fila no se usa; nadie en este overlay la llama (se usara desde fuera o desde SetPath) |
| InitLife | 0x9C7860 | reescrita+test |  |
| ControlDispLife | 0x9C7900 | reescrita+test | solo la parte numerica; dibujo (func_948FB0) omitido |
| CheckWeakHit | 0x9C8C60 | reescrita+test |  |
| ClearWeakHit | 0x9C8CA0 | reescrita+test | solo el hook (recorre las 4 partes del objeto); se comprueba que CheckDamage lo invoca |
| CheckDamage | 0x9C8CE0 | reescrita+test | HitInfo (slot/partType/weakIdx) inferido de gp-0x7350/-0x7348 y +0x70, igual que en boss4 |
| WeakHit | 0x9C8E20 | reescrita+test | HitMarkEff/HitMarkEff2 solo como hook |
| NoWeakHit | 0x9C8FE0 | reescrita+test | vacia (jr ra) |
| DamageUpperBody | 0x9C8FF0 | reescrita+test | efecto inchiki_dammot_set + func_1FAA60 como hook (sus parametros no estan modelados) |
| Damage | 0x9C9340 | reescrita+test |  |
| SetDamageMotion | 0x9C9490 | reescrita+test |  |
| Dead | 0x9C9520 | reescrita+test | *(owner+0x1C9C) &= ~3 como hook |
| actDelete | 0x9C9650 | reescrita+test |  |
| actDead | 0x9C96E0 | reescrita+test |  |
| CheckDamageEnd (parcial) | 0x9C7250 | reescrita+test | solo el bloque inicial (cuenta atras del aturdimiento 1F5C). El resto (retroceso del cuerpo con matrices, switch por 1CC8 y func_214FF0) PENDIENTE |
| SelectAttackTarget | 0x9CAB30 | reescrita+test |  |
| StartAttack | 0x9C64C0 | reescrita+test |  |
| ControlAttack | 0x9C61E0 | reescrita+test | 7 hitos de jtbl_009DBD50 |
| AttackDamageFrame | 0x9C6180 | reescrita+test | new_Boss5Sphere / new_Boss5XIce como hooks |
| DamageToPlayer | 0x9CAC70 | reescrita+test |  |
| DecNowEnemyAlvNumChk | 0x9CAC10 | reescrita+test |  |
| DecNowEnemyNumChk | 0x9CAC40 | reescrita+test |  |
| ControlAlpha | 0x9C6670 | reescrita+test |  |
| EnableCollision | 0x9CA7E0 | reescrita+test | bits de las partes como hook |
| DisableCollision | 0x9CA840 | reescrita+test |  |
| AllowCameraStart | 0x9CA7C0 | reescrita+test | a traves de ops 4 y actDead |
| ForbidCameraStart | 0x9CA7D0 | reescrita+test | a traves del op 4 |
| SetHeight | 0x9CA8A0 | reescrita+test | func_951300 inyectado (groundY) |
| FallCheck | 0x9CA960 | reescrita+test | gravedad 0xBD20902E leida del asm |
| CharInit | 0x9C79D0 | reescrita+test | vida del tipo 1 = func_14C518(2500.0 * D_985830[func_96CED0(1)]) inyectada (D_985830 es del ELF, no leida) |
| ControlWaterPillar | 0x9CC030 | reescrita+test | el movimiento de cada pilar (Start__17TBoss5WaterPillar) es hook; el significado de 1E62/1E64 (fase/subida) es inferido |
| ETurn | 0x9CAD50 | reescrita+test | asm comparado a mano con la version de boss4 (equivalente) |
| AngChk | 0x9CAE00 | reescrita+test |  |
| CheckAttackEnd (TBoss5Ice) | 0x9CD600 | reescrita+test | distancia = func_944FF0 inyectada |
| Attack_sphere (TBoss5Ice) | 0x9CD6D0 | reescrita+test |  |
| Attack_tagger (TBoss5Ice) | 0x9CD860 | reescrita+test | codigo identico a Attack_sphere segun m2c (no releido en asm) |
| Attack_chaku (TBoss5Ice) | 0x9CD7E0 | reescrita+test |  |
| UpShape (TBoss5Ice) | 0x9CD8B0 | reescrita+test |  |
| Morph (TBoss5Ice) | 0x9CD8F0 | reescrita+test | campos +0x1C0..+0x1CC leidos del asm; consumidor desconocido |

## Pendiente (no reescrito)

| Grupo | Direccion (primera) | Notas |
|---|---|---|
| CtrlPre | 0x9C5010 | pendiente | orden por fotograma: guarda pos/rot, copia pos/rot a 1EF4.., quita el bit 0x40 de 1E40, llama a CtrlPre_battle1/2, luego barra de vida (1E40&0x200) |
| CtrlPre_battle1 | 0x9C5560 | pendiente | orden: func_1C6C80 (despacho por estado 1F24), FallCheck, CheckDamageEnd, CheckDamage, ControlAttack, ControlPos, ControlAlpha, ControlByMotion; orquestacion no escrita |
| CtrlPre_battle2 | 0x9C5690 | pendiente | orden: despacho, FallCheck, CheckDamageEnd, CheckDamage, ControlAttack, ControlWaitPath, ControlPos, ControlWaterPillar, ControlByMotionX; orquestacion no escrita |
| CtrlPut / CtrlPost / EntryCameraTarget / EntryCollision | 0x9C5240 | pendiente | dibujo/colisiones del motor |
| ControlPos / ControlPath / InitPath / InitWaitPath / ControlWaitPath / GetWaitPos / GetCamRot / GetCamPos / SetPath | 0x9C6560 | pendiente | movimiento por rutas: matematica 3D con func_1B....; SetPath y ControlPath tienen jump table |
| ControlByMotion / ControlByMotionX / UpdateMotionFrame | 0x9C59B0 | pendiente | avance del fotograma y cambios por movimiento |
| SetBodyMove / ClearBodyMove / SetAlpha / BodyInit / DispParts* / prtsDispFunc / oskDispFunc / oskAttachFunc / colliFunc / LookAtTarget / Sippo* / DispSitaMizu / InitSitaMizu / DispBodyAttachedParts | 0x9CAE90 | pendiente | cuerpo articulado, dibujo y colisiones |
| HitMarkEff / HitMarkEff2 / inchiki_dammot_set / StartWeakDisp / DispLife | 0x9C9250 | pendiente | efectos y dibujo |
| FinishTask0 / FinishTask | 0x9CA5F0 | pendiente | limpieza de objetos del motor (leida, no reescrita) |
| CheckDamageEnd (resto) | 0x9C7250 | pendiente | retroceso del cuerpo |
| TBoss5WaterPillar (ct, CtrlPre/Put/Post, Start), InitWaterPillar | 0x9CBB90 | pendiente | movimiento de pilares |
| TBoss5Ice: ct/dt/PrePut/Put/PostPut/StartDestroy/StartTurn/StartMorph/StartAttack/Attack_spear | 0x9CD720 | pendiente | StartAttack contiene la balistica (func_14B2E0...); Attack_spear: m2c con constantes dudosas, no verificada |
| new_Boss5Sphere / new_Boss5XIce / TBoss5XIceShape::loop | 0x9CD980 | pendiente | proyectiles/efectos |
| __ct__6TBoss5 / new_Boss5 / __dt__* | 0x9C4F20 | pendiente | construccion/destruccion |

## Datos extraidos (boss5_data.cpp) y comprobados en el test contra BS5MOD.BIN / ELF

kCmdLen (D_009CF8D0, 47 bytes), kScript (0x9CEF60..0x9CF3C0 + 4 scripts centinela del ELF 0x2D87C8/D8/E0/E8), kSeqTab0 (ELF 0x2D87D0, 2 entradas), kSeqTab1 (D_009CF3C0, 17 entradas), kCallScript (gp-0x7930), kKindConfig (gp-0x7900 -> 0x9CF410/0x9CF430), kAttackTable (D_009DBD00, 4 x 20 B), kStrength (D_009CF4E0), kPartLife (D_009CF680), kDmgMul (D_009CF730), kDiffStep (ELF 0x2D87F8), kPillarSpeed (D_009CF450), kPose (D_009CE850). gp = 0x2E00F0 (como en boss4).

## No verificado / inferido

- Nombres de campos por uso; el significado exacto de 1E58 (phaseCount), 1F48, 1F5E, 1D74 y de los bits de 1E40 distintos de los citados es inferido.
- Extension de kPartLife a 5 selectores (170 bytes) inferida del hueco entre D_009CF680 y D_009CF730; solo se sabe que op 46 fija el selector.
- Los scripts se recorren en el test solo para comprobar que todos los codigos son <47 y terminan; no se ha simulado un combate completo.
- Funciones del motor (func_*) quedan tras `Boss5Env`/`hook`; su comportamiento no se reproduce.

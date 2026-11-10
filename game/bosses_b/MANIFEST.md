# Manifiesto de game/bosses_b

Estados: reescrita+test / reescrita sin test / pendiente. Direcciones = primera instruccion del simbolo en el overlay (base 0x9C4800).

Una fila "reescrita+test" significa que alguna asercion de game/test/bosses_b_test.cpp ejecuta la funcion (directa o indirectamente, anotado). Los tests comprueban coherencia con el asm leido a mano y los datos extraidos de los binarios; no se ha comparado contra ejecucion real del juego.

## bs4mod (TBoss4)

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| SeqCmdInit | 0x9CCBF0 | reescrita+test |  |
| SeqCmdNextTable | 0x9CCC30 | reescrita+test |  |
| SeqCmdSetTable | 0x9CCC60 | reescrita+test |  |
| SeqCmdPush | 0x9CCCA0 | reescrita+test | cubierto solo indirectamente por scripts de prueba (no aserciones propias) |
| SeqCmdPop | 0x9CCCE0 | reescrita+test |  |
| SeqCmdCall | 0x9CCD10 | reescrita+test |  |
| SeqCmdCancelWait | 0x9CCD60 | reescrita+test |  |
| ExecSeqCmd | 0x9CBCA0 | reescrita+test | 67 ops transcritos; test cubre ops 7,9,37,50 y el primer script del tipo 0; op 62 sin test; significado de globales 0x500DFC y campos de camara no verificado |
| MotSupSet2 | 0x9CD330 | reescrita sin test | solo la variante Set2/Set3; MotSupSet (primera variante) pendiente |
| MotSupSet3 | 0x9CD410 | reescrita sin test |  |
| InitDifficulty | 0x9CA300 | reescrita sin test |  |
| SetDiffParam | 0x9CA140 | reescrita+test |  |
| UpDifficulty | 0x9CA360 | reescrita+test |  |
| DownDifficulty | 0x9CA390 | reescrita+test |  |
| NoChgDifficultyReq | 0x9CA350 | reescrita sin test |  |
| InitLife | 0x9C9CB0 | reescrita+test |  |
| ControlDispLife | 0x9C9D50 | reescrita+test | solo la parte numerica; dibujo omitido |
| CheckWeakHit | 0x9CADE0 | reescrita sin test | sin test directo |
| CheckDamage | 0x9CAE60 | reescrita sin test | sin test; depende de HitInfo inferido |
| WeakHit | 0x9CAFA0 | reescrita+test |  |
| NoWeakHit | 0x9CB180 | reescrita sin test |  |
| DamageUpperBody | 0x9CB220 | reescrita sin test |  |
| Damage | 0x9CB600 | reescrita+test | cubierto indirectamente via WeakHit (tipos 0 y 2) |
| Dead | 0x9CB8C0 | reescrita+test | cubierto indirectamente via WeakHit (tipo 2) |
| SetDamageMotion | 0x9CB7C0 | reescrita sin test |  |
| SetDeadMotion | 0x9CB9D0 | reescrita sin test |  |
| actDelete | 0x9CBA90 | reescrita sin test |  |
| actDead | 0x9CBBA0 | reescrita sin test |  |
| actBunshin | 0x9CBB20 | reescrita sin test |  |
| actShoukan | 0x9CBB50 | reescrita sin test |  |
| SelectAttackTarget | 0x9CD500 | reescrita+test |  |
| DamageToPlayer | 0x9CD640 | reescrita+test |  |
| AttackDamageFrame | 0x9C7980 | reescrita sin test |  |
| StartAttack | 0x9C7DC0 | reescrita+test | solo el ataque 0 probado; ids 15..21 sin test |
| ControlAttack | 0x9C7A10 | reescrita+test | probado solo con ataque 0 (dano en f90, fin en 140) |
| SetRunKindRand | 0x9CE080 | reescrita sin test |  |
| DecNowEnemyAlvNumChk | 0x9CD5E0 | reescrita sin test |  |
| DecNowEnemyNumChk | 0x9CD610 | reescrita sin test |  |
| ControlAlpha | 0x9C9840 | reescrita sin test |  |
| StartAlphaAdd | 0x9C98E0 | reescrita sin test |  |
| StartAlphaSub | 0x9C9900 | reescrita sin test |  |
| StartBunshin | 0x9C9E20 | reescrita sin test |  |
| StartShusoku | 0x9C9E90 | reescrita sin test |  |
| CancelBunshin | 0x9C9EF0 | reescrita sin test |  |
| EnableCollision | 0x9CCFA0 | reescrita sin test |  |
| DisableCollision | 0x9CD000 | reescrita sin test |  |
| FallCheck | 0x9CD120 | reescrita+test | groundY inyectado (func_951300) |
| CharInit | 0x9C9F40 | reescrita sin test | sin test; usa kKindConfig generado |
| ETurn | 0x9CD700 | reescrita+test |  |
| AngChk | 0x9CD7B0 | reescrita+test |  |
|  | 0x9C4880 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| new_Boss4 | 0x9C5330 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre | 0x9C5450 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPut | 0x9C5740 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPost | 0x9C5B90 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_battle1 | 0x9C5BE0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_battle2 | 0x9C5D60 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_battle3 | 0x9C5EE0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_shoukan | 0x9C6050 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_bunshin | 0x9C68D0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| BnStartBunshin | 0x9C6A60 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| BnStartShusoku | 0x9C6B10 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| BnStartStandby | 0x9C6B90 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| BnCheckCmd | 0x9C6BE0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_bunshin_battle1 | 0x9C6C60 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CtrlPre_bunshin_battle2 | 0x9C6E90 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| EntryCameraTarget | 0x9C70A0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| EntryCollision | 0x9C7290 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| UpdateMotionFrame | 0x9C7330 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlSound | 0x9C7370 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlAttack_battle3 | 0x9C7CF0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| StartAttackIwa | 0x9C7FD0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| StartAttackKiriPre | 0x9C8330 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| StartAttackKiri | 0x9C8490 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| StartAttackKiri_shoukan | 0x9C8660 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlPos | 0x9C8770 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| SetFcv | 0x9C8FE0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitFcv | 0x9C9290 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| SetCamPathAttr | 0x9C92E0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| GetShoukanPos | 0x9C94B0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlCamera | 0x9C95D0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlBunshin_battle1 | 0x9C9660 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlBunshin_battle2 | 0x9C97E0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlEffect | 0x9C9920 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| CheckDamageEnd | 0x9C9930 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispLife | 0x9C9D00 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| StartWeakDisp | 0x9C9DE0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DownDifficultyReq | 0x9CA330 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| BodyInit | 0x9CA3C0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| oskDispFunc | 0x9CA450 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| oskAttachFunc | 0x9CA5F0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| prtsDispFunc | 0x9CA630 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispParts | 0x9CA6F0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispPartsX | 0x9CA8B0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispPartsXX | 0x9CA9C0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| LookAtTarget | 0x9CAAF0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| colliFunc | 0x9CADA0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ClearWeakHit | 0x9CAE20 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| HitMarkEff2 | 0x9CB3E0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| HitMarkEff | 0x9CB510 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| actNop | 0x9CBA80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| actExecSeqCmd | 0x9CCDC0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| FinishTask0 | 0x9CCDE0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| FinishTask | 0x9CCF60 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| AllowCameraStart | 0x9CCF80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ForbidCameraStart | 0x9CCF90 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| SetHeight | 0x9CD060 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| MotSupSet | 0x9CD230 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| TurnPos | 0x9CD6B0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| SetBodyMove | 0x9CD840 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ClearBodyMove | 0x9CDA70 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| SetBossLight | 0x9CDCD0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| SetAlpha | 0x9CDF30 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitGareki | 0x9CE0F0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlGareki | 0x9CE120 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PutGareki | 0x9CE2F0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| RaiseGareki | 0x9CE430 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| RaiseGareki2 | 0x9CE460 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| RaiseGareki3 | 0x9CE490 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| FallGareki | 0x9CE4C0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitBodyAttachedParts | 0x9CE4E0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispBodyAttachedParts0 | 0x9CE560 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispBodyAttachedParts1 | 0x9CE7A0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitJumonMahoujin | 0x9CE950 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispJumonMahoujin | 0x9CE9F0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitBackEff | 0x9CEBC0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispBackEff | 0x9CEBD0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispMaskModel | 0x9CECB0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispMaskModel_hand | 0x9CED30 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitMaskModel | 0x9CEF40 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlMaskModel | 0x9CF030 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| ControlMaskModel_rockhole | 0x9CF050 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispMaskModel_shoukan | 0x9CF330 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispMaskModel_rockhole | 0x9CF640 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| opmp_genzan | 0x9CF760 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| InitKiriKabe | 0x9CF770 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PutKiriKabe | 0x9CF840 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PutKabeMoyou | 0x9CFA80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| Egure | 0x9CFB30 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| GetHandMot | 0x9CFDB0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PutHand | 0x9CFF80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D0050 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@48@oskAttachFunc | 0x9D0100 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@48@oskDispFunc | 0x9D0110 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@48@colliFunc | 0x9D0120 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@48@prtsDispFunc | 0x9D0130 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@12132@64@ | 0x9D0140 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| opmp_strip_no_alphablend | 0x9D0150 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D0180 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PrePut | 0x9D0870 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| Put | 0x9D08B0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PostPut | 0x9D0D80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| new_Boss4Shoukan | 0x9D0F00 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D0FF0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| loop | 0x9D1050 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| new_St4YamiKomono | 0x9D1100 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| DispSt4YamiGareki | 0x9D11F0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D12C0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PrePut | 0x9D1510 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| Put | 0x9D19A0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PostPut | 0x9D1C60 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D1D30 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PrePut | 0x9D1E50 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| Put | 0x9D1E80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| new_Boss4Egure | 0x9D1F30 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D2020 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| loop | 0x9D2080 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| new_Boss4KiriKabe | 0x9D20C0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D21B0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D2280 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PrePut | 0x9D25C0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| Put | 0x9D2610 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| PostPut | 0x9D2700 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| new_Boss4HensinEff | 0x9D2710 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D2800 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D2850 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D28E0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D2980 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
|  | 0x9D29D0 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@29768@64@ | 0x9D2A60 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@452@64@ | 0x9D2A70 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@36@64@ | 0x9D2A80 | pendiente | no reescrita (dibujo, efectos o sin analizar) |
| "@17540@64@ | 0x9D2A90 | pendiente | no reescrita (dibujo, efectos o sin analizar) |

## bs5mod (TBoss5)

No empezado: ninguna funcion reescrita. Solo se localizo el ExecSeqCmd de Boss5 (0x9C97D0, 0x2F opcodes, jtbl_009DBD90; config en ELF gp-0x7900).

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| inchiki_dammot_set | 0x9C4880 | pendiente | |
|  | 0x9C4900 | pendiente | |
| new_Boss5 | 0x9C4F20 | pendiente | |
| CtrlPre | 0x9C5010 | pendiente | |
| CtrlPut | 0x9C5240 | pendiente | |
| CtrlPost | 0x9C5500 | pendiente | |
| CtrlPre_battle1 | 0x9C5560 | pendiente | |
| CtrlPre_battle2 | 0x9C5690 | pendiente | |
| EntryCameraTarget | 0x9C57C0 | pendiente | |
| EntryCollision | 0x9C58F0 | pendiente | |
| UpdateMotionFrame | 0x9C5970 | pendiente | |
| ControlByMotion | 0x9C59B0 | pendiente | |
| ControlByMotionX | 0x9C5A70 | pendiente | |
| GetPosRotY | 0x9C60C0 | pendiente | |
| AttackDamageFrame | 0x9C6180 | pendiente | |
| ControlAttack | 0x9C61E0 | pendiente | |
| StartAttack | 0x9C64C0 | pendiente | |
| ControlPos | 0x9C6560 | pendiente | |
| ControlAlpha | 0x9C6670 | pendiente | |
| GetStrength | 0x9C6710 | pendiente | |
| SetPath | 0x9C67C0 | pendiente | |
| InitPath | 0x9C6B30 | pendiente | |
| ControlPath | 0x9C6B80 | pendiente | |
| InitWaitPath | 0x9C6E60 | pendiente | |
| ControlWaitPath | 0x9C6EE0 | pendiente | |
| GetWaitPos | 0x9C6F90 | pendiente | |
| GetCamRot | 0x9C7160 | pendiente | |
| GetCamPos | 0x9C71C0 | pendiente | |
| ControlEffect | 0x9C7240 | pendiente | |
| CheckDamageEnd | 0x9C7250 | pendiente | |
| InitLife | 0x9C7860 | pendiente | |
| DispLife | 0x9C78B0 | pendiente | |
| ControlDispLife | 0x9C7900 | pendiente | |
| StartWeakDisp | 0x9C7990 | pendiente | |
| CharInit | 0x9C79D0 | pendiente | |
| SetDiffParam | 0x9C7B20 | pendiente | |
| InitDifficulty | 0x9C7BE0 | pendiente | |
| DownDifficultyReq | 0x9C7C10 | pendiente | |
| NoChgDifficultyReq | 0x9C7C30 | pendiente | |
| UpDifficulty | 0x9C7C40 | pendiente | |
| DownDifficulty | 0x9C7C70 | pendiente | |
| BodyInit | 0x9C7CA0 | pendiente | |
| oskDispFunc | 0x9C7D70 | pendiente | |
| oskAttachFunc | 0x9C7E90 | pendiente | |
| prtsDispFunc | 0x9C7ED0 | pendiente | |
| DispParts | 0x9C7F80 | pendiente | |
| InitSippoChain | 0x9C80D0 | pendiente | |
| InitSippoPos | 0x9C81B0 | pendiente | |
| CalcSippoChain | 0x9C8300 | pendiente | |
| DispPartsX | 0x9C8560 | pendiente | |
| LookAtTarget | 0x9C86F0 | pendiente | |
| colliFunc | 0x9C89A0 | pendiente | |
| CheckWeakHit | 0x9C8C60 | pendiente | |
| ClearWeakHit | 0x9C8CA0 | pendiente | |
| CheckDamage | 0x9C8CE0 | pendiente | |
| WeakHit | 0x9C8E20 | pendiente | |
| NoWeakHit | 0x9C8FE0 | pendiente | |
| DamageUpperBody | 0x9C8FF0 | pendiente | |
| HitMarkEff2 | 0x9C9120 | pendiente | |
| HitMarkEff | 0x9C9250 | pendiente | |
| Damage | 0x9C9340 | pendiente | |
| SetDamageMotion | 0x9C9490 | pendiente | |
| Dead | 0x9C9520 | pendiente | |
| actNop | 0x9C9640 | pendiente | |
| actDelete | 0x9C9650 | pendiente | |
| actDead | 0x9C96E0 | pendiente | |
| ExecSeqCmd | 0x9C97D0 | pendiente | |
| SeqCmdInit | 0x9CA3F0 | pendiente | |
| SeqCmdNextTable | 0x9CA430 | pendiente | |
| SeqCmdSetTable | 0x9CA460 | pendiente | |
| SeqCmdPush | 0x9CA4A0 | pendiente | |
| SeqCmdPop | 0x9CA4E0 | pendiente | |
| SeqCmdCall | 0x9CA510 | pendiente | |
| SeqCmdCancelWait | 0x9CA560 | pendiente | |
| actExecSeqCmd | 0x9CA5C0 | pendiente | |
| InitBranchParam | 0x9CA5E0 | pendiente | |
| FinishTask0 | 0x9CA5F0 | pendiente | |
| FinishTask | 0x9CA7A0 | pendiente | |
| AllowCameraStart | 0x9CA7C0 | pendiente | |
| ForbidCameraStart | 0x9CA7D0 | pendiente | |
| EnableCollision | 0x9CA7E0 | pendiente | |
| DisableCollision | 0x9CA840 | pendiente | |
| SetHeight | 0x9CA8A0 | pendiente | |
| FallCheck | 0x9CA960 | pendiente | |
| CheckSwimMotion | 0x9CAA70 | pendiente | |
| MotSupSet2 | 0x9CAAD0 | pendiente | |
| SelectAttackTarget | 0x9CAB30 | pendiente | |
| DecNowEnemyAlvNumChk | 0x9CAC10 | pendiente | |
| DecNowEnemyNumChk | 0x9CAC40 | pendiente | |
| DamageToPlayer | 0x9CAC70 | pendiente | |
| TurnPos | 0x9CAD00 | pendiente | |
| ETurn | 0x9CAD50 | pendiente | |
| AngChk | 0x9CAE00 | pendiente | |
| SetBodyMove | 0x9CAE90 | pendiente | |
| ClearBodyMove | 0x9CB0C0 | pendiente | |
| SetAlpha | 0x9CB320 | pendiente | |
| DispBodyAttachedParts | 0x9CB4F0 | pendiente | |
| InitBodyAttachedParts | 0x9CB5A0 | pendiente | |
| InitSitaMizu | 0x9CB5E0 | pendiente | |
| DispSitaMizu | 0x9CB680 | pendiente | |
|  | 0x9CB880 | pendiente | |
|  | 0x9CBB00 | pendiente | |
| CtrlPre | 0x9CBB90 | pendiente | |
| CtrlPut | 0x9CBDA0 | pendiente | |
| CtrlPost | 0x9CBF10 | pendiente | |
| Start | 0x9CBF20 | pendiente | |
| InitWaterPillar | 0x9CBF30 | pendiente | |
| ControlWaterPillar | 0x9CC030 | pendiente | |
|  | 0x9CC230 | pendiente | |
| "@48@oskAttachFunc | 0x9CC2E0 | pendiente | |
| "@48@oskDispFunc | 0x9CC2F0 | pendiente | |
| "@48@colliFunc | 0x9CC300 | pendiente | |
| "@48@prtsDispFunc | 0x9CC310 | pendiente | |
| "@18768@64@ | 0x9CC320 | pendiente | |
| "@580@64@ | 0x9CC330 | pendiente | |
|  | 0x9CC340 | pendiente | |
|  | 0x9CC6F0 | pendiente | |
|  | 0x9CC7E0 | pendiente | |
|  | 0x9CC880 | pendiente | |
| PrePut | 0x9CCA10 | pendiente | |
| Put | 0x9CCD60 | pendiente | |
| PostPut | 0x9CD180 | pendiente | |
| StartDestroy | 0x9CD220 | pendiente | |
| StartTurn | 0x9CD2B0 | pendiente | |
| StartMorph | 0x9CD350 | pendiente | |
| StartAttack | 0x9CD3B0 | pendiente | |
| CheckAttackEnd | 0x9CD600 | pendiente | |
| Attack_sphere | 0x9CD6D0 | pendiente | |
| Attack_spear | 0x9CD720 | pendiente | |
| Attack_chaku | 0x9CD7E0 | pendiente | |
| Attack_tagger | 0x9CD860 | pendiente | |
| UpShape | 0x9CD8B0 | pendiente | |
| Morph | 0x9CD8F0 | pendiente | |
| new_Boss5Sphere | 0x9CD980 | pendiente | |
|  | 0x9CDD70 | pendiente | |
| loop | 0x9CDE80 | pendiente | |
| new_Boss5XIce | 0x9CE1F0 | pendiente | |
|  | 0x9CE700 | pendiente | |
| "@128@160@putEntry | 0x9CE790 | pendiente | |
| "@128@160@put | 0x9CE7A0 | pendiente | |
| "@672@64@ | 0x9CE7B0 | pendiente | |

## bs6mod (Boss6Task)

No empezado: ninguna funcion reescrita. Solo se localizo el ExecSeqCmd de Boss5 (0x9C97D0, 0x2F opcodes, jtbl_009DBD90; config en ELF gp-0x7900).

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
|  | 0x9C4880 | pendiente | |
| new_Boss6 | 0x9C4DD0 | pendiente | |
| InitBS6Difficulty | 0x9C4EE0 | pendiente | |
| CtrlPre | 0x9C4F30 | pendiente | |
| CtrlPut | 0x9C5110 | pendiente | |
| CtrlPost | 0x9C51D0 | pendiente | |
| EntryCameraTarget | 0x9C5240 | pendiente | |
| EntryCollision | 0x9C53A0 | pendiente | |
| AddMotionFrame | 0x9C5460 | pendiente | |
| UpdatePath | 0x9C54A0 | pendiente | |
| AddPathFrame | 0x9C5570 | pendiente | |
| B6InterDivAngF | 0x9C55F0 | pendiente | |
| SetApproach | 0x9C56A0 | pendiente | |
| ApproachANPO | 0x9C5700 | pendiente | |
| AttackDamageFrame | 0x9C58C0 | pendiente | |
| ControlAttack | 0x9C5B10 | pendiente | |
| StartAttack | 0x9C62A0 | pendiente | |
| CheckDamageEnd | 0x9C6380 | pendiente | |
| InitLife | 0x9C6740 | pendiente | |
| DispLife | 0x9C67F0 | pendiente | |
| ControlDispLife | 0x9C6850 | pendiente | |
| ExecCmd | 0x9C69B0 | pendiente | |
| CharInit | 0x9C6A00 | pendiente | |
| BodyInit | 0x9C6CC0 | pendiente | |
| oskDispFunc | 0x9C6CD0 | pendiente | |
| DispParts | 0x9C6DE0 | pendiente | |
| prtsDispFunc | 0x9C6F00 | pendiente | |
| colliFunc | 0x9C6F20 | pendiente | |
| CheckWeakHit | 0x9C6F30 | pendiente | |
| ClearWeakHit | 0x9C6F70 | pendiente | |
| CheckDamage | 0x9C6FB0 | pendiente | |
| WeakHit | 0x9C7140 | pendiente | |
| HitMarkEff0 | 0x9C7520 | pendiente | |
| HitMarkEff | 0x9C76E0 | pendiente | |
| NoWeakHit | 0x9C7750 | pendiente | |
| DamageUpperBody | 0x9C7780 | pendiente | |
| Damage | 0x9C7900 | pendiente | |
| SetDamageMotion | 0x9C79A0 | pendiente | |
| batKillAdd | 0x9C7B60 | pendiente | |
| setLightColor | 0x9C7BA0 | pendiente | |
| IncBS6Difficulty | 0x9C7D00 | pendiente | |
| DecBS6Difficulty | 0x9C7D80 | pendiente | |
| GetBS6Difficulty | 0x9C7E00 | pendiente | |
| actNop | 0x9C7E70 | pendiente | |
| actDelete | 0x9C7E80 | pendiente | |
| ExecSeqCmd | 0x9C7F10 | pendiente | |
| SeqCmdInit | 0x9C9690 | pendiente | |
| SeqCmdNextTable | 0x9C96C0 | pendiente | |
| SeqCmdSetTable | 0x9C96F0 | pendiente | |
| actExecSeqCmd | 0x9C9720 | pendiente | |
| actDead | 0x9C9740 | pendiente | |
| FinishTask0 | 0x9C9750 | pendiente | |
| FinishTask | 0x9C9840 | pendiente | |
| AllowCameraStart | 0x9C9860 | pendiente | |
| ForbidCameraStart | 0x9C9870 | pendiente | |
| EnableCollision | 0x9C9880 | pendiente | |
| DisableCollision | 0x9C98E0 | pendiente | |
| MotSupSet2 | 0x9C9940 | pendiente | |
| SelectAttackTarget | 0x9C9A70 | pendiente | |
| SetBulletNum | 0x9C9B50 | pendiente | |
| AtcPlFlgClear | 0x9C9C20 | pendiente | |
| DecNowEnemyAlvNumChk | 0x9C9C50 | pendiente | |
| DecNowEnemyNumChk | 0x9C9C80 | pendiente | |
| GetPathEndFrame | 0x9C9CB0 | pendiente | |
| TracePathParam | 0x9C9D00 | pendiente | |
| GetPathParam | 0x9C9DC0 | pendiente | |
| GetPathParam | 0x9CA100 | pendiente | |
| GetPathParamCT | 0x9CA250 | pendiente | |
| BodyAlphaControl | 0x9CA2C0 | pendiente | |
| BodyAlphaDirectSet | 0x9CA420 | pendiente | |
| BodyAlphaSupSet | 0x9CA430 | pendiente | |
| CtrlBossLight | 0x9CA440 | pendiente | |
| SetBossLight | 0x9CA480 | pendiente | |
| InitBranchRate | 0x9CAAE0 | pendiente | |
| GetRandomBranchRate | 0x9CAB60 | pendiente | |
| LoopSoundOff | 0x9CACB0 | pendiente | |
| SetCircleMove | 0x9CACF0 | pendiente | |
| CircleMove | 0x9CAEF0 | pendiente | |
| SetSpeedBaseRate | 0x9CB010 | pendiente | |
| EffectCtrl | 0x9CB100 | pendiente | |
| SetAlpha | 0x9CB170 | pendiente | |
| initBlur | 0x9CB280 | pendiente | |
| endBlur | 0x9CB2C0 | pendiente | |
|  | 0x9CB2F0 | pendiente | |
| "@48@oskDispFunc | 0x9CB3B0 | pendiente | |
| "@48@colliFunc | 0x9CB3C0 | pendiente | |
| "@48@prtsDispFunc | 0x9CB3D0 | pendiente | |
| "@8336@64@ | 0x9CB3E0 | pendiente | |
|  | 0x9CB3F0 | pendiente | |
| CtrlPre | 0x9CB700 | pendiente | |
| CtrlPut | 0x9CB890 | pendiente | |
| ChkFrmSnd | 0x9CB930 | pendiente | |
| CtrlPost | 0x9CB9A0 | pendiente | |
| oskDispFunc | 0x9CBC90 | pendiente | |
| DispParts | 0x9CBD60 | pendiente | |
| prtsDispFunc | 0x9CBE00 | pendiente | |
| EntryCollision | 0x9CBE20 | pendiente | |
| CheckDamage | 0x9CBE30 | pendiente | |
| SetAlpha | 0x9CBF40 | pendiente | |
| SetTargetID | 0x9CC070 | pendiente | |
| SetLocalTarget | 0x9CC0E0 | pendiente | |
| SetGlobalTarget | 0x9CC1E0 | pendiente | |
|  | 0x9CC290 | pendiente | |
| PlHitChk | 0x9CC680 | pendiente | |
| RotInit | 0x9CC7E0 | pendiente | |
| RotPos | 0x9CC870 | pendiente | |
| SetShotParam | 0x9CCA80 | pendiente | |
| SetPathParam | 0x9CCB90 | pendiente | |
| TraceShotPath | 0x9CCC50 | pendiente | |
| SetBatSpiral | 0x9CCD00 | pendiente | |
| AddBatSpiralCount | 0x9CCEA0 | pendiente | |
| SetSpiralParam | 0x9CCEC0 | pendiente | |
| TraceSpiralPath | 0x9CCF20 | pendiente | |
| ChkMode | 0x9CD070 | pendiente | |
| ExecMode | 0x9CD3A0 | pendiente | |
| SetApproach | 0x9CD480 | pendiente | |
| Approach | 0x9CD4A0 | pendiente | |
| SetFallParam | 0x9CD570 | pendiente | |
| FallDown | 0x9CD620 | pendiente | |
| ChgDead | 0x9CD660 | pendiente | |
| CtrlPre | 0x9CD670 | pendiente | |
| CtrlPut | 0x9CD880 | pendiente | |
| CtrlPost | 0x9CDB60 | pendiente | |
| new_Boss6Bat | 0x9CDBB0 | pendiente | |
| new_Boss6AllBat | 0x9CDC60 | pendiente | |
|  | 0x9CDD40 | pendiente | |
|  | 0x9CE020 | pendiente | |
| ClearTBoss6XBulletDamage | 0x9CE190 | pendiente | |
| CalcAppearPoint | 0x9CE1A0 | pendiente | |
| CalcWaitPoint | 0x9CE410 | pendiente | |
| SetShotParam | 0x9CE740 | pendiente | |
| PlHitChk | 0x9CE850 | pendiente | |
| CalcRunAngle | 0x9CE9D0 | pendiente | |
| CtrlPre | 0x9CEA50 | pendiente | |
| CtrlPut | 0x9CED70 | pendiente | |
| CtrlPost | 0x9CED90 | pendiente | |
| PutSetObj | 0x9CEF00 | pendiente | |
| SetB6B2BDead | 0x9CF250 | pendiente | |
| SetB6B3BDead | 0x9CF2B0 | pendiente | |
| SetB6B4BDead | 0x9CF310 | pendiente | |
| new_TBoss6XBullet | 0x9CF390 | pendiente | |
|  | 0x9CF4E0 | pendiente | |
| CtrlPre | 0x9CFA30 | pendiente | |
| CtrlPut | 0x9CFC80 | pendiente | |
| CtrlPost | 0x9CFD20 | pendiente | |
|  | 0x9CFFC0 | pendiente | |
|  | 0x9D02A0 | pendiente | |
| SetShotParam | 0x9D0360 | pendiente | |
| CalcPos | 0x9D0490 | pendiente | |
| CtrlPre | 0x9D05A0 | pendiente | |
| CtrlPut | 0x9D0890 | pendiente | |
| CtrlPost | 0x9D0BA0 | pendiente | |
| new_BS6B5Water | 0x9D0CC0 | pendiente | |
|  | 0x9D0D70 | pendiente | |
| MulShadowMatrix_sub | 0x9D1070 | pendiente | |
| MulShadowMatrix | 0x9D1210 | pendiente | |
| CtrlPre | 0x9D12C0 | pendiente | |
| CtrlPut | 0x9D13C0 | pendiente | |
| CtrlPost | 0x9D1540 | pendiente | |
| oskDispFunc | 0x9D15C0 | pendiente | |
| oskDispFunc | 0x9D1600 | pendiente | |
|  | 0x9D1640 | pendiente | |
| CtrlPre | 0x9D1780 | pendiente | |
| CtrlPut | 0x9D17D0 | pendiente | |
| new_Boss6AfterImage | 0x9D1890 | pendiente | |
|  | 0x9D1940 | pendiente | |
| Ctrl | 0x9D1B10 | pendiente | |
| new_Boss6_2CameraCtrl | 0x9D1CA0 | pendiente | |
|  | 0x9D1D20 | pendiente | |
| CtrlPre | 0x9D1F10 | pendiente | |
| CtrlPut | 0x9D1FA0 | pendiente | |
| CtrlPost | 0x9D2030 | pendiente | |
| new_Boss6Moon | 0x9D2050 | pendiente | |
|  | 0x9D2160 | pendiente | |
| Ctrl | 0x9D2240 | pendiente | |
| new_Boss6SkyFade | 0x9D2360 | pendiente | |
|  | 0x9D24A0 | pendiente | |
| CtrlPre | 0x9D25E0 | pendiente | |
| CtrlPut | 0x9D2630 | pendiente | |
| CtrlPost | 0x9D26A0 | pendiente | |
| new_Boss6MaskFeather | 0x9D26E0 | pendiente | |
|  | 0x9D27D0 | pendiente | |
|  | 0x9D2860 | pendiente | |
|  | 0x9D28F0 | pendiente | |
|  | 0x9D2980 | pendiente | |
|  | 0x9D2A10 | pendiente | |
|  | 0x9D2AA0 | pendiente | |
|  | 0x9D2B30 | pendiente | |
|  | 0x9D2BE0 | pendiente | |
|  | 0x9D2C80 | pendiente | |
| "@48@oskDispFunc | 0x9D2D10 | pendiente | |
| "@48@prtsDispFunc | 0x9D2D20 | pendiente | |
| "@7688@64@ | 0x9D2D30 | pendiente | |
| "@644@64@ | 0x9D2D40 | pendiente | |
| "@536@64@ | 0x9D2D50 | pendiente | |
| "@776@64@ | 0x9D2D60 | pendiente | |
| "@636@64@ | 0x9D2D70 | pendiente | |
| "@22428@64@ | 0x9D2D80 | pendiente | |
| "@36@64@ | 0x9D2D90 | pendiente | |
| "@60@64@ | 0x9D2DA0 | pendiente | |
| "@56@64@ | 0x9D2DB0 | pendiente | |

## Totales (contados de las tablas de arriba)
- reescritas: 50 (de ellas con test: 23, sin test: 27)

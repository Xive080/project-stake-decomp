# bosses56: jefes 5 y 6 (overlays bs5mod / bs6mod, carga 0x9C4800)

Reescritura a mano en C++17 de las reglas (no del dibujo). Motor y geometria (rutas, matrices, atan2, sqrt, sonido, efectos, rand)
pasan por interfaces Env. Estado: `reescrita+test` = hay aserciones en game/test/bosses56_test.cpp; ninguna se comparo ejecutando el
original, solo contra el asm leido y las tablas del BIN.

Comando (desde la raiz del repo):
`mkdir -p build/verify_bosses56 && c++ -std=c++17 -Wall -Wextra game/test/bosses56_test.cpp game/bosses56/bosses56_b5.cpp game/bosses56/bosses56_b6.cpp game/bosses56/bosses56_data.cpp game/bosses_b/boss6.cpp game/bosses_b/boss6_data.cpp -o build/verify_bosses56/bosses56_test && build/verify_bosses56/bosses56_test`
Resultado: `bosses56_test: OK` (sin avisos con -Wall -Wextra).

## Jefe 5 (bosses56_b5.*): 14 funciones
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| GetCamRot | 0x9C7160 | reescrita+test | |
| GetCamPos | 0x9C71C0 | reescrita+test | rotacion via env.rotYaw |
| UpdateMotionFrame | 0x9C5970 | reescrita+test | |
| ControlByMotion | 0x9C59B0 | reescrita+test | estructura leida de m2c, NO verificada linea a linea contra el asm |
| ControlByMotionX | 0x9C5A70 | reescrita+test | avisos verificados contra asm; efecto Y = bits 0xC1319999 |
| InitPath | 0x9C6B30 | reescrita+test | |
| ControlPath | 0x9C6B80 | reescrita+test | 0x9D/0xAF: resta de 2.0822 a una Y no usada, omitida |
| GetWaitPos | 0x9C6F90 | reescrita+test | |
| InitWaitPath | 0x9C6E60 | reescrita+test | rand % 4 con resto con signo |
| ControlWaitPath | 0x9C6EE0 | reescrita+test | guarda de longitud <= 0 (el original se colgaria): difiere |
| ControlPos | 0x9C6560 | reescrita+test | |
| CtrlPre | 0x9C5010 | reescrita+test | copias estaticas unicas de tablas del motor omitidas |
| CtrlPre_battle1 | 0x9C5560 | reescrita+test | solo el orden de pasos (los pasos van por B5StepEnv) |
| CtrlPre_battle2 | 0x9C5690 | reescrita+test | idem |

## Jefe 6 (bosses56_b6.*): 32 funciones
| Original | Direccion | Estado | Notas |
|---|---|---|---|
| AddMotionFrame, AddPathFrame | n/d | reescrita+test | |
| ApproachANPO | n/d | reescrita+test | usa Boss6Logic::b6InterDivAngF |
| GetPathParam (3 variantes) | n/d | reescrita+test | las variantes Point/CT solo se ejercitan a traves de GetPathParam |
| TracePathParam, UpdatePath | n/d | reescrita+test | |
| SetCircleMove, CircleMove | n/d | reescrita+test | contra kPoseB_flat; 240 pasos |
| EffectCtrl, CtrlPre | n/d | reescrita+test | |
| Bat: SetApproach, Approach, SetFallParam, FallDown (0xBDA740DA), ChgDead | n/d | reescrita+test | |
| Bat: SetPathParam, TraceShotPath, SetSpiralParam, TraceSpiralPath, RotInit | n/d | reescrita+test | tablas D_009D4720/47A8/4830 comparadas con disc/TBL/BS6MOD.BIN |
| Bat: SetBatSpiral (solo parte escalar), AddBatSpiralCount | n/d | reescrita+test | dist se recibe ya calculada |
| Bat: ChkMode, ExecMode (jtbl_009D52F0), PlHitChk, SetTargetID | n/d | reescrita+test | |
| SkyFadeCtrl, MaskFeatherPre/Post, AfterImagePre | n/d | reescrita+test | |

(Las direcciones de las funciones del jefe 6 no se recopilaron en esta tabla; estan en los comentarios de bosses56_b6.cpp.)

## Pendiente (no reescrito)
CtrlPut/CtrlPost, EntryCameraTarget/EntryCollision, SetPath y funciones con tabla de saltos, familia HitMarkEff*, XWing, Bullet, B4Mask,
B5Water, Shadow, Moon, CameraCtrl, RotPos y SetShotParam (detras de env.rotPos / env.setShotParam), SetBossLight, WaterPillar,
partes de TBoss5Ice, XIceShape, resto de CheckDamageEnd. Tampoco InitWaterPillar ni Ice StartAttack/Attack_spear.

## No verificado / supuestos
- Significados supuestos: func_1A79A0 = atan2 a 16 bits, func_1B0AA0 = sqrt, func_1B09B4/1B0938 = sin/cos (cual es cual sin verificar),
  func_1CC710 = muestra de ruta, func_1CCCB0 = longitud, func_1AF830 = suma de vectores.
- +0x44 como fotograma actual es inferido.
- Varias salidas de m2c eran incorrectas y se corrigieron a mano desde el asm; no hay ejecucion del original para contrastar.
- Tablas generadas con tools/gen_tables.py.

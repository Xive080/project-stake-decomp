# MANIFEST boss6 (overlay bs6mod, base 0x9C4800, Boss6Task)

Archivos: `boss6.h`, `boss6.cpp`, `boss6_data.cpp` (generado por `build/verify_boss6/gen6.py` desde `disc/TBL/BS6MOD.BIN` y `baserom/SCES_504.11`), test `game/test/boss6_test.cpp`.

Verificacion (desde la raiz del repo):

    mkdir -p build/verify_boss6/bin && c++ -std=c++17 -Wall -Wextra game/test/boss6_test.cpp game/bosses_b/boss6.cpp game/bosses_b/boss6_data.cpp -o build/verify_boss6/bin/boss6_test && build/verify_boss6/bin/boss6_test

Resultado: `boss6_test: OK`. Si faltan los binarios del disco/baserom el test omite la comparacion de tablas con un mensaje.

Metodo: borrador con m2c sobre el asm (con jump tables), contrastado leyendo el asm; las tablas se extraen por script. El test es de caracterizacion: los valores esperados son del BIN leido a mano y de lo que dice el asm, pero la logica es la reescrita (no hay ejecucion del original, no hay emulador).

Estados: "reescrita+test" = el test ejecuta la funcion con comprobaciones de resultados; "reescrita sin test"; "pendiente".

| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| InitBS6Difficulty | 0x9C4EE0 | reescrita+test | |
| B6InterDivAngF | 0x9C55F0 | reescrita+test | solo camino corto, 3 casos |
| SetApproach | 0x9C56A0 | reescrita+test | |
| AttackDamageFrame | 0x9C58C0 | reescrita+test | solo tipos 0 y 8 ejercitados; resto de tipos (bullets, murcielagos, agua) via hook sin comprobar |
| ControlAttack | 0x9C5B10 | reescrita+test | solo tipos 0, 2/8 ejercitados; tipos 1,3,4,5,6,7 sin test (caida del caso 4 al 5 verificada solo contra el asm) |
| StartAttack | 0x9C62A0 | reescrita+test | |
| CheckDamageEnd | 0x9C6380 | reescrita+test | matrices/pose via hook H_RESTORE_POSE_AFTER_DAMAGE |
| InitLife | 0x9C6740 | reescrita+test | |
| ControlDispLife | 0x9C6850 | reescrita+test | solo parte numerica; dibujo de barras por hook |
| CheckWeakHit | 0x9C6F30 | reescrita+test | |
| ClearWeakHit | 0x9C6F70 | reescrita+test | |
| CheckDamage | 0x9C6FB0 | reescrita+test | los impactos llegan como estructura HitInfo (la lectura de la lista de golpes del motor no se reescribe) |
| WeakHit | 0x9C7140 | reescrita+test | los 3 tipos, modo especial, golpe suelto; efectos por hook |
| NoWeakHit | 0x9C7750 | reescrita+test | |
| DamageUpperBody | 0x9C7780 | reescrita+test | |
| Damage | 0x9C7900 | reescrita+test | |
| SetDamageMotion | 0x9C79A0 | reescrita+test | orientacion a camara (matrices) por hook |
| batKillAdd | 0x9C7B60 | reescrita+test | |
| setLightColor | 0x9C7BA0 | reescrita+test | |
| IncBS6Difficulty | 0x9C7D00 | reescrita+test | |
| DecBS6Difficulty | 0x9C7D80 | reescrita+test | |
| GetBS6Difficulty | 0x9C7E00 | reescrita+test | |
| actNop | 0x9C7E70 | reescrita+test | |
| actDelete | 0x9C7E80 | reescrita+test | |
| ExecSeqCmd | 0x9C7F10 | reescrita+test | 68 operaciones escritas; el test ejecuta solo unas 20 (0,2,4,6,7,10,12,13,17,19,23,24,26,28,35,45,46,47,48,49); el resto sin test. Las ops 55,56,57,61,63 son bucles infinitos en el original: aqui devuelven. Ops de geometria (19,20,43,66...) por hook |
| SeqCmdInit | 0x9C9690 | reescrita+test | |
| SeqCmdNextTable | 0x9C96C0 | reescrita+test | |
| SeqCmdSetTable | 0x9C96F0 | reescrita+test | |
| actExecSeqCmd | 0x9C9720 | reescrita+test | |
| actDead | 0x9C9740 | reescrita+test | |
| FinishTask0 | 0x9C9750 | reescrita+test | |
| FinishTask | 0x9C9840 | reescrita+test | delega en func_950F80 por hook |
| AllowCameraStart | 0x9C9860 | reescrita+test | |
| ForbidCameraStart | 0x9C9870 | reescrita+test | |
| EnableCollision | 0x9C9880 | reescrita+test | |
| DisableCollision | 0x9C98E0 | reescrita+test | |
| MotSupSet2 | 0x9C9940 | reescrita+test | probado solo a traves de la op 10 de ExecSeqCmd |
| SelectAttackTarget | 0x9C9A70 | reescrita+test | |
| SetBulletNum | 0x9C9B50 | reescrita+test | |
| AtcPlFlgClear | 0x9C9C20 | reescrita+test | |
| DecNowEnemyAlvNumChk | 0x9C9C50 | reescrita+test | |
| DecNowEnemyNumChk | 0x9C9C80 | reescrita+test | |
| GetPathEndFrame | 0x9C9CB0 | reescrita+test | |
| BodyAlphaControl | 0x9CA2C0 | reescrita+test | |
| BodyAlphaDirectSet | 0x9CA420 | reescrita+test | |
| BodyAlphaSupSet | 0x9CA430 | reescrita+test | |
| CtrlBossLight | 0x9CA440 | reescrita+test | |
| InitBranchRate | 0x9CAAE0 | reescrita+test | |
| GetRandomBranchRate | 0x9CAB60 | reescrita+test | caso sin acierto devuelve 0 (en el original es el valor previo del registro; inferido) |
| LoopSoundOff | 0x9CACB0 | reescrita+test | solo a traves de ControlAttack/FinishTask0 |
| SetSpeedBaseRate | 0x9CB010 | reescrita+test | |
| (CtrlPre / CtrlPut / CtrlPost / ExecCmd / CharInit / BodyInit / EntryCameraTarget / EntryCollision) | 0x9C4F30..0x9C6CC0 | pendiente | motor: bucle de tarea, entrada de colisiones, camara, inicializacion de cuerpo |
| AddMotionFrame / UpdatePath / AddPathFrame / ApproachANPO | 0x9C5460..0x9C58C0 | pendiente | avance de movimiento/ruta y aproximacion con matrices (motor) |
| SetCircleMove / CircleMove | 0x9CACF0 / 0x9CAEF0 | pendiente | trigonometria y matrices; se llama por hook (op 66) |
| TracePathParam / GetPathParam* / SetBossLight | 0x9C9D00..0x9CA250, 0x9CA480 | pendiente | evaluacion de rutas y luz del motor |
| HitMarkEff0 / HitMarkEff | 0x9C7520 / 0x9C76E0 | pendiente | efectos de golpe (dibujo) |
| DispLife / DispParts / oskDispFunc / prtsDispFunc / colliFunc | 0x9C67F0, 0x9C6CD0..0x9C6F20 | pendiente | dibujo |
| EffectCtrl / SetAlpha / initBlur / endBlur | 0x9CB100..0x9CB2C0 | pendiente | efectos; initBlur/endBlur como hooks H_INIT_BLUR / H_END_BLUR |
| Boss6XWingTask, TBoss6XBat, Bullet, B4Mask, B5Water, Shadow, AfterImage, CameraCtrl, Moon, SkyFade, MaskFeather | 0x9CB3F0..0x9D1CA0 | pendiente | clases auxiliares (disparos, murcielagos, cielo, camara); no tocadas |

## Contados

Reescritas: 51 funciones (todas "reescrita+test"; tres de ellas, ExecSeqCmd, ControlAttack y AttackDamageFrame, con cobertura parcial de ramas).

## Inferido o sin verificar

- Nombres de campos de `Boss6State` y de hooks: por uso, no por simbolos.
- Los valores esperados del test salen del BIN (tablas) y de la lectura del asm; no hay ejecucion real del original para contrastar.
- Muchas ramas del interprete y de ControlAttack no tienen test (ver tabla).
- Tablas: `kLight_flat` incluye 5 filas iniciales (0x9D430C..0x9D43C0) que no son floats (contienen NaN/datos); se conservan por layout, pero no se sabe su significado real. setLightColor solo lee a partir de la fila 5.
- Tablas del ELF (0x2D8800, script centinela {48,54}) leidas como int16.

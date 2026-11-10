# bosses_a: logica de los jefes de bs2mod y bs3mod (C++17, sin dependencias de plataforma)

Estado de cada funcion. "T" = reescrita + test; "S" = reescrita sin test propio (solo se ejercita indirectamente); "P" = pendiente.
Todas las reescritas se leyeron contra el asm (asm/bs2mod, asm/bs3mod); m2c solo se uso de apoyo y se corrigio varias veces.
Los tests usan motores simulados (Mock2/Mock3): comprueban las reglas que salen del asm (umbrales, tablas, ramas), NO comparan contra una ejecucion real del juego.

Compilar y ejecutar (desde la raiz del repo):

    c++ -std=c++17 -Wall game/test/bosses_a_test.cpp game/bosses_a/boss2.cpp game/bosses_a/boss3.cpp game/bosses_a/boss_data.cpp -o build/bosses_a_test && ./build/bosses_a_test

## Datos (boss_data.cpp, sacados del BIN/ELF)
kBs2Attack[24], kBs2Diff[5][2], kBs2CancelSkip[5], kBs3Attack[20], kBs3Motion[10], kBs3DmgNormal/kBs3DmgBit8 [3][34], kBs3BulletsA/B [34], kBs3PhaseParam[17][6].
Sin verificar: kBs3Motion tiene al menos 12 entradas en el overlay (0x9D0880..); solo se copian 0..9 (las que usa el codigo). kBs3PhaseParam[17]: la cuenta de filas no esta comprobada contra el limite real de la tabla.

## Boss2 (boss2.h/.cpp, bs2mod)

| Original | Direccion | Estado | Notas |
|---|---|---|---|
| InitLife | 0x9C7740 | T | |
| ControlDispLife | 0x9C77C0 | S | solo las proporciones; el original ademas las pasa al HUD |
| SetDiffParam | 0x9C7D20 | T | |
| InitDifficulty | 0x9C7DF0 | T | |
| UpDifficulty | 0x9C7E50 | T | |
| DownDifficulty / DownDifficultyReq | 0x9C7E80 / 0x9C7E20 | T | el mismo codigo |
| NoChgDifficultyReq | 0x9C7E40 | T | |
| CheckWeakHit | 0x9C88B0 | T | |
| CheckDamage | 0x9C8940 | T | |
| WeakHit | 0x9C8A70 | T | |
| NoWeakHit | 0x9C8C40 | S | via checkDamage |
| DamageUpperBody | 0x9C8C70 | T | |
| SetDamageMotion | 0x9C9230 | S | forma fuera de rango: el original usa un registro sin inicializar; se devuelve -1 |
| Damage | 0x9C90C0 | T | |
| Dead | 0x9C92A0 | T | |
| CheckDamageEnd | 0x9C76D0 | T | |
| DamageToPlayer | 0x9CB370 | T | |
| AttackDamageFrame | 0x9C6E20 | T | |
| StartAttack | 0x9C72B0 | T | id negativo distinto de -1: indefinido en el original (registros sin inicializar), no reproducido |
| ControlAttack | 0x9C7020 | T | |
| SelectAttackTarget | 0x9CB200 | T | |
| AtcPlFlgClear | 0x9CB2E0 | T | |
| DecNowEnemyAlvNumChk / DecNowEnemyNumChk | 0x9CB310 / 0x9CB340 | T | |
| MotSupSet2 | 0x9CB140 | T | |
| SetHeight | 0x9CAF40 | T | |
| FallCheck | 0x9CB030 | T | |
| ControlSound | 0x9C6A70 | T | |
| ControlOutline | 0x9C6970 | T | releida contra el asm |
| StartWarpOut | 0x9C6B00 | T | releida contra el asm |
| ControlWarp | 0x9C64B0 | T | NO incluye la eleccion del movimiento de entrada (tablas D_009D4DF0/D_009D4E00/D_9D7800) ni el giro hacia el jugador |
| ETurn | 0x9CB620 | T | |
| AngChk | 0x9CB6D0 | T | |
| SeqCmdInit / NextTable / Push / Pop / Call / CancelWait | 0x9C9DD0..0x9C9F00 | T | solo ayudantes de pila y rama |
| BranchInit / Branch | 0x9C9F60 / 0x9C9FE0 | T | |

Total Boss2: 41 funciones C++ (36 originales distintos con los alias contados una vez), de ellas 38 con test directo y 3 sin (ControlDispLife, NoWeakHit, SetDamageMotion: indirectas).

## Boss3 (boss3.h/.cpp, bs3mod)

| Original | Direccion | Estado | Notas |
|---|---|---|---|
| InitBS3Difficulty | 0x9C4EC0 | T | |
| IncBS3Difficulty / DecBS3Difficulty | 0x9C7E30 / 0x9C7EB0 | T | |
| GetBS3Difficulty | 0x9C7F30 | T | |
| InitLife | 0x9C68A0 | T | |
| ControlDispLife | 0x9C69F0 | T | |
| CheckWeakHit | 0x9C74D0 | T | |
| CheckDamage | 0x9C7540 | T | |
| WeakHit | 0x9C7670 | T | |
| DamageUpperBody | 0x9C7C80 | T | usa kBs3Motion[9] (0x9D08C8 = {343,358}) |
| SetDamageMotion | 0x9C7DF0 | T | forma 2: registro sin inicializar, se devuelve -1 |
| Damage | 0x9C7D60 | T | |
| CheckDamageEnd | 0x9C6750 | T | |
| SelectAttackTarget | 0x9C95D0 | T | |
| SetBulletNum | 0x9C96B0 | T | |
| AtcPlFlgClear | 0x9C9770 | T | |
| DecNowEnemyAlvNumChk / DecNowEnemyNumChk | 0x9C97A0 / 0x9C97D0 | T | |
| AttackDamageFrame | 0x9C5FB0 | T | |
| StartAttack | 0x9C66D0 | T | ver nota de motion abajo |
| ControlAttack | 0x9C60B0 | T | caso por defecto de los tipos 0..2 comparado con el asm solo a ojo |
| MotSupSet2 | 0x9C94A0 | T | indices >= 10 pasan crudos al motor (sin verificar) |
| B3InterDivAngF | 0x9C5CD0 | T | |
| SetApproach | 0x9C5D80 | T | |
| ApproachANPO | 0x9C5E00 | T | |
| SeqCmdInit / NextTable / SetTable | 0x9C9160 / 0x9C9190 / 0x9C91C0 | T | |

Total Boss3: 28 funciones C++, todas con test directo.

## Sin verificar / dudas abiertas
- Boss3 StartAttack/ControlAttack: kBs3Attack[].motion vale 324, 304... (>= 10) y el codigo lo trata como indice de kBs3Motion cuando la escena != 0x14. Con valores asi el indice se sale de la tabla de 10 entradas. No he resuelto como lo interpreta el original (la tabla real puede ser mas larga o esos ids van directos); por eso se protege con un limite y el comportamiento para motion >= 10 fuera de la escena 0x14 NO esta verificado.
- La semantica de cada llamada inyectada (sonidos, efectos, camara, blur, rutas de ataque) es una interpretacion mia de los argumentos que ve el asm; los motores simulados solo registran las llamadas.
- Boss2 controlWarp llama a DisableCollision en cada fotograma mientras alfa <= umbral y el alfa previo >= umbral (dos veces seguidas en el test). Coincide con el asm leido, pero no esta contrastado en ejecucion.

## Pendiente (no reescrito)
- ExecSeqCmd de ambos jefes (interpretes de bytecode), CtrlPre / CtrlPut / CtrlPost, CharInit, ExecCmd, FinishTask, ControlPos.
- Boss2: eleccion del movimiento de entrada del teletransporte, AttackPosSet / SetAttackDist.
- Clases auxiliares: Bomb, Laser, XWing, Subchr; HitMarkEff*, GetPathParam*, las clases de efectos.

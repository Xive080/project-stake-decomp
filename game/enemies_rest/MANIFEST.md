# MANIFEST game/enemies_rest (enemigos comunes, segunda tanda)

Completa las filas "pendiente" de game/enemies/MANIFEST.md (EnVampire, PeopleTask, ErsPplTask). No modifica nada de game/enemies:
reutiliza sus estructuras y añade `VampExt` / `PeopleExt` (campos que solo se usan aqui) y eventos `RE_*` (EV_SCRIPT, a >= 0x7001).
Verificacion: leyendo el asm y tests unitarios (game/test/enemies_rest_test.cpp). El original NUNCA se ejecuto: no hay comparacion en vivo.
m2c no se uso (todo se decodifico a mano del asm; las funciones son cortas salvo checkDamage_Eknd, revisada linea a linea).

Compilar y ejecutar (desde la raiz del repo):

    c++ -std=c++17 -Wall game/test/enemies_rest_test.cpp game/enemies_rest/enemies_rest.cpp game/enemies_rest/rest_tables.cpp game/enemies/enemy_common.cpp game/enemies/vampire.cpp game/enemies/vampire_tables.cpp game/enemies/people.cpp -o build/verify_enemies_rest/enemies_rest_test && ./build/verify_enemies_rest/enemies_rest_test

Resultado de la ultima ejecucion: `OK: 255 comprobaciones, 0 fallos` (las tablas se compararon con disc/TBL/GAMEMOD.BIN y baserom/SCES_504.11; si faltan, el test lo avisa y las omite).

Estado: R+T = reescrita con test; R = reescrita sin test; PEND = pendiente.

## EnVampireTask (GAMEMOD, vram = offset + 0x92B000)
| Funcion | Direccion | Estado | Notas (no verificado / inferido) |
|---|---|---|---|
| changeDamageMot | 0x954000 | R+T | La tabla de tipo 0x1F2F860[tipo] es RAM de ejecucion (no esta en el ELF): entra como `motTypeFlag`. Si != 0x12 el original usa los registros a2, a3, t3 SIN inicializar (heredados del llamador): se modelan como entradas `staleA2/A3/Threshold` (defecto 1/0/11), valor real sin resolver. Animacion de dano por clase: D_009A23D0 -> D_009A23C0 -> D_009B5D50 (extraida; los 18 tipos y 4 subtipos coinciden). func_1FAA60 (parte baja) se emite como RE_DAMAGE_LAYER sin interpretar sus argumentos |
| checkDamage_Eknd | 0x953870 | R+T | Dano fijo 45 (0x2D) via decLife. Eleccion aleatoria de la armadura que se rompe portada literalmente (muestreo sin reposicion con s1 = 1, consume rand como el original). `armorCount` = 0x1F29860[tipo] (RAM de ejecucion, inyectado). Posiciones de chispa/trazadora salen en `EkndOut`; el paso por la matriz de camara (func_1B00E0/1AFC00) y TyakudanLauncherMgrBorn quedan para el motor. EnPartsChain recibe (1, 1, jugador, tabla[0], tabla[4]) con tabla D_009A2470 extraida. Con invulnerabilidad SI da +10 de puntos (distinto de checkDamage, que no) |
| Wait | 0x955A40 | R+T | Tablas D_009A22C0 -> ELF 0x2D8860 -> listas s16 de GAMEMOD extraidas (25 entradas, solo las 2 primeras animaciones de cada lista, las unicas que se usan: rand%2). Supone nlRand >= 0 (con negativo impar el original leeria lista[-1]). motLen (0x1FB73E4) inyectado |
| turnTargetSub (parte de reglas) | 0x954AA0 | R+T | `turnClamp`: giro acotado. La parte de matrices y nlArcTan2 (angulo al objetivo) es del motor: entra como `angleToTarget` |
| turnTarget | 0x954BD0 | R+T | Seleccion de objetivo por +0x1DAC (-1 camara/lado, -2/-3 reflejo, -4 reflejo de la marca, otro = puntero). Traslaciones con matriz de camara inyectadas (`sideTarget`, `mirrorSrc`); el desplazamiento lateral +-3.5 esta en `turnSideOffsetX` |
| FirstCtrl | 0x9526D0 | R+T | Cierre en dos tiempos (marca con +0x1D98 = 1, cierra al siguiente), liberacion del hueco de ataque, integracion de movimiento en el aire (+0x30: pos += vel; vel += acel) y llamada a turnTarget |
| LastCtrl | 0x952B40 | R+T | Compone chkParam y getParam existentes. Entrada en camara (CameraTask func_1C97D0) como RE_CAM_ENTRY con escala 1000/|v|^2 o 100/|v|^2 (v = +0x34C); el punto lo calcula el motor |
| KneadEffect | 0x952DA0 | R+T | Solo la regla (intensidad de la sombra, id de efecto 0x2600B8 + frame%5, escala 0.7*s). Y en camara de cada parte entra inyectado (`viewY`); el dibujo (func_214AE0) lo hace el motor |
| weakInit | 0x953360 | PEND | no examinada |
| turnTargetSub (matematica), adjChest/adjPos, getHight | - | PEND | matematica de motor |
| display / colli | - | PEND | dibujo, fuera de alcance |

## PeopleTask (ELF main)
| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| FirstCtrl | 0x205880 | R+T | A diferencia de EnVampire cierra al instante al pasar el camino/fotograma. Fuera de pantalla (CheckOnSc) inyectado. La llamada a turnTarget sale como RE_TURN_TARGET (turnTarget de People sigue pendiente). Calcula +0x1DF8 (distancia XZ) y +0x1DFC (3D) con sqrt |
| LastCtrl (completa) | 0x205D70 | R+T | Compone lastCtrlWait + getParam existentes + entrada en camara (si gp-0x7210 < gp-0x7214) + avance de frames (+0x3C y +0x40 += velocidad si +0x1DAC != 0 y la animacion no cambio). `gp7210` es un s16 cuyo significado no se identifico |
| turnTarget, backWaitPos, keepAwaySpeed | - | PEND | matematica vectorial (fuera de esta tarea) |
| ChangeEffect, WalkNoisily, shapeAnim*, display, colli | - | PEND | |

## ErsPplTask (ELF main)
| Funcion | Direccion | Estado | Notas |
|---|---|---|---|
| FirstCtrl | 0x20D100 | R+T | Cierra si +0x1D78 del padre == 0; alfa triangular: 2a si a <= 0.5, si no 2 - 2a. El original sigue ejecutando tras TaskClose |
| LastCtrl | 0x20D230 | R+T | frame3C y +0x40 += +0x1D78 propio |
| constructor, Disp, prtsDispFunc, oskDispFunc, colliFunc | - | PEND | motor / dibujo |

## Total
Reescritas con test: 12 (EnVampire 8: changeDamageMot, checkDamage_Eknd, Wait, turnTargetSub (solo regla), turnTarget, FirstCtrl, LastCtrl, KneadEffect;
People 2: FirstCtrl, LastCtrl completa; ErsPpl 2: FirstCtrl, LastCtrl). Helpers auxiliares: pickWaitMot, turnClamp, turnSideOffsetX (no contados aparte). Reescritas sin test: 0.
Cada "R+T" tiene aserciones de rama (valores deducidos del asm); los tests de tablas comparan contra los binarios reales.

## Sin verificar / hallazgos
- Registros sin inicializar en changeDamageMot cuando 0x1F2F860[tipo] != 0x12 (ver tabla): comportamiento real desconocido.
- Tablas de RAM de ejecucion (0x1F29860 armaduras por tipo, 0x1F2F860 flag de animacion, 0x1FB73E4 duraciones): no estan en ningun fichero; inyectadas.
- En EnVampire el campo +0x1DAC (VampireState::motLoop, game/enemies) es en realidad el selector de objetivo de giro (FirstCtrl lo comprueba != 0 para llamar a turnTarget); el nombre "motLoop" de vampire.h es dudoso. En PeopleTask +0x1DAC si es el contador de repeticiones y +0x1DD0 el selector de giro.
- PeopleTaskState::p1D78 esta declarado int en people.h, pero ErsPplTask lee +0x1D78 del padre como float (progreso/alfa).
- Los significados de los argumentos de func_1FAA60 (RE_DAMAGE_LAYER), func_1A2A10 (RE_PRE_CLOSE) y func_9357A0 no se investigaron.
- Eventos RE_* y los offsets nuevos (VampExt/PeopleExt) son inferidos del asm.

# MANIFEST game/enemies3 (clases En* auxiliares de GAMEMOD, tercera tanda)

Cubre las clases En* de GAMEMOD que no estaban en game/enemies, enemies_ov (EnZombie), enemies_rest (EnVampire/People):
EnEtcThrowObj, EnEtcTask, EnEtcBone, EnEtcAttach, EnDamSphere, WeakTask, ArmorTask, PetitZombieTask, EnVtxEffCtrl, EnemyMgrClass y
las utilidades sueltas (EnmID*, Enemy_Set_*Buff, EnSndReq, EnPartsChain, EntryRankingBufferSub).
Direcciones = vram de GAMEMOD (carga en 0x92B000; offset de fichero = addr - 0x92B000).

Fuentes: asm/gamemod/gamemod_text.s + borrador m2c (copia sin opcodes vsqi/vlqd en build/m2c/enemies3_gamemod.s; las que tienen
tabla de saltos en m2c-fallo se leyeron del asm). El original NUNCA se ejecuto: no hay comparacion de comportamiento en vivo.

Compilar y ejecutar (desde la raiz del repo):

    c++ -std=c++17 -Wall game/test/enemies3_test.cpp game/enemies3/enemies3_misc.cpp game/enemies3/enemies3_throw.cpp -o build/verify_enemies3/enemies3_test && ./build/verify_enemies3/enemies3_test

Ultima ejecucion: `OK: 274 comprobaciones, 0 fallos` (tambien limpio con -Wextra).

Verificacion real hecha:
- Tablas D_009857A0 (EnSndReq) y D_00985430 (claves de color de particulas) comparadas con GAMEMOD.BIN.
- Cada constante de 32 bits transcrita (sonidos, ids de animacion, floats por patron de bits, umbrales) se busca como `lui/ori` (o `addiu/slti`)
  dentro del rango de bytes de su funcion original en GAMEMOD.BIN. Eso confirma el VALOR, no la logica que lo rodea.
- Lectura directa del asm (no solo m2c) de: Enemy_Set_ColliBuff, Enemy_Set_WaitPosBuff, EnSndReq, EnPartsChain, Ctrl2_Pos, Ctrl2_Pre (desplazamientos
  de pila de las dos transformaciones de camara), Init5 (division sin signo por 0x88888889), Ctrl_Prtc (tramos de escala/color).
- El resto de la logica sale del borrador m2c leido a mano; los tests comprueban casos construidos a mano, no ejecucion real.

Estado: R+T = reescrita con test; R+T parcial = solo la parte de reglas (el dibujo/matrices es del motor); PEND = pendiente.

## Utilidades (enemies3_misc.cpp)
| Funcion original | Direccion | Estado | Notas |
|---|---|---|---|
| EnmIDInit | 0x9356F0 | R+T | 32 punteros D_501310 a 0 |
| EnmIDSet | 0x935740 | R+T | tabla llena deja +0x24 = -1 y no toca flags2 |
| EnmIDClear | 0x9357A0 | R+T | id como s8; exige flags2 & 1; no limpia flags2 |
| Enemy_Set_ColliBuff | 0x92CD90 | R+T | buffer D_9B9000 (0x14 B) + contador gp-0x7004 < 0xFF; la transformacion de la colision (flag 0x10) sale como evento |
| Enemy_Set_WaitPosBuff | 0x936170 | R+T | la distancia (func_1B0AA0 de x^2+z^2, con mula/madd) entra ya calculada |
| EnSndReq | 0x9426E0 | R+T | tabla D_009857A0; el 2o 0x2001B (indice 2) es inalcanzable (gana el primero) |
| EntryRankingBufferSub | 0x9708C0 | R+T | indice -> ASCII; negativos devuelven idx+0x41 (como el original, no se acotan) |
| EnPartsChain | 0x93B1E0 | R+T | dos peticiones new_EnEtcParts (modo 2 y 3) con el mismo EnemyTask |
| EnemyMgrClass::entryEvent | 0x935380 | R+T | recorre palabras hasta -1; entryEventSub es del motor (evento E3_ENTRY_SUB con indice de palabra) |
| EnemyMgrClass::entryEvent2 | 0x935420 | R+T | modo 0 = MENM, 1 = RAND |
| entryEvent2_MENM | 0x935480 | R+T | una entrada por fallo acumulado (gp-0x7358, s16) |
| entryEvent2_RAND | 0x9354F0 | R+T | eleccion sin reposicion con nlRandom; supone que nlRandom devuelve float (m2c) |

## EnEtcTask (enemies3_misc.cpp)
| Funcion original | Direccion | Estado | Notas |
|---|---|---|---|
| St4_02TimerBunki | 0x9387A0 | R+T | v50*50/60 fotogramas; cuenta fallos si hay enemigos en pantalla |
| EnEtc_St4_Tama | 0x9385D0 | R+T | rampa de alpha 0 -> 0.9 -> 0; el objeto de pantalla gp-0x7440 entra como St4Fade |
| EnEtcST3_SC02_02FireBall | 0x938230 | R+T parcial | solo la maquina de estados. La posicion por camino (func_1CC710) entra como callback; con otro tipo que 6/7 el original deja la posicion SIN inicializar (aqui se conserva). Dibujo y sombra no |
| EnEtc_BranchMark | 0x938860 | R+T parcial | reglas de activacion/cierre; func_A10B00 (ORGMOD) sale como evento; dibujo y colision no |
| EnEtc_ItemCheck | - | no incluida | solo llama a TaskClose |
| SnowCtrl, EnEtc_ST3.., ctor 0x....(EnEtcTask) | - | PEND | SnowCtrl usa mula/madd (distancia) y buffer D_9C3990; ctor sin leer |

## Objetos pequenos (enemies3_misc.cpp)
| Funcion original | Direccion | Estado | Notas |
|---|---|---|---|
| EnEtcAttach::Ctrl_Pre | 0x93CA70 | R+T | caida con rebote (0.7/-0.3/0.7), rotacion x0.5 por rebote, sonido 0x2001B una vez, fundido de color y alpha |
| EnDamSphere::Ctrl_Pre | 0x942A20 | R+T parcial | traspaso de impactos al duenyo; posicion en mundo (matrices) no |
| WeakTask::Ctrl_Pre | 0x9392A0 | R+T | contador s16, estados 0/1/2 (151 fotogramas = 0x97), resultado por jugador, animacion; `func_1A2630` (cierre) no corta la funcion en el original (aqui tampoco) |
| ArmorTask::ComeOff | 0x957040 | R+T parcial | emite new_EnEtcAttach (matriz de la parte la pone el motor) |
| ArmorTask::NotDisp | 0x956FC0 | R+T | el valor de D_009AE640 entra inyectado |
| ArmorTask::PreDisp | 0x956F00 | R+T | |
| PetitZombieTask::Ctrl | 0x951C40 | R+T parcial | limites del area y giro (+-0x100/0x40); llamadas func_1A3F60/1A3F80/animacion como eventos |
| EnEtcBone::Ctrl_Pre | 0x93BF40 | R+T parcial | fisica del hueso y aleatorios (orden: rand&7, rand%2, rand&7). Las posiciones/angulos de matriz (func_1B00E0.., func_1C9290) entran en BoneInput. En la transicion 0->1 el original usa sp44 SIN inicializar (valor residual de pila): aqui entra por `partY` |
| EnVtxEffCtrl::Ctrl_Prtc | 0x93B280 | R+T parcial | vida, escala, fisica con suelo, fundido de color, etapas por D_00985430 (la etapa se incrementa ANTES de leer la tabla: con stage 4 lee la fila terminadora -1); dibujo no |
| EnVtxEffCtrl::Ctrl_Fire | 0x93BA50 | R+T | |
| EnVtxEffReqBufInit | 0x93BEF0 | R+T | |
| EnVtxEffCtrl::Ctrl / Const_Prtc / Const_Fire | 0x93B1xx.. | PEND | Ctrl recorre peticiones y llama a Const_*; no leidas |

## EnEtcThrowObj (enemies3_throw.cpp)
| Funcion original | Direccion | Estado | Notas |
|---|---|---|---|
| EnEtcThrowObj1Init | 0x93DB60 | R+T | destino = viewToWorld(+-160*-1.5/focal, 0, -1.5) o centro en coop; buffer de 17 partes como evento |
| EnEtcThrowObj2Init | 0x93EBC0 | R+T | destino aleatorio (x%240, y%168, 2 signos); tabla por tipo 5/8/9/otro |
| EnEtcThrowObj3Init | 0x93F160 | R+T | |
| EnEtcThrowObj5Init | 0x93FDC0 | R+T | temporizador ((n*5+0x50)*50)/60 SIN signo |
| EnEtcThrowObj7Init | 0x941480 | R+T | igual con 0xA4 y sonido 0x100019 |
| EnEtcThrowObj6Init | 0x9408A0 | R+T | giro +-0x1000 segun tipo == 1 |
| Ctrl_Pre (tipo 1) | 0x93D2F0 | R+T | interpola start->target en `total` fotogramas, hiere (tipo 7) y se desvanece; derribado: sonido 0xE0411, encoge |
| Ctrl2_Pre | 0x93DE60 | R+T | camara: vuelo por direccion normalizada * total/10, limites de pantalla +-320/224 (pos anterior) y +-240/168, umbral z = -1.5 - radio*escala; jugador por signo de x en pantalla; tipos 5/8 distinto sonido/efecto |
| Ctrl6_Pre | 0x93FFB0 | R+T | como 2 pero marca DEAD al borrar y suma un fallo (gp-0x7358) al golpear; sonido 0x100014, efecto 0x3A |
| Ctrl_Pos (tipo 1) | 0x93D950 | R+T parcial | contador animT 0..10, animacion, entrada en camara; el original ademas rellena la posicion en camara y llama a ColliBuff (se devuelve el booleano) |
| Ctrl2_Pos | 0x93EA00 | R+T parcial | idem sin animT |
| Ctrl6_Pos | 0x9406D0 | R+T parcial | idem + rotY += rotSpeed; colision si no 0x8000 |
| Ctrl5_Pre, Ctrl7_Pre | 0x93F230, 0x940B00 | PEND | m2c falla (jtbl); leida la 5 solo hasta ~0x93F730: usa registros float calleros no inicializados ($f21/$f22) y mula/madd, no se reescribe sin resolverlo |
| Ctrl9_Pre / Init9 / Ctrl3_*, Ctrl5/7_Pos/Put, Ctrl*_Put | - | PEND | Ctrl9 usa mula/madd + conversion a angulo (func_1B0938); los Put son dibujo |
| ctor EnEtcThrowObj (0x...) | - | PEND | m2c falla (usa t0/t1 sin inicializar) |

## Pendiente / no incluido
- EnEtcParts (Ctrl 796 instr., ctor): pelado de textura por pixeles (D_9C0D70, D_00985160/D_009851B0); EnEffTask (ctor, Ctrl_Pos, Ctrl_Put), EnEtc4ObjTask/EnEtc4Obj2Task, EnEtcSnd, EnEtcThrowObj ctor, EngJimaPut, EnAtPosGet (solo llamada a func_1CC590), EntryRankingBuffer (usa registros no inicializados), EnVtx Const_*,
  ColliCtrlTask (resto), EnVampire/EnZombie/People (otras carpetas), NpcTask (59 funciones, no es En*).
- Dibujo (Put/Disp/PreDisp de 4Obj) saltado a proposito.

## Total
Reescritas con test: **39** filas (utilidades 12, EnEtcTask 4, objetos pequenos/vertices 11, EnEtcThrowObj 12 = 6 Init + 3 Ctrl_Pre + 3 Ctrl_Pos).
De ellas 10 son parciales (solo la parte de reglas): FireBall, BranchMark, DamSphere, ComeOff, PetitZombie::Ctrl, Bone::Ctrl_Pre, Ctrl_Prtc, Ctrl_Pos x3 (son 10 con las tres Pos; Ctrl_Prtc incluido).
Reescritas sin test: 0. Pendientes: ver arriba (las mas grandes de la lista: EnEtcParts::Ctrl, EnEffTask::ctor, Ctrl7_Pre, Ctrl5_Pre, Ctrl9_Pre).

## Sin verificar (inferido)
- Nombres y significado de campos inferidos del uso en el asm; offsets en comentarios.
- func_1AF9FC = normalizar vector (inferido: aparece en nlFastInvertMatrix); func_1AFC00 = aplicar la matriz activa a un vector; func_1B00E0 = fijar la matriz activa (view = cam+0x10, inversa = cam+0x50).
- Las dos transformaciones de camara de Ctrl2_Pre/Ctrl6_Pre usan la posicion ANTERIOR para los limites de pantalla y la NUEVA solo para el umbral z (confirmado en los offsets de pila de Ctrl2_Pre; en Ctrl6_Pre solo por m2c).
- Los tests de fisica (Bone, Attach, Prtc) comprueban aritmetica contra formulas del propio codigo transcrito; la ejecucion real no se comparo.
- nlRand con valores negativos: `%` de C++ coincide con div/mfhi del original.

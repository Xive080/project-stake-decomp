# Formatos de nivel (campath, colision, CAMNOTBL, EVTPAL)

Solo ingenieria inversa de datos (no se ejecuto el juego). Herramientas: `tools/afs_campath.py`, `tools/afs_colli.py`,
`tools/afs_evtpal.py`. Cada seccion dice que prueba respalda cada afirmacion y que queda abierto.

## 1. Camino de camara (campath) y CAMNOTBL.BIN

### Quien carga que
`CamPathMgr::entryExec_STAT_00/005/01/02` (asm/CamPathMgrClass_1CBD90.s):
- Indice de ruta `p` (1..17; el ELF 0x26CB20 da `p -> ID de entrada AFS`: 916, 918, 917, 925, ... 932) -> se lee la entrada completa
  del AFS de la fase (ADXF_OpenAfs/ReadNw32) y se aliena a 64 B.
- `CAMNOTBL.BIN` (RAM 0x1FC0000): **+0x590** tabla de 20 punteros RAM (indice = ruta `p`) a listas `s16` de numeros de camara
  (fin = valor <0). **+0x5E0** `s16[696]`: numero de camara -> ruta `p`. **+0xB50** `s16[856]`: numero de camara -> duracion nominal en cuadros.
  Las listas ocupan 0x000..0x58F.
- Fichero de ruta = `u32 offs[N]`, `u32 0xFFFFFFFF`, y detras camara por camara (las curvas de cada camara, luego su registro).
  `N` = longitud de la lista de la ruta; `offs[k]` = posicion del registro de la camara `k` de la lista. La zona de curvas empieza en
  `4*(N+1)`; cada indice de curva `i` del registro apunta a `4*(N+1) + 4*i`.
- Registro (indices u32, uno por curva): `[0..2]` posicion xyz (`getCameraPath` a2), `[3..5]` destino/look-at xyz (a3), `[6]` float (fov, 0..90),
  `[7]` entero (solo con el bit 0 del flag global `-0x730C($gp)`), `[8]` curva extra no leida por el codigo mirado. Longitud 9 o 8 palabras en camaras,
  **6 palabras en trayectorias de objeto** (`getObjectPath`: 3 floats + 3 enteros cvt.w.s; todas las de las rutas 13..17, 11 y parte de 7..10).
  La longitud se deduce de que el registro termina donde empiezan las curvas de la camara siguiente (en la ultima se hereda de la anterior).
- Curva `CPFRM` (`CalcFCurve`, asm/lib_1CC410.s): `u16 n, u16 iteraciones, n x {t f32, valor f32, tangente_salida f32, tangente_entrada f32}`;
  Hermite cubico entre claves (tangentes en valor/cuadro, segmento = claves j-1 y j con la tangente de salida de j-1 y la de entrada de j),
  valor constante fuera del rango. `getStartFrame` = `t` de la primera clave, `getEndFrame` = `t` de la ultima + 0.5.

### Verificacion (afs_campath.py)
| Comprobacion | Resultado |
|---|---|
| Los 34 ficheros de ruta presentes en ST1..ST6/TRN/ORG (17 rutas distintas) | `N` = longitud de la lista de CAMNOTBL y `s16[camno]` = ruta, **34/34** |
| Las curvas + registros de cada camara teselan el fichero sin huecos ni solapes y lo que sobra es relleno a cero; todas las curvas con n>0 y tiempos crecientes | **34/34** |
| Las 694 camaras de las listas son unicas (0..693) y todas tienen `s16[camno] = ruta` | si |
| Duracion CAMNOTBL+0xB50 == tiempo de la ultima clave de la curva x | **819 de 824** camaras (cuentas repetidas entre AFS; las 5 restantes son mas largas que la ultima clave, p. ej. cam 1: 960 vs 918) |
| Evaluar la curva en el tiempo de cada clave devuelve su valor | 190 423 de 190 428 claves (las 5 restantes: saltos con dos claves de igual tiempo) |
| Tangente de entrada de claves interiores a paso 1 == diferencia central (v[j+1]-v[j-1])/2 | 60 349 claves: error relativo mediano 2e-5 (el control con signo cambiado: 1.0); 85% dentro del 10% |
| Posicion de camara dentro de la caja de colision de su fase (+25%) | ST2->colision 2: 0.99, ST4->4: 0.97, ST6: 1.0 (ver abajo); las camaras comparten unidades con la colision |

Limites: la suavidad espacial de las trayectorias por si sola es una prueba debil (hay cortes de plano reales: el 28% de las camaras tiene un salto
de mas de 10x la mediana de paso). El resto de pruebas (tangentes, teselado, listas, duraciones) es lo que sostiene el formato. No se
confirmo el significado de la palabra 9 ni de las curvas `[6]`/`[7]` mas alla de su rango (fov 0..90, mediana 42).
Se desconoce la numeracion de los `Event` que piden cada camara (viene de los scripts de EVTPAL, parcialmente decodificados).

## 2. Colision (COLLIDAT.BIN)

Gestor: `ColliMgr` (asm/ColliMgrClass_1D30F0.s), pedido por el opcode `ENTRYCOLLI n` de los scripts de EVTPAL
(`EventCtrlTask::ENTRYCOLLI` -> `EntryDatClass::entryColli`). `entryExec_Init` elige trozo y tamano con la tabla de saltos `jtbl_002C0C50` (8 casos):

| n | offset en FS/COLLIDAT.BIN | tamano | destino RAM |
|---|---|---|---|
| 0 | 0x00000 | 0x00800 | 0x1ECD000 |
| 1 | 0x00800 | 0x0A000 | 0x1ECD800 |
| 2 | 0x0A800 | 0x2C000 | 0x1ECD800 |
| 3 | 0x36800 | 0x06000 | " |
| 4 | 0x3C800 | 0x01800 | " |
| 5 | 0x3E000 | 0x09000 | " |
| 6 | 0x47000 | 0x03800 | " |
| 7 | 0x4A800 | 0x02000 | " |

La suma cubre exactamente el fichero (0x4C800 = 313 344 B). En los scripts, la etapa `s` (EVTPAL[s], s = 0..5) pide siempre `ENTRYCOLLI 0` y
`ENTRYCOLLI s+1` en todas sus escenas (patron `0x56 0x00 0x56 k`: etapa 0->1, 1->2, 2->3, 3->4, 4->5, 5->6; el trozo 7 lo piden eventos de las etapas 6 y 7 de EVTPAL, seguido de `WAIT_FLUSHDATA` y `NEXTEV`). Excepciones: la escena 35 de la etapa 1 pide 3 y los eventos 10..17 de la etapa 6 piden 1, 2, 4, 5.

Formato de un trozo: grupos hasta relleno a cero.
- grupo: `u32 tipo` (siempre 1), `u32 npoly`, AABB `f32[6]` (xmax,ymax,zmax,xmin,ymin,zmin), poligonos.
- poligono (72 B): plano `f32[4]` (nx,ny,nz,d con n.v+d=0), `u32 kind` (0, 1, 2), 4 vertices `f32[3]`, `u32 mat` (52, 51, 99, 62...; id de material/superficie sin confirmar).
  Triangulos = 4o vertice repetido (3 vertices distintos en 2 977 de 4 217 poligonos; kind 1: 2 285 de 2 661).

### Verificacion (afs_colli.py, 8 trozos, 4 217 poligonos)
- Los grupos teselan cada trozo y el resto es relleno a cero: 8/8.
- Todos los vertices dentro del AABB de su grupo: 4 217/4 217.
- Normal unitaria: 4 213/4 217 (4 degenerados); los 3 primeros vertices cumplen el plano (error < 0.05) en el 100%, error maximo 0.01. El 4o
  vertice no es coplanar en 93 cuadrilateros de kind 1 (superficies alabeadas), por eso se usa el plano con los 3 primeros.
- Volumen de etapa: posiciones de camara de la fase (seccion 1) dentro de la caja del trozo s+1 (+25%): ST1 0.99, ST2 0.99, ST3 0.93, ST4 0.97, ST5 0.83, ST6 1.0. Prueba debil: los trozos grandes tambien contienen las camaras de otras fases (ST1 tambien cabe en el 3, ST4 en el 2).
- `afs_colli.py --obj DIR` exporta cada trozo a OBJ para inspeccion visual (no revisado visualmente).

Abierto: significado de `kind` y `mat`; que usa el juego como "grupo" (probable particion espacial); el trozo 0 (2 grupos, 23 poligonos, +-5 en x) es comun a todas las etapas.

## 3. EVTPAL.BIN (scripts de evento) - decodificacion parcial

`EventCtrlTask` (asm/EventCtrlTask_1E29F0.s `__ct__`, `EventCtrl_000`; _1E2D10.s opcodes): `pc = EVTPAL[etapa][evento][fase]`, tres niveles de punteros RAM base 0x1FC2000
(listas de evento y de fase acaban en `0xFFFFFFFF`; `EVTPAL[10]`==`[11]`; `EVTPAL[12]` apunta a codigo). `NEXTEV` avanza fase/evento, el interprete lee
`opcode = *pc` y llama al handler `D_0026BC20 + 12*opcode` (ptmf que arma `__sinit_eventctrl.cc` copiando plantillas del ELF 0x26B530). Hay 448 entradas de script
(378 pc distintos, compartidos). Verificado: todos los punteros de los 3 niveles caen dentro del fichero (afs_evtpal.py).

Tabla de opcodes: handlers y argumentos tomados de los incrementos de `pc` en el asm. La correspondencia opcode->handler: 53..110 = plantilla + 4 (probado con el emulador
de `__sinit` para 57..66 y 104..110, y con los scripts: `0x36 SCNINIT, 0x5B NOWLOADINGSET 1, 0x35 CHGSM 5, 0x41 ENTRYCPBK 4, 0x44 ENTRYSPR 0x10B, 0x42 ENTRYBANK 0x17, 0x46 ENTRYMOT,
0x1B WAIT_FLUSHDATA, 0x40 ENTRYSTGDATA`, termina en `0x37 NEXTEV`); 0..5, 7, 9..18, 20..28, 33..35, 37, 40 salen del emulador de `__sinit` (solo una pasada, sin comprobar en datos); el resto sin resolver.
`tools/afs_evtpal.py` lo desensambla mientras conozca todos los opcodes: de 448 scripts, solo 24 llegan a `NEXTEV` (315 paran por handlers con argumentos de longitud variable: LIGHTCTRL, CCTB, ENTB,
ENTRYFANI/FAMOT...; 50 por el opcode 49, que no se pudo asignar).

Lo confirmado de forma independiente: `ENTRYBANK` (0x42) y `ENTRYCOLLI` (0x56) y su argumento (ver seccion 2); cada etapa carga su trozo de colision con la regla `s+1`.

Abierto: tabla completa opcode->handler (las plantillas pasan por un buffer de pila en `__sinit`, el emulador sencillo pierde parte); longitud de argumentos variables;
semantica de los argumentos (IDs de banco/movimiento/sprites, punteros a tablas de EVTPAL); relacion de la camara/campath con los `CCTB`/`CAMDR` (que numero de camara pide cada evento).

## 4. Decorado de la fase 1 (ST1): donde esta y como se coloca

Herramienta: `tools/afs_stage_export.py` (exporta a `bank.vnm` + `tex_NN.png` + `stage.json`, y renderiza una captura desde una camara de `afs_campath`).
Ejemplo: `.venv/bin/python tools/afs_stage_export.py build/stage_st1 --shot build/stage_st1/cam_r1_c12.png --route 1 --cam 12`.
Salida de referencia en `build/stage_st1/` (113 bloques colocados, 263 mallas, 79 texturas; capturas `cam_r1_c12.png`, `cam_r4_c5.png`, `viewer_orbit.png`).

### Respuesta corta
El decorado **no es el banco 58** ni los bancos de personajes: son los **bancos de objetos 175..179** (entradas AFS 348, 350, 352, 354, 356 de ST1.AFS), mas el 180 (cielo animado,
sin texturas en ST1) y piezas sueltas de los bancos 58 (fondos lejanos) y 256. Las mallas **ya estan en coordenadas de mundo** (eje Y arriba); no hay matriz de colocacion por pieza:
una **tabla de dibujo por fase** dice que bloques se dibujan en cada "estado de fondo", y el opcode `BGTB` elige el estado.

### Cadena (leida del asm y de los datos)
- `EVTPAL` etapa 0: el opcode 49 (= plantilla 45 `BGTB` + 4; el que `afs_evtpal.py` no sabia asignar) llama `BGCtrlClass::appointBG(pc)` (asm/BGCtrlClass_1C8710.s), que guarda el estado
  anterior en `+0xA` y el nuevo (`u16` en `pc+4`) en `+8`, y avanza el `pc` 8 bytes. En los scripts de la etapa 0 aparecen los estados 1, 2, 3 y 4 (`FLRTB`/`SKYKD`/`SKYDSP` = opcodes 50/51/52, 1 argumento).
- `BGCtrlClass::setBGCtrlTbl(fase)` copia a `+0xC` y `+0x10` los punteros `D_002665A0[fase]` y `D_002665D0[fase]` (ELF 0x2665A0 / 0x2665D0, un u32 por fase). La tabla de la fase 0 (ST1) tiene
  **6 estados** (0 vacio, 1..5; 52 B = 26 x s16 por estado, fin = -1) y **27 entradas** de 28 B: `s32 id_primero, s32 id_ultimo, s16 flags, s16 +0xA, f32 x, y, z, s32`.
- `PutForEntry_sub` recorre la lista del estado y para cada entrada llama `GetObjCulling` + `CheckCulling` + `PutPolyMatF(id)` para `id_primero..id_ultimo`; `id = (banco << 16) | bloque`.
  `PutForEntry_st1` hace lo mismo con una tabla fija `D_00266660` (12 entradas de 28 B) que es una version anterior/reducida de las mismas entradas de ST1 (ids 0xB2/0xB3/0xB1/0xB4 y 0x3A coinciden con las entradas 13..25 de la tabla de la fase). Cual de las dos rutas usa el juego (`lb -0x73D8($gp)`) no se resolvio.
- Con `flags == 0` (todas las entradas de 175..179) el codigo no toca la matriz: `flags & 2` (traslacion; posicion tomada de las entradas/otro objeto, sin seguir), `& 0x30` (giro con contador `+0x48`), `& 0x40`, `& 0x80` (escala) solo las usan
  las entradas 22/23 (banco 180, flags 22 y 6) y 25 (banco 58, flags 4) = cielo/fondo animado. Esto es lectura del asm, no ejecutado.

### Lo comprobado (con cuentas)
| Comprobacion | Resultado |
|---|---|
| ENTRYBANK de los scripts de la etapa 0 (barrido de palabras `66, n`) | pide 175, 176, 177, 178, 179, 180, 256 y 58; las demas etapas solo 256 y/o 58 (175..180 son exclusivos de ST1) |
| Ids de la tabla de fase | bancos 0xAF..0xB4 = 175..180 (+ 0x3A = 58 y 0x100 = 256); bloques dentro del numero de bloques de cada banco |
| Estados de fondo (`BGTB`) 1..5 | 1: 176(36 bl.) 177(29) 175(2) 58(2); 2: ademas 178(32) 179(2); 3: solo 178(31); 4: solo el banco 256 (7); 5: 176 175 178 179. El 180 (220 bloques, flags) esta en 1, 2, 3 y 5 |
| Geometria (vertices de 176..179, 20 394) | x -1674..1132, y -300..299, z -3930..781; suelo con normales int8 `ny>0` en el 58-65 % de los vertices de 176/177 y 42 % de 178 => Y arriba |
| Camaras de ST1 (335 posiciones de las rutas 1 y 4, `afs_campath`) frente al decorado | distancia al vertice mas cercano: mediana **20**, p90 184; puntos aleatorios en la misma caja: mediana 152; decorado desplazado 1500 en x: 521; decorado reflejado en z: 44 (misma escala de unidades que las camaras y la colision, y orientacion de z coherente) |
| Camara de la ruta 1, camara 12 (y -238, z -1482) | la captura muestra una calle nevada con muros de piedra, escalera, un abeto y el cielo naranja: es un escenario reconocible (`build/stage_st1/cam_r1_c12.png`) |

### Texturas (hallazgo aparte, comprobado por consistencia)
- Los indices de textura de los materiales (`header.tex`, `+0x10`) de un banco N indexan los registros de **OBJTBL del fichero `tex` que precede a su objeto en el AFS (entrada obj-1)**:
  175->347, 176->349, 177->351, 178->353, 179->355, 180->357. La tabla ELF 0x26C6B0 da para estos bancos obj+1 (349, 351, ...) y el puntero por banco de OBJTBL (cola, +0x83FA4) da el fichero propio;
  ninguna de las dos sirve. Prueba: para cada banco, el rango de indices usados cabe exactamente en el fichero obj-1 (176 usa hasta 37 de 39 subidas; 178 hasta 45 de 46; 177 hasta 19 de 21) y la
  coherencia modo<->tipo de registro (material -1 = color directo, -2 = T4/T8 con paleta en el registro siguiente) da 795/795 (178), 235/235 (177), 160/160 (176) con ese fichero y 26/235, 307/795, 77/160 con el propio.
  Para el banco 124 (obj 247, tex 246) la regla obj-1 coincide con la tabla; por tanto **la tabla 0x26C6B0 solo es fiable donde coincide con obj-1** (nota: el banco 172 exportado por `vnm_export.py` usa `tex 343` = obj+1; con la regla seria 341: no se reviso si eso explica la textura "rara" de la cara).
- Paletas T4/T8 dentro del fichero: el registro siguiente (kind 2) es la paleta y **se lee como imagen en raster** (entrada i = pixel i; T8 con la permutacion CSM1 `(i&0xe7)|((i&8)<<1)|((i&0x10)>>1)`);
  leerla como palabras lineales (`gs_vram.clut_rgba`) da imagenes mucho mas ruidosas (aspereza media 12-212 frente a 5-56 en unas 55 texturas medidas).

### No comprobado / abierto
- Que estado de fondo (`BGTB` 1..4) corresponde a cada escena/camara: solo se vio que los argumentos son 1..4; el exportador une los estados 1..5 (`--bg all`), por lo que en la union pueden solaparse piezas
  alternativas. `--bg N` exporta un estado.
- Lateralidad (espejo en x): la prueba de reflejo en z favorece la orientacion usada, pero no hay texto/rotulo en el decorado para confirmar la x. El renderizado de camara usa `fov` del registro como vertical (sin confirmar).
- Cielo (banco 180, 260 bloques casi todos de 544 B, sin fichero de texturas en ST1) y su animacion (traslacion con la camara + giro): solo leido del asm; `--sky` lo incluye sin transformacion. Los fondos lejanos del banco 58 estan en z = -4626 (planos de 2500 x 2900); el banco 256 (props de +-100 alrededor del origen, estado 4) no se identifico.
- Sin aplicar: colores de vertice (los del disco son relleno 0x80), iluminacion de VU1, mezcla/alfa de los registros GS (solo se descartan texels con alfa 0), niebla (`FLRTB`), bloques de 544 B (cuadrilateros sueltos, p. ej. 178 bloques 0..39 con textura 12) tal cual.
- No hay un modo de camara en `vn_viewer` (solo orbita): la captura desde camara de ruta sale del renderizador por CPU de `afs_stage_export.py` (numpy, lento: ~1-2 s por cuadro de 960x540), no del visor nativo. `vn_viewer` corta a 5000 unidades: usar `--scale 0.5` y `--skip-banks 175,58,256` para encuadrar (`viewer_orbit.png`).
- Otras fases (ST2..ST6): `bg_tables(stage)` lee las tablas por fase del ELF (hay 8 en D_002665A0), pero solo se probo la fase 0; las rutas de texturas por banco (obj-1) y las colisiones no se contrastaron con ellas.
- La colision (seccion 2, trozo 1 de ST1: x -371..269, z -1718..136) cubre solo la parte central del decorado (176/177, z -1123..781); la calle de 178 llega a z -3930.

> Correccion (2026-11-09, ver docs/EMULADOR.md): en ST1 el juego dibuja el fondo con la tabla fija de 12 entradas de ELF 0x266660 (`PutForEntry_st1`, porque gp-0x73D8 == 0), no con las listas de estado BGTB; los bancos 175 y 176 no se dibujan en esa ruta. La ruta de camara activa de la demo es la 4 (no la 1). `afs_stage_export.py --bg st1` replica esa tabla.

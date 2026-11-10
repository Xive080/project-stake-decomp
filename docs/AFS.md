# FS/*.AFS (Vampire Night PAL)

Contenedor CRI AFS: `AFS\0`, u32 count, tabla (offset,size)[count]. Tabla de slots dispersa
(~933, la mayoria size 0 = espacio de IDs global), sin tabla de nombres. Hay 17 AFS con datos
(COMMON, ORG, HUNTER, ST1-6, TRN, TUT, TITLE, LOGO, RATING, RANK, ADV, CSOPTION).

## Tipos de entrada (tools/afs_classify.py; 2470 de 2506 entradas clasificadas)
| Clase | Nº | Descripcion |
|---|---|---|
| tex | 1700 | Cadena DMA GS (ver abajo), 324 MB |
| lz | 648 | Contenedor LZSS: `u32 total_expandido, u32 n, n x (offset, tam_expandido)`, bloques comprimidos con afs_lz; el tam. declarado va redondeado a 32 B. Cada bloque expandido empieza con `u32 0x1000, u32 tam, u32 cuenta...` (geometria/datos de objeto, formato interno sin decodificar) |
| lz1 | 286 | Un solo bloque: `u32 a, u32 b, u32 tam_expandido` + LZSS desde 0xC |
| otbl | 241 | Tabla de offsets absolutos sin cuenta (el primero = tamaño de la tabla), subficheros sin comprimir (floats: claves de animacion/movimiento probable) |
| adx | 51 | Audio ADX (CRI) en streaming, 193 MB (sobre todo ST*/TRN, indices >= 738) |
| raw | 8 | `u32 n, u32 tam_fichero, u32 0` + floats |
| otro | 36 | Sin clasificar (5 MB) |

## LZSS (tools/afs_lz.py)
Es la rutina del juego `func_001E5490` (EvCancelTask_1E51A0.s), que descomprime byte a byte con estado en D_46A800.
Bits de control LSB primero: `1` literal; `0,1` copia larga (u16: bits 15..3 = offset-0x2000, bits 2..0 = longitud;
0 = fin de bloque; si longitud 0 un tercer byte n = total n+1, si no total = longitud+2); `0,0` copia corta
(2 bits = total-2, un byte = offset-0x100).

## Texturas
u32 al inicio = inicio de datos; cadena DMA desde 0x10: tags cnt/id1 con paquetes GIF A+D (0x50 BITBLTBUF:
DBP/DBW/DPSM; 0x52 TRXREG: w,h) y tags ref/id3 con los datos consecutivos desde la region de datos; id7 = fin.
Formatos GS: CT32, CT16, T8 (CLUT CSM1 con intercambio de bloques), T4.
Las subidas llevan la direccion de VRAM (DBP), es decir el fichero es una instantanea de VRAM; los TEX0 no estan en los
AFS (los arma el codigo), asi que la paleta de cada T8/T4 se empareja eligiendo, entre las paletas del mismo fichero
(CT32 16x16 para T8; CT32/CT16 8x2 para T4), la que da menor variacion de color entre pixeles vecinos
(afs_textures.py). Las imagenes T8/T4 de tira estrecha (8x256, 16x256) son rampas reales, no un fallo.

## Herramientas
afs.py (list/types/extract), gs_stream.py, afs_textures.py (`FILE.AFS INDEX|all [OUT]`), afs_lz.py,
afs_unpack.py (parse_container), afs_classify.py.

## Significado de los IDs (tools/afs_ids.py)
El ID de entrada = indice de slot; se reutiliza entre ST1..ST6 (cada fase trae su banco). Los gestores del juego traducen
un indice de banco a ID con tablas s16 del ELF y llaman `ADXF_OpenAfs(0, id)`:
| Tabla ELF | Gestor | Clase de entrada |
|---|---|---|
| 0x26C490 (279 bancos) | PolyMgr::entryBankObj | objeto = contenedor `lz` (262/279 verificados; 11 apuntan a `tex`, 6 vacios) |
| 0x26C6B0 | TexMgr::entryBank | textura del banco anterior = `tex` (273/279) |
| 0x26CA60 | MotMgr | movimiento = `otbl` (90/96) |
| 0x26CB20 | CamPathMgr | rutas de camara = `otro` |
| 0x26CB50 | OneSkinMgr::entryOskBank | `lz1` y `tex` (nombres/bancos OneSkin) |
| 0x26E880 / 0x26F6C0 | OneSkinMgr::entryBodyNo | pares (cuerpo, 168), `lz`/`tex` (semantica a medias) |
Sin tabla: IDs 547..737 son texturas de sprites pedidas directamente; 738..769 son ADX (BGM/voces de fase).
Los TBL/*.BIN (OBJTBL, MOTTBL, EVTPAL, DATA00, CAMNOTBL) son otras estructuras cargadas a RAM fija (0x1F2xxxx..0x1FCxxxx).

## Bloques de objeto (tools/ps2_model.py)
Bloque expandido = lista de display de PS2 para VU1: `u32 0x1000, u32 tam, u32 cuenta, ..., u32 ntags @0x2c`, y desde 0x30
`ntags` tags DMA de 16 B (`0x3000_0000|qwc`, direccion RELATIVA al bloque, w2/w3 = VIF). Verificado en 39 152 de 39 233 bloques
(los demas tienen >512 tags). Cada tag apunta a un flujo VIF: STROW, UNPACK (posiciones V3-32, normales/colores V4-8,
UV V2-32, S-32), STCYCL, MSCAL y DIRECT. Los tags con direccion 0 y DIRECT son huecos de paquete GIF que el motor rellena
en ejecucion con TEX0 (nlObjPutUpdateTexInfoPoly2): por eso los TEX0 no estan en disco.

## Mallas (tools/afs_mesh.py) - verificado visualmente
Familia dominante (51 755 paquetes en COMMON/ST1/ORG/HUNTER/ADV): `UNPACK S-32@0 (4 escalares: n_vertices, flag 0x41.., dos constantes GIF),
STCYCL 0x105 (cl=5, wl=1: los vertices quedan intercalados cada 5 quads en VU mem), STMASK, UNPACK V3-32@4 (posicion float3),
V4-8@5 (normal int8), V4-8@6 (color RGBA u8), V2-32@7 (UV float2)`. Se interpreta como tira de triangulos de n vertices:
`afs_mesh.py FILE.AFS ENTRADA BLOQUE out.obj|out.png`. Validado: caras de personaje (varios LOD) y un cuerpo completo en T-pose
(ST1 entradas 342, 338, 334, 247...). La malla mas grande tiene 2442 vertices.
Otra familia (7 210 paquetes: `V4-32@0, S-8@4, V2-32@4, V4-8@16`) sin decodificar (probable geometria con huesos/pesos o efectos).
El microcodigo VU1 esta en el ELF 0x1B1C88..0x1C5A20 (8 programas, mal desensamblados como MIPS; se reconocen por la NOP
inferior 0x8000033C). No hizo falta desensamblarlo para las mallas estaticas; si hara falta para la familia sin decodificar.
No se comprobaron los bits ADC (rupturas de tira) ni los indices de textura (el flag 0x41/0x42/0x43 podria elegir material).

## Texturas y exportacion de bancos (tools/gs_vram.py, tools/afs_export.py, tools/obj_render.py)
El fichero `tex` de un banco es una cadena GIF (subidas a VRAM) sin TEX0. Se simula la VRAM del GS (`gs_vram.py`: paginas/bloques/columnas
de CT32/CT16/T8/T4, permutacion CSM1 de paletas) y los descriptores (psm, log2 ancho/alto, tbp, kind) salen de `TBL/OBJTBL.BIN`:
17 440 registros de 32 B; el banco se localiza buscando la secuencia de `dbp` de las subidas = secuencia de `tbp` de los registros
(`psm = r[0]>>16`, tw/th = log2 en `+8`, `+0xC` = desplazamiento en bytes dentro de los datos, `+0x10` = tbp, `+0x18` = kind).
Los materiales de un bloque (`+0x10` = indice de textura, `+0xC` = modo/CLUT) enlazan paquete -> textura.
`afs_export.py AFS BANCO` escribe `build/export/<AFS>_bank<N>/obj_NNN.obj`, `bank.mtl`, `tex_NN.png`; `obj_render.py` renderiza con z-buffer.
Estado: la geometria y los UV son correctos (banco 124 = criatura en T-pose, 172 = cabeza y cuerpo). Las texturas CT16/CT32 salen bien.
**Paletas (resuelto)**: no van en el banco. El campo `+0xC` del material es un slot de la tabla global de 154 paletas que `PolyMgr::init1st2`
carga al arrancar desde el ELF 0x28BA10 (8 B: ptr, ncolores u16, flag u16; flag 1 = CT16, 8 = CT32; datos en orden de bloque GS, se aplica la
permutacion CSM1 al leer) via `PalMgr::initPal` -> `nlSetPalette`; `nlObjPutUpdateTexInfoPoly2` copia la CBP/CPSM del slot al TEX0 (`lw -0x7530($gp)`,
12 B por slot). Modo -1 = sin paleta (CT16/CT32). `afs_export.elf_palette` lo implementa. Banco 124 (slot 4) sale en color correcto.
Banco 172 (slot 81; 83 y 90 en los materiales 16-17): las 16 texturas se leen con el mismo metodo. La textura de la cara (64x64 en 0x180, 132 de
las 189 caras de la cabeza) es un atlas simetrico de baja resolucion con bandas de color: se ve "raro" pero la estructura es coherente
(linea de la nariz, nacimiento del pelo) y no es un error de direccionamiento (probado: tbw 1/2/4/8, lineal, T4, otras dimensiones).
Verificacion adicional: la rugosidad de la imagen de indices es igual en las T8 buenas (0-3) y en las "raras" (4-15), y los indices de cada textura
ocupan pocos tramos de la paleta 81, que es una paleta compartida de ~170 colores usados (cara, pelo, ropa); por eso cada textura tiene solo
30-130 colores y se ve en mosaico. Una paleta mas "suave" por textura (busqueda entre las 154 globales) no da resultados fiables: las paletas planas puntuan 0.

## Movimientos (tools/afs_motion.py) - verificado por suavidad entre cuadros
`MotMgr::getMotAnmAdr(motion, tipo_cuerpo, cuadro)` = base[motion] + cuadro * ((6n+15)&~3), con n = s16 de la tabla de DATA00.BIN +0xF860
(indexada por tipo de cuerpo; 18 para el humano, 5/10/16/17/19/20 otros). Cuadro = 12 B de traslacion de la raiz (3 floats) + n x 3 s16 de
angulos (65536 = 360 grados) + relleno. Los bancos `mot` (96, otbl: 548 B de tabla + subficheros) contienen un movimiento por subfichero, con
cola de 12-76 B tras los cuadros. Carga en RAM de los TBL (tabla en ELF 0x26B2FC): EVTPAL 0x1FC2000, MOTTBL 0x1FB3000, OBJTBL 0x1F2A000,
CAMNOTBL 0x1FC0000, DATA00 0x1F1A000. MOTTBL.BIN +0: u16 por indice de movimiento = (banco mot << 9) | subfichero; luego tablas de duraciones.
Algunos subficheros pequenos (p. ej. ST1 785, 789) no son cuadros de esqueleto (otro tipo: camara/cara/efectos), sin decodificar.
Las Osk (lz1) no son el esqueleto sino la union piel-huesos (lista de huesos por parte y pesos); la jerarquia y las longitudes de hueso siguen sin localizar.

## Esqueleto (tools/afs_skeleton.py) - verificado con una figura de palos
Arbol de partes (BODYPT) en `DATA00.BIN`: tabla de raices por tipo de cuerpo en +0xF730 (punteros RAM, base 0x1F1A000); cada nodo: id u32 en +0,
indice de parte s16 en +0x20, numero de hijos s16 en +0x22, punteros a hijos desde +0x24 (73 arboles; 18 partes en el humano). Desplazamiento local
de cada parte respecto a su padre = registro de 20 B `{id, x, y, z, escala}` numero (parte-1) del array que apunta la tabla D_9B5580[tipo] de
GAMEMOD.BIN (0x9B5580; `partsInit_rcv`, `putMat_rcv`); la parte 0 no tiene desplazamiento (traslacion de la raiz = los 12 B de cada cuadro).
Humano (tipo 0): 0 raiz -> 1 pelvis (0,2.7,-0.1) -> 2 (columna), 3->4->5->6 pierna izq., 7->8->9->10 pierna der.; 0 -> 11 (torso) -> 12->13->14 brazo izq.,
15->16->17 brazo der. Con los angulos del movimiento ST1 776/0 sale un humanoide bien formado; el orden de Euler (ZYX segun GetRotZYX/BodyMgr::init,
que tiene 5 variantes) y los signos no estan confirmados (las tres pruebas se ven iguales en cuadros distintos).
Pendiente: asociar cada bloque de malla de un banco con su parte (MotObjTask::prtsDispFunc / tabla de partes) y posar el OBJ.

## Partes de cuerpo y piel (PutPolyMat, Osk)
El `id` de cada nodo BODYPT (u32 en +0) es `(slot de banco << 16) | bloque`: `PutPolyMat(id)` busca el slot en la tabla D_501430 (16 B, bit31 = cargado,
+4 = puntero a registros de 12 B, uno por bloque del objeto) y dibuja `nlObjPut(bloque)`. `0x1000e` (slot 1, bloque 14) se repite en las partes sin
geometria propia. Hay dos estilos: cuerpos rigidos (una malla por hueso: tipos 1/5, ids 0x7b00xx / 0x7f00xx) y "OneSkin" (tipo 0: una unica malla
`0x7a0001` con pesos). El banco ST1 124 es OneSkin: el bloque 0 es el cuerpo entero en T-pose (2042 vertices, 22 x 20 unidades) y los bloques 1-34
son manos/dedos en coordenadas locales (pares izquierdo/derecho espejados). La escala del esqueleto no coincide con la de la malla (esqueleto ~4.7 de
pierna frente a ~10 en la malla): falta el factor (probablemente el campo escala w del registro o 1CA0 de MotObjTask).
Osk (lz1) = pesos de piel: cabecera `n`, lista de n huesos por parte y, por parte, registros de vertices `{hueso, peso float}` terminados con
`{0xFFFFFFFF, 0}` (p. ej. `(4, 0.9) (0, 0.1)`), separados por cabeceras VIF (`0x14000004`, `0x6cXX8080`). ST1 864/866/873 tienen ~1000-1350 vertices cada una.
No se ha emparejado aun cada Osk con su banco de cuerpo (OneSkinMgr::entryOskBank).

### Numero de cuerpo, banco y Osk (OneSkinMgr::entryBodyNo/entryOskBank)
El numero del banco de objetos (el de `afs_export.py`, tabla ELF 0x26C490) es el `slot` de los `id` de BODYPT: `id>>16 == banco` (0x7c = 124).
Tabla `body_a` (ELF 0x26E880, 200 x u32) = `(banco << 16) | bloque` por numero de cuerpo (p. ej. cuerpos 86..95 -> banco 124, bloques 2-9, 0x19, 0x1a...);
`body2` (0x26F6C0, u32) = numero de Osk de ese cuerpo; Osk numero n -> id AFS en la tabla 0x26CB50 (s16): 0 -> 864, 1 -> 865, 6 -> 870.
Los tipos de cuerpo (DATA00) que usan el banco 124 son el tipo 2 (id 0x7c0021 = bloque 33). Consecuencia: los Osk pequenos (p. ej. 870, 4 partes, ~150-300
vertices por parte) son pesos de *suavizado de articulaciones* de cada bloque-miembro (pares hueso/peso 0.9/0.1), y el bloque 0 de 2042 vertices
(cuerpo entero en T-pose) es el modelo estatico/de baja calidad, no la malla que se deforma. Sin verificar: relacion numero de cuerpo -> tipo de cuerpo.

### Banco 122, Osk 868 y formato de pesos (lectura de OneSkinMgr)
El banco 122 (entradas 242/243) solo esta en ST5.AFS y TRN.AFS (vacio en el resto). Solo el tipo de cuerpo 0 (humano OneSkin, n=18) lo usa en el esqueleto:
hueso 2 = `0x7a0001` (bloque 1, 826 vertices, cuerpo entero); bloque 2 = 392 vertices; los bloques 3..27 son piezas intercambiables que `body_a` (cuerpos 53..76)
asigna al numero de Osk 4 (Osk 868, 724 B, solo ~20 vertices ponderados). Los nombres `changeLowModel/NormalModel/EndingModel/ShadowModel` de OneSkinMgr
indican que un cuerpo tiene varios modelos (baja/normal/final/sombra): el bloque 0 del banco 124 es uno de ellos, no necesariamente "el estatico".
`weight100_cut` (asm 0x1FEBF0) revela el formato de Osk: `u32 n; u32 hueso[n]`, relleno a `(n+4)&~3` palabras, y despues grupos
`{u32 v1 (qwords de datos), u32 nverts, ...}` + v1 qwords de pares `{indice u32, peso f32}` terminados en -1 por vertice; los pares de peso 1.0 se marcan
con -2 (100 % = rigido a un solo hueso). Los indices de los pares (0, 4...) no son ids de hueso directos: indexan la lista `hueso[n]` o una paleta de matrices; sin resolver.
Hipotesis a probar: los Osk 864/866/873 (1000-1350 vertices) corresponden a los cuerpos de banco 124 (enemigos), el 868 solo a recambios del jugador/ST5.

### Ensamblado del cuerpo por piezas (tabla ELF 0x273CC0) - corrige lo anterior
`data_set` (OneSkinMgr) usa la tabla de punteros 0x273CC0 (por tipo de cuerpo) con registros de 16 B `{numero de cuerpo, 0, parte, lista}` (algunas partes llevan una lista
de huesos marcados con el bit 0x4000). Tipo 0, primer conjunto: cuerpos 65/68/70/56/58/53/72/74/60/62 -> partes 1/4/5/8/9/11/12/13/15/16;
`body_a[cuerpo]` da (banco, bloque): banco 122 bloques 17/20/22/7/9/4/24/26/11/13 (ST5.AFS entrada 243). El cuerpo OneSkin NO es una sola malla: son piezas por parte
(`tools/afs_assemble_body.py`). Las piezas estan en espacio de modelo con T-pose (brazos horizontales): los centroides de las piernas coinciden con las articulaciones del
esqueleto a escala 1 (no hay factor de escala), pero el esqueleto de DATA00/GAMEMOD tiene los brazos colgando (reposo distinto de la T-pose). Con un movimiento real (ST1 776/0)
los brazos salen levantados y hay un hueco torso/cadera: falta la pose de referencia (bind) o la convencion de rotacion; la metrica de plausibilidad probada no distingue ordenes.
Correccion: partes 4-6/8-10 = brazos, 12-14/15-17 = piernas (la nota anterior los daba al reves); 11 = cadera, 1 = torso.

## Pendiente
- Asociar bloque de malla <-> parte del esqueleto y posar/animar un OBJ exportado; fijar el orden de Euler.
- Banco 172: materiales 16-17 (slots 83/90) apuntan a texturas fuera del banco (probable banco comun de ojos); comprobar la cara con la animacion/los ojos.
- Familia V4-32@0/S-8@4 (huesos/pesos?) y desensamblar el microcodigo VU1; bits ADC y relacion flag -> textura (TEX0 en ejecucion).
- Formato interno de las otbl (movimientos: clave probable para animar), lz1 (Osk) y campath.
- Tablas body (0x26E880/0x26F6C0) y los 11 bancos de objeto que apuntan a `tex`.
- Las 36 entradas `otro` restantes; T8 swizzled subido como CT32 (COMMON[0] #069).
- Extraer FS/SND, MVI (SFD), ADX, FANI.BIN.

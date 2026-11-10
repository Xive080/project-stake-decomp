# Render de bloques de objeto: cabeceras de lote, microcodigo VU1, ADC y flags

Herramientas nuevas: `tools/vu1_dis.py` (desensamblador VU1 minimo), `tools/afs_mesh2.py` (lotes cabecera + vertices, comprobaciones, render).
No se ha tocado nada existente. Todo lo marcado "verificado" tiene una cuenta o prueba detras (comando al final); lo demas es hipotesis.

## Microcodigo VU1 (tools/vu1_dis.py)
- La subida la describe una tabla del ELF: `0x1B1A40` = 5 punteros a descriptores; cada descriptor = 8 x `{vaddr, destino en micro-mem, 0x800}` + `0xFFFFFFFF`.
  Hay **5 programas de 16 KB** (2048 instr, 8 trozos de 256 instr), en `0x1B1C80, 0x1B5C80, 0x1B9C80, 0x1BDC80, 0x1C1C80` (no 8: el rango
  0x1B1C88..0x1C5A20 de AFS.md empieza 8 B tarde y los 5 programas ocupan hasta 0x1C5C80). Los 5 comparten el mismo mapa de entradas
  (variantes; no se ha determinado en que difieren ni cual elige el motor).
- Instruccion = 8 B LE: palabra baja = lower, alta = upper. NOP lower = `0x8000033C`, NOP upper = `0x000002FF`.
- Tabla de entradas (`pc` en unidades de 8 B; MSCAL imm = pc): `pc0 b` / `pc2 b` / `pc4 b` / `pc8 b`; las entradas 6 y 10-16 saltan a direcciones negativas (sin usar).
  Los paquetes del disco solo usan **MSCAL 0, 2 y 4** (24 222 x MSCAL 4, 5 867 x MSCAL 2, 2 340 x MSCAL 0 en ST1+COMMON); MSCAL 8 (preparacion de
  fotograma: matrices en vf16-19 y vf28-31, constantes en mem 24..27) lo envia el motor, no el disco.
- Mapa de memoria VU1 observado (programa 0): TOPS = datos del paquete; 996.z/997.xyzw/998.xyzw = estado entre llamadas (culling, flags de pipeline);
  1000..1009 = matrices/constantes; 1012.y, 1017.y, 1018.y = doble buffer de salida XGKICK (`0x400` + otra base); 1018..1023 = tabla pequena de modos;
  mem 24..30 = constantes de material/luz; 31.x = flag de vertices, 31.y = flags_estado de la cabecera, 31.w / 34.xyzw = rutinas de pipeline elegidas.
- MSCAL 0 (`pc 0 -> 77`): `xtop; lq vf01,0(TOPS)` = esfera (cx,cy,cz,r); la transforma con la matriz de fotograma, hace `clipw` y guarda en 997.y si
  esta fuera del volumen (los paquetes siguientes salen con `ibeq 997 -> pc 325` sin dibujar). Verificado leyendo el codigo (pc 77..84, 109..145).
- MSCAL 2 (`pc 2 -> 86`, cabecera): culling con la esfera del quad 1; cuenta bits de `flags_estado & 0x300` (998.z); escoge las rutinas de
  iluminacion/pipeline segun `flags_estado` (bits 0x1, 0x2, 0x4/0x8/0x10 (=0x1c), 0x20, 0x41, 0x800, 0x1000; pc 163..277) y las guarda en mem 34/31;
  copia los quads TOPS+3..+12 al buffer de salida (`sqi ... (vi12++)`, pc 291..310: etiqueta A+D + 9 registros A+D) y guarda la plantilla de
  etiqueta de vertices (quad 2) en mem 32/33.
- MSCAL 4 (`pc 4 -> 18`, vertices): lee n (quad 0), flag (quad 1), calcula `996.z = TOPS+4+5*n` (fin de datos, 5 quads por vertice), transforma
  posiciones (matriz 4x4 vf16-19), rota normales (itof12 -> /4096 con int8, `vf13-15`), aplica iluminacion (rutinas `bal 393/496/563/960`), reescribe
  los 5 quads de cada vertice in situ y finalmente genera el paquete GIF (ftoi4/ftoi0/ftoi12, `xgkick`, pc 1700..1760 es el bucle sin recorte).

## Segunda familia (7 210 paquetes segun AFS.md; 15 842 contando COMMON/ST1/ORG/HUNTER/ADV): NO es geometria, es la cabecera de estado del lote
Forma fija (siempre las mismas cuentas: 4 / 12 / 12 / 6): `V4-32@0 x4, S-8@4 x12, STMASK, V2-32@4 x12, V4-8@16 x6, MSCAL 2`. Se apoya en un paquete
previo `STROW + V4-32@0 x1 + MSCAL 0` (la misma esfera, para el culling). Cada cabecera abre un lote: le siguen 1 o mas paquetes de vertices con la misma cabecera.

| Quad | Contenido (decodificado) |
|---|---|
| 0 | `x` = flags de cabecera (`0x8000000n`; bit 0 ~ textura presente, bit 1 sin determinar), `y` = flags_estado (0,1,2,0x10,0xc,0x20,..., los lee el microcodigo), **`z` = slot de paleta CLUT global (= campo +0xC del registro de material, -1/-2 = sin paleta), `w` = indice de textura (= campo +0x10)** |
| 1 | esfera envolvente (cx, cy, cz, r) en coordenadas de objeto |
| 2 | plantilla de etiqueta GIF de los vertices: `NREG=3, EOP=1, regs = ST, RGBAQ, XYZF2` (`0x8000 / 0 / 0x412 / 0`); el motor/VU inyecta el PRIM |
| 3 | etiqueta GIF A+D (`EOP=1, NLOOP=9, NREG=1, A+D`) |
| 4..12 | 9 registros A+D: dato 64 bit = V2-32 (x=bajo, y=alto), direccion = S-8: **TEX0_1 (0x06), TEX1_1 (0x14), ALPHA_1 (0x42), TEXA (0x3B), TEST_1 (0x47), MIPTBP1_1 (0x34), MIPTBP2_1 (0x36), PABE (0x49), CLAMP_1 (0x08)**; los quads 13..15 son relleno (direccion 0) |
| 16..21 | 6 x V4-8 de parametros de material/luz; los quads 16-18 se convierten con `itof0` y se multiplican por constantes de fotograma (mem 26/27) y van a mem 28/29/30. Semantica fina sin decodificar |

Valores reales de los registros (todas las cabeceras, ST1+COMMON+ORG): TEX0 `hi=0x4` (TCC=1, TFX=MODULATE; `0x1c` = TFX 3; TBP/PSM/CBP = 0, los parchea
`nlObjPutUpdateTexInfoPoly2`), TEX1 `0x60` (MMAG/MMIN lineal), ALPHA `0x2a` opaco / `0x44` mezcla (Cs-Cd)*As+Cd / `0x48` aditiva Cs*As+Cd / `0x68`+FIX 0x80 aditiva
fija, TEXA `TA1=0x80`, TEST `0x5000d` (alfa>0 y Z GEQUAL) / `0x53001` (ATST NEVER, AFAIL=RGB_ONLY) / `0x30000`, CLAMP `5` (clamp S y T) / `0`/`4`/`1`, MIPTBP/PABE 0.
`afs_mesh2.decode_gs` los decodifica; `blend_class(header)` da opaco / alfa / aditivo.

Por que se ve como un "paquete de geometria raro": el UNPACK S-8 + V2-32 con STMASK intercala direccion (z,w) y dato (x,y) en el mismo quad de VU mem
y VIF lo escribe de una vez; es un GIF A+D empaquetado con UNPACK.

### Verificacion (afs_mesh2.py --check, ST1+COMMON+ORG+HUNTER+ADV)
- 15 842 cabeceras, **15 842** con las 9 direcciones A+D esperadas (en ese orden) y con >= 1 paquete de vertices.
- 115 578 paquetes de vertices (835 643 vertices): **todos los vertices caen dentro de la esfera de su cabecera** (835 643/835 643) y la esfera es ajustada:
  max(distancia)/radio por paquete tiene mediana 0.87, p90 y maximo 1.000. Una esfera ajena no cumpliria esto.
- `q0.w == indice de textura` y `q0.z == modo/paleta` del registro de material que contiene el paquete: 7 346/7 346 donde `ps2_model.materials` localiza el registro
  (las otras ~8 500 cabeceras estan en bloques sin registro localizable por ese metodo: sin verificar).
- Render: `afs_mesh2.py disc/FS/ST1.AFS 247 0 out.png` (banco 124, un lote por material, 7 lotes, 2 042 vertices) da la criatura en T-pose con normales int8 suaves;
  el OBJ separa un material por lote (`batches.mtl`, con clase de mezcla y bandera de clamp en el nombre/cabecera).

## Tira y bits ADC
- Cada paquete de vertices es **una tira de triangulos** y esta confirmado por el propio disco: la etiqueta GIF que traen (S-32 q2) tiene `PRIM = (w>>15)&0x7ff`, tipo 4 (TRISTRIP)
  en 115 578/115 578 paquetes; la palabra de q3 es la misma con tipo 5 (TRIFAN) en 115 578/115 578 (el microcodigo la usa para la salida recortada en abanico).
  PRIM trae IIP (bit 3), TME (4), FGE (5), ABE (6): p. ej. `0x301e4000` -> `0x3c` (gouraud + textura + niebla), `0x303e4000` -> `0x7c` (+ mezcla), `0x301a4000` -> `0x34` (plano).
- **Los bits ADC no estan en los datos**: los genera el microcodigo. En el bucle de salida (pc ~1712..1749) cada vertice hace `clipw` y, tras `fcand vi01,0x3ffff`
  (flags de recorte de los 3 ultimos vertices), `ior vi03,vi04,0x8000` (ADC=1, no dibujar) o `iand vi03,vi04,0x7fff` (ADC=0) y `isw.w vi03,0(vi12)`; antes de empezar la tira
  `fcset 0xffffc0` (historia de recorte "todo fuera") hace que los dos primeros vertices salgan con ADC=1, como en cualquier tira. El campo F (bits 4..11 de ese w) sale de
  `ftoi12(0.996)` = niebla ~0xFE por defecto. Consecuencia: no hay que interpretar ADC en el disco; dibujar las n-2 caras de la tira es correcto, y se comprobo que
  **no hay triangulos degenerados** (0 de 604 487) que actuen de ruptura de tira.
- No hay reinicios de tira dentro de un paquete: una tira por paquete (n vertices, n-2 triangulos).

## Flag 0x41/0x42/0x43 (S-32 q1 del paquete de vertices)
- **Bit 6 (0x40) = Gouraud (PRIM.IIP)**: `flag&0x40 == PRIM.IIP` en 115 578/115 578. En el microcodigo, `flag&0x40` decide si se interpola el color por vertice o
  se usa el del primer vertice (pc 1251-1258, 1537-1544).
- Bits 0-1: el microcodigo hace `vi10 = mem[1020 + (flag&3)].y` (pc 34) y lo usa como signo/contador de rutinas de recorte/tira (`ibgez vi10`, pc 466, 982, 989, 1336).
  Los valores de esa tabla (1020..1023) se cargan desde el motor y **no se han localizado**; el significado de 1/2/3 no esta resuelto.
  Lo descartado por datos: no es topologia (tira gana en 41/42/43 por igual al medir el perimetro tira vs abanico vs lista) ni cambia la estructura de UV/normales (normales unidad en todos;
  `col` V4-8@6 es siempre `0x80808080` = relleno que VU1 sobrescribe, no color). Distribucion: 0x41 = 56 k paquetes, 0x42 = 5.2 k, 0x43 = 0.7 k, 1 y 2 (sin Gouraud) = 1 k.
- `q0.x` de la cabecera (`0x80000001`/`3`) y `flags_estado` (`q0.y`) tampoco estan resueltos bit a bit; lo que se sabe esta en la tabla de arriba y en la descripcion de MSCAL 2.

## Reproducir
```
.venv/bin/python tools/vu1_dis.py --list            # los 5 programas
.venv/bin/python tools/vu1_dis.py --prog 0          # desensamblado (pc, vaddr, upper | lower)
.venv/bin/python tools/afs_mesh2.py --check disc/FS/ST1.AFS disc/FS/COMMON.AFS disc/FS/ORG.AFS disc/FS/HUNTER.AFS disc/FS/ADV.AFS
.venv/bin/python tools/afs_mesh2.py disc/FS/ST1.AFS 247 0 build/mesh2/out.png
```

## Abierto
- Semantica de `flag&3` y de los bits de `flags_estado`/`q0.x`; tabla VU mem 1018..1023 (la carga el motor en `nlObjPut*`, no localizada).
- Quads 16..21 (V4-8) y como se calculan las constantes mem 24..30 (iluminacion).
- En que se diferencian los 5 programas VU1 y cual usa cada tipo de objeto (OneSkin: `nlObjPutLinkOneSkinStripData` no se ha leido en esta pasada).
- El desensamblador marca `efu?` los opcodes de la unidad EFU (solo 2 apariciones en el programa 0) y no se ha contrastado contra otro desensamblador.

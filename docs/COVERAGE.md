# Cobertura (generado por tools/coverage_report.py)

Tamaños de `st_size` de la symtab; instr = bytes/4. `main` tiene su código en el ELF; el resto son **overlays con filesz 0**: su código vive en ficheros del disco.

| Módulo | Funciones | Bytes | <=1 instr | 2-4 | 5-10 | 11-20 | 21-40 | 41-100 | 101-300 | >300 |
|---|---|---|---|---|---|---|---|---|---|---|
| **main (en el ELF)** | 4492 | 858,640 | 0 | 749 | 536 | 694 | 1017 | 1036 | 378 | 82 |
| **overlays (en el disco)** | 2861 | 1,072,164 | 0 | 313 | 131 | 309 | 540 | 827 | 577 | 164 |
| &nbsp;&nbsp;gamemod | 991 | 354,640 | 0 | 105 | 48 | 88 | 183 | 313 | 201 | 53 |
| &nbsp;&nbsp;aomod | 227 | 158,676 | 0 | 23 | 6 | 13 | 25 | 40 | 87 | 33 |
| &nbsp;&nbsp;bs2mod | 221 | 64,744 | 0 | 30 | 12 | 25 | 39 | 64 | 41 | 10 |
| &nbsp;&nbsp;bs6mod | 205 | 60,856 | 0 | 28 | 11 | 23 | 39 | 66 | 32 | 6 |
| &nbsp;&nbsp;bs3mod | 191 | 47,760 | 0 | 28 | 12 | 25 | 37 | 59 | 25 | 5 |
| &nbsp;&nbsp;bs4mod | 185 | 63,324 | 0 | 17 | 14 | 30 | 29 | 50 | 37 | 8 |
| &nbsp;&nbsp;acmod | 173 | 45,384 | 0 | 15 | 7 | 25 | 48 | 53 | 20 | 5 |
| &nbsp;&nbsp;optmod | 160 | 73,684 | 0 | 22 | 6 | 18 | 35 | 41 | 30 | 8 |
| &nbsp;&nbsp;trnmod | 159 | 77,716 | 0 | 13 | 6 | 18 | 32 | 46 | 26 | 18 |
| &nbsp;&nbsp;bs5mod | 143 | 40,112 | 0 | 19 | 5 | 22 | 30 | 38 | 24 | 5 |
| &nbsp;&nbsp;shpmod | 123 | 47,824 | 0 | 5 | 3 | 15 | 30 | 30 | 35 | 5 |
| &nbsp;&nbsp;hntmod | 55 | 26,740 | 0 | 4 | 0 | 5 | 6 | 18 | 16 | 6 |
| &nbsp;&nbsp;orgmod | 28 | 10,704 | 0 | 4 | 1 | 2 | 7 | 9 | 3 | 2 |

## Clases con más funciones en `main`

(libre/C) (3324), FcvEffect (114), EventCtrlTask (114), PlayerTask (51), PeopleTask (50), EntryDatClass (43), OneSkinMgrClass (40), SoundMgrClass (30), CFcvTRS (25), CameraMgrInternalClass (24), CCloth (22), TrnKomono2Task (20), BodyMgrClass (20), FaceAnimMgrClass (19), PolyMgrClass (18), IraiNote (17), CChain (17), TexMgrClass (17), LightCtrlTask (16), MojiUchi (16), DofCtrlTask (15), DynamicsTask (15), TexBufMgrClass (15), CamPathMgrClass (13), CameraTask (13)

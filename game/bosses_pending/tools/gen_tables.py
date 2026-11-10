#!/usr/bin/env python3
# Genera boss_pending_data.cpp desde disc/TBL/BS3MOD.BIN, BS4MOD.BIN y baserom/SCES_504.11. Ejecutar desde la raiz del repo.
import struct
LOAD = 0x9C4800
b3 = open('disc/TBL/BS3MOD.BIN', 'rb').read()
b4 = open('disc/TBL/BS4MOD.BIN', 'rb').read()
elf = open('baserom/SCES_504.11', 'rb').read()
GP = 0x2E00F0   # .reginfo gp
def at4(a, n): o = a - LOAD; return b4[o:o+n]
def at3(a, n): o = a - LOAD; return b3[o:o+n]
def ef(v, n): o = v - 0x140000 + 0x800; return elf[o:o+n]
def ints(d): return struct.unpack('<%di' % (len(d)//4), d)
def uints(d): return struct.unpack('<%dI' % (len(d)//4), d)
def flts(d): return struct.unpack('<%df' % (len(d)//4), d)
def fl(x): return repr(x) + 'f' if x == x else '0.0f'
o = ['// GENERADO por game/bosses_pending/tools/gen_tables.py (no editar a mano).',
     '// Fuentes: disc/TBL/BS4MOD.BIN, disc/TBL/BS3MOD.BIN (carga 0x9C4800) y baserom/SCES_504.11 (gp = 0x2E00F0).',
     '#include "boss_pending_data.h"', 'namespace vn { namespace bosspend {', '']
# ---- Boss4 ----
t = uints(at4(0x9D2B00, 72))
o.append('// TBoss4 D_009D2B00: 6 filas de 12 B que CtrlPre copia (una vez, global gp-0x7460) a D_26B430, la tabla que func_1C6C80 consulta por\n// estado de tarea (+0x1F1C). Bits crudos: contienen punteros a act* del overlay (0x9CCDC0 actExecSeqCmd, 0x9CBB20 actBunshin, 0x9CBB50 actShoukan, 0x9CBA90 actDelete) y -1.')
o.append('const uint32_t kB4StateRows[6][3] = {' + ','.join('{0x%08X,0x%08X,0x%08X}' % t[i*3:i*3+3] for i in range(6)) + '};')
o.append('// TBoss4 D_009D3AE0: numero de rocas [nivel][dificultad 0..16] (bytes; fila de 17).')
bb = at4(0x9D3AE0, 34)
o.append('const int8_t kB4IwaCount[2][17] = {' + ','.join('{' + ','.join(str(x) for x in struct.unpack('<17b', bb[r*17:(r+1)*17])) + '}' for r in range(2)) + '};')
f = flts(at4(0x9D3B10, 4*34))
o.append('// TBoss4 D_009D3B10: velocidad de las rocas [nivel][dificultad] (filas de 0x44 B).')
o.append('const float kB4IwaSpeed[2][17] = {' + ','.join('{' + ','.join(fl(x) for x in f[r*17:(r+1)*17]) + '}' for r in range(2)) + '};')
# D_009D3D30: 7 entradas de 24 B (x,y,z, ?, ang@+0x10, ?)
rows = []
for k in range(7):
    d = at4(0x9D3D30 + k*24, 24)
    x, y, z = flts(d[:12]); ang = ints(d[16:20])[0]
    rows.append('{%s,%s,%s,%d}' % (fl(x), fl(y), fl(z), ang))
o.append('// TBoss4 D_009D3D30: 7 puntos de invocacion/aparicion {x,y,z, angulo (+0x10)}, indexados por +0x1E10.')
o.append('const B4ShoukanSlot kB4ShoukanSlot[7] = {' + ','.join(rows) + '};')
rows = []
for k in range(7):
    d = at4(0x9D3DE0 + k*24, 24)
    x, y, z = flts(d[:12]); rows.append('{%s,%s,%s}' % (fl(x), fl(y), fl(z)))
o.append('// TBoss4 D_009D3DE0: 7 desplazamientos {x,y,z} para la ruta de camara (ControlPos, subfase 0).')
o.append('const float kB4PathShift[7][3] = {' + ','.join(rows) + '};')
rows = []
for k in range(25):
    d = at4(0x9D3FB0 + k*24, 24)
    x, y, z = flts(d[:12]); rows.append('{%s,%s,%s}' % (fl(x), fl(y), fl(z)))
o.append('// TBoss4 D_009D3FB0: 25 puntos de origen de rocas (24 B por fila; se usan los 12 primeros).')
o.append('const float kB4IwaOrigin[25][3] = {' + ','.join(rows) + '};')
f = flts(at4(0x9D3F10, 24))
o.append('// TBoss4 D_009D3F10: factor de desplazamiento de los clones [1E39 de la fila (0..2)][indice de clon (0..1)].')
o.append('const float kB4BunshinSpread[3][2] = {' + ','.join('{%s,%s}' % (fl(f[2*i]), fl(f[2*i+1])) for i in range(3)) + '};')
v = ints(at4(0x9D4210, 16))
o.append('// TBoss4 D_009D4210 [1E12 (0..1)][arg (0..1)] y gp-0x79B0 / gp-0x79A8 (ELF) para StartAttackKiriPre.')
o.append('const int32_t kB4KiriPreAngle[2][2] = {{%d,%d},{%d,%d}};' % (v[0], v[1], v[2], v[3]))
a = ints(ef(GP - 0x79B0, 8)); m = ints(ef(GP - 0x79A8, 8))
o.append('const int32_t kB4KiriPreAngleAdd[2] = {%d,%d};   // gp-0x79B0' % a)
o.append('const int32_t kB4KiriPreMotion[2] = {%d,%d};   // gp-0x79A8' % m)
fz = flts(at4(0x9D4220, 12)); ka = ints(at4(0x9D4230, 12)); sm = ints(at4(0x9D4240, 12))
o.append('const float kB4KiriY[3] = {%s,%s,%s};          // D_009D4220' % tuple(fl(x) for x in fz))
o.append('const int32_t kB4KiriAngle[3] = {%d,%d,%d};      // D_009D4230' % ka)
o.append('const int32_t kB4KiriShoukanMotion[3] = {%d,%d,%d};   // D_009D4240' % sm)
# tabla de saltos de ControlBunshin_battle2 / ControlBunshin (jtbl_009DA760)
t = struct.unpack('<19I', at4(0x9DA760, 76))
o.append('// TBoss4 jtbl_009DA760 (ControlBunshin_battle2): 0 = pone 0x4000 en +0x1E14, 1 = lo quita, 2 = nada. Indice = movimiento - 0x16C.')
cls = {0x9C980C: 0, 0x9C981C: 1, 0x9C982C: 2}
o.append('const uint8_t kB4BunshinMotAction[19] = {' + ','.join(str(cls[x]) for x in t) + '};')

# D_009D3F30: movimientos de los clones en la pelea 2 [fila de 1E39 (0..2)][clon (0..1)][5]; jtbl_009DA690 (19 entradas, idx = movimiento - 0x16C)
rows = ints(at4(0x9D3F30, 120))
o.append('// TBoss4 D_009D3F30 (CtrlPre_bunshin_battle2): [1E39 del padre][indice de clon][5 movimientos].')
o.append('const int32_t kB4BunshinMotRows[3][2][5] = {' + ','.join('{' + ','.join('{' + ','.join(str(x) for x in rows[(i*2+j)*5:(i*2+j)*5+5]) + '}' for j in range(2)) + '}' for i in range(3)) + '};')
t = struct.unpack('<19I', at4(0x9DA690, 76))
ccls = {0x9C6F6C: 0, 0x9C6F5C: 1, 0x9C6F64: 2, 0x9C6F0C: 3, 0x9C6F84: 4, 0x9C6F80: 5}
o.append('// jtbl_009DA690: 0 = fila[2 + 1E3B del padre], 1 = fila[0], 2 = fila[1], 3 = copia pos/rot del padre, 4 = ninguno (-1), 5 = 0x17E.')
o.append('const uint8_t kB4BunshinB2Class[19] = {' + ','.join(str(ccls[x]) for x in t) + '};')
# ---- Boss3: tabla completa D_009D0880 (13 entradas validas) ----
rows = []
for i in range(13):
    an, ln = ints(at3(0x9D0880 + i*8, 8)); rows.append('{%d,%d}' % (an, ln))
o.append('// TBoss3 D_009D0880: mapa de movimientos {animacion, longitud}; 13 entradas validas (indices 0..12; 0x9D08E8 ya es otra tabla).')
o.append('const B3MotionEntry kB3MotionFull[13] = {' + ','.join(rows) + '};')
o.append('}} // namespace')
open('game/bosses_pending/boss_pending_data.cpp', 'w').write('\n'.join(o) + '\n')

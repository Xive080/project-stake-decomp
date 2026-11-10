#!/usr/bin/env python3
# Genera boss_rest_data.cpp desde disc/TBL/BS3MOD.BIN (overlay bs3mod, carga 0x9C4800). Ejecutar desde la raiz del repo.
import struct, sys
LOAD = 0x9C4800
bin3 = open('disc/TBL/BS3MOD.BIN', 'rb').read()
def at(addr, n): o = addr - LOAD; return bin3[o:o+n]
out = ['// Generado por tools/gen_tables.py desde disc/TBL/BS3MOD.BIN. No editar a mano.',
       '#include "boss_rest_data.h"', 'namespace vn { namespace boss {', '']
rows = at(0x9D0A20, 3 * 68)
out.append('// D_009D0A20 (0x9D0A20): [fase][2*dificultad + {0 = golpes por impacto, 1 = golpes para aturdir}], 68 B por fase.')
out.append('const int8_t kBs3HitRows[3][34][2] = {')
for p in range(3):
    r = struct.unpack('<68b', rows[p*68:(p+1)*68])
    out.append('  {' + ','.join('{%d,%d}' % (r[2*i], r[2*i+1]) for i in range(34)) + '},')
out.append('};')
bin2 = open('disc/TBL/BS2MOD.BIN', 'rb').read()
o2 = 0x9D6250 - LOAD
out.append('// Boss2Task D_009D6250: longitud (en s16) de cada operacion de ExecSeqCmd (0..29).')
out.append('const int8_t kBs2OpLen[30] = {' + ','.join(str(v) for v in struct.unpack('<30b', bin2[o2:o2+30])) + '};')
out.append('}} // namespace')
open('game/bosses_rest/boss_rest_data.cpp', 'w').write('\n'.join(out) + '\n')

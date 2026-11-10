#!/usr/bin/env python3
# Genera bosses56_data.cpp a partir de asm/bs6mod/data/bs6mod_data.data.s (bytes del comentario /* off addr XXXXXXXX */, little endian).
# Uso (desde la raiz del repo): python3 game/bosses56/tools/gen_tables.py > game/bosses56/bosses56_data.cpp
import re, struct, sys
src = open('asm/bs6mod/data/bs6mod_data.data.s').read().splitlines()
def words(label, n):
    out = []; on = False
    for l in src:
        if l.startswith('dlabel ' + label): on = True; continue
        if on and l.startswith('enddlabel'): break
        if on:
            m = re.search(r'/\*\s*\w+\s+\w+\s+([0-9A-F]{8})\s*\*/', l)
            if m: out.append(struct.unpack('<I', bytes.fromhex(m.group(1)))[0])
    return out[:n]
def fl(u): return struct.unpack('<f', struct.pack('<I', u))[0]
print('// Generado por game/bosses56/tools/gen_tables.py desde asm/bs6mod/data/bs6mod_data.data.s. No editar a mano.')
print('#include "bosses56_b6.h"')
print('namespace vn { namespace boss56 {')
for name, label, n in (('kBatSpeedA', 'D_009D4720', 17), ('kBatSpeedB', 'D_009D47A8', 17)):
    w = words(label, n)
    print('// %s (%d floats)' % (label, n))
    print('const uint32_t %s_bits[%d] = {%s};' % (name, n, ', '.join('0x%08X' % x for x in w)))
w = words('D_009D4830', 17)
print('// D_009D4830 (17 enteros: ids de ruta de los murcielagos)')
print('const int32_t kBatPathIds[17] = {%s};' % ', '.join(str(x) for x in w))
print('}}  // namespace vn::boss56')

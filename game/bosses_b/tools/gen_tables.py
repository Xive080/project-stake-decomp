#!/usr/bin/env python3
"""Genera game/bosses_b/bossN_data.cpp leyendo disc/TBL/BSnMOD.BIN (overlay, base 0x9C4800) y baserom/SCES_504.11
(ELF main; offset = v-0x140000+0x800). Uso (desde la raiz del repo): python3 game/bosses_b/tools/gen_tables.py 4|5|6
Todo valor emitido sale de los binarios; no se edita a mano."""
import struct, sys, os
BASE = 0x9C4800
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..')
ELF = open(os.path.join(ROOT, 'baserom', 'SCES_504.11'), 'rb').read()

class Ovl:
    def __init__(s, n):
        s.n = n
        s.d = open(os.path.join(ROOT, 'disc', 'TBL', 'BS%dMOD.BIN' % n), 'rb').read()
    def raw(s, a, n):
        if a >= BASE and a < 0xA00000:
            o = a - BASE; return s.d[o:o + n]
        o = a - 0x140000 + 0x800; return ELF[o:o + n]
    def rd(s, a, fmt, n=1):
        sz = struct.calcsize('<' + fmt)
        return list(struct.unpack('<' + fmt * n, s.raw(a, sz * n)))

def arr(name, ctype, vals, per=12, fmt=str, indent=''):
    out = 'const %s %s[%d] = {\n' % (ctype, name, len(vals))
    for i in range(0, len(vals), per):
        out += '    ' + ', '.join(fmt(v) for v in vals[i:i + per]) + ',\n'
    return out + '};\n'

def ffmt(v):
    r = '%.9g' % v
    if 'e' not in r and '.' not in r: r += '.0'
    return r + 'f'

def words_ptr(ov, a):
    return ov.rd(a, 'I')[0]

class Scripts:
    """Recoge scripts (int16) del overlay: region contigua + copias cortas de scripts del ELF (centinelas)."""
    def __init__(s, ov, lo, hi):
        s.ov = ov; s.lo = lo; s.hi = hi
        s.words = ov.rd(lo, 'h', (hi - lo) // 2)
        s.extra = {}
    def idx(s, addr):
        if s.lo <= addr < s.hi: return (addr - s.lo) // 2
        if addr not in s.extra:
            s.extra[addr] = len(s.words) + 4 * len(s.extra)
        return s.extra[addr]
    def finish(s):
        w = list(s.words)
        for a, i in sorted(s.extra.items(), key=lambda kv: kv[1]):
            assert len(w) == i
            w += s.ov.rd(a, 'h', 4)
        return w

def hdr(n, what):
    return ('// GENERADO por game/bosses_b/tools/gen_tables.py %d (no editar a mano). Fuente: disc/TBL/BS%dMOD.BIN y baserom/SCES_504.11.\n'
            '// %s\n#include "boss%d.h"\nnamespace vn { namespace boss%d {\n\n' % (n, n, what, n, n))

def diff_rows(ov, base, nrows, n17=17):
    """D_*3C90: filas de 16 B {ptr hp(s8[17]), ptr dmgMul(s32[17]), ptr speed(f32[17]), ptr step(s16[2])}"""
    rows = []
    for i in range(nrows):
        p = ov.rd(base + 16 * i, 'I', 4)
        rows.append(dict(hp=ov.rd(p[0], 'b', n17), dmg=ov.rd(p[1], 'i', n17), spd=ov.rd(p[2], 'f', n17), step=ov.rd(p[3], 'h', 2)))
    return rows

def emit_diff_rows(name, rows):
    o = 'const DiffRow %s[%d] = {\n' % (name, len(rows))
    for r in rows:
        o += '    {{%s},\n     {%s},\n     {%s},\n     %d, %d},\n' % (
            ', '.join(map(str, r['hp'])), ', '.join(map(str, r['dmg'])), ', '.join(ffmt(v) for v in r['spd']), r['step'][0], r['step'][1])
    return o + '};\n'

def gen4():
    ov = Ovl(4)
    o = hdr(4, 'Tablas de TBoss4 (overlay bs4mod, carga 0x9C4800).')
    ln = ov.rd(0x9D4250, 'b', 67)
    o += '// D_009D4250: longitud (en int16) de cada comando de ExecSeqCmd\n' + arr('kCmdLen', 'int8_t', ln, 16)
    sc = Scripts(ov, 0x9D2C20, 0x9D32D0)
    tabs = []
    for t in (0x9D2DD0, 0x9D2F40, 0x9D31D0, 0x9D32D0):
        ent = []
        a = t
        while True:
            p = words_ptr(ov, a)
            if p == 0: break
            ent.append(sc.idx(p)); a += 4
        tabs.append(ent)
    calls = [sc.idx(words_ptr(ov, 0x9D2CA0 + 4 * i)) for i in range(5)]
    cfg = []
    tabname = {0x9D2DD0: 0, 0x9D2F40: 1, 0x9D31D0: 2, 0x9D32D0: 3}
    for k in range(5):
        c = ov.rd(0x9D3390 + 0 and 0, 'I', 0) if False else None
        cp = words_ptr(ov, 0x9D3390 + 4 * k)
        w = ov.rd(cp, 'I', 6)
        cfg.append((w, tabname.get(w[4], -1)))
    w = sc.finish()
    o += '// Region D_009D2C20..D_009D32D0 (scripts de comandos, int16) + 4 int16 de cada script centinela del ELF (0x2D8700..0x2D871C). Indice = (dir-0x9D2C20)/2.\n'
    o += arr('kScript', 'int16_t', w, 16) + 'const unsigned kScriptLen = %d;\n\n' % len(w)
    for i, e in enumerate(tabs):
        o += '// tabla de scripts del tipo %d (D_%08X): indices en kScript; 0xFFFF = fin (puntero nulo)\n' % (i, [0x9D2DD0, 0x9D2F40, 0x9D31D0, 0x9D32D0][i])
        o += arr('kSeqTab%d' % i, 'uint16_t', e + [0xFFFF], 16) + '\n'
    o += '// D_009D2CA0: scripts de reaccion (SeqCmdCall)\n' + arr('kCallScript', 'uint16_t', calls, 8) + '\n'
    o += '// D_009D3390 -> 5 configuraciones de CharInit {orC, andC, or1E14, and1E14, tabla, estado inicial 1F1C}\n'
    o += 'const KindConfig kKindConfig[5] = {\n'
    for w6, ti in cfg:
        o += '    {0x%08XU, 0x%08XU, 0x%08XU, 0x%08XU, %s, %d},\n' % (w6[0], w6[1], w6[2], w6[3], ('kSeqTab%d' % ti) if ti >= 0 else 'nullptr', w6[5])
    o += '};\n\n'
    # tabla de ataques: 15 x 24 B
    o += '// D_009DA500: 15 entradas de 24 B {motion, flags/dano(+3), inicio, f0..f4 (frames, -1 = no usar), fin}\n'
    o += 'const AttackEntry kAttackTable[15] = {\n'
    for i in range(15):
        s16 = ov.rd(0x9DA500 + 24 * i, 'h', 12)
        b = ov.raw(0x9DA500 + 24 * i, 24)
        o += '    {%d, %d, %d, {%s}, %d, %s},\n' % (s16[0], b[3], s16[2], ', '.join(map(str, s16[3:9])), s16[9], ffmt(struct.unpack_from('<f', b, 20)[0]))
    o += '};\n\n'
    # 2BB0: 2 filas de 14 B
    rr = []
    for i in range(2):
        s = ov.rd(0x9D2BB0 + 14 * i, 'h', 5); b = ov.rd(0x9D2BB0 + 14 * i + 10, 'b', 4)
        rr.append('    {{%s}, {%s}}' % (', '.join(map(str, s)), ', '.join(map(str, b[:3]))))
    o += '// D_009D2BB0: 2 filas de 14 B (5 movimientos + 3 ataques; D_009D2BBA = +10)\nconst RunRowA kRunRowA[2] = {\n' + ',\n'.join(rr) + '\n};\n\n'
    rr = []
    for i in range(3):
        s = ov.rd(0x9D2BD0 + 8 * i, 'h', 2); b = ov.rd(0x9D2BD0 + 8 * i + 4, 'b', 4)
        rr.append('    {{%s}, {%s}}' % (', '.join(map(str, s)), ', '.join(map(str, b[:3]))))
    o += '// D_009D2BD0: 3 filas de 8 B (2 movimientos + 3 ataques; D_009D2BD4 = +4)\nconst RunRowB kRunRowB[3] = {\n' + ',\n'.join(rr) + '\n};\n\n'
    f = ov.rd(0x9D2B60, 'f', 18)
    o += '// D_009D2B60: 3 poses {pos xyz, rot xyz} de 24 B (comando 26)\nconst float kPose[3][6] = {\n' + ',\n'.join('    {%s}' % ', '.join(ffmt(v) for v in f[6 * i:6 * i + 6]) for i in range(3)) + '\n};\n\n'
    f = ov.rd(0x9D2BF0, 'f', 9)
    o += '// D_009D2BF0: 3 posiciones de reaparicion (comando 62)\nconst float kReappearPos[3][3] = {\n' + ',\n'.join('    {%s}' % ', '.join(ffmt(v) for v in f[3 * i:3 * i + 3]) for i in range(3)) + '\n};\n\n'
    o += '// ELF 0x2D86F8 (gp-0x79F8; gp = 0x2E00F0 de .reginfo): frames de camara de las reapariciones del comando 62 (indice = phaseCount-1)\n' + arr('kSchedFrame', 'int16_t', ov.rd(0x2D86F8, 'h', 8), 8) + '\n'
    rows = diff_rows(ov, 0x9D3C90, 10)
    o += '// D_009D3C90: 5 tipos x 2 niveles de dificultad de juego; cada fila resuelve sus 4 punteros\n' + emit_diff_rows('kDiffRows', rows) + '\n'
    h = ov.rd(0x9D3470, 'b', 102)
    o += '// D_009D3470: vida del tipo 2 [subfase 1E50][nivel][dificultad 0..16]\nconst int8_t kHpKind2[3][2][17] = {\n'
    for a in range(3):
        o += '    {{%s},\n     {%s}},\n' % (', '.join(map(str, h[34 * a:34 * a + 17])), ', '.join(map(str, h[34 * a + 17:34 * a + 34])))
    o += '};\n'
    for nm, ad, cm in (('kLoopKind1', 0x9D36C0, 'D_009D36C0: 1E30 del tipo 1 [nivel][dificultad]'), ('kLoopKind2Sub0', 0x9D3A90, 'D_009D3A90: 1E30 del tipo 2, subfase 0'), ('kLoopKind2Sub2', 0x9D3BA0, 'D_009D3BA0: 1E30 del tipo 2, subfase 2')):
        v = ov.rd(ad, 'h', 34)
        o += '// %s\nconst int16_t %s[2][17] = {\n    {%s},\n    {%s}};\n' % (cm, nm, ', '.join(map(str, v[:17])), ', '.join(map(str, v[17:])))
    o += '\n}}  // namespace vn::boss4\n'
    open(os.path.join(ROOT, 'game', 'bosses_b', 'boss4_data.cpp'), 'w').write(o)

if __name__ == '__main__':
    for a in sys.argv[1:]:
        globals()['gen' + a]()

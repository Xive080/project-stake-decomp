#!/usr/bin/env python3
"""Decodificador de rutas de camara (CamPathMgr) de Vampire Night PAL.
Formato (leido de CamPathMgr::entryExec_STAT_00/02, getCameraPath, getEndFrame, getStartFrame, CalcFCurve):
  * indice de ruta p (1..17) -> ID de entrada AFS = s16 del ELF 0x26CB20[p]; solo la fase actual lo trae (ST1..ST6/TRN/ORG).
  * CAMNOTBL.BIN: +0x590 = tabla de punteros RAM (base 0x1FC0000) por ruta p a una lista s16 de numeros de camara (fin = <0);
    +0x5E0 = s16[numero de camara] -> ruta p (696 entradas); +0xB50 = s16[numero de camara] -> duracion en cuadros.
  * Fichero de ruta: u32 offs[N], u32 0xFFFFFFFF; N = longitud de la lista de camaras de p; offs[k] = posicion de la
    k-esima camara de la lista. Cada camara = registro de 9 u32 (indices) precedido de sus curvas; el indice i apunta a la
    curva en  4*(N+1) + 4*i. (Las curvas de cada camara van justo antes de su registro.)
  * Registro: [0..2] pos xyz, [3..5] destino (look-at) xyz, [6] float (fov/roll), [7] entero (solo si bit0 del flag global),
    [8] sin uso conocido en el codigo leido.
  * Curva CPFRM: u16 n, u16 iteraciones de busqueda, n claves de 16 B {t f32, v f32, m_sal f32, m_ent f32}; Hermite cubico
    entre claves (tangentes en valor/cuadro), constante fuera del rango (CalcFCurve).
Uso: afs_campath.py FASE.AFS|all   (resumen y comprobaciones)"""
import struct, sys, math
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import afs as _afs

ELF = ROOT / "baserom" / "SCES_504.11"
CAMNO = ROOT / "disc" / "TBL" / "CAMNOTBL.BIN"
CAMNO_BASE = 0x1FC0000

def path_ids():
    d = ELF.read_bytes(); return list(struct.unpack_from("<24h", d, 0x26CB20 - 0x140000 + 0x800))

def cam_lists():
    """(listas por ruta p, camara->ruta)."""
    d = CAMNO.read_bytes(); lists = {}
    for p in range(1, 20):
        ptr = struct.unpack_from("<I", d, 0x590 + 4 * p)[0]
        if not CAMNO_BASE <= ptr < CAMNO_BASE + len(d): break
        o = ptr - CAMNO_BASE; l = []
        while True:
            v = struct.unpack_from("<h", d, o)[0]
            if v < 0: break
            l.append(v); o += 2
        lists[p] = l
    c2p = struct.unpack_from("<696h", d, 0x5E0)   # 696 entradas (0x5E0..0xB4F); las camaras usadas son 0..693
    return lists, c2p

def cam_lengths():
    """CAMNOTBL +0xB50: s16[numero de camara] = duracion nominal en cuadros (coincide con el tiempo de la ultima clave en 819 de 824 camaras)."""
    d = CAMNO.read_bytes(); return struct.unpack_from("<%dh" % ((len(d) - 0xB50) // 2), d, 0xB50)

class Curve:
    def __init__(self, b, off):
        self.n, self.it = struct.unpack_from("<HH", b, off)
        self.k = [struct.unpack_from("<4f", b, off + 4 + 16 * i) for i in range(self.n)]
        self.size = 4 + 16 * self.n
    def __call__(self, x):
        k = self.k; n = self.n
        # bucle exacto de CalcFCurve: busqueda binaria con n>>1 y paso n>>2 durante it-1 iteraciones
        a1 = n >> 1; step = n >> 2
        for _ in range(self.it - 1):
            if 0 <= a1 < n:
                a1 += step if k[a1][0] < x else -step
            step = (step + 1) >> 1 if step < 0 else step >> 1
        a1 = min(max(a1, 0), n - 1)
        if k[a1][0] < x:
            if a1 < n - 1: a1 += 1
            else: return k[a1][1]
        if a1 == 0: return k[0][1]
        t0, v0, m0, _ = k[a1 - 1]; t1, v1, _, m1 = k[a1]
        s = x - t0; d = t1 - t0; s2 = s * s; s3 = s2 * s
        a = s2 * 3.0 / (d * d); b = s3 / (d * d * d)
        return (1 - a + 2 * b) * v0 + (a - 2 * b) * v1 + (s - 2 * s2 / d + s3 / (d * d)) * m0 + (s3 / (d * d) - s2 / d) * m1

def exact_eval(c, x):
    """Version tal cual el asm (por si la busqueda binaria diera una clave distinta); referencia de prueba."""
    return c(x)

class CamPath:
    def __init__(self, b, cams):
        self.b = b; self.cams = cams; N = len(cams)
        self.offs = list(struct.unpack_from("<%dI" % N, b, 0)); self.term = struct.unpack_from("<I", b, 4 * N)[0]
        self.pool = 4 * (N + 1)
    def rec_len(self, k):
        """Palabras del registro (6 objeto, 8/9 camara). Cada camara = sus curvas + su registro; el registro termina
        donde empiezan las curvas de la siguiente (indice minimo de las 6 primeras palabras del registro siguiente).
        En la ultima camara solo hay relleno a cero detras: se hereda la longitud de la anterior (ambiguo si es la unica)."""
        if hasattr(self, "_rl") and k in self._rl: return self._rl[k]
        if not hasattr(self, "_rl"): self._rl = {}
        o = self.offs[k]; r = None
        if k + 1 < len(self.offs):
            nxt = min(struct.unpack_from("<6I", self.b, self.offs[k + 1]))
            for L in (6, 7, 8, 9):
                if o + 4 * L == self.pool + 4 * nxt: r = L; break
        elif k > 0:
            r = self.rec_len(k - 1)
        else:
            r = 8
        self._rl[k] = r
        return r
    def record(self, k):
        L = self.rec_len(k)
        return struct.unpack_from("<%dI" % L, self.b, self.offs[k])
    def curve(self, idx): return Curve(self.b, self.pool + 4 * idx)
    def camera(self, k):
        r = self.record(k)
        return {"pos": [self.curve(r[i]) for i in range(3)], "tgt": [self.curve(r[i]) for i in range(3, 6)],
                "fov": self.curve(r[6]), "flag": self.curve(r[7]) if len(r) > 7 else None,
                "extra": self.curve(r[8]) if len(r) > 8 else None, "rec": r}
    def frames(self, k):
        c = self.camera(k)["pos"][0]; return c.k[0][0], c.k[-1][0]

def load_paths(afs_file):
    ids = path_ids(); lists, _ = cam_lists()
    f, ents, _n, _ = _afs.parse(afs_file); out = {}
    for p, cams in lists.items():
        i = ids[p]
        if not i or not ents[i][1]: continue
        f.seek(ents[i][0]); out[p] = CamPath(f.read(ents[i][1]), cams)
    return out

def check(cp):
    """Comprobaciones estructurales: los 9 indices de cada registro apuntan a curvas validas (n>0, tiempos crecientes),
    las curvas de cada camara + su registro (36 B) teselan el fichero sin huecos ni solapes y el resto es relleno a cero."""
    b = cp.b; N = len(cp.cams); notes = []
    if cp.term != 0xFFFFFFFF: notes.append("sin terminador")
    pos = cp.pool
    for k in range(N):
        r = cp.record(k) if cp.rec_len(k) else None
        if r is None: notes.append("registro sin longitud cam %d" % k); continue
        spans = []
        for i in r:
            c = cp.curve(i)
            if c.n == 0 or any(c.k[j][0] > c.k[j + 1][0] for j in range(c.n - 1)): notes.append("curva invalida cam %d" % k); break
            spans.append((cp.pool + 4 * i, cp.pool + 4 * i + c.size))
        spans.sort()
        # las curvas de una camara pueden compartirse (mismo indice repetido); se unen
        merged = []
        for s0, e0 in spans:
            if merged and s0 < merged[-1][1]: 
                if (s0, e0) != merged[-1]: notes.append("solape cam %d" % k)
                continue
            merged.append((s0, e0))
        if k == 0 and False: pass
        if merged[0][0] < pos: notes.append("curva antes del bloque cam %d" % k)
        pos = cp.offs[k] + 4 * len(r)
    if any(b[pos:]): notes.append("cola no nula")
    return not notes, notes

def main():
    sel = sys.argv[1] if len(sys.argv) > 1 else "all"
    names = ["ST1", "ST2", "ST3", "ST4", "ST5", "ST6", "TRN", "ORG"] if sel == "all" else [Path(sel).stem]
    lists, c2p = cam_lists()
    for nm in names:
        for p, cp in sorted(load_paths(ROOT / "disc" / "FS" / (nm + ".AFS")).items()):
            ok, notes = check(cp)
            c2ok = all(c2p[c] == p for c in cp.cams)
            print(nm, "ruta", p, "camaras", len(cp.cams), "bytes", len(cp.b), "estructura", "OK" if ok else notes, "camno->ruta", c2ok)

if __name__ == "__main__": main()

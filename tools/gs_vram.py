#!/usr/bin/env python3
"""Simulacion minima de la VRAM del GS (PS2): escribe las subidas CT32 (disposicion por paginas/bloques/columnas) y lee texturas
T8 (PSMT8) y paletas CSM1. Tablas de bloque/columna del GS (verificadas por resultado: la imagen T8 sale suave)."""
import numpy as np

BT32 = np.array([[0, 1, 4, 5, 16, 17, 20, 21], [2, 3, 6, 7, 18, 19, 22, 23], [8, 9, 12, 13, 24, 25, 28, 29], [10, 11, 14, 15, 26, 27, 30, 31]])
CT32 = np.array([[0, 1, 4, 5, 8, 9, 12, 13], [2, 3, 6, 7, 10, 11, 14, 15], [16, 17, 20, 21, 24, 25, 28, 29], [18, 19, 22, 23, 26, 27, 30, 31],
                 [32, 33, 36, 37, 40, 41, 44, 45], [34, 35, 38, 39, 42, 43, 46, 47], [48, 49, 52, 53, 56, 57, 60, 61], [50, 51, 54, 55, 58, 59, 62, 63]])
BT8 = np.array([[0, 1, 4, 5, 16, 17, 20, 21], [2, 3, 6, 7, 18, 19, 22, 23], [8, 9, 12, 13, 24, 25, 28, 29], [10, 11, 14, 15, 26, 27, 30, 31]])
CT8 = np.array([
    [0, 4, 16, 20, 32, 36, 48, 52, 2, 6, 18, 22, 34, 38, 50, 54], [8, 12, 24, 28, 40, 44, 56, 60, 10, 14, 26, 30, 42, 46, 58, 62],
    [33, 37, 49, 53, 1, 5, 17, 21, 35, 39, 51, 55, 3, 7, 19, 23], [41, 45, 57, 61, 9, 13, 25, 29, 43, 47, 59, 63, 11, 15, 27, 31],
    [96, 100, 112, 116, 64, 68, 80, 84, 98, 102, 114, 118, 66, 70, 82, 86], [104, 108, 120, 124, 72, 76, 88, 92, 106, 110, 122, 126, 74, 78, 90, 94],
    [65, 69, 81, 85, 97, 101, 113, 117, 67, 71, 83, 87, 99, 103, 115, 119], [73, 77, 89, 93, 105, 109, 121, 125, 75, 79, 91, 95, 107, 111, 123, 127],
    [128, 132, 144, 148, 160, 164, 176, 180, 130, 134, 146, 150, 162, 166, 178, 182], [136, 140, 152, 156, 168, 172, 184, 188, 138, 142, 154, 158, 170, 174, 186, 190],
    [161, 165, 177, 181, 129, 133, 145, 149, 163, 167, 179, 183, 131, 135, 147, 151], [169, 173, 185, 189, 137, 141, 153, 157, 171, 175, 187, 191, 139, 143, 155, 159],
    [224, 228, 240, 244, 192, 196, 208, 212, 226, 230, 242, 246, 194, 198, 210, 214], [232, 236, 248, 252, 200, 204, 216, 220, 234, 238, 250, 254, 202, 206, 218, 222],
    [193, 197, 209, 213, 225, 229, 241, 245, 195, 199, 211, 215, 227, 231, 243, 247], [201, 205, 217, 221, 233, 237, 249, 253, 203, 207, 219, 223, 235, 239, 251, 255]])

def addr32(x, y, dbp, dbw):
    """Indice de palabra de VRAM del pixel (x,y) CT32 (arrays numpy)."""
    page = (y // 32) * dbw + (x // 64)
    return dbp * 64 + page * 2048 + BT32[(y % 32) // 8, (x % 64) // 8] * 64 + CT32[y % 8, x % 8]

def addr8(x, y, tbp, tbw):
    """Indice de BYTE de VRAM del pixel (x,y) PSMT8."""
    page = (y // 64) * (tbw // 2 if tbw >= 2 else 1) + (x // 128)
    word = tbp * 64 + page * 2048 + BT8[(y % 64) // 16, (x % 128) // 16] * 64
    return word * 4 + CT8[y % 16, x % 16]

class Vram:
    def __init__(self): self.w = np.zeros(1 << 20, np.uint32)        # 4 MB = 1M palabras
    def upload_ct32(self, data, w, h, dbp, dbw, psm=0):
        """Escribe una subida de imagen (psm 0 CT32 o 2 CT16) en la disposicion de VRAM del GS."""
        ys, xs = np.mgrid[0:h, 0:w]; bv = self.w.view(np.uint8)
        if psm == 19:
            bv[addr8(xs, ys, dbp, max(dbw, 1)) % (1 << 22)] = np.frombuffer(data[: w * h], np.uint8).reshape(h, w); return
        if psm == 20:
            raw = np.frombuffer(data[: w * h // 2], np.uint8); nib = np.empty(w * h, np.uint8); nib[0::2] = raw & 15; nib[1::2] = raw >> 4
            a = (addr4(xs, ys, dbp, max(dbw, 1)) % (1 << 23)).ravel(); lo = (a & 1) == 0
            bv[a[lo] >> 1] = (bv[a[lo] >> 1] & 0xF0) | nib[lo]; bv[a[~lo] >> 1] = (bv[a[~lo] >> 1] & 0x0F) | (nib[~lo] << 4); return
        if psm == 2:
            px = np.frombuffer(data[: w * h * 2], "<u2").reshape(h, w); self.w.view(np.uint16)[addr16(xs, ys, dbp, max(dbw, 1)) % (1 << 21)] = px; return
        px = np.frombuffer(data[: w * h * 4], "<u4").reshape(h, w)
        self.w[addr32(xs, ys, dbp, max(dbw, 1)) % (1 << 20)] = px
    def bytes_view(self): return self.w.view(np.uint8)
    def read_t8(self, tbp, tbw, w, h):
        ys, xs = np.mgrid[0:h, 0:w]; return self.bytes_view()[addr8(xs, ys, tbp, tbw) % (1 << 22)]
    def read_clut256(self, cbp):
        i = np.arange(256); perm = (i & 0xe7) | ((i & 8) << 1) | ((i & 0x10) >> 1)
        words = self.w[cbp * 64: cbp * 64 + 256]; ent = words[perm]
        return np.stack([(ent >> s) & 255 for s in (0, 8, 16, 24)], 1).astype(np.uint8)    # RGBA

BT16 = np.array([[0, 2, 8, 10], [1, 3, 9, 11], [4, 6, 12, 14], [5, 7, 13, 15], [16, 18, 24, 26], [17, 19, 25, 27], [20, 22, 28, 30], [21, 23, 29, 31]])
_r0 = [0, 2, 8, 10, 16, 18, 24, 26, 1, 3, 9, 11, 17, 19, 25, 27]; _r1 = [4, 6, 12, 14, 20, 22, 28, 30, 5, 7, 13, 15, 21, 23, 29, 31]
CT16 = np.array([_r0, _r1, [v + 32 for v in _r0], [v + 32 for v in _r1], [v + 64 for v in _r0], [v + 64 for v in _r1], [v + 96 for v in _r0], [v + 96 for v in _r1]])
BT4 = BT16
_q = [[0, 8, 32, 40, 64, 72, 96, 104, 2, 10, 34, 42, 66, 74, 98, 106, 4, 12, 36, 44, 68, 76, 100, 108, 6, 14, 38, 46, 70, 78, 102, 110],
      [16, 24, 48, 56, 80, 88, 112, 120, 18, 26, 50, 58, 82, 90, 114, 122, 20, 28, 52, 60, 84, 92, 116, 124, 22, 30, 54, 62, 86, 94, 118, 126],
      [65, 73, 97, 105, 1, 9, 33, 41, 67, 75, 99, 107, 3, 11, 35, 43, 69, 77, 101, 109, 5, 13, 37, 45, 71, 79, 103, 111, 7, 15, 39, 47],
      [81, 89, 113, 121, 17, 25, 49, 57, 83, 91, 115, 123, 19, 27, 51, 59, 85, 93, 117, 125, 21, 29, 53, 61, 87, 95, 119, 127, 23, 31, 55, 63]]
CT4 = np.array([[v + 128 * g for v in row] for g in range(4) for row in _q])

def addr16(x, y, tbp, tbw):
    """Indice de media palabra (16 bit) del pixel CT16."""
    page = (y // 64) * tbw + (x // 64)
    word = tbp * 64 + page * 2048 + BT16[(y % 64) // 8, (x % 64) // 16] * 64
    return word * 2 + CT16[y % 8, x % 16]

def addr4(x, y, tbp, tbw):
    """Indice de nibble (4 bit) del pixel PSMT4."""
    page = (y // 128) * (tbw // 2 if tbw >= 2 else 1) + (x // 128)
    word = tbp * 64 + page * 2048 + BT4[(y % 128) // 16, (x % 128) // 32] * 64
    return word * 8 + CT4[y % 16, x % 32]

def _read(vr, kind, x, y, tbp, tbw):
    if kind == 0: return vr.w[addr32(x, y, tbp, max(tbw, 1)) % (1 << 20)]
    if kind == 2: return vr.w.view(np.uint16)[addr16(x, y, tbp, max(tbw, 1)) % (1 << 21)]
    if kind == 19: return vr.w.view(np.uint8)[addr8(x, y, tbp, max(tbw, 1)) % (1 << 22)]
    a = addr4(x, y, tbp, max(tbw, 1)) % (1 << 23); b = vr.w.view(np.uint8)[a >> 1]; return np.where(a & 1, b >> 4, b & 15)

def read_texture(vr, psm, w, h, tbp, tbw=None):
    """Lee una textura (array h x w) de la VRAM segun PSM (0 CT32, 2 CT16, 19 T8, 20 T4); tbw por defecto = ancho/64."""
    ys, xs = np.mgrid[0:h, 0:w]; return _read(vr, psm, xs, ys, tbp, tbw or max(1, w // 64))

def clut_rgba(vr, cbp, n, cpsm):
    """Paleta de n entradas (16 o 256) en RGBA 0..255 (alfa ya escalado) desde CBP; cpsm 0 = CT32, 2 = CT16."""
    i = np.arange(n); perm = ((i & 0xe7) | ((i & 8) << 1) | ((i & 0x10) >> 1)) if n == 256 else i
    if cpsm == 0:
        e = vr.w[cbp * 64: cbp * 64 + 256][perm]; rgba = np.stack([(e >> s) & 255 for s in (0, 8, 16, 24)], 1); rgba[:, 3] = np.minimum(255, rgba[:, 3] * 2)
    else:
        e = vr.w.view(np.uint16)[cbp * 128: cbp * 128 + 512][perm].astype(np.int64)
        rgba = np.stack([(e & 31) * 255 // 31, ((e >> 5) & 31) * 255 // 31, ((e >> 10) & 31) * 255 // 31, np.where(e & 0x8000, 255, 0)], 1)
    return rgba.astype(np.uint8)

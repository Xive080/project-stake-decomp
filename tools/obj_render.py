#!/usr/bin/env python3
"""Render de comprobacion (z-buffer, textura por UV) de un .obj exportado por afs_export. Uso: obj_render.py obj_NNN.obj out.png [tamano]"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image

def load(fn):
    V, VT, F, mat = [], [], [], None
    for l in open(fn):
        t = l.split()
        if not t: continue
        if t[0] == "v": V.append(list(map(float, t[1:4])))
        elif t[0] == "vt": VT.append(list(map(float, t[1:3])))
        elif t[0] == "usemtl": mat = t[1]
        elif t[0] == "f": F.append(([int(a.split("/")[0]) - 1 for a in t[1:4]], mat))
    return np.array(V), np.array(VT), F

def render(fn, out, S=500):
    d = Path(fn).parent; V, VT, F = load(fn); tex = {}
    def T(m):
        if m not in tex:
            try: tex[m] = np.array(Image.open(d / f"{m}.png").convert("RGBA"))
            except Exception: tex[m] = None
        return tex[m]
    mn, mx = V.min(0), V.max(0); c = (mn + mx) / 2; sc = (mx - mn).max() or 1
    P = np.stack([(V[:, 0] - c[0]) / sc * S * .9 + S / 2, -(V[:, 1] - c[1]) / sc * S * .9 + S / 2, V[:, 2]], 1)
    img = np.zeros((S, S, 3), np.uint8) + 30; zb = np.full((S, S), -1e9)
    for tri, m in F:
        a, b, cc = [P[i] for i in tri]; uv = [VT[i] for i in tri]; im = T(m)
        x0, x1 = int(max(0, min(a[0], b[0], cc[0]))), int(min(S - 1, max(a[0], b[0], cc[0])))
        y0, y1 = int(max(0, min(a[1], b[1], cc[1]))), int(min(S - 1, max(a[1], b[1], cc[1])))
        den = (b[1] - cc[1]) * (a[0] - cc[0]) + (cc[0] - b[0]) * (a[1] - cc[1])
        if abs(den) < 1e-9 or x1 < x0 or y1 < y0: continue
        xs, ys = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
        l1 = ((b[1] - cc[1]) * (xs - cc[0]) + (cc[0] - b[0]) * (ys - cc[1])) / den; l2 = ((cc[1] - a[1]) * (xs - cc[0]) + (a[0] - cc[0]) * (ys - cc[1])) / den; l3 = 1 - l1 - l2
        ok = (l1 >= 0) & (l2 >= 0) & (l3 >= 0); z = l1 * a[2] + l2 * b[2] + l3 * cc[2]; ok &= z > zb[y0:y1 + 1, x0:x1 + 1]
        if not ok.any(): continue
        u = l1 * uv[0][0] + l2 * uv[1][0] + l3 * uv[2][0]; v = l1 * uv[0][1] + l2 * uv[1][1] + l3 * uv[2][1]
        if im is None: col = np.full(xs.shape + (3,), 150, np.uint8)
        else: col = im[((v % 1) * (im.shape[0] - 1)).astype(int), ((u % 1) * (im.shape[1] - 1)).astype(int)][..., :3]
        sub = img[y0:y1 + 1, x0:x1 + 1]; sub[ok] = col[ok]; zb[y0:y1 + 1, x0:x1 + 1][ok] = z[ok]
    Image.fromarray(img).save(out)

if __name__ == "__main__": render(sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 500)

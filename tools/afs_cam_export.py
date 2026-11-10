#!/usr/bin/env python3
"""Exporta una ruta de camara de una fase a `cam.txt` para el visor nativo (vn_viewer reproduce DIR/cam.txt con --cam).
Formato: una linea por cuadro: `ex ey ez tx ty tz fov` (posicion, destino y fov del registro; Y arriba; sin escalar).
Las camaras de la ruta se encadenan en el orden de la lista. Uso: afs_cam_export.py AFS RUTA OUTDIR [--scale S]"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import numpy as np, afs_campath as C

def main():
    afs_path, route, out = sys.argv[1], int(sys.argv[2]), Path(sys.argv[3]); sc = float(sys.argv[sys.argv.index("--scale") + 1]) if "--scale" in sys.argv else 1.0
    cp = C.load_paths(afs_path)[route]; lines = []; ncam = 0
    for cam in range(len(cp.cams)):
        try: c = cp.camera(cam); t0, t1 = cp.frames(cam)
        except Exception: continue
        if len(c["rec"]) < 7: continue          # trayectoria de objeto (6 palabras), no camara
        ncam += 1
        for t in np.arange(t0, t1, 1.0):
            p = np.array([k(t) for k in c["pos"]]) * sc; g = np.array([k(t) for k in c["tgt"]]) * sc
            lines.append("%.3f %.3f %.3f %.3f %.3f %.3f %.3f" % (*p, *g, c["fov"](t)))
    out.mkdir(parents=True, exist_ok=True); (out / "cam.txt").write_text("\n".join(lines) + "\n")
    print(f"ruta {route}: {ncam} camaras, {len(lines)} cuadros -> {out/'cam.txt'}")

if __name__ == "__main__": main()

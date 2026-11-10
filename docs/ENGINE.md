# Motor nativo (port)

Objetivo: ejecutar en Mac, Windows, Linux y Android. Decidido 2026-11-09: **C++17 + SDL3 + OpenGL 3.3 core (escritorio) / OpenGL ES 3.0 (Android)**; sin dependencias salvo SDL3 y `stb_image.h` (en `engine/third_party/`).
Los datos del juego NO se versionan: `tools/vnm_export.py` convierte cada banco a un formato propio (`bank.vnm` + `tex_NN.png`) y el motor solo lee ese formato.

- `tools/vnm_export.py AFS BANCO OUTDIR` -> VNM1 (mallas trianguladas, pos/uv/color/normal int8, textura por malla).
- `engine/src/vnm.*` carga VNM1; `engine/src/renderer.*` sube a GPU y dibuja; `engine/src/gl.h` abstrae GL/GLES (en Windows/Linux carga las funciones por `SDL_GL_GetProcAddress`; sin probar fuera de macOS).
- `engine/src/viewer.cpp`: visor (arrastrar = orbitar, rueda = zoom, flechas = bloque, T = texturas, `--shot out.bmp` para captura sin interaccion).

Compilar: `cmake -S engine -B build/engine -G Ninja && cmake --build build/engine`. Ejecutar: `build/engine/vn_viewer build/engine_assets/ST1_124`.

Estado (2026-11-09): macOS arm64 compilado y verificado con una captura (banco ST1 124 con texturas y normales). NO probado: Windows, Linux, Android (hace falta proyecto Gradle/SDL3 para Android; el codigo ya usa solo GLES3-compatible y eventos de raton que SDL mapea al tacto).
Sin implementar: visor de nivel (escenario, colisiones, camaras), animacion en el motor, iluminacion/mezcla de los registros GS de cada lote (docs/RENDER.md), audio, entradas.

## Visor de fase con camara de ruta (2026-11-09)
- Decorado de la fase 1: `tools/afs_stage_export.py` (ver docs/LEVELS.md, "Decorado de la fase 1") -> `build/stage_st1/` (bank.vnm + tex_NN.png).
- Ruta de camara: `tools/afs_cam_export.py AFS RUTA OUTDIR` escribe `OUTDIR/cam.txt` (una linea por cuadro: posicion, destino, fov; las camaras de la ruta encadenadas).
- `vn_viewer DIR --cam [--frame N] [--shot out.bmp]` reproduce `cam.txt` (30 cuadros/s; espacio = pausa, `,` `.` = cuadro a cuadro). Sin `--cam` sigue siendo el visor orbital.
- Plano lejano subido a 30 000 (el decorado llega a z -3930). Fov del registro tomado como vertical (sin confirmar).
- Verificado en macOS arm64 con capturas (ruta 1, cuadros 0, 600, 2000, 4000, 6500): geometria y texturas del decorado se ven y la camara se mueve por el. La ruta 1 NO es el recorrido jugable de la fase 1 (se ven interiores/arena); falta saber que ruta/camara corresponde a cada escena y que estado BGTB se dibuja en cada una (el exportador une todos los estados).
- Sin iluminacion VU1, alfa/mezcla GS, niebla ni cielo animado.

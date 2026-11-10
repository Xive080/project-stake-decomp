# game/trnkomono: reescritura a mano de las clases TrnKomono*

Direcciones: ELF principal (0x21xxxx) u overlay TRNMOD (0x9Exxxx, base 0x9DF000). Estado: **T** = reescrita con prueba, **S** = reescrita sin prueba propia.

| Funcion | Direccion | Estado |
|---|---|---|
| CTrnBoxPoint::init / initPos / setLink | 0x21AE40 / 0x21AE70 / 0x21AEA0 | T |
| CTrnKomonoBase::initEndCP / checkEndCP | 0x21AF60 / 0x21B170 | T |
| CTrnTumiageColli::ctor / setEnClName | ver trnkomono_main.cpp | T |
| TrnKomonoLogic::secondObj / findRadius / findBreak (ayudas) | tablas D_002C3260 / D_002C3540 | T |
| TrnKomonoLogic::init / ctorCounter / dtorCounter / ctrl / ctrlPost / close | ver trnkomono_main.cpp | T |
| CTrn05Komono::init / ctrl | 0x9EDFB0 / 0x9EE210 | T |
| Trn05KomonoMgrTask::ctor / ctrl | 0x9EE5A0 / 0x9EE730 | T |
| CTrn08Komono::setInterpolate / init / checkColli / ctrl | trnkomono_balls.cpp | T |
| Trn08KomonoMgrTask::ctor / ctrl | 0x9EF9E0 / 0x9F00D0 | T |
| CTrn10Komono::init / checkColli / ctrl / setHitMark (estado) | trnkomono_balls.cpp | T |
| Trn10 Mgr: seleccion de bolas (make) | trnkomono_balls.cpp | T |
| debrisStep (paso comun de piezas) | Croset 0x21B650 / Dish 0x21C5B0 | T (prueba de mi lectura) |
| CrosetKoware::initPiece / ctrl | 0x21B650 / 0x21C0E0 | T (idem) |
| DishKoware::setup / initPiece / ctrl / findRow | 0x21C5B0 | T (idem) |

Total: 36 funciones definidas en los .cpp (29 en main/balls, 7 en debris), todas con alguna comprobacion. Mas las inline de trnkomono.h (accesores de TrnTumiageColli y CTrnKomonoBase2), que no cuento.
Tablas verificadas byte a byte contra el ELF y TRNMOD.BIN en la prueba (si los ficheros no estan, se saltan con aviso).

## Pendiente / no verificado
- Pendiente (sin escribir): TrnKomono2Task::trn05Life (0x2235D0, no lo he leido entero), calcDynamicsMotion y matriz de camara de Croset/Dish (motor), dibujo, ctor/dtor de tareas de motor, resto de CTrn10Komono (getPosY, setVel, ctrl1, closeColli) y ejecucion de las rutas de camara.
- Sin verificar: asignacion seno/coseno de las rutinas VU0 func_1B09B4 / func_1B0938 (inyectadas); formulas de ajuste de nivel dificil y temblor de Trn05 salen del asm pero no se vieron en el juego; el original lee sched[15] y word[5] fuera de rango (aqui se acota); la parte de debris (Croset/Dish) la escribi tras compactar el contexto, sin releer el asm completo.
- Las pruebas comprueban mi lectura del asm, no ejecucion diferencial contra el juego original.

## Compilar y probar (desde la raiz del repo)
```
c++ -std=c++17 -Wall -o build/verify_trnkomono/trnkomono_test game/test/trnkomono_test.cpp game/trnkomono/*.cpp game/stages/komono.cpp game/stages/training.cpp game/stages/training_tables.cpp game/stages/org.cpp game/stages/org_tables.cpp game/stages_rest/*.cpp && build/verify_trnkomono/trnkomono_test
```
Resultado: `trnkomono_test: TODO OK (913 comprobaciones)`.

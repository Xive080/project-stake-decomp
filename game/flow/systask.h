// Planificador de tareas (SystemTask 0x1A4100, SystemTask2 0x1A3C40..0x1A43C0, SystemTaskLevelCheck 0x2197F0), reescrito a mano.
// SystemTask2 mantiene una lista simple de "ejecuciones" ordenada por prioridad ascendente (nodos de 0x14 B en el original:
// +0 funcion, +4 contexto, +8 prioridad, +0xC identificador de fusion vrID, +0x10 siguiente). `current` (+0x38) es el nodo en ejecucion.
// Aqui los nodos se guardan en un vector con indices (-1 = nulo); Malloc/Free del motor no se modelan.
#pragma once
#include <cstdint>
#include <vector>
namespace vn {

using ExecFn = void (*)(void* ctx);

struct ExecNode {
    ExecFn fn = nullptr; void* ctx = nullptr;   // +0, +4
    int32_t prio = 0;                           // +8 (SystemTaskLevelCheck lo lee como u16)
    const void* vrID = nullptr;                 // +0xC (puntero a un identificador; 0 = sin identificador)
    int next = -1;                              // +0x10
    bool used = false;
};

// cmp_vrID(a, b): 1 = mismo identificador. Se inyecta (funcion de biblioteca del ELF).
using VrIdCmp = int (*)(const void* a, const void* b);

enum class DelResult { Removed, ListEmptyTaskClose };

struct ExecList {
    std::vector<ExecNode> nodes;
    int head = -1;      // +0x34
    int current = -1;   // +0x38

    int alloc();        // Malloc2: reserva un nodo
    // newExec (0x1A3DC0): alta ordenada por prioridad. Si vrID != 0 y ya existe un nodo con el mismo id (cmp == 1):
    //   - misma prioridad -> se sustituye funcion y contexto en el sitio;
    //   - otra prioridad -> el nodo se desengancha y se reinserta con la nueva.
    // Empates: el nodo nuevo va DETRAS de los de igual prioridad (se inserta antes solo si prio < prio de la cabeza).
    void newExec(ExecFn fn, void* ctx, int prio, const void* vrID, VrIdCmp cmp);
    void changeExec(ExecFn fn, void* ctx);           // 0x1A3F60: cambia funcion y contexto del nodo en ejecucion
    DelResult deleteExec();                          // 0x1A3F80: quita el nodo en ejecucion; si era el unico, pide cerrar la tarea (TaskClose)
    void threadCtrl();                               // 0x1A3D70: ejecuta todos los nodos
    void threadCtrl2();                              // 0x1A4080: continua desde el siguiente al actual (tras un skipExec)
    // SystemTaskLevelCheck::threadCtrlLevelCheck (0x219840) y ...2 (0x2198E0): solo ejecuta los nodos con prio >= nivel de pausa (PauseMgr::get()).
    void threadCtrlLevelCheck(int pauseLevel);
    void threadCtrlLevelCheck2(int pauseLevel);
    int count() const;
};

// SystemTask (base): marcas de estado de la tarea (+0x2E) y contadores globales.
enum TaskFlags : uint16_t { TF_NOLINK = 1, TF_LINKED = 4, TF_TASK2 = 8, TF_STATIC = 0x10, TF_DEAD = 0x20 };
struct TaskGlobals { int32_t idCounter = 0; int32_t taskCount = 0; int32_t creatingViaNew = 1; };   // gp-0x7630, gp-0x7644, gp-0x75D8
struct SystemTaskState {
    uint16_t flags = 0; int32_t id = 0; uint32_t magic = 0;   // +0x2E, +8, +0xC
    // __ct__10SystemTaskFv (0x1A4140): asigna id incremental, marca vinculado; si no vino de operator new marca TF_STATIC.
    void construct(TaskGlobals& g);
    // __dt__10SystemTaskFv (0x1A4220): devuelve true si desenlazo la tarea de la lista (flags &1 == 0 y estaba vinculada).
    bool destruct(TaskGlobals& g);
};
}

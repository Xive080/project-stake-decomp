#include "systask.h"
namespace vn {

int ExecList::alloc() {
    for (size_t i = 0; i < nodes.size(); i++) if (!nodes[i].used) { nodes[i] = ExecNode(); nodes[i].used = true; return (int)i; }
    nodes.push_back(ExecNode()); nodes.back().used = true; return (int)nodes.size() - 1;
}

void ExecList::newExec(ExecFn fn, void* ctx, int prio, const void* vrID, VrIdCmp cmp) {
    int n = -1;
    if (vrID != nullptr) {
        int prev = -1;
        for (int c = head; c != -1; prev = c, c = nodes[c].next) {
            if (nodes[c].vrID != nullptr && cmp(nodes[c].vrID, vrID) == 1) {
                if (nodes[c].prio == prio) { nodes[c].fn = fn; nodes[c].ctx = ctx; return; }
                if (c == head) head = nodes[c].next; else nodes[prev].next = nodes[c].next;
                n = c;
                break;
            }
        }
    }
    if (n == -1) n = alloc();
    nodes[n].fn = fn; nodes[n].ctx = ctx; nodes[n].prio = prio; nodes[n].vrID = vrID;
    int a0 = head;
    if (a0 == -1 || prio < nodes[a0].prio) { nodes[n].next = a0; head = n; return; }
    int a1 = a0;
    for (a0 = nodes[a0].next; a0 != -1 && prio >= nodes[a0].prio; a0 = nodes[a0].next) a1 = a0;
    nodes[n].next = nodes[a1].next; nodes[a1].next = n;
}

void ExecList::changeExec(ExecFn fn, void* ctx) { nodes[current].fn = fn; nodes[current].ctx = ctx; }

DelResult ExecList::deleteExec() {
    int cur = current;
    if (cur == head) {
        if (nodes[cur].next == -1) return DelResult::ListEmptyTaskClose;   // TaskClose(): el original no libera aqui
        head = nodes[cur].next; nodes[cur].used = false;
        return DelResult::Removed;                                          // y RetryReturn(): el planificador repite
    }
    int prev = head;
    while (nodes[prev].next != cur) prev = nodes[prev].next;
    nodes[prev].next = nodes[cur].next; nodes[cur].used = false;
    current = prev;                                                         // luego virtual +0xC del estado
    return DelResult::Removed;
}

void ExecList::threadCtrl() {
    int c = head;
    do { current = c; nodes[c].fn(nodes[c].ctx); c = nodes[c].next; } while (c != -1);   // do-while: asume lista no vacia
}
void ExecList::threadCtrl2() {
    for (int c = nodes[current].next; c != -1; c = nodes[c].next) { current = c; nodes[c].fn(nodes[c].ctx); }
}
void ExecList::threadCtrlLevelCheck(int lvl) {
    int c = head;
    do { current = c; if ((uint16_t)nodes[c].prio >= lvl) nodes[c].fn(nodes[c].ctx); c = nodes[c].next; } while (c != -1);
}
void ExecList::threadCtrlLevelCheck2(int lvl) {
    for (int c = nodes[current].next; c != -1; c = nodes[c].next) {
        current = c; if ((uint16_t)nodes[c].prio >= lvl) nodes[c].fn(nodes[c].ctx);
    }
}
int ExecList::count() const { int n = 0; for (int c = head; c != -1; c = nodes[c].next) n++; return n; }

void SystemTaskState::construct(TaskGlobals& g) {
    flags = 0; id = g.idCounter++; magic = 0x80018691;
    if (g.creatingViaNew == 0) flags |= TF_STATIC;
    g.taskCount++;
    flags |= TF_LINKED;
    g.creatingViaNew = 0;
}
bool SystemTaskState::destruct(TaskGlobals& g) {
    bool unlinked = false;
    if (!(flags & TF_DEAD)) {
        if (flags & TF_LINKED) {
            if (!(flags & TF_NOLINK)) unlinked = true;
            id = 0; magic = 0;
        }
        g.taskCount--;
        flags |= TF_DEAD;
    }
    return unlinked;
}
}

#include "priority.h"

static size_t priority_pick_next(const SchedulerProcessView *ready,
                                 size_t ready_count,
                                 int current_time,
                                 void *context) {
    size_t best;
    size_t index;

    (void) current_time;
    (void) context;

    if (ready_count == 0) {
        return SCHEDULER_NO_SELECTION;
    }

    /*
     * Convencao (docs/cenarios-de-carga.md): menor valor numerico == maior
     * prioridade. Desempate (docs/decisoes-tecnicas.md): quem entrou
     * primeiro na fila de prontos e, por fim, menor PID.
     */
    best = 0;
    for (index = 1; index < ready_count; ++index) {
        if (ready[index].priority < ready[best].priority ||
            (ready[index].priority == ready[best].priority &&
             ready[index].ready_since < ready[best].ready_since) ||
            (ready[index].priority == ready[best].priority &&
             ready[index].ready_since == ready[best].ready_since &&
             ready[index].pid < ready[best].pid)) {
            best = index;
        }
    }

    return best;
}

static const SchedulerOps PRIORITY_OPS = {
    .name = "priority",
    .pick_next = priority_pick_next,
    .should_preempt = NULL
};

Scheduler priority_scheduler(void) {
    return (Scheduler) {.ops = &PRIORITY_OPS, .context = NULL};
}

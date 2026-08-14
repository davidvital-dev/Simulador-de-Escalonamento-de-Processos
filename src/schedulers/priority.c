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
     * Menor numero = maior prioridade. Em caso de empate, fica quem ja
     * esta na fila ha mais tempo: por isso so trocamos de "melhor" quando
     * a prioridade e de fato menor, nunca em empate.
     */
    best = 0;
    for (index = 1; index < ready_count; ++index) {
        if (ready[index].priority < ready[best].priority) {
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

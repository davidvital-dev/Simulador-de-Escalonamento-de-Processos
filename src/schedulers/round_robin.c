#include "round_robin.h"

static size_t round_robin_pick_next(const SchedulerProcessView *ready,
                                    size_t ready_count,
                                    int current_time,
                                    void *context) {
    (void) ready;
    (void) current_time;
    (void) context;

    /*
     * A propria ordem da fila circular determina o proximo processo:
     * chegadas, retornos de E/S e processos preemptados entram no final
     * conforme a ordem em que os eventos sao processados no tick
     * (docs/decisoes-tecnicas.md).
     */
    return ready_count > 0 ? 0 : SCHEDULER_NO_SELECTION;
}

static bool round_robin_should_preempt(const SchedulerProcessView *running,
                                       int slice_ticks,
                                       int current_time,
                                       void *context) {
    const RoundRobinContext *rr = context;

    (void) running;
    (void) current_time;

    return rr != NULL && slice_ticks >= rr->quantum;
}

static const SchedulerOps ROUND_ROBIN_OPS = {
    .name = "round-robin",
    .pick_next = round_robin_pick_next,
    .should_preempt = round_robin_should_preempt
};

Scheduler round_robin_scheduler(RoundRobinContext *context) {
    return (Scheduler) {.ops = &ROUND_ROBIN_OPS, .context = context};
}

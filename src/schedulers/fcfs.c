#include "fcfs.h"

static size_t fcfs_pick_next(const SchedulerProcessView *ready,
                             size_t ready_count,
                             int current_time,
                             void *context) {
    (void) ready;
    (void) current_time;
    (void) context;

    return ready_count > 0 ? 0 : SCHEDULER_NO_SELECTION;
}

static const SchedulerOps FCFS_OPS = {
    .name = "fcfs",
    .pick_next = fcfs_pick_next,
    .should_preempt = NULL
};

Scheduler fcfs_scheduler(void) {
    return (Scheduler) {.ops = &FCFS_OPS, .context = NULL};
}

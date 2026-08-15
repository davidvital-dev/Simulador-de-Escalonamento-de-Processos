#include "proposed.h"

#include <limits.h>

static int observed_burst_estimate(const SchedulerProcessView *process,
                                   const ProposedContext *context) {
    if (process->completed_cpu_bursts <= 0 || process->total_cpu_executed < 0) {
        return context->initial_burst_estimate;
    }

    return process->total_cpu_executed / process->completed_cpu_bursts;
}

static int ready_wait_time(const SchedulerProcessView *process,
                           int current_time) {
    if (process->ready_since < 0 || current_time <= process->ready_since) {
        return 0;
    }

    return current_time - process->ready_since;
}

static long long proposed_score(const SchedulerProcessView *process,
                                int current_time,
                                const ProposedContext *context) {
    const int estimate = observed_burst_estimate(process, context);
    const int wait_time = ready_wait_time(process, current_time);
    const int aging_bonus = wait_time / context->aging_interval;

    return (long long) context->priority_weight * process->priority +
           (long long) context->burst_weight * estimate -
           (long long) aging_bonus;
}

static size_t proposed_pick_next(const SchedulerProcessView *ready,
                                 size_t ready_count,
                                 int current_time,
                                 void *context) {
    const ProposedContext *proposed = context;
    size_t best;
    long long best_score;
    size_t index;

    if (ready_count == 0 || !proposed_context_is_valid(proposed)) {
        return SCHEDULER_NO_SELECTION;
    }

    best = 0;
    best_score = proposed_score(&ready[0], current_time, proposed);

    for (index = 1; index < ready_count; ++index) {
        const long long score = proposed_score(&ready[index], current_time, proposed);

        /*
         * Em empate preservamos a posição já existente no vetor READY.
         * O motor mantém essa ordem deterministicamente.
         */
        if (score < best_score) {
            best = index;
            best_score = score;
        }
    }

    return best;
}

static const SchedulerOps PROPOSED_OPS = {
    .name = "proposed",
    .pick_next = proposed_pick_next,
    .should_preempt = NULL
};

ProposedContext proposed_default_context(void) {
    return (ProposedContext) {
        .priority_weight = 8,
        .burst_weight = 1,
        .aging_interval = 4,
        .initial_burst_estimate = 8
    };
}

bool proposed_context_is_valid(const ProposedContext *context) {
    return context != NULL &&
           context->priority_weight >= 0 &&
           context->burst_weight >= 0 &&
           context->aging_interval > 0 &&
           context->initial_burst_estimate >= 0;
}

Scheduler proposed_scheduler(ProposedContext *context) {
    if (!proposed_context_is_valid(context)) {
        return (Scheduler) {.ops = NULL, .context = NULL};
    }

    return (Scheduler) {.ops = &PROPOSED_OPS, .context = context};
}

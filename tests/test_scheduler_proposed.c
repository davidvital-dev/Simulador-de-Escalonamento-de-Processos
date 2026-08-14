#include <assert.h>
#include <stdio.h>

#include "proposed.h"

static size_t pick(Scheduler *scheduler,
                   const SchedulerProcessView *ready,
                   size_t ready_count,
                   int current_time) {
    return scheduler->ops->pick_next(ready, ready_count, current_time,
                                     scheduler->context);
}

static void test_default_context_is_valid(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);

    assert(proposed_context_is_valid(&context));
    assert(context.priority_weight == 8);
    assert(context.burst_weight == 1);
    assert(context.aging_interval == 4);
    assert(context.initial_burst_estimate == 8);
    assert(scheduler.ops != NULL);
    assert(scheduler.ops->should_preempt == NULL);
}

static void test_invalid_context_is_rejected(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler;

    context.aging_interval = 0;
    assert(!proposed_context_is_valid(&context));

    scheduler = proposed_scheduler(&context);
    assert(scheduler.ops == NULL);
}

static void test_priority_matters_without_other_differences(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SchedulerProcessView ready[] = {
        {
            .process_index = 0,
            .pid = 1,
            .priority = 7,
            .ready_since = 0,
            .total_cpu_executed = 0,
            .completed_cpu_bursts = 0,
            .last_cpu_burst_duration = -1
        },
        {
            .process_index = 1,
            .pid = 2,
            .priority = 1,
            .ready_since = 0,
            .total_cpu_executed = 0,
            .completed_cpu_bursts = 0,
            .last_cpu_burst_duration = -1
        }
    };

    assert(pick(&scheduler, ready, 2, 0) == 1);
}

static void test_observed_short_history_is_favored(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SchedulerProcessView ready[] = {
        {
            .process_index = 0,
            .pid = 10,
            .priority = 3,
            .ready_since = 20,
            .total_cpu_executed = 30,
            .completed_cpu_bursts = 3,
            .last_cpu_burst_duration = 10
        },
        {
            .process_index = 1,
            .pid = 11,
            .priority = 3,
            .ready_since = 20,
            .total_cpu_executed = 6,
            .completed_cpu_bursts = 3,
            .last_cpu_burst_duration = 2
        }
    };

    /* Mesmo aging e mesma prioridade; média observada 2 vence média 10. */
    assert(pick(&scheduler, ready, 2, 20) == 1);
}

static void test_aging_eventually_overcomes_low_priority(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SchedulerProcessView ready[] = {
        {
            .process_index = 0,
            .pid = 20,
            .priority = 9,
            .ready_since = 0,
            .total_cpu_executed = 0,
            .completed_cpu_bursts = 0,
            .last_cpu_burst_duration = -1
        },
        {
            .process_index = 1,
            .pid = 21,
            .priority = 0,
            .ready_since = 400,
            .total_cpu_executed = 0,
            .completed_cpu_bursts = 0,
            .last_cpu_burst_duration = -1
        }
    };

    /*
     * Em t=400: PID 20 tem score 9*8 + 8 - 400/4 = -20;
     * PID 21 tem score 0*8 + 8 - 0 = 8.
     */
    assert(pick(&scheduler, ready, 2, 400) == 0);
}

static void test_tie_preserves_ready_queue_order(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SchedulerProcessView ready[] = {
        {
            .process_index = 7,
            .pid = 99,
            .priority = 2,
            .ready_since = 10,
            .total_cpu_executed = 8,
            .completed_cpu_bursts = 1,
            .last_cpu_burst_duration = 8
        },
        {
            .process_index = 3,
            .pid = 1,
            .priority = 2,
            .ready_since = 10,
            .total_cpu_executed = 8,
            .completed_cpu_bursts = 1,
            .last_cpu_burst_duration = 8
        }
    };

    /* PID menor não fura a ordem já estabelecida da fila. */
    assert(pick(&scheduler, ready, 2, 20) == 0);
}

static void test_empty_ready_queue_returns_no_selection(void) {
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);

    assert(pick(&scheduler, NULL, 0, 0) == SCHEDULER_NO_SELECTION);
}

int main(void) {
    test_default_context_is_valid();
    test_invalid_context_is_rejected();
    test_priority_matters_without_other_differences();
    test_observed_short_history_is_favored();
    test_aging_eventually_overcomes_low_priority();
    test_tie_preserves_ready_queue_order();
    test_empty_ready_queue_returns_no_selection();

    puts("OK: AHR combina prioridade, histórico observado e aging deterministicamente.");
    return 0;
}

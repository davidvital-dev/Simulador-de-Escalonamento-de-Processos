#include <assert.h>
#include <stdio.h>

#include "proposed.h"
#include "simulator.h"

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

static void test_simulation_aging_serves_old_low_priority_before_new_high_priority(void) {
    Burst running_bursts[] = {{BURST_CPU, 400, 400}};
    Burst old_low_bursts[] = {{BURST_CPU, 1, 1}};
    Burst new_high_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = running_bursts,
            .burst_count = 1
        },
        {
            .pid = 2,
            .arrival_time = 0,
            .priority = 9,
            .state = PROCESS_NEW,
            .bursts = old_low_bursts,
            .burst_count = 1
        },
        {
            .pid = 3,
            .arrival_time = 400,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = new_high_bursts,
            .burst_count = 1
        }
    };
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 3, &config, &scheduler, &result));

    /*
     * PID 2 esperou 400 ticks. Quando PID 1 termina, PID 3 acabou de chegar,
     * mas o aging acumulado faz PID 2 ser escolhido antes.
     */
    assert(processes[0].finish_time == 400);
    assert(processes[1].finish_time == 401);
    assert(processes[2].finish_time == 402);
}

static void test_simulation_io_bound_processes_complete(void) {
    Burst p1_bursts[] = {
        {BURST_CPU, 1, 1},
        {BURST_IO, 3, 3},
        {BURST_CPU, 1, 1}
    };
    Burst p2_bursts[] = {
        {BURST_CPU, 1, 1},
        {BURST_IO, 2, 2},
        {BURST_CPU, 1, 1}
    };
    Process processes[] = {
        {
            .pid = 10,
            .arrival_time = 0,
            .priority = 3,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 3
        },
        {
            .pid = 11,
            .arrival_time = 0,
            .priority = 3,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 3
        }
    };
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.completed_processes == 2);
    assert(processes[0].state == PROCESS_FINISHED);
    assert(processes[1].state == PROCESS_FINISHED);
    assert(processes[0].completed_cpu_bursts == 2);
    assert(processes[1].completed_cpu_bursts == 2);
}

static void test_simulation_cpu_bound_is_non_preemptive(void) {
    Burst p1_bursts[] = {{BURST_CPU, 6, 6}};
    Burst p2_bursts[] = {{BURST_CPU, 4, 4}};
    Process processes[] = {
        {
            .pid = 20,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 21,
            .arrival_time = 1,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(processes[0].finish_time == 6);
    assert(processes[1].finish_time == 10);
    assert(result.context_switches == 1);
}

static void test_first_decision_does_not_use_real_future_burst_length(void) {
    Burst long_future[] = {{BURST_CPU, 20, 20}};
    Burst short_future[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 4,
            .state = PROCESS_NEW,
            .bursts = long_future,
            .burst_count = 1
        },
        {
            .pid = 2,
            .arrival_time = 0,
            .priority = 4,
            .state = PROCESS_NEW,
            .bursts = short_future,
            .burst_count = 1
        }
    };
    ProposedContext context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /*
     * Sem histórico, ambos recebem a mesma estimativa inicial. Se o AHR
     * espionasse a rajada futura real, PID 2 (1 tick) passaria na frente.
     * Como não espiona, preserva a ordem READY e PID 1 roda primeiro.
     */
    assert(processes[0].finish_time == 20);
    assert(processes[1].finish_time == 21);
}

int main(void) {
    test_default_context_is_valid();
    test_invalid_context_is_rejected();
    test_priority_matters_without_other_differences();
    test_observed_short_history_is_favored();
    test_aging_eventually_overcomes_low_priority();
    test_tie_preserves_ready_queue_order();
    test_empty_ready_queue_returns_no_selection();
    test_simulation_aging_serves_old_low_priority_before_new_high_priority();
    test_simulation_io_bound_processes_complete();
    test_simulation_cpu_bound_is_non_preemptive();
    test_first_decision_does_not_use_real_future_burst_length();

    puts("OK: AHR validado em aging, histórico, E/S, CPU-bound e ausência de informação futura.");
    return 0;
}

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "simulator.h"

typedef struct {
    int quantum;
} RoundRobinTestContext;

static size_t pick_first(const SchedulerProcessView *ready,
                         size_t ready_count,
                         int current_time,
                         void *context) {
    (void) ready;
    (void) current_time;
    (void) context;
    return ready_count > 0 ? 0 : SCHEDULER_NO_SELECTION;
}

static bool preempt_on_quantum(const SchedulerProcessView *running,
                               const SchedulerProcessView *ready,
                               size_t ready_count,
                               int slice_ticks,
                               int current_time,
                               void *context) {
    const RoundRobinTestContext *rr = context;
    (void) running;
    (void) ready;
    (void) ready_count;
    (void) current_time;
    return rr != NULL && slice_ticks >= rr->quantum;
}

static const SchedulerOps NON_PREEMPTIVE_OPS = {
    .name = "test-non-preemptive",
    .pick_next = pick_first,
    .should_preempt = NULL
};

static const SchedulerOps PREEMPTIVE_OPS = {
    .name = "test-preemptive",
    .pick_next = pick_first,
    .should_preempt = preempt_on_quantum
};

static void test_context_switch_cost(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 2,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = {.ops = &NON_PREEMPTIVE_OPS, .context = NULL};

    assert(config.context_switch_cost == 2);
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.elapsed_time == 4);
    assert(result.completed_processes == 2);
    assert(result.context_switches == 1);
    assert(result.context_switch_ticks == 2);
    assert(result.idle_ticks == 0);
    assert(processes[0].finish_time == 1);
    assert(processes[1].finish_time == 4);
}

static void test_idle_to_process_has_no_switch_cost(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 2,
            .arrival_time = 5,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = {.ops = &NON_PREEMPTIVE_OPS, .context = NULL};

    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.elapsed_time == 6);
    assert(result.context_switches == 0);
    assert(result.context_switch_ticks == 0);
    assert(result.idle_ticks == 4);
    assert(processes[1].finish_time == 6);
}

static void test_preemption_support(void) {
    Burst p1_bursts[] = {{BURST_CPU, 3, 3}};
    Burst p2_bursts[] = {{BURST_CPU, 2, 2}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 2,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    RoundRobinTestContext rr = {.quantum = 1};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = {.ops = &PREEMPTIVE_OPS, .context = &rr};

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.completed_processes == 2);
    assert(result.elapsed_time == 5);
    assert(result.context_switches >= 3);
    assert(processes[0].state == PROCESS_FINISHED);
    assert(processes[1].state == PROCESS_FINISHED);
    assert(processes[0].total_cpu_executed == 3);
    assert(processes[1].total_cpu_executed == 2);
}

int main(void) {
    test_context_switch_cost();
    test_idle_to_process_has_no_switch_cost();
    test_preemption_support();

    puts("OK: troca de contexto, idle e preempcao validados.");
    return 0;
}

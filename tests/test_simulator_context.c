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

static size_t pick_highest_priority(const SchedulerProcessView *ready,
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

static bool preempt_on_quantum(const SchedulerProcessView *running,
                               int slice_ticks,
                               int current_time,
                               void *context) {
    const RoundRobinTestContext *rr = context;
    (void) running;
    (void) current_time;
    return rr != NULL && slice_ticks >= rr->quantum;
}

static const SchedulerOps NON_PREEMPTIVE_OPS = {
    .name = "test-non-preemptive",
    .pick_next = pick_first,
    .should_preempt = NULL
};

static const SchedulerOps PRIORITY_OPS = {
    .name = "test-priority",
    .pick_next = pick_highest_priority,
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

static void test_selected_process_survives_context_switch(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p3_bursts[] = {{BURST_CPU, 1, 1}};
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
            .priority = 5,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        },
        {
            .pid = 3,
            .arrival_time = 2,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p3_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = {.ops = &PRIORITY_OPS, .context = NULL};

    assert(simulator_run(processes, 3, &config, &scheduler, &result));

    assert(result.completed_processes == 3);
    assert(result.elapsed_time == 7);
    assert(result.context_switches == 2);
    assert(result.context_switch_ticks == 4);
    assert(processes[0].finish_time == 1);
    assert(processes[1].finish_time == 4);
    assert(processes[2].finish_time == 7);
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
    test_selected_process_survives_context_switch();
    test_preemption_support();

    puts("OK: troca de contexto, idle, selecao e preempcao validados.");
    return 0;
}

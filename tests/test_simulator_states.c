#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "simulator.h"

static size_t pick_first(const SchedulerProcessView *ready,
                         size_t ready_count,
                         int current_time,
                         void *context) {
    (void) ready;
    (void) current_time;
    (void) context;
    return ready_count > 0 ? 0 : SCHEDULER_NO_SELECTION;
}

static const SchedulerOps FCFS_TEST_OPS = {
    .name = "test-fcfs",
    .pick_next = pick_first,
    .should_preempt = NULL
};

static Scheduler test_scheduler(void) {
    return (Scheduler) {
        .ops = &FCFS_TEST_OPS,
        .context = NULL
    };
}

static void test_cpu_io_cpu_and_arrival(void) {
    Burst p1_bursts[] = {
        {BURST_CPU, 1, 1},
        {BURST_IO, 2, 2},
        {BURST_CPU, 1, 1}
    };
    Burst p2_bursts[] = {
        {BURST_CPU, 2, 2}
    };
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 3
        },
        {
            .pid = 2,
            .arrival_time = 1,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = test_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.completed_processes == 2);
    assert(result.elapsed_time == 4);
    assert(result.context_switches == 2);
    assert(result.context_switch_ticks == 0);
    assert(result.idle_ticks == 0);

    assert(processes[0].state == PROCESS_FINISHED);
    assert(processes[0].finish_time == 4);
    assert(processes[0].total_cpu_executed == 2);
    assert(processes[0].completed_cpu_bursts == 2);
    assert(processes[0].last_cpu_burst_duration == 1);

    assert(processes[1].state == PROCESS_FINISHED);
    assert(processes[1].finish_time == 3);
    assert(processes[1].total_cpu_executed == 2);
}

static void test_parallel_io(void) {
    Burst p1_bursts[] = {
        {BURST_CPU, 1, 1},
        {BURST_IO, 2, 2},
        {BURST_CPU, 1, 1}
    };
    Burst p2_bursts[] = {
        {BURST_CPU, 1, 1},
        {BURST_IO, 2, 2},
        {BURST_CPU, 1, 1}
    };
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 3
        },
        {
            .pid = 2,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 3
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = test_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.completed_processes == 2);
    assert(result.idle_ticks == 1);
    assert(processes[0].finish_time == 4);
    assert(processes[1].finish_time == 5);
    assert(p1_bursts[1].remaining_time == 0);
    assert(p2_bursts[1].remaining_time == 0);
}

static void test_initial_idle_cpu(void) {
    Burst bursts[] = {
        {BURST_CPU, 1, 1}
    };
    Process process = {
        .pid = 1,
        .arrival_time = 3,
        .priority = 0,
        .state = PROCESS_NEW,
        .bursts = bursts,
        .burst_count = 1
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = test_scheduler();

    assert(simulator_run(&process, 1, &config, &scheduler, &result));
    assert(result.elapsed_time == 4);
    assert(result.idle_ticks == 3);
    assert(result.context_switches == 0);
    assert(process.finish_time == 4);
}

int main(void) {
    test_cpu_io_cpu_and_arrival();
    test_parallel_io();
    test_initial_idle_cpu();

    puts("OK: estados, chegadas, CPU ociosa e E/S paralela validados.");
    return 0;
}

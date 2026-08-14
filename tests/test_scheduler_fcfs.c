#include <assert.h>
#include <stdio.h>

#include "fcfs.h"
#include "simulator.h"

static void test_fcfs_respects_arrival_order(void) {
    Burst p1_bursts[] = {{BURST_CPU, 3, 3}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 5,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
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
    Scheduler scheduler = fcfs_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* PID 2 chega no meio da execucao de PID 1, mas o FCFS nao interrompe
     * ninguem: PID 1 termina primeiro mesmo assim. */
    assert(processes[0].finish_time == 3);
    assert(processes[1].finish_time == 4);
    assert(result.context_switches == 1);
}

static void test_fcfs_returns_to_ready_order_after_io(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}, {BURST_IO, 5, 5}, {BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
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
            .arrival_time = 2,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = fcfs_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* PID 1 fica bloqueado em E/S ate t=6, entao PID 2 (que chegou antes
     * disso) passa na frente. */
    assert(processes[1].finish_time == 3);
    assert(processes[0].finish_time == 7);
}

static void test_fcfs_three_processes_known_order(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p3_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 30,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 10,
            .arrival_time = 1,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        },
        {
            .pid = 20,
            .arrival_time = 2,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p3_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = fcfs_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 3, &config, &scheduler, &result));

    /* Quem chega primeiro roda primeiro, o PID nao importa aqui. */
    assert(processes[0].finish_time == 1);
    assert(processes[1].finish_time == 2);
    assert(processes[2].finish_time == 3);
}

static void test_fcfs_ties_break_deterministically(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 9,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 3,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = fcfs_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* Chegando junto, quem tem o PID menor entra primeiro na fila e roda
     * primeiro, sempre. */
    assert(processes[1].finish_time == 1);
    assert(processes[0].finish_time == 2);
}

int main(void) {
    test_fcfs_respects_arrival_order();
    test_fcfs_returns_to_ready_order_after_io();
    test_fcfs_three_processes_known_order();
    test_fcfs_ties_break_deterministically();

    puts("OK: FCFS respeita ordem de entrada na fila de prontos, sem preempcao.");
    return 0;
}

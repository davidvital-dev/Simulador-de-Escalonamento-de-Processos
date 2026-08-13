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

    /* PID 2 chega antes do fim da rajada de PID 1, mas o FCFS nao preempta:
     * PID 1 continua ate terminar, mesmo tendo prioridade "pior". */
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

    /* PID 1 bloqueia em t=1 e so retorna a fila em t=6, entao PID 2 (chegado
     * em t=2) executa primeiro por ordem de entrada na fila de prontos. */
    assert(processes[1].finish_time == 3);
    assert(processes[0].finish_time == 7);
}

int main(void) {
    test_fcfs_respects_arrival_order();
    test_fcfs_returns_to_ready_order_after_io();

    puts("OK: FCFS respeita ordem de entrada na fila de prontos, sem preempcao.");
    return 0;
}

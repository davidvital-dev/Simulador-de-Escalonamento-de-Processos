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

    /* A ordem e definida pela chegada, nao pelo PID: PID 30 (t=0), depois
     * PID 10 (t=1), depois PID 20 (t=2). */
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

    /* Mesma chegada: o desempate (docs/decisoes-tecnicas.md) cai no menor
     * PID, que e admitido primeiro na fila de prontos. PID 3 (indice 1)
     * executa antes de PID 9 (indice 0), sempre na mesma ordem. */
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

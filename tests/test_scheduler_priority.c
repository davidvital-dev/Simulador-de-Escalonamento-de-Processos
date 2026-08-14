#include <assert.h>
#include <stdio.h>

#include "priority.h"
#include "simulator.h"

static void test_priority_selects_lower_numeric_value_first(void) {
    /* Convencao: 0 e a maior prioridade. */
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p3_bursts[] = {{BURST_CPU, 1, 1}};
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
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        },
        {
            .pid = 3,
            .arrival_time = 0,
            .priority = 2,
            .state = PROCESS_NEW,
            .bursts = p3_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = priority_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 3, &config, &scheduler, &result));

    /* Roda na ordem de prioridade: PID 2, depois PID 3, depois PID 1. */
    assert(processes[1].finish_time == 1);
    assert(processes[2].finish_time == 2);
    assert(processes[0].finish_time == 3);
}

static void test_priority_does_not_preempt_running_process(void) {
    Burst p1_bursts[] = {{BURST_CPU, 5, 5}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 1,
            .arrival_time = 0,
            .priority = 9,
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
    Scheduler scheduler = priority_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* PID 2 chega com prioridade melhor, mas ninguem tira PID 1 da CPU. */
    assert(processes[0].finish_time == 5);
    assert(processes[1].finish_time == 6);
    assert(result.context_switches == 1);
}

static void test_priority_ties_break_by_arrival_then_pid(void) {
    Burst p1_bursts[] = {{BURST_CPU, 1, 1}};
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 5,
            .arrival_time = 0,
            .priority = 3,
            .state = PROCESS_NEW,
            .bursts = p1_bursts,
            .burst_count = 1
        },
        {
            .pid = 2,
            .arrival_time = 0,
            .priority = 3,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = priority_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* Mesma prioridade e mesma chegada: quem tem o PID menor entra
     * primeiro na fila e roda primeiro. */
    assert(processes[1].finish_time == 1);
    assert(processes[0].finish_time == 2);
}

static void test_priority_same_tick_io_completion_beats_new_arrival(void) {
    /*
     * Caso encontrado na revisao: um processo volta da E/S bem na hora que
     * outro chega, com a mesma prioridade. Quem voltou da E/S entrou
     * primeiro na fila e deve rodar antes, mesmo com PID maior.
     */
    Burst p_bursts[] = {
        {BURST_CPU, 2, 2},
        {BURST_IO, 3, 3},
        {BURST_CPU, 1, 1}
    };
    Burst q_bursts[] = {{BURST_CPU, 1, 1}};
    Process processes[] = {
        {
            .pid = 9,
            .arrival_time = 0,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p_bursts,
            .burst_count = 3
        },
        {
            .pid = 1,
            .arrival_time = 5,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = q_bursts,
            .burst_count = 1
        }
    };
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = priority_scheduler();

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* PID 9 volta da E/S no exato instante em que PID 1 chega; por ter
     * entrado primeiro na fila, PID 9 roda antes. */
    assert(processes[0].finish_time == 6);
    assert(processes[1].finish_time == 7);
}

int main(void) {
    test_priority_selects_lower_numeric_value_first();
    test_priority_does_not_preempt_running_process();
    test_priority_ties_break_by_arrival_then_pid();
    test_priority_same_tick_io_completion_beats_new_arrival();

    puts("OK: Prioridade nao preemptiva, convencao e desempate validados.");
    return 0;
}

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

    /* Ordem esperada: PID 2 (prioridade 0), PID 3 (prioridade 2), PID 1
     * (prioridade 5). */
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

    /* PID 2 chega em t=1 com prioridade melhor, mas PID 1 (em execucao)
     * so libera a CPU ao terminar sua rajada em t=5. */
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

    /* Mesma prioridade e mesma chegada: o desempate final e o menor PID.
     * PID 2 (indice 1) vence e executa antes de PID 5 (indice 0). */
    assert(processes[1].finish_time == 1);
    assert(processes[0].finish_time == 2);
}

int main(void) {
    test_priority_selects_lower_numeric_value_first();
    test_priority_does_not_preempt_running_process();
    test_priority_ties_break_by_arrival_then_pid();

    puts("OK: Prioridade nao preemptiva, convencao e desempate validados.");
    return 0;
}

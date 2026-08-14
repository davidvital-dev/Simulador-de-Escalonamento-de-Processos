#include <assert.h>
#include <stdio.h>

#include "round_robin.h"
#include "simulator.h"

static void test_round_robin_rotates_on_quantum_expiry(void) {
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
    RoundRobinContext ctx = {.quantum = 1};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = round_robin_scheduler(&ctx);

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(result.completed_processes == 2);
    assert(result.elapsed_time == 5);
    assert(result.context_switches >= 3);
    assert(processes[0].total_cpu_executed == 3);
    assert(processes[1].total_cpu_executed == 2);
}

static void test_round_robin_quantum_is_configurable(void) {
    /* Com um quantum bem grande, ninguem chega a ser interrompido. */
    Burst p1_bursts[] = {{BURST_CPU, 5, 5}};
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
            .arrival_time = 1,
            .priority = 0,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    RoundRobinContext ctx = {.quantum = 10};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = round_robin_scheduler(&ctx);

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(processes[0].finish_time == 5);
    assert(processes[1].finish_time == 6);
    assert(result.context_switches == 1);
}

static void test_round_robin_quantum_two_alternates_processes(void) {
    Burst p1_bursts[] = {{BURST_CPU, 4, 4}};
    Burst p2_bursts[] = {{BURST_CPU, 4, 4}};
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
    RoundRobinContext ctx = {.quantum = 2};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = round_robin_scheduler(&ctx);

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* Os dois se revezam a cada 2 ticks ate terminar. */
    assert(result.context_switches == 3);
    assert(processes[0].finish_time == 6);
    assert(processes[1].finish_time == 8);
    assert(processes[0].total_cpu_executed == 4);
    assert(processes[1].total_cpu_executed == 4);
}

static void test_round_robin_handles_io_during_execution(void) {
    Burst p1_bursts[] = {
        {BURST_CPU, 2, 2},
        {BURST_IO, 3, 3},
        {BURST_CPU, 1, 1}
    };
    Burst p2_bursts[] = {{BURST_CPU, 3, 3}};
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
            .burst_count = 1
        }
    };
    RoundRobinContext ctx = {.quantum = 2};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = round_robin_scheduler(&ctx);

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* PID 1 roda um pouco e vai pra E/S; PID 2 assume a CPU enquanto isso
     * e, quando PID 1 volta, os dois terminam em sequencia. */
    assert(result.completed_processes == 2);
    assert(processes[1].finish_time == 5);
    assert(processes[0].finish_time == 6);
    assert(processes[0].total_cpu_executed == 3);
    assert(processes[1].total_cpu_executed == 3);
    assert(result.context_switches == 2);
}

static void test_round_robin_no_artificial_preemption_at_quantum_boundary(void) {
    Burst p1_bursts[] = {{BURST_CPU, 2, 2}};
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
    /* PID 1 termina bem na hora que o quantum dele acaba. */
    RoundRobinContext ctx = {.quantum = 2};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = round_robin_scheduler(&ctx);

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    assert(processes[0].finish_time == 2);
    assert(processes[1].finish_time == 3);
    /* Se tivesse preempcao artificial aqui, apareceria uma troca extra. */
    assert(result.context_switches == 1);
}

static void test_round_robin_ties_break_deterministically(void) {
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
    RoundRobinContext ctx = {.quantum = 5};
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = round_robin_scheduler(&ctx);

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* Chegando junto, o PID menor entra primeiro e roda primeiro. */
    assert(processes[1].finish_time == 1);
    assert(processes[0].finish_time == 2);
}

int main(void) {
    test_round_robin_rotates_on_quantum_expiry();
    test_round_robin_quantum_is_configurable();
    test_round_robin_quantum_two_alternates_processes();
    test_round_robin_handles_io_during_execution();
    test_round_robin_no_artificial_preemption_at_quantum_boundary();
    test_round_robin_ties_break_deterministically();

    puts("OK: Round Robin roda em fatias de tempo com quantum configuravel.");
    return 0;
}

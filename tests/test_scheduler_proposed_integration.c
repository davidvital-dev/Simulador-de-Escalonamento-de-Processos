#include <assert.h>
#include <stdio.h>

#include "proposed.h"
#include "simulator.h"
#include "workload.h"

static void test_proposed_is_non_preemptive(void) {
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
    ProposedContext proposed_context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&proposed_context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /* PID 2 chega com score melhor, mas AHR nao interrompe PID 1. */
    assert(processes[0].finish_time == 5);
    assert(processes[1].finish_time == 6);
    assert(result.completed_processes == 2);
}

static void test_observed_history_affects_dispatch_after_io(void) {
    Burst p1_bursts[] = {
        {BURST_CPU, 1, 1},
        {BURST_IO, 2, 2},
        {BURST_CPU, 1, 1}
    };
    Burst p2_bursts[] = {{BURST_CPU, 1, 1}};
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
            .arrival_time = 3,
            .priority = 3,
            .state = PROCESS_NEW,
            .bursts = p2_bursts,
            .burst_count = 1
        }
    };
    ProposedContext proposed_context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&proposed_context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;

    config.context_switch_cost = 0;
    assert(simulator_run(processes, 2, &config, &scheduler, &result));

    /*
     * Em t=3, PID 10 retorna da E/S com historico observado de 1 tick e
     * PID 11 chega sem historico (estimativa inicial 8). Mesma prioridade e
     * mesmo tempo de espera naquele instante: PID 10 deve ser escolhido.
     */
    assert(processes[0].finish_time == 4);
    assert(processes[1].finish_time == 5);
    assert(processes[0].completed_cpu_bursts == 2);
}

static void assert_scenario_completes(ScenarioType type, uint64_t seed) {
    ScenarioConfig scenario;
    Workload workload = {0};
    ProposedContext proposed_context = proposed_default_context();
    Scheduler scheduler = proposed_scheduler(&proposed_context);
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    size_t index;

    assert(workload_default_config(type, &scenario));
    assert(workload_generate(seed, &scenario, 64, &workload));
    assert(simulator_run(workload.processes, workload.process_count,
                         &config, &scheduler, &result));

    assert(result.completed_processes == workload.process_count);
    for (index = 0; index < workload.process_count; ++index) {
        assert(workload.processes[index].state == PROCESS_FINISHED);
        assert(workload.processes[index].finish_time >=
               workload.processes[index].arrival_time);
    }

    workload_free(&workload);
}

static void test_all_required_scenarios_complete(void) {
    assert_scenario_completes(SCENARIO_BALANCED, 17);
    assert_scenario_completes(SCENARIO_IO_BOUND, 17);
    assert_scenario_completes(SCENARIO_CPU_BOUND, 17);
    assert_scenario_completes(SCENARIO_PRIORITY_IMBALANCED, 17);
}

static void test_same_seed_is_deterministic_with_proposed(void) {
    ScenarioConfig scenario;
    Workload first = {0};
    Workload second = {0};
    ProposedContext first_context = proposed_default_context();
    ProposedContext second_context = proposed_default_context();
    Scheduler first_scheduler = proposed_scheduler(&first_context);
    Scheduler second_scheduler = proposed_scheduler(&second_context);
    SimulationConfig config = simulator_default_config();
    SimulationResult first_result;
    SimulationResult second_result;
    size_t index;

    assert(workload_default_config(SCENARIO_PRIORITY_IMBALANCED, &scenario));
    assert(workload_generate(12345, &scenario, 48, &first));
    assert(workload_generate(12345, &scenario, 48, &second));

    assert(simulator_run(first.processes, first.process_count,
                         &config, &first_scheduler, &first_result));
    assert(simulator_run(second.processes, second.process_count,
                         &config, &second_scheduler, &second_result));

    assert(first_result.elapsed_time == second_result.elapsed_time);
    assert(first_result.completed_processes == second_result.completed_processes);
    assert(first_result.context_switches == second_result.context_switches);
    assert(first_result.context_switch_ticks == second_result.context_switch_ticks);
    assert(first_result.idle_ticks == second_result.idle_ticks);

    for (index = 0; index < first.process_count; ++index) {
        assert(first.processes[index].finish_time == second.processes[index].finish_time);
        assert(first.processes[index].total_cpu_executed ==
               second.processes[index].total_cpu_executed);
        assert(first.processes[index].completed_cpu_bursts ==
               second.processes[index].completed_cpu_bursts);
    }

    workload_free(&first);
    workload_free(&second);
}

int main(void) {
    test_proposed_is_non_preemptive();
    test_observed_history_affects_dispatch_after_io();
    test_all_required_scenarios_complete();
    test_same_seed_is_deterministic_with_proposed();

    puts("OK: AHR integrado ao motor, deterministico e valido nos quatro cenarios.");
    return 0;
}

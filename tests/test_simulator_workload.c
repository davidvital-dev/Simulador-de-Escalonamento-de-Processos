#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "simulator.h"
#include "workload.h"

static size_t pick_first(const SchedulerProcessView *ready,
                         size_t ready_count,
                         int current_time,
                         void *context) {
    (void) ready;
    (void) current_time;
    (void) context;
    return ready_count > 0 ? 0 : SCHEDULER_NO_SELECTION;
}

static const SchedulerOps TEST_OPS = {
    .name = "integration-fcfs",
    .pick_next = pick_first,
    .should_preempt = NULL
};

static void test_scenario(ScenarioType type) {
    ScenarioConfig scenario;
    Workload workload = {0};
    Process *simulation_copy = NULL;
    SimulationConfig config = simulator_default_config();
    SimulationResult result;
    Scheduler scheduler = {.ops = &TEST_OPS, .context = NULL};
    size_t index;

    assert(workload_default_config(type, &scenario));
    assert(workload_generate(20260812, &scenario, 1000, &workload));
    assert(process_clone_array(workload.processes, workload.process_count,
                               &simulation_copy));

    config.context_switch_cost = 0;
    assert(simulator_run(simulation_copy, workload.process_count,
                         &config, &scheduler, &result));

    assert(result.completed_processes == workload.process_count);
    assert(result.elapsed_time > 0);

    for (index = 0; index < workload.process_count; ++index) {
        assert(simulation_copy[index].state == PROCESS_FINISHED);
        assert(simulation_copy[index].finish_time >=
               simulation_copy[index].arrival_time);
        assert(simulation_copy[index].bursts[0].duration ==
               workload.processes[index].bursts[0].duration);
    }

    process_array_destroy(simulation_copy, workload.process_count);
    workload_free(&workload);
}

int main(void) {
    ScenarioType type;

    for (type = SCENARIO_BALANCED; type < SCENARIO_COUNT; ++type) {
        test_scenario(type);
    }

    puts("OK: motor executou 1000 processos nos quatro cenarios.");
    return 0;
}

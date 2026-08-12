#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "workload.h"

static bool value_is_in_range(int value, IntRange range) {
    return value >= range.min && value <= range.max;
}

static void assert_process_is_valid(const Process *process,
                                    const Process *previous,
                                    const ScenarioConfig *config,
                                    size_t expected_index) {
    size_t burst_index;
    size_t cpu_burst_count = 0;

    assert(process->pid == (int) expected_index + 1);
    assert(process->state == PROCESS_NEW);
    assert(value_is_in_range(process->priority, config->priority));
    assert(process->current_burst_index == 0);
    assert(process->bursts != NULL);
    assert(process->burst_count > 0);
    assert(process->burst_count % 2 == 1);

    if (previous != NULL) {
        const int interval = process->arrival_time - previous->arrival_time;
        assert(value_is_in_range(interval, config->interarrival_time));
    } else {
        assert(process->arrival_time == 0);
    }

    for (burst_index = 0; burst_index < process->burst_count; ++burst_index) {
        const Burst *burst = &process->bursts[burst_index];
        const bool is_cpu = burst_index % 2 == 0;

        assert(burst->type == (is_cpu ? BURST_CPU : BURST_IO));
        assert(burst->remaining_time == burst->duration);
        assert(value_is_in_range(
            burst->duration,
            is_cpu ? config->cpu_burst_duration : config->io_burst_duration));

        if (is_cpu) {
            ++cpu_burst_count;
        }
    }

    assert(value_is_in_range((int) cpu_burst_count,
                             config->cpu_burst_count));
    assert(process->bursts[0].type == BURST_CPU);
    assert(process->bursts[process->burst_count - 1].type == BURST_CPU);
}

static void test_scenario(ScenarioType type) {
    ScenarioConfig config;
    Workload workload = {0};
    size_t index;

    assert(workload_default_config(type, &config));
    assert(workload_generate(20260812, &config, 1000, &workload));

    for (index = 0; index < workload.process_count; ++index) {
        const Process *previous = index > 0
            ? &workload.processes[index - 1]
            : NULL;
        assert_process_is_valid(&workload.processes[index], previous,
                                &config, index);
    }

    workload_free(&workload);
    assert(workload.processes == NULL);
    assert(workload.process_count == 0);
}

int main(void) {
    ScenarioType type;

    for (type = SCENARIO_BALANCED; type < SCENARIO_COUNT; ++type) {
        test_scenario(type);
    }

    puts("OK: PID, chegada, prioridade e rajadas validados.");
    return 0;
}

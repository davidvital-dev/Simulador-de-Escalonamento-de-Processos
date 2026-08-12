#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "workload.h"

static void test_fixed_custom_intervals(void) {
    const ScenarioConfig config = {
        .type = SCENARIO_BALANCED,
        .interarrival_time = {2, 2},
        .priority = {7, 7},
        .cpu_burst_count = {3, 3},
        .cpu_burst_duration = {11, 11},
        .io_burst_duration = {13, 13},
        .priority_bias_percent = 0,
        .biased_priority = {0, 0}
    };
    Workload workload = {0};
    size_t process_index;

    assert(workload_config_is_valid(&config));
    assert(workload_generate(123, &config, 20, &workload));

    for (process_index = 0;
         process_index < workload.process_count;
         ++process_index) {
        const Process *process = &workload.processes[process_index];
        size_t burst_index;

        assert(process->arrival_time == (int) process_index * 2);
        assert(process->priority == 7);
        assert(process->burst_count == 5);

        for (burst_index = 0;
             burst_index < process->burst_count;
             ++burst_index) {
            const bool is_cpu = burst_index % 2 == 0;
            const Burst *burst = &process->bursts[burst_index];

            assert(burst->type == (is_cpu ? BURST_CPU : BURST_IO));
            assert(burst->duration == (is_cpu ? 11 : 13));
            assert(burst->remaining_time == burst->duration);
        }
    }

    workload_free(&workload);
}

static void test_custom_priority_bias(void) {
    ScenarioConfig config;
    Workload workload = {0};
    size_t index;

    assert(workload_default_config(SCENARIO_PRIORITY_IMBALANCED, &config));
    config.priority_bias_percent = 100;
    config.biased_priority = (IntRange) {4, 4};
    assert(workload_config_is_valid(&config));
    assert(workload_generate(456, &config, 100, &workload));

    for (index = 0; index < workload.process_count; ++index) {
        assert(workload.processes[index].priority == 4);
    }

    workload_free(&workload);
}

static void test_invalid_configs(void) {
    ScenarioConfig config;

    assert(workload_default_config(SCENARIO_BALANCED, &config));
    config.cpu_burst_duration = (IntRange) {10, 5};
    assert(!workload_config_is_valid(&config));

    assert(workload_default_config(SCENARIO_BALANCED, &config));
    config.io_burst_duration.min = 0;
    assert(!workload_config_is_valid(&config));

    assert(workload_default_config(SCENARIO_BALANCED, &config));
    config.priority_bias_percent = 101;
    assert(!workload_config_is_valid(&config));

    assert(workload_default_config(SCENARIO_BALANCED, &config));
    config.priority_bias_percent = 50;
    config.biased_priority = (IntRange) {8, 10};
    assert(!workload_config_is_valid(&config));
}

int main(void) {
    test_fixed_custom_intervals();
    test_custom_priority_bias();
    test_invalid_configs();

    puts("OK: configuracao personalizada validada.");
    return 0;
}

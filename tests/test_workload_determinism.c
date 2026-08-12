#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "workload.h"

enum {
    DETERMINISM_PROCESS_COUNT = 64,
    EXPERIMENT_SEED_COUNT = 100
};

static bool ranges_are_equal(IntRange left, IntRange right) {
    return left.min == right.min && left.max == right.max;
}

static bool configs_are_equal(const ScenarioConfig *left,
                              const ScenarioConfig *right) {
    return left->type == right->type &&
           ranges_are_equal(left->interarrival_time,
                            right->interarrival_time) &&
           ranges_are_equal(left->priority, right->priority) &&
           ranges_are_equal(left->cpu_burst_count,
                            right->cpu_burst_count) &&
           ranges_are_equal(left->cpu_burst_duration,
                            right->cpu_burst_duration) &&
           ranges_are_equal(left->io_burst_duration,
                            right->io_burst_duration) &&
           left->priority_bias_percent == right->priority_bias_percent &&
           ranges_are_equal(left->biased_priority, right->biased_priority);
}

static bool processes_are_equal(const Process *left, const Process *right) {
    size_t index;

    if (left->pid != right->pid ||
        left->arrival_time != right->arrival_time ||
        left->priority != right->priority ||
        left->state != right->state ||
        left->burst_count != right->burst_count ||
        left->current_burst_index != right->current_burst_index) {
        return false;
    }

    for (index = 0; index < left->burst_count; ++index) {
        if (left->bursts[index].type != right->bursts[index].type ||
            left->bursts[index].duration != right->bursts[index].duration ||
            left->bursts[index].remaining_time !=
                right->bursts[index].remaining_time) {
            return false;
        }
    }

    return true;
}

static bool workloads_are_equal(const Workload *left, const Workload *right) {
    size_t index;

    if (left->process_count != right->process_count ||
        left->seed != right->seed ||
        !configs_are_equal(&left->config, &right->config)) {
        return false;
    }

    for (index = 0; index < left->process_count; ++index) {
        if (!processes_are_equal(&left->processes[index],
                                 &right->processes[index])) {
            return false;
        }
    }

    return true;
}

static bool generated_processes_differ(const Workload *left,
                                       const Workload *right) {
    size_t index;

    if (left->process_count != right->process_count) {
        return true;
    }

    for (index = 0; index < left->process_count; ++index) {
        if (!processes_are_equal(&left->processes[index],
                                 &right->processes[index])) {
            return true;
        }
    }

    return false;
}

static void test_scenario_seed(ScenarioType type, uint64_t seed) {
    ScenarioConfig config;
    Workload first = {0};
    Workload intervening = {0};
    Workload repeated = {0};

    assert(workload_default_config(type, &config));
    assert(workload_generate(seed, &config,
                             DETERMINISM_PROCESS_COUNT, &first));

    /* Uma geração intermediária não pode alterar o resultado da repetição. */
    assert(workload_generate(seed + 1, &config,
                             DETERMINISM_PROCESS_COUNT, &intervening));
    assert(workload_generate(seed, &config,
                             DETERMINISM_PROCESS_COUNT, &repeated));

    assert(workloads_are_equal(&first, &repeated));
    assert(generated_processes_differ(&first, &intervening));

    workload_free(&first);
    workload_free(&intervening);
    workload_free(&repeated);
}

static void test_zero_seed(void) {
    ScenarioConfig config;
    Workload first = {0};
    Workload repeated = {0};

    assert(workload_default_config(SCENARIO_BALANCED, &config));
    assert(workload_generate(0, &config, 32, &first));
    assert(workload_generate(0, &config, 32, &repeated));
    assert(workloads_are_equal(&first, &repeated));

    workload_free(&first);
    workload_free(&repeated);
}

int main(void) {
    ScenarioType type;
    uint64_t seed;

    for (type = SCENARIO_BALANCED; type < SCENARIO_COUNT; ++type) {
        for (seed = 1; seed <= EXPERIMENT_SEED_COUNT; ++seed) {
            test_scenario_seed(type, seed);
        }
    }
    test_zero_seed();

    puts("OK: 100 seeds deterministicas validadas nos quatro cenarios.");
    return 0;
}

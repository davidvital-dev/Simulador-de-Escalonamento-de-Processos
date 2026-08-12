#include "workload.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

static bool range_is_non_negative(IntRange range) {
    return range.min >= 0 && range.min <= range.max;
}

static bool range_is_positive(IntRange range) {
    return range.min > 0 && range.min <= range.max;
}

/* SplitMix64 evita o estado global e as diferenças de implementação de rand(). */
static uint64_t random_next(uint64_t *state) {
    uint64_t value;

    *state += UINT64_C(0x9E3779B97F4A7C15);
    value = *state;
    value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
    return value ^ (value >> 31);
}

static int random_in_range(uint64_t *state, IntRange range) {
    uint64_t value;
    const uint64_t span = (uint64_t) (range.max - range.min) + 1;
    const uint64_t threshold = (UINT64_C(0) - span) % span;

    do {
        value = random_next(state);
    } while (value < threshold);

    return range.min + (int) (value % span);
}

static int random_priority(uint64_t *state, const ScenarioConfig *config) {
    if (config->priority_bias_percent > 0) {
        const IntRange percent_range = {1, 100};
        if (random_in_range(state, percent_range) <=
            config->priority_bias_percent) {
            return random_in_range(state, config->biased_priority);
        }
    }

    return random_in_range(state, config->priority);
}

bool workload_default_config(ScenarioType type, ScenarioConfig *config) {
    if (config == NULL) {
        return false;
    }

    switch (type) {
        case SCENARIO_BALANCED:
            *config = (ScenarioConfig) {
                .type = type,
                .interarrival_time = {0, 3},
                .priority = {0, 9},
                .cpu_burst_count = {1, 5},
                .cpu_burst_duration = {4, 12},
                .io_burst_duration = {4, 12},
                .priority_bias_percent = 0,
                .biased_priority = {0, 0}
            };
            return true;

        case SCENARIO_IO_BOUND:
            *config = (ScenarioConfig) {
                .type = type,
                .interarrival_time = {0, 3},
                .priority = {0, 9},
                .cpu_burst_count = {2, 6},
                .cpu_burst_duration = {1, 4},
                .io_burst_duration = {10, 30},
                .priority_bias_percent = 0,
                .biased_priority = {0, 0}
            };
            return true;

        case SCENARIO_CPU_BOUND:
            *config = (ScenarioConfig) {
                .type = type,
                .interarrival_time = {0, 3},
                .priority = {0, 9},
                .cpu_burst_count = {1, 3},
                .cpu_burst_duration = {15, 40},
                .io_burst_duration = {1, 5},
                .priority_bias_percent = 0,
                .biased_priority = {0, 0}
            };
            return true;

        case SCENARIO_PRIORITY_IMBALANCED:
            *config = (ScenarioConfig) {
                .type = type,
                .interarrival_time = {0, 3},
                .priority = {0, 9},
                .cpu_burst_count = {1, 5},
                .cpu_burst_duration = {4, 12},
                .io_burst_duration = {4, 12},
                .priority_bias_percent = 80,
                .biased_priority = {0, 1}
            };
            return true;

        case SCENARIO_COUNT:
            return false;
    }

    return false;
}

bool workload_config_is_valid(const ScenarioConfig *config) {
    if (config == NULL || config->type < SCENARIO_BALANCED ||
        config->type >= SCENARIO_COUNT) {
        return false;
    }

    if (!range_is_non_negative(config->interarrival_time) ||
        !range_is_non_negative(config->priority) ||
        !range_is_positive(config->cpu_burst_count) ||
        !range_is_positive(config->cpu_burst_duration) ||
        !range_is_positive(config->io_burst_duration)) {
        return false;
    }

    if (config->priority_bias_percent < 0 ||
        config->priority_bias_percent > 100) {
        return false;
    }

    if (config->priority_bias_percent == 0) {
        return true;
    }

    return range_is_non_negative(config->biased_priority) &&
           config->biased_priority.min >= config->priority.min &&
           config->biased_priority.max <= config->priority.max;
}

const char *workload_scenario_name(ScenarioType type) {
    switch (type) {
        case SCENARIO_BALANCED:
            return "balanced";
        case SCENARIO_IO_BOUND:
            return "io-bound";
        case SCENARIO_CPU_BOUND:
            return "cpu-bound";
        case SCENARIO_PRIORITY_IMBALANCED:
            return "priority-imbalanced";
        case SCENARIO_COUNT:
            return "invalid";
    }

    return "invalid";
}

bool workload_generate(uint64_t seed, const ScenarioConfig *config,
                       size_t process_count, Workload *workload) {
    Process *processes;
    uint64_t random_state = seed;
    int arrival_time = 0;
    size_t index;

    if (workload == NULL || !workload_config_is_valid(config) ||
        process_count == 0 || process_count > (size_t) INT_MAX) {
        return false;
    }

    if (config->interarrival_time.max > 0 &&
        process_count - 1 >
            (size_t) INT_MAX / (size_t) config->interarrival_time.max) {
        return false;
    }

    processes = calloc(process_count, sizeof(*processes));
    if (processes == NULL) {
        return false;
    }

    for (index = 0; index < process_count; ++index) {
        if (index > 0) {
            arrival_time += random_in_range(&random_state,
                                            config->interarrival_time);
        }

        processes[index] = (Process) {
            .pid = (int) index + 1,
            .arrival_time = arrival_time,
            .priority = random_priority(&random_state, config),
            .state = PROCESS_NEW
        };
    }

    *workload = (Workload) {
        .processes = processes,
        .process_count = process_count,
        .seed = seed,
        .config = *config
    };
    return true;
}

void workload_free(Workload *workload) {
    if (workload == NULL) {
        return;
    }

    free(workload->processes);
    *workload = (Workload) {0};
}

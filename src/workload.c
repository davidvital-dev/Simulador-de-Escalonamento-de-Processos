#include "workload.h"

#include <stddef.h>

static bool range_is_non_negative(IntRange range) {
    return range.min >= 0 && range.min <= range.max;
}

static bool range_is_positive(IntRange range) {
    return range.min > 0 && range.min <= range.max;
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

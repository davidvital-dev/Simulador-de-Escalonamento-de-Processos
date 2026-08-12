#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "workload.h"

enum {
    SAMPLE_PROCESS_COUNT = 10000
};

typedef struct {
    uint64_t cpu_duration_sum;
    uint64_t io_duration_sum;
    size_t cpu_burst_count;
    size_t io_burst_count;
    size_t high_priority_count;
} ScenarioStats;

static ScenarioStats collect_stats(const Workload *workload) {
    ScenarioStats stats = {0};
    size_t process_index;

    for (process_index = 0;
         process_index < workload->process_count;
         ++process_index) {
        const Process *process = &workload->processes[process_index];
        size_t burst_index;

        if (process->priority <= 1) {
            ++stats.high_priority_count;
        }

        for (burst_index = 0;
             burst_index < process->burst_count;
             ++burst_index) {
            const Burst *burst = &process->bursts[burst_index];

            if (burst->type == BURST_CPU) {
                stats.cpu_duration_sum += (uint64_t) burst->duration;
                ++stats.cpu_burst_count;
            } else {
                stats.io_duration_sum += (uint64_t) burst->duration;
                ++stats.io_burst_count;
            }
        }
    }

    return stats;
}

static double mean(uint64_t sum, size_t count) {
    assert(count > 0);
    return (double) sum / (double) count;
}

static Workload generate_scenario(ScenarioType type) {
    ScenarioConfig config;
    Workload workload = {0};

    assert(workload_default_config(type, &config));
    assert(workload_config_is_valid(&config));
    assert(workload_generate(UINT64_C(20260812), &config,
                             SAMPLE_PROCESS_COUNT, &workload));
    return workload;
}

static void test_scenario_names(void) {
    assert(strcmp(workload_scenario_name(SCENARIO_BALANCED),
                  "balanced") == 0);
    assert(strcmp(workload_scenario_name(SCENARIO_IO_BOUND),
                  "io-bound") == 0);
    assert(strcmp(workload_scenario_name(SCENARIO_CPU_BOUND),
                  "cpu-bound") == 0);
    assert(strcmp(workload_scenario_name(SCENARIO_PRIORITY_IMBALANCED),
                  "priority-imbalanced") == 0);
}

static void test_balanced_profile(void) {
    Workload workload = generate_scenario(SCENARIO_BALANCED);
    const ScenarioStats stats = collect_stats(&workload);
    const double cpu_mean = mean(stats.cpu_duration_sum,
                                 stats.cpu_burst_count);
    const double io_mean = mean(stats.io_duration_sum,
                                stats.io_burst_count);
    const double high_priority_share =
        (double) stats.high_priority_count / (double) workload.process_count;

    assert(cpu_mean >= 0.90 * io_mean);
    assert(cpu_mean <= 1.10 * io_mean);
    assert(high_priority_share >= 0.15);
    assert(high_priority_share <= 0.25);

    workload_free(&workload);
}

static void test_io_bound_profile(void) {
    Workload workload = generate_scenario(SCENARIO_IO_BOUND);
    const ScenarioStats stats = collect_stats(&workload);
    const double cpu_mean = mean(stats.cpu_duration_sum,
                                 stats.cpu_burst_count);
    const double io_mean = mean(stats.io_duration_sum,
                                stats.io_burst_count);

    assert(io_mean > 4.0 * cpu_mean);
    assert(workload.config.cpu_burst_count.min >= 2);

    workload_free(&workload);
}

static void test_cpu_bound_profile(void) {
    Workload workload = generate_scenario(SCENARIO_CPU_BOUND);
    const ScenarioStats stats = collect_stats(&workload);
    const double cpu_mean = mean(stats.cpu_duration_sum,
                                 stats.cpu_burst_count);
    const double io_mean = mean(stats.io_duration_sum,
                                stats.io_burst_count);

    assert(cpu_mean > 5.0 * io_mean);

    workload_free(&workload);
}

static void test_priority_imbalanced_profile(void) {
    Workload workload = generate_scenario(SCENARIO_PRIORITY_IMBALANCED);
    const ScenarioStats stats = collect_stats(&workload);
    const double high_priority_share =
        (double) stats.high_priority_count / (double) workload.process_count;

    /* O viés de 80% é somado aos acertos 0/1 da distribuição restante. */
    assert(high_priority_share >= 0.80);
    assert(high_priority_share <= 0.90);

    workload_free(&workload);
}

int main(void) {
    test_scenario_names();
    test_balanced_profile();
    test_io_bound_profile();
    test_cpu_bound_profile();
    test_priority_imbalanced_profile();

    puts("OK: quatro cenarios experimentais validados.");
    return 0;
}

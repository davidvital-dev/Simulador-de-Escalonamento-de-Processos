#include "metrics.h"

#include <stdio.h>

/*
 * Acumulador do indice de Jain: mantem apenas soma e soma dos quadrados,
 * sem alocar um vetor, para poder ser alimentado uma amostra por vez.
 * Usado por metrics_compute_run.
 */
typedef struct {
    double sum;
    double sum_squares;
    size_t count;
} JainAccumulator;

static void jain_accumulator_add(JainAccumulator *accumulator, double value) {
    accumulator->sum += value;
    accumulator->sum_squares += value * value;
    ++accumulator->count;
}

/* jain = soma(x)^2 / (n * soma(x^2)). Retorna -1.0 sem amostras validas. */
static double jain_accumulator_index(const JainAccumulator *accumulator) {
    if (accumulator->count == 0 || accumulator->sum_squares <= 0.0) {
        return -1.0;
    }

    return (accumulator->sum * accumulator->sum) /
           ((double) accumulator->count * accumulator->sum_squares);
}

int metrics_turnaround(const Process *process) {
    return process->finish_time - process->arrival_time;
}

int metrics_ideal_time(const Process *process) {
    int total = 0;
    size_t index;

    for (index = 0; index < process->burst_count; ++index) {
        total += process->bursts[index].duration;
    }

    return total;
}

double metrics_slowdown(const Process *process) {
    const int ideal_time = metrics_ideal_time(process);

    if (ideal_time <= 0) {
        return -1.0;
    }

    return metrics_turnaround(process) / (double) ideal_time;
}

bool metrics_compute_run(const Process *processes, size_t process_count,
                         const SimulationResult *result, unsigned int seed,
                         const char *scenario, const char *algorithm,
                         RunMetrics *out) {
    JainAccumulator jain = {0};
    double turnaround_sum = 0.0;
    size_t valid_count = 0;
    size_t index;

    if (processes == NULL || result == NULL || scenario == NULL ||
        algorithm == NULL || out == NULL) {
        return false;
    }

    for (index = 0; index < process_count; ++index) {
        const Process *process = &processes[index];
        double slowdown;

        if (process->state != PROCESS_FINISHED) {
            continue;
        }

        slowdown = metrics_slowdown(process);
        if (slowdown < 0.0) {
            continue;
        }

        turnaround_sum += metrics_turnaround(process);
        jain_accumulator_add(&jain, slowdown);
        ++valid_count;
    }

    if (valid_count == 0) {
        return false;
    }

    out->seed = seed;
    snprintf(out->scenario, sizeof(out->scenario), "%s", scenario);
    snprintf(out->algorithm, sizeof(out->algorithm), "%s", algorithm);
    out->process_count = process_count;
    out->avg_turnaround = turnaround_sum / (double) valid_count;
    out->context_switches = result->context_switches;
    out->jain_slowdown = jain_accumulator_index(&jain);

    return true;
}

void metrics_placeholder(void) {
    /* TODO(Henrique): exportação CSV. */
}

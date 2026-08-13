#include "metrics.h"

/*
 * Acumulador do indice de Jain: mantem apenas soma e soma dos quadrados,
 * sem alocar um vetor, para poder ser alimentado uma amostra por vez
 * quando RunMetrics for adicionado.
 */
typedef struct {
    double sum;
    double sum_squares;
    size_t count;
} JainAccumulator;

/*
 * Ainda sem chamador em producao: sera consumida por RunMetrics no proximo
 * commit. O atributo evita -Wunused-function ate la; os testes ja exercitam
 * as duas funcoes incluindo este arquivo diretamente.
 */
static void __attribute__((unused))
jain_accumulator_add(JainAccumulator *accumulator, double value) {
    accumulator->sum += value;
    accumulator->sum_squares += value * value;
    ++accumulator->count;
}

/* jain = soma(x)^2 / (n * soma(x^2)). Retorna -1.0 sem amostras validas. */
static double __attribute__((unused))
jain_accumulator_index(const JainAccumulator *accumulator) {
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

void metrics_placeholder(void) {
    /* TODO(Henrique): turnaround, slowdown, Jain e exportação CSV. */
}

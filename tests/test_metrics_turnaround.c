#include <assert.h>
#include <math.h>
#include <stdio.h>

/*
 * Inclui a implementacao diretamente (em vez de linkar src/metrics.c e
 * incluir metrics.h) para poder exercitar JainAccumulator/jain_accumulator_*,
 * que sao internas de proposito: so serao expostas no header quando
 * RunMetrics for adicionado em um commit futuro.
 */
#include "../src/metrics.c"

/*
 * Cargas construidas a mao: arrival_time e finish_time sao atribuidos
 * diretamente, sem executar o motor, porque este commit cobre apenas o
 * calculo das metricas, nao a integracao com o simulador.
 */

static void test_turnaround_single_process(void) {
    Process process = {
        .pid = 1,
        .arrival_time = 0,
        .finish_time = 10
    };

    /* turnaround calculado a mao: 10 - 0 = 10. */
    assert(metrics_turnaround(&process) == 10);
}

static void test_turnaround_with_nonzero_arrival(void) {
    Process process = {
        .pid = 2,
        .arrival_time = 3,
        .finish_time = 15
    };

    /* turnaround calculado a mao: 15 - 3 = 12. */
    assert(metrics_turnaround(&process) == 12);
}

static void test_ideal_time_uses_original_duration_not_remaining(void) {
    /*
     * remaining_time propositalmente diferente de duration em cada rajada,
     * para garantir que metrics_ideal_time nunca leia remaining_time.
     */
    Burst bursts[] = {
        {BURST_CPU, 4, 0},
        {BURST_IO, 3, 1},
        {BURST_CPU, 3, 0}
    };
    Process process = {
        .pid = 3,
        .bursts = bursts,
        .burst_count = 3
    };

    /* tempo minimo ideal calculado a mao: 4 + 3 + 3 = 10. */
    assert(metrics_ideal_time(&process) == 10);
}

static void test_ideal_time_multiple_io_bursts(void) {
    Burst bursts[] = {
        {BURST_CPU, 2, 0},
        {BURST_IO, 6, 6},
        {BURST_CPU, 2, 1},
        {BURST_IO, 4, 4},
        {BURST_CPU, 1, 1}
    };
    Process process = {
        .pid = 4,
        .bursts = bursts,
        .burst_count = 5
    };

    /* tempo minimo ideal calculado a mao: 2 + 6 + 2 + 4 + 1 = 15. */
    assert(metrics_ideal_time(&process) == 15);
}

static void test_turnaround_and_ideal_time_together(void) {
    Burst bursts[] = {
        {BURST_CPU, 5, 2}
    };
    Process process = {
        .pid = 5,
        .arrival_time = 2,
        .finish_time = 9,
        .bursts = bursts,
        .burst_count = 1
    };

    /* turnaround a mao: 9 - 2 = 7. tempo minimo ideal a mao: 5. */
    assert(metrics_turnaround(&process) == 7);
    assert(metrics_ideal_time(&process) == 5);
}

static void test_slowdown_matches_manual_calculation(void) {
    Burst bursts[] = {
        {BURST_CPU, 5, 2}
    };
    Process process = {
        .pid = 5,
        .arrival_time = 2,
        .finish_time = 9,
        .bursts = bursts,
        .burst_count = 1
    };

    /* slowdown a mao: turnaround 7 / tempo minimo ideal 5 = 1.4. */
    assert(fabs(metrics_slowdown(&process) - 1.4) < 1e-9);
}

static void test_slowdown_invalid_when_ideal_time_not_positive(void) {
    Process process = {
        .pid = 6,
        .arrival_time = 0,
        .finish_time = 5,
        .bursts = NULL,
        .burst_count = 0
    };

    /* sem rajadas, tempo minimo ideal = 0: divisao por zero e evitada. */
    assert(metrics_slowdown(&process) == -1.0);
}

static void test_jain_index_is_one_when_slowdowns_are_equal(void) {
    /*
     * Tres processos com turnaround e tempo minimo ideal diferentes, mas
     * com a mesma razao (slowdown a mao: 4/2 = 6/3 = 10/5 = 2.0 para todos).
     */
    Burst bursts_a[] = {{BURST_CPU, 2, 0}};
    Burst bursts_b[] = {{BURST_CPU, 3, 0}};
    Burst bursts_c[] = {{BURST_CPU, 5, 0}};
    Process process_a = {
        .pid = 10, .arrival_time = 0, .finish_time = 4,
        .bursts = bursts_a, .burst_count = 1
    };
    Process process_b = {
        .pid = 11, .arrival_time = 0, .finish_time = 6,
        .bursts = bursts_b, .burst_count = 1
    };
    Process process_c = {
        .pid = 12, .arrival_time = 0, .finish_time = 10,
        .bursts = bursts_c, .burst_count = 1
    };
    JainAccumulator accumulator = {0};

    jain_accumulator_add(&accumulator, metrics_slowdown(&process_a));
    jain_accumulator_add(&accumulator, metrics_slowdown(&process_b));
    jain_accumulator_add(&accumulator, metrics_slowdown(&process_c));

    /* Jain calculado a mao com slowdowns iguais: (3*2)^2/(3*3*2^2) = 1.0. */
    assert(fabs(jain_accumulator_index(&accumulator) - 1.0) < 1e-9);
}

int main(void) {
    test_turnaround_single_process();
    test_turnaround_with_nonzero_arrival();
    test_ideal_time_uses_original_duration_not_remaining();
    test_ideal_time_multiple_io_bursts();
    test_turnaround_and_ideal_time_together();
    test_slowdown_matches_manual_calculation();
    test_slowdown_invalid_when_ideal_time_not_positive();
    test_jain_index_is_one_when_slowdowns_are_equal();

    puts("OK: turnaround, tempo minimo ideal, slowdown e indice de Jain validados.");
    return 0;
}

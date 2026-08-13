#include <assert.h>
#include <stdio.h>

#include "metrics.h"
#include "process.h"

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

int main(void) {
    test_turnaround_single_process();
    test_turnaround_with_nonzero_arrival();
    test_ideal_time_uses_original_duration_not_remaining();
    test_ideal_time_multiple_io_bursts();
    test_turnaround_and_ideal_time_together();

    puts("OK: turnaround e tempo minimo ideal validados.");
    return 0;
}

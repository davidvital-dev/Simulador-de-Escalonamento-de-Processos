#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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

static void test_compute_run_matches_manual_calculation(void) {
    Burst bursts_a[] = {{BURST_CPU, 2, 0}};
    Burst bursts_b[] = {{BURST_CPU, 3, 0}};
    Burst bursts_c[] = {{BURST_CPU, 5, 0}};
    Process processes[] = {
        {
            .pid = 20, .arrival_time = 0, .finish_time = 4,
            .state = PROCESS_FINISHED, .bursts = bursts_a, .burst_count = 1
        },
        {
            .pid = 21, .arrival_time = 0, .finish_time = 6,
            .state = PROCESS_FINISHED, .bursts = bursts_b, .burst_count = 1
        },
        {
            .pid = 22, .arrival_time = 0, .finish_time = 10,
            .state = PROCESS_FINISHED, .bursts = bursts_c, .burst_count = 1
        }
    };
    SimulationResult result = {
        .elapsed_time = 10,
        .completed_processes = 3,
        .context_switches = 7,
        .context_switch_ticks = 14,
        .idle_ticks = 0
    };
    RunMetrics metrics;

    /*
     * turnaround a mao: 4, 6, 10 -> media = 20/3.
     * slowdown a mao: 4/2=2.0, 6/3=2.0, 10/5=2.0 -> Jain = 1.0.
     */
    assert(metrics_compute_run(processes, 3, &result, 42, "balanced", "fcfs",
                               &metrics));

    assert(metrics.seed == 42);
    assert(strcmp(metrics.scenario, "balanced") == 0);
    assert(strcmp(metrics.algorithm, "fcfs") == 0);
    assert(metrics.process_count == 3);
    assert(fabs(metrics.avg_turnaround - (20.0 / 3.0)) < 1e-9);
    assert(metrics.context_switches == 7);
    assert(fabs(metrics.jain_slowdown - 1.0) < 1e-9);
}

static void test_compute_run_ignores_unfinished_and_invalid_processes(void) {
    Burst bursts_a[] = {{BURST_CPU, 2, 0}};
    Burst bursts_b[] = {{BURST_CPU, 4, 0}};
    Process processes[] = {
        {
            .pid = 30, .arrival_time = 0, .finish_time = 4,
            .state = PROCESS_FINISHED, .bursts = bursts_a, .burst_count = 1
        },
        /*
         * ainda em execucao: nao deve entrar no calculo. finish_time
         * propositalmente valido (8) para isolar o filtro de estado: se o
         * filtro falhar, este processo teria slowdown positivo (2.0) e
         * mudaria avg_turnaround de 4.0 para 6.0, denunciando o bug mesmo
         * que o Jain continue dando 1.0 por coincidencia de valores iguais.
         */
        {
            .pid = 31, .arrival_time = 0, .finish_time = 8,
            .state = PROCESS_RUNNING, .bursts = bursts_b, .burst_count = 1
        },
        /* finalizado mas sem rajadas: slowdown invalido, deve ser ignorado. */
        {
            .pid = 32, .arrival_time = 0, .finish_time = 3,
            .state = PROCESS_FINISHED, .bursts = NULL, .burst_count = 0
        }
    };
    SimulationResult result = {.context_switches = 5};
    RunMetrics metrics;

    assert(metrics_compute_run(processes, 3, &result, 7, "io", "rr",
                               &metrics));

    /* somente o processo 30 entra no calculo: turnaround 4, slowdown 2.0. */
    assert(fabs(metrics.avg_turnaround - 4.0) < 1e-9);
    assert(fabs(metrics.jain_slowdown - 1.0) < 1e-9);
    assert(metrics.context_switches == 5);
    /* process_count reflete o tamanho total da carga, nao so os validos. */
    assert(metrics.process_count == 3);
}

static void test_compute_run_fails_without_valid_finished_processes(void) {
    Process processes[] = {
        {.pid = 40, .arrival_time = 0, .finish_time = -1, .state = PROCESS_READY}
    };
    SimulationResult result = {.context_switches = 0};
    RunMetrics metrics = {.seed = 999};

    assert(!metrics_compute_run(processes, 1, &result, 1, "cpu", "priority",
                                &metrics));
    /* out nao deve ser alterado quando a funcao falha. */
    assert(metrics.seed == 999);
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
    test_compute_run_matches_manual_calculation();
    test_compute_run_ignores_unfinished_and_invalid_processes();
    test_compute_run_fails_without_valid_finished_processes();

    puts("OK: turnaround, ideal, slowdown, Jain e RunMetrics validados.");
    return 0;
}

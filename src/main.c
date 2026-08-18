#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fcfs.h"
#include "metrics.h"
#include "priority.h"
#include "proposed.h"
#include "round_robin.h"
#include "simulator.h"
#include "workload.h"

#define DEFAULT_QUANTUM 4

typedef struct {
    ScenarioType scenario;
    const char *scenario_name;
    const char *algorithm_name;
    uint64_t seed;
    size_t process_count;
    int quantum;
    int context_switch_cost;
    bool debug;
} CliOptions;

typedef enum {
    PARSE_OK,
    PARSE_HELP,
    PARSE_ERROR
} ParseResult;

static void print_usage(FILE *stream, const char *program_name) {
    fprintf(stream,
            "Uso: %s --scenario CENARIO --algorithm ALGORITMO "
            "--seed N --processes N [opcoes]\n\n"
            "Cenarios:\n"
            "  balanced | io-bound | cpu-bound | priority-imbalanced\n\n"
            "Algoritmos:\n"
            "  fcfs | rr | priority | proposed\n\n"
            "Opcoes:\n"
            "  --quantum N                Quantum do RR (padrao: %d)\n"
            "  --context-switch-cost N    Custo da troca (padrao: 2)\n"
            "  --debug                    Logs da simulacao em stderr\n"
            "  --help                     Exibe esta ajuda\n",
            program_name, DEFAULT_QUANTUM);
}

static bool parse_uint64(const char *text, uint64_t maximum, uint64_t *value) {
    char *end = NULL;
    unsigned long long parsed;

    if (text == NULL || value == NULL || text[0] == '\0' || text[0] == '-') {
        return false;
    }

    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed > (unsigned long long) maximum) {
        return false;
    }

    *value = (uint64_t) parsed;
    return true;
}

static bool parse_scenario(const char *name, ScenarioType *scenario) {
    if (strcmp(name, "balanced") == 0) {
        *scenario = SCENARIO_BALANCED;
    } else if (strcmp(name, "io-bound") == 0) {
        *scenario = SCENARIO_IO_BOUND;
    } else if (strcmp(name, "cpu-bound") == 0) {
        *scenario = SCENARIO_CPU_BOUND;
    } else if (strcmp(name, "priority-imbalanced") == 0) {
        *scenario = SCENARIO_PRIORITY_IMBALANCED;
    } else {
        return false;
    }

    return true;
}

static bool algorithm_is_valid(const char *name) {
    return strcmp(name, "fcfs") == 0 || strcmp(name, "rr") == 0 ||
           strcmp(name, "priority") == 0 || strcmp(name, "proposed") == 0;
}

static const char *option_value(int argc, char **argv, int *index) {
    if (*index + 1 >= argc) {
        fprintf(stderr, "Erro: a opcao %s exige um valor.\n", argv[*index]);
        return NULL;
    }

    ++*index;
    return argv[*index];
}

static ParseResult parse_arguments(int argc, char **argv, CliOptions *options) {
    bool has_scenario = false;
    bool has_algorithm = false;
    bool has_seed = false;
    bool has_processes = false;
    bool has_quantum = false;
    bool has_context_switch_cost = false;
    int index;

    *options = (CliOptions) {
        .quantum = DEFAULT_QUANTUM,
        .context_switch_cost = simulator_default_config().context_switch_cost,
        .debug = false
    };

    for (index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        const char *value;
        uint64_t parsed;

        if (strcmp(argument, "--help") == 0) {
            print_usage(stdout, argv[0]);
            return PARSE_HELP;
        }

        if (strcmp(argument, "--debug") == 0) {
            options->debug = true;
            continue;
        }

        if (strcmp(argument, "--scenario") == 0) {
            if (has_scenario || (value = option_value(argc, argv, &index)) == NULL) {
                if (has_scenario) {
                    fputs("Erro: --scenario foi informado mais de uma vez.\n", stderr);
                }
                return PARSE_ERROR;
            }
            if (!parse_scenario(value, &options->scenario)) {
                fprintf(stderr, "Erro: cenario desconhecido: %s.\n", value);
                return PARSE_ERROR;
            }
            options->scenario_name = value;
            has_scenario = true;
            continue;
        }

        if (strcmp(argument, "--algorithm") == 0) {
            if (has_algorithm || (value = option_value(argc, argv, &index)) == NULL) {
                if (has_algorithm) {
                    fputs("Erro: --algorithm foi informado mais de uma vez.\n", stderr);
                }
                return PARSE_ERROR;
            }
            if (!algorithm_is_valid(value)) {
                fprintf(stderr, "Erro: algoritmo desconhecido: %s.\n", value);
                return PARSE_ERROR;
            }
            options->algorithm_name = value;
            has_algorithm = true;
            continue;
        }

        if (strcmp(argument, "--seed") == 0) {
            if (has_seed || (value = option_value(argc, argv, &index)) == NULL) {
                if (has_seed) {
                    fputs("Erro: --seed foi informado mais de uma vez.\n", stderr);
                }
                return PARSE_ERROR;
            }
            if (!parse_uint64(value, UINT_MAX, &options->seed)) {
                fprintf(stderr, "Erro: seed invalida: %s.\n", value);
                return PARSE_ERROR;
            }
            has_seed = true;
            continue;
        }

        if (strcmp(argument, "--processes") == 0) {
            if (has_processes || (value = option_value(argc, argv, &index)) == NULL) {
                if (has_processes) {
                    fputs("Erro: --processes foi informado mais de uma vez.\n", stderr);
                }
                return PARSE_ERROR;
            }
            if (!parse_uint64(value, INT_MAX, &parsed) || parsed == 0) {
                fprintf(stderr, "Erro: quantidade de processos invalida: %s.\n",
                        value);
                return PARSE_ERROR;
            }
            options->process_count = (size_t) parsed;
            has_processes = true;
            continue;
        }

        if (strcmp(argument, "--quantum") == 0) {
            if (has_quantum || (value = option_value(argc, argv, &index)) == NULL) {
                if (has_quantum) {
                    fputs("Erro: --quantum foi informado mais de uma vez.\n", stderr);
                }
                return PARSE_ERROR;
            }
            if (!parse_uint64(value, INT_MAX, &parsed) || parsed == 0) {
                fprintf(stderr, "Erro: quantum invalido: %s.\n", value);
                return PARSE_ERROR;
            }
            options->quantum = (int) parsed;
            has_quantum = true;
            continue;
        }

        if (strcmp(argument, "--context-switch-cost") == 0) {
            if (has_context_switch_cost ||
                (value = option_value(argc, argv, &index)) == NULL) {
                if (has_context_switch_cost) {
                    fputs("Erro: --context-switch-cost foi informado mais de uma vez.\n",
                          stderr);
                }
                return PARSE_ERROR;
            }
            if (!parse_uint64(value, INT_MAX, &parsed)) {
                fprintf(stderr, "Erro: custo de troca invalido: %s.\n", value);
                return PARSE_ERROR;
            }
            options->context_switch_cost = (int) parsed;
            has_context_switch_cost = true;
            continue;
        }

        fprintf(stderr, "Erro: opcao desconhecida: %s.\n", argument);
        return PARSE_ERROR;
    }

    if (!has_scenario || !has_algorithm || !has_seed || !has_processes) {
        fputs("Erro: --scenario, --algorithm, --seed e --processes sao obrigatorios.\n",
              stderr);
        return PARSE_ERROR;
    }

    return PARSE_OK;
}

static bool build_scheduler(const CliOptions *options,
                            RoundRobinContext *round_robin_context,
                            ProposedContext *proposed_context,
                            Scheduler *scheduler) {
    if (strcmp(options->algorithm_name, "fcfs") == 0) {
        *scheduler = fcfs_scheduler();
    } else if (strcmp(options->algorithm_name, "rr") == 0) {
        *round_robin_context = (RoundRobinContext) {.quantum = options->quantum};
        *scheduler = round_robin_scheduler(round_robin_context);
    } else if (strcmp(options->algorithm_name, "priority") == 0) {
        *scheduler = priority_scheduler();
    } else if (strcmp(options->algorithm_name, "proposed") == 0) {
        *proposed_context = proposed_default_context();
        *scheduler = proposed_scheduler(proposed_context);
    } else {
        return false;
    }

    return scheduler->ops != NULL;
}

int main(int argc, char **argv) {
    CliOptions options;
    const ParseResult parse_result = parse_arguments(argc, argv, &options);
    ScenarioConfig scenario_config;
    SimulationConfig simulation_config;
    SimulationResult simulation_result;
    RoundRobinContext round_robin_context = {0};
    ProposedContext proposed_context = {0};
    Scheduler scheduler = {0};
    Workload workload = {0};
    RunMetrics metrics;
    int exit_code = EXIT_FAILURE;

    if (parse_result == PARSE_HELP) {
        return EXIT_SUCCESS;
    }
    if (parse_result == PARSE_ERROR) {
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    if (!workload_default_config(options.scenario, &scenario_config)) {
        fputs("Erro: nao foi possivel configurar o cenario.\n", stderr);
        return EXIT_FAILURE;
    }

    if (!workload_generate(options.seed, &scenario_config,
                           options.process_count, &workload)) {
        fputs("Erro: nao foi possivel gerar a carga de trabalho.\n", stderr);
        return EXIT_FAILURE;
    }

    if (!build_scheduler(&options, &round_robin_context,
                         &proposed_context, &scheduler)) {
        fputs("Erro: nao foi possivel configurar o escalonador.\n", stderr);
        goto cleanup;
    }

    simulation_config = simulator_default_config();
    simulation_config.context_switch_cost = options.context_switch_cost;
    simulation_config.debug = options.debug;
    simulation_config.debug_stream = stderr;

    if (!simulator_run(workload.processes, workload.process_count,
                       &simulation_config, &scheduler, &simulation_result)) {
        fputs("Erro: a simulacao falhou.\n", stderr);
        goto cleanup;
    }

    if (!metrics_compute_run(workload.processes, workload.process_count,
                             &simulation_result, (unsigned int) options.seed,
                             options.scenario_name, options.algorithm_name,
                             &metrics)) {
        fputs("Erro: nao foi possivel calcular as metricas.\n", stderr);
        goto cleanup;
    }

    metrics_csv_write_row(stdout, &metrics);
    if (ferror(stdout)) {
        fputs("Erro: nao foi possivel escrever o resultado.\n", stderr);
        goto cleanup;
    }

    exit_code = EXIT_SUCCESS;

cleanup:
    workload_free(&workload);
    return exit_code;
}

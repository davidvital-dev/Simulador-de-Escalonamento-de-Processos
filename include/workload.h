#ifndef WORKLOAD_H
#define WORKLOAD_H

#include <stdbool.h>

typedef enum {
    SCENARIO_BALANCED,
    SCENARIO_IO_BOUND,
    SCENARIO_CPU_BOUND,
    SCENARIO_PRIORITY_IMBALANCED,
    SCENARIO_COUNT
} ScenarioType;

typedef struct {
    int min;
    int max;
} IntRange;

/*
 * Parâmetros usados na geração de uma carga. Todos os limites são inclusivos.
 * Os valores-padrão são decisões experimentais da equipe e podem ser alterados
 * antes da geração sem modificar o gerador.
 *
 * cpu_burst_count determina quantas rajadas de CPU um processo possui. Quando
 * houver N rajadas de CPU, o gerador deverá criar N - 1 rajadas de E/S.
 *
 * priority_bias_percent vale zero para uma distribuição uniforme. Quando for
 * maior que zero, representa a porcentagem de processos cuja prioridade deverá
 * pertencer a biased_priority.
 */
typedef struct {
    ScenarioType type;
    IntRange interarrival_time;
    IntRange priority;
    IntRange cpu_burst_count;
    IntRange cpu_burst_duration;
    IntRange io_burst_duration;
    int priority_bias_percent;
    IntRange biased_priority;
} ScenarioConfig;

/* Preenche config com os parâmetros-padrão do cenário informado. */
bool workload_default_config(ScenarioType type, ScenarioConfig *config);

/* Verifica intervalos, percentuais e consistência geral da configuração. */
bool workload_config_is_valid(const ScenarioConfig *config);

/* Retorna um identificador estável, apropriado para logs e arquivos CSV. */
const char *workload_scenario_name(ScenarioType type);

#endif

#ifndef PROPOSED_H
#define PROPOSED_H

#include <stdbool.h>

#include "scheduler.h"

typedef struct {
    int priority_weight;
    int burst_weight;
    int aging_interval;
    int initial_burst_estimate;
} ProposedContext;

/* Parâmetros principais definidos em docs/decisoes-tecnicas.md. */
ProposedContext proposed_default_context(void);

/* Pesos não podem ser negativos; aging_interval deve ser positivo. */
bool proposed_context_is_valid(const ProposedContext *context);

/*
 * AHR - Aging com Histórico de Rajadas.
 * Política não preemptiva identificada como "proposed" no pipeline.
 * O chamador deve manter o contexto vivo durante toda a simulação.
 */
Scheduler proposed_scheduler(ProposedContext *context);

#endif

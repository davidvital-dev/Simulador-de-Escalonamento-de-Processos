#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H

#include "scheduler.h"

typedef struct {
    int quantum;
} RoundRobinContext;

/*
 * Quantum principal dos experimentos: 4 ticks (docs/decisoes-tecnicas.md).
 * O chamador possui o contexto e deve mante-lo vivo durante toda a execucao
 * do simulador.
 */
Scheduler round_robin_scheduler(RoundRobinContext *context);

#endif

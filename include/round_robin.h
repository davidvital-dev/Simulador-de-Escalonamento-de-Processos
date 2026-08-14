#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H

#include "scheduler.h"

typedef struct {
    int quantum;
} RoundRobinContext;

/* Quem chama precisa manter o context vivo durante toda a simulacao. */
Scheduler round_robin_scheduler(RoundRobinContext *context);

#endif

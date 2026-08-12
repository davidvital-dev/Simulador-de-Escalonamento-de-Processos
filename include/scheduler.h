#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "process.h"

#define SCHEDULER_NO_SELECTION SIZE_MAX

typedef struct {
    size_t process_index;
    int pid;
    int priority;
    int arrival_time;
    int ready_since;
    int total_cpu_executed;
    int completed_cpu_bursts;
    int last_cpu_burst_duration;
} SchedulerProcessView;

typedef struct {
    const char *name;

    /*
     * Retorna a POSICAO dentro do vetor ready, e nao o indice global do
     * processo. O vetor preserva a ordem de entrada na fila de prontos.
     */
    size_t (*pick_next)(const SchedulerProcessView *ready,
                        size_t ready_count,
                        int current_time,
                        void *context);

    /*
     * Chamado apenas depois de um tick de CPU que nao terminou a rajada.
     * A fila pronta tambem e fornecida para permitir politicas preemptivas
     * que dependam de concorrentes atuais. Politicas nao preemptivas podem
     * deixar este callback como NULL.
     */
    bool (*should_preempt)(const SchedulerProcessView *running,
                           const SchedulerProcessView *ready,
                           size_t ready_count,
                           int slice_ticks,
                           int current_time,
                           void *context);
} SchedulerOps;

typedef struct {
    const SchedulerOps *ops;
    void *context;
} Scheduler;

#endif

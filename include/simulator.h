#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "process.h"
#include "scheduler.h"

typedef struct {
    int context_switch_cost;
    bool debug;
    FILE *debug_stream;
} SimulationConfig;

typedef struct {
    int elapsed_time;
    size_t completed_processes;
    size_t context_switches;
    size_t context_switch_ticks;
    size_t idle_ticks;
} SimulationResult;

/* Configuracao dos experimentos principais: custo de troca = 2 ticks. */
SimulationConfig simulator_default_config(void);

/*
 * Executa um vetor mutavel de processos usando uma politica externa.
 * Para comparar algoritmos, o chamador deve fornecer uma copia profunda da
 * mesma carga a cada execucao. O motor reseta os campos de runtime ao iniciar.
 */
bool simulator_run(Process *processes,
                   size_t process_count,
                   const SimulationConfig *config,
                   const Scheduler *scheduler,
                   SimulationResult *result);

#endif

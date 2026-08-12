#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_FINISHED
} ProcessState;

typedef enum {
    BURST_CPU,
    BURST_IO
} BurstType;

typedef struct {
    BurstType type;
    int duration;
    int remaining_time;
} Burst;

typedef struct {
    int pid;
    int arrival_time;
    int priority;
    ProcessState state;
    Burst *bursts;
    size_t burst_count;
    size_t current_burst_index;

    /* Campos atualizados exclusivamente durante a simulacao. */
    int finish_time;
    int ready_since;
    int total_cpu_executed;
    int completed_cpu_bursts;
    int last_cpu_burst_duration;
} Process;

/* Restaura os campos mutaveis para uma nova execucao da mesma carga. */
void process_reset_runtime(Process *process);
void process_reset_array(Process *processes, size_t process_count);

/* Copias profundas: o vetor de rajadas da copia nunca compartilha memoria. */
bool process_clone(const Process *source, Process *destination);
bool process_clone_array(const Process *source, size_t process_count,
                         Process **destination);

/* Liberacao dos recursos pertencentes ao processo/copia. */
void process_destroy(Process *process);
void process_array_destroy(Process *processes, size_t process_count);

#endif

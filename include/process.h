#ifndef PROCESS_H
#define PROCESS_H

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
} Process;

#endif

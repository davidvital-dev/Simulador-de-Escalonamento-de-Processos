#include "process.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void process_reset_runtime(Process *process) {
    size_t index;

    if (process == NULL) {
        return;
    }

    process->state = PROCESS_NEW;
    process->current_burst_index = 0;
    process->finish_time = -1;
    process->ready_since = -1;
    process->total_cpu_executed = 0;
    process->completed_cpu_bursts = 0;
    process->last_cpu_burst_duration = -1;

    for (index = 0; index < process->burst_count; ++index) {
        process->bursts[index].remaining_time = process->bursts[index].duration;
    }
}

void process_reset_array(Process *processes, size_t process_count) {
    size_t index;

    if (processes == NULL) {
        return;
    }

    for (index = 0; index < process_count; ++index) {
        process_reset_runtime(&processes[index]);
    }
}

bool process_clone(const Process *source, Process *destination) {
    Burst *bursts = NULL;

    if (source == NULL || destination == NULL ||
        (source->burst_count > 0 && source->bursts == NULL)) {
        return false;
    }

    if (source->burst_count > SIZE_MAX / sizeof(*bursts)) {
        return false;
    }

    if (source->burst_count > 0) {
        bursts = malloc(source->burst_count * sizeof(*bursts));
        if (bursts == NULL) {
            return false;
        }
        memcpy(bursts, source->bursts,
               source->burst_count * sizeof(*bursts));
    }

    *destination = *source;
    destination->bursts = bursts;
    return true;
}

bool process_clone_array(const Process *source, size_t process_count,
                         Process **destination) {
    Process *copy;
    size_t index;

    if (destination == NULL ||
        (process_count > 0 && source == NULL) ||
        process_count > SIZE_MAX / sizeof(*copy)) {
        return false;
    }

    *destination = NULL;
    if (process_count == 0) {
        return true;
    }

    copy = calloc(process_count, sizeof(*copy));
    if (copy == NULL) {
        return false;
    }

    for (index = 0; index < process_count; ++index) {
        if (!process_clone(&source[index], &copy[index])) {
            process_array_destroy(copy, process_count);
            return false;
        }
    }

    *destination = copy;
    return true;
}

void process_destroy(Process *process) {
    if (process == NULL) {
        return;
    }

    free(process->bursts);
    *process = (Process) {0};
}

void process_array_destroy(Process *processes, size_t process_count) {
    size_t index;

    if (processes == NULL) {
        return;
    }

    for (index = 0; index < process_count; ++index) {
        free(processes[index].bursts);
    }
    free(processes);
}

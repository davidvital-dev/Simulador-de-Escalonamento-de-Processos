#include "metrics.h"

int metrics_turnaround(const Process *process) {
    return process->finish_time - process->arrival_time;
}

int metrics_ideal_time(const Process *process) {
    int total = 0;
    size_t index;

    for (index = 0; index < process->burst_count; ++index) {
        total += process->bursts[index].duration;
    }

    return total;
}

void metrics_placeholder(void) {
    /* TODO(Henrique): turnaround, slowdown, Jain e exportação CSV. */
}

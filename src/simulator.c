#include "simulator.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#define NO_PROCESS SIZE_MAX

typedef struct {
    size_t *items;
    size_t count;
    size_t capacity;
} ReadyQueue;

static bool process_definition_is_valid(const Process *process) {
    size_t index;

    if (process == NULL || process->arrival_time < 0 ||
        process->burst_count == 0 || process->bursts == NULL ||
        process->burst_count % 2 == 0) {
        return false;
    }

    for (index = 0; index < process->burst_count; ++index) {
        const BurstType expected_type = index % 2 == 0 ? BURST_CPU : BURST_IO;
        if (process->bursts[index].type != expected_type ||
            process->bursts[index].duration <= 0) {
            return false;
        }
    }

    return true;
}

static bool workload_is_valid(const Process *processes, size_t process_count) {
    size_t index;

    if (processes == NULL || process_count == 0) {
        return false;
    }

    for (index = 0; index < process_count; ++index) {
        if (!process_definition_is_valid(&processes[index])) {
            return false;
        }
    }

    return true;
}

static bool ready_queue_init(ReadyQueue *queue, size_t capacity) {
    if (queue == NULL || capacity == 0 ||
        capacity > SIZE_MAX / sizeof(*queue->items)) {
        return false;
    }

    queue->items = malloc(capacity * sizeof(*queue->items));
    if (queue->items == NULL) {
        return false;
    }

    queue->count = 0;
    queue->capacity = capacity;
    return true;
}

static void ready_queue_destroy(ReadyQueue *queue) {
    if (queue == NULL) {
        return;
    }

    free(queue->items);
    *queue = (ReadyQueue) {0};
}

static bool ready_queue_push(ReadyQueue *queue, size_t process_index) {
    if (queue == NULL || queue->items == NULL || queue->count >= queue->capacity) {
        return false;
    }

    queue->items[queue->count++] = process_index;
    return true;
}

static size_t ready_queue_remove_at(ReadyQueue *queue, size_t position) {
    size_t selected;
    size_t index;

    if (queue == NULL || position >= queue->count) {
        return NO_PROCESS;
    }

    selected = queue->items[position];
    for (index = position + 1; index < queue->count; ++index) {
        queue->items[index - 1] = queue->items[index];
    }
    --queue->count;
    return selected;
}

static SchedulerProcessView process_view(const Process *process,
                                         size_t process_index) {
    return (SchedulerProcessView) {
        .process_index = process_index,
        .pid = process->pid,
        .priority = process->priority,
        .arrival_time = process->arrival_time,
        .ready_since = process->ready_since,
        .total_cpu_executed = process->total_cpu_executed,
        .completed_cpu_bursts = process->completed_cpu_bursts,
        .last_cpu_burst_duration = process->last_cpu_burst_duration
    };
}

static bool build_ready_view(const Process *processes,
                             const ReadyQueue *queue,
                             SchedulerProcessView *view) {
    size_t position;

    if (processes == NULL || queue == NULL || view == NULL) {
        return false;
    }

    for (position = 0; position < queue->count; ++position) {
        const size_t process_index = queue->items[position];
        if (processes[process_index].state != PROCESS_READY) {
            return false;
        }
        view[position] = process_view(&processes[process_index], process_index);
    }

    return true;
}

static FILE *debug_stream(const SimulationConfig *config) {
    return config->debug_stream != NULL ? config->debug_stream : stderr;
}

static void debug_log(const SimulationConfig *config, const char *format, ...) {
    va_list arguments;

    if (!config->debug) {
        return;
    }

    va_start(arguments, format);
    vfprintf(debug_stream(config), format, arguments);
    va_end(arguments);
}

static bool admit_arrivals(Process *processes,
                           size_t process_count,
                           ReadyQueue *ready,
                           int current_time,
                           const SimulationConfig *config) {
    size_t index;

    for (index = 0; index < process_count; ++index) {
        Process *process = &processes[index];
        if (process->state == PROCESS_NEW &&
            process->arrival_time <= current_time) {
            process->state = PROCESS_READY;
            process->ready_since = current_time;
            if (!ready_queue_push(ready, index)) {
                return false;
            }
            debug_log(config, "[t=%d] PID %d: NEW -> READY\n",
                      current_time, process->pid);
        }
    }

    return true;
}

static bool complete_io(Process *processes,
                        size_t process_count,
                        ReadyQueue *ready,
                        int current_time,
                        const SimulationConfig *config) {
    size_t index;

    for (index = 0; index < process_count; ++index) {
        Process *process = &processes[index];
        Burst *io_burst;

        if (process->state != PROCESS_BLOCKED) {
            continue;
        }
        if (process->current_burst_index >= process->burst_count) {
            return false;
        }

        io_burst = &process->bursts[process->current_burst_index];
        if (io_burst->type != BURST_IO || io_burst->remaining_time < 0) {
            return false;
        }
        if (io_burst->remaining_time > 0) {
            continue;
        }
        if (process->current_burst_index + 1 >= process->burst_count ||
            process->bursts[process->current_burst_index + 1].type != BURST_CPU) {
            return false;
        }

        ++process->current_burst_index;
        process->state = PROCESS_READY;
        process->ready_since = current_time;
        if (!ready_queue_push(ready, index)) {
            return false;
        }
        debug_log(config, "[t=%d] PID %d: BLOCKED -> READY\n",
                  current_time, process->pid);
    }

    return true;
}

static bool progress_blocked_io(Process *processes, size_t process_count) {
    size_t index;

    for (index = 0; index < process_count; ++index) {
        Process *process = &processes[index];
        Burst *burst;

        if (process->state != PROCESS_BLOCKED) {
            continue;
        }
        if (process->current_burst_index >= process->burst_count) {
            return false;
        }

        burst = &process->bursts[process->current_burst_index];
        if (burst->type != BURST_IO || burst->remaining_time <= 0) {
            return false;
        }
        --burst->remaining_time;
    }

    return true;
}

static bool advance_time(int *current_time) {
    if (current_time == NULL || *current_time == INT_MAX) {
        return false;
    }
    ++*current_time;
    return true;
}

static bool perform_context_switch(Process *processes,
                                   size_t process_count,
                                   ReadyQueue *ready,
                                   int *current_time,
                                   const SimulationConfig *config,
                                   SimulationResult *result) {
    int tick;

    ++result->context_switches;
    result->context_switch_ticks += (size_t) config->context_switch_cost;

    for (tick = 0; tick < config->context_switch_cost; ++tick) {
        debug_log(config, "[t=%d] troca de contexto (%d/%d)\n",
                  *current_time, tick + 1, config->context_switch_cost);

        if (!progress_blocked_io(processes, process_count) ||
            !advance_time(current_time)) {
            return false;
        }

        if (tick + 1 < config->context_switch_cost &&
            (!complete_io(processes, process_count, ready, *current_time, config) ||
             !admit_arrivals(processes, process_count, ready, *current_time, config))) {
            return false;
        }
    }

    return true;
}

SimulationConfig simulator_default_config(void) {
    return (SimulationConfig) {
        .context_switch_cost = 2,
        .debug = false,
        .debug_stream = NULL
    };
}

bool simulator_run(Process *processes,
                   size_t process_count,
                   const SimulationConfig *config,
                   const Scheduler *scheduler,
                   SimulationResult *result) {
    SimulationConfig effective_config;
    ReadyQueue ready = {0};
    SchedulerProcessView *ready_view = NULL;
    size_t running_index = NO_PROCESS;
    size_t previous_index = NO_PROCESS;
    bool previous_is_active = false;
    int current_time = 0;
    int slice_ticks = 0;
    bool success = false;

    if (!workload_is_valid(processes, process_count) || result == NULL ||
        scheduler == NULL || scheduler->ops == NULL ||
        scheduler->ops->pick_next == NULL) {
        return false;
    }

    effective_config = config != NULL ? *config : simulator_default_config();
    if (effective_config.context_switch_cost < 0) {
        return false;
    }

    if (!ready_queue_init(&ready, process_count) ||
        process_count > SIZE_MAX / sizeof(*ready_view)) {
        ready_queue_destroy(&ready);
        return false;
    }

    ready_view = malloc(process_count * sizeof(*ready_view));
    if (ready_view == NULL) {
        ready_queue_destroy(&ready);
        return false;
    }

    process_reset_array(processes, process_count);
    *result = (SimulationResult) {0};

    while (result->completed_processes < process_count) {
        Process *running;
        Burst *cpu_burst;

        if (!complete_io(processes, process_count, &ready,
                         current_time, &effective_config) ||
            !admit_arrivals(processes, process_count, &ready,
                            current_time, &effective_config)) {
            goto cleanup;
        }

        if (running_index == NO_PROCESS) {
            size_t selected_position;
            size_t selected_index;

            if (ready.count == 0) {
                debug_log(&effective_config, "[t=%d] CPU ociosa\n", current_time);
                if (!progress_blocked_io(processes, process_count) ||
                    !advance_time(&current_time)) {
                    goto cleanup;
                }
                ++result->idle_ticks;
                previous_is_active = false;
                continue;
            }

            if (!build_ready_view(processes, &ready, ready_view)) {
                goto cleanup;
            }
            selected_position = scheduler->ops->pick_next(
                ready_view, ready.count, current_time, scheduler->context);
            if (selected_position == SCHEDULER_NO_SELECTION ||
                selected_position >= ready.count) {
                goto cleanup;
            }
            selected_index = ready.items[selected_position];

            if (previous_is_active && selected_index != previous_index) {
                if (!perform_context_switch(processes, process_count, &ready,
                                            &current_time, &effective_config,
                                            result)) {
                    goto cleanup;
                }
                previous_is_active = false;
                continue;
            }

            running_index = ready_queue_remove_at(&ready, selected_position);
            if (running_index == NO_PROCESS) {
                goto cleanup;
            }
            processes[running_index].state = PROCESS_RUNNING;
            slice_ticks = 0;
            debug_log(&effective_config, "[t=%d] PID %d: READY -> RUNNING\n",
                      current_time, processes[running_index].pid);
        }

        running = &processes[running_index];
        if (running->state != PROCESS_RUNNING ||
            running->current_burst_index >= running->burst_count) {
            goto cleanup;
        }

        cpu_burst = &running->bursts[running->current_burst_index];
        if (cpu_burst->type != BURST_CPU || cpu_burst->remaining_time <= 0) {
            goto cleanup;
        }

        if (!progress_blocked_io(processes, process_count)) {
            goto cleanup;
        }

        --cpu_burst->remaining_time;
        ++running->total_cpu_executed;
        ++slice_ticks;
        debug_log(&effective_config, "[t=%d] PID %d executa CPU (restante=%d)\n",
                  current_time, running->pid, cpu_burst->remaining_time);

        if (!advance_time(&current_time)) {
            goto cleanup;
        }

        if (cpu_burst->remaining_time == 0) {
            ++running->completed_cpu_bursts;
            running->last_cpu_burst_duration = cpu_burst->duration;
            previous_index = running_index;
            previous_is_active = true;

            if (running->current_burst_index + 1 >= running->burst_count) {
                running->state = PROCESS_FINISHED;
                running->finish_time = current_time;
                ++result->completed_processes;
                debug_log(&effective_config,
                          "[t=%d] PID %d: RUNNING -> FINISHED\n",
                          current_time, running->pid);
            } else {
                ++running->current_burst_index;
                if (running->bursts[running->current_burst_index].type != BURST_IO) {
                    goto cleanup;
                }
                running->state = PROCESS_BLOCKED;
                debug_log(&effective_config,
                          "[t=%d] PID %d: RUNNING -> BLOCKED\n",
                          current_time, running->pid);
            }

            running_index = NO_PROCESS;
            slice_ticks = 0;
            continue;
        }

        if (scheduler->ops->should_preempt != NULL) {
            const SchedulerProcessView running_view =
                process_view(running, running_index);

            if (scheduler->ops->should_preempt(&running_view, slice_ticks,
                                               current_time,
                                               scheduler->context)) {
                running->state = PROCESS_READY;
                running->ready_since = current_time;
                if (!ready_queue_push(&ready, running_index)) {
                    goto cleanup;
                }
                previous_index = running_index;
                previous_is_active = true;
                debug_log(&effective_config,
                          "[t=%d] PID %d: RUNNING -> READY (preempcao)\n",
                          current_time, running->pid);
                running_index = NO_PROCESS;
                slice_ticks = 0;
            }
        }
    }

    result->elapsed_time = current_time;
    success = true;

cleanup:
    free(ready_view);
    ready_queue_destroy(&ready);
    return success;
}

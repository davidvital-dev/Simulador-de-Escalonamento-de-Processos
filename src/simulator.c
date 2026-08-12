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

typedef struct {
    size_t process_index;
    int arrival_time;
    int pid;
} ArrivalEvent;

typedef struct {
    ArrivalEvent *items;
    size_t count;
    size_t next;
} ArrivalQueue;

typedef struct {
    size_t process_index;
    int completion_time;
    int pid;
} BlockedEvent;

typedef struct {
    BlockedEvent *items;
    size_t count;
    size_t capacity;
} BlockedHeap;

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

static int compare_arrivals(const void *left_pointer, const void *right_pointer) {
    const ArrivalEvent *left = left_pointer;
    const ArrivalEvent *right = right_pointer;

    if (left->arrival_time != right->arrival_time) {
        return left->arrival_time < right->arrival_time ? -1 : 1;
    }
    if (left->pid != right->pid) {
        return left->pid < right->pid ? -1 : 1;
    }
    if (left->process_index == right->process_index) {
        return 0;
    }
    return left->process_index < right->process_index ? -1 : 1;
}

static bool arrival_queue_init(ArrivalQueue *arrivals,
                               const Process *processes,
                               size_t process_count) {
    size_t index;

    if (arrivals == NULL || processes == NULL || process_count == 0 ||
        process_count > SIZE_MAX / sizeof(*arrivals->items)) {
        return false;
    }

    arrivals->items = malloc(process_count * sizeof(*arrivals->items));
    if (arrivals->items == NULL) {
        return false;
    }

    arrivals->count = process_count;
    arrivals->next = 0;
    for (index = 0; index < process_count; ++index) {
        arrivals->items[index] = (ArrivalEvent) {
            .process_index = index,
            .arrival_time = processes[index].arrival_time,
            .pid = processes[index].pid
        };
    }

    qsort(arrivals->items, arrivals->count, sizeof(*arrivals->items),
          compare_arrivals);
    return true;
}

static void arrival_queue_destroy(ArrivalQueue *arrivals) {
    if (arrivals == NULL) {
        return;
    }

    free(arrivals->items);
    *arrivals = (ArrivalQueue) {0};
}

static bool blocked_event_precedes(BlockedEvent left, BlockedEvent right) {
    if (left.completion_time != right.completion_time) {
        return left.completion_time < right.completion_time;
    }
    if (left.pid != right.pid) {
        return left.pid < right.pid;
    }
    return left.process_index < right.process_index;
}

static bool blocked_heap_init(BlockedHeap *heap, size_t capacity) {
    if (heap == NULL || capacity == 0 ||
        capacity > SIZE_MAX / sizeof(*heap->items)) {
        return false;
    }

    heap->items = malloc(capacity * sizeof(*heap->items));
    if (heap->items == NULL) {
        return false;
    }
    heap->count = 0;
    heap->capacity = capacity;
    return true;
}

static void blocked_heap_destroy(BlockedHeap *heap) {
    if (heap == NULL) {
        return;
    }

    free(heap->items);
    *heap = (BlockedHeap) {0};
}

static bool blocked_heap_push(BlockedHeap *heap, BlockedEvent event) {
    size_t index;

    if (heap == NULL || heap->items == NULL || heap->count >= heap->capacity) {
        return false;
    }

    index = heap->count++;
    heap->items[index] = event;
    while (index > 0) {
        const size_t parent = (index - 1) / 2;
        BlockedEvent temporary;

        if (!blocked_event_precedes(heap->items[index], heap->items[parent])) {
            break;
        }
        temporary = heap->items[parent];
        heap->items[parent] = heap->items[index];
        heap->items[index] = temporary;
        index = parent;
    }

    return true;
}

static bool blocked_heap_pop(BlockedHeap *heap, BlockedEvent *event) {
    size_t index = 0;

    if (heap == NULL || event == NULL || heap->count == 0) {
        return false;
    }

    *event = heap->items[0];
    --heap->count;
    if (heap->count == 0) {
        return true;
    }

    heap->items[0] = heap->items[heap->count];
    while (true) {
        const size_t left = index * 2 + 1;
        const size_t right = left + 1;
        size_t smallest = index;
        BlockedEvent temporary;

        if (left < heap->count &&
            blocked_event_precedes(heap->items[left], heap->items[smallest])) {
            smallest = left;
        }
        if (right < heap->count &&
            blocked_event_precedes(heap->items[right], heap->items[smallest])) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }

        temporary = heap->items[index];
        heap->items[index] = heap->items[smallest];
        heap->items[smallest] = temporary;
        index = smallest;
    }

    return true;
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
                           ArrivalQueue *arrivals,
                           ReadyQueue *ready,
                           int current_time,
                           const SimulationConfig *config) {
    while (arrivals->next < arrivals->count &&
           arrivals->items[arrivals->next].arrival_time <= current_time) {
        const ArrivalEvent event = arrivals->items[arrivals->next++];
        Process *process = &processes[event.process_index];

        if (process->state != PROCESS_NEW) {
            return false;
        }
        process->state = PROCESS_READY;
        process->ready_since = event.arrival_time;
        if (!ready_queue_push(ready, event.process_index)) {
            return false;
        }
        debug_log(config, "[t=%d] PID %d: NEW -> READY\n",
                  current_time, process->pid);
    }

    return true;
}

static bool complete_io(Process *processes,
                        BlockedHeap *blocked,
                        ReadyQueue *ready,
                        int current_time,
                        const SimulationConfig *config) {
    while (blocked->count > 0 &&
           blocked->items[0].completion_time <= current_time) {
        BlockedEvent event;
        Process *process;
        Burst *io_burst;

        if (!blocked_heap_pop(blocked, &event)) {
            return false;
        }
        process = &processes[event.process_index];
        if (process->state != PROCESS_BLOCKED ||
            process->current_burst_index >= process->burst_count) {
            return false;
        }

        io_burst = &process->bursts[process->current_burst_index];
        if (io_burst->type != BURST_IO ||
            process->current_burst_index + 1 >= process->burst_count ||
            process->bursts[process->current_burst_index + 1].type != BURST_CPU) {
            return false;
        }

        io_burst->remaining_time = 0;
        ++process->current_burst_index;
        process->state = PROCESS_READY;
        process->ready_since = event.completion_time;
        if (!ready_queue_push(ready, event.process_index)) {
            return false;
        }
        debug_log(config, "[t=%d] PID %d: BLOCKED -> READY\n",
                  current_time, process->pid);
    }

    return true;
}

static bool process_system_events(Process *processes,
                                  ArrivalQueue *arrivals,
                                  BlockedHeap *blocked,
                                  ReadyQueue *ready,
                                  int current_time,
                                  const SimulationConfig *config) {
    /* A ordem oficial do tick coloca conclusoes de E/S antes de chegadas. */
    return complete_io(processes, blocked, ready, current_time, config) &&
           admit_arrivals(processes, arrivals, ready, current_time, config);
}

static bool advance_time(int *current_time) {
    if (current_time == NULL || *current_time == INT_MAX) {
        return false;
    }
    ++*current_time;
    return true;
}

static bool perform_context_switch(Process *processes,
                                   ArrivalQueue *arrivals,
                                   BlockedHeap *blocked,
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

        if (!advance_time(current_time) ||
            !process_system_events(processes, arrivals, blocked, ready,
                                   *current_time, config)) {
            return false;
        }
    }

    return true;
}

static bool idle_until_next_event(const ArrivalQueue *arrivals,
                                  const BlockedHeap *blocked,
                                  int *current_time,
                                  SimulationResult *result,
                                  const SimulationConfig *config) {
    int next_time = INT_MAX;
    int delta;

    if (arrivals->next < arrivals->count &&
        arrivals->items[arrivals->next].arrival_time < next_time) {
        next_time = arrivals->items[arrivals->next].arrival_time;
    }
    if (blocked->count > 0 && blocked->items[0].completion_time < next_time) {
        next_time = blocked->items[0].completion_time;
    }
    if (next_time == INT_MAX || next_time <= *current_time) {
        return false;
    }

    delta = next_time - *current_time;
    debug_log(config, "[t=%d] CPU ociosa por %d tick(s)\n",
              *current_time, delta);
    result->idle_ticks += (size_t) delta;
    *current_time = next_time;
    return true;
}

static bool block_running_process(Process *process,
                                  size_t process_index,
                                  int current_time,
                                  BlockedHeap *blocked) {
    Burst *io_burst;
    int completion_time;

    ++process->current_burst_index;
    if (process->current_burst_index >= process->burst_count) {
        return false;
    }

    io_burst = &process->bursts[process->current_burst_index];
    if (io_burst->type != BURST_IO || io_burst->duration <= 0 ||
        io_burst->duration > INT_MAX - current_time) {
        return false;
    }

    completion_time = current_time + io_burst->duration;
    process->state = PROCESS_BLOCKED;
    return blocked_heap_push(blocked, (BlockedEvent) {
        .process_index = process_index,
        .completion_time = completion_time,
        .pid = process->pid
    });
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
    ArrivalQueue arrivals = {0};
    BlockedHeap blocked = {0};
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
    if (effective_config.context_switch_cost < 0 ||
        process_count > SIZE_MAX / sizeof(*ready_view)) {
        return false;
    }

    if (!ready_queue_init(&ready, process_count) ||
        !arrival_queue_init(&arrivals, processes, process_count) ||
        !blocked_heap_init(&blocked, process_count)) {
        ready_queue_destroy(&ready);
        arrival_queue_destroy(&arrivals);
        blocked_heap_destroy(&blocked);
        return false;
    }

    ready_view = malloc(process_count * sizeof(*ready_view));
    if (ready_view == NULL) {
        ready_queue_destroy(&ready);
        arrival_queue_destroy(&arrivals);
        blocked_heap_destroy(&blocked);
        return false;
    }

    process_reset_array(processes, process_count);
    *result = (SimulationResult) {0};

    while (result->completed_processes < process_count) {
        Process *running;
        Burst *cpu_burst;

        if (!process_system_events(processes, &arrivals, &blocked, &ready,
                                   current_time, &effective_config)) {
            goto cleanup;
        }

        if (running_index == NO_PROCESS) {
            size_t selected_position;
            size_t selected_index;

            if (ready.count == 0) {
                if (!idle_until_next_event(&arrivals, &blocked, &current_time,
                                           result, &effective_config)) {
                    goto cleanup;
                }
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
                if (!perform_context_switch(processes, &arrivals, &blocked,
                                            &ready, &current_time,
                                            &effective_config, result)) {
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
                if (!block_running_process(running, running_index,
                                           current_time, &blocked)) {
                    goto cleanup;
                }
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
    arrival_queue_destroy(&arrivals);
    blocked_heap_destroy(&blocked);
    return success;
}

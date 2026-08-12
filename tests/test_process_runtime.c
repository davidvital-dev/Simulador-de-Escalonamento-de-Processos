#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "process.h"

static Process make_process(void) {
    Process process = {
        .pid = 7,
        .arrival_time = 3,
        .priority = 2,
        .state = PROCESS_FINISHED,
        .bursts = calloc(3, sizeof(Burst)),
        .burst_count = 3,
        .current_burst_index = 2,
        .finish_time = 99,
        .ready_since = 12,
        .total_cpu_executed = 8,
        .completed_cpu_bursts = 2,
        .last_cpu_burst_duration = 5
    };

    assert(process.bursts != NULL);
    process.bursts[0] = (Burst) {BURST_CPU, 3, 0};
    process.bursts[1] = (Burst) {BURST_IO, 4, 0};
    process.bursts[2] = (Burst) {BURST_CPU, 5, 0};
    return process;
}

static void test_reset_runtime(void) {
    Process process = make_process();

    process_reset_runtime(&process);

    assert(process.state == PROCESS_NEW);
    assert(process.current_burst_index == 0);
    assert(process.finish_time == -1);
    assert(process.ready_since == -1);
    assert(process.total_cpu_executed == 0);
    assert(process.completed_cpu_bursts == 0);
    assert(process.last_cpu_burst_duration == -1);
    assert(process.bursts[0].remaining_time == 3);
    assert(process.bursts[1].remaining_time == 4);
    assert(process.bursts[2].remaining_time == 5);

    process_destroy(&process);
    assert(process.bursts == NULL);
    assert(process.burst_count == 0);
}

static void test_deep_clone(void) {
    Process original = make_process();
    Process copy = {0};

    assert(process_clone(&original, &copy));
    assert(copy.bursts != original.bursts);
    assert(copy.pid == original.pid);
    assert(copy.burst_count == original.burst_count);
    assert(copy.bursts[2].duration == original.bursts[2].duration);

    copy.bursts[0].duration = 42;
    copy.priority = 9;
    assert(original.bursts[0].duration == 3);
    assert(original.priority == 2);

    process_destroy(&copy);
    process_destroy(&original);
}

static void test_array_clone(void) {
    Process originals[2] = {make_process(), make_process()};
    Process *copies = NULL;

    originals[1].pid = 8;
    assert(process_clone_array(originals, 2, &copies));
    assert(copies != NULL);
    assert(copies[0].bursts != originals[0].bursts);
    assert(copies[1].bursts != originals[1].bursts);
    assert(copies[1].pid == 8);

    copies[1].bursts[2].remaining_time = 1;
    assert(originals[1].bursts[2].remaining_time == 0);

    process_array_destroy(copies, 2);
    process_destroy(&originals[0]);
    process_destroy(&originals[1]);
}

int main(void) {
    test_reset_runtime();
    test_deep_clone();
    test_array_clone();

    puts("OK: runtime e copia profunda de processos validados.");
    return 0;
}

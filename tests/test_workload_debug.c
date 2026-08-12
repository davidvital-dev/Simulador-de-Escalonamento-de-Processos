#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "workload.h"

static void test_complete_output(void) {
    const ScenarioConfig config = {
        .type = SCENARIO_BALANCED,
        .interarrival_time = {2, 2},
        .priority = {7, 7},
        .cpu_burst_count = {2, 2},
        .cpu_burst_duration = {11, 11},
        .io_burst_duration = {13, 13},
        .priority_bias_percent = 0,
        .biased_priority = {0, 0}
    };
    const char *expected =
        "Workload scenario=balanced seed=123 processes=2\n"
        "PID ARRIVAL PRIORITY BURSTS\n"
        "1 0 7 CPU(11) -> IO(13) -> CPU(11)\n"
        "2 2 7 CPU(11) -> IO(13) -> CPU(11)\n";
    Workload workload = {0};
    FILE *stream;
    char output[512];
    size_t bytes_read;

    assert(workload_generate(123, &config, 2, &workload));
    stream = tmpfile();
    assert(stream != NULL);
    assert(workload_print_debug(&workload, stream, 2));
    rewind(stream);
    bytes_read = fread(output, 1, sizeof(output) - 1, stream);
    output[bytes_read] = '\0';

    assert(strcmp(output, expected) == 0);

    fclose(stream);
    workload_free(&workload);
}

static void test_limited_output(void) {
    ScenarioConfig config;
    Workload workload = {0};
    FILE *stream;
    char output[1024];
    size_t bytes_read;

    assert(workload_default_config(SCENARIO_IO_BOUND, &config));
    assert(workload_generate(456, &config, 5, &workload));
    stream = tmpfile();
    assert(stream != NULL);
    assert(workload_print_debug(&workload, stream, 2));
    rewind(stream);
    bytes_read = fread(output, 1, sizeof(output) - 1, stream);
    output[bytes_read] = '\0';

    assert(strstr(output, "scenario=io-bound") != NULL);
    assert(strstr(output, "... 3 process(es) omitted\n") != NULL);

    fclose(stream);
    workload_free(&workload);
}

static void test_invalid_arguments(void) {
    Workload workload = {0};
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(!workload_print_debug(NULL, stream, 1));
    assert(!workload_print_debug(&workload, NULL, 1));
    fclose(stream);
}

int main(void) {
    test_complete_output();
    test_limited_output();
    test_invalid_arguments();

    puts("OK: impressao de debug validada.");
    return 0;
}

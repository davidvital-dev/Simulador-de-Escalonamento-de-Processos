CC := gcc
PYTHON := python3
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude -g
BUILD_DIR := build
TARGET := $(BUILD_DIR)/simulator
WORKLOAD_DETERMINISM_TEST := $(BUILD_DIR)/tests/test_workload_determinism
WORKLOAD_BURSTS_TEST := $(BUILD_DIR)/tests/test_workload_bursts
WORKLOAD_SCENARIOS_TEST := $(BUILD_DIR)/tests/test_workload_scenarios
WORKLOAD_CONFIG_TEST := $(BUILD_DIR)/tests/test_workload_config
WORKLOAD_DEBUG_TEST := $(BUILD_DIR)/tests/test_workload_debug
PROCESS_RUNTIME_TEST := $(BUILD_DIR)/tests/test_process_runtime
SIMULATOR_STATES_TEST := $(BUILD_DIR)/tests/test_simulator_states
SIMULATOR_CONTEXT_TEST := $(BUILD_DIR)/tests/test_simulator_context

SOURCES := $(wildcard src/*.c) $(wildcard src/schedulers/*.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run test-workload-determinism test-workload-bursts \
	test-workload-scenarios test-workload-config test-workload-debug \
	test-experiment-seeds test-workload test-process-runtime \
	test-simulator-states test-simulator-context test-simulator test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: test-workload test-simulator

test-workload: test-experiment-seeds test-workload-determinism \
	test-workload-bursts test-workload-scenarios test-workload-config \
	test-workload-debug

test-workload-determinism: $(WORKLOAD_DETERMINISM_TEST)
	./$(WORKLOAD_DETERMINISM_TEST)

$(WORKLOAD_DETERMINISM_TEST): tests/test_workload_determinism.c src/workload.c include/workload.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_workload_determinism.c src/workload.c -o $@

test-workload-bursts: $(WORKLOAD_BURSTS_TEST)
	./$(WORKLOAD_BURSTS_TEST)

$(WORKLOAD_BURSTS_TEST): tests/test_workload_bursts.c src/workload.c include/workload.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_workload_bursts.c src/workload.c -o $@

test-workload-scenarios: $(WORKLOAD_SCENARIOS_TEST)
	./$(WORKLOAD_SCENARIOS_TEST)

$(WORKLOAD_SCENARIOS_TEST): tests/test_workload_scenarios.c src/workload.c include/workload.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_workload_scenarios.c src/workload.c -o $@

test-workload-config: $(WORKLOAD_CONFIG_TEST)
	./$(WORKLOAD_CONFIG_TEST)

$(WORKLOAD_CONFIG_TEST): tests/test_workload_config.c src/workload.c include/workload.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_workload_config.c src/workload.c -o $@

test-workload-debug: $(WORKLOAD_DEBUG_TEST)
	./$(WORKLOAD_DEBUG_TEST)

$(WORKLOAD_DEBUG_TEST): tests/test_workload_debug.c src/workload.c include/workload.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_workload_debug.c src/workload.c -o $@

test-experiment-seeds:
	$(PYTHON) tests/test_experiment_seeds.py

test-process-runtime: $(PROCESS_RUNTIME_TEST)
	./$(PROCESS_RUNTIME_TEST)

$(PROCESS_RUNTIME_TEST): tests/test_process_runtime.c src/process.c include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_process_runtime.c src/process.c -o $@

test-simulator-states: $(SIMULATOR_STATES_TEST)
	./$(SIMULATOR_STATES_TEST)

$(SIMULATOR_STATES_TEST): tests/test_simulator_states.c src/simulator.c src/process.c include/simulator.h include/scheduler.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_simulator_states.c src/simulator.c src/process.c -o $@

test-simulator-context: $(SIMULATOR_CONTEXT_TEST)
	./$(SIMULATOR_CONTEXT_TEST)

$(SIMULATOR_CONTEXT_TEST): tests/test_simulator_context.c src/simulator.c src/process.c include/simulator.h include/scheduler.h include/process.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/test_simulator_context.c src/simulator.c src/process.c -o $@

test-simulator: test-process-runtime test-simulator-states test-simulator-context

clean:
	rm -rf $(BUILD_DIR)

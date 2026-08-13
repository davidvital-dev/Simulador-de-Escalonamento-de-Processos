CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude -g
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
PYTHON ?= py
EXEEXT := .exe
MKDIR_P = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
RUN_BIN = $(subst /,\,$(1))
CLEAN_BUILD = if exist "$(subst /,\,$(BUILD_DIR))" rmdir /S /Q "$(subst /,\,$(BUILD_DIR))"
else
PYTHON ?= python3
EXEEXT :=
MKDIR_P = mkdir -p "$(1)"
RUN_BIN = ./$(1)
CLEAN_BUILD = rm -rf "$(BUILD_DIR)"
endif

TARGET := $(BUILD_DIR)/simulator$(EXEEXT)
WORKLOAD_DETERMINISM_TEST := $(BUILD_DIR)/tests/test_workload_determinism$(EXEEXT)
WORKLOAD_BURSTS_TEST := $(BUILD_DIR)/tests/test_workload_bursts$(EXEEXT)
WORKLOAD_SCENARIOS_TEST := $(BUILD_DIR)/tests/test_workload_scenarios$(EXEEXT)
WORKLOAD_CONFIG_TEST := $(BUILD_DIR)/tests/test_workload_config$(EXEEXT)
WORKLOAD_DEBUG_TEST := $(BUILD_DIR)/tests/test_workload_debug$(EXEEXT)
PROCESS_RUNTIME_TEST := $(BUILD_DIR)/tests/test_process_runtime$(EXEEXT)
SIMULATOR_STATES_TEST := $(BUILD_DIR)/tests/test_simulator_states$(EXEEXT)
SIMULATOR_CONTEXT_TEST := $(BUILD_DIR)/tests/test_simulator_context$(EXEEXT)
SIMULATOR_WORKLOAD_TEST := $(BUILD_DIR)/tests/test_simulator_workload$(EXEEXT)
METRICS_TURNAROUND_TEST := $(BUILD_DIR)/tests/test_metrics_turnaround$(EXEEXT)

SOURCES := $(wildcard src/*.c) $(wildcard src/schedulers/*.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run test-workload-determinism test-workload-bursts \
	test-workload-scenarios test-workload-config test-workload-debug \
	test-experiment-seeds test-workload test-process-runtime \
	test-simulator-states test-simulator-context test-simulator-workload \
	test-simulator test-metrics-turnaround test-metrics test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.c
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	$(call RUN_BIN,$(TARGET))

test: test-workload test-simulator test-metrics

test-workload: test-experiment-seeds test-workload-determinism \
	test-workload-bursts test-workload-scenarios test-workload-config \
	test-workload-debug

test-workload-determinism: $(WORKLOAD_DETERMINISM_TEST)
	$(call RUN_BIN,$(WORKLOAD_DETERMINISM_TEST))

$(WORKLOAD_DETERMINISM_TEST): tests/test_workload_determinism.c src/workload.c include/workload.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_workload_determinism.c src/workload.c -o $@

test-workload-bursts: $(WORKLOAD_BURSTS_TEST)
	$(call RUN_BIN,$(WORKLOAD_BURSTS_TEST))

$(WORKLOAD_BURSTS_TEST): tests/test_workload_bursts.c src/workload.c include/workload.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_workload_bursts.c src/workload.c -o $@

test-workload-scenarios: $(WORKLOAD_SCENARIOS_TEST)
	$(call RUN_BIN,$(WORKLOAD_SCENARIOS_TEST))

$(WORKLOAD_SCENARIOS_TEST): tests/test_workload_scenarios.c src/workload.c include/workload.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_workload_scenarios.c src/workload.c -o $@

test-workload-config: $(WORKLOAD_CONFIG_TEST)
	$(call RUN_BIN,$(WORKLOAD_CONFIG_TEST))

$(WORKLOAD_CONFIG_TEST): tests/test_workload_config.c src/workload.c include/workload.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_workload_config.c src/workload.c -o $@

test-workload-debug: $(WORKLOAD_DEBUG_TEST)
	$(call RUN_BIN,$(WORKLOAD_DEBUG_TEST))

$(WORKLOAD_DEBUG_TEST): tests/test_workload_debug.c src/workload.c include/workload.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_workload_debug.c src/workload.c -o $@

test-experiment-seeds:
	$(PYTHON) tests/test_experiment_seeds.py

test-process-runtime: $(PROCESS_RUNTIME_TEST)
	$(call RUN_BIN,$(PROCESS_RUNTIME_TEST))

$(PROCESS_RUNTIME_TEST): tests/test_process_runtime.c src/process.c include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_process_runtime.c src/process.c -o $@

test-simulator-states: $(SIMULATOR_STATES_TEST)
	$(call RUN_BIN,$(SIMULATOR_STATES_TEST))

$(SIMULATOR_STATES_TEST): tests/test_simulator_states.c src/simulator.c src/process.c include/simulator.h include/scheduler.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_simulator_states.c src/simulator.c src/process.c -o $@

test-simulator-context: $(SIMULATOR_CONTEXT_TEST)
	$(call RUN_BIN,$(SIMULATOR_CONTEXT_TEST))

$(SIMULATOR_CONTEXT_TEST): tests/test_simulator_context.c src/simulator.c src/process.c include/simulator.h include/scheduler.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_simulator_context.c src/simulator.c src/process.c -o $@

test-simulator-workload: $(SIMULATOR_WORKLOAD_TEST)
	$(call RUN_BIN,$(SIMULATOR_WORKLOAD_TEST))

$(SIMULATOR_WORKLOAD_TEST): tests/test_simulator_workload.c src/simulator.c src/process.c src/workload.c include/simulator.h include/scheduler.h include/process.h include/workload.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_simulator_workload.c src/simulator.c src/process.c src/workload.c -o $@

test-simulator: test-process-runtime test-simulator-states test-simulator-context \
	test-simulator-workload

test-metrics: test-metrics-turnaround

test-metrics-turnaround: $(METRICS_TURNAROUND_TEST)
	$(call RUN_BIN,$(METRICS_TURNAROUND_TEST))

$(METRICS_TURNAROUND_TEST): tests/test_metrics_turnaround.c src/metrics.c include/metrics.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_metrics_turnaround.c src/metrics.c -o $@

clean:
	@$(CLEAN_BUILD)

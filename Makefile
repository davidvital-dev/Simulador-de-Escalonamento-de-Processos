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
SCHEDULER_FCFS_TEST := $(BUILD_DIR)/tests/test_scheduler_fcfs$(EXEEXT)
SCHEDULER_ROUND_ROBIN_TEST := $(BUILD_DIR)/tests/test_scheduler_round_robin$(EXEEXT)
SCHEDULER_PRIORITY_TEST := $(BUILD_DIR)/tests/test_scheduler_priority$(EXEEXT)
SCHEDULER_PROPOSED_TEST := $(BUILD_DIR)/tests/test_scheduler_proposed$(EXEEXT)
SCHEDULER_PROPOSED_INTEGRATION_TEST := $(BUILD_DIR)/tests/test_scheduler_proposed_integration$(EXEEXT)
METRICS_TURNAROUND_TEST := $(BUILD_DIR)/tests/test_metrics_turnaround$(EXEEXT)

SOURCES := $(wildcard src/*.c) $(wildcard src/schedulers/*.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run test-workload-determinism test-workload-bursts \
	test-workload-scenarios test-workload-config test-workload-debug \
	test-experiment-seeds test-workload test-process-runtime \
	test-simulator-states test-simulator-context test-simulator-workload \
	test-simulator test-scheduler-fcfs test-scheduler-round-robin \
	test-scheduler-priority test-scheduler-proposed \
	test-scheduler-proposed-integration test-schedulers \
	test-metrics-turnaround test-metrics test-stats test-plots \
	test-run-experiments test-validate-dataset test-analyze-results test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.c
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	$(call RUN_BIN,$(TARGET))

test: test-workload test-simulator test-schedulers test-metrics test-stats test-plots test-run-experiments test-validate-dataset test-analyze-results

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

test-scheduler-fcfs: $(SCHEDULER_FCFS_TEST)
	$(call RUN_BIN,$(SCHEDULER_FCFS_TEST))

$(SCHEDULER_FCFS_TEST): tests/test_scheduler_fcfs.c src/schedulers/fcfs.c src/simulator.c src/process.c include/fcfs.h include/simulator.h include/scheduler.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_scheduler_fcfs.c src/schedulers/fcfs.c src/simulator.c src/process.c -o $@

test-scheduler-round-robin: $(SCHEDULER_ROUND_ROBIN_TEST)
	$(call RUN_BIN,$(SCHEDULER_ROUND_ROBIN_TEST))

$(SCHEDULER_ROUND_ROBIN_TEST): tests/test_scheduler_round_robin.c src/schedulers/round_robin.c src/simulator.c src/process.c include/round_robin.h include/simulator.h include/scheduler.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_scheduler_round_robin.c src/schedulers/round_robin.c src/simulator.c src/process.c -o $@

test-scheduler-priority: $(SCHEDULER_PRIORITY_TEST)
	$(call RUN_BIN,$(SCHEDULER_PRIORITY_TEST))

$(SCHEDULER_PRIORITY_TEST): tests/test_scheduler_priority.c src/schedulers/priority.c src/simulator.c src/process.c include/priority.h include/simulator.h include/scheduler.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_scheduler_priority.c src/schedulers/priority.c src/simulator.c src/process.c -o $@

test-scheduler-proposed: $(SCHEDULER_PROPOSED_TEST)
	$(call RUN_BIN,$(SCHEDULER_PROPOSED_TEST))

$(SCHEDULER_PROPOSED_TEST): tests/test_scheduler_proposed.c src/schedulers/proposed.c src/simulator.c src/process.c include/proposed.h include/simulator.h include/scheduler.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_scheduler_proposed.c src/schedulers/proposed.c src/simulator.c src/process.c -o $@

test-scheduler-proposed-integration: $(SCHEDULER_PROPOSED_INTEGRATION_TEST)
	$(call RUN_BIN,$(SCHEDULER_PROPOSED_INTEGRATION_TEST))

$(SCHEDULER_PROPOSED_INTEGRATION_TEST): tests/test_scheduler_proposed_integration.c src/schedulers/proposed.c src/simulator.c src/process.c src/workload.c include/proposed.h include/simulator.h include/scheduler.h include/process.h include/workload.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_scheduler_proposed_integration.c src/schedulers/proposed.c src/simulator.c src/process.c src/workload.c -o $@

test-schedulers: test-scheduler-fcfs test-scheduler-round-robin test-scheduler-priority \
	test-scheduler-proposed test-scheduler-proposed-integration

test-metrics: test-metrics-turnaround

test-metrics-turnaround: $(METRICS_TURNAROUND_TEST)
	$(call RUN_BIN,$(METRICS_TURNAROUND_TEST))

$(METRICS_TURNAROUND_TEST): tests/test_metrics_turnaround.c src/metrics.c include/metrics.h include/process.h
	@$(call MKDIR_P,$(patsubst %/,%,$(dir $@)))
	$(CC) $(CFLAGS) tests/test_metrics_turnaround.c -o $@

test-stats:
	$(PYTHON) tests/test_stats.py

test-plots:
	$(PYTHON) tests/test_plots.py

test-run-experiments:
	$(PYTHON) tests/test_run_experiments.py

test-validate-dataset:
	$(PYTHON) tests/test_validate_dataset.py

test-analyze-results:
	$(PYTHON) tests/test_analyze_results.py

clean:
	@$(CLEAN_BUILD)

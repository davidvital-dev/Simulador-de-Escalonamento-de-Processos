CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude -g
BUILD_DIR := build
TARGET := $(BUILD_DIR)/simulator
WORKLOAD_DETERMINISM_TEST := $(BUILD_DIR)/tests/test_workload_determinism
WORKLOAD_BURSTS_TEST := $(BUILD_DIR)/tests/test_workload_bursts
WORKLOAD_SCENARIOS_TEST := $(BUILD_DIR)/tests/test_workload_scenarios
WORKLOAD_CONFIG_TEST := $(BUILD_DIR)/tests/test_workload_config
WORKLOAD_DEBUG_TEST := $(BUILD_DIR)/tests/test_workload_debug

SOURCES := $(wildcard src/*.c) $(wildcard src/schedulers/*.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run test-workload-determinism test-workload-bursts \
	test-workload-scenarios test-workload-config test-workload-debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

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

clean:
	rm -rf $(BUILD_DIR)

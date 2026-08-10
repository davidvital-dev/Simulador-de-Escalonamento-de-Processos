CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude -g
BUILD_DIR := build
TARGET := $(BUILD_DIR)/simulator

SOURCES := $(wildcard src/*.c) $(wildcard src/schedulers/*.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

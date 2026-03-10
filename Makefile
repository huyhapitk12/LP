# LP Compiler - Makefile
# Usage:
#   make          Build the LP compiler
#   make clean    Remove build artifacts
#   make install  Install to /usr/local/bin (Linux/macOS)

CC      ?= gcc
CFLAGS  := -std=c99 -O2 -Wall
LDFLAGS := -lm

SRC_DIR := compiler/src
INC_DIR := -I compiler/src -I runtime
BUILD_DIR := build

SRCS := $(SRC_DIR)/main.c \
        $(SRC_DIR)/lexer.c \
        $(SRC_DIR)/ast.c \
        $(SRC_DIR)/parser.c \
        $(SRC_DIR)/codegen.c \
        $(SRC_DIR)/repl.c

TARGET := $(BUILD_DIR)/lp

# Windows detection
ifeq ($(OS),Windows_NT)
    TARGET := $(BUILD_DIR)/lp.exe
    MKDIR  := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
else
    MKDIR  := mkdir -p $(BUILD_DIR)
endif

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRCS)
	$(MKDIR)
	$(CC) $(CFLAGS) $(SRCS) $(INC_DIR) -o $(TARGET) $(LDFLAGS)
	@echo "[LP] Build successful: $(TARGET)"

clean:
ifeq ($(OS),Windows_NT)
	if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
else
	rm -rf $(BUILD_DIR)
endif

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/lp
	@echo "[LP] Installed to /usr/local/bin/lp"

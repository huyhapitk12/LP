# LP Compiler - Makefile
# Usage:
#   make          Build the LP compiler
#   make clean    Remove build artifacts
#   make install  Install to /usr/local/bin (Linux/macOS)

CC      ?= gcc
WARNFLAGS ?= -Wall -Wextra -Wpedantic
CFLAGS  := -std=c99 -O2 $(WARNFLAGS)
LDFLAGS := -lm

SRC_DIR := compiler/src
INC_DIR := -I compiler/src -I runtime
BUILD_DIR := build
SQLITE_OBJ := runtime/sqlite3.o

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

.PHONY: all clean install test-c

all: $(TARGET) $(SQLITE_OBJ)

test-c: $(SRC_DIR)/ast.c compiler/tests/test_ast.c
	$(MKDIR)
	$(CC) $(CFLAGS) -fsanitize=address -g $(SRC_DIR)/ast.c compiler/tests/test_ast.c $(INC_DIR) -o $(BUILD_DIR)/test_ast
	@echo "[LP] Running C tests with AddressSanitizer..."
	@$(BUILD_DIR)/test_ast

$(TARGET): $(SRCS)
	$(MKDIR)
	$(CC) $(CFLAGS) $(SRCS) $(INC_DIR) -o $(TARGET) $(LDFLAGS)
	@echo "[LP] Build successful: $(TARGET)"

$(SQLITE_OBJ): runtime/sqlite3.c runtime/sqlite3.h
	$(CC) -O2 -c runtime/sqlite3.c -o $(SQLITE_OBJ)
	@echo "[LP] Built runtime object: $(SQLITE_OBJ)"

clean:
ifeq ($(OS),Windows_NT)
	if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
else
	rm -rf $(BUILD_DIR)
	rm -f $(SQLITE_OBJ)
endif

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/lp
	@echo "[LP] Installed to /usr/local/bin/lp"

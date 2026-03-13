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
        $(SRC_DIR)/repl.c \
        $(SRC_DIR)/process_utils.c

TARGET := $(BUILD_DIR)/lp

# Windows detection
ifeq ($(OS),Windows_NT)
    TARGET := $(BUILD_DIR)/lp.exe
    MKDIR  := if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
    EXE_EXT := .exe
else
    MKDIR  := mkdir -p $(BUILD_DIR)
    EXE_EXT := 
endif

.PHONY: all clean install test_unit test-c

all: $(TARGET) $(SQLITE_OBJ)

$(TARGET): $(SRCS)
	$(MKDIR)
	$(CC) $(CFLAGS) $(SRCS) $(INC_DIR) -o $(TARGET) $(LDFLAGS)
	@echo "[LP] Build successful: $(TARGET)"

$(SQLITE_OBJ): runtime/sqlite3.c runtime/sqlite3.h
	$(CC) -O2 -c runtime/sqlite3.c -o $(SQLITE_OBJ)
	@echo "[LP] Built runtime object: $(SQLITE_OBJ)"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(SQLITE_OBJ)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/lp
	@echo "[LP] Installed to /usr/local/bin/lp"

test_unit: $(SRC_DIR)/test_ast_unit.c $(SRC_DIR)/ast.c $(SRC_DIR)/lexer.c
	$(MKDIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/test_ast_unit.c $(SRC_DIR)/ast.c $(SRC_DIR)/lexer.c $(INC_DIR) -o $(BUILD_DIR)/test_ast_unit$(EXE_EXT) $(LDFLAGS)
	./$(BUILD_DIR)/test_ast_unit$(EXE_EXT)

test-c: compiler/tests/test_codegen.c compiler/src/codegen.c compiler/tests/test_lexer.c compiler/src/lexer.c
	$(MKDIR)
	$(CC) $(CFLAGS) compiler/tests/test_codegen.c compiler/src/codegen.c -I compiler/src -I runtime -o $(BUILD_DIR)/test_codegen$(EXE_EXT) $(LDFLAGS)
	./$(BUILD_DIR)/test_codegen$(EXE_EXT)
	$(CC) $(CFLAGS) compiler/tests/test_lexer.c compiler/src/lexer.c -I compiler/src -o $(BUILD_DIR)/test_lexer$(EXE_EXT) $(LDFLAGS)
	./$(BUILD_DIR)/test_lexer$(EXE_EXT)
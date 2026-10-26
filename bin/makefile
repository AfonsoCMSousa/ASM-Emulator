# Compiler settings
CC = g++
CFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lncurses

# Output executable name
OUT = ASM

# Source files
SRC = ../src/main.cpp ../src/panels/window.cpp ../src/files/rw.cpp
HDR = ../src/cpu.h ../src/panels/window.h ../src/files/rw.h

# Test runner
TEST_SRC = ../tests/cpu_test.cpp
TEST_OUT = cpu_test

# OS detection
ifeq ($(OS),Windows_NT)
    RM = del /Q
    EXE = .exe
    CFLAGS += -I"C:/msys64/mingw64/include"
    LDFLAGS += -L"C:/msys64/mingw64/lib" -lncursesw -static-libgcc -static-libstdc++
else
    RM = rm -f
    EXE =
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        CC = clang++
        CFLAGS += -I/usr/local/include
        LDFLAGS = -L/usr/local/lib -lncurses
    endif
endif

.PHONY: all test clean

# All target
all: $(OUT)$(EXE)

# Build target
$(OUT)$(EXE): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)$(EXE) $(LDFLAGS)

# Headless assembler/CPU tests (no ncurses needed)
test: $(TEST_OUT)$(EXE)
	"./$(TEST_OUT)$(EXE)"

$(TEST_OUT)$(EXE): $(TEST_SRC) ../src/cpu.h
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_OUT)$(EXE)

# Clean target
clean:
	$(RM) $(OUT)$(EXE) $(TEST_OUT)$(EXE)

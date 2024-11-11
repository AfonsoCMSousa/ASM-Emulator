# Compiler settings
CC = g++  # Default to g++
CFLAGS = -std=c++17 -municode
LDFLAGS = -lncurses

# Output executable name
OUT = ASM

# Source files
SRC = ../src/ui.cpp #../src/panels/window.cpp ../src/files/rw.cpp

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
        CFLAGS += -I/usr/local/include
        LDFLAGS += -L/usr/local/lib -lncurses -static-libgcc -static-libstdc++
        CC = clang++
    endif
endif

# All target
all: $(OUT)$(EXE)

# Build target
$(OUT)$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)$(EXE) $(LDFLAGS)

# Clean target
clean:
	$(RM) $(OUT)$(EXE)
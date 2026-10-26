# ASM-Emulator

A small virtual CPU with its own assembly language, an assembler, and an ncurses debugger, written in C++17.

Programs are assembled into a fixed 4-byte instruction format and run on an emulated machine with 8-bit and 16-bit registers, 16 KiB of memory, and a 256-byte stack. The terminal UI shows memory, registers, the stack, and the source side by side, and lets you step through a program one instruction at a time.

This is a learning tool, not an emulator for real hardware: the instruction set is custom and intentionally small.

## Features

- Two-pass assembler with labels, comments, and decimal/hex/binary literals
- Assembly errors reported per line and highlighted in the editor panel
- Step-by-step (`F`) and continuous (`R`) execution
- Runtime faults instead of undefined behaviour: running past the end of the program, stack overflow/underflow, and invalid opcodes stop execution with a message
- Headless test suite for the assembler and CPU core

## Example

```asm
; Push 10..0 onto the stack
        load a8 10
loop:   push a8
        beqz a8 done
        subb a8 1
        jmp loop
done:   halt
```

## Instruction set

| Instruction | Operation |
|---|---|
| `load r imm` | `r = imm` |
| `addi rd [rs] imm` | `rd = rs + imm` (`rs` defaults to `rd`) |
| `subb rd [rs] imm` | `rd = rs - imm` |
| `addr rd rs rt` | `rd = rs + rt` |
| `subr rd rs rt` | `rd = rs - rt` |
| `jmp t` | jump to `t` |
| `beq r1 r2 t` | jump if `r1 == r2` |
| `bne r1 r2 t` | jump if `r1 != r2` |
| `bgt r1 r2 t` | jump if `r1 > r2` (`stl` is an alias) |
| `beqz r t` | jump if `r == 0` |
| `push r` / `pop r` | stack push / pop |
| `halt` | stop execution |
| `null` | no operation (`nop` is an alias) |

Registers: `a8` `b8` `c8` `d8` (8-bit) and `a16` `b16` (16-bit). `af` `bf` `cf` are reserved for floating point and not yet usable.

See [GUIDE.md](GUIDE.md) for the full syntax, operand ranges, fault list, and binary encoding.

## Building

Requires a C++17 compiler, GNU Make, and ncurses.

**Linux**

```sh
sudo apt install g++ make libncurses-dev   # or your distro's equivalent
cd bin
make
```

**macOS** (ncurses ships with the Xcode Command Line Tools)

```sh
xcode-select --install
cd bin
make
```

**Windows (MSYS2)**

Install [MSYS2](https://www.msys2.org) to `C:\msys64`, then in the **MINGW64** shell:

```sh
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-ncurses mingw-w64-x86_64-make
cd bin
mingw32-make
```

`compile.bat` runs the same build from PowerShell or `cmd` if `C:\msys64\mingw64\bin` is on your `PATH`.

## Usage

```sh
cd bin
./ASM
```

Enter a file name without the `.s` extension (for example `main` for `main.s`). Files are looked up in the current directory, then next to the executable.

| Key | Action |
|---|---|
| `F` | execute one instruction |
| `R` | run continuously (press `Q` to stop) |
| `Q` | quit |

## Tests

```sh
cd bin
make test
```

## Project layout

```
src/
  cpu.h              assembler and CPU core
  main.cpp           terminal UI and main loop
  panels/window.*    ncurses panel helpers
  files/rw.*         source file loading
tests/
  cpu_test.cpp       assembler and CPU tests
bin/
  makefile
  main.s             example program
```

## Roadmap

- Memory load/store instructions
- `call` / `ret`
- Floating-point instructions
- Current source line highlighting and breakpoints in the debugger

## License

MIT

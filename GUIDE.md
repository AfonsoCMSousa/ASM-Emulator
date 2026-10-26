# ASM Emulator Guide

**Welcome to the ASM Emulator!**  
This is a simple and fun project designed to give you an introduction to assembly-like programming. Unlike real assembly language, this custom emulator gives you a controlled environment to explore without interacting directly with your CPU.

> **Disclaimer**: This is a custom set of instructions (opcodes) created for learning purposes, and it doesn't interact with real hardware.

## Table of Contents

- [How to Get Started](#how-to-get-started)
- [Syntax](#syntax)
- [Registers](#registers)
- [Available Commands (Opcodes)](#available-commands-opcodes)
- [Example Program](#example-program)
- [How to Run](#how-to-run)
- [Errors and Faults](#errors-and-faults)
- [Instruction Encoding](#instruction-encoding)

## How to Get Started

1. **Create a `.s` file**: Write your assembly-like program using the opcodes listed below.
2. **Load the program**: Run the emulator and type the file name without the `.s` extension. The file is looked up in the current directory first, then next to the executable.

---

## Syntax

```asm
; This is a comment. Everything after ';' or '#' is ignored.

start:              ; a label on its own line
loop:   addi a8 1   ; a label followed by an instruction
        bne a8, b8, loop   ; commas between operands are optional
```

- **One instruction per line.** Blank lines are fine.
- **Labels** are `name:` at the start of a line. Names use letters, digits and `_`, can't start with a digit, and can't be a register or instruction name.
- **Numbers** can be decimal (`42`), hex (`0x2A`), binary (`0b101010`) or negative (`-1`).
- **Jump/branch targets** can be a label (`loop` or `[loop]`) or a byte address in memory (`0x08` or `[0x08]`). Every instruction is 4 bytes, so addresses must be multiples of 4: the first instruction is at `0x00`, the second at `0x04`, and so on. Labels are easier!

---

## Registers

| Register | Size | Notes |
|---|---|---|
| `a8` `b8` `c8` `d8` | 8-bit | values 0–255, wrap around on overflow |
| `a16` `b16` | 16-bit | values 0–65535, wrap around on overflow |
| `af` `bf` `cf` | float | reserved: no instruction can use them yet |

Integer registers of different sizes can be mixed in one instruction. The result is cut down to the size of the destination register (for example `addr a8 a16 b8` keeps only the low 8 bits).

---

## Available Commands (Opcodes)

### **load**
- **Description**: Loads a value into a register.
- **Usage**: `load <reg> <value>`
- **Range**: −128 to 255 for 8-bit registers, −32768 to 65535 for 16-bit ones (negative values are stored as two's complement).
- **Example**: `load a16 0x1234`

### **addi**
- **Description**: Adds an immediate value (0–255).
- **Usage**: `addi <rd> <rs> <value>` sets `rd = rs + value`, or the short form `addi <rd> <value>` sets `rd = rd + value`.
- **Example**: `addi a8 a8 5` or `addi a8 5`

### **subb**
- **Description**: Subtracts an immediate value (0–255).
- **Usage**: `subb <rd> <rs> <value>` or `subb <rd> <value>`
- **Example**: `subb b16 3`

### **addr** / **subr**
- **Description**: Adds / subtracts two registers.
- **Usage**: `addr <rd> <rs> <rt>` sets `rd = rs + rt`; `subr <rd> <rs> <rt>` sets `rd = rs - rt`.
- **Example**: `addr c8 a8 b8`

### **jmp**
- **Description**: Unconditional jump.
- **Usage**: `jmp <target>`
- **Example**: `jmp loop`

### **beq** / **bne**
- **Description**: Branches if two registers are equal / not equal.
- **Usage**: `beq <r1> <r2> <target>`
- **Limit**: the target must be one of the first 256 instructions (address `0x3FC` or lower). Use `beqz` or `jmp` to go further.
- **Example**: `bne a8 b8 loop`

### **bgt** (also `stl`)
- **Description**: Branches if `r1` is **strictly greater** than `r2`. `stl` is the old name and still works.
- **Usage**: `bgt <r1> <r2> <target>`
- **Limit**: same as `beq`.
- **Example**: `bgt b8 a8 loop` loops while `b8 > a8`.

### **beqz**
- **Description**: Branches if a register is zero.
- **Usage**: `beqz <reg> <target>`
- **Example**: `beqz c8 done`

### **push** / **pop**
- **Description**: Pushes a register onto the stack / pops the top of the stack into a register. 8-bit registers use 1 stack byte, 16-bit registers use 2. The stack is 256 bytes.
- **Usage**: `push <reg>`, `pop <reg>`
- **Example**: `push a16` then `pop b16`

### **halt**
- **Description**: Stops the program. Every program needs one, otherwise it faults when it runs off the end.
- **Usage**: `halt`

### **null** (also `nop`)
- **Description**: Does nothing.
- **Usage**: `null`

---

## Example Program

Counts `a8` from 10 down to 0, pushing every value onto the stack:

```asm
; countdown
        load a8 10
loop:   push a8
        beqz a8 done     ; stop once we pushed 0
        subb a8 1
        jmp loop
done:   halt
```

---

## How to Run

Build with `make` inside `bin/` (or `compile.bat` on Windows), then run `./ASM` and type the file name without `.s` (for example `main`).

- **[F]** runs one instruction at a time.
- **[R]** runs automatically, speeding up as it goes. Press **[Q]** to stop it, which helps with programs that loop forever.
- **[Q]** quits.

`make test` builds and runs the headless assembler/CPU tests.

---

## Errors and Faults

**Assembly errors** are found before anything runs. Every bad line is shown in red in the editor panel with the reason, for example:

```
  2: bogus b8      <-- unknown instruction 'bogus'
  3: jmp nowhere   <-- unknown label 'nowhere'
  4: load a8 300   <-- value 300 is out of range (-128..255)
```

**Faults** happen while running and stop the program with a message in the info panel:

| Fault | Cause |
|---|---|
| `ran past the end of the program (missing halt?)` | execution reached the end without a `halt`, or jumped past it |
| `stack overflow` | `push` with the 256-byte stack full |
| `stack underflow` | `pop` with nothing on the stack |
| `unknown opcode` | jumped into something that isn't an instruction |

---

## Instruction Encoding

Every instruction is 4 bytes: an opcode followed by 3 operand bytes. This is what you see in the memory panel.

| Instruction | Opcode | Bytes |
|---|---|---|
| `addi rd rs imm` | `0x09` | `op rd rs imm` |
| `subb rd rs imm` | `0x0A` | `op rd rs imm` |
| `load r imm` | `0x0B` | `op r imm_lo imm_hi` |
| `jmp t` | `0x0C` | `op t_lo t_hi 0` |
| `push r` | `0x0D` | `op r 0 0` |
| `pop r` | `0x0E` | `op r 0 0` |
| `beq r1 r2 t` | `0x0F` | `op r1 r2 t` |
| `bne r1 r2 t` | `0x10` | `op r1 r2 t` |
| `beqz r t` | `0x11` | `op r t_lo t_hi` |
| `bgt r1 r2 t` | `0x12` | `op r1 r2 t` |
| `addr rd rs rt` | `0x13` | `op rd rs rt` |
| `subr rd rs rt` | `0x14` | `op rd rs rt` |
| `halt` | `0xF0` | `op 0 0 0` |
| `null` | `0xFF` | `op 0 0 0` |

Register ids: `a8`=0, `b8`=1, `c8`=2, `d8`=3, `a16`=4, `b16`=5, `af`=6, `bf`=7, `cf`=8.  
Jump targets are stored as instruction numbers (byte address ÷ 4), little-endian.

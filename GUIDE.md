# ASM Emulator Guide

**Welcome to the ASM Emulator!**  
This is a simple and fun project designed to give you an introduction to assembly-like programming. Unlike real assembly language, this custom emulator gives you a controlled environment to explore without interacting directly with your CPU.

> **Disclaimer**: This is a custom set of instructions (opcodes) created for learning purposes, and it doesn't interact with real hardware.

## Table of Contents

- [How to Get Started](#how-to-get-started)
- [Available Commands (Opcodes)](#available-commands-opcodes)
- [Registers](#registers)
- [Example Program](#example-program)
- [How to Run](#how-to-run)
- [Troubleshooting](#troubleshooting)

## How to Get Started

1. **Create a `.s` file**: Write your assembly-like program using the opcodes listed below.
2. **Load the program**: Run the program using the emulator, which will read and execute your `.s` file.

---

## Available Commands (Opcodes)

Here is a list of available commands you can use in your ASM program. Each command corresponds to a specific operation.

### **1. load**
- **Description**: Loads an immediate value into a register.
- **Usage**:  
  `load <register> <value>`  
  Example: `load a8 13` — Loads the value `13` into the 8-bit register `a8`.

---

### **2. addi**
- **Description**: Adds an immediate value to a register.
- **Usage**:  
  `addi <register> <value>`  
  Example: `addi a8 5` — Adds `5` to the value in the 8-bit register `a8`.

---

### **3. subb**
- **Description**: Subtracts an immediate value from a register.
- **Usage**:  
  `subb <register> <value>`  
  Example: `subb b16 3` — Subtracts `3` from the value in the 16-bit register `b16`.

---

### **4. jmp**
- **Description**: Unconditional jump to a specific memory address. Execution continues from the specified address.
- **Usage**:  
  `jmp [address]`  
  Example: `jmp [0x0040]` — Jumps to memory location `0x0040`.

---

### **5. push**
- **Description**: Pushes the value of a register onto the stack.
- **Usage**:  
  `push <register>`  
  Example: `push a8` — Pushes the value in the 8-bit register `a8` onto the stack.

---

### **6. pop**
- **Description**: Pops the top value from the stack into a register.
- **Usage**:  
  `pop <register>`  
  Example: `pop b16` — Pops the top value from the stack into the 16-bit register `b16`.

---

### **7. beq**
- **Description**: Branches to the specified address if two registers are equal.
- **Usage**:  
  `beq <reg1> <reg2> [address]`  
  Example: `beq a8 b8 [0x0100]` — If `a8` equals `b8`, jumps to address `0x0100`.

---

### **8. bne**
- **Description**: Branches to the specified address if two registers are not equal.
- **Usage**:  
  `bne <reg1> <reg2> [address]`  
  Example: `bne a16 b16 [0x0100]` — If `a16` does not equal `b16`, jumps to address `0x0100`.

---

### **9. beqz**
- **Description**: Branches to the specified address if the register value is zero.
- **Usage**:  
  `beqz <register> [address]`  
  Example: `beqz c8 [0x0080]` — If `c8` is `0`, jumps to address `0x0080`.

---

### **10. stl**
- **Description**: Stores the value from a register into a memory location.
- **Usage**:  
  `stl <register> [address]`  
  Example: `stl a8 [0x2000]` — Stores the value from `a8` into memory location `0x2000`.

---

### **11. halt**
- **Description**: Halts program execution.
- **Usage**:  
  `halt`  
  Example: `halt` — Stops the program execution.

---

### **12. null**
- **Description**: No operation (NOP). It does nothing and is used as a placeholder.
- **Usage**:  
  `null`  
  Example: `null` — Skips this instruction.

---

## Registers

The emulator supports the following registers. Each register is associated with a specific bit width or type:

- **a8**: 8-bit register `a`
- **b8**: 8-bit register `b`
- **c8**: 8-bit register `c`
- **d8**: 8-bit register `d`

- **a16**: 16-bit register `a`
- **b16**: 16-bit register `b`
- **c16**: 16-bit register `c`
- **d16**: 16-bit register `d`

- **af**: Floating-point register `a`
- **bf**: Floating-point register `b`

---

## Example Program

Here’s an example of a simple ASM program:

```asm

start:
load a8 10       ; Load value 10 into register a8
load b8 5        ; Load value 5 into register b8
addi a8 5        ; Add 5 to the value in register a8 (a8 becomes 15)
beq a8 b8 [end]  ; If a8 equals b8, jump to "end"
jmp [start]      ; Jump back to start (infinite loop)
end:
halt             ; Stop program execution

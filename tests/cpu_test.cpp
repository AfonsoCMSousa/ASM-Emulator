// Headless tests for the assembler and CPU core. Build and run with `make test` in bin/.
#include "../src/cpu.h"

#include <iostream>

static int failures = 0;

#define CHECK(cond)                                                              \
    do                                                                           \
    {                                                                            \
        if (!(cond))                                                             \
        {                                                                        \
            std::cerr << __FILE__ << ":" << __LINE__ << ": FAILED: " #cond "\n"; \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

static std::vector<std::string> src(const std::string &text)
{
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
        lines.push_back(line);
    return lines;
}

static bool assembles(CPU &cpu, const std::string &text)
{
    std::vector<AsmError> errs;
    bool ok = cpu.assemble(src(text), errs);
    for (auto &e : errs)
        std::cerr << "    line " << e.line << ": " << e.message << "\n";
    return ok;
}

// Expects assembly to fail with an error on `line` whose message contains `needle`.
static bool failsWith(const std::string &text, int line, const std::string &needle)
{
    CPU cpu;
    std::vector<AsmError> errs;
    if (cpu.assemble(src(text), errs))
        return false;
    for (auto &e : errs)
        if (e.line == line && e.message.find(needle) != std::string::npos)
            return true;
    for (auto &e : errs)
        std::cerr << "    got line " << e.line << ": " << e.message << "\n";
    return false;
}

int main()
{
    // Registers are distinct (a16/af used to alias a8).
    {
        CPU cpu;
        CHECK(assembles(cpu, "load a8 1\nload a16 1000\nload b16 0xBEEF\nhalt"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(0) == 1);
        CHECK(cpu.getREG16(0) == 1000);
        CHECK(cpu.getREG16(1) == 0xBEEF);
    }

    // Labels, comments, blank lines, both addi forms, bracketed targets.
    {
        CPU cpu;
        CHECK(assembles(cpu,
                        "; count a8 up to 10\n"
                        "\n"
                        "        load b8, 10   # commas and both comment styles\n"
                        "loop:   addi a8 1\n"
                        "        addi c8 c8 2\n"
                        "        bne a8 b8 [loop]\n"
                        "end:    halt\n"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(0) == 10);
        CHECK(cpu.getREG8(2) == 20);
        CHECK(cpu.getPC() == 4 * 4); // PC stays on the halt
    }

    // Numeric targets are byte addresses; forward references work.
    {
        CPU cpu;
        CHECK(assembles(cpu, "jmp 0x8\nload a8 99\nload a8 7\nbeqz d8 skip\nload a8 1\nskip: halt"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(0) == 7);
    }

    // bgt / stl branch on strictly greater.
    {
        CPU cpu;
        CHECK(assembles(cpu, "load a8 5\nload b8 5\nbgt a8 b8 no\nstl a8 c8 yes\nno: halt\nyes: load d8 1\nhalt"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(3) == 1);
    }

    // Arithmetic wraps to register width; mixed widths truncate to destination.
    {
        CPU cpu;
        CHECK(assembles(cpu,
                        "load a8 250\naddi a8 10\n"
                        "load a16 300\naddr b8 a16 a8\n"
                        "load b16 0\nsubb b16 1\n"
                        "load c8 -1\nhalt"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(0) == 4);
        CHECK(cpu.getREG8(1) == ((300 + 4) & 0xFF));
        CHECK(cpu.getREG16(1) == 0xFFFF);
        CHECK(cpu.getREG8(2) == 0xFF);
    }

    // Stack: 8- and 16-bit push/pop round trip.
    {
        CPU cpu;
        CHECK(assembles(cpu, "load a16 0x1234\nload a8 7\npush a16\npush a8\npop b8\npop b16\nhalt"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(1) == 7);
        CHECK(cpu.getREG16(1) == 0x1234);
        CHECK(cpu.getSP() == 0);
    }

    // The original sample program (updated to use a label).
    {
        CPU cpu;
        CHECK(assembles(cpu, "load b8 30\nloop: addi a8 a8 1\npush a8\nbgt b8 a8 loop\nhalt"));
        CHECK(cpu.run() == CPU_HALTED);
        CHECK(cpu.getREG8(0) == 30);
        CHECK(cpu.getSP() == 30);
    }

    // ---- faults: none of these may loop forever or touch memory out of bounds
    {
        CPU cpu;
        CHECK(assembles(cpu, "load a8 1"));
        CHECK(cpu.run(1000) == CPU_FAULT);
        CHECK(cpu.getFault().find("missing halt") != std::string::npos);
        CHECK(cpu.execute() == CPU_FAULT); // sticky
    }
    {
        CPU cpu;
        CHECK(assembles(cpu, "pop a8\nhalt"));
        CHECK(cpu.run(1000) == CPU_FAULT);
        CHECK(cpu.getFault().find("underflow") != std::string::npos);
        CHECK(cpu.getSP() == 0);
    }
    {
        CPU cpu;
        CHECK(assembles(cpu, "loop: push a8\njmp loop"));
        CHECK(cpu.run(10000) == CPU_FAULT);
        CHECK(cpu.getFault().find("overflow") != std::string::npos);
        CHECK(cpu.getSP() == CPU::STACK_SIZE);
    }
    {
        CPU cpu;
        CHECK(assembles(cpu, "load a16 1\nloop: push a16\njmp loop"));
        CHECK(cpu.run(10000) == CPU_FAULT);
        CHECK(cpu.getSP() == CPU::STACK_SIZE);
    }
    {
        CPU cpu; // jump into the zeroed memory after the program
        CHECK(assembles(cpu, "jmp 0x100\nhalt"));
        CHECK(cpu.run(1000) == CPU_FAULT);
    }
    {
        CPU cpu; // infinite loop is legal; step limit stops it
        CHECK(assembles(cpu, "loop: jmp loop"));
        CHECK(cpu.run(500) == CPU_RUNNING);
    }

    // ---- assembler errors, with line numbers
    CHECK(failsWith("load a8 1\n\nfoo a8", 3, "unknown instruction"));
    CHECK(failsWith("load q8 1\nhalt", 1, "not a register"));
    CHECK(failsWith("load af 1\nhalt", 1, "float"));
    CHECK(failsWith("load a8 256\nhalt", 1, "out of range"));
    CHECK(failsWith("load a8 0xZZ\nhalt", 1, "not a number"));
    CHECK(failsWith("addi a8\nhalt", 1, "operand"));
    CHECK(failsWith("addi a8 -1\nhalt", 1, "out of range"));
    CHECK(failsWith("halt a8", 1, "operand"));
    CHECK(failsWith("jmp nowhere\nhalt", 1, "unknown label"));
    CHECK(failsWith("jmp 2\nhalt", 1, "multiple of 4"));
    CHECK(failsWith("jmp [0x0040", 1, "unknown label"));
    CHECK(failsWith("x: halt\nx: halt", 2, "defined twice"));
    CHECK(failsWith("a8: halt", 1, "clashes"));
    CHECK(failsWith("beq a8 b8 0x400\nhalt", 1, "too far"));
    CHECK(failsWith("; nothing here\n\n", 1, "no instructions"));
    CHECK(failsWith("", 1, "no instructions"));

    // All errors are reported at once, in line order.
    {
        CPU cpu;
        std::vector<AsmError> errs;
        CHECK(!cpu.assemble(src("load a8 1\nbogus b8\njmp nowhere\nload af 2\nhalt"), errs));
        CHECK(errs.size() == 3);
        CHECK(errs.size() == 3 && errs[0].line == 2 && errs[1].line == 3 && errs[2].line == 4);
    }

    // A failed assemble leaves nothing loaded.
    {
        CPU cpu;
        std::vector<AsmError> errs;
        CHECK(!cpu.assemble(src("load a8 1\nbogus"), errs));
        CHECK(cpu.getSC() == 0);
        CHECK(cpu.execute() == CPU_FAULT);
    }

    if (failures == 0)
        std::cout << "All tests passed.\n";
    else
        std::cout << failures << " check(s) failed.\n";
    return failures == 0 ? 0 : 1;
}

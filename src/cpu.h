#pragma once
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Every instruction is exactly 4 bytes: [opcode, b1, b2, b3].
//
//   load  r imm          [op, r, imm_lo, imm_hi]   imm fits the register width
//   addi  rd rs imm8     [op, rd, rs, imm8]        (also: addi rd imm8 -> rd += imm8)
//   subb  rd rs imm8     [op, rd, rs, imm8]        (also: subb rd imm8 -> rd -= imm8)
//   addr  rd rs rt       [op, rd, rs, rt]
//   subr  rd rs rt       [op, rd, rs, rt]
//   jmp   target         [op, idx_lo, idx_hi, 0]
//   push  r / pop r      [op, r, 0, 0]             16-bit registers use 2 stack bytes
//   beq/bne/bgt r1 r2 t  [op, r1, r2, idx8]        target must be < instruction 256
//   beqz  r target       [op, r, idx_lo, idx_hi]
//   halt / null          [op, 0, 0, 0]
//
// Branch targets are stored as instruction indices (byte address / 4).
enum opcode : uint8_t
{
    OPCODE_ADDI = 0x09,
    OPCODE_SUBB = 0x0A,
    OPCODE_LOAD = 0x0B,
    OPCODE_JMP = 0x0C,
    OPCODE_PUSH = 0x0D,
    OPCODE_POP = 0x0E,

    OPCODE_BEQ = 0x0F,  // Branch if r1 == r2
    OPCODE_BNE = 0x10,  // Branch if r1 != r2
    OPCODE_BEQZ = 0x11, // Branch if r == 0
    OPCODE_STL = 0x12,  // Branch if r1 > r2 (strictly greater); mnemonics "stl" and "bgt"

    OPCODE_ADDR = 0x13,
    OPCODE_SUBR = 0x14,

    OPCODE_HALT = 0xF0,
    OPCODE_NULL = 0xFF, // No operation
};

// Register ids as encoded in instruction operands.
enum reg_id : uint8_t
{
    REG_A8 = 0,
    REG_B8 = 1,
    REG_C8 = 2,
    REG_D8 = 3,
    REG_A16 = 4,
    REG_B16 = 5,
    REG_AF = 6,
    REG_BF = 7,
    REG_CF = 8,
    REG_COUNT = 9,
};

enum cpu_status
{
    CPU_FAULT = -1,
    CPU_RUNNING = 0,
    CPU_HALTED = 1,
};

struct AsmError
{
    int line; // 1-based source line
    std::string message;
};

class CPU
{
public:
    static const int MEM_SIZE = 16384;
    static const int STACK_SIZE = 256;
    static const int INSTR_SIZE = 4;

private:
    uint8_t reg8[4] = {0};
    uint16_t reg16[2] = {0};
    float regf[3] = {0};

    int pc = 0;       // byte address of the next instruction
    uint16_t sc = 0;  // size of the assembled program in bytes
    uint16_t sp = 0;  // next free stack byte

    cpu_status status = CPU_RUNNING;
    std::string fault;

    uint8_t mem[MEM_SIZE] = {0};
    uint8_t stack[STACK_SIZE] = {0};

    // ---------------------------------------------------------------- registers

    static bool isIntReg(int r) { return r >= REG_A8 && r <= REG_B16; }
    static bool is16(int r) { return r == REG_A16 || r == REG_B16; }

    uint16_t readInt(int r) const
    {
        return r < REG_A16 ? reg8[r] : reg16[r - REG_A16];
    }

    void writeInt(int r, uint32_t v)
    {
        if (r < REG_A16)
            reg8[r] = static_cast<uint8_t>(v);
        else
            reg16[r - REG_A16] = static_cast<uint16_t>(v);
    }

    static const std::unordered_map<std::string, int> &registerTable()
    {
        static const std::unordered_map<std::string, int> table = {
            {"a8", REG_A8}, {"b8", REG_B8}, {"c8", REG_C8}, {"d8", REG_D8},
            {"a16", REG_A16}, {"b16", REG_B16},
            {"af", REG_AF}, {"bf", REG_BF}, {"cf", REG_CF},
        };
        return table;
    }

    static const std::unordered_map<std::string, opcode> &mnemonicTable()
    {
        static const std::unordered_map<std::string, opcode> table = {
            {"addi", OPCODE_ADDI}, {"subb", OPCODE_SUBB}, {"load", OPCODE_LOAD},
            {"jmp", OPCODE_JMP}, {"push", OPCODE_PUSH}, {"pop", OPCODE_POP},
            {"beq", OPCODE_BEQ}, {"bne", OPCODE_BNE}, {"beqz", OPCODE_BEQZ},
            {"stl", OPCODE_STL}, {"bgt", OPCODE_STL},
            {"addr", OPCODE_ADDR}, {"subr", OPCODE_SUBR},
            {"halt", OPCODE_HALT}, {"null", OPCODE_NULL}, {"nop", OPCODE_NULL},
        };
        return table;
    }

    // ---------------------------------------------------------------- assembler helpers

    static std::string lower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    static bool isIdentifier(const std::string &s)
    {
        if (s.empty() || !(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_'))
            return false;
        for (char c : s)
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_'))
                return false;
        return true;
    }

    // Decimal, 0x hex or 0b binary, with optional leading '-'.
    static bool parseNumber(const std::string &tok, long &out)
    {
        if (tok.empty())
            return false;
        std::string s = tok;
        bool neg = false;
        if (s[0] == '-' || s[0] == '+')
        {
            neg = s[0] == '-';
            s = s.substr(1);
        }
        int base = 10;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
            base = 16;
            s = s.substr(2);
        }
        else if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B'))
        {
            base = 2;
            s = s.substr(2);
        }
        if (s.empty())
            return false;
        errno = 0;
        char *end = nullptr;
        long v = std::strtol(s.c_str(), &end, base);
        if (errno != 0 || *end != '\0')
            return false;
        out = neg ? -v : v;
        return true;
    }

    // Splits a line into tokens, dropping comments (';' or '#') and commas.
    static std::vector<std::string> tokenize(const std::string &line)
    {
        std::string code = line.substr(0, line.find_first_of(";#"));
        for (char &c : code)
            if (c == ',' || c == '\r' || c == '\t')
                c = ' ';
        std::vector<std::string> out;
        std::stringstream ss(code);
        std::string tok;
        while (ss >> tok)
            out.push_back(tok);
        return out;
    }

    struct Pending
    {
        int line;
        std::vector<std::string> tokens; // tokens[0] is the mnemonic (lower-case)
    };

    // Encodes one instruction into out[4]. Returns an empty string on success.
    static std::string encode(const Pending &p,
                              const std::unordered_map<std::string, int> &labels,
                              uint8_t out[4])
    {
        const std::string &mn = p.tokens[0];
        const opcode op = mnemonicTable().at(mn);
        const size_t argc = p.tokens.size() - 1;
        out[0] = op;
        out[1] = out[2] = out[3] = 0;

        auto arg = [&](size_t i) { return p.tokens[i + 1]; };

        auto wantArgs = [&](size_t n, const char *usage) -> std::string {
            if (argc != n)
                return std::string("'") + mn + "' takes " + std::to_string(n) +
                       " operand(s), got " + std::to_string(argc) + " (usage: " + usage + ")";
            return "";
        };

        // Integer register operand.
        auto reg = [&](size_t i, int &r) -> std::string {
            auto it = registerTable().find(lower(arg(i)));
            if (it == registerTable().end())
                return "'" + arg(i) + "' is not a register";
            if (!isIntReg(it->second))
                return "'" + arg(i) + "': float registers aren't supported by '" + mn + "' yet";
            r = it->second;
            return "";
        };

        // Branch target: label, [label], byte address or [address]. Yields an instruction index.
        auto target = [&](size_t i, long maxIndex, long &idx) -> std::string {
            std::string t = arg(i);
            if (t.size() >= 2 && t.front() == '[' && t.back() == ']')
                t = t.substr(1, t.size() - 2);
            long addr;
            if (parseNumber(t, addr))
            {
                if (addr < 0 || addr >= MEM_SIZE)
                    return "address " + t + " is outside memory";
                if (addr % INSTR_SIZE != 0)
                    return "address " + t + " is not a multiple of 4 (instructions are 4 bytes)";
                idx = addr / INSTR_SIZE;
            }
            else
            {
                auto it = labels.find(t);
                if (it == labels.end())
                    return "unknown label '" + t + "'";
                idx = it->second;
            }
            if (idx > maxIndex)
                return "target '" + arg(i) + "' is too far for '" + mn +
                       "' (max address " + std::to_string(maxIndex * INSTR_SIZE) +
                       "); use beqz or jmp";
            return "";
        };

        auto imm = [&](size_t i, long lo, long hi, long &v) -> std::string {
            if (!parseNumber(arg(i), v))
                return "'" + arg(i) + "' is not a number";
            if (v < lo || v > hi)
                return "value " + arg(i) + " is out of range (" + std::to_string(lo) +
                       ".." + std::to_string(hi) + ")";
            return "";
        };

        std::string e;
        int r1 = 0, r2 = 0, r3 = 0;
        long v = 0;

        switch (op)
        {
        case OPCODE_LOAD:
            if (!(e = wantArgs(2, "load reg value")).empty() || !(e = reg(0, r1)).empty())
                return e;
            if (is16(r1))
                e = imm(1, -32768, 65535, v);
            else
                e = imm(1, -128, 255, v);
            if (!e.empty())
                return e;
            out[1] = r1;
            out[2] = static_cast<uint8_t>(v & 0xFF);
            out[3] = static_cast<uint8_t>((v >> 8) & 0xFF);
            return "";

        case OPCODE_ADDI:
        case OPCODE_SUBB:
            if (argc == 2) // short form: addi rd imm
            {
                if (!(e = reg(0, r1)).empty() || !(e = imm(1, 0, 255, v)).empty())
                    return e;
                r2 = r1;
            }
            else
            {
                if (!(e = wantArgs(3, "addi rd rs value  or  addi rd value")).empty() ||
                    !(e = reg(0, r1)).empty() || !(e = reg(1, r2)).empty() ||
                    !(e = imm(2, 0, 255, v)).empty())
                    return e;
            }
            out[1] = r1;
            out[2] = r2;
            out[3] = static_cast<uint8_t>(v);
            return "";

        case OPCODE_ADDR:
        case OPCODE_SUBR:
            if (!(e = wantArgs(3, "addr rd rs rt")).empty() || !(e = reg(0, r1)).empty() ||
                !(e = reg(1, r2)).empty() || !(e = reg(2, r3)).empty())
                return e;
            out[1] = r1;
            out[2] = r2;
            out[3] = r3;
            return "";

        case OPCODE_JMP:
            if (!(e = wantArgs(1, "jmp target")).empty() ||
                !(e = target(0, MEM_SIZE / INSTR_SIZE - 1, v)).empty())
                return e;
            out[1] = static_cast<uint8_t>(v & 0xFF);
            out[2] = static_cast<uint8_t>(v >> 8);
            return "";

        case OPCODE_PUSH:
        case OPCODE_POP:
            if (!(e = wantArgs(1, "push reg")).empty() || !(e = reg(0, r1)).empty())
                return e;
            out[1] = r1;
            return "";

        case OPCODE_BEQ:
        case OPCODE_BNE:
        case OPCODE_STL:
            if (!(e = wantArgs(3, "beq r1 r2 target")).empty() || !(e = reg(0, r1)).empty() ||
                !(e = reg(1, r2)).empty() || !(e = target(2, 255, v)).empty())
                return e;
            out[1] = r1;
            out[2] = r2;
            out[3] = static_cast<uint8_t>(v);
            return "";

        case OPCODE_BEQZ:
            if (!(e = wantArgs(2, "beqz reg target")).empty() || !(e = reg(0, r1)).empty() ||
                !(e = target(1, MEM_SIZE / INSTR_SIZE - 1, v)).empty())
                return e;
            out[1] = r1;
            out[2] = static_cast<uint8_t>(v & 0xFF);
            out[3] = static_cast<uint8_t>(v >> 8);
            return "";

        case OPCODE_HALT:
        case OPCODE_NULL:
            return wantArgs(0, mn.c_str());
        }
        return "internal error: unhandled opcode";
    }

    std::string faultAt(const std::string &msg)
    {
        std::ostringstream ss;
        ss << msg << " at 0x" << std::hex << std::setw(4) << std::setfill('0') << pc;
        fault = ss.str();
        status = CPU_FAULT;
        return fault;
    }

public:
    int getMemSize() const { return sizeof(mem); }

    // Clears registers, memory and stack.
    void reset()
    {
        std::memset(reg8, 0, sizeof(reg8));
        std::memset(reg16, 0, sizeof(reg16));
        std::memset(regf, 0, sizeof(regf));
        std::memset(mem, 0, sizeof(mem));
        std::memset(stack, 0, sizeof(stack));
        pc = 0;
        sc = 0;
        sp = 0;
        status = CPU_RUNNING;
        fault.clear();
    }

    // Two-pass assembler. Pass 1 collects labels and instruction positions,
    // pass 2 encodes. On any error nothing is loaded and `errors` is filled.
    bool assemble(const std::vector<std::string> &lines, std::vector<AsmError> &errors)
    {
        reset();
        errors.clear();

        std::unordered_map<std::string, int> labels;
        std::vector<Pending> program;

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const int lineNo = static_cast<int>(i) + 1;
            std::vector<std::string> toks = tokenize(lines[i]);

            // Leading "label:" (possibly several, possibly followed by an instruction)
            while (!toks.empty() && toks[0].size() > 1 && toks[0].back() == ':')
            {
                std::string name = toks[0].substr(0, toks[0].size() - 1);
                if (!isIdentifier(name))
                    errors.push_back({lineNo, "invalid label name '" + name + "'"});
                else if (registerTable().count(lower(name)) || mnemonicTable().count(lower(name)))
                    errors.push_back({lineNo, "label '" + name + "' clashes with a register or instruction name"});
                else if (labels.count(name))
                    errors.push_back({lineNo, "label '" + name + "' is defined twice"});
                else
                    labels[name] = static_cast<int>(program.size());
                toks.erase(toks.begin());
            }

            if (toks.empty())
                continue;

            toks[0] = lower(toks[0]);
            if (!mnemonicTable().count(toks[0]))
            {
                errors.push_back({lineNo, "unknown instruction '" + toks[0] + "'"});
                continue;
            }
            program.push_back({lineNo, toks});
        }

        if (program.empty() && errors.empty())
            errors.push_back({1, "program has no instructions"});
        if (program.size() > static_cast<size_t>(MEM_SIZE / INSTR_SIZE))
        {
            errors.push_back({program[MEM_SIZE / INSTR_SIZE].line, "program does not fit in memory"});
            return false;
        }

        // Encode everything, even after pass-1 errors, so all problems are reported at once.
        std::vector<uint8_t> bytes(program.size() * INSTR_SIZE);
        for (size_t i = 0; i < program.size(); ++i)
        {
            std::string e = encode(program[i], labels, &bytes[i * INSTR_SIZE]);
            if (!e.empty())
                errors.push_back({program[i].line, e});
        }
        if (!errors.empty())
        {
            std::stable_sort(errors.begin(), errors.end(),
                             [](const AsmError &a, const AsmError &b) { return a.line < b.line; });
            return false;
        }

        std::memcpy(mem, bytes.data(), bytes.size());
        sc = static_cast<uint16_t>(bytes.size());
        return true;
    }

    // Executes one instruction. Returns CPU_RUNNING, CPU_HALTED or CPU_FAULT;
    // once halted or faulted, further calls are no-ops returning the same status.
    int execute()
    {
        if (status != CPU_RUNNING)
            return status;

        if (pc < 0 || pc % INSTR_SIZE != 0 || pc + INSTR_SIZE > MEM_SIZE)
        {
            faultAt("PC out of range");
            return status;
        }
        if (pc >= sc)
        {
            faultAt("ran past the end of the program (missing halt?)");
            return status;
        }

        const uint8_t op = mem[pc];
        const uint8_t b1 = mem[pc + 1], b2 = mem[pc + 2], b3 = mem[pc + 3];
        const int here = pc;
        pc += INSTR_SIZE;

        auto badReg = [&](uint8_t r) {
            if (isIntReg(r))
                return false;
            pc = here;
            faultAt(r < REG_COUNT ? "float register used in integer instruction"
                                  : "invalid register id " + std::to_string(r));
            return true;
        };
        auto jumpTo = [&](int index) { pc = index * INSTR_SIZE; };

        switch (op)
        {
        case OPCODE_LOAD:
            if (badReg(b1))
                break;
            writeInt(b1, b2 | (b3 << 8));
            break;

        case OPCODE_ADDI:
            if (badReg(b1) || badReg(b2))
                break;
            writeInt(b1, readInt(b2) + b3);
            break;

        case OPCODE_SUBB:
            if (badReg(b1) || badReg(b2))
                break;
            writeInt(b1, readInt(b2) - b3);
            break;

        case OPCODE_ADDR:
            if (badReg(b1) || badReg(b2) || badReg(b3))
                break;
            writeInt(b1, readInt(b2) + readInt(b3));
            break;

        case OPCODE_SUBR:
            if (badReg(b1) || badReg(b2) || badReg(b3))
                break;
            writeInt(b1, readInt(b2) - readInt(b3));
            break;

        case OPCODE_JMP:
            jumpTo(b1 | (b2 << 8));
            break;

        case OPCODE_PUSH:
        {
            if (badReg(b1))
                break;
            const int n = is16(b1) ? 2 : 1;
            if (sp + n > STACK_SIZE)
            {
                pc = here;
                faultAt("stack overflow");
                break;
            }
            const uint16_t v = readInt(b1);
            stack[sp++] = static_cast<uint8_t>(v & 0xFF);
            if (n == 2)
                stack[sp++] = static_cast<uint8_t>(v >> 8);
            break;
        }

        case OPCODE_POP:
        {
            if (badReg(b1))
                break;
            const int n = is16(b1) ? 2 : 1;
            if (sp < n)
            {
                pc = here;
                faultAt("stack underflow");
                break;
            }
            uint16_t v = 0;
            if (n == 2)
                v = static_cast<uint16_t>(stack[--sp] << 8);
            v |= stack[--sp];
            writeInt(b1, v);
            break;
        }

        case OPCODE_BEQ:
            if (badReg(b1) || badReg(b2))
                break;
            if (readInt(b1) == readInt(b2))
                jumpTo(b3);
            break;

        case OPCODE_BNE:
            if (badReg(b1) || badReg(b2))
                break;
            if (readInt(b1) != readInt(b2))
                jumpTo(b3);
            break;

        case OPCODE_STL:
            if (badReg(b1) || badReg(b2))
                break;
            if (readInt(b1) > readInt(b2))
                jumpTo(b3);
            break;

        case OPCODE_BEQZ:
            if (badReg(b1))
                break;
            if (readInt(b1) == 0)
                jumpTo(b2 | (b3 << 8));
            break;

        case OPCODE_NULL:
            break;

        case OPCODE_HALT:
            pc = here; // keep PC on the halt so the UI highlights it
            status = CPU_HALTED;
            break;

        default:
        {
            pc = here;
            std::ostringstream ss;
            ss << "unknown opcode 0x" << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(op);
            faultAt(ss.str());
            break;
        }
        }

        return status;
    }

    // Runs until halt or fault. maxSteps = 0 means no limit.
    // Returns the final status (CPU_RUNNING if the step limit was hit).
    int run(long maxSteps = 0)
    {
        for (long n = 0; maxSteps == 0 || n < maxSteps; ++n)
            if (execute() != CPU_RUNNING)
                break;
        return status;
    }

    const char *dump(int line) const
    {
        static char buffer[50];
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << std::hex << line << ": ";
        for (int i = 0; i < 4; ++i)
        {
            if (line + i < MEM_SIZE)
                ss << "0x" << std::setw(2) << std::setfill('0') << std::hex
                   << static_cast<int>(mem[line + i]) << " ";
            else
                ss << "0x-- ";
        }
        std::snprintf(buffer, sizeof(buffer), "%s", ss.str().c_str());
        return buffer;
    }

    const char *getStack(int line) const
    {
        static char buffer[50];
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << std::hex << line << ": ";
        for (int i = 0; i < 4; ++i)
        {
            if (line + i < STACK_SIZE)
                ss << "0x" << std::setw(2) << std::setfill('0') << std::hex
                   << static_cast<int>(stack[line + i]) << " ";
            else
                ss << "0x-- ";
        }
        std::snprintf(buffer, sizeof(buffer), "%s", ss.str().c_str());
        return buffer;
    }

    int getREG8(int index) const { return (index >= 0 && index < 4) ? reg8[index] : 0; }
    int getREG16(int index) const { return (index >= 0 && index < 2) ? reg16[index] : 0; }
    float getREGF(int index) const { return (index >= 0 && index < 3) ? regf[index] : 0.0f; }

    uint16_t getPC() const { return static_cast<uint16_t>(pc); }
    uint16_t getSC() const { return sc; }
    uint16_t getSP() const { return sp; }
    int getStatus() const { return status; }
    const std::string &getFault() const { return fault; }
};

#pragma once

#include <cstdint>
#include <cstdio>
#include <ostream>

struct Instruction
{
    uint8_t instruction;
    bool prefix;
    uint8_t operand0;
    uint8_t operand1;
    uint8_t cycles;
    uint8_t cycles_if_taken;

    bool operator==(const Instruction& rhs) const {
        return instruction == rhs.instruction &&
               prefix == rhs.prefix &&
               operand0 == rhs.operand0 &&
               operand1 == rhs.operand1 &&
               cycles == rhs.cycles &&
               cycles_if_taken == rhs.cycles_if_taken;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Instruction& instr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Instr: %02X, prefix: %s, operand0: %02X, operand1: %02X, Cycles: %d, cycles if taken: %d",
             instr.instruction, instr.prefix ? "true" : "false", instr.operand0, instr.operand1, instr.cycles, instr.cycles_if_taken);
    os << buf << " ";
    return os;
}

class InstructionDecoder
{
public:
    InstructionDecoder();
    Instruction decode(const uint8_t* memory, uint16_t* pc);
    void decodeByte(uint8_t byte);
private:
    void decodeInstruction(uint8_t byte);
    void decodeOperand(uint8_t byte);
    void finishDecoding();

    uint8_t instruction_;
    uint8_t instruction_length_;
    uint8_t instruction_cycles_;
    uint8_t instruction_cycles_if_taken_;
    uint8_t operands_[2];
    uint8_t op_bytes_to_decode_;
    bool prefix_;
    bool decoding_finished_;
};

#include "instructiondecoder.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// Taken from http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
static uint8_t instruction_lengths[256] = {
    1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1, // 0x
    2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, // 1x
    2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, // 2x
    2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, // 3x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 8x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 9x
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // Ax
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // Bx
    1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1, // Cx
    1, 1, 3, 0, 3, 1, 2, 1, 1, 1, 3, 0, 3, 0, 2, 1, // Dx
    2, 1, 1, 0, 0, 1, 2, 1, 2, 1, 3, 0, 0, 0, 2, 1, // Ex // The website is wrong about the length of 0xE2
    2, 1, 1, 1, 0, 1, 2, 1, 2, 1, 3, 1, 0, 0, 2, 1  // Fx // The website is wrong about the length of 0xF2
};

static uint8_t instruction_lengths_prefix[256] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 1x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 2x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 3x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 4x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 5x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 6x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 7x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 8x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 9x
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // Ax
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // Bx
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // Cx
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // Dx
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // Ex
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2  // Fx
};

// This is always the lower number of cycles an instruction takes
static uint8_t instruction_cycles[256] = {
    4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4, // 0x
    4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4, // 1x
    8, 12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4, // 2x
    8, 12, 8, 8, 12, 12, 12, 4, 8, 8, 8, 8, 4, 4, 8, 4, // 3x
    4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 4x
    4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 5x
    4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 6x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 7x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 8x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 9x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // Ax
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // Bx
    8, 12, 12, 16, 12, 16, 8, 16, 8, 16, 12, 4, 12, 24, 8, 16, // Cx
    8, 12, 12, 0, 12, 16, 8, 16, 8, 16, 12, 0, 12, 0, 8, 16, // Dx
    12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 0, 0, 8, 16, // Ex
    12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 0, 0, 8, 16  // Fx
};

static uint8_t instruction_cycles_if_taken[256] = {
    4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4, // 0x
    4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4, // 1x
    12, 12, 8, 8, 4, 4, 8, 4, 8, 12, 8, 8, 4, 4, 8, 4, // 2x
    12, 12, 8, 8, 12, 12, 12, 4, 12, 8, 8, 8, 4, 4, 8, 4, // 3x
    4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 4x
    4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 5x
    4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 6x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 7x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 8x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 9x
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // Ax
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // Bx
    20, 12, 16, 16, 24, 16, 8, 16, 20, 16, 16, 4, 24, 24, 8, 16, // Cx
    20, 12, 16, 0, 24, 16, 8, 16, 20, 16, 16, 0, 24, 0, 8, 16, // Dx
    12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 0, 0, 8, 16, // Ex
    12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 0, 0, 8, 16  // Fx
};

static uint8_t instruction_cycles_prefix[256] = {
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // 0x
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // 1x
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // 2x
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // 3x
    8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, // 4x
    8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, // 5x
    8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, // 6x
    8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, // 7x
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // 8x
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // 9x
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // Ax
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // Bx
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // Cx
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // Dx
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, // Ex
    8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8  // Fx
};

InstructionDecoder::InstructionDecoder():
    instruction_(0),
    instruction_length_(0),
    instruction_cycles_(0),
    instruction_cycles_if_taken_(0),
    op_bytes_to_decode_(0),
    prefix_(false),
    decoding_finished_(false)
{
    operands_[0] = 0x00;
    operands_[1] = 0x00;
}

Instruction InstructionDecoder::decode(const uint8_t* memory, uint16_t* pc) {
    uint8_t internal_pc = 0;
    if (memory[0] == 0xCB) {
        internal_pc++;
        (*pc)++;
        prefix_ = true;
    }
    while (!decoding_finished_) {
        decodeByte(memory[internal_pc++]);
        (*pc)++;
    }
    internal_pc = 0;
    decoding_finished_ = false;
    Instruction instr{instruction_, prefix_, operands_[0], operands_[1], instruction_cycles_, instruction_cycles_if_taken_};
    instruction_ = 0x00;
    prefix_ = false;
    operands_[0] = 0x00;
    operands_[1] = 0x00;
    return instr;
}

void InstructionDecoder::decodeByte(uint8_t byte) {
    // Is the byte an instruction or an operand?
    const bool is_instruction = op_bytes_to_decode_ == 0;
    if (is_instruction) {
        decodeInstruction(byte);
    } else {
        decodeOperand(byte);
    }
}

void InstructionDecoder::decodeInstruction(uint8_t byte) {
    instruction_ = byte;
    if (!prefix_) {
        instruction_length_ = instruction_lengths[byte];
        instruction_cycles_ = instruction_cycles[byte];
        instruction_cycles_if_taken_ = instruction_cycles_if_taken[byte];
    } else {
        instruction_length_ = instruction_lengths_prefix[byte] - 1;
        // 4 additonal cycles for the 0xCB instruction
        instruction_cycles_ = instruction_cycles_prefix[byte] + 4;
        instruction_cycles_if_taken_ = instruction_cycles_;
    }

    if (instruction_length_ == 0 || instruction_length_ > 4) {
        fprintf(stderr, "ERROR Unknown instruction length of instruction %02X\n", byte);
        // sleep(10);
        exit(1);
    }

    op_bytes_to_decode_ = instruction_length_ - 1;

    if (op_bytes_to_decode_ == 0) {
        finishDecoding();
    }

    if (!prefix_ && byte == 0xCB) {
        prefix_ = true;
    }
}

void InstructionDecoder::decodeOperand(uint8_t byte) {
    operands_[2-op_bytes_to_decode_] = byte;
    op_bytes_to_decode_--;

    if (op_bytes_to_decode_ == 0) {
        finishDecoding();
    }
    return;
}

void InstructionDecoder::finishDecoding() {
    /* if (instruction_length_ == 1) {
        printf("Decoding finished: Prefix: %d, Instruction: %02X\n", prefix_, instruction_);
    } else if (instruction_length_ == 2) {
        printf("Decoding finished: Prefix: %d, Instruction: %02X, Operands: %02X\n", prefix_, instruction_, operands_[1]);
    } else if (instruction_length_ == 3) {
        printf("Decoding finished: Prefix: %d, Instruction: %02X, Operands: %02X %02X\n", prefix_, instruction_, operands_[0], operands_[1]);
    }
    fflush(stdout); */
    decoding_finished_ = true;

    // Reset prefix flag
    /* if (prefix_) {
        prefix_ = false;
    } */
}

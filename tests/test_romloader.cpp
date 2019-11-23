#define CATCH_CONFIG_MAIN
#include <string>

#include "catch2/catch.hpp"

#include "romloader.h"


TEST_CASE("Write ROM", "[romloader]") {
    const std::string filename{"/tmp/test.gb"};

    RomLoader rl(filename);
    uint8_t instructions[] = {0x01, 0x02, 0x03};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("Read bytes", "[romloader]") {
    const std::string filename{"/tmp/test.gb"};

    RomLoader rl(filename);
    uint8_t rom_byte;
    bool success = true;
    while (success) {
        success = rl.getNextByte(&rom_byte);
        printf("Byte: %02X\n", rom_byte);
    }
    REQUIRE(success == false);
}

TEST_CASE("Write instruction test ROM", "[romloader]") {
    const std::string filename{"/tmp/test_instr.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Set B to 0xFF
    // LD B, FF
    // 0x06 0xFF

    // Decrease B
    // DEC B
    // 0x05

    // Instruction to test
    // AND B, XOR B
    // 0xA0, 0xA8

    // Result is in A and F
    // PUSH AF
    // 0xF5

    // POP DE
    // 0xD1

    // XOR D
    // 0xAA

    // XOR E
    // 0xAB

    // Check B
    // CP B
    // 0xB8

    // Jump back to loop
    // JP NZ, a16
    // 0xC2 0x53 0x01

    RomLoader rl(filename);
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0xA0, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // AND B
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0xA8, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // XOR B
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0xB0, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // OR B
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x07, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RLCA, fixed
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x09, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // ADD HL, BC
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x0A, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // LD A, (BC), fails
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x0B, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // DEC BC
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x0C, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // INC C
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x0D, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // DEC C
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x0F, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RRCA, fixed
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x17, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RLA, fixed
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x1F, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RRA, fixed
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x23, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // INC HL
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x24, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // INC H
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x25, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // DEC H
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x26, 0x42, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // LD H, d8
    uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x27, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // DAA
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x29, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // ADD HL, HL
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x2A, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // LD A, (HL+)
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x2F, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // CPL
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0xCB, 0x38, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // SRL B
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0xCB, 0x08, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RRC B
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0xCB, 0x1A, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RRD
    // uint8_t instructions[] = {0xAF, 0x06, 0xFF, 0x05, 0x2A, 0xF5, 0xD1, 0xAA, 0xAB, 0xB8, 0xC2, 0x53, 0x01}; // RRA

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("Write interrupt test ROM", "[romloader]") {
    const std::string filename{"/tmp/test_interrupts.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // Enable interrupts FB
    // 0xFB

    // LD A, 04
    // 0x3E 0x04

    // LD 0xFF00 + FF, A (Write to IE)
    // 0xE0 0xFF

    // Set interrupt flag
    // LD 0xFF00 + 0F, A (Write to IE)
    // 0xE0 0x0F

    // At this point the CPU should immediately jump to 0x50


    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0xFB, 0x3E, 0x04, 0xE0, 0xFF, 0xE0, 0x0F};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("Write interrupt test ROM2", "[romloader]") {
    const std::string filename{"/tmp/test_interrupts2.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // Enable interrupts FB
    // 0xFB

    // LD A, 04
    // 0x3E 0x04

    // LD 0xFF00 + FF, A (Write to IE)
    // 0xE0 0xFF

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    // At this point the timer does not run, never triggers

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0xFB, 0x3E, 0x04, 0xE0, 0xFF, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("Test V-Blank interrupt", "[romloader]") {
    const std::string filename{"/tmp/test_vblank_interrupt.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // LD A, 01 (V-Blank)
    // 0x3E 0x01

    // LD 0xFF00 + FF, A (Write to IE)
    // 0xE0 0xFF

    // Enable interrupts FB
    // 0xFB

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    // At some point v-blank should trigger

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0x3E, 0x01, 0xE0, 0xFF, 0xFB, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}
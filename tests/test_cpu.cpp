#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "cpu.h"

TEST_CASE( "LD SP, d16", "[cpu]" ) {
    CPU cpu(nullptr);
    cpu.executeInstruction(0x31, false, 0xFE, 0xFF);
    REQUIRE(cpu.getSP() == 0xFFFE);
}

TEST_CASE( "OR A", "[cpu]" ) {
    CPU cpu(nullptr);

    // XOR A to get defined flags
    cpu.executeInstruction(0xAF, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to 0x30
    cpu.executeInstruction(0x3E, false, 0x42, 0x30);
    REQUIRE(cpu.getA() == 0x30);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // OR A
    cpu.executeInstruction(0xB7, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0x30);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);
}

TEST_CASE( "XOR A", "[cpu]" ) {
    CPU cpu(nullptr);
    cpu.executeInstruction(0xAF, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);
}

TEST_CASE( "LD (HL-), A\n", "[cpu]" ) {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to 0x01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // LD (a16), A sets (0xC000) to 1
    cpu.executeInstruction(0xEA, false, 0x00, 0xC0);

    // Set A to 0x02
    cpu.executeInstruction(0x3E, false, 0x00, 0x02);
    REQUIRE(cpu.getA() == 0x02);

    // LD (a16), A sets (0xC001) to 2
    cpu.executeInstruction(0xEA, false, 0x01, 0xC0);

    // Set HL to 0xC001
    cpu.executeInstruction(0x21, false, 0x01, 0xC0);
    REQUIRE(cpu.getHL() == 0xC001);

    // Set RAM at address HL to A and decrement HL
    cpu.executeInstruction(0x32, false, 0x42, 0x42);
    REQUIRE(cpu.getHL() == 0xC000);
    REQUIRE(cpu.getA() == 0x02);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xEA, 0x00, 0xC0, 0x3E, 0x02, 0xEA, 0x01, 0xC0, 0x21, 0x01, 0xC0, 0x32};
    RomLoader rl("");
    rl.writeRom("/tmp/ldhl-a.gb", instructions, sizeof(instructions));
}

TEST_CASE( "LD A, (HL-)\n", "[cpu]" ) {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to 0x01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // LD (a16), A sets (0xC000) to 1
    cpu.executeInstruction(0xEA, false, 0x00, 0xC0);

    // Set A to 0x02
    cpu.executeInstruction(0x3E, false, 0x00, 0x02);
    REQUIRE(cpu.getA() == 0x02);

    // LD (a16), A sets (0xC001) to 2
    cpu.executeInstruction(0xEA, false, 0x01, 0xC0);

    // Set HL to 0xC001
    cpu.executeInstruction(0x21, false, 0x01, 0xC0);
    REQUIRE(cpu.getHL() == 0xC001);

    // LD A, (HL-)
    cpu.executeInstruction(0x3A, false, 0x42, 0x42);
    REQUIRE(cpu.getHL() == 0xC000);
    REQUIRE(cpu.getA() == 0x02);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xEA, 0x00, 0xC0, 0x3E, 0x02, 0xEA, 0x01, 0xC0, 0x21, 0x01, 0xC0, 0x3A};
    RomLoader rl("");
    rl.writeRom("/tmp/ldahl-.gb", instructions, sizeof(instructions));
}

TEST_CASE( "LD C, d8\n", "[cpu]" ) {
    CPU cpu(nullptr);

    // Set C to 42
    cpu.executeInstruction(0x0E, false, 0xFF, 0x2A);
    REQUIRE(cpu.getC() == 42);
}

TEST_CASE("LD B, d8\n", "[cpu]") {
    CPU cpu(nullptr);

    // Set B to 42
    cpu.executeInstruction(0x06, false, 0xFF, 0x2A);
    REQUIRE(cpu.getB() == 42);
}

TEST_CASE("JP a16", "[cpu]") {
    CPU cpu(nullptr);

    // Jump to address 1
    cpu.executeInstruction(0xC3, false, 0x01, 0x00);
    REQUIRE(cpu.getPC() == 1);
}

TEST_CASE( "Test stack", "[cpu]" ) {
    CPU cpu(nullptr);
    // Set stackpointer to 0xFFFE
    cpu.executeInstruction(0x31, false, 0xFE, 0xFF);
    REQUIRE(cpu.getSP() == 0xFFFE);
    cpu.printStack();

    // Set BC to 0xABCD
    cpu.executeInstruction(0x01, false, 0xCD, 0xAB);
    REQUIRE(cpu.getBC() == 0xABCD);

    // Push BC onto stack and check that sp was decremented twice
    cpu.executeInstruction(0xC5, false, 0x42, 0x42);
    REQUIRE(cpu.getSP() == 0xFFFC);
    cpu.printStack();
}

TEST_CASE( "Test rotate left", "[cpu]" ) {
    CPU cpu(nullptr);

    // Set B to 1
    cpu.executeInstruction(0x06, false, 0xFF, 0x01);
    REQUIRE(cpu.getB() == 1);

    // Rotate B left (RLC)
    cpu.executeInstruction(0x00, true, 0x42, 0x42);
    REQUIRE(cpu.getB() == 2);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set B to 0b10000000
    cpu.executeInstruction(0x06, false, 0xFF, 0x80);
    REQUIRE(cpu.getB() == 128);

    // Rotate B left (RLC)
    cpu.executeInstruction(0x00, true, 0x42, 0x42);
    REQUIRE(cpu.getB() == 1);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagC() == 1);

    // Set B to 0b00000000
    cpu.executeInstruction(0x06, false, 0xFF, 0x00);
    REQUIRE(cpu.getB() == 0);

    // Rotate B left (RLC)
    cpu.executeInstruction(0x00, true, 0x42, 0x42);
    REQUIRE(cpu.getB() == 0);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagC() == 0);
}

TEST_CASE( "Test rotate left through carry", "[cpu]" ) {
    CPU cpu(nullptr);

    // Set C to 1
    cpu.executeInstruction(0x0E, false, 0xFF, 0x01);
    REQUIRE(cpu.getC() == 1);

    // Set carry bit
    cpu.setFlagC();

    // Rotate C left (RL)
    cpu.executeInstruction(0x11, true, 0x42, 0x42);
    REQUIRE(cpu.getC() == 3);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagC() == 0);
}

TEST_CASE( "Test rotate right through carry", "[cpu]" ) {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to 1
    cpu.executeInstruction(0x3E, false, 0x00, 0x02);
    REQUIRE(cpu.getA() == 2);

    // Reset carry bit
    // cpu.resetFlagC();

    // Rotate A right through carry RRA
    cpu.executeInstruction(0x1F, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 1);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Rotate A right through carry RRA
    cpu.executeInstruction(0x1F, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0);
    REQUIRE(cpu.getFlagZ() == 0); // RRA always resets Z
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 1);

    // Rotate A right through carry RRA
    cpu.executeInstruction(0x1F, false, 0x42, 0x42);
    REQUIRE(cpu.getA() == 0x80);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x3E, 0x02, 0x1F, 0x1F, 0x1F};
    RomLoader rl("");
    rl.writeRom("/tmp/rra.gb", instructions, sizeof(instructions));
}

TEST_CASE( "Test push and pop", "[cpu]" ) {
    CPU cpu(nullptr);

    // Set stackpointer to 0xFFFE
    cpu.executeInstruction(0x31, false, 0xFE, 0xFF);
    REQUIRE(cpu.getSP() == 0xFFFE);
    cpu.printStack();

    // Set DE to 0xABCD
    cpu.executeInstruction(0x11, false, 0xCD, 0xAB);
    REQUIRE(cpu.getDE() == 43981);

    // Push DE onto stack
    cpu.executeInstruction(0xD5, false, 0x42, 0x42);
    REQUIRE(cpu.getSP() == 0xFFFC);
    cpu.printStack();

    // Set BC to 0
    cpu.executeInstruction(0x01, false, 0x00, 0x00);
    REQUIRE(cpu.getBC() == 0);

    // Pop stack into BC
    cpu.executeInstruction(0xC1, false, 0x42, 0x42);
    REQUIRE(cpu.getBC() == 0xABCD);
    REQUIRE(cpu.getSP() == 0xFFFE);
    cpu.printStack();
}

TEST_CASE("Decrement", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set B to 42
    cpu.executeInstruction(0x06, false, 0x00, 0x2A);
    REQUIRE(cpu.getB() == 42);

    // Decrement B
    cpu.executeInstruction(0x05, false, 0x00, 0x00);
    REQUIRE(cpu.getB() == 41);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);

    // Set B to 1
    cpu.executeInstruction(0x06, false, 0x00, 0x01);
    REQUIRE(cpu.getB() == 1);

    // Decrement B
    cpu.executeInstruction(0x05, false, 0x00, 0x00);
    REQUIRE(cpu.getB() == 0);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);

    // Decrement B
    cpu.executeInstruction(0x05, false, 0x00, 0x00);
    REQUIRE(cpu.getB() == 0xFF);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 1);

    uint8_t instructions[] = {0xAF, 0x06, 0x2A, 0x05, 0x06, 0x01, 0x05, 0x05};
    RomLoader rl("");
    rl.writeRom("/tmp/dec.gb", instructions, sizeof(instructions));
}

TEST_CASE("Increment BC", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set B to 0
    cpu.executeInstruction(0x06, false, 0x00, 0x00);
    REQUIRE(cpu.getB() == 0);

    // Set C to 0
    cpu.executeInstruction(0x0E, false, 0x00, 0x00);
    REQUIRE(cpu.getC() == 0);

    // Increment BC
    cpu.executeInstruction(0x03, false, 0x00, 0x00);
    REQUIRE(cpu.getB() == 0);
    REQUIRE(cpu.getC() == 1);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x06, 0x00, 0x0E, 0x00, 0x03};
    RomLoader rl("");
    rl.writeRom("/tmp/inc_bc.gb", instructions, sizeof(instructions));
}

TEST_CASE("Increment SP", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set SP to 0
    cpu.executeInstruction(0x31, false, 0x00, 0x00);
    REQUIRE(cpu.getSP() == 0x0000);

    // Increment SP
    cpu.executeInstruction(0x33, false, 0x00, 0x00);
    REQUIRE(cpu.getSP() == 0x0001);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set SP to 0x000F
    cpu.executeInstruction(0x31, false, 0x0F, 0x00);
    REQUIRE(cpu.getSP() == 0x000F);

    // Increment SP
    cpu.executeInstruction(0x33, false, 0x00, 0x00);
    REQUIRE(cpu.getSP() == 0x0010);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set SP to 0xFFFF
    cpu.executeInstruction(0x31, false, 0xFF, 0xFF);
    REQUIRE(cpu.getSP() == 0xFFFF);

    // Increment SP
    cpu.executeInstruction(0x33, false, 0x00, 0x00);
    REQUIRE(cpu.getSP() == 0x0000);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x31, 0x00, 0x00, 0x33, 0x31, 0x0F, 0x00, 0x33, 0x31, 0xFF, 0xFF, 0x33};
    RomLoader rl("");
    rl.writeRom("/tmp/inc_sp.gb", instructions, sizeof(instructions));
}

TEST_CASE("Half carry increment", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set E to 0E
    cpu.executeInstruction(0x1E, false, 0x00, 0x0E);
    REQUIRE(cpu.getE() == 0x0E);

    // Increment E
    cpu.executeInstruction(0x1C, false, 0x00, 0x00);
    REQUIRE(cpu.getE() == 0x0F);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Increment E
    cpu.executeInstruction(0x1C, false, 0x00, 0x00);
    REQUIRE(cpu.getE() == 0x10);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 0);

    // Increment E
    cpu.executeInstruction(0x1C, false, 0x00, 0x00);
    REQUIRE(cpu.getE() == 0x11);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x1E, 0x0E, 0x1C, 0x1C, 0x1C};
    RomLoader rl("");
    rl.writeRom("/tmp/inc_e_hc.gb", instructions, sizeof(instructions));
}

TEST_CASE("Lower 4 bits of F are always zero", "[cpu]") {
    CPU cpu(nullptr);

    // LD BC, 0xFFFF
    cpu.executeInstruction(0x01, false, 0xFF, 0xFF);
    REQUIRE(cpu.getB() == 0xFF);
    REQUIRE(cpu.getC() == 0xFF);

    // Push BC onto stack
    cpu.executeInstruction(0xC5, false, 0x42, 0x42);

    // Pop AF
    cpu.executeInstruction(0xF1, false, 0x42, 0x42);

    // Test that lower 4 bits of F are 0
    REQUIRE(cpu.getA() == 0xFF);
    REQUIRE(cpu.getF() == 0xF0);
}

TEST_CASE("Shift right into carry", "[cpu]") {
    CPU cpu(nullptr);

    // Set B to 0x42
    cpu.executeInstruction(0x06, false, 0x00, 0x42);
    REQUIRE(cpu.getB() == 0x42);

    // SRL B
    cpu.executeInstruction(0x38, true, 0xFF, 0xFF);
    REQUIRE(cpu.getB() == 0x21);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set B to 0x01
    cpu.executeInstruction(0x06, false, 0x00, 0x01);
    REQUIRE(cpu.getB() == 0x01);

    // SRL B
    cpu.executeInstruction(0x38, true, 0xFF, 0xFF);
    REQUIRE(cpu.getB() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 1);

    uint8_t instructions[] = {0x06, 0x42, 0xCB, 0x38, 0x06, 0x01, 0xCB, 0x38};
    RomLoader rl("");
    rl.writeRom("/tmp/srl_b.gb", instructions, sizeof(instructions));
}

TEST_CASE("Add N plus carry into A", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 0x01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // ADC A, 0x01
    cpu.executeInstruction(0xCE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x02);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // ADC A, 0xFD
    cpu.executeInstruction(0xCE, false, 0x00, 0xFD);
    REQUIRE(cpu.getA() == 0xFF);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // ADC A, 0x01 (triggers overflow)
    cpu.executeInstruction(0xCE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    // ADC A, 0x01 (should now add carry, result is 2)
    cpu.executeInstruction(0xCE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x02);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to FF
    // LD A, 0xFF
    cpu.executeInstruction(0x3E, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0xFF);

    // ADC A, 0x01 (triggers overflow)
    cpu.executeInstruction(0xCE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    // ADC A, 0xFF
    cpu.executeInstruction(0xCE, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xCE, 0x01, 0xCE, 0xFD, 0xCE, 0x01, 0xCE, 0x01, 0x3E, 0xFF, 0xCE, 0x01, 0xCE, 0xFF};
    RomLoader rl("");
    rl.writeRom("/tmp/adc_a.gb", instructions, sizeof(instructions));
}

TEST_CASE("Subtract N plus carry from A", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 0x01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // SBC A, 0x01
    cpu.executeInstruction(0xDE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // SBC A, 0xFD
    cpu.executeInstruction(0xDE, false, 0x00, 0xFD);
    REQUIRE(cpu.getA() == 0x03);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    // SBC A, 0x01
    cpu.executeInstruction(0xDE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // SBC A, 0x01
    cpu.executeInstruction(0xDE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // Set A to FF
    // LD A, 0xFF
    cpu.executeInstruction(0x3E, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0xFF);

    // SBC A, 0x01
    cpu.executeInstruction(0xDE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0xFE);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // SBC A, 0xFF
    cpu.executeInstruction(0xDE, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0xFF);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    // Set A to 01
    // LD A, 0x01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // SBC A, 0x01 (should now subtract 2)
    cpu.executeInstruction(0xDE, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0xFF);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xDE, 0x01, 0xDE, 0xFD, 0xDE, 0x01, 0xDE, 0x01, 0x3E, 0xFF, 0xDE, 0x01, 0xDE, 0xFF, 0x3E, 0x01, 0xDE, 0x01};
    RomLoader rl("");
    rl.writeRom("/tmp/sbc_a.gb", instructions, sizeof(instructions));
}

TEST_CASE("SUB d8", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 0x01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // SUB d8
    cpu.executeInstruction(0xD6, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 0xFF
    cpu.executeInstruction(0x3E, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0xFF);

    // SUB d8
    cpu.executeInstruction(0xD6, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 0x02
    cpu.executeInstruction(0x3E, false, 0x00, 0x02);
    REQUIRE(cpu.getA() == 0x02);

    // SUB d8
    cpu.executeInstruction(0xD6, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // SUB d8
    cpu.executeInstruction(0xD6, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // SUB d8
    cpu.executeInstruction(0xD6, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0xFF);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xD6, 0x01, 0x3E, 0xFF, 0xD6, 0xFF, 0x3E, 0x02, 0xD6, 0x01, 0xD6, 0x01, 0xD6, 0x01};
    RomLoader rl("");
    rl.writeRom("/tmp/subd8.gb", instructions, sizeof(instructions));
}

TEST_CASE("ADD SP", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD SP, d16
    cpu.executeInstruction(0x31, false, 0x34, 0x12);
    REQUIRE(cpu.getSP() == 0x1234);

    // ADD 2 to SP
    cpu.executeInstruction(0xE8, false, 0x00, 0x02);
    REQUIRE(cpu.getSP() == 0x1236);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD SP, d16
    cpu.executeInstruction(0x31, false, 0x3F, 0x12);
    REQUIRE(cpu.getSP() == 0x123F);

    // ADD 2 to SP
    cpu.executeInstruction(0xE8, false, 0x00, 0x01);
    REQUIRE(cpu.getSP() == 0x1240);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x31, 0x34, 0x12, 0xE8, 0x02, 0x31, 0x3F, 0x12, 0xE8, 0x01};
    RomLoader rl("");
    rl.writeRom("/tmp/add_sp.gb", instructions, sizeof(instructions));
}

TEST_CASE("ADD HL", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD HL, d16
    cpu.executeInstruction(0x21, false, 0x00, 0x00);
    REQUIRE(cpu.getHL() == 0x0000);

    // LD SP, d16
    cpu.executeInstruction(0x31, false, 0x34, 0x12);
    REQUIRE(cpu.getSP() == 0x1234);

    // ADD SP to HL
    cpu.executeInstruction(0x39, false, 0x00, 0x00);
    REQUIRE(cpu.getHL() == 0x1234);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD HL, d16
    cpu.executeInstruction(0x21, false, 0x01, 0x00);
    REQUIRE(cpu.getHL() == 0x0001);

    // LD SP, d16
    cpu.executeInstruction(0x31, false, 0xFF, 0x00);
    REQUIRE(cpu.getSP() == 0x00FF);

    // ADD SP to HL
    cpu.executeInstruction(0x39, false, 0x00, 0x00);
    REQUIRE(cpu.getHL() == 0x0100);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD HL, d16
    cpu.executeInstruction(0x21, false, 0xFF, 0xFF);
    REQUIRE(cpu.getHL() == 0xFFFF);

    // LD SP, d16
    cpu.executeInstruction(0x31, false, 0x01, 0x00);
    REQUIRE(cpu.getSP() == 0x0001);

    // ADD SP to HL
    cpu.executeInstruction(0x39, false, 0x00, 0x00);
    REQUIRE(cpu.getHL() == 0x0000);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    // LD HL, d16
    cpu.executeInstruction(0x21, false, 0xFF, 0x0F);
    REQUIRE(cpu.getHL() == 0x0FFF);

    // LD SP, d16
    cpu.executeInstruction(0x31, false, 0x01, 0x00);
    REQUIRE(cpu.getSP() == 0x0001);

    // ADD SP to HL
    cpu.executeInstruction(0x39, false, 0x00, 0x00);
    REQUIRE(cpu.getHL() == 0x1000);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x21, 0x00, 0x00, 0x31, 0x34, 0x12, 0x39, 0x21, 0x01, 0x00, 0x31, 0xFF, 0x00, 0x39, 0x21, 0xFF, 0xFF, 0x31, 0x01, 0x00, 0x39, 0x21, 0xFF, 0x0F, 0x31, 0x01, 0x00, 0x39};
    RomLoader rl("");
    rl.writeRom("/tmp/add_hl.gb", instructions, sizeof(instructions));
}

TEST_CASE("Compare", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD B, 00
    cpu.executeInstruction(0x06, false, 0x00, 0x00);
    REQUIRE(cpu.getB() == 0x00);

    // CP B
    cpu.executeInstruction(0xB8, false, 0x00, 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD B, 01
    cpu.executeInstruction(0x06, false, 0x00, 0x01);
    REQUIRE(cpu.getB() == 0x01);

    // CP B
    cpu.executeInstruction(0xB8, false, 0x00, 0x00);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 1);
    REQUIRE(cpu.getFlagC() == 1);

    // LD A, 2
    cpu.executeInstruction(0x3E, false, 0x00, 0x02);
    REQUIRE(cpu.getA() == 0x02);

    // CP B
    cpu.executeInstruction(0xB8, false, 0x00, 0x00);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 1);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    uint8_t instructions[] = {0xAF, 0x06, 0x00, 0xB8, 0x06, 0x01, 0xB8, 0x3E, 0x02, 0xB8};
    RomLoader rl("");
    rl.writeRom("/tmp/cp_b.gb", instructions, sizeof(instructions));
}

TEST_CASE("DAA", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 00
    cpu.executeInstruction(0x3E, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);

    // DAA
    cpu.executeInstruction(0x27, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // DAA
    cpu.executeInstruction(0x27, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x01);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 0F
    cpu.executeInstruction(0x3E, false, 0x00, 0x0F);
    REQUIRE(cpu.getA() == 0x0F);

    // DAA
    cpu.executeInstruction(0x27, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x15);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, FF
    cpu.executeInstruction(0x3E, false, 0x00, 0xFF);
    REQUIRE(cpu.getA() == 0xFF);

    // DAA
    cpu.executeInstruction(0x27, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x65);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 1);

    // LD A, 00
    cpu.executeInstruction(0x3E, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);

    // DAA
    cpu.executeInstruction(0x27, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x60);
    REQUIRE(cpu.getFlagZ() == 0);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 1);

    uint8_t instructions[] = {0xAF, 0x3E, 0x00, 0x27, 0x3E, 0x01, 0x27, 0x3E, 0x0F, 0x27, 0x3E, 0xFF, 0x27, 0x3E, 0x00, 0x27};
    RomLoader rl("");
    rl.writeRom("/tmp/daa.gb", instructions, sizeof(instructions));
}

TEST_CASE("HALT bug 1", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // SET IE and IF to 1 (enable v-blank)
    cpu.executeInstruction(0xEA, false, 0x0F, 0xFF); // IF
    cpu.executeInstruction(0xEA, false, 0xFF, 0xFF); // IE

    // DI
    cpu.executeInstruction(0xF3, false, 0x00, 0x00);

    // HALT
    cpu.executeInstruction(0x76, false, 0x00, 0x00);

    // INC A
    cpu.executeInstruction(0x3C, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x02);

    // LD (HL+), A
    cpu.executeInstruction(0x22, false, 0x00, 0x00);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xEA, 0x0F, 0xFF, 0xEA, 0xFF, 0xFF, 0xF3, 0x76, 0x3C, 0x22};
    RomLoader rl("");
    rl.writeRom("/tmp/halt_bug1.gb", instructions, sizeof(instructions));
}

TEST_CASE("HALT bug 2", "[cpu]") {
    CPU cpu(nullptr);

    // XOR A (sets flags to defined state)
    cpu.executeInstruction(0xAF, false, 0x00, 0x00);
    REQUIRE(cpu.getA() == 0x00);
    REQUIRE(cpu.getFlagZ() == 1);
    REQUIRE(cpu.getFlagN() == 0);
    REQUIRE(cpu.getFlagH() == 0);
    REQUIRE(cpu.getFlagC() == 0);

    // LD A, 01
    cpu.executeInstruction(0x3E, false, 0x00, 0x01);
    REQUIRE(cpu.getA() == 0x01);

    // SET IE and IF to 1 (enable v-blank)
    cpu.executeInstruction(0xEA, false, 0x0F, 0xFF); // IF
    cpu.executeInstruction(0xEA, false, 0xFF, 0xFF); // IE

    // DI
    cpu.executeInstruction(0xF3, false, 0x00, 0x00);

    // HALT
    cpu.executeInstruction(0x76, false, 0x00, 0x00);

    // LD A, 14
    cpu.executeInstruction(0x3E, false, 0x00, 0x14);
    // REQUIRE(cpu.getA() == 0x3E);
    REQUIRE(cpu.getA() == 0x14);

    // LD (HL+), A
    cpu.executeInstruction(0x22, false, 0x00, 0x00);

    uint8_t instructions[] = {0xAF, 0x3E, 0x01, 0xEA, 0x0F, 0xFF, 0xEA, 0xFF, 0xFF, 0xF3, 0x76, 0x3E, 0x14, 0x22};
    RomLoader rl("");
    rl.writeRom("/tmp/halt_bug2.gb", instructions, sizeof(instructions));
}

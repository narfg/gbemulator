#define CATCH_CONFIG_MAIN
#include <string>

#include "catch2/catch.hpp"

#include "romloader.h"


// Tests that DIV is 0 after writing any value to it
TEST_CASE( "Test DIV", "[timer]" ) {
    const std::string filename{"/tmp/test_div_write.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // LD A, 0x42
    // 0x3E 0x42

    // Write A to DIV (DIV should reset to 0)
    // LD 0xFF00 + 04, A (Write to DIV)
    // 0xE0 0x04

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0x3E, 0x42, 0xE0, 0x04, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("Write timer interrupt test", "[timer]") {
    const std::string filename{"/tmp/test_timer_interrupt.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // LD A, 04
    // 0x3E 0x04

    // LD 0xFF00 + FF, A (Write to IE)
    // 0xE0 0xFF

    // Enable timer at cpu-clock / 16
    // LD A, 05
    // 0x3E 0x05

    // LD 0xFF00 + 07, A (Write to TAC)
    // 0xE0 0x07

    // Enable interrupts FB
    // 0xFB

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    // At this point the timer runs and triggers at some point

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0x3E, 0x04, 0xE0, 0xFF, 0x3E, 0x05, 0xE0, 0x07, 0xFB, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "ppu.h"
#include "romloader.h"

TEST_CASE("printTile", "[ppu]") {
    uint8_t ram[65536] = {0};

    // Fill one tile (example from https://fms.komkon.org/GameBoy/Tech/Software.html)
    const uint8_t tile[] = {0x7C, 0x7C, 0x00, 0xC6, 0xC6, 0x00, 0x00, 0xFE, 0xC6, 0xC6, 0x00, 0xC6, 0xC6, 0x00, 0x00, 0x00};
    memcpy(ram + 0x8000, tile, 16);

    PPU ppu(ram);
    ppu.printTile(0);
    ppu.showTiles();
    ppu.showDisplay();
    sleep(10);
}

TEST_CASE("LCD interrupt LY = LYC", "[ppu]") {
    const std::string filename{"/tmp/test_lcd_lyc_interrupt.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // LD A, 02
    // 0x3E 0x02

    // LD 0xFF00 + FF, A (Write to IE, enable LCD interrupt)
    // 0xE0 0xFF

    // Set LYC to 0x03
    // LD A, 0x03
    // 0x3E 0x03
    // LD 0xFF00 + 0x45, A
    // 0xE0 0x45

    // Set bit 6 of STAT FF41 (enables ly=lyc interrupt)
    // LD HL, 0xFF41
    // 0x21 0x41 0xFF
    // Set 6, (HL)
    // 0xCB 0xF6

    // Enable interrupts FB
    // 0xFB

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    // At this point LCD interrupt should be triggered eventually

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0x3E, 0x02, 0xE0, 0xFF, 0x3E, 0x03, 0xE0, 0x45, 0x21, 0x41, 0xFF, 0xCB, 0xF6, 0xFB, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("LCD interrupt vblank", "[ppu]") {
    const std::string filename{"/tmp/test_lcd_vblank_interrupt.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // LD A, 02
    // 0x3E 0x02

    // LD 0xFF00 + FF, A (Write to IE, enable LCD interrupt)
    // 0xE0 0xFF

    // Set bit 4 of STAT FF41 (enables vblank interrupt)
    // LD HL, 0xFF41
    // 0x21 0x41 0xFF
    // Set 4, (HL)
    // 0xCB 0xE6

    // Enable interrupts FB
    // 0xFB

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    // At this point LCD interrupt should be triggered eventually

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0x3E, 0x02, 0xE0, 0xFF, 0x21, 0x41, 0xFF, 0xCB, 0xE6, 0xFB, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

TEST_CASE("LCD interrupt vblank and vblank", "[ppu]") {
    const std::string filename{"/tmp/test_lcd_vblank2_interrupt.gb"};

    // Set AF to defined state
    // XOR A
    // 0xAF

    // Disable interrupts F3
    // 0xF3

    // LD A, 03
    // 0x3E 0x03

    // LD 0xFF00 + FF, A (Write to IE, enable LCD interrupt and vblank interrupt)
    // 0xE0 0xFF

    // Set bit 4 of STAT FF41 (enables ly=lyc interrupt)
    // LD HL, 0xFF41
    // 0x21 0x41 0xFF
    // Set 4, (HL)
    // 0xCB 0xE6

    // Enable interrupts FB
    // 0xFB

    // NOP
    // 0x00

    // JR -3
    // 0x18 0xFD

    // At this point LCD interrupt should be triggered eventually

    RomLoader rl(filename);
    uint8_t instructions[] = {0xAF, 0xF3, 0x3E, 0x03, 0xE0, 0xFF, 0x21, 0x41, 0xFF, 0xCB, 0xE6, 0xFB, 0x00, 0x18, 0xFD};

    rl.writeRom(filename, instructions, sizeof(instructions));
}

// FF40 LCDC
// FF41 STAT
//

// Test LY = LYC interrupt
// FF44 LY  (R)
// FF45 LYC (R/W)

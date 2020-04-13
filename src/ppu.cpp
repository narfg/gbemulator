#include "ppu.h"

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "utils.h"

#define WIDTH 512
#define HEIGHT 64

#define WIDTH_D 256
#define HEIGHT_D 256

PPU::PPU(uint8_t* ram, std::unique_ptr<Display> display):
    display_(std::move(display)),
    ram_(ram),
    mode_(0),
    line_(0),
    col_(0)
{
    start_ = std::chrono::system_clock::now();
    display_->init(WIDTH_D, HEIGHT_D);
}

PPU::~PPU() {
    display_->close();
}

void PPU::tick()
{
    if (!getDisplayEnable()) {
        return;
    }
    // Handle interrupts
    // 0xFF41 LCD STAT register
    uint8_t interrupt_enable = ram_[0xFF41] & 0x7C;

    line_ = ram_[0xFF44];
    col_++;
    if (col_ > 113) {
        col_ = 0;
        line_++;
        ram_[0xFF44] = line_;
        // H-Blank interrupt
        if (interrupt_enable && getBitN(ram_[0xFF41], 3)) {
            ram_[0xFF0F] |= 1;
        }
    }
    if (line_ > 153) {
        line_ = 0;
        ram_[0xFF44] = line_;
    }

    if (line_ == 144 && col_ == 0) {
        // V-Blank interrupt
        ram_[0xFF0F] |= 1;
        showDisplay();
        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        if (elapsed.count() < 16750) {  // 59.7 Hz
            // std::this_thread::sleep_for(std::chrono::microseconds(16750 - elapsed.count()));
        } else {
            printf("Emulation running too slow.\n");
        }
        // auto end2 = std::chrono::system_clock::now();
        // auto elapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start_);
        // printf("microseconds: %ld\n", elapsed.count());
        // printf("microseconds after sleep: %ld\n", elapsed2.count());
        start_ = std::chrono::system_clock::now();
    }

    // Set mode
    if (line_ > 143) {
        // V-Blank
        // Mode 1
        setMode(1);
        // ram_[0xFF85] = 1;
    } else {
        if (col_ < 20) {
            // OAM search
            // Mode 2
            setMode(2);
        } else if (col_ < 62) {
            // Pixel transfer
            // Mode 3
            setMode(3);
        } else {
            // H-Blank
            // Mode 0
            setMode(0);
        }
    }
    // Set LY
    // ram_[0xFF44] = line_;

    // if (interrupt_enable) {
        // exit(1);
    // }

    // Set stat register coincidence flag LYC=LY
    uint8_t ly = ram_[0xFF44];
    uint8_t lyc = ram_[0xFF45];
    if (lyc == ly) {
        // Set coincidence flag
        ram_[0xFF41] |= (1 << 2);
        // Register LCD STAT interrupt
        if (interrupt_enable && getBitN(ram_[0xFF41], 6)) {
            ram_[0xFF0F] |= 2;
        }
    } else {
        // Reset coincidence flag
        ram_[0xFF41] &= ~(1 << 2);
    }

    /* printf("row: %d, col: %d\n", line_, col_);
    fflush(stdout); */
}

void PPU::setMode(uint8_t mode) {
    mode_ = mode;

    // Clear mode bits
    ram_[0xFF41] &= ~(0x03);
    // Set mode bits
    ram_[0xFF41] |= 0x03 & mode;
}

bool PPU::getDisplayEnable()
{
    return getBitN(ram_[0xFF40], 7) == 0x01;
}

uint16_t PPU::getBGTileMapStart()
{
    if (getBitN(ram_[0xFF40], 3) == 0x01) {
        return 0x9C00;
    } else {
        return 0x9800;
    }
}

uint16_t PPU::getTileDataStart(uint8_t number) const
{
    // (0=8800-97FF, 1=8000-8FFF)
    if (getBitN(ram_[0xFF40], 4) == 0x01) {
        return 0x8000 + number * 16;
    } else {
        if (number < 128) {
            return 0x9000 + number * 16;
        } else {
            return 0x8800 + (number-128) * 16;
        }
    }
}

void PPU::printTile(uint8_t number) const {
    // Tiles are stored between 0x8000 and 0x8FFF or 0x8800-0x97FF.
    // Tiles are 16 byte big (8x8 * 2bit)
    uint8_t* tile_start = ram_ + getTileDataStart(number);
    for (uint8_t row = 0; row < 8; ++row) {
        // Get two bytes that represent this row
        uint8_t byte0 = *(tile_start + row * 2 + 0);
        uint8_t byte1 = *(tile_start + row * 2 + 1);
        for (uint8_t col = 0; col < 8; ++col) {
            uint8_t low  = getBitN(byte0, 7-col);
            uint8_t high = getBitN(byte1, 7-col);
            uint8_t color = static_cast<uint8_t>(high << 1) | low;
            printf("%d", color);
        }
        printf("\n");
    }
}

void PPU::drawPixel(uint16_t x, uint16_t y, uint8_t color) {
    display_->drawPixel(x, y, color);
}

void PPU::showDisplay() {
    // Read from BG map 1
    for (uint8_t row = 0; row < 32; ++row) {
        for (uint8_t col = 0; col < 32; ++col) {
            uint8_t tile_number = *(ram_ + getBGTileMapStart() + row * 32 + col);
            uint16_t x_start = col * 8;
            uint16_t y_start = row * 8;
            showTile(tile_number, x_start, y_start);
        }
    }
    // Draw tiles
    for (uint8_t k = 0; k < 40; ++k) {
        showSprite(k);
    }

    display_->update();
}

void PPU::showTile(uint8_t number, uint16_t x_start, uint16_t y_start) {
    // Tiles are stored between 0x8000 and 0x8FFF or 0x8800-0x97FF.
    // Tiles are 16 byte big (8x8 * 2bit)
    uint8_t* tile_start = ram_ + getTileDataStart(number);

    for (uint8_t row = 0; row < 8; ++row) {
        // Get two bytes that represent this row
        uint8_t byte0 = *(tile_start + row * 2 + 0);
        uint8_t byte1 = *(tile_start + row * 2 + 1);
        for (uint8_t col = 0; col < 8; ++col) {
            uint8_t low  = getBitN(byte0, 7-col);
            uint8_t high = getBitN(byte1, 7-col);
            uint8_t color = static_cast<uint8_t>(high << 1) | low;
            // Get grey shade from palette
            uint8_t grey_value_palette = (ram_[0xFF47] >> (2*color)) & 0x03;
            drawPixel(x_start + col, y_start + row, grey_value_palette);
        }
    }
}

void PPU::showSprite(uint8_t number) {
    // TODO: Sprite ordering
    // TODO: Sprite flipping
    // TODO: Use flags

    // Read sprite attributes
    uint8_t* object_attributes = ram_ + 0xFE00 + number * 4;
    uint8_t y_pos = object_attributes[0];
    uint8_t x_pos = object_attributes[1];
    uint8_t tile  = object_attributes[2];
    // uint8_t flags = object_attributes[3];

    // Tiles are stored between 0x8000 and 0x8FFF
    // Tiles are 16 byte big (8x8 * 2bit)
    uint8_t* tile_start = ram_ + 0x8000 + tile * 16;

    for (uint8_t row = 0; row < 8; ++row) {
        // Get two bytes that represent this row
        uint8_t byte0 = *(tile_start + row * 2 + 0);
        uint8_t byte1 = *(tile_start + row * 2 + 1);
        for (uint8_t col = 0; col < 8; ++col) {
            uint8_t low  = getBitN(byte0, 7-col);
            uint8_t high = getBitN(byte1, 7-col);
            uint8_t color = static_cast<uint8_t>(high << 1) | low;
            // Get grey shade from palette
            uint8_t grey_value_palette = (ram_[0xFF47] >> (2*color)) & 0x03;
            int16_t x = x_pos + col - 8 + ram_[0xFF43];
            int16_t y = y_pos + row - 16 + ram_[0xFF42];
            if (grey_value_palette !=0 && x >= 0 && y >= 0) {
                drawPixel(x, y, grey_value_palette);
            }
        }
    }
}

void PPU::showTiles() {
    uint8_t tile_number = 0;
    for (uint8_t row = 0; row < 32; ++row) {
        for (uint8_t col = 0; col < 32; ++col) {
            uint16_t x_start = col * 8;
            uint16_t y_start = row * 8;
            showTile(tile_number, x_start, y_start);
            tile_number++;
        }
    }

    display_->update();
}

void PPU::printTileMemory() const {
    for (uint16_t addr = 0x8000; addr < 0x8FFF; ++addr) {
        printf("%02X", ram_[addr]);
    }
}

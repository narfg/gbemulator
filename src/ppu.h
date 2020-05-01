#pragma once
#include <chrono>
#include <cstdint>
#include <memory>

#include "display.h"

class PPU
{
public:
    PPU(uint8_t* ram, std::unique_ptr<Display> display);
    ~PPU();

    void tick();
    void printTile(uint8_t number) const;
    void printTileMemory() const;
    void showTiles();
    void showDisplay();

private:
    bool getDisplayEnable();
    uint16_t getBGTileMapStart();
    uint16_t getTileDataStart(uint8_t number) const;
    void drawPixel(int16_t x, int16_t y, uint8_t color);
    void showTile(uint8_t number, int16_t x_start, int16_t y_start);
    void showSprite(uint8_t number);
    void setMode(uint8_t mode);

    std::unique_ptr<Display> display_;

    uint8_t* ram_;
    uint8_t mode_;
    uint8_t line_;
    uint8_t col_;
    std::chrono::time_point<std::chrono::system_clock> start_;
};

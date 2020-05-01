#pragma once
#include <cstdint>
#include <array>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

#include "display.h"

// Display Settings
const int TFT_CS  = 5;
const int TFT_RST = 19;
const int TFT_DC  = 2;
// HW SPI will use those pins
// const int TFT_MOSI = 23
// const int TFT_SCLK = 18

class ST7789Display : public Display {
public:
    ST7789Display() : tft_(Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST)) {
    }
    void init(uint16_t /* width */, uint16_t /* height */) override {
        tft_.init(240, 320);
        tft_.fillScreen(ST77XX_BLACK);
    }
    void close() override {}
    void update() override {}
    void drawPixel(uint16_t x, uint16_t y, uint8_t color) override {
        // tft_.drawPixel(x, y, colors_[color]);
        // Landscape mode
        if (y < 240)
            tft_.drawPixel(y, tft_.height() - ((320-256)/2+x), colors_[color]);
    }
private:
    Adafruit_ST7789 tft_;

    // Green shades in 16bit RGB565 format
    std::array<uint16_t, 4> colors_{{0xE7DA, 0x8E0E, 0x334A, 0x08C4}};
};

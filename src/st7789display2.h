#pragma once
#include <array>
#include <cstdint>
#include <thread>

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

class FastDisplay : public Adafruit_ST7789 {
public:
    FastDisplay(int8_t cs, int8_t dc, int8_t rst) : Adafruit_ST7789(cs, dc, rst) {
    }

    void drawLines(uint16_t y) {
        startWrite();
        setAddrWindow(0, y, 240, 1);
        SPI.write16(0x0);
        endWrite();
    }

    void setFBPixel(uint16_t x, uint16_t y, uint16_t color) {
        // This rotates the framebuffer
        uint16_t y_display = x;
        uint16_t x_display = 143 - y;
        fb_[y_display * 144 + x_display] = color;
    }

    void drawFB() {
        std::thread thread{&FastDisplay::drawFB2, this};
        thread.detach();
    }

    void drawFB2() {
        constexpr uint16_t x_start = (240 - 144)/2;
        constexpr uint16_t y_start = (320 - 160)/2;
        drawRGBBitmap(x_start, y_start, fb_, 144, 160);
    }

private:
    uint16_t fb_[144 * 160] = {0xE7DA};
};

class ST7789Display : public Display {
public:
    ST7789Display() : tft_(FastDisplay(TFT_CS, TFT_DC, TFT_RST)) {
    }
    void init(uint16_t /* width */, uint16_t /* height */) override {
        tft_.init(240, 320);
        tft_.fillScreen(colors_[0]);
    }
    void close() override {}
    void update() override {
        tft_.drawFB();
    }
    void drawPixel(uint16_t x, uint16_t y, uint8_t color) override {
        // tft_.drawPixel(x, y, colors_[color]);
        // Landscape mode
        /*if (y < 240)
            tft_.drawPixel(y, tft_.height() - ((320-256)/2+x), colors_[color]);*/
        if (x < 160 && y < 144)
            tft_.setFBPixel(x, y, colors_[color]);

    }
private:
    FastDisplay tft_;

    // Green shades in 16bit RGB565 format
    std::array<uint16_t, 4> colors_{{0xE7DA, 0x8E0E, 0x334A, 0x08C4}};
};

#pragma once
#include <SSD1306Wire.h>

#include <cstdint>

#include "display.h"

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = 5;
const int SDC_PIN = 4;

// Initialize the oled display for address 0x3c
SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);

class OledDisplay : public Display {
    void init(uint16_t /* width */, uint16_t /* height */) override {
        display.init();
        display.clear();
        display.display();
    }
    void close() override {}
    void update() override { display.display(); }
    void drawPixel(uint16_t x, uint16_t y, uint8_t color) override {
        x = x - 16;
        y = y - 63;
        if (x < 128 && y < 64) {
            if (color > 1) {
                display.setColor(WHITE);
            } else {
                display.setColor(BLACK);
            }
            // Serial.println("drawPixel");
            display.setPixel(x, y);
        }
    }
};

#pragma once
#include <array>
#include <cstdint>
#include <thread>

#include <odroid_go.h>
#undef min

#include "display.h"
#include "joypad.h"

class FastDisplay {
public:
    FastDisplay(int8_t cs, int8_t dc, int8_t rst) {
    }

    void setFBPixel(uint16_t x, uint16_t y, uint16_t color) {
        fb_[y * 160 + x] = color;
    }

    void drawFB() {
        // std::thread thread{&FastDisplay::drawFB2, this};
        // thread.detach();
        drawFB2();
    }

    void drawFB2() {
        constexpr uint16_t x_start = (320 - 160)/2;
        constexpr uint16_t y_start = (240 - 144)/2;
        // drawRGBBitmap(x_start, y_start, fb_, 144, 160);
        GO.lcd.drawBitmap(x_start, y_start, 160, 144, fb_);
    }

private:
    uint16_t fb_[144 * 160] = {0xDAE7};
};

class OdroidDisplay : public Display {
public:
    OdroidDisplay(uint8_t* ram = nullptr, Joypad* joypad = nullptr):
        tft_(FastDisplay(TFT_CS, TFT_DC, TFT_RST)),
        ram_(ram),
        joypad_(joypad)
    {
    }
    void init(uint16_t /* width */, uint16_t /* height */) override {
        GO.begin();
        muteSpeaker();
        // GO.lcd.fillScreen(colors_[0]); // Doesn't work
    }
    void close() override {}
    void update() override {
        updateButtons();
        tft_.drawFB();
    }
    void drawPixel(uint16_t x, uint16_t y, uint8_t color) override {
        if (x < 160 && y < 144)
            tft_.setFBPixel(x, y, colors_[color]);
    }
private:

    void muteSpeaker() {
        // The Odroid GO will amplify noise to the speaker if these pins are not pulled low
        digitalWrite(25, 0);
        digitalWrite(26, 0);
    }

    void updateButtons() {
        GO.JOY_Y.readAxis();
        GO.JOY_X.readAxis();
        GO.BtnA.read();
        GO.BtnB.read();
        GO.BtnSelect.read();
        GO.BtnStart.read();

        // D-Pad
        joypad_->setUp(GO.JOY_Y.isAxisPressed() == 2);
        joypad_->setDown(GO.JOY_Y.isAxisPressed() == 1);
        joypad_->setLeft(GO.JOY_X.isAxisPressed() == 2);
        joypad_->setRight(GO.JOY_X.isAxisPressed() == 1);

        // Buttons
        joypad_->setA(GO.BtnA.isPressed());
        joypad_->setB(GO.BtnB.isPressed());
        joypad_->setStart(GO.BtnStart.isPressed());
        joypad_->setSelect(GO.BtnSelect.isPressed());

        // Keypad interrupt
        // ram_[0xFF0F] |= (1 << 4);
    }

    FastDisplay tft_;
    uint8_t *ram_;
    Joypad* joypad_;

    // Green shades in 16bit RGB565 format, Odroid has bytes swapped
    std::array<uint16_t, 4> colors_{{0xDAE7, 0x0E8E, 0x4A33, 0xC408}};
};

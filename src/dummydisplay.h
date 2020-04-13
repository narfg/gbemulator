#pragma once
#include "display.h"

class DummyDisplay : public Display
{
public:
    DummyDisplay() {}
    void init(uint16_t /* width */, uint16_t /* height */) override {}
    void close() override {}
    void update() override {}
    void drawPixel(uint16_t /* x */, uint16_t /* y */, uint8_t /* color */) override {}
};

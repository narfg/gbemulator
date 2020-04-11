#pragma once
#include <cstdint>

// This is an abstract wrapper around the actual display implementation
class Display
{
public:
    virtual ~Display() = default;
    virtual void init(uint16_t width, uint16_t height) = 0;
    virtual void close() = 0;
    virtual void update() = 0;
    virtual void drawPixel(uint16_t x, uint16_t y, uint8_t color) = 0;
};

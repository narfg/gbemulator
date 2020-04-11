#pragma once
#include <cstdint>

class Joypad
{
public:
    Joypad();
    void setDown(bool down);
    void setUp(bool down);
    void setLeft(bool down);
    void setRight(bool down);
    void setStart(bool down);
    void setSelect(bool down);
    void setB(bool down);
    void setA(bool down);

    uint8_t getButtons() const { return buttons_; }
    uint8_t getDirections() const { return directions_; }

private:
    uint8_t buttons_; // start, select, b, a
    uint8_t directions_; // down, up, left, right
};

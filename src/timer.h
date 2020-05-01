#pragma once
#include <cstdint>

class Timer
{
public:
    Timer(uint8_t* ram);
    void tick();
    void tick4();

private:
    uint8_t* ram_;
    uint8_t div_tick_;
    uint16_t tima_tick_;
};



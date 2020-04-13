#include "timer.h"

Timer::Timer(uint8_t* ram):
    ram_(ram),
    div_tick_(0),
    tima_tick_(0)
{

}

void Timer::tick()
{
    // Game Boy clock frequency is 4 * 1024 * 1024 = 4194304 Hz
    div_tick_++;
    // On overflow (happens at 16384 Hz)
    if (div_tick_ == 0x00) {
        // Increment DIV
        ram_[0xFF04]++;
    }

    // TIMA
    uint8_t timer_enabled = ram_[0xFF07] & 0x04;
    if (timer_enabled) {
        uint16_t divider = 0;
        switch (ram_[0xFF07] & 0x03) {
        case 0x00:
            divider = 1024;
            break;
        case 0x01:
            divider = 16;
            break;
        case 0x02:
            divider = 64;
            break;
        case 0x03:
            divider = 256;
            break;
        }
        tima_tick_++;
        if (tima_tick_ % divider == 0) {
            tima_tick_ = 0;
            ram_[0xFF05]++;
            // Overflow
            if (ram_[0xFF05] == 0x00) {
                // Set TIMA to TMA
                ram_[0xFF05] = ram_[0xFF06];
                // Request timer interrupt
                ram_[0xFF0F] |= (1 << 2);
            }
        }
    }
}

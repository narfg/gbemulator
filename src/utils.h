#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>

// #define PRINT_INSTR

static inline void printf_instr(const char *format, ...) {
    // Suppress warning if PRINT_INSTR is not defined
    (void)format;

#ifdef PRINT_INSTR
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
#endif
}

static inline uint16_t to16(uint8_t low, uint8_t high) {
    uint16_t val = 0;
    val |= (low);
    val |= (high << 8);
    return val;
}

static inline uint8_t getLow(uint16_t value) {
    uint8_t val = value & 0xFF;
    return val;
}

static inline uint8_t getHigh(uint16_t value) {
    uint8_t val = (value >> 8) & 0xFF;
    return val;
}

static inline uint8_t getBitN(uint8_t value, uint8_t n) {
    return (value >> n) & 1;
}

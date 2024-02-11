#pragma once
#include <cstdint>
#include "gbcomponent.h"

namespace TimerFrequency {
    enum TimerFrequency {
        DIV1024 = 0,
        DIV16 = 1,
        DIV64 = 2,
        DIV256 = 3
    };
}

constexpr uint16_t TIMA_BITS[] = {
    1 << 9, // Falling edge of bit 9
    1 << 3, // Falling edge of bit 3
    1 << 5, // Falling edge of bit 5
    1 << 7, // Falling edge of bit 7
};

constexpr uint16_t TIMA_MASKS[] = {
    0x3FF, // Falling edge of bit 9
    0xF, // Falling edge of bit 3
    0x3F, // Falling edge of bit 5
    0xFF, // Falling edge of bit 7
};

class Timer : GBComponent {
    private:
    // Upper 8 bits are the true div value
    uint16_t _div = 0;

    // Registers
    uint8_t _tima = 0;
    uint8_t _tma = 0;
    bool _enabled = true;
    TimerFrequency::TimerFrequency _clock_select = TimerFrequency::DIV1024;

    public:
    Timer(GBSystem& gb);

    void tick();
    void tick_tima();

    uint8_t read_io_register(uint16_t address);
    void write_io_register(uint16_t address, uint8_t value);

    uint8_t div() const {
        return _div >> 8;
    }

    uint16_t div_full() const {
        return _div;
    }

    bool enabled() const {
        return _enabled;
    }
};

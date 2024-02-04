#pragma once
#include <stdint.h>
#include "gbcomponent.h"

namespace TimerFrequency {
    enum TimerFrequency {
        DIV1024 = 0,
        DIV16 = 1,
        DIV64 = 2,
        DIV256 = 3
    };
}

constexpr uint16_t TIMER_FREQUENCY_MASKS[] = {
    0x3FF,
    0xF,
    0x3F,
    0xFF,
};

class Timer : GBComponent {
    protected:
    // Increments TIMA when (this & TIMER_FREQUENCY_MASK) == 0
    uint16_t _tima_counter = 0;

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

    uint8_t write_io_register(uint16_t address);
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

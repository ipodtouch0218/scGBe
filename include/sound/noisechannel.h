#pragma once
#include "soundchannel.h"

// Source: https://github.com/GhostSonic21/GhostBoy/blob/master/GhostBoy/NoiseChannel.h
constexpr uint8_t NOISE_TIMER_DIVISORS[8] = {
    0x08, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
};

class NoiseChannel : public SoundChannel {

    protected:
    uint16_t _lsfr = 0;
    bool _shifted_value = false;

    // NR43
    bool _7_bit_lsfr = false;
    uint8_t _clock_divider = 0;
    uint8_t _clock_shift = 0;

    public:
    NoiseChannel(uint16_t base_address);

    void tick();
    void trigger();

    int16_t current_sample();

    uint8_t get_register(uint16_t address);
    void set_register(uint16_t address, uint8_t value);
    void clear_registers();
};
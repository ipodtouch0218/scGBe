#pragma once
#include <stdint.h>
#include "soundchannel.h"

constexpr uint8_t PULSE_DUTY_CYCLES[4] = {
    0b01111111,
    0b01111110,
    0b00011110,
    0b00000001,
};

class PulseChannel : public SoundChannel {

    protected:
    bool _has_frequency_sweep = false;
    bool _frequency_sweep_enabled = false;
    uint8_t _frequency_sweep_pace = 0;
    bool _frequency_sweep_downwards = false;
    uint8_t _frequency_sweep_step = 0;

    uint8_t _duty_cycle = 0;
    uint8_t _duty_cycle_index = 0;
    uint16_t _frequency_sweep_timer = 0;

    public:
    PulseChannel(uint16_t base_address, bool has_frequency_sweep);

    void tick();
    void div_tick(uint8_t div_apu);

    int16_t current_sample();

    void trigger();

    uint8_t get_register(uint16_t address);
    void set_register(uint16_t address, uint8_t value);

    uint16_t calculate_sweep_period();
};
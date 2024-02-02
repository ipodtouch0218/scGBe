#pragma once
#include <stdint.h>
#include "soundchannel.h"

class WaveChannel : public SoundChannel {

    protected:
    uint8_t _wave_samples[32];
    uint8_t _wave_index = 0;
    uint8_t _current_wave_sample;

    public:
    WaveChannel(uint16_t base_address);

    void tick();
    void div_tick(uint8_t div_apu);

    int16_t current_sample();

    void trigger();

    uint8_t get_register(uint16_t address);
    void set_register(uint16_t address, uint8_t value);
    void clear_registers();
};
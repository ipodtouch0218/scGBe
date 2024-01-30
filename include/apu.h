#pragma once
#include "registers.h"

constexpr uint8_t PULSE_DUTY_CYCLES[4] = {
    0b01111111,
    0b01111110,
    0b00011110,
    0b00000001,
};

class GBSystem;

class APU {

    public:
    GBSystem* gb;
    uint8_t div_apu = 0;

    uint16_t ch1_timer = 0;
    uint8_t ch1_pulse_index = 0;
    uint8_t ch1_length = 0;
    uint8_t ch1_volume = 0;
    uint8_t ch1_envelope_timer = 0;
    uint8_t ch1_sweep_timer = 0;

    uint16_t ch2_timer = 0;
    uint8_t ch2_pulse_index = 0;
    uint8_t ch2_length = 0;
    uint8_t ch2_volume = 0;
    uint8_t ch2_envelope_timer = 0;

    uint16_t ch3_timer = 0;
    uint16_t ch3_lsfr = 0;
    bool ch3_shifted_value = false;
    uint8_t ch3_length = 0;
    uint8_t ch3_volume = 0;
    uint8_t ch3_envelope_timer = 0;

    APU(GBSystem* gb);

    void tick();

    int16_t current_sample();

    void handle_pulse_channel_1(bool div_apu_updated);
    int16_t sample_pulse_channel_1();

    void handle_pulse_channel_2(bool div_apu_updated);
    int16_t sample_pulse_channel_2();

    void handle_noise_channel(bool div_apu_updated);
    int16_t sample_noise_channel();

};
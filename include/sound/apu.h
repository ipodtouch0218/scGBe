#pragma once
#include "registers.h"
#include "pulsechannel.h"
#include "noisechannel.h"
#include "wavechannel.h"
#include "gbcomponent.h"

class APU : public GBComponent {

    protected:
    bool _enabled = true;
    bool _channel_panning[2][4] = {false, false, false, false, false, false, false, false};
    uint8_t _left_volume = 8;
    uint8_t _right_volume = 8;

    public:
    uint8_t div_apu = 0;

    PulseChannel pulse_channel_1 = PulseChannel(SND_P1_ORIGIN, true);
    PulseChannel pulse_channel_2 = PulseChannel(SND_P2_ORIGIN, false);
    WaveChannel wave_channel_3 = WaveChannel(SND_WV_ORIGIN);
    NoiseChannel noise_channel_4 = NoiseChannel(SND_NS_ORIGIN);

    APU(GBSystem& gb);

    void tick();
    void div_tick();

    int16_t current_sample(bool right_channel);

    uint8_t get_register(uint16_t address);
    void set_register(uint16_t address, uint8_t value);
};
#pragma once
#include "gbcomponent.h"
#include "noisechannel.h"
#include "pulsechannel.h"
#include "registers.h"
#include "wavechannel.h"

class APU : public GBComponent {

    protected:
    bool _enabled = true;
    bool _channel_panning[2][4] = {true, true, true, true, true, true, true, true};
    bool _vin_left = true;
    uint8_t _left_volume = 8;
    bool _vin_right = true;
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

    uint8_t read_io_register(uint16_t address);
    void write_io_register(uint16_t address, uint8_t value);

};
#include "apu.h"
#include "gbsystem.h"
#include "utils.h"

#include <iostream>

APU::APU(GBSystem& gb) :
    GBComponent::GBComponent(gb)
{
    gb.add_register_callbacks_range(this, 0xFF10, 0xFF27); // Normal Registers
    gb.add_register_callbacks_range(this, SND_WV_TABLE, SND_WV_TABLE + 16); // Wave Table
    gb.add_register_callbacks(this, {NR50, NR51, NR52}); // Global Registers
}

void APU::tick() {
    if (!_enabled) {
        return;
    }

    // Check for DIV_APU updates
    uint16_t div_mask = gb.cgb_mode() ? 0x3FFF : 0x1FFF;
    if ((gb.timer().div_full() & div_mask) == 0) {
        div_tick();
    }

    pulse_channel_1.tick();
    pulse_channel_2.tick();
    wave_channel_3.tick();
    noise_channel_4.tick();
}

void APU::div_tick() {
    div_apu++;
    pulse_channel_1.div_tick(div_apu);
    pulse_channel_2.div_tick(div_apu);
    wave_channel_3.div_tick(div_apu);
    noise_channel_4.div_tick(div_apu);
}

int16_t APU::current_sample(bool right_channel) {
    if (!_enabled) {
        return 0;
    }

    int16_t sum = 0;
    sum += _channel_panning[right_channel][0] ? pulse_channel_1.current_sample() : 0;
    sum += _channel_panning[right_channel][1] ? pulse_channel_2.current_sample() : 0;
    sum += _channel_panning[right_channel][2] ? wave_channel_3.current_sample()  : 0;
    sum += _channel_panning[right_channel][3] ? noise_channel_4.current_sample() : 0;

    float volume = right_channel ? _right_volume : _left_volume;
    sum = (int16_t) (sum * ((volume + 1) / 8));

    return sum;
}

uint8_t APU::get_register(uint16_t address) {
    if (address != NR52 && !_enabled) {
        return 0xFF;
    }

    if (address >= SND_P1_ORIGIN && address < SND_P1_ORIGIN + 5) {
        return pulse_channel_1.get_register(address);
    } else if (address >= SND_P2_ORIGIN && address < SND_P2_ORIGIN + 5) {
        return pulse_channel_2.get_register(address);
    } else if ((address >= SND_WV_ORIGIN && address < SND_WV_ORIGIN + 5) || (address >= SND_WV_TABLE && address < SND_WV_TABLE + 16)) {
        wave_channel_3.get_register(address);
    } else if (address >= SND_NS_ORIGIN && address < SND_NS_ORIGIN + 5) {
        noise_channel_4.get_register(address);
    }

    switch (address) {
    case NR50: {
        uint8_t value = 0;
        value |= _right_volume;
        value = utils::set_bit_value(value, 3, _vin_right);
        value |= (_left_volume << 4);
        value = utils::set_bit_value(value, 7, _vin_left);
        return value;
    }
    case NR51: {
        uint8_t value = 0xFF;
        value = utils::set_bit_value(value, 7, _channel_panning[0][3]);
        value = utils::set_bit_value(value, 6, _channel_panning[0][2]);
        value = utils::set_bit_value(value, 5, _channel_panning[0][1]);
        value = utils::set_bit_value(value, 4, _channel_panning[0][0]);
        value = utils::set_bit_value(value, 3, _channel_panning[1][3]);
        value = utils::set_bit_value(value, 2, _channel_panning[1][2]);
        value = utils::set_bit_value(value, 1, _channel_panning[1][1]);
        value = utils::set_bit_value(value, 0, _channel_panning[1][0]);
        return value;
    }
    case NR52: {
        uint8_t value = 0xFF;
        value = utils::set_bit_value(value, 0, pulse_channel_1.active());
        value = utils::set_bit_value(value, 1, pulse_channel_2.active());
        value = utils::set_bit_value(value, 2, wave_channel_3.active());
        value = utils::set_bit_value(value, 3, noise_channel_4.active());
        value = utils::set_bit_value(value, 7, _enabled);

        if (!_enabled) {
            pulse_channel_1.clear_registers();
            pulse_channel_2.clear_registers();
            wave_channel_3.clear_registers();
            noise_channel_4.clear_registers();
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 4; j++) {
                    _channel_panning[i][j] = true;
                }
            }
            _right_volume = 0b111;
            _vin_right = true;
            _left_volume = 0b111;
            _vin_left = true;
        }
        return value;
    }
    }

    return 0xFF;
}

void APU::set_register(uint16_t address, uint8_t value) {
    if (address != NR52 && !_enabled) {
        return;
    }

    if (address >= SND_P1_ORIGIN && address < SND_P1_ORIGIN + 5) {
        pulse_channel_1.set_register(address, value);
    } else if (address >= SND_P2_ORIGIN && address < SND_P2_ORIGIN + 5) {
        pulse_channel_2.set_register(address, value);
    } else if ((address >= SND_WV_ORIGIN && address < SND_WV_ORIGIN + 5) || (address >= SND_WV_TABLE && address < SND_WV_TABLE + 16)) {
        wave_channel_3.set_register(address, value);
    } else if (address >= SND_NS_ORIGIN && address < SND_NS_ORIGIN + 5) {
        noise_channel_4.set_register(address, value);
    }

    switch (address) {
    case NR50: {
        // TODO: vin stuff
        _right_volume = value & 0b111;
        _vin_right = utils::get_bit_value(value, 3);
        _left_volume = (value >> 4) & 0b111;
        _vin_left = utils::get_bit_value(value, 7);
        break;
    }
    case NR51: {
        _channel_panning[0][3] = utils::get_bit_value(value, 7);
        _channel_panning[0][2] = utils::get_bit_value(value, 6);
        _channel_panning[0][1] = utils::get_bit_value(value, 5);
        _channel_panning[0][0] = utils::get_bit_value(value, 4);
        _channel_panning[1][3] = utils::get_bit_value(value, 3);
        _channel_panning[1][2] = utils::get_bit_value(value, 2);
        _channel_panning[1][1] = utils::get_bit_value(value, 1);
        _channel_panning[1][0] = utils::get_bit_value(value, 0);
        break;
    }
    case NR52: {
        _enabled = utils::get_bit_value(value, 7);
        break;
    }
    }
}
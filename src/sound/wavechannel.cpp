#include "registers.h"
#include "wavechannel.h"
#include "utils.h"

#include <iostream>

WaveChannel::WaveChannel(uint16_t base_address) :
    SoundChannel::SoundChannel(base_address)
{

}

void WaveChannel::tick() {
    if (!_active || !_dac_enabled) {
        return;
    }

    if (_timer-- == 0) {
        _wave_index++;
        _wave_index %= 32;
        _timer = (2048 - _period) * 2;
    }
}

void WaveChannel::div_tick(uint8_t div_apu) {
    if (!_active || !_dac_enabled) {
        return;
    }

    if (_length_enable && (div_apu % 2) == 0) {
        // Sound length
        if (--_length_timer == 0) {
            _active = false;
            return;
        }
    }
}

int16_t WaveChannel::current_sample() {
    if (!_active || !_dac_enabled) {
        return 0;
    }

    float sample = _wave_samples[_wave_index] / 15.0f;
    sample -= 0.5f;
    sample *= (_volume / 15.0f);

    return (int16_t) (sample * 32768 / 2);
}

void WaveChannel::trigger() {
    if (_active || !_dac_enabled) {
        return;
    }

    _active = true;
    _timer = 0;
    _volume = _initial_volume;
    _wave_index = 0;
}

uint8_t WaveChannel::get_register(uint16_t address) {
    if (address >= SND_WV_TABLE && address < SND_WV_TABLE + 16) {
        uint8_t index = (address - SND_WV_TABLE) * 2;
        return ((_wave_samples[index] & 0xF) << 4) | (_wave_samples[index + 1] & 0xF);
    }

    switch (address - _base_address) {
    case 0: { // NRx0: DAC Enable
        return utils::set_bit_value(0, 7, _dac_enabled);
    }
    case 1: { // NRx1: Initial Length Timer (write-only)
        return 0xFF;
    }
    case 2: { // NRx2: Initial Volume
        uint8_t value;
        switch (_initial_volume) {
        case 0x0: return 0 << 5;
        case 0xF: return 1 << 5;
        case 0x8: return 2 << 5;
        case 0x4: return 3 << 5;
        }
    }
    case 3: { // NRx3: Period Low (write-only)
        return 0xFF;
    }
    case 4: { // NRx4: Length Enable
        return utils::set_bit_value(0, 6, _length_enable);
    }
    default: return 0xFF;
    }
}

void WaveChannel::set_register(uint16_t address, uint8_t value) {
    if (address >= SND_WV_TABLE && address < SND_WV_TABLE + 16) {
        uint8_t index = (address - SND_WV_TABLE) * 2;
        _wave_samples[index] = (value >> 4) & 0xF;
        _wave_samples[index + 1] = value & 0xF;
        return;
    }

    switch (address - _base_address) {
    case 0: { // NRx0: DAC enable
        _dac_enabled = utils::get_bit_value(value, 7);
        _active &= _dac_enabled;
        break;
    }
    case 1: { // NRx1: Initial Length Timer
        _length_timer = 256 - value;
        break;
    }
    case 2: { // NRx2: Initial Volume
        switch ((value >> 5) & 0b11) {
        case 0: _initial_volume = 0x0; break;
        case 1: _initial_volume = 0xF; break;
        case 2: _initial_volume = 0x8; break;
        case 3: _initial_volume = 0x4; break;
        }
        _volume = _initial_volume;

        break;
    }
    case 3: { // NRx3: Period Low
        _period &= 0xFF00;
        _period |= value;
        break;
    }
    case 4: { // NRx4: Period High, Length Enable, Trigger
        _period &= 0xFF;
        _period |= ((uint16_t) (value & 0b111)) << 8;
        _length_enable = utils::get_bit_value(value, 6);

        if (utils::get_bit_value(value, 7)) {
            trigger();
        }
        break;
    }
    }
}
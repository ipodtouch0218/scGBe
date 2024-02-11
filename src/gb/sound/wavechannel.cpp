#include "registers.h"
#include "wavechannel.h"
#include "utils.h"

WaveChannel::WaveChannel(uint16_t base_address) :
    SoundChannel::SoundChannel(base_address)
{
    // Initialize wave table with 00FF00FF00FF...
    for (size_t i = 0; i < 32; i++) {
        _wave_samples[i] = 0xF * ((i % 4) >= 2);
    }
}

void WaveChannel::tick() {
    if (!_active || !_dac_enabled) {
        return;
    }

    _wave_sample_read = false;
    if (_timer-- == 0) {
        _wave_index++;
        _wave_index %= 32;
        _current_wave_sample = _wave_samples[_wave_index];
        _wave_sample_read = true;
        _timer = (2048 - _period) * 2;
    }
}

void WaveChannel::div_tick(uint8_t div_apu) {
    if (!_dac_enabled) {
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

    float sample = _current_wave_sample / 15.0f;
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

uint8_t WaveChannel::read_io_register(uint16_t address) {
    if (address >= SND_WV_TABLE && address < SND_WV_TABLE + 16) {
        // !_wave_sample_read = edge case, when the APU reads, that value is accessible.
        if (_wave_sample_read && (_wave_index % 2 == 0)) {
            return ((_wave_samples[_wave_index] & 0xF) << 4) | (_wave_samples[_wave_index + 1] & 0xF);
        } else if (active()) {
            return 0xFF;
        } else {
            uint8_t index = (address - SND_WV_TABLE) * 2;
            return ((_wave_samples[index] & 0xF) << 4) | (_wave_samples[index + 1] & 0xF);
        }
    }

    switch (address - _base_address) {
    case 0: { // NRx0: DAC Enable
        return utils::set_bit_value(0xFF, 7, _dac_enabled);
    }
    case 1: { // NRx1: Initial Length Timer (write-only)
        return 0xFF;
    }
    case 2: { // NRx2: Initial Volume
        uint8_t value = 0b10011111;
        switch (_initial_volume) {
        case 0x0: value |= (0 << 5); break;
        case 0xF: value |= (1 << 5); break;
        case 0x8: value |= (2 << 5); break;
        case 0x4: value |= (3 << 5); break;
        }
        return value;
    }
    case 3: { // NRx3: Period Low (write-only)
        return 0xFF;
    }
    case 4: { // NRx4: Length Enable
        return utils::set_bit_value(0xFF, 6, _length_enable);
    }
    default: return 0xFF;
    }
}

void WaveChannel::write_io_register(uint16_t address, uint8_t value) {
    if (address >= SND_WV_TABLE && address < SND_WV_TABLE + 16) {
        uint8_t index = (address - SND_WV_TABLE) * 2;
        _wave_samples[index] = (value >> 4) & 0xF;
        _wave_samples[index + 1] = value & 0xF;
        return;
    }

    switch (address - _base_address) {
    case 0: { // NRx0: DAC enable
        _dac_enabled = utils::get_bit_value(value, 7);
        if (!_dac_enabled) {
            _active = false;
        }
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
        _period &= 0x00FF;
        _period |= ((uint16_t) (value & 0b111)) << 8;
        _length_enable = utils::get_bit_value(value, 6);

        if (utils::get_bit_value(value, 7)) {
            trigger();
        }
        break;
    }
    }
}

void WaveChannel::clear_registers() {
    _dac_enabled = false;

    _length_timer = 255;

    _volume = _initial_volume = 0;

    _period = 0;
    _length_enable = false;

    _active = false;
}
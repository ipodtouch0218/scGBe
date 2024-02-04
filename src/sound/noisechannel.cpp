#include "noisechannel.h"
#include "utils.h"

#include <iostream>

NoiseChannel::NoiseChannel(uint16_t base_address) :
    SoundChannel::SoundChannel(base_address)
{

}

void NoiseChannel::tick() {
    if (!_active || !_dac_enabled) {
        return;
    }

    if (_timer-- == 0) {
        bool inserted_value = !((_lsfr & 1) ^ ((_lsfr >> 1) & 1));
        _lsfr = utils::set_bit_value(_lsfr, 15, inserted_value);
        if (_7_bit_lsfr) {
            _lsfr = utils::set_bit_value(_lsfr, 7, inserted_value);
        }
        _lsfr >>= 1;
        _shifted_value = _lsfr & 1;

        _timer = NOISE_TIMER_DIVISORS[_clock_divider] << _clock_shift;
    }
}

void NoiseChannel::trigger() {
    if (!_dac_enabled) {
        return;
    }

    SoundChannel::trigger();
    _lsfr = 0;
}

int16_t NoiseChannel::current_sample() {
    if (!_active || !_dac_enabled) {
        return 0;
    }

    float sample = !_shifted_value;
    sample -= 0.5f;
    sample *= (_volume / 15.0f);

    return (int16_t) (sample * 32768 / 2);
}

uint8_t NoiseChannel::read_io_register(uint16_t address) {
    switch (address - _base_address) {
    case 0: { // NRx0: Unused
        return 0xFF;
    }
    case 1: { // NRx1: Initial length timer (write-only)
        return 0xFF;
    }
    case 2: { // NRx2: Initial Volume, Volume Sweep
        uint8_t value = 0;
        value |= _volume_sweep_pace & 0b111;
        value = utils::set_bit_value(value, 3, _volume_sweep_increments);
        value |= (_initial_volume & 0b1111) << 4;
        return value;
    }
    case 3: { // NRx3: Clock Shift, LSFR Width, Clock Divider
        uint8_t value = 0;
        value |= _clock_divider & 0b111;
        value = utils::set_bit_value(value, 3, _7_bit_lsfr);
        value |= (_clock_shift & 0b1111) << 4;
        return value;
    }
    case 4: { // NRx4: Length Enable
        return utils::set_bit_value(0, 6, _length_enable);
    }
    default: return 0xFF;
    }
}

void NoiseChannel::write_io_register(uint16_t address, uint8_t value) {
    switch (address - _base_address) {
    case 0: { // NRx0: Unused
        break;
    }
    case 1: { // NRx1: Initial Length Timer
        _length_timer = 64 - (value & 0b111111);
        break;
    }
    case 2: { // NRx2: Initial Volume, Volume Sweep
        _volume_sweep_pace = value & 0b111;
        _volume_sweep_timer = _volume_sweep_pace;
        _volume_sweep_increments = utils::get_bit_value(value, 3);
        _initial_volume = (value >> 4) & 0b1111;
        _volume = _initial_volume;
        _dac_enabled = value & 0b11111000;

        if (!_dac_enabled) {
            _active = false;
        }
        break;
    }
    case 3: { // NRx3: Clock Shift, LSFR Width, Clock Divider
        _clock_divider = value & 0b111;
        _7_bit_lsfr = utils::get_bit_value(value, 3);
        _clock_shift = (value >> 4) & 0b1111;
        break;
    }
    case 4: { // NRx4: Trigger, Length Enable
        _length_enable = utils::get_bit_value(value, 6);

        if (utils::get_bit_value(value, 7)) {
            trigger();
        }
        break;
    }
    }
}

void NoiseChannel::clear_registers() {
    _length_timer = 63;
    _volume_sweep_pace = 0b111;
    _volume_sweep_timer = 0b111;
    _volume_sweep_increments = true;
    _initial_volume = 0b1111;
    _volume = 0b1111;
    _clock_divider = 0b111;
    _7_bit_lsfr = true;
    _clock_shift = 0b1111;
    _length_enable = true;
}
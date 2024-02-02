#include "pulsechannel.h"
#include "utils.h"

#include <iostream>

PulseChannel::PulseChannel(uint16_t base_address, bool has_frequency_sweep) :
    SoundChannel::SoundChannel(base_address),
    _has_frequency_sweep(has_frequency_sweep)
{

}

void PulseChannel::tick() {
    if (!_active || !_dac_enabled) {
        return;
    }

    if (_timer-- == 0) {
        _duty_cycle_index <<= 1;
        if (_duty_cycle_index == 0) {
            _duty_cycle_index = 1;
        }

        _timer = (2048 - _period) * 4;
    }
}

uint16_t PulseChannel::calculate_sweep_period() {
    uint16_t offset = _period >> _frequency_sweep_step;
    uint16_t new_period = _period;
    if (_frequency_sweep_downwards) {
        new_period -= offset;
    } else {
        new_period += offset;

        if (new_period >= 0x800) {
            _active = false;
        }
    }

    return new_period;
}

void PulseChannel::div_tick(uint8_t div_apu) {
    SoundChannel::div_tick(div_apu);

    if (!_active || !_dac_enabled) {
        return;
    }

    if (_has_frequency_sweep && _frequency_sweep_enabled && (div_apu % 4) == 0) {
        // Frequency sweep
        if (_frequency_sweep_timer-- == 0) {
            _period = calculate_sweep_period();
            _frequency_sweep_timer = _frequency_sweep_pace;
            // _frequency_sweep_enabled = _frequency_sweep_pace != 0;
        }
    }
}

int16_t PulseChannel::current_sample() {
    if (!_active || !_dac_enabled) {
        return 0;
    }

    float sample = (PULSE_DUTY_CYCLES[_duty_cycle] & _duty_cycle_index) != 0;
    sample -= 0.5f;
    sample *= (_volume / 15.0f);

    return (int16_t) (sample * 32768 / 2);
}

void PulseChannel::trigger() {
    if (!_dac_enabled) {
        return;
    }

    SoundChannel::trigger();

    if (_has_frequency_sweep) {
        _frequency_sweep_timer = _frequency_sweep_pace;
        _frequency_sweep_enabled = _frequency_sweep_step;

        if (_frequency_sweep_step) {
            calculate_sweep_period();
        }
    }
}

uint8_t PulseChannel::get_register(uint16_t address) {
    switch (address - _base_address) {
    case 0: { // NRx0: Frequency Sweep
        if (!_has_frequency_sweep) {
            return 0xFF;
        }

        uint8_t value = 0b10000000;
        value |= _frequency_sweep_step & 0b111;
        value = utils::set_bit_value(value, 3, _frequency_sweep_downwards);
        value |= (_frequency_sweep_pace & 0b111) << 4;
        return value;
    }
    case 1: { // NRx1: Duty Cycle
        return 0b00111111 | (_duty_cycle << 6);
    }
    case 2: { // NRx2: Initial Volume, Volume Sweep
        uint8_t value = 0;
        value |= _volume_sweep_pace & 0b111;
        value = utils::set_bit_value(value, 3, _volume_sweep_increments);
        value |= (_initial_volume & 0b1111) << 4;
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

void PulseChannel::set_register(uint16_t address, uint8_t value) {
    switch (address - _base_address) {
    case 0: { // NRx0: Frequency Sweep
        if (!_has_frequency_sweep) {
            break;
        }

        _frequency_sweep_step = value & 0b111;
        _frequency_sweep_downwards = utils::get_bit_value(value, 3);
        _frequency_sweep_pace = (value >> 4) & 0b111;
        if (_frequency_sweep_pace == 0) {
            _frequency_sweep_pace = 8;
        }
        _frequency_sweep_enabled = _frequency_sweep_step;
        break;
    }
    case 1: { // NRx1: Duty Cycle, Initial Length Timer
        _length_timer = 64 - (value & 0b111111);
        _duty_cycle = (value >> 6) & 0b11;
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

void PulseChannel::clear_registers() {
    _length_timer = 63;
    _frequency_sweep_step = 0b111;
    _frequency_sweep_downwards = true;
    _frequency_sweep_pace = 0b111;
    _duty_cycle = 0b11;
    _volume_sweep_pace = 0b111;
    _volume_sweep_increments = true;
    _volume = 0b1111;
    _initial_volume = 0b1111;
    _period = 0b111111111111;
    _length_enable = true;
}
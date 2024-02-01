#include "soundchannel.h"

#include <iostream>

void SoundChannel::div_tick(uint8_t div_apu) {
    if (!_active || !_dac_enabled) {
        return;
    }

    if (_volume_sweep_pace > 0 && (div_apu % 8) == 0) {
        // Volume (Envelope) Sweep
        if (--_volume_sweep_timer == 0) {
            if (_volume_sweep_increments && _volume < 15) {
                _volume++;
            } else if (!_volume_sweep_increments && _volume > 0) {
                _volume--;
            }

            _volume_sweep_timer = _volume_sweep_pace;
        }
    }

    if (_length_enable && (div_apu % 2) == 0) {
        // Sound length
        if (--_length_timer == 0) {
            _active = false;
            return;
        }
    }
}

void SoundChannel::trigger() {
    if (!_dac_enabled) {
        return;
    }

    _active = true;
    _timer = 0;

    if (_length_timer == 0) {
        _length_timer = 64;
    }

    _volume = _initial_volume;
    _volume_sweep_timer = _volume_sweep_pace;
}
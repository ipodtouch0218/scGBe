#pragma once
#include <stdint.h>

class SoundChannel {

    protected:
    // Register settings
    uint16_t _base_address = 0;
    uint8_t _initial_length_timer = 0;
    uint8_t _initial_volume = 0;
    bool _volume_sweep_increments = false;
    uint8_t _volume_sweep_pace = 0;
    bool _length_enable = false;
    uint16_t _period = 0;
    bool _dac_enabled = false;

    // Active timers
    bool _active = false;
    uint8_t _volume = 0;
    uint16_t _timer = 0;
    uint8_t _volume_sweep_timer = 0;
    uint8_t _length_timer = 0;

    public:
    SoundChannel(uint16_t base_address) :
        _base_address(base_address)
    {

    }

    virtual void tick() = 0;
    virtual void div_tick(uint8_t div_apu);

    virtual int16_t current_sample() = 0;

    virtual void trigger();

    virtual uint8_t read_io_register(uint16_t address) = 0;
    virtual void write_io_register(uint16_t address, uint8_t value) = 0;
    virtual void clear_registers() = 0;

    bool active() const {
        return _dac_enabled && _active;
    }
};
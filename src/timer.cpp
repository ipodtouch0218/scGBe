#include "timer.h"
#include "gbsystem.h"
#include "utils.h"

#include <iostream>

Timer::Timer(GBSystem& gb_param)
    : GBComponent::GBComponent(gb_param)
{
    // Register callbacks:
    gb.add_register_callbacks(this, {DIV, TIMA, TMA, TAC});
}

void Timer::tick() {
    // No need to check every 256 cycles,
    // This is a 16 bit value and the upper 8 bits are the true DIV.
    _div++;

    uint16_t mask = TIMA_MASKS[_clock_select];
    if ((_div & mask) == 0) {
        // Increment TIMA
        tick_tima();
    }
}

void Timer::tick_tima() {
    if (!enabled()) {
        return;
    }

    if (++_tima == 0) {
        // Overflowed, request a timer interrupt.
        gb.request_interrupt(Interrupts::Timer);
    }
}

uint8_t Timer::read_io_register(uint16_t address) {
    switch (address) {
    case DIV: return div();
    case TIMA: return _tima;
    case TMA: return _tma;
    case TAC: {
        uint8_t value = 0b1111000;
        value = utils::set_bit_value(value, 2, _enabled);
        value |= ((uint8_t) _clock_select) & 0b11;
        return value;
    }
    default: return 0xFF;
    }
}

void Timer::write_io_register(uint16_t address, uint8_t value) {
    switch (address) {
    case DIV: {
        // Timer bug: if selected bit of div is set (for TIMA), increment TIMA
        uint16_t bit = TIMA_BITS[_clock_select];
        if ((_div & bit) != 0) {
            // Increment TIMA
            tick_tima();
        }

        _div = 0;
        break;
    }
    case TIMA: {
        _tima = value;
        break;
    }
    case TMA: {
        _tma = value;
        break;
    }
    case TAC: {
        // Timer bug: if _enabled is now set when it wasnt, flash if mask matches.
        bool was_enabled = _enabled;
        _clock_select = (TimerFrequency::TimerFrequency) (value & 0b11);
        _enabled = utils::get_bit_value(value, 2);

        uint16_t bit = TIMA_BITS[_clock_select];
        if (!was_enabled && _enabled && (_div & bit) != 0) {
            // Increment TIMA
            tick_tima();
        }

        break;
    }
    }
}
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

    // TODO: implement https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
    if (enabled()) {
        // Timer enabled
        uint16_t mask = TIMER_FREQUENCY_MASKS[_clock_select];
        if ((++_tima_counter & mask) == 0) {
            // Increment TIMA
            if (++_tima == 0) {
                // Overflowed, request a timer interrupt.
                gb.request_interrupt(Interrupts::Timer);
            }
        }
    }
}

uint8_t Timer::write_io_register(uint16_t address) {
    switch (address) {
    case DIV: return div();
    case TIMA: return _tima;
    case TMA: return _tma;
    case TAC: return (_enabled << 2) | ((uint8_t) _clock_select);
    default: return 0xFF;
    }
}

void Timer::write_io_register(uint16_t address, uint8_t value) {
    switch (address) {
    case DIV: {
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
        _clock_select = (TimerFrequency::TimerFrequency) (value & 0b11);
        _enabled = utils::get_bit_value(value, 2);
        break;
    }
    }
}
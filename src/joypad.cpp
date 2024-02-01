#include "joypad.h"
#include "gbsystem.h"
#include "utils.h"

#include <iostream>

uint8_t inputs = 0xFF;

Joypad::Joypad(GBSystem& gb) :
    GBComponent::GBComponent(gb)
{
    gb.add_register_callbacks(this, {JOYP});
}

void Joypad::tick() {
    uint8_t new_buttons = 0xF;
    if (_selected_buttons) {
        new_buttons = (inputs >> 4) & 0xF;
    } else if (_selected_dpad) {
        new_buttons = inputs & 0xF;
    }

    if ((_current_register & 0xF) > new_buttons) {
        // Interrupt!
        gb.request_interrupt(Interrupts::Joypad);
    }
}

uint8_t Joypad::get_register(uint16_t address) {
    if (address != JOYP) {
        return 0xFF;
    }
    return _current_register;
}

void Joypad::set_register(uint16_t address, uint8_t value) {
    if (address != JOYP) {
        return;
    }

    _current_register = (value & 0xF0) | 0xF;
    _selected_buttons = false;
    _selected_dpad = false;

    bool dpad_enabled = utils::get_bit_value(_current_register, 4) == 0;
    bool buttons_enabled = utils::get_bit_value(_current_register, 5) == 0;

    if (dpad_enabled == buttons_enabled) {
        return;
    }

    _current_register &= 0xF0;
    if (dpad_enabled) {
        _current_register |= inputs & 0xF;
        _selected_dpad = true;
    } else if (buttons_enabled) {
        _current_register |= (inputs >> 4) & 0xF;
        _selected_buttons = true;
    }
}
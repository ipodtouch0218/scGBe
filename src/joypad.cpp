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
    update_joypad();
}

uint8_t Joypad::read_io_register(uint16_t address) {
    switch (address) {
    case JOYP: return 0b11000000 | _current_register;
    default: return 0xFF;
    }
}

void Joypad::write_io_register(uint16_t address, uint8_t value) {
    if (address != JOYP) {
        return;
    }

    _current_register = (value & 0xF0) | 0xF;
    _selected_dpad = utils::get_bit_value(_current_register, 4) == 0;
    _selected_buttons = utils::get_bit_value(_current_register, 5) == 0;

    update_joypad();
}

void Joypad::update_joypad() {
    uint8_t old_register = _current_register;

    _current_register |= 0x0F;
    if (_selected_dpad) {
        _current_register &= inputs & 0xF;
    }
    if (_selected_buttons) {
        _current_register &= (inputs >> 4) & 0xF;
    }

    bool any_button_pressed = false;
    any_button_pressed |= (utils::get_bit_value(old_register, 0) - utils::get_bit_value(_current_register, 0)) == 1;
    any_button_pressed |= (utils::get_bit_value(old_register, 1) - utils::get_bit_value(_current_register, 1)) == 1;
    any_button_pressed |= (utils::get_bit_value(old_register, 2) - utils::get_bit_value(_current_register, 2)) == 1;
    any_button_pressed |= (utils::get_bit_value(old_register, 3) - utils::get_bit_value(_current_register, 3)) == 1;

    if (any_button_pressed) {
        // Interrupt!
        gb.request_interrupt(Interrupts::Joypad);
    }
}
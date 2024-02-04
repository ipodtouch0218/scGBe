#pragma once
#include "gbcomponent.h"

class Joypad : public GBComponent {

    protected:
    uint8_t _current_register = false;
    bool _selected_dpad = false;
    bool _selected_buttons = false;

    public:
    Joypad(GBSystem& gb);

    void tick();

    uint8_t read_io_register(uint16_t address);
    void write_io_register(uint16_t address, uint8_t value);

};
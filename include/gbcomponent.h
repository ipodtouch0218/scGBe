#pragma once
#include "registers.h"

class GBSystem;

class GBComponent {
    protected:
    GBSystem& gb;

    public:
    GBComponent(GBSystem& gb_param) :
        gb(gb_param)
    {

    }

    virtual void tick() = 0;

    virtual uint8_t get_register(uint16_t address) { return 0xFF; }
    virtual void set_register(uint16_t address, uint8_t value) {}
};
#pragma once
#include "gbcomponent.h"

class DMAController : public GBComponent {

    protected:
    uint8_t _counter = -1;
    uint16_t _source_addr_msb = 0;

    public:
    DMAController(GBSystem& gb);

    void tick();

    uint8_t get_register(uint16_t address);
    void set_register(uint16_t address, uint8_t value);

    bool active() const {
        return _counter < 160;
    }
};
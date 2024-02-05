#pragma once
#include "gbcomponent.h"

class DMAController : public GBComponent {

    protected:
    uint8_t _counter = -1;
    uint16_t _source_addr_msb; // purposefully not initialized

    public:
    DMAController(GBSystem& gb);

    void tick();

    uint8_t read_io_register(uint16_t address);
    void write_io_register(uint16_t address, uint8_t value);

    bool active() const {
        return _counter < 160;
    }

    uint16_t current_address() const {
        return _source_addr_msb | _counter;
    }
};
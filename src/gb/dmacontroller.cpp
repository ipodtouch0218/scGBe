#include "dmacontroller.h"
#include <iostream>
#include "gbsystem.h"

DMAController::DMAController(GBSystem& gb) :
    GBComponent::GBComponent(gb)
{
    gb.add_register_callbacks(this, {DMA});
}

void DMAController::tick() {
    if (_counter < 160) {
        uint16_t source_addr = _source_addr_msb | _counter;
        uint16_t dest_addr = 0xFE00 | _counter;

        gb.write_address(dest_addr, gb.read_address(source_addr, true), true);
        _counter++;
    }
}

uint8_t DMAController::read_io_register(uint16_t address) {
    switch (address) {
    case DMA: {
        // Source: https://gekkio.fi/files/gb-docs/gbctr.pdf
        return _source_addr_msb;
    }
    default: return 0xFF;
    }
}

void DMAController::write_io_register(uint16_t address, uint8_t value) {
    switch (address) {
    case DMA: {
        // if (_counter < 160) {
        //     break;
        // }

        _source_addr_msb = ((uint16_t) value) << 8;
        _counter = 0;
        break;
    }
    }
}
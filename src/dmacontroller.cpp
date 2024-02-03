#include "dmacontroller.h"
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

        gb.address_space[dest_addr] = gb.address_space[source_addr];
        _counter++;
    }
}

uint8_t DMAController::get_register(uint16_t address) {
    switch (address) {
    case DMA: {
        // Source: https://gekkio.fi/files/gb-docs/gbctr.pdf
        return _source_addr_msb;
    }
    default: return 0xFF;
    }
}

void DMAController::set_register(uint16_t address, uint8_t value) {
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
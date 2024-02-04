#pragma once
#include <initializer_list>
#include <stdint.h>
#include <fstream>
#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "dmacontroller.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"

constexpr uint16_t RST_VECTORS = 0x0000;
constexpr uint16_t INTERRUPT_VECTORS = 0x0040;
constexpr uint16_t DMA_DEST = 0xFE00;

class GBSystem {

    protected:
    Cartridge _cartridge = Cartridge(*this);
    CPU _cpu = CPU(*this);
    PPU _ppu = PPU(*this);
    APU _apu = APU(*this);
    Timer _timer = Timer(*this);
    DMAController _dma_controller = DMAController(*this);
    Joypad _joypad = Joypad(*this);
    bool _cgb = false;

    uint8_t _wram[WRAM_SIZE * WRAM_BANKS]; // Only 2 banks in DMG mode
    uint8_t _hram[HRAM_SIZE];

    public:
    uint32_t clock_speed = 4194304;
    uint64_t cycles = 0;
    uint64_t frame_cycles = 0;
    uint8_t timer_cycles = 0;
    uint64_t frame_number = 0;

    GBComponent* register_handlers[0x80];

    GBSystem(bool cgb);

    bool tick();
    void reset();

    uint8_t read_address(uint16_t addr, bool internal = false);
    void write_address(uint16_t addr, uint8_t value, bool internal = false);

    void add_register_callbacks(GBComponent* component, std::initializer_list<uint16_t> addresses);
    void add_register_callbacks_range(GBComponent* component, uint16_t address_start, uint16_t address_end_exclusive);
    void request_interrupt(Interrupts::Interrupts interrupt);

    CPU& cpu() {
        return _cpu;
    }

    PPU& ppu() {
        return _ppu;
    }

    APU& apu() {
        return _apu;
    }

    Timer& timer() {
        return _timer;
    }

    DMAController& dma() {
        return _dma_controller;
    }

    Cartridge& cartridge() {
        return _cartridge;
    }

    bool cgb_mode() const {
        return _cgb;
    }
};
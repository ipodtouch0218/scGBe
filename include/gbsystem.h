#pragma once
#include <stdint.h>
#include "cpu.h"
#include "ppu.h"

constexpr uint16_t RST_VECTORS = 0x0000;
constexpr uint16_t INTERRUPT_VECTORS = 0x0040;

constexpr uint8_t INT_VBLANK = 0;
constexpr uint8_t INT_STAT = 1;
constexpr uint8_t INT_TIMER = 2;
constexpr uint8_t INT_SERIAL = 3;
constexpr uint8_t INT_JOYPAD = 4;

constexpr uint16_t DMA_DEST = 0xFE00;


class GBSystem {

    protected:
    CPU _cpu = CPU(this);
    PPU _lcd = PPU(this);

    public:
    uint32_t clock_speed = 4194304;
    uint32_t cycles = 0;
    uint8_t timer_cycles = 0;
    uint8_t dma_counter = 0;

    uint8_t address_space[0xFFFF];


    GBSystem(uint32_t clock_speed);

    void tick();

    void reset();

    uint8_t get_address_space_byte(uint16_t addr);
    void set_address_space_byte(uint16_t addr, uint8_t value);

    CPU& cpu() {
        return _cpu;
    }

    PPU& ppu() {
        return _lcd;
    }
};
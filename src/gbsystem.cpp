#include "gbsystem.h"

#include <iostream>
#include <cstring>
#include "registers.h"
#include "gbcomponent.h"

GBSystem::GBSystem(bool cgb) :
    _cgb(cgb)
{
    if (cgb) {
        clock_speed *= 2;
    }
}

void GBSystem::reset() {
    cycles = 0;
    timer_cycles = 0;

    cpu().reset();
    // memset(address_space + 0x8000, 0, 0xA000-0x8000); // Clear VRAM
    memset(address_space + 0x8000, 0, 0xFFFF-0x8000); // Clear RAM
}

bool GBSystem::tick() {

    // Timer
    timer().tick();

    // PPU
    ppu().tick();

    // APU
    apu().tick();

    // DMA
    dma().tick();

    // 4 clock cycles = 1 CPU cycle
    if (cycles % 4 == 0) {
        cpu().tick();
    }

    frame_cycles++;
    cycles++;
    if (frame_cycles == 70224) {
        frame_number++;
        frame_cycles = 0;
        return true;
    }
    return false;
}

uint8_t GBSystem::get_address_space_byte(uint16_t addr) {

    if (addr >= IO_REGISTERS_START && addr <= IO_REGISTERS_END) {
        // IO Registers
        GBComponent* handling_component = register_handlers[addr - IO_REGISTERS_START];
        if (handling_component) {
            return handling_component->get_register(addr);
        }
    }

    if (addr >= 0xE000 && addr <= 0xFDFF) {
        // Echo RAM
        addr -= 0x2000;
    }
    if (dma().active() && addr < 0xFF80) {
        // Cannot access non-HRAM during DMA
        return 0xFF;
    }
    if (ppu().enabled()) {
        // Cannot access vram during rendering mode 3
        if (addr >= 0x8000 && addr <= 0x9FFF && ppu().mode == LCDDrawMode::Drawing) {
            return 0xFF;
        }
        // Cannot access OAM during rendering modes 2 / 3
        if (addr >= 0xFE00 && addr <= 0xFE9F && (ppu().mode == LCDDrawMode::Drawing || ppu().mode == LCDDrawMode::OAM_Scan)) {
            return 0xFF;
        }
    }

    return address_space[addr];
}

void GBSystem::set_address_space_byte(uint16_t addr, uint8_t value) {

    if (addr >= IO_REGISTERS_START && addr <= IO_REGISTERS_END) {
        // IO Registers
        GBComponent* handling_component = register_handlers[addr - IO_REGISTERS_START];
        if (handling_component) {
            handling_component->set_register(addr, value);
            return;
        }
    }

    if (addr <= 0x7FFF) {
        // Protect ROM
        return;
    }
    if (dma().active() && addr < 0xFF80) {
        // Cannot access non-HRAM during DMA
        return;
    }
    if (ppu().enabled()) {
        // Cannot access vram during rendering mode 3
        if (addr >= 0x8000 && addr <= 0x9FFF && ppu().mode == LCDDrawMode::Drawing) {
            return;
        }
        // Cannot access OAM during rendering modes 2 / 3
        if (addr >= 0xFE00 && addr <= 0xFE9F && (ppu().mode == LCDDrawMode::Drawing || ppu().mode == LCDDrawMode::OAM_Scan)) {
            return;
        }
    }

    if (addr >= 0xE000 && addr <= 0xFDFF) {
        // Echo RAM
        addr -= 0x2000;
    }

    address_space[addr] = value;
}


void GBSystem::add_register_callbacks(GBComponent* component, std::initializer_list<uint16_t> addresses) {
    for (uint16_t addr : addresses) {
        register_handlers[addr - IO_REGISTERS_START] = component;
    }
}

void GBSystem::add_register_callbacks_range(GBComponent* component, uint16_t address_start, uint16_t address_end_exclusive) {
    for (uint16_t addr = address_start; addr < address_end_exclusive; addr++) {
        register_handlers[addr - IO_REGISTERS_START] = component;
    }
}

void GBSystem::request_interrupt(Interrupts::Interrupts interrupt) {
    address_space[IF] |= (uint8_t) interrupt;
}
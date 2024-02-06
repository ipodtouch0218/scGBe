#include "gbsystem.h"
#include <cstring>
#include <iostream>
#include <iomanip>
#include "registers.h"
#include "gbcomponent.h"
#include "memorymap.h"

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

    // Joypad
    joypad().tick();

    // 4 clock cycles = 1 CPU cycle
    if (cycles % 4 == 0) {
        cpu().tick();
    }

    frame_cycles++;
    cycles++;
    if (frame_cycles == (int) (clock_speed / 59.7275)) {
        frame_number++;
        frame_cycles = 0;
        return true;
    }
    return false;
}

uint8_t GBSystem::read_address(uint16_t address, bool internal) {

    if (address >= IO_REGISTERS_START && address < (IO_REGISTERS_START + IO_REGISTERS_SIZE)) {
        // IO Registers
        GBComponent* handling_component = register_handlers[address - IO_REGISTERS_START];
        if (handling_component) {
            return handling_component->read_io_register(address);
        }
        return 0xFF;
    }

    if (address >= HRAM_START && address < (HRAM_START + HRAM_SIZE)) {
        // HRAM
        return _hram[address - HRAM_START];
    }

    // TODO: ROM is readable on GBC, apparently.
    if (!internal && dma().active()) {
        // Cannot access non-HRAM during DMA
        // Return what the DMA is reading...
        return read_address(dma().current_address(), true);
    }

    if ((address >= ROM_START && address < (ROM_START + (ROM_SIZE * 2))) || (address >= SRAM_START && address < (SRAM_START + SRAM_SIZE))) {
        // ROM / SRAM
        return cartridge().read_address(address);
    }

    if (address >= ERAM_START && address < (ERAM_START + ERAM_SIZE)) {
        // ERAM
        address -= 0x2000;
    }

    if (address >= WRAM_BANK0_START && address < (WRAM_BANK0_START + (WRAM_SIZE * 2))) {
        // WRAM
        if (cgb_mode()) {
            // TODO: second bank
            address -= WRAM_BANK0_START;
        } else {
            address -= WRAM_BANK0_START;
        }
        return _wram[address];
    }

    if ((address >= VRAM_START && address < (VRAM_START + VRAM_SIZE)) || (address >= OAM_START && address < (OAM_START + OAM_SIZE))) {
        // VRAM / OAM
        return ppu().read_address(address, internal);
    }

    return 0xFF;
}

void GBSystem::write_address(uint16_t address, uint8_t value, bool internal) {

    if (address >= IO_REGISTERS_START && address < (IO_REGISTERS_START + IO_REGISTERS_SIZE)) {
        // IO Registers
        GBComponent* handling_component = register_handlers[address - IO_REGISTERS_START];
        if (handling_component) {
            handling_component->write_io_register(address, value);
        }
        return;
    }

    if (address >= HRAM_START && address < (HRAM_START + HRAM_SIZE)) {
        // HRAM
        _hram[address - HRAM_START] = value;
        return;
    }

    if (!internal && dma().active()) {
        // Cannot access non-HRAM during DMA
        return;
    }

    if ((address >= ROM_START && address < (ROM_START + (ROM_SIZE * 2))) || (address >= SRAM_START && address < (SRAM_START + SRAM_SIZE))) {
        // ROM / SRAM
        cartridge().write_address(address, value);
        return;
    }

    if (address >= ERAM_START && address < (ERAM_START + ERAM_SIZE)) {
        // ERAM
        address -= 0x2000;
    }

    if (address >= WRAM_BANK0_START && address < (WRAM_BANK0_START + (WRAM_SIZE * 2))) {
        // WRAM
        if (cgb_mode()) {
            // TODO: second bank
            address -= WRAM_BANK0_START;
        } else {
            address -= WRAM_BANK0_START;
        }
        _wram[address] = value;
        return;
    }

    if ((address >= VRAM_START && address < (VRAM_START + VRAM_SIZE)) || (address >= OAM_START && address < (OAM_START + OAM_SIZE))) {
        // VRAM / OAM
        return ppu().write_address(address, value, internal);
    }
}


void GBSystem::add_register_callbacks(GBComponent* component, std::initializer_list<uint16_t> addresses) {
    for (uint16_t address : addresses) {
        register_handlers[address - IO_REGISTERS_START] = component;
    }
}

void GBSystem::add_register_callbacks_range(GBComponent* component, uint16_t address_start, uint16_t address_end_exclusive) {
    for (uint16_t address = address_start; address < address_end_exclusive; address++) {
        register_handlers[address - IO_REGISTERS_START] = component;
    }
}

bool GBSystem::request_interrupt(Interrupts::Interrupts interrupt) {
    uint8_t old_value = read_address(IF, true);
    write_address(IF, old_value | (uint8_t) interrupt, true);
    return (old_value & (uint8_t) interrupt) == 0;
}

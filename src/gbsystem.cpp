#include "gbsystem.h"

#include <iostream>
#include <cstring>
#include "registers.h"

extern uint8_t joypad_buttons;

GBSystem::GBSystem(uint32_t clock_speed) {
    this->clock_speed = clock_speed;
}

void GBSystem::reset() {
    cycles = 0;
    timer_cycles = 0;

    cpu().reset();
    // memset(address_space + 0x8000, 0, 0xA000-0x8000); // Clear VRAM
    address_space[STAT] = 0;
    memset(address_space + 0x8000, 0, 0xFFFF-0x8000); // Clear RAM
}

bool GBSystem::tick() {


    // PPU
    ppu().tick();

    // Timer
    // TODO: implement https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
    if (cycles % 256) {
        address_space[DIV]++;
    }
    uint8_t tac_register = address_space[TAC];
    if (tac_register & (1 << 2)) {
        // Timer enabled
        uint16_t frequency;
        switch(tac_register & 0b11) {
        case 0: frequency = 1024; break;
        case 1: frequency = 16; break;
        case 2: frequency = 64; break;
        case 3: frequency = 256; break;
        }

        // TODO: is this accurate?
        if (cycles % frequency == 0) {
            // Increment TIMA
            if (++address_space[TIMA] == 0) {
                // Overflowed. Create an interrupt and reset to TMA
                address_space[TIMA] = address_space[TMA];
                address_space[IF] |= (1 << INT_TIMER);
            }
        }
    }

    // DMA
    if (dma_counter > 0) {
        uint8_t addr_msb = address_space[DMA];
        uint8_t addr_lsb = 0xA0 - dma_counter;
        uint16_t source_addr = (((uint16_t) addr_msb) << 8) | addr_lsb;
        uint16_t dest_addr = 0xFE00 | addr_lsb;

        address_space[dest_addr] = address_space[source_addr];
        dma_counter--;
    }

    // 4 clock cycles = 1 CPU cycle
    if (cycles % 4 == 0) {
        cpu().tick();
    }

    if (++cycles == (int) (clock_speed / 59.7275)) {
        cycles = 0;
        frame_number++;
        return true;
    }
    return false;
}

uint8_t GBSystem::get_address_space_byte(uint16_t addr) {
    if (addr >= 0xE000 && addr <= 0xFDFF) {
        // ERAM support:
        addr -= 0x2000;
    }
    // Cannot access during DMA
    if (dma_counter > 0 && addr < 0xFF80) {
        return 0xFF;
    }
    if (ppu().enabled()) {
        // Cannot access vram during rendering mode 3
        if (addr >= 0x8000 && addr <= 0x9FFF && ppu().mode == LCDDrawMode::DRAWING) {
            return 0xFF;
        }
        // Cannot access OAM during rendering modes 2 / 3
        if (addr >= 0xFE00 && addr <= 0xFE9F && (ppu().mode == LCDDrawMode::DRAWING || ppu().mode == LCDDrawMode::OAM_SCAN)) {
            return 0xFF;
        }
    }
    if (addr == JOYP) {
        // Joypad register
        uint8_t joypad_register = address_space[addr];
        uint8_t results;
        switch ((joypad_register & 0b00110000) >> 4) {
        case 1: {
            // Dpad buttons
            results = (joypad_buttons >> 4) & 0xF;
            break;
        }
        case 2: {
            // Other buttons
            results = joypad_buttons & 0xF;
            break;
        }
        default: results = 0xF; break;
        }
        return joypad_register | results;
    }

    // std::cout << "[R] 0x" << std::hex << (int) addr << " -> " << (int) address_space[addr] << std::endl;
    return address_space[addr];
}

void GBSystem::set_address_space_byte(uint16_t addr, uint8_t value) {
    if (addr == LY) {
        // Protect LY
        return;
    }
    if (addr == DIV) {
        value = 0;
    }
    if (addr <= 0x7FFF) {
        // Protect ROM
        return;
    }
    // Cannot access during DMA
    if (dma_counter > 0 && addr < 0xFF80) {
        return;
    }
    if (ppu().enabled()) {
        // Cannot access vram during rendering mode 3
        if (addr >= 0x8000 && addr <= 0x9FFF && ppu().mode == LCDDrawMode::DRAWING) {
            return;
        }
        // Cannot access OAM during rendering modes 2 / 3
        if (addr >= 0xFE00 && addr <= 0xFE9F && (ppu().mode == LCDDrawMode::DRAWING || ppu().mode == LCDDrawMode::OAM_SCAN)) {
            return;
        }
    }


    if (addr >= 0xE000 && addr <= 0xFDFF) {
        // ERAM support:
        addr -= 0x2000;
    }

    address_space[addr] = value;

    // Start DMA
    if (addr == DMA) {
        dma_counter = 0xA0;
    }
}
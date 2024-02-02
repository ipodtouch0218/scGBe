#include "cartridge.h"
#include <iostream>

extern bool log_on;

Cartridge::Cartridge(GBSystem& gb_param) :
    gb(gb_param)
{

}

void Cartridge::load_rom(std::vector<uint8_t>& bytes) {
    _rom = bytes;

    switch (_rom[0x0147]) {
    case 0x01:
    case 0x02:
    case 0x03: {
        _mbc = MBC::MBC1;
        break;
    }
    default: {
        _mbc = MBC::None;
    }
    }
    _rom_size = _rom[0x0148];
    _sram_size = _rom[0x0149];

    uint32_t sram_bytes;
    switch (_sram_size) {
    case 0x02: {
        sram_bytes = 1 * 8192;
        break;
    }
    case 0x43: {
        sram_bytes = 4 * 8192;
        break;
    }
    case 0x04: {
        sram_bytes = 16 * 8192;
        break;
    }
    case 0x05: {
        sram_bytes = 8 * 8192;
        break;
    }
    default: {
        sram_bytes = 0;
        break;
    }
    }
    _sram = std::vector<uint8_t>(sram_bytes);
}

uint8_t Cartridge::read_address(uint16_t address) {
    uint32_t target_addr = address;
    if (address >= ROM_BANK0 && address < ROM_BANK1 + ROM_BANK_SIZE) {
        // ROM
        uint8_t effective_rom_bank;
        switch (_mbc) {
        case MBC::MBC1: {
            effective_rom_bank = _rom_bank & (_rom_size <= 0x03 ? 0xF : 0x1F);
            if (address >= ROM_BANK0 && address < ROM_BANK1) {
                if (_banking_mode) {
                    // Advanced mode
                    // Upper 2 bits control the lower rom bank
                    effective_rom_bank &= 0x60;
                } else {
                    effective_rom_bank = 0;
                }
            } else {
                // If lower 5 bits == 1, set lsb
                effective_rom_bank |= ((_rom_bank & 0x1F) == 0);
                effective_rom_bank--;
            }
            target_addr = address + (((uint32_t) effective_rom_bank) * ROM_BANK_SIZE);
            break;
        }
        }
        return _rom[target_addr];
    } else {
        // SRAM
        if (!_sram_enabled) {
            return 0xFF;
        }

        switch (_mbc) {
        case MBC::MBC1: {
            if (_banking_mode) {
                target_addr = address + (_sram_bank * ROM_BANK_SIZE);
            }
        }
        }

        return _sram[target_addr];
    }
}

void Cartridge::write_address(uint16_t address, uint8_t value) {

    switch (_mbc) {
    case MBC::MBC1: {
        if (address <= 0x1FFF) {
            // RAM enable
            _sram_enabled = (value & 0xF) == 0xA;
        } else if (address <= 0x3FFF) {
            // ROM Bank number lower bits selection
            _rom_bank &= ~0x1F;
            _rom_bank |= (value & 0x1F);

        } else if (address <= 0x5FFF) {
            // RAM Bank selection or ROM Bank number upper bits
            if (_rom_size >= 0x05) {
                // ROM bank upper bits selection
                _rom_bank &= 0x1F;
                _rom_bank |= (value << 4);
            } else {
                _sram_bank = (value & 0x3);
            }
        } else if (address <= 0x7FFF) {
            // Banking mode select
            _banking_mode = (value & 1);

        } else if (address >= SRAM_BANK && address < SRAM_BANK + SRAM_BANK_SIZE) {
            // SRAM
            if (!_sram_enabled) {
                break;
            }

            address -= SRAM_BANK;
            if (_banking_mode) {
                address += (_sram_bank * ROM_BANK_SIZE);
            }

            if (address >= _sram.size()) {
                break;
            }
            _sram[address] = value;
        }
        break;
    }
    default: {
        break;
    }
    }
}

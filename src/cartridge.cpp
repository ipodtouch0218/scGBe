#include "cartridge.h"
#include <ctime>
#include <iostream>
#include "memorymap.h"

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
    case 0x05:
    case 0x06: {
        _mbc = MBC::MBC2;
        break;
    }
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13: {
        _mbc = MBC::MBC3;
        break;
    }
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E: {
        _mbc = MBC::MBC5;
        break;
    }
    default: {
        _mbc = MBC::None;
        break;
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
    case 0x03: {
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
    if (_mbc == MBC::MBC2) {
        // 512 4 bit bytes
        sram_bytes = 512;
    }
    _sram = std::vector<uint8_t>(sram_bytes);
}

uint8_t Cartridge::read_address(uint16_t address) {
    uint32_t target_addr = address;
    if (address >= ROM_START && address < (ROM_START + (ROM_SIZE * 2))) {
        // ROM
        switch (_mbc) {
        case MBC::MBC1: {
            uint8_t effective_rom_bank = _rom_bank & (_rom_size <= 0x03 ? 0xF : 0x1F);
            if (address >= ROM_START && address < (ROM_START + ROM_SIZE)) {
                if (_banking_mode) {
                    // Advanced mode
                    // Upper 2 bits control the lower rom bank
                    effective_rom_bank &= 0x60;
                } else {
                    effective_rom_bank = 0;
                }
            }
            target_addr = (address % ROM_SIZE) + (((uint32_t) effective_rom_bank) * ROM_SIZE);
            break;
        }
        case MBC::MBC2: {
            uint8_t effective_rom_bank = (_rom_bank == 0) ? 1 : _rom_bank;
            if (address >= (ROM_START + ROM_SIZE)) {
                target_addr = (address % ROM_SIZE) + (((uint32_t) effective_rom_bank) * ROM_SIZE);
            }
            break;
        }
        case MBC::MBC3: {
            uint8_t effective_rom_bank = _rom_bank;
            if (address >= (ROM_START + ROM_SIZE)) {
                // If bank == 0, set it to 1.
                effective_rom_bank |= (_rom_bank == 0);
            } else {
                effective_rom_bank = 0;
            }

            target_addr = (address % ROM_SIZE) + (((uint32_t) effective_rom_bank) * ROM_SIZE);
            break;
        }
        case MBC::MBC5: {
            uint8_t effective_rom_bank = _rom_bank;
            if (address < (ROM_START + ROM_SIZE)) {
                effective_rom_bank = 0;
            }
            target_addr = (address % ROM_SIZE) + (((uint32_t) effective_rom_bank) * ROM_SIZE);
            break;
        }
        }

        if (target_addr >= _rom.size()) {
            return 0xFF;
        }
        return _rom[target_addr];
    } else {
        // SRAM
        if (!_sram_enabled) {
            return 0xFF;
        }

        target_addr -= SRAM_START;
        switch (_mbc) {
        case MBC::MBC1: {
            if (_banking_mode) {
                target_addr += (_sram_bank * ROM_SIZE);
            }
            break;
        }
        case MBC::MBC2: {
            // Mirrored across all of sram bank
            target_addr %= 512;
            // Only lower 4 bits are usable
            return _sram[target_addr] & 0xF;
        }
        case MBC::MBC3: {
            if (_sram_bank <= 0x03) {
                // SRAM
                target_addr += (_sram_bank * ROM_SIZE);
            } else {
                if (_rtc_latched) {
                    // Latched
                    return _latched_rtc_value;
                } else {
                    // Realtime
                    time_t now = time(0);
                    tm* local_now = localtime(&now);
                    switch (_sram_bank) {
                    case 0x08: {
                        // Seconds (0-59)
                        return local_now->tm_sec;
                    }
                    case 0x09: {
                        // Minutes (0-59)
                        return local_now->tm_min;
                    }
                    case 0x0A: {
                        // Hours (0-23)
                        return local_now->tm_hour;
                    }
                    case 0x0B: {
                        // Days (lower)
                        return local_now->tm_yday;
                    }
                    case 0x0C: {
                        // Days (upper bit) + control
                        return (local_now->tm_yday >> 8) & 1;
                    }
                    default: {
                        return 0xFF;
                    }
                    }
                }
            }
            break;
        }
        case MBC::MBC5: {
            target_addr += (_sram_bank * ROM_SIZE);
            break;
        }
        default: {
            break;
        }
        }

        if (target_addr >= _sram.size()) {
            return 0xFF;
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
            // Values of "0" are automatically converted to "1", regardless of upper bits.
            value |= (value == 0);
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

        } else if (address >= SRAM_START && address < (SRAM_START + SRAM_SIZE)) {
            // SRAM
            if (!_sram_enabled) {
                break;
            }

            address -= SRAM_START;
            if (_banking_mode) {
                address += (_sram_bank * ROM_SIZE);
            }

            if (address >= _sram.size()) {
                break;
            }
            _sram[address] = value;
        }
        break;
    }
    case MBC::MBC2: {
        if (address <= 0x3FFF) {
            // RAM enable and ROM Bank number lower bits selection
            if (address & 0x100) {
                // ROM bank (16 max)
                _rom_bank = (value & 0xF);
            } else {
                // SRAM enable
                _sram_enabled = (value == 0x0A);
            }

        } else if (address >= SRAM_START && address < (SRAM_START + SRAM_SIZE)) {
            // SRAM
            if (!_sram_enabled) {
                break;
            }

            address -= SRAM_START;
            if (address >= _sram.size()) {
                break;
            }
            _sram[address] = value & 0xF;
        }
        break;
    }
    case MBC::MBC3: {
        if (address <= 0x1FFF) {
            // RAM + RTC enable
            _sram_enabled = (value & 0xF) == 0xA;

        } else if (address <= 0x3FFF) {
            // ROM Bank number selection
            _rom_bank = value;

        } else if (address <= 0x5FFF) {
            // RAM Bank + RTC selection
            _sram_bank = value;

        } else if (address <= 0x7FFF) {
            // Latch Clock Data
            if (value == 0) {
                _rtc_latched = false;
            } else {
                time_t now = time(0);
                tm* local_now = localtime(&now);
                switch (_sram_bank) {
                case 0x08: {
                    // Seconds (0-59)
                    _latched_rtc_value = local_now->tm_sec;
                    break;
                }
                case 0x09: {
                    // Minutes (0-59)
                    _latched_rtc_value = local_now->tm_sec;
                    break;
                }
                case 0x0A: {
                    // Hours (0-23)
                    _latched_rtc_value = local_now->tm_hour;
                    break;
                }
                case 0x0B: {
                    // Days (lower)
                    _latched_rtc_value = local_now->tm_yday;
                    break;
                }
                case 0x0C: {
                    // Days (upper bit) + control
                    _latched_rtc_value = (local_now->tm_yday >> 8) & 1;
                    break;
                }
                default: {
                    // Invalid
                    _latched_rtc_value = 0xFF;
                    break;
                }
                }
                _rtc_latched = true;
            }
        } else if (address >= SRAM_START && address < (SRAM_START + SRAM_SIZE)) {
            // SRAM
            if (!_sram_enabled) {
                break;
            }

            if (_sram_bank <= 0x03) {
                // SRAM
                address -= SRAM_START;
                address += (_sram_bank * ROM_SIZE);
                address %= _sram.size();
                _sram[address] = value;
                break;
            } else {
                time_t now = time(0);
                tm* local_now = localtime(&now);
                switch (_sram_bank) {
                case 0x08: {
                    // Seconds (0-59)
                    break;
                }
                case 0x09: {
                    // Minutes (0-59)
                    break;
                }
                case 0x0A: {
                    // Hours (0-23)
                    break;
                }
                case 0x0B: {
                    // Days (lower)
                    break;
                }
                case 0x0C: {
                    // Days (upper bit) + control
                    break;
                }
                default: {
                    // Invalid
                    break;
                }
                }
            }
        }
        break;
    }
    case MBC::MBC5: {
        if (address <= 0x1FFF) {
            // RAM  enable
            _sram_enabled = (value & 0xF) == 0xA;

        } else if (address <= 0x2FFF) {
            // ROM Bank number lower bits
            _rom_bank &= 0xFF00;
            _rom_bank |= value;

        } else if (address <= 0x3FFF) {
            // ROM Bank number higher bit
            _rom_bank &= 0x00FF;
            _rom_bank |= ((uint16_t) (value & 1)) << 8;

        } else if (address <= 0x5FFF) {
            // RAM Bank
            _sram_bank = value;

        } else if (address >= SRAM_START && address < (SRAM_START + SRAM_SIZE)) {
            // SRAM
            if (!_sram_enabled) {
                break;
            }

            address -= SRAM_START;
            address += (_sram_bank * ROM_SIZE);
            address %= _sram.size();
            _sram[address] = value;
        }
        break;
    }
    default: {
        break;
    }
    }
}

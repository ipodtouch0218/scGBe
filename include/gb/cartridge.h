#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <iostream>

class GBSystem;

namespace MBC {
    enum MBC {
        None,
        MBC1,
        MBC2,
        MBC3,
        MBC5,
        MBC6,
        MBC7,
        MMM01,
        M161,
        HuC1,
        HuC_3,
        Unknown,
    };
}

struct CartridgeHeader {
    uint8_t entry_point[4]; // 0x100 - 0x103: Code entry point
    uint8_t nintendo_logo[0x30]; // 0x104 - 0x133: Nintendo logo
    uint8_t title[11]; // 0x134 - 0x13E: title
    uint8_t manufacturer_code[4]; // 0x13F - 0x142: Manufacturer Code
    uint8_t cgb_flag; // 0x143: CGB Flag
    uint8_t licensee_code[2]; // 0x144 - 0x145: Licensee Code
    uint8_t sgb_flag; // 0x146: Super Game Boy Flag
    uint8_t cartridge_type; // 0x147: Cartridge Type (MBC + Functionality)
    uint8_t rom_size; // 0x148: ROM size, 32kb * (1 << value)
    uint8_t sram_size; // 0x149: SRAM size
    uint8_t destination_code; // 0x14A: Destination code
    uint8_t old_licensee_code; // 0x14B: (Old) Licensee code
    uint8_t version_number; // 0x14C: ROM Version Number
    uint8_t header_checksum; // 0x14D: Header checksum, includes bytes 0x134..0x14C
    uint8_t global_checksum[2]; // 0x14E - 0x14F: Global checksum, equal to all bytes summed, excluding the global checksum itself

    const std::string str_title() const {
        return std::string((char*) title, 11);
    }

    const uint8_t calculate_header_checksum() const {
        uint8_t checksum = 0;
        for (const uint8_t* pointer = title; pointer <= &version_number; pointer++) {
            checksum = checksum - (uint8_t) (*pointer) - 1;
        }
        return checksum;
    }
};

class Cartridge {

    private:
    GBSystem& gb;

    const CartridgeHeader* _header;
    MBC::MBC _mbc = MBC::None;

    std::vector<uint8_t> _rom;
    uint16_t _rom_bank = 0;
    uint8_t _banking_mode = 0;

    std::vector<uint8_t> _sram;
    bool _sram_enabled = false;
    uint16_t _sram_bank = 0;

    bool _rtc_latched = false;
    uint8_t _latched_rtc_value = 0;

    public:
    Cartridge(GBSystem& gb);

    void load_rom(std::vector<uint8_t>& bytes);

    uint8_t read_address(uint16_t address);
    void write_address(uint16_t address, uint8_t value);

    const CartridgeHeader& header() const {
        return *_header;
    }
};
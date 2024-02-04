#pragma once
#include <stdint.h>
#include <vector>

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

class Cartridge {

    protected:
    GBSystem& gb;
    MBC::MBC _mbc = MBC::None;

    std::vector<uint8_t> _rom;
    uint8_t _rom_size = 0;
    uint16_t _rom_bank = 0;
    uint8_t _banking_mode = 0;

    std::vector<uint8_t> _sram;
    bool _sram_enabled = false;
    uint16_t _sram_bank = 0;
    uint8_t _sram_size = 0;

    bool _rtc_latched = false;
    uint8_t _latched_rtc_value = 0;


    public:
    Cartridge(GBSystem& gb);

    void load_rom(std::vector<uint8_t>& bytes);

    uint8_t read_address(uint16_t address);
    void write_address(uint16_t address, uint8_t value);

};
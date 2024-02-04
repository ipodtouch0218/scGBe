#pragma once
#include <stdint.h>

constexpr uint16_t ROM_START = 0x0000;
constexpr uint16_t ROM_BANK0_START = 0x0000;
constexpr uint16_t ROM_BANK1_START = 0x0000;
constexpr uint16_t ROM_SIZE = 0x4000;

constexpr uint16_t VRAM_START = 0x8000;
constexpr uint16_t VRAM_SIZE = 0x2000;
constexpr uint8_t VRAM_BANKS = 2;

constexpr uint16_t SRAM_START = 0xA000;
constexpr uint16_t SRAM_SIZE = 0x2000;

constexpr uint16_t WRAM_BANK0_START = 0xC000;
constexpr uint16_t WRAM_BANK1_START = 0xD000;
constexpr uint16_t WRAM_SIZE = 0x1000;
constexpr uint8_t WRAM_BANKS = 8;

constexpr uint16_t ERAM_START = 0xE000;
constexpr uint16_t ERAM_SIZE = 0x1E00;

constexpr uint16_t OAM_START = 0xFE00;
constexpr uint16_t OAM_SIZE = 0x00A0;

constexpr uint16_t IO_REGISTERS_START = 0xFF00;
constexpr uint16_t IO_REGISTERS_SIZE = 0x0080;

constexpr uint16_t HRAM_START = 0xFF80;
constexpr uint16_t HRAM_SIZE = 0x0080; // Including the interrupt register 0xFFFF in here.
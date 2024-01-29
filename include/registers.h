#pragma once
#include <stdint.h>

// Joypad inputs
constexpr uint16_t JOYP = 0xFF00;

// Timer stuff
constexpr uint16_t DIV = 0xFF04;
constexpr uint16_t TIMA = 0xFF05;
constexpr uint16_t TMA = 0xFF06;
constexpr uint16_t TAC = 0xFF07;

// PPU stuff
constexpr uint16_t LCDC = 0xFF40;
constexpr uint16_t STAT = 0xFF41;
constexpr uint16_t SCY = 0xFF42;
constexpr uint16_t SCX = 0xFF43;
constexpr uint16_t LY = 0xFF44;
constexpr uint16_t LYC = 0xFF45;
constexpr uint16_t DMA = 0xFF46;
constexpr uint16_t BGP = 0xFF47;
constexpr uint16_t OBP0 = 0xFF48;
constexpr uint16_t OBP1 = 0xFF49;

// APU stuff
constexpr uint16_t SND_P1_SW = 0xFF10;
constexpr uint16_t SND_P1_LD = 0xFF11;
constexpr uint16_t SND_P1_VE = 0xFF12;
constexpr uint16_t SND_P1_PERLOW = 0xFF13;
constexpr uint16_t SND_P1_PERHI = 0xFF14;
constexpr uint16_t SND_P2_LD = 0xFF16;
constexpr uint16_t SND_P2_VE = 0xFF17;
constexpr uint16_t SND_P2_PERLOW = 0xFF18;
constexpr uint16_t SND_P2_PERHI = 0xFF19;
constexpr uint16_t SND_WV_EN = 0xFF1A;
constexpr uint16_t SND_WV_LEN = 0xFF1B;
constexpr uint16_t SND_WV_VOL = 0xFF1C;
constexpr uint16_t SND_WV_PERLOW = 0xFF1D;
constexpr uint16_t SND_WV_PERHI = 0xFF1E;
constexpr uint16_t SND_WV_TABLE = 0xFF30;
constexpr uint16_t SND_NS_LEN = 0xFF20;
constexpr uint16_t SND_NS_VOL = 0xFF21;
constexpr uint16_t SND_NS_FREQ = 0xFF22;
constexpr uint16_t SND_NS_CTRL = 0xFF23;
constexpr uint16_t NR50 = 0xFF24;
constexpr uint16_t NR51 = 0xFF25;
constexpr uint16_t NR52 = 0xFF26;

// Interrupt stuff
constexpr uint16_t IF = 0xFF0F;
constexpr uint16_t IE = 0xFFFF;
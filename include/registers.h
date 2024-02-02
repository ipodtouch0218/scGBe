#pragma once
#include <functional>
#include <stdint.h>

using registergetter_t = std::function<uint8_t(uint16_t)>;
// typedef uint8_t (*registergetter_t)(uint16_t);
using registersetter_t = std::function<void(uint16_t, uint8_t)>;
// typedef void (*registersetter_t)(uint16_t, uint8_t);

namespace Interrupts {
    enum Interrupts {
        VBlank = 1 << 0,
        Stat = 1 << 1,
        Timer = 1 << 2,
        Serial = 1 << 3,
        Joypad = 1 << 4,
    };
}

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
constexpr uint16_t WY = 0xFF4A;
constexpr uint16_t WX = 0xFF4B;

// APU stuff
constexpr uint16_t SND_P1_ORIGIN = 0xFF10;
constexpr uint16_t SND_P1_SWEEP = 0xFF10;
constexpr uint16_t SND_P1_LEN_DUTY = 0xFF11;
constexpr uint16_t SND_P1_VOL_ENV = 0xFF12;
constexpr uint16_t SND_P1_PER_LOW = 0xFF13;
constexpr uint16_t SND_P1_PER_HI = 0xFF14;

constexpr uint16_t SND_P2_ORIGIN = 0xFF15;
constexpr uint16_t SND_P2_LEN_DUTY = 0xFF16;
constexpr uint16_t SND_P2_VOL_ENV = 0xFF17;
constexpr uint16_t SND_P2_PER_LOW = 0xFF18;
constexpr uint16_t SND_P2_PER_HI = 0xFF19;

constexpr uint16_t SND_WV_ORIGIN = 0xFF1A;
constexpr uint16_t SND_WV_EN = 0xFF1A;
constexpr uint16_t SND_WV_LEN = 0xFF1B;
constexpr uint16_t SND_WV_VOL = 0xFF1C;
constexpr uint16_t SND_WV_PERLOW = 0xFF1D;
constexpr uint16_t SND_WV_PERHI = 0xFF1E;
constexpr uint16_t SND_WV_TABLE = 0xFF30;

constexpr uint16_t SND_NS_ORIGIN = 0xFF1F;
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
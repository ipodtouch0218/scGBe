#pragma once
#include <stdint.h>

constexpr uint8_t SCREEN_W = 172;
constexpr uint8_t SCREEN_H = 144;
constexpr uint16_t OAM_SCAN_DOTS = 80;
constexpr uint16_t DOTS_PER_SCANLINE = 456;
constexpr uint16_t SCANLINES_PER_FRAME = 154;

constexpr uint8_t PPU_MODE = 0;
constexpr uint8_t LYC_EQ = 2;
constexpr uint8_t MODE0_INT_SEL = 3;
constexpr uint8_t MODE1_INT_SEL = 4;
constexpr uint8_t MODE2_INT_SEL = 5;
constexpr uint8_t LYC_INT_SEL = 6;

class GBSystem;

namespace LCDDrawMode {
    enum LCDDrawMode {
        HBLANK = 0,
        VBLANK = 1,
        OAM_SCAN = 2,
        DRAWING = 3
    };
}

class PPU {
    public:
    GBSystem* gb;
    LCDDrawMode::LCDDrawMode mode;
    uint16_t dots;
    uint8_t penalty_dots;
    uint8_t draw_pixel_x;

    PPU(GBSystem* gb);

    void tick();
};
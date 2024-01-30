#pragma once
#include <stdint.h>
#include <vector>

constexpr uint8_t SCREEN_W = 160;
constexpr uint8_t SCREEN_H = 144;
constexpr uint16_t OAM_SCAN_DOTS = 80;
constexpr uint16_t DOTS_PER_SCANLINE = 456;
constexpr uint16_t SCANLINES_PER_FRAME = 154;
constexpr uint8_t MAX_SPRITES_PER_SCANLINE = 10;

constexpr uint8_t PPU_MODE = 0;
constexpr uint8_t LYC_EQ = 2;
constexpr uint8_t MODE0_INT_SEL = 3;
constexpr uint8_t MODE1_INT_SEL = 4;
constexpr uint8_t MODE2_INT_SEL = 5;
constexpr uint8_t LYC_INT_SEL = 6;

constexpr uint16_t TILE_BLOCK0_ADDR = 0x8000;
constexpr uint16_t TILE_BLOCK1_ADDR = 0x8800;
constexpr uint16_t TILE_BLOCK2_ADDR = 0x9000;

constexpr uint16_t TILEMAP0_ADDR = 0x9800;
constexpr uint16_t TILEMAP1_ADDR = 0x9C00;

constexpr uint16_t OAM_ADDR = 0xFE00;
constexpr uint8_t OAM_MAX_ELEMENTS = 40;
constexpr uint8_t OAM_ELEMENT_SIZE = 4;

class GBSystem;

namespace LCDDrawMode {
    enum LCDDrawMode {
        HBLANK = 0,
        VBLANK = 1,
        OAM_SCAN = 2,
        DRAWING = 3
    };
}

struct OAMEntry {
    uint8_t y_position;
    uint8_t x_position;
    uint8_t tile_index;
    uint8_t attributes;

    bool priority() {
        return attributes & (1 << 7) != 0;
    }

    bool y_flip() {
        return attributes & (1 << 6) != 0;
    }

    bool x_flip() {
        return attributes & (1 << 5) != 0;
    }

    bool dmg_palette() {
        return attributes & (1 << 4) != 0;
    }

    bool vram_bank() {
        return attributes & (1 << 3) != 0;
    }

    uint8_t cgb_palette() {
        return attributes & 0b111;
    }
};

class PPU {
    public:
    GBSystem* gb;
    LCDDrawMode::LCDDrawMode mode;
    uint16_t dots;
    uint8_t penalty_dots;
    uint8_t draw_pixel_x;

    uint8_t framebuffer[SCREEN_W * SCREEN_H * 4];

    std::vector<OAMEntry*> scanline_sprite_buffer;
    uint8_t last_drawn_sprite_tile_x = -1;

    bool was_disabled = false;

    PPU(GBSystem* gb);

    void tick();
    bool enabled();
};
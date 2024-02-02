#pragma once
#include <stdint.h>
#include <vector>
#include "gbcomponent.h"
#include "utils.h"

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
constexpr uint8_t OAM_SIZE = 40;

class GBSystem;

namespace LCDDrawMode {
    enum LCDDrawMode {
        HBlank = 0,
        VBlank = 1,
        OAM_Scan = 2,
        Drawing = 3
    };
}

struct OAMEntry {
    uint8_t y_position;
    uint8_t x_position;
    uint8_t tile_index;
    uint8_t attributes;

    bool priority() const {
        return utils::get_bit_value(attributes, 7);
    }

    bool y_flip() const {
        return utils::get_bit_value(attributes, 6);
    }

    bool x_flip() const {
        return utils::get_bit_value(attributes, 5);
    }

    bool dmg_palette() const {
        return utils::get_bit_value(attributes, 4);
    }

    bool vram_bank() const {
        return utils::get_bit_value(attributes, 3);
    }

    uint8_t cgb_palette() const {
        return attributes & 0b111;
    }
};

class PPU : public GBComponent {

    private:
    // LY
    uint8_t _current_scanline = 0;
    // LYC
    uint8_t _compare_scanline = 0;
    // STAT
    bool _compare_scanline_interrupt_select = false;
    bool _mode_interrupt_select[3] = {false, false, false};
    // LCDC
    bool _enabled = true;
    bool _window_tilemap_high = false;
    bool _window_enabled = false;
    bool _bg_tile_data_low = false;
    bool _bg_tilemap_high = false;
    bool _obj_tall = false;
    bool _obj_enabled = true;
    bool _bg_window_enable_priority = true;
    // SCY
    uint8_t _bg_scroll_y = 0;
    // SCX
    uint8_t _bg_scroll_x = 0;
    // BGP
    uint8_t _bg_palette[4];
    uint8_t _obj_palettes[2][4];
    // WY
    uint8_t _window_scroll_y = 0;
    // WX
    uint8_t _window_scroll_x = 0;

    // Drawing stuff
    LCDDrawMode::LCDDrawMode _mode = LCDDrawMode::OAM_Scan;
    uint16_t _dots = 0;
    uint8_t _penalty_dots = 0;
    uint8_t _draw_pixel_x = 0;

    std::vector<OAMEntry*> _scanline_sprite_buffer;
    uint8_t _last_drawn_sprite_tile_x = -1;
    bool _was_disabled = false;

    public:
    uint8_t framebuffer[SCREEN_W * SCREEN_H * 4];

    PPU(GBSystem& gb);

    void tick();

    uint8_t get_register(uint16_t address);
    void set_register(uint16_t address, uint8_t value);

    bool enabled() const {
        return _enabled;
    }

    LCDDrawMode::LCDDrawMode mode() const {
        return _mode;
    }

    protected:
    uint8_t get_pixel_of_tile(uint16_t tile_addr, uint8_t x, uint8_t y) const;
};
#include "ppu.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "gbsystem.h"
#include "registers.h"
#include "utils.h"

constexpr uint32_t SCREEN_COLORS[] = {
    0xFF0D6862,
    0xFF356148,
    0xFF3B472E,
    0xFF2E3421
};

PPU::PPU(GBSystem& gb) :
    GBComponent::GBComponent(gb)
{
    gb.add_register_callbacks(this, {LY, LYC, STAT, LCDC, SCY, SCX, BGP, OBP0, OBP1, WY, WX});
    _scanline_sprite_buffer.reserve(10);
}

bool oam_entry_pointer_sort(OAMEntry* a, OAMEntry* b) {
    return a->x_position < b->x_position;
}

void PPU::tick() {

    if (!enabled()) {
        _was_disabled = true;
        return;
    }

    if (_was_disabled) {
        // Recovering from being disabled.
        // Wait until the next frame to reset everything (TODO: is this accurate?)
        if (gb.frame_cycles == 0) {
            // Next scanline
            _dots = 0;
            _draw_pixel_x = 0;
            _current_scanline = 0;
            _drawing_window = false;
            _window_scanline = 0;
            _mode = LCDDrawMode::OAM_Scan;
            _was_disabled = false;
        } else {
            return;
        }
    }

    LCDDrawMode::LCDDrawMode previous_draw_mode = _mode;

    if (_mode == LCDDrawMode::OAM_Scan) {
        if (_dots == 0) {
            _drawing_window |= _current_scanline == _window_scroll_y;
        }
        if (_dots == OAM_SCAN_DOTS) {
            _scanline_sprite_buffer.clear();
            uint8_t sprite_height = 8 + (8 * _obj_tall);

            constexpr uint32_t invalid_entry = 0xFFFFFFFF;

            for (int index = 0; index < (OAM_SIZE / sizeof(OAMEntry)); index++) {
                OAMEntry* entry;
                if (gb.dma().active()) {
                    entry = (OAMEntry*) &invalid_entry;
                } else {
                    entry = (OAMEntry*) (_oam + (index * sizeof(OAMEntry)));
                }

                uint8_t y_diff = (_current_scanline + 16) - entry->y_position;
                if (y_diff < sprite_height) {
                    // On this scanline!
                    _scanline_sprite_buffer.push_back(entry);
                }
            }

            if (!gb.cgb_mode()) {
                // TODO Sort entries by x-coordinate, instead of OAM order.
                std::sort(_scanline_sprite_buffer.begin(), _scanline_sprite_buffer.end(), oam_entry_pointer_sort);
            }

            for (size_t i = 0; i < MAX_SPRITES_PER_SCANLINE; i++) {
                _scanline_sprite_penalties[i] = false;
            }

            _mode = LCDDrawMode::Drawing;
        }
    }

    if (_mode == LCDDrawMode::Drawing) {
        // Out here, used for determining the obj penalties
        // BG
        uint8_t target_pixel_in_bg_x = _draw_pixel_x + _bg_scroll_x;
        uint8_t target_pixel_in_bg_y = _current_scanline + _bg_scroll_y;

        uint8_t bg_tilemap_x = (target_pixel_in_bg_x / 8);
        uint8_t bg_tilemap_y = (target_pixel_in_bg_y / 8);
        uint8_t draw_pixel_of_bg_tile_x = target_pixel_in_bg_x % 8;
        uint8_t draw_pixel_of_bg_tile_y = target_pixel_in_bg_y % 8;

        // WINDOW
        bool in_window = _window_enabled && _drawing_window && (_draw_pixel_x + 7) >= _window_scroll_x;
        uint8_t target_pixel_in_window_x = _draw_pixel_x + 7 - _window_scroll_x;
        uint8_t target_pixel_in_window_y = _window_scanline - _window_scroll_y;

        uint8_t window_tilemap_x = target_pixel_in_window_x / 8;
        uint8_t window_tilemap_y = target_pixel_in_window_y / 8;
        uint8_t draw_pixel_of_window_tile_x = target_pixel_in_window_x % 8;
        uint8_t draw_pixel_of_window_tile_y = target_pixel_in_window_y % 8;

        if (_dots == OAM_SCAN_DOTS) {
            _penalty_dots = 12; // 12 wasted cycles at the beginning
            _penalty_dots += (_bg_scroll_x % 8); // Discarding pixels in leftmost tile
        }

        // if statement only runs in DMG mode.
        std::vector<OAMEntry*> sprites_to_draw;
        if (_obj_enabled && !gb.cgb_mode()) {
            for (size_t i = 0; i < std::min(_scanline_sprite_buffer.size(), (size_t) MAX_SPRITES_PER_SCANLINE); i++) {
                OAMEntry* entry = _scanline_sprite_buffer[i];
                uint8_t x_diff = (_draw_pixel_x + 8) - entry->x_position;

                if (!_scanline_sprite_penalties[i] && x_diff == 0) {
                    // 6 dot penalty for retrieving the tile...
                    _penalty_dots += 6;

                    // And additional penalties for the first sprite in the tile
                    uint8_t pixel_x_to_check = in_window ? draw_pixel_of_window_tile_x : draw_pixel_of_bg_tile_x;
                    uint8_t tile_x_to_check = in_window ? window_tilemap_x : bg_tilemap_x;
                    if (tile_x_to_check != _last_drawn_sprite_tile_x) {
                        uint8_t remaining_tile_pixels = (7 - pixel_x_to_check);
                        int8_t additional_penalty = remaining_tile_pixels - 2;
                        if (additional_penalty > 0) {
                            _penalty_dots += additional_penalty;
                        }
                        _last_drawn_sprite_tile_x = tile_x_to_check;
                    }
                    _scanline_sprite_penalties[i] = true;
                }

                if (x_diff >= 8) {
                    continue;
                }

                // Draw this sprite!
                sprites_to_draw.push_back(entry);
            }
        }

        // Window penalty: 6 dots when enabling
        if (!_window_penalty && in_window) {
            _penalty_dots += 6;
            _window_penalty = true;
        }

        if (_penalty_dots) {
            // Don't draw right now, in a penalty
            _penalty_dots--;
        } else {
            // Draw!
            int framebuffer_index = ((int) _draw_pixel_x) + ((int) _current_scanline) * SCREEN_W;

            // Sprites:
            OAMEntry* drew_sprite = nullptr;
            if (_obj_enabled) {
                for (OAMEntry* sprite : sprites_to_draw) {
                    uint8_t x_diff = (_draw_pixel_x + 16) - sprite->x_position;
                    uint8_t y_diff = (_current_scanline + 16) - sprite->y_position;
                    uint8_t tile_index = sprite->tile_index;
                    if (_obj_tall) {
                        if (y_diff > 7 ^ sprite->y_flip()) {
                            tile_index |= 0x01;
                        } else {
                            tile_index &= 0xFE;
                        }
                    }

                    uint8_t draw_pixel_of_sprite_x = x_diff % 8;
                    if (sprite->x_flip()) {
                        draw_pixel_of_sprite_x = 7 - draw_pixel_of_sprite_x;
                    }
                    uint8_t draw_pixel_of_sprite_y = y_diff % 8;
                    if (sprite->y_flip()) {
                        draw_pixel_of_sprite_y = 7 - draw_pixel_of_sprite_y;
                    }

                    uint16_t offset = tile_index;
                    uint16_t sprite_tile_addr = TILE_BLOCK0_ADDR + (offset * 0x10);
                    uint8_t pixel_value = get_pixel_of_tile(sprite_tile_addr, draw_pixel_of_sprite_x, draw_pixel_of_sprite_y);

                    if (pixel_value != 0) {
                        uint8_t final_color_index = _obj_palettes[sprite->dmg_palette()][pixel_value];
                        ((uint32_t*) framebuffer)[framebuffer_index] = SCREEN_COLORS[final_color_index];
                        drew_sprite = sprite;
                        break;
                    }
                }
            }

            // TODO: window bugs (scx==0, stuff like that.)
            uint8_t draw_pixel_value = 0;
            if (_bg_window_enable_priority) {
                if (in_window) {
                    // Window:
                    uint16_t window_tilemap_addr = _window_tilemap_high ? TILEMAP1_ADDR : TILEMAP0_ADDR;
                    uint8_t window_tile_index = read_address(window_tilemap_addr + window_tilemap_x + (window_tilemap_y * 32), true);

                    uint16_t window_tile_addr;
                    if (_bg_tile_data_low) {
                        uint32_t offset = window_tile_index;
                        window_tile_addr = TILE_BLOCK0_ADDR + (offset * 0x10);
                    } else {
                        uint32_t offset = (int8_t) window_tile_index;
                        window_tile_addr = TILE_BLOCK2_ADDR + (offset * 0x10);
                    }

                    draw_pixel_value = get_pixel_of_tile(window_tile_addr, draw_pixel_of_window_tile_x, draw_pixel_of_window_tile_y);

                } else {
                    // Background:
                    uint16_t bg_tilemap_addr = _bg_tilemap_high ? TILEMAP1_ADDR : TILEMAP0_ADDR;
                    uint8_t bg_tile_index = read_address(bg_tilemap_addr + bg_tilemap_x + (bg_tilemap_y * 32), true);

                    uint16_t bg_tile_addr;
                    if (_bg_tile_data_low) {
                        uint32_t offset = bg_tile_index;
                        bg_tile_addr = TILE_BLOCK0_ADDR + (offset * 0x10);
                    } else {
                        int32_t offset = (int8_t) bg_tile_index;
                        bg_tile_addr = TILE_BLOCK2_ADDR + (offset * 0x10);
                    }

                    draw_pixel_value = get_pixel_of_tile(bg_tile_addr, draw_pixel_of_bg_tile_x, draw_pixel_of_bg_tile_y);
                }
            }
            if (!drew_sprite || (drew_sprite->priority() && draw_pixel_value != 0)) {
                ((uint32_t*) framebuffer)[framebuffer_index] = SCREEN_COLORS[_bg_palette[draw_pixel_value]];
            }

            if (++_draw_pixel_x == SCREEN_W) {
                // Done drawing
                _mode = LCDDrawMode::HBlank;
                _last_drawn_sprite_tile_x = -1;
                _window_penalty = false;
            }
        }
    }

    _dots++;
    if (_dots == DOTS_PER_SCANLINE) {
        // Next scanline
        _dots = 0;
        _draw_pixel_x = 0;
        _current_scanline++;
        _window_scanline += (_window_scroll_x <= 166 && _window_scroll_y <= 143);

        if (_current_scanline == SCREEN_H) {
            // Entering VBlank
            _mode = LCDDrawMode::VBlank;
            gb.request_interrupt(Interrupts::VBlank);

        } else if (_mode == LCDDrawMode::HBlank) {
            // Back to OAM
            _mode = LCDDrawMode::OAM_Scan;

        } else if (_current_scanline == SCANLINES_PER_FRAME) {
            // Next frame
            _current_scanline = 0;
            _drawing_window = false;
            _window_scanline = 0;
            _mode = LCDDrawMode::OAM_Scan;
        }
    }

    // Interrupts
    bool trigger_interrupt = (_compare_scanline_interrupt_select && (_current_scanline == _compare_scanline)) || (previous_draw_mode != _mode && _mode <= 2 && _mode_interrupt_select[_mode]);
    if (trigger_interrupt && !_stat_blocking) {
        gb.request_interrupt(Interrupts::Stat);
    }
    _stat_blocking = trigger_interrupt;
}

uint8_t PPU::get_pixel_of_tile(uint16_t tile_addr, uint8_t x, uint8_t y) {
    tile_addr += (y * 2);
    uint8_t graphics_lsb = read_address(tile_addr, true);
    uint8_t graphics_msb = read_address(tile_addr + 1, true);

    uint8_t tile_pixel_mask = 1 << (7 - x);
    return ((graphics_msb & tile_pixel_mask) != 0) << 1 | ((graphics_lsb & tile_pixel_mask) != 0);
}

uint8_t PPU::read_address(uint16_t address, bool internal) {
    if (address >= VRAM_START && address < (VRAM_START + VRAM_SIZE)) {
        // VRAM
        if (enabled() && !internal && mode() == LCDDrawMode::Drawing) {
            // Can't get VRAM during mode 3
            // std::cerr << "READ FROM VRAM WHILE MODE 3 ACTIVE!" << std::endl;
            return 0xFF;
        }

        // TODO: CGB VRAM BANKING
        address -= VRAM_START;
        address %= VRAM_SIZE;
        return _vram[address];

    } else if (address >= OAM_START && address < (OAM_START + OAM_SIZE)) {
        // OAM
        if (enabled() && !internal && (mode() == LCDDrawMode::Drawing || mode() == LCDDrawMode::OAM_Scan)) {
            // Cant get OAM during modes 2/3
            // std::cerr << "READ FROM OAM WHILE MODE 2/3 ACTIVE!" << std::endl;
            return 0xFF;
        }

        address -= OAM_START;
        address %= OAM_SIZE;
        return _oam[address];
    }

    return 0xFF;
}

void PPU::write_address(uint16_t address, uint8_t value, bool internal) {
    if (address >= VRAM_START && address < (VRAM_START + VRAM_SIZE)) {
        // VRAM
        if (enabled() && !internal && mode() == LCDDrawMode::Drawing) {
            // Can't get VRAM during mode 3
            // std::cerr << "WRITE TO VRAM WHILE MODE 3 ACTIVE!" << std::endl;
            return;
        }

        // TODO: CGB VRAM BANKING
        address -= VRAM_START;
        address %= VRAM_SIZE;
        _vram[address] = value;

    } else if (address >= OAM_START && address < (OAM_START + OAM_SIZE)) {
        // OAM
        if (enabled() && !internal && (mode() == LCDDrawMode::Drawing || mode() == LCDDrawMode::OAM_Scan)) {
            // Cant get OAM during modes 2/3
            // std::cerr << "WRITE TO OAM WHILE MODE 2/3 ACTIVE!" << std::endl;
            return;
        }

        address -= OAM_START;
        _oam[address] = value;
    }
}

uint8_t PPU::read_io_register(uint16_t address) {
    switch (address) {
    case LY: return _current_scanline;
    case LYC: return _compare_scanline;
    case STAT: {
        uint8_t result = 0b10000000;
        result |= ((uint8_t) _mode) & 0b11;
        result = utils::set_bit_value(result, 2, _current_scanline == _compare_scanline);
        result = utils::set_bit_value(result, 3, _mode_interrupt_select[0]);
        result = utils::set_bit_value(result, 4, _mode_interrupt_select[1]);
        result = utils::set_bit_value(result, 5, _mode_interrupt_select[2]);
        result = utils::set_bit_value(result, 6, _compare_scanline_interrupt_select);
        return result;
    }
    case LCDC: {
        uint8_t result = 0;
        result = utils::set_bit_value(result, 0, _bg_window_enable_priority);
        result = utils::set_bit_value(result, 1, _obj_enabled);
        result = utils::set_bit_value(result, 2, _obj_tall);
        result = utils::set_bit_value(result, 3, _bg_tilemap_high);
        result = utils::set_bit_value(result, 4, _bg_tile_data_low);
        result = utils::set_bit_value(result, 5, _window_enabled);
        result = utils::set_bit_value(result, 6, _window_tilemap_high);
        result = utils::set_bit_value(result, 7, _enabled);

        if (!_enabled) {
            _current_scanline = 0;
            _drawing_window = false;
            _window_scanline = 0;
            _dots = 0;
            _draw_pixel_x = 0;
            _mode = LCDDrawMode::HBlank;
        }
        return result;
    }
    case SCY: return _bg_scroll_y;
    case SCX: return _bg_scroll_x;
    case BGP: {
        uint8_t result = 0;
        result |= _bg_palette[0] << 0;
        result |= _bg_palette[1] << 2;
        result |= _bg_palette[2] << 4;
        result |= _bg_palette[3] << 6;
        return result;
    }
    case OBP0: {
        uint8_t result = 0;
        result |= _obj_palettes[0][0] << 0;
        result |= _obj_palettes[0][1] << 2;
        result |= _obj_palettes[0][2] << 4;
        result |= _obj_palettes[0][3] << 6;
        return result;
    }
    case OBP1: {
        uint8_t result = 0;
        result |= _obj_palettes[1][0] << 0;
        result |= _obj_palettes[1][1] << 2;
        result |= _obj_palettes[1][2] << 4;
        result |= _obj_palettes[1][3] << 6;
        return result;
    }
    case WY: {
        return _window_scroll_y;
    }
    case WX: {
        return _window_scroll_x;
    }
    default: return 0xFF;
    }
}

void PPU::write_io_register(uint16_t address, uint8_t value) {
    switch (address) {
    case LY: break;
    case LYC: {
        _compare_scanline = value;
        break;
    }
    case STAT: {
        if (!gb.cgb_mode()) {
            // Check for interrupts, as if all were enabled.
            bool trigger_interrupt = (_current_scanline == _compare_scanline) || (_mode <= 2);
            if (trigger_interrupt && !_stat_blocking) {
                gb.request_interrupt(Interrupts::Stat);
            }
            _stat_blocking = trigger_interrupt;
        }
        _mode_interrupt_select[0] = utils::get_bit_value(value, 3);
        _mode_interrupt_select[1] = utils::get_bit_value(value, 4);
        _mode_interrupt_select[2] = utils::get_bit_value(value, 5);
        _compare_scanline_interrupt_select = utils::get_bit_value(value, 6);
        break;
    }
    case LCDC: {
        _bg_window_enable_priority = utils::get_bit_value(value, 0);
        _obj_enabled = utils::get_bit_value(value, 1);
        _obj_tall = utils::get_bit_value(value, 2);
        _bg_tilemap_high = utils::get_bit_value(value, 3);
        _bg_tile_data_low = utils::get_bit_value(value, 4);
        _window_enabled = utils::get_bit_value(value, 5);
        _window_tilemap_high = utils::get_bit_value(value, 6);
        _enabled = utils::get_bit_value(value, 7);
        break;
    }
    case SCY: {
        _bg_scroll_y = value;
        break;
    }
    case SCX: {
        _bg_scroll_x = value;
        break;
    }
    case BGP: {
        _bg_palette[0] = (value >> 0) & 0b11;
        _bg_palette[1] = (value >> 2) & 0b11;
        _bg_palette[2] = (value >> 4) & 0b11;
        _bg_palette[3] = (value >> 6) & 0b11;
        break;
    }
    case OBP0: {
        _obj_palettes[0][0] = (value >> 0) & 0b11;
        _obj_palettes[0][1] = (value >> 2) & 0b11;
        _obj_palettes[0][2] = (value >> 4) & 0b11;
        _obj_palettes[0][3] = (value >> 6) & 0b11;
        break;
    }
    case OBP1: {
        _obj_palettes[1][0] = (value >> 0) & 0b11;
        _obj_palettes[1][1] = (value >> 2) & 0b11;
        _obj_palettes[1][2] = (value >> 4) & 0b11;
        _obj_palettes[1][3] = (value >> 6) & 0b11;
        break;
    }
    case WY: {
        _window_scroll_y = value;
        break;
    }
    case WX: {
        _window_scroll_x = value;
        break;
    }
    }
}
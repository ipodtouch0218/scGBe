#include "ppu.h"
#include <iostream>
#include <fstream>
#include "gbsystem.h"
#include "registers.h"
#include "utils.h"

constexpr uint32_t SCREEN_COLORS[] = {
    0xFFFFFFFF,
    0xFFAAAAAA,
    0xFF545454,
    0xFF000000
};

PPU::PPU(GBSystem* gb) {
    this->gb = gb;
    gb->address_space[LY] = 0;
    dots = 0;
    draw_pixel_x = 0;
    penalty_dots = 0;
    mode = LCDDrawMode::OAM_SCAN;

    scanline_sprite_buffer.reserve(10);
}

void PPU::tick() {

    if (!enabled()) {
        was_disabled = true;
        return;
    }

    if (was_disabled) {
        // Recovering from being disabled.
        // Guesswork: wait until the next frame to reset everything
        if (gb->frame_cycles == 0) {
            // Next scanline
            dots = 0;
            draw_pixel_x = 0;
            gb->address_space[LY] = 0;
            mode = LCDDrawMode::OAM_SCAN;
            was_disabled = false;
        } else {
            return;
        }
    }

    uint8_t current_scanline = gb->address_space[LY];
    LCDDrawMode::LCDDrawMode previous_draw_mode = mode;

    if (mode == LCDDrawMode::OAM_SCAN) {
        if (dots == OAM_SCAN_DOTS) {
            scanline_sprite_buffer.clear();
            uint8_t sprite_height = 8 + 8 * ((gb->address_space[LCDC] & (1 << 2)) != 0);

            for (int index = 0; index < OAM_MAX_ELEMENTS; index++) {
                OAMEntry* entry = (OAMEntry*) (gb->address_space + OAM_ADDR + (index * sizeof(OAMEntry)));

                uint8_t y_diff = (current_scanline + 16) - entry->y_position;
                if (y_diff < sprite_height) {
                    // On this scanline!
                    scanline_sprite_buffer.push_back(entry);
                    if (scanline_sprite_buffer.size() == MAX_SPRITES_PER_SCANLINE) {
                        break;
                    }
                }
            }

            if (!gb->cgb()) {
                // TODO Sort entries by x-coordinate, instead of OAM order.

            }

            mode = LCDDrawMode::DRAWING;
        }
    }

    if (mode == LCDDrawMode::DRAWING) {
        if (dots == OAM_SCAN_DOTS) {
            penalty_dots = 12; // 12 wasted cycles at the beginning
            penalty_dots += (gb->address_space[SCX] % 8); // Discarding pixels in leftmost tile
        }

        // Out here, used for determining the obj penalties
        uint8_t bg_scroll_x = gb->address_space[SCX];
        uint8_t bg_scroll_y = gb->address_space[SCY];
        uint8_t target_pixel_in_bg_x = bg_scroll_x + draw_pixel_x;
        uint8_t target_pixel_in_bg_y = bg_scroll_y + current_scanline;

        uint8_t tilemap_x = (target_pixel_in_bg_x / 8);
        uint8_t tilemap_y = (target_pixel_in_bg_y / 8);
        uint8_t draw_pixel_of_tile_x = draw_pixel_x % 8;
        uint8_t draw_pixel_of_tile_y = current_scanline % 8;

        OAMEntry* sprite_to_draw = nullptr;
        bool double_height_sprites = (gb->address_space[LCDC] & (1 << 2)) != 0;
        for (OAMEntry* entry : scanline_sprite_buffer) {
            uint8_t x_diff = (draw_pixel_x + 8) - entry->x_position;
            if (x_diff >= 8 && x_diff != -1) {
                continue;
            }
            // Draw this sprite!

            if (x_diff == -1) {
                // 6 dot penalty for retrieving the tile...
                penalty_dots += 6;

                // And additional penalties for the first sprite in the tile
                if (tilemap_x != last_drawn_sprite_tile_x) {
                    uint8_t remaining_tile_pixels = (8 - draw_pixel_of_tile_x);
                    int8_t additional_penalty = remaining_tile_pixels - 2;
                    if (additional_penalty > 0) {
                        penalty_dots += additional_penalty;
                    }
                    last_drawn_sprite_tile_x = tilemap_x;
                }
            }

            if (sprite_to_draw == nullptr) {
                sprite_to_draw = entry;
            }

            // Don't break, since we have to consider other sprites' penalties
        }


        if (penalty_dots) {
            // Don't draw right now, in a penalty
            penalty_dots--;
        } else {
            // Draw!
            int index = ((int) draw_pixel_x) + ((int) current_scanline) * SCREEN_W;

            // Sprites:
            bool drew_sprite = false;
            if (sprite_to_draw) {
                uint8_t x_diff = (draw_pixel_x + 16) - sprite_to_draw->x_position;
                uint8_t y_diff = (current_scanline + 16) - sprite_to_draw->y_position;
                uint8_t tile_index = sprite_to_draw->tile_index;
                if (double_height_sprites) {
                    if (y_diff > 8 ^ sprite_to_draw->y_flip()) {
                        tile_index | 0x01;
                    } else {
                        tile_index & 0xFE;
                    }
                }

                uint8_t draw_pixel_of_sprite_x = x_diff % 8;
                if (sprite_to_draw->x_flip()) {
                    draw_pixel_of_sprite_x = 7 - draw_pixel_of_sprite_x;
                }
                uint8_t draw_pixel_of_sprite_y = y_diff % 8;
                if (sprite_to_draw->y_flip()) {
                    draw_pixel_of_sprite_y = 7 - draw_pixel_of_sprite_y;
                }

                uint16_t tile_addr = TILE_BLOCK0_ADDR + (((uint16_t) tile_index) * 0x10);

                uint8_t graphics_lsb = gb->address_space[tile_addr + (draw_pixel_of_sprite_y * 2)];
                uint8_t graphics_msb = gb->address_space[tile_addr + 1 + (draw_pixel_of_sprite_y * 2)];

                uint8_t tile_pixel_mask = 1 << (7 - draw_pixel_of_sprite_x);
                uint8_t pixel_value = ((graphics_msb & tile_pixel_mask) != 0) << 1 | ((graphics_lsb & tile_pixel_mask) != 0);

                uint8_t obj_palette = gb->address_space[OBP0 + sprite_to_draw->dmg_palette()];
                uint8_t final_color = (obj_palette >> (pixel_value * 2)) & 0b11;

                ((uint32_t*) framebuffer)[index] = SCREEN_COLORS[final_color];
            }

            // Tiles:
            bool lower_addressing_mode = (gb->address_space[LCDC] & (1 << 4)) != 0;

            uint8_t tilemap_index = gb->address_space[(uint16_t) (TILEMAP0_ADDR + tilemap_x) + (tilemap_y * 32)];

            uint16_t tile_addr;
            if (lower_addressing_mode) {
                tile_addr = TILE_BLOCK0_ADDR + ((uint16_t) tilemap_index) * 0x10;
            } else {
                tile_addr = TILE_BLOCK2_ADDR - ((int16_t) tilemap_index) * 0x10;
            }

            uint8_t graphics_lsb = gb->address_space[tile_addr + (draw_pixel_of_tile_y * 2)];
            uint8_t graphics_msb = gb->address_space[tile_addr + 1 + (draw_pixel_of_tile_y * 2)];

            uint8_t tile_pixel_mask = 1 << (7 - draw_pixel_of_tile_x);
            uint8_t pixel_value = ((graphics_msb & tile_pixel_mask) != 0) << 1 | ((graphics_lsb & tile_pixel_mask) != 0);

            if (!sprite_to_draw || (sprite_to_draw->priority() && pixel_value != 0)) {
                // Draw, either over the sprite or there's no sprite at all.
                uint8_t bg_palette = gb->address_space[BGP];
                uint8_t final_color = (bg_palette >> (pixel_value * 2)) & 0b11;

                ((uint32_t*) framebuffer)[index] = SCREEN_COLORS[final_color];
            }


            if (++draw_pixel_x == SCREEN_W) {
                // Done drawing
                mode = LCDDrawMode::HBLANK;
                last_drawn_sprite_tile_x = -1;
                // std::cout << "Entering hblank mode X:" << (int) dots << std::endl;
            }
        }
    }

    dots++;
    if (dots == DOTS_PER_SCANLINE) {
        // Next scanline
        dots = 0;
        draw_pixel_x = 0;
        current_scanline++;
        // std::cout << (int) current_scanline << std::endl;

        if (current_scanline == SCREEN_H) {
            // Entering VBlank
            mode = LCDDrawMode::VBLANK;
            gb->address_space[IF] |= (1 << INT_VBLANK);
            // std::cout << "Entering vblank mode Y:" << (int) gb->cycles << std::endl;

        } else if (mode == LCDDrawMode::HBLANK) {
            // Back to OAM
            mode = LCDDrawMode::OAM_SCAN;
            // std::cout << "Entering OAM SCAN mode" << std::endl;

        } else if (current_scanline == SCANLINES_PER_FRAME) {
            // Next frame
            current_scanline = 0;
            mode = LCDDrawMode::OAM_SCAN;
            // std::cout << "Entering OAM SCAN mode (after VBLANK) " << std::endl;
        }
        gb->address_space[LY] = current_scanline;
    }

    // Status register
    uint8_t old_stat = gb->address_space[STAT];
    uint8_t new_stat = old_stat;
    uint8_t compare_scanline = gb->address_space[LYC];
    new_stat = utils::set_bit_value(new_stat, PPU_MODE, mode & 0b1);
    new_stat = utils::set_bit_value(new_stat, PPU_MODE + 1, (mode & 0b10) >> 1);
    new_stat = utils::set_bit_value(new_stat, LYC_EQ, current_scanline == compare_scanline);
    gb->address_space[STAT] = new_stat;

    // Interrupts
    if ((new_stat & (1 << LYC_INT_SEL)) != 0 && (old_stat & (1 << LYC_EQ) == 0)) {
        // LYC interrupt
        gb->address_space[IF] |= (1 << INT_STAT);
        // std::cout << "LYC Interrupt Requested" << std::endl;
    }
    if (previous_draw_mode != mode && mode != LCDDrawMode::DRAWING) {
        // Check for mode change interrupts
        uint8_t stat_bit = 1 << (MODE0_INT_SEL + mode);
        if ((stat_bit & new_stat) != 0) {
            // std::cout << "Mode " << mode << " Interrupt Requested " << (int) new_stat << std::endl;
            gb->address_space[IF] |= (1 << INT_STAT);
        }
    }
}

bool PPU::enabled() {
    return (gb->address_space[LCDC] & (1 << 7)) != 0;
}
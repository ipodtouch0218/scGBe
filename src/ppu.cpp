#include "ppu.h"

#include <iostream>
#include "gbsystem.h"
#include "registers.h"
#include "utils.h"

PPU::PPU(GBSystem* gb) {
    this->gb = gb;
    gb->address_space[LY] = 0;
    dots = 0;
    draw_pixel_x = 0;
    penalty_dots = 0;
    mode = LCDDrawMode::OAM_SCAN;
}

void PPU::tick() {
    uint8_t current_scanline = gb->address_space[LY];
    LCDDrawMode::LCDDrawMode previous_draw_mode = mode;

    if (mode == LCDDrawMode::OAM_SCAN && dots == OAM_SCAN_DOTS) {
        mode = LCDDrawMode::DRAWING;
        // std::cout << "Entering drawing mode X:" << (int) dots << std::endl;
    }

    if (mode == LCDDrawMode::DRAWING) {
        if (penalty_dots) {
            // Don't draw right now, in a penalty
            penalty_dots--;
        } else {
            // Draw!
            // TODO
            draw_pixel_x++;
            if (draw_pixel_x == SCREEN_W) {
                // Done drawing
                mode = LCDDrawMode::HBLANK;
                // std::cout << "Entering hlank mode X:" << (int) dots << std::endl;
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
            // std::cout << "Entering vblank mode Y:" << (int) current_scanline << std::endl;

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
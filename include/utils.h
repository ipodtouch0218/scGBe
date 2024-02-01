#pragma once
#include <stdint.h>

namespace utils {
    static uint32_t set_bit_value(uint32_t value, uint32_t bit_index, bool bit_value) {
        return (value & ~(1 << bit_index)) | (bit_value << bit_index);
    }
    static bool get_bit_value(uint32_t value, uint32_t bit_index) {
        return (value & (1 << bit_index)) != 0;
    }
}
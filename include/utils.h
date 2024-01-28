#pragma once
#include <stdint.h>

namespace utils {
    uint8_t set_bit_value(uint8_t value, uint8_t bit_index, bool bit_value) {
        return (value & ~(1 << bit_index)) | (bit_value << bit_index);
    }
}
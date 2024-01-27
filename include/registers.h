#pragma once 
#include <stdint.h>

struct Flags {
    bool zero, subtraction, half_carry, carry;

    uint8_t to_byte() const {
        uint8_t result = 0;
        result |= zero << 7;
        result |= subtraction << 6;
        result |= half_carry << 5;
        result |= carry << 4;
        return result;
    }

    static Flags from_byte(uint8_t value) {
        Flags flags;
        flags.zero = (value & (1 << 7)) != 0;
        flags.subtraction = (value & (1 << 6)) != 0;
        flags.half_carry = (value & (1 << 5)) != 0;
        flags.carry = (value & (1 << 4)) != 0;
        return flags;
    }
};

struct Registers {
    uint8_t a, b, c, d, e, h, l;
    uint16_t sp, pc;
    Flags flags;

    // F sepcial register
    uint8_t f() const {
        return flags.to_byte();
    }

    void set_f(uint8_t value) {
        flags = Flags::from_byte(value);
    }

    // AF combined register

    uint16_t af() const {
        return ((uint16_t) a) << 8 | f();
    }

    void set_af(uint16_t value) {
        a = (uint8_t) (value >> 8);
        set_f((uint8_t) value);
    }

    // BC combined register
    uint16_t bc() const {
        return ((uint16_t) b) << 8 | ((uint16_t) c);
    }

    void set_bc(uint16_t value) {
        b = (uint8_t) (value >> 8);
        c = (uint8_t) (value);
    }

    // DE combined register

    uint16_t de() const {
        return ((uint16_t) d) << 8 | ((uint16_t) e);
    }

    void set_de(uint16_t value) {
        d = (uint8_t) (value >> 8);
        e = (uint8_t) (value);
    }

    // HL combined register

    uint16_t hl() const {
        return ((uint16_t) h) << 8 | ((uint16_t) l);
    }

    void set_hl(uint16_t value) {
        h = (uint8_t) (value >> 8);
        l = (uint8_t) (value);
    }

    void clear() {
        a = 0;
        b = 0;
        c = 0;
        d = 0;
        e = 0;
        flags = Flags::from_byte(0);
        h = 0;
        l = 0;
        sp = 0xFFFE;
        pc = 0x100;
    }
};
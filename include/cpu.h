#pragma once
#include <stdint.h>
#include "instructions.h"

class GBSystem;

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

class CPU {
    public:
    GBSystem* gb;
    Registers registers;
    bool ime_flag = false;

    uint8_t wait_ticks = 0;

    CPU(GBSystem* gb);

    void reset();
    void tick();
    uint8_t execute();
    uint8_t check_for_interrupts();

    uint8_t get_register_byte(ByteRegister::ByteRegister target);
    void set_register_byte(ByteRegister::ByteRegister target, uint8_t value);

    uint16_t get_register_word(WordRegister::WordRegister target);
    void set_register_word(WordRegister::WordRegister target, uint16_t value);

    private:
    uint8_t add_byte_with_overflow(uint8_t value, bool add_carry);
    uint8_t sub_byte_with_overflow(uint8_t value, bool sub_carry);

    bool evaluate_condition(Condition::Condition cc);
    void call_function(uint16_t address);
    void return_function();

    void set_all_flags(bool zero, bool subtraction, bool half_carry, bool carry);
};
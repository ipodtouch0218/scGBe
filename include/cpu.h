#pragma once
#include <stdint.h>
#include "instructions.h"
#include "gbcomponent.h"
#include "utils.h"

struct Flags {
    bool zero, subtraction, half_carry, carry;

    uint8_t to_byte() const {
        uint8_t result = 0;
        result = utils::set_bit_value(result, 7, zero);
        result = utils::set_bit_value(result, 6, subtraction);
        result = utils::set_bit_value(result, 5, half_carry);
        result = utils::set_bit_value(result, 4, carry);
        return result;
    }

    static Flags from_byte(uint8_t value) {
        Flags flags;
        flags.zero = utils::get_bit_value(value, 7);
        flags.subtraction = utils::get_bit_value(value, 6);
        flags.half_carry = utils::get_bit_value(value, 5);
        flags.carry = utils::get_bit_value(value, 4);
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

class CPU : public GBComponent {

    private:
    uint8_t _interrupt_flags = 0;
    bool _ime_flag = false;
    bool _halted = false;

    public:
    Registers registers;
    bool ime_enable_next_cycle = false;
    bool halt_bug = false;

    uint8_t wait_ticks = 0;

    CPU(GBSystem& gb_param);

    void tick();
    void reset();

    uint8_t execute();
    uint8_t check_for_interrupts();

    uint8_t read_io_register(uint16_t address);
    void write_io_register(uint16_t address, uint8_t value);

    private:
    uint8_t read_cpu_register_byte(ByteRegister::ByteRegister target);
    void read_cpu_register_byte(ByteRegister::ByteRegister target, uint8_t value);

    uint16_t read_cpu_register_word(WordRegister::WordRegister target);
    void read_cpu_register_word(WordRegister::WordRegister target, uint16_t value);

    uint8_t add_byte_with_overflow(uint8_t value, bool add_carry);
    uint8_t sub_byte_with_overflow(uint8_t value, bool sub_carry);

    bool evaluate_condition(Condition::Condition cc);
    void call_function(uint16_t address);
    void return_function();

    void set_all_flags(bool zero, bool subtraction, bool half_carry, bool carry);
};
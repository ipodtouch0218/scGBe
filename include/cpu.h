#pragma once
#include <stdint.h>

#include "instructions.h"
#include "registers.h"

constexpr uint16_t IME_ADDR = 0xFFFF;

class CPU {
    public:
    Registers registers;
    uint8_t address_space[0xFFFF];

    void reset();
    void execute();

    uint8_t get_register_byte(ByteRegister::ByteRegister target);
    void set_register_byte(ByteRegister::ByteRegister target, uint8_t value);

    uint16_t get_register_word(WordRegister::WordRegister target);
    void set_register_word(WordRegister::WordRegister target, uint16_t value);

    uint8_t get_address_space_byte(uint16_t addr);
    void set_address_space_byte(uint16_t addr, uint8_t value);

    private:
    uint8_t add_byte_with_overflow(uint8_t value, bool add_carry);
    uint8_t sub_byte_with_overflow(uint8_t value, bool sub_carry);

    bool evaluate_condition(Condition::Condition cc);
    void call_function(uint16_t address);
    void return_function();

    void set_flags(bool zero, bool subtraction, bool half_carry, bool carry);
};
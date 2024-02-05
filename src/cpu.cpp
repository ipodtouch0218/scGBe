#include "cpu.h"
#include <cstring>
#include <strings.h>
#include <stdlib.h>
#include "gbsystem.h"
#include "gbcomponent.h"
#include "registers.h"

#include <fstream>
#include <iostream>
#include <iomanip>

CPU::CPU(GBSystem& gb_param)
    : GBComponent::GBComponent(gb_param)
{
    gb.add_register_callbacks(this, {IF});
}

void CPU::tick() {
    if (wait_ticks == 0) {
        bool ime_enable_was_active = _ime_enable_next_cycle;
        wait_ticks = execute();

        if (ime_enable_was_active && _ime_enable_next_cycle) {
            _ime_flag = true;
            _ime_enable_next_cycle = false;
        }
    }
    wait_ticks--;
}

uint8_t CPU::execute() {

    uint8_t interrupt_cycles = check_for_interrupts();
    if (interrupt_cycles != 0) {
        return interrupt_cycles;
    }

    if (_halted) {
        return 1;
    }

    uint8_t opcode = gb.read_address(registers.pc++);
    if (halt_bug) {
        registers.pc--;
        halt_bug = false;
    }

    switch (opcode) {
    case 0x00: {
        // NOP: 0b00000000
        return 1;
    }

    case 0x01:
    case 0x11:
    case 0x21:
    case 0x31: {
        // LD WORD REGISTER FROM IMMEDIATE: 0b00xx0001
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);

        uint8_t lsb = gb.read_address(registers.pc++);
        uint8_t msb = gb.read_address(registers.pc++);
        uint16_t result = (((uint16_t) msb) << 8) + lsb;

        read_cpu_register_word(target, result);
        return 3;
    }

    case 0x02:
    case 0x12:
    case 0x22:
    case 0x32: {
        // LD TO (INDIRECT) WORD REGISTER FROM A REGISTER: 0b00xx0010
        WordRegister::WordRegister target;
        uint16_t auto_increment = 0;
        switch ((opcode & 0b00110000) >> 4) {
        case 0: target = WordRegister::BC; break;
        case 1: target = WordRegister::DE; break;
        case 2: target = WordRegister::HL; auto_increment = 1; break;
        case 3: target = WordRegister::HL; auto_increment = -1; break;
        }

        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        uint16_t addr = read_cpu_register_word(target);
        if (auto_increment != 0) {
            read_cpu_register_word(target, addr + auto_increment);
        }
        gb.write_address(addr, value);
        return 2;
    }

    case 0x03:
    case 0x13:
    case 0x23:
    case 0x33: {
        // INC WORD REGISTER: 0b00xx0011
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);
        uint16_t value = read_cpu_register_word(target) + 1;
        read_cpu_register_word(target, value);
        return 2;
    }

    case 0x04:
    case 0x0C:
    case 0x14:
    case 0x1C:
    case 0x24:
    case 0x2C:
    case 0x34:
    case 0x3C: {
        // INC BYTE REGISTER: 0b00xxx100
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00111000) >> 3);
        uint8_t value = read_cpu_register_byte(target);
        uint8_t result = value + 1;

        registers.flags.zero = result == 0;
        registers.flags.subtraction = false;
        registers.flags.half_carry = (value & 0xF) == 0xF;
        read_cpu_register_byte(target, result);
        return 1 + ((target == ByteRegister::HL_INDIRECT) * 2);
    }

    case 0x05:
    case 0x0D:
    case 0x15:
    case 0x1D:
    case 0x25:
    case 0x2D:
    case 0x35:
    case 0x3D: {
        // DEC BYTE REGISTER: 0b00xxx101
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00111000) >> 3);
        uint8_t value = read_cpu_register_byte(target);
        uint8_t result = value - 1;

        registers.flags.zero = result == 0;
        registers.flags.subtraction = true;
        registers.flags.half_carry = (value & 0xF) == 0;
        read_cpu_register_byte(target, result);
        return 1 + ((target == ByteRegister::HL_INDIRECT) * 2);
    }

    case 0x06:
    case 0x0E:
    case 0x16:
    case 0x1E:
    case 0x26:
    case 0x2E:
    case 0x36:
    case 0x3E: {
        // LD TO BYTE REGISTER FROM IMMEDIATE VALUE: 0b00xxx110
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00111000) >> 3);
        uint8_t value = gb.read_address(registers.pc++);
        read_cpu_register_byte(target, value);
        return 2 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0x07: {
        // ROTATE LEFT ACCUMULATOR: 0b00000111
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b10000000) != 0;
        value <<= 1;
        value |= shifted_out_bit;

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        return 1;
    }

    case 0x08: {
        // LD TO WORD (INDIRECT) IMMEDIATE VALUE FROM REGISTER SP: 0b00001000
        uint8_t addr_lsb = gb.read_address(registers.pc++);
        uint8_t addr_msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) addr_msb) << 8) + addr_lsb;

        uint8_t sp_lsb = registers.sp & 0xFF;
        uint8_t sp_msb = (registers.sp >> 8) & 0xFF;

        gb.write_address(addr, sp_lsb);
        gb.write_address(addr + 1, sp_msb);
        return 5;
    }

    case 0x09:
    case 0x19:
    case 0x29:
    case 0x39: {
        // ADD WORD: 0b00xx1001
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);
        uint16_t current_value = read_cpu_register_word(WordRegister::HL);
        uint16_t value = read_cpu_register_word(target);

        uint16_t result = current_value + value;
        read_cpu_register_word(WordRegister::HL, result);
        registers.flags.subtraction = false;
        registers.flags.half_carry = ((value & 0xFFF) + (current_value & 0xFFF)) & 0x1000;
        registers.flags.carry = result < value || result < current_value;
        return 2;
    }

    case 0x0A:
    case 0x1A:
    case 0x2A:
    case 0x3A: {
        // LD TO (INDIRECT) WORD REGISTER FROM BYTE REGISTER A : 0b00xx0110
        WordRegister::WordRegister source;
        uint16_t auto_increment = 0;
        switch ((opcode & 0b00110000) >> 4) {
        case 0: source = WordRegister::BC; break;
        case 1: source = WordRegister::DE; break;
        case 2: source = WordRegister::HL; auto_increment = 1; break;
        case 3: source = WordRegister::HL; auto_increment = -1; break;
        }

        uint16_t addr = read_cpu_register_word(source);
        uint16_t value = gb.read_address(addr);
        if (auto_increment != 0) {
            read_cpu_register_word(source, addr + auto_increment);
        }
        read_cpu_register_byte(ByteRegister::A, value);
        return 2;
    }

    case 0x0B:
    case 0x1B:
    case 0x2B:
    case 0x3B: {
        // DEC WORD REGISTER: 0b00xx1011
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);
        uint16_t value = read_cpu_register_word(target) - 1;
        read_cpu_register_word(target, value);
        return 2;
    }

    case 0x0F: {
        // ROTATE RIGHT ACCUMULATOR: 0b00001111
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b00000001) != 0;
        value >>= 1;
        value |= (shifted_out_bit << 7);

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        return 1;
    }

    case 0x10: {
        // STOP: 0b00010000
        // TODO
        // throw std::runtime_error("Stop!");
        gb.write_address(DIV, 0, true);
        return 1;
    }

    case 0x17: {
        // ROTATE LEFT ACCUMULATOR (THROUGH CARRY): 0b00010111
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b10000000) != 0;
        value <<= 1;
        value |= registers.flags.carry;

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        return 1;
    }

    case 0x18: {
        // RELATIVE JUMP: 0b00011000
        int8_t offset = (int8_t) gb.read_address(registers.pc++);
        registers.pc += offset;
        return 3;
    }

    case 0x1F: {
        // ROTATE RIGHT ACCUMULATOR (THROUGH CARRY): 0b00011111
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b00000001) != 0;
        value >>= 1;
        value |= (registers.flags.carry << 7);

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        return 1;
    }

    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38: {
        // RELATIVE JUMP (CONDITIONAL): 0b001xx000
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        int8_t offset = (int8_t) gb.read_address(registers.pc++);

        if (evaluate_condition(cc)) {
            registers.pc += offset;
            return 3;
        }

        return 2;
    }

    case 0x27: {
        // DECIMAL ADJUST ACCUMULATOR: 0b00100111
        // Source: https://forums.nesdev.org/viewtopic.php?t=15944
        uint16_t value = read_cpu_register_byte(ByteRegister::A);

        if (!registers.flags.subtraction) {
            if (registers.flags.half_carry || (value & 0x0F) > 0x09) {
                value += 0x06;
            }
            if (registers.flags.carry || value > 0x9F) {
                value += 0x60;
            }
        } else {
            if (registers.flags.half_carry) {
                value -= 0x06;
                if (!registers.flags.carry) {
                    value &= 0xFF;
                }
            }
            if (registers.flags.carry) {
                value -= 0x60;
            }
        }

        registers.flags.zero = !(value & 0xFF);
        registers.flags.carry |= (value > 0xFF);
        registers.flags.half_carry = false;
        read_cpu_register_byte(ByteRegister::A, value);
        return 1;
    }

    case 0x2F: {
        // COMPLEMENT ACCUMULATOR: 0b00111111
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        read_cpu_register_byte(ByteRegister::A, ~value);

        registers.flags.subtraction = true;
        registers.flags.half_carry = true;
        return 1;
    }

    case 0x37: {
        // SET CARRY FLAG: 0b00110111
        registers.flags.subtraction = false;
        registers.flags.half_carry = false;
        registers.flags.carry = true;
        return 1;
    }

    case 0x3F: {
        // COMPLEMENT CARRY FLAG: 0b00111111
        registers.flags.subtraction = false;
        registers.flags.half_carry = false;
        registers.flags.carry = !registers.flags.carry;
        return 1;
    }

    case 0x40 ... 0x75:
    case 0x77 ... 0x7F: {
        // LOAD BYTE REGISTER FROM BYTE REGISTER: 0b01xxxyyy
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00111000) >> 3);
        ByteRegister::ByteRegister source = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = read_cpu_register_byte(source);
        read_cpu_register_byte(target, value);
        return 1 + (source == ByteRegister::HL_INDIRECT || target == ByteRegister::HL_INDIRECT);
    }

    case 0x76: {
        // HALT: 0b01110110
        _halted = true;

        // Halt bug
        if (!_ime_flag && (gb.read_address(IE, true) & _interrupt_flags)) {
            _halted = _ime_enable_next_cycle;
            halt_bug = true;
        }
        return 1;
    }

    case 0x80 ... 0x8F: {
        // ADD: 0b1000cxxx (c = use carry)
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t value = read_cpu_register_byte(target);
        uint8_t result = add_byte_with_overflow(value, use_carry);
        read_cpu_register_byte(ByteRegister::A, result);
        return 1 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0x90 ... 0x9F: {
        // SUBTRACT: 0b1001cxxx (c = use carry)
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t value = read_cpu_register_byte(target);
        uint8_t result = sub_byte_with_overflow(value, use_carry);
        read_cpu_register_byte(ByteRegister::A, result);
        return 1 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0xA0 ... 0xA7: {
        // AND: 0b10100xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = read_cpu_register_byte(ByteRegister::A) & read_cpu_register_byte(target);

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, true, false);
        return 1 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0xA8 ... 0xAF: {
        // XOR: 0b10101xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = read_cpu_register_byte(ByteRegister::A) ^ read_cpu_register_byte(target);

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        return 1 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0xB0 ... 0xB7: {
        // OR: 0b10110xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = read_cpu_register_byte(ByteRegister::A) | read_cpu_register_byte(target);

        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        return 1 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0xB8 ... 0xBF: {
        // COMPARE WITH REGISTER A: 0b10111xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = read_cpu_register_byte(target);
        sub_byte_with_overflow(value, false);
        return 1 + (target == ByteRegister::HL_INDIRECT);
    }

    case 0xC0:
    case 0xC8:
    case 0xD0:
    case 0xD8: {
        // CONDITIONAL RETURN FROM FUNC: 0b110cc000
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        if (evaluate_condition(cc)) {
            return_function();
            return 5;
        }
        return 2;
    }

    case 0xC1:
    case 0xD1:
    case 0xE1:
    case 0xF1: {
        // POP INTO WORD REGISTER: 0b11xx0001
        WordRegister::WordRegister target;
        switch ((opcode & 0b00110000) >> 4) {
        case 0: target = WordRegister::BC; break;
        case 1: target = WordRegister::DE; break;
        case 2: target = WordRegister::HL; break;
        case 3: target = WordRegister::AF; break;
        default: throw std::invalid_argument("Invalid WordRegister target for POP");
        }

        uint8_t lsb = gb.read_address(registers.sp++);
        uint8_t msb = gb.read_address(registers.sp++);
        uint16_t value = (((uint16_t) msb) << 8) | lsb;
        read_cpu_register_word(target, value);
        return 3;
    }

    case 0xC2:
    case 0xCA:
    case 0xD2:
    case 0xDA: {
        // CONDITIONAL ABSOLUTE JUMP: 0b110cc010
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        uint8_t lsb = gb.read_address(registers.pc++);
        uint8_t msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;
        if (evaluate_condition(cc)) {
            registers.pc = addr;
            return 4;
        }
        return 3;
    }

    case 0xC3: {
        // UNCONDITIONAL ABSOLUTE JUMP: 0b11000011
        uint8_t lsb = gb.read_address(registers.pc++);
        uint8_t msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;
        registers.pc = addr;
        return 4;
    }

    case 0xC4:
    case 0xCC:
    case 0xD4:
    case 0xDC: {
        // CONDITIONAL CALL: 0b110cc100
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        uint8_t addr_lsb = gb.read_address(registers.pc++);
        uint8_t addr_msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) addr_msb) << 8) | addr_lsb;
        if (evaluate_condition(cc)) {
            call_function(addr);
            return 6;
        }
        return 3;
    }

    case 0xC5:
    case 0xD5:
    case 0xE5:
    case 0xF5: {
        // PUSH WORD REGISTER: 0b11xx0101
        WordRegister::WordRegister target;
        switch ((opcode & 0b00110000) >> 4) {
        case 0: target = WordRegister::BC; break;
        case 1: target = WordRegister::DE; break;
        case 2: target = WordRegister::HL; break;
        case 3: target = WordRegister::AF; break;
        default: throw std::invalid_argument("Invalid WordRegister target for PUSH");
        }

        uint16_t value = read_cpu_register_word(target);
        uint8_t msb = (value >> 8) & 0xFF;
        uint8_t lsb = value & 0xFF;

        gb.write_address(--registers.sp, msb);
        gb.write_address(--registers.sp, lsb);
        return 4;
    }

    case 0xC6:
    case 0xCE: {
        // ADD IMMEDIATE: 0b1100c110 (c = use carry)
        uint8_t value = gb.read_address(registers.pc++);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t result = add_byte_with_overflow(value, use_carry);
        read_cpu_register_byte(ByteRegister::A, result);
        return 2;
    }

    case 0xC7:
    case 0xCF:
    case 0xD7:
    case 0xDF:
    case 0xE7:
    case 0xEF:
    case 0xF7:
    case 0xFF: {
        // RESTART / CALL FUNCTION (implied): 0b11xxx111
        uint8_t offset = ((opcode & 0b00111000) >> 3);
        uint16_t addr = RST_VECTORS + (offset * 0x08);
        call_function(addr);

        return 4;
    }

    case 0xC9: {
        // UNCONDITIONAL RETURN FROM FUNC: 0b11001001
        return_function();
        return 4;
    }

    case 0xCB: {
        // CB: 0b11001011
        uint8_t second_opcode = gb.read_address(registers.pc++);
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) (second_opcode & 0b00000111);

        switch (second_opcode) {
        case 0x00 ... 0x07: {
            // ROTATE REGISTER LEFT
            uint8_t value = read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b10000000) != 0;
            value <<= 1;
            value |= shifted_out_bit;

            read_cpu_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x08 ... 0x0F: {
            // ROTATE REGISTER RIGHT
            uint8_t value = read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            value |= (shifted_out_bit << 7);

            read_cpu_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x10 ... 0x17: {
            // ROTATE REGISTER LEFT THROUGH CARRY
            uint8_t value = read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b10000000) != 0;
            value <<= 1;
            value |= registers.flags.carry;

            read_cpu_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x18 ... 0x1F: {
            // ROTATE REGISTER RIGHT  THROUGH CARRY
            uint8_t value = read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            value |= (registers.flags.carry << 7);

            read_cpu_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x20 ... 0x27: {
            // ARITHMETIC SHIFT LEFT
            int8_t value = (int8_t) read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b10000000) != 0;
            value <<= 1;
            read_cpu_register_byte(target, (uint8_t) value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x28 ... 0x2F: {
            // ARITHMETIC SHIFT RIGHT
            int8_t value = (int8_t) read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            read_cpu_register_byte(target, (uint8_t) value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x30 ... 0x37: {
            // SWAP
            uint8_t value = read_cpu_register_byte(target);
            uint8_t lower_nibble = value & 0xF;
            value >>= 4;
            value |= (lower_nibble << 4);
            read_cpu_register_byte(target, value);
            set_all_flags(value == 0, false, false, false);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x38 ... 0x3F: {
            // LOGICAL SHIFT RIGHT
            uint8_t value = read_cpu_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            read_cpu_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0x40 ... 0x7F: {
            // TEST BIT IN REGISTER: 0b01bbbxxx (b = bit)
            uint8_t bit = (second_opcode & 0b00111000) >> 3;
            uint8_t mask = (1 << bit);
            uint8_t value = read_cpu_register_byte(target) & mask;
            registers.flags.zero = value == 0;
            registers.flags.subtraction = false;
            registers.flags.half_carry = true;
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 1);
        }
        case 0x80 ... 0xBF: {
            // RESET BIT IN REGISTER 0b10bbbxxx (b = bit)
            uint8_t bit = (second_opcode & 0b00111000) >> 3;
            uint8_t mask = ~(1 << bit);
            uint8_t value = read_cpu_register_byte(target) & mask;
            read_cpu_register_byte(target, value);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        case 0xC0 ... 0xFF: {
            // SET BIT IN REGISTER: 0b11bbbxxx (b = bit)
            uint8_t bit = (second_opcode & 0b00111000) >> 3;
            uint8_t mask = (1 << bit);
            uint8_t value = read_cpu_register_byte(target) | mask;
            read_cpu_register_byte(target, value);
            return 2 + ((target == ByteRegister::HL_INDIRECT) * 2);
        }
        }
        return 2;
    }

    case 0xCD: {
        // UNCONDITIOANL CALL: 0b11001101
        uint8_t addr_lsb = gb.read_address(registers.pc++);
        uint8_t addr_msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) addr_msb) << 8) | addr_lsb;
        call_function(addr);
        return 6;
    }

    case 0xD6:
    case 0xDE: {
        // SUBTRACT IMMEDIATE: 0b1101c110 (c = use carry)
        uint8_t value = gb.read_address(registers.pc++);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t result = sub_byte_with_overflow(value, use_carry);
        read_cpu_register_byte(ByteRegister::A, result);
        return 2;
    }

    case 0xD9: {
        // RETURN FROM INTERRUPT
        _ime_flag = true;
        return_function();
        return 4;
    }

    case 0xE0: {
        // LD (0xFF00+IMMEDIATE) FROM ACCUMULATOR: 0b11100000
        uint8_t offset = gb.read_address(registers.pc++);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        gb.write_address(addr, value);
        return 3;
    }

    case 0xE2: {
        // LD (0xFF00+C) FROM ACCUMULATOR: 0b11100010
        uint8_t offset = read_cpu_register_byte(ByteRegister::C);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        gb.write_address(addr, value);
        return 2;
    }

    case 0xE6: {
        // AND IMMEDIATE: 0b11100110
        uint8_t value = gb.read_address(registers.pc++) & read_cpu_register_byte(ByteRegister::A);
        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, true, false);
        return 2;
    }

    case 0xE8: {
        // ADD SP,e
        int8_t value = gb.read_address(registers.pc++);
        uint16_t current_value = read_cpu_register_word(WordRegister::SP);
        uint16_t result = current_value + value;
        read_cpu_register_word(WordRegister::SP, result);

        bool half_carry = ((value & 0xF) + (current_value & 0xF)) & 0x10;
        bool carry = ((value & 0xFF) + (current_value & 0xFF)) & 0x100;
        set_all_flags(false, false, half_carry, carry);
        return 4;
    }

    case 0xE9: {
        // UNCONDITIONAL JUMP TO HL: 0b11101001
        registers.pc = read_cpu_register_word(WordRegister::HL);
        return 1;
    }

    case 0xEA: {
        // LOAD ACCUMULATOR TO IMMEDIATE ADDR: 0b11101010
        uint8_t lsb = gb.read_address(registers.pc++);
        uint8_t msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;

        uint8_t value = read_cpu_register_byte(ByteRegister::A);
        gb.write_address(addr, value);
        return 4;
    }

    case 0xEE: {
        // XOR IMMEDIATE: 0b11101110
        uint8_t value = gb.read_address(registers.pc++) ^ read_cpu_register_byte(ByteRegister::A);
        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        return 2;
    }

    case 0xF0: {
        // LOAD ACCUMULATOR FROM (0xFF00+IMMEDIATE): 0b11110000
        uint8_t offset = gb.read_address(registers.pc++);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = gb.read_address(addr);
        read_cpu_register_byte(ByteRegister::A, value);
        return 3;
    }

    case 0xF2: {
        // LOAD ACCUMULATOR FROM (0xFF00+C): 0b11110010
        uint8_t offset = read_cpu_register_byte(ByteRegister::C);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = gb.read_address(addr);
        read_cpu_register_byte(ByteRegister::A, value);
        return 2;
    }

    case 0xF3: {
        // DISABLE INTERRUPTS: 0b11110011
        _ime_flag = false;
        _ime_enable_next_cycle = false;
        return 1;
    }

    case 0xF6: {
        // OR IMMEDIATE: 0b11110110
        uint8_t value = gb.read_address(registers.pc++) | read_cpu_register_byte(ByteRegister::A);
        read_cpu_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        return 2;
    }

    case 0xF8: {
        // LOAD HL WITH SP+offset
        int8_t value = (int8_t) gb.read_address(registers.pc++);
        uint16_t current_value = read_cpu_register_word(WordRegister::SP);
        uint16_t result = current_value + value;
        read_cpu_register_word(WordRegister::HL, result);

        bool half_carry = ((value & 0xF) + (current_value & 0xF)) & 0x10;
        bool carry = ((value & 0xFF) + (current_value & 0xFF)) & 0x100;
        set_all_flags(false, false, half_carry, carry);
        return 3;
    }

    case 0xF9: {
        // LOAD SP FROM HL WORD
        uint16_t value = read_cpu_register_word(WordRegister::HL);
        read_cpu_register_word(WordRegister::SP, value);
        return 2;
    }

    case 0xFA: {
        // LOAD ACCUMULATOR ABSOLUTE: 0b11111010
        uint8_t lsb = gb.read_address(registers.pc++);
        uint8_t msb = gb.read_address(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;
        uint8_t value = gb.read_address(addr);
        read_cpu_register_byte(ByteRegister::A, value);
        return 4;
    }

    case 0xFB: {
        // ENABLE INTERRUPTS: 0b11111011
        _ime_enable_next_cycle = true;
        return 1;
    }

    case 0xFE: {
        // COMPARE IMMEDIATE WITH ACCUMULATOR: 0b11111110
        uint8_t value = gb.read_address(registers.pc++);
        sub_byte_with_overflow(value, false);
        return 2;
    }

    default: {
        std::cerr << "Unknown opcode! 0x" << std::hex << std::uppercase << std::setw(2) << (int) opcode << std::endl;
        std::cerr << std::hex << std::uppercase << std::setfill('0') <<
            "A:"       << std::setw(2) << (int) registers.a <<
            " B:"      << std::setw(2) << (int) registers.b <<
            " C:"      << std::setw(2) << (int) registers.c <<
            " D:"      << std::setw(2) << (int) registers.d <<
            " E:"      << std::setw(2) << (int) registers.e <<
            " F:"      << std::setw(2) << (int) registers.flags.to_byte() <<
            " H:"      << std::setw(2) << (int) registers.h <<
            " L:"      << std::setw(2) << (int) registers.l <<
            " SP:"     << std::setw(4) << (int) registers.sp <<
            " PC:"     << std::setw(4) << (int) registers.pc <<
            " PCMEM: " << std::setw(2) << (int) gb.read_address(registers.pc-3) <<
            ","        << std::setw(2) << (int) gb.read_address(registers.pc-2) <<
            ","        << std::setw(2) << (int) gb.read_address(registers.pc-1) <<
            ","        << std::setw(2) << (int) gb.read_address(registers.pc+0) <<
            ","        << std::setw(2) << (int) gb.read_address(registers.pc+1) <<
            ","        << std::setw(2) << (int) gb.read_address(registers.pc+2) <<
            ","        << std::setw(2) << (int) gb.read_address(registers.pc+3) <<
            std::endl;
        throw std::invalid_argument("Unknown opcode " + opcode);
        return 1;
    }
    }
}

uint8_t CPU::read_io_register(uint16_t address) {
    switch (address) {
    case IF: return ~0x1F | _interrupt_flags;
    default: return 0xFF;
    }
}

void CPU::write_io_register(uint16_t address, uint8_t value) {
    switch (address) {
    case IF: _interrupt_flags = value & 0x1F; break;
    default: break;
    }
}

uint8_t CPU::check_for_interrupts() {
    uint8_t ie = gb.read_address(IE, true);
    if (!_ime_flag) {
        if (_halted && (ie & _interrupt_flags & 0x1F)) {
            _halted = false;
        }
        return 0;
    }

    uint8_t pending_interrupts = ie & _interrupt_flags & 0x1F;
    int lowest_set_bit_index = __builtin_ctz(pending_interrupts);
    if (lowest_set_bit_index <= 4) {
        uint16_t addr = INTERRUPT_VECTORS + lowest_set_bit_index * 0x8;
        _ime_flag = false;
        _halted = false;
        call_function(addr);
        _interrupt_flags = 0;
        return 5;
    }

    return 0;
}

void CPU::set_all_flags(bool zero, bool subtraction, bool half_carry, bool carry) {
    registers.flags.zero = zero;
    registers.flags.subtraction = subtraction;
    registers.flags.half_carry = half_carry;
    registers.flags.carry = carry;
}

uint8_t CPU::read_cpu_register_byte(ByteRegister::ByteRegister target) {
    switch (target) {
    case ByteRegister::A: return registers.a;
    case ByteRegister::B: return registers.b;
    case ByteRegister::C: return registers.c;
    case ByteRegister::D: return registers.d;
    case ByteRegister::E: return registers.e;
    case ByteRegister::H: return registers.h;
    case ByteRegister::L: return registers.l;
    case ByteRegister::HL_INDIRECT: return gb.read_address(registers.hl());
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

void CPU::read_cpu_register_byte(ByteRegister::ByteRegister target, uint8_t value) {
    switch (target) {
    case ByteRegister::A: registers.a = value; break;
    case ByteRegister::B: registers.b = value; break;
    case ByteRegister::C: registers.c = value; break;
    case ByteRegister::D: registers.d = value; break;
    case ByteRegister::E: registers.e = value; break;
    case ByteRegister::H: registers.h = value; break;
    case ByteRegister::L: registers.l = value; break;
    case ByteRegister::HL_INDIRECT: gb.write_address(registers.hl(), value); break;
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

uint16_t CPU::read_cpu_register_word(WordRegister::WordRegister target) {
    switch (target) {
    case WordRegister::BC: return registers.bc();
    case WordRegister::DE: return registers.de();
    case WordRegister::HL: return registers.hl();
    case WordRegister::SP: return registers.sp;
    case WordRegister::AF: return registers.af();
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

void CPU::read_cpu_register_word(WordRegister::WordRegister target, uint16_t value) {
    switch (target) {
    case WordRegister::BC: registers.set_bc(value); break;
    case WordRegister::DE: registers.set_de(value); break;
    case WordRegister::HL: registers.set_hl(value); break;
    case WordRegister::SP: registers.sp = value; break;
    case WordRegister::AF: registers.set_af(value); break;
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

uint8_t CPU::add_byte_with_overflow(uint8_t value, bool add_carry) {
    uint8_t current_value = registers.a;
    uint8_t carry_value = add_carry & registers.flags.carry;
    uint8_t result = current_value + value + carry_value;

    bool half_carry = ((current_value & 0xF) + (value & 0xF) + carry_value) & 0x10;
    bool carry = result < (value + carry_value) || result < (current_value + carry_value);
    set_all_flags(result == 0, false, half_carry, carry);
    return result;
}

uint8_t CPU::sub_byte_with_overflow(uint8_t value, bool sub_carry) {
    uint8_t current_value = registers.a;
    uint8_t carry_value = sub_carry & registers.flags.carry;
    uint8_t result = current_value - value - carry_value;

    bool half_carry = ((current_value & 0xF) - (value & 0xF) - carry_value) & 0x10;
    bool carry = current_value < (value + carry_value);
    set_all_flags(result == 0, true, half_carry, carry);
    return result;
}

bool CPU::evaluate_condition(Condition::Condition cc) {
    switch (cc) {
    case Condition::NONZERO: return !registers.flags.zero;
    case Condition::ZERO: return registers.flags.zero;
    case Condition::NOCARRY: return !registers.flags.carry;
    case Condition::CARRY: return registers.flags.carry;
    default: throw std::invalid_argument("Unknown conditional value " + cc);
    }
}

void CPU::call_function(uint16_t address) {
    uint8_t pc_msb = (registers.pc >> 8) & 0xFF;
    uint8_t pc_lsb = registers.pc & 0xFF;
    gb.write_address(--registers.sp, pc_msb);
    gb.write_address(--registers.sp, pc_lsb);

    registers.pc = address;
}

void CPU::return_function() {
    uint8_t lsb = gb.read_address(registers.sp++);
    uint8_t msb = gb.read_address(registers.sp++);
    uint16_t addr = (((uint16_t) msb) << 8) | lsb;
    registers.pc = addr;
}

void CPU::reset() {
    registers.clear();

    // Post-bootrom state:
    registers.a = 0x01;
    registers.flags = Flags::from_byte(0xB0);
    registers.b = 0x00;
    registers.c = 0x13;
    registers.d = 0x00;
    registers.e = 0xD8;
    registers.h = 0x01;
    registers.l = 0x4D;
    registers.pc = 0x0100;
    registers.sp = 0xFFFE;

    // TODO: registers.
}
#include "cpu.h"
#include <iostream>
#include <iomanip>
#include <cstring>

void CPU::set_all_flags(bool zero, bool subtraction, bool half_carry, bool carry) {
    registers.flags.zero = zero;
    registers.flags.subtraction = subtraction;
    registers.flags.half_carry = half_carry;
    registers.flags.carry = carry;
}

uint8_t CPU::get_register_byte(ByteRegister::ByteRegister target) {
    switch (target) {
    case ByteRegister::A: return registers.a;
    case ByteRegister::B: return registers.b;
    case ByteRegister::C: return registers.c;
    case ByteRegister::D: return registers.d;
    case ByteRegister::E: return registers.e;
    case ByteRegister::H: return registers.h;
    case ByteRegister::L: return registers.l;
    case ByteRegister::HL_INDIRECT: return get_address_space_byte(registers.hl());
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

void CPU::set_register_byte(ByteRegister::ByteRegister target, uint8_t value) {
    switch (target) {
    case ByteRegister::A: registers.a = value; break;
    case ByteRegister::B: registers.b = value; break;
    case ByteRegister::C: registers.c = value; break;
    case ByteRegister::D: registers.d = value; break;
    case ByteRegister::E: registers.e = value; break;
    case ByteRegister::H: registers.h = value; break;
    case ByteRegister::L: registers.l = value; break;
    case ByteRegister::HL_INDIRECT: set_address_space_byte(registers.hl(), value); break;
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

uint16_t CPU::get_register_word(WordRegister::WordRegister target) {
    switch (target) {
    case WordRegister::BC: return registers.bc();
    case WordRegister::DE: return registers.de();
    case WordRegister::HL: return registers.hl();
    case WordRegister::SP: return registers.sp;
    case WordRegister::AF: return registers.af();
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

void CPU::set_register_word(WordRegister::WordRegister target, uint16_t value) {
    switch (target) {
    case WordRegister::BC: registers.set_bc(value); break;
    case WordRegister::DE: registers.set_de(value); break;
    case WordRegister::HL: registers.set_hl(value); break;
    case WordRegister::SP: registers.sp = value; break;
    case WordRegister::AF: registers.set_af(value); break;
    default: throw std::invalid_argument("Invalid register target " + target);
    }
}

uint8_t CPU::get_address_space_byte(uint16_t addr) {
    if (addr >= 0xE000 && addr <= 0xFDFF) {
        // ERAM support:
        addr -= 0x2000;
    }

    // std::cout << "[R] 0x" << std::hex << (int) addr << " -> " << (int) address_space[addr] << std::endl;
    return address_space[addr];
}

void CPU::set_address_space_byte(uint16_t addr, uint8_t value) {
    if (addr <= 0x7FFF) {
        // Protect ROM
        throw std::runtime_error("Tried to write to ROM! " + addr);
    }

    if (addr >= 0xE000 && addr <= 0xFDFF) {
        // ERAM support:
        addr -= 0x2000;
    }

    // std::cout << "[W] 0x" << std::hex << (int) addr << " <- " << (int) value << std::endl;
    address_space[addr] = value;
}

uint8_t CPU::add_byte_with_overflow(uint8_t value, bool add_carry) {
    uint8_t current_value = registers.a;
    uint8_t result = current_value + value + (add_carry && registers.flags.carry);

    bool half_carry = (value & 0xF) + (current_value & 0xF) > 0xF;
    bool carry = result < value || result < current_value;
    set_all_flags(result == 0, false, half_carry, carry);
    return result;
}

uint8_t CPU::sub_byte_with_overflow(uint8_t value, bool sub_carry) {
    uint8_t current_value = registers.a;
    uint8_t result = current_value - value - (sub_carry && registers.flags.carry);

    bool half_carry = (value & 0xF) + (current_value & 0xF) > 0xF;
    bool carry = current_value < value;
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
    set_address_space_byte(--registers.sp, pc_msb);
    set_address_space_byte(--registers.sp, pc_lsb);
    registers.pc = address;
}

void CPU::return_function() {
    uint8_t lsb = get_address_space_byte(registers.sp++);
    uint8_t msb = get_address_space_byte(registers.sp++);
    uint16_t addr = (((uint16_t) msb) << 8) | lsb;
    registers.pc = addr;
}

void CPU::reset() {
    registers.clear();
    memset(address_space + 0x8000, 0, 0xA000-0x8000); // Clear VRAM
}

void CPU::execute() {

    uint8_t opcode = get_address_space_byte(registers.pc++);
    std::cout << "Executing at 0x" << std::hex << (int) (registers.pc - 1) << " opcode 0x" << std::hex << (int) opcode << std::endl;

    switch (opcode) {
    case 0x00: {
        // NOP
        break;
    }

    case 0x01:
    case 0x11:
    case 0x21:
    case 0x31: {
        // LD WORD REGISTER FROM IMMEDIATE
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);

        uint8_t lsb = get_address_space_byte(registers.pc++);
        uint8_t msb = get_address_space_byte(registers.pc++);
        uint16_t result = (((uint16_t) msb) << 8) + lsb;

        set_register_word(target, result);
        break;
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

        uint8_t value = get_register_byte(ByteRegister::A);
        uint16_t addr = get_register_word(target);
        set_register_word(target, addr + auto_increment);
        set_address_space_byte(addr, value);
        break;
    }

    case 0x03:
    case 0x13:
    case 0x23:
    case 0x33: {
        // INC WORD REGISTER: 0b00xx0011
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);
        uint16_t value = get_register_word(target) + 1;
        set_register_word(target, value);
        break;
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
        uint8_t value = get_register_byte(target);
        uint8_t result = value + 1;

        registers.flags.zero = result == 0;
        registers.flags.subtraction = false;
        registers.flags.half_carry = (value & 0xF) == 0xF;
        set_register_byte(target, result);
        break;
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
        uint8_t value = get_register_byte(target);
        uint8_t result = value - 1;

        registers.flags.zero = result == 0;
        registers.flags.subtraction = true;
        registers.flags.half_carry = (value & 0xF) == 0;
        set_register_byte(target, result);
        break;
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
        uint8_t value = get_address_space_byte(registers.pc++);
        set_register_byte(target, value);
        break;
    }

    case 0x07: {
        // ROTATE LEFT ACCUMULATOR (THROUGH CARRY): 0b00000111
        uint8_t value = get_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b10000000) != 0;
        value <<= 1;
        value |= registers.flags.carry;

        set_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        break;
    }

    case 0x08: {
        // LD TO WORD (INDIRECT) IMMEDIATE VALUE FROM REGISTER SP: 0b00001000
        uint8_t addr_lsb = get_address_space_byte(registers.pc++);
        uint8_t addr_msb = get_address_space_byte(registers.pc++);
        uint16_t addr = (((uint16_t) addr_msb) << 8) + addr_lsb;

        uint8_t sp_lsb = registers.sp & 0xFF;
        uint8_t sp_msb = (registers.sp >> 8) & 0xFF;

        set_address_space_byte(addr, sp_lsb);
        set_address_space_byte(addr + 1, sp_msb);
        break;
    }

    case 0x09:
    case 0x19:
    case 0x29:
    case 0x39: {
        // ADD WORD: 0b00xx1001
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00110000) >> 4);
        uint16_t current_value = get_register_word(WordRegister::HL);
        uint16_t value = get_register_word(target);

        uint16_t result = current_value + value;
        set_register_word(WordRegister::HL, value);
        registers.flags.subtraction = false;
        registers.flags.half_carry = current_value <= 0xFFF && result > 0xFFF; // TODO: is this correct?
        registers.flags.carry = result < value || result < current_value;
        break;
    }

    case 0x0A:
    case 0x1A:
    case 0x2A:
    case 0x3A: {
        // LD TO BYTE REGISTER A FROM (INDIRECT) WORD REGISTER: 0b000xx110
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00011000) >> 3);
        uint16_t addr = get_register_word(target);
        uint16_t value = get_address_space_byte(addr);
        set_register_byte(ByteRegister::A, value);
        break;
    }

    case 0x0B:
    case 0x1B:
    case 0x2B:
    case 0x3B: {
        // DEC WORD REGISTER: 0b00xx0011
        WordRegister::WordRegister target = (WordRegister::WordRegister) ((opcode & 0b00011000) >> 3);
        uint16_t value = get_register_word(target) - 1;
        set_register_word(target, value);
        break;
    }

    case 0x0F: {
        // ROTATE RIGHT ACCUMULATOR THROUGH CARRY: 0b00001111
        uint8_t value = get_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b00000001) != 0;
        value >>= 1;
        value |= (registers.flags.carry << 7);

        set_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        break;
    }

    case 0x10: {
        // STOP: 0b00010000
        // TODO
        throw std::runtime_error("Stop!");
        break;
    }

    case 0x17: {
        // ROTATE LEFT ACCUMULATOR: 0b00010111
        uint8_t value = get_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b10000000) != 0;
        value <<= 1;
        value |= shifted_out_bit;

        set_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        break;
    }

    case 0x18: {
        // RELATIVE JUMP: 0b00011000
        int8_t offset = (int8_t) get_address_space_byte(registers.pc++);
        registers.pc += offset;
        break;
    }

    case 0x1F: {
        // ROTATE RIGHT ACCUMULATOR: 0b00011111
        uint8_t value = get_register_byte(ByteRegister::A);
        bool shifted_out_bit = (value & 0b00000001) != 0;
        value >>= 1;
        value |= (shifted_out_bit << 7);

        set_register_byte(ByteRegister::A, value);
        set_all_flags(false, false, false, shifted_out_bit);
        break;
    }

    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38: {
        // RELATIVE JUMP (CONDITIONAL): 0b001xx000
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        int8_t offset = (int8_t) get_address_space_byte(registers.pc++);

        if (evaluate_condition(cc)) {
            registers.pc += offset;
        }
        break;
    }

    case 0x27: {
        // DECIMAL ADJUST ACCUMULATOR: 0b00100111
        // Source: https://forums.nesdev.org/viewtopic.php?t=15944

        uint8_t value = get_register_byte(ByteRegister::A);

        if (!registers.flags.subtraction) {
            // after an addition, adjust if (half-)carry occurred or if result is out of bounds
            if (registers.flags.carry || value > 0x99) {
                value += 0x60;
                registers.flags.carry = true;
            }
            if (registers.flags.half_carry || (value & 0x0f) > 0x09) {
                value += 0x6;
            }
        } else {
            // after a subtraction, only adjust if (half-)carry occurred
            if (registers.flags.carry) {
                value -= 0x60;
            }
            if (registers.flags.half_carry) {
                value -= 0x6;
            }
        }

        registers.flags.zero = (value == 0);
        registers.flags.half_carry = false;
        break;
    }

    case 0x2F: {
        // COMPLEMENT ACCUMULATOR: 0b00111111
        uint8_t value = get_register_byte(ByteRegister::A);
        set_register_byte(ByteRegister::A, ~value);

        registers.flags.subtraction = true;
        registers.flags.half_carry = true;
        break;
    }

    case 0x37: {
        // SET CARRY FLAG: 0b00110111
        registers.flags.carry = true;
        registers.flags.subtraction = false;
        registers.flags.half_carry = false;
        break;
    }

    case 0x3F: {
        // COMPLEMENT CARRY FLAG: 0b00111111
        registers.flags.carry = !registers.flags.carry;
        registers.flags.subtraction = false;
        registers.flags.half_carry = false;
        break;
    }

    case 0x76: {
        // HALT: 0b01110110
        // TODO
        throw std::runtime_error("Halt!");
        break;
    }

    case 0x40 ... 0x75:
    case 0x77 ... 0x7F: {
        // LOAD BYTE REGISTER FROM BYTE REGISTER: 0b01xxxyyy
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00111000) >> 3);
        ByteRegister::ByteRegister source = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = get_register_byte(source);
        set_register_byte(target, value);
        break;
    }

    case 0x80 ... 0x8F: {
        // ADD: 0b1000cxxx (c = use carry)
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t value = get_register_byte(target);
        uint8_t result = add_byte_with_overflow(value, use_carry);
        set_register_byte(ByteRegister::A, result);
        break;
    }

    case 0x90 ... 0x9F: {
        // SUBTRACT: 0b1001cxxx (c = use carry)
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t value = get_register_byte(target);
        uint8_t result = sub_byte_with_overflow(value, use_carry);
        set_register_byte(ByteRegister::A, result);
        break;
    }

    case 0xA0 ... 0xA7: {
        // AND: 0b10100xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = get_register_byte(ByteRegister::A) & get_register_byte(target);

        set_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, true, false);
        break;
    }

    case 0xA8 ... 0xAF: {
        // XOR: 0b10101xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = get_register_byte(ByteRegister::A) ^ get_register_byte(target);

        set_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        break;
    }

    case 0xB0 ... 0xB7: {
        // OR: 0b10110xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = get_register_byte(ByteRegister::A) | get_register_byte(target);

        set_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        break;
    }

    case 0xB8 ... 0xBF: {
        // COMPARE WITH REGISTER A: 0b10111xxx
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) ((opcode & 0b00000111) >> 0);
        uint8_t value = get_register_byte(target);
        sub_byte_with_overflow(value, false);
        break;
    }

    case 0xC0:
    case 0xC8:
    case 0xD0:
    case 0xD8: {
        // CONDITIONAL RETURN FROM FUNC: 0b110cc000
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        if (evaluate_condition(cc)) {
            return_function();
        }
        break;
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

        uint8_t lsb = get_address_space_byte(registers.sp++);
        uint8_t msb = get_address_space_byte(registers.sp++);
        uint16_t value = (((uint16_t) msb) << 8) | lsb;
        set_register_word(target, value);
        break;
    }

    case 0xC2:
    case 0xCA:
    case 0xD2:
    case 0xDA: {
        // CONDITIONAL ABSOLUTE JUMP: 0b110cc010
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        uint8_t lsb = get_address_space_byte(registers.pc++);
        uint8_t msb = get_address_space_byte(registers.pc++);
        uint8_t addr = (((uint16_t) msb) << 8) | lsb;
        if (evaluate_condition(cc)) {
            registers.pc = addr;
        }
    }

    case 0xC3: {
        // UNCONDITIONAL ABSOLUTE JUMP: 0b11000011
        uint8_t lsb = get_address_space_byte(registers.pc++);
        uint8_t msb = get_address_space_byte(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;
        registers.pc = addr;
        break;
    }

    case 0xC4:
    case 0xCC:
    case 0xD4:
    case 0xDC: {
        // CONDITIONAL CALL: 0b110cc100
        Condition::Condition cc = (Condition::Condition) ((opcode & 0b00011000) >> 3);
        uint8_t addr_lsb = get_address_space_byte(registers.pc++);
        uint8_t addr_msb = get_address_space_byte(registers.pc++);
        uint16_t addr = (((uint16_t) addr_msb) << 8) | addr_lsb;
        if (evaluate_condition(cc)) {
            call_function(addr);
        }
        break;
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

        uint16_t value = get_register_word(target);
        uint8_t msb = (value >> 8) & 0xFF;
        uint8_t lsb = value & 0xFF;

        set_address_space_byte(--registers.sp, msb);
        set_address_space_byte(--registers.sp, lsb);
        break;
    }

    case 0xC6:
    case 0xCE: {
        // ADD IMMEDIATE: 0b1100c110 (c = use carry)
        uint8_t value = get_address_space_byte(registers.pc++);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t result = add_byte_with_overflow(value, use_carry);
        set_register_byte(ByteRegister::A, result);
        break;
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
        uint8_t addr = 0x08 * ((opcode & 0b00111000) >> 3);
        call_function(addr);
        break;
    }

    case 0xC9: {
        // UNCONDITIONAL RETURN FROM FUNC: 0b11001001
        return_function();
        break;
    }

    case 0xCB: {
        // CB: 0b11001011
        uint8_t second_opcode = get_address_space_byte(registers.pc++);
        ByteRegister::ByteRegister target = (ByteRegister::ByteRegister) (second_opcode & 0b00000111);

        switch (second_opcode) {
        case 0x00 ... 0x07: {
            // ROTATE REGISTER LEFT THROUGH CARRY
            uint8_t value = get_register_byte(target);
            bool shifted_out_bit = (value & 0b10000000) != 0;
            value <<= 1;
            value |= registers.flags.carry;

            set_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x08 ... 0x0F: {
            // ROTATE REGISTER RIGHT THROUGH CARRY
            uint8_t value = get_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            value |= (registers.flags.carry << 7);

            set_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x10 ... 0x17: {
            // ROTATE REGISTER LEFT
            uint8_t value = get_register_byte(target);
            bool shifted_out_bit = (value & 0b10000000) != 0;
            value <<= 1;
            value |= shifted_out_bit;

            set_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x18 ... 0x1F: {
            // ROTATE REGISTER RIGHT
            uint8_t value = get_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            value |= (shifted_out_bit << 7);

            set_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x20 ... 0x27: {
            // ARITHMETIC SHIFT LEFT
            int8_t value = (int8_t) get_register_byte(target);
            bool shifted_out_bit = (value & 0b10000000) != 0;
            value <<= 1;
            set_register_byte(target, (uint8_t) value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x28 ... 0x2F: {
            // ARITHMETIC SHIFT RIGHT
            int8_t value = (int8_t) get_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            set_register_byte(target, (uint8_t) value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x30 ... 0x37: {
            // SWAP
            uint8_t value = get_register_byte(target);
            uint8_t lower_nibble = value & 0xF;
            value >>= 4;
            value |= (lower_nibble << 4);
            set_register_byte(target, value);
            set_all_flags(value == 0, false, false, false);
            break;
        }
        case 0x38 ... 0x3F: {
            // LOGICAL SHIFT RIGHT
            uint8_t value = get_register_byte(target);
            bool shifted_out_bit = (value & 0b00000001) != 0;
            value >>= 1;
            set_register_byte(target, value);
            set_all_flags(value == 0, false, false, shifted_out_bit);
            break;
        }
        case 0x40 ... 0x7F: {
            // TEST BIT IN REGISTER: 0b01bbbxxx (b = bit)
            uint8_t bit = (second_opcode & 0b00111000) >> 3;
            uint8_t mask = (1 << bit);
            uint8_t value = get_register_byte(target) & mask;
            registers.flags.zero = value == 0;
            registers.flags.subtraction = false;
            registers.flags.half_carry = 1;
            break;
        }
        case 0x80 ... 0xBF: {
            // RESET BIT IN REGISTER 0b10bbbxxx (b = bit)
            uint8_t bit = (second_opcode & 0b00111000) >> 3;
            uint8_t mask = ~(1 << bit);
            uint8_t value = get_register_byte(target) & mask;
            set_register_byte(target, value);
            break;
        }
        case 0xC0 ... 0xFF: {
            // SET BIT IN REGISTER: 0b11bbbxxx (b = bit)
            uint8_t bit = (second_opcode & 0b00111000) >> 3;
            uint8_t mask = (1 << bit);
            uint8_t value = get_register_byte(target) | mask;
            set_register_byte(target, value);
            break;
        }
        }
        break;
    }

    case 0xCD: {
        // UNCONDITIOANL CALL: 0b11001101
        uint8_t addr_lsb = get_address_space_byte(registers.pc++);
        uint8_t addr_msb = get_address_space_byte(registers.pc++);
        uint8_t addr = (((uint16_t) addr_msb) << 8) | addr_lsb;
        call_function(addr);
        break;
    }

    case 0xD6:
    case 0xDE: {
        // SUBTRACT IMMEDIATE: 0b1101c110 (c = use carry)
        uint8_t value = get_address_space_byte(registers.pc++);
        bool use_carry = (opcode & 0b00001000) != 0;
        uint8_t result = sub_byte_with_overflow(value, use_carry);
        set_register_byte(ByteRegister::A, result);
        break;
    }

    case 0xD9: {
        // RETURN FROM INTERRUPT
        return_function();
        set_address_space_byte(IME_ADDR, 1);
        break;
    }

    case 0xE0: {
        // LD (0xFF00+IMMEDIATE) FROM ACCUMULATOR: 0b11100000
        uint8_t offset = get_address_space_byte(registers.pc++);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = get_register_byte(ByteRegister::A);
        set_address_space_byte(addr, value);
        break;
    }

    case 0xE2: {
        // LD (0xFF00+C) FROM ACCUMULATOR: 0b11100010
        uint8_t offset = get_register_byte(ByteRegister::C);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = get_register_byte(ByteRegister::A);
        set_address_space_byte(addr, value);
    }

    case 0xE6: {
        // AND IMMEDIATE: 0b11100110
        uint8_t value = get_address_space_byte(registers.pc++) & get_register_byte(ByteRegister::A);
        set_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, true, false);
        break;
    }

    case 0xE8: {
        // ADD SP,e
        // TODO
        break;
    }

    case 0xE9: {
        // UNCONDITIONAL JUMP TO HL: 0b11101001
        registers.pc = get_register_word(WordRegister::HL);
        break;
    }

    case 0xEA: {
        // LOAD ACCUMULATOR TO IMMEDIATE ADDR: 0b11101010
        uint8_t lsb = get_address_space_byte(registers.pc++);
        uint8_t msb = get_address_space_byte(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;

        uint8_t value = get_register_byte(ByteRegister::A);
        set_address_space_byte(addr, value);
        break;
    }

    case 0xEE: {
        // XOR IMMEDIATE: 0b11101110
        uint8_t value = get_address_space_byte(registers.pc++) ^ get_register_byte(ByteRegister::A);
        set_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        break;
    }

    case 0xF0: {
        // LOAD ACCUMULATOR FROM (0xFF00+IMMEDIATE): 0b11110000
        uint8_t offset = get_address_space_byte(registers.pc++);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = get_address_space_byte(addr);
        set_register_byte(ByteRegister::A, value);
        break;
    }

    case 0xF2: {
        // LOAD ACCUMULATOR FROM (0xFF00+C): 0b11110010
        uint8_t offset = get_register_byte(ByteRegister::C);
        uint16_t addr = 0xFF00 + offset;
        uint8_t value = get_address_space_byte(addr);
        set_register_byte(ByteRegister::A, value);
        break;
    }

    case 0xF3: {
        // DISABLE INTERRUPTS: 0b11110011
        set_address_space_byte(IME_ADDR, 0);
        break;
    }

    case 0xF6: {
        // OR IMMEDIATE: 0b11110110
        uint8_t value = get_address_space_byte(registers.pc++) | get_register_byte(ByteRegister::A);
        set_register_byte(ByteRegister::A, value);
        set_all_flags(value == 0, false, false, false);
        break;
    }

    case 0xF8: {
        // LOAD HL FROM (SP+offset) WORD
        // TODO verify that this is correct
        int8_t offset = (int8_t) get_address_space_byte(registers.pc++);
        uint16_t addr = get_register_word(WordRegister::SP) + offset;

        uint8_t lsb = get_address_space_byte(addr);
        uint8_t msb = get_address_space_byte(addr + 1);
        uint16_t value = (((uint16_t) msb) << 8) | lsb;

        set_register_word(WordRegister::HL, value);
        break;
    }

    case 0xF9: {
        // LOAD SP FROM HL WORD
        // TODO verify that this is correct
        uint16_t value = get_register_word(WordRegister::HL);
        set_register_word(WordRegister::SP, value);
        break;
    }

    case 0xFA: {
        // LOAD ACCUMULATOR ABSOLUTE: 0b11111010
        uint8_t lsb = get_address_space_byte(registers.pc++);
        uint8_t msb = get_address_space_byte(registers.pc++);
        uint16_t addr = (((uint16_t) msb) << 8) | lsb;
        uint8_t value = get_address_space_byte(addr);
        set_register_byte(ByteRegister::A, value);
        break;
    }

    case 0xFB: {
        // ENABLE INTERRUPTS: 0b11111011
        set_address_space_byte(IME_ADDR, 1);// TODO: this should enabled after the next cycle
        break;
    }

    case 0xFE: {
        // COMPARE IMMEDIATE WITH ACCUMULATOR: 0b11111110
        uint8_t value = get_address_space_byte(registers.pc++);
        sub_byte_with_overflow(value, false);
        break;
    }

    default: {
        throw std::invalid_argument("Unknown opcode " + opcode);
    }

    registers.pc++;
    }
}